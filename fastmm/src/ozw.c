// FP64 GEMM emulation (Ozaki scheme II, see ozaki.c) with one level of Strassen-Winograd applied
// *exactly* to every modular product, specialised and memory-blocked.
//
// Residues are kept unsigned in [0, p) so that the Winograd sums are cheap byte operations
// (64 lanes per instruction), and the products use AMX u8 x u8 (TDPBUUD, exact in int32 while
// k/2 * 255^2 < 2^31).  Because Winograd's identities hold in any commutative ring, the integer
// product recovered by the CRT -- and the final result -- is bit-identical to oz_dgemm(s, ...).
//
// Per modulus p (all arithmetic mod p):
//   S1 = A21+A22  S2 = S1-A11  S3 = A11-A21  S4 = A12-S2      (left factors: A11 A12 S4 A22 S1 S2 S3)
//   T1 = B12-B11  T2 = B22-T1  T3 = B22-B12  T4 = T2-B21      (right: B11 B21 B22 T4 T1 T2 T3)
//   P_l = left_l * right_l (7 AMX products of half size)
//   C11 = P1+P2  U2 = P1+P6  U3 = U2+P7  C21 = U3-P4  C22 = U3+P5  C12 = U2+P5+P3
// The output combination is fused into the CRT reconstruction.
#include <omp.h>
#include <unistd.h>
#include "oz_internal.h"
#include "amx.h"
#include "ozaki.h"
#include "blas.h"

#define KMAX_W 66000  // half-k products: (k/2) * 255^2 < 2^31

// ---- byte arithmetic mod p (unsigned residues in [0,p)) --------------------------------
static inline __m512i madd8(__m512i a, __m512i b, int p) {
  if (p == 256) return _mm512_add_epi8(a, b);
  __m512i t = _mm512_sub_epi8(_mm512_set1_epi8((char)p), b);  // p - b in [1, p]
  __mmask64 ge = _mm512_cmpge_epu8_mask(a, t);                 // a + b >= p
  return _mm512_mask_sub_epi8(_mm512_add_epi8(a, b), ge, a, t);
}
static inline __m512i msub8(__m512i a, __m512i b, int p) {
  __m512i d = _mm512_sub_epi8(a, b);  // a - b (+256 if a < b)
  if (p == 256) return d;
  __mmask64 lt = _mm512_cmplt_epu8_mask(a, b);
  return _mm512_mask_add_epi8(d, lt, d, _mm512_set1_epi8((char)p));  // a - b + p (mod 256)
}
// 8 unsigned residues of x*q mod p as int32 in [0,p)
static inline __m256i uresid(split_t x, double c, double q, double p, double invp) {
  __m256i r = resid_split(x, c, q, p, invp);  // [-p/2, p/2)
  return _mm256_mask_add_epi32(r, _mm256_cmplt_epi32_mask(r, _mm256_setzero_si256()), r, _mm256_set1_epi32((int)p));
}

typedef struct {
  size_t m, k, n;      // full problem
  size_t kh, kbp;      // k half and its padding to 64
} kgeom_t;

// ---- left factors (VNNI operands, columns = rows of A) for a row panel ---------------------
// Row panel: rows [r0, r0 + H) of A; halves of h2 = ceil(H/2) rows; plane dims h2p x kbp.
static void wpack_left(int s, int L, const kgeom_t *G, size_t r0, size_t H, const double *A, size_t lda,
                       const double *ek, int *sig, uint8_t **out /* [l*7 + f] */, size_t h2p) {
  size_t h2 = (H + 1) / 2, nkb = G->kbp / 64, ni16 = h2p / 16;
  modc_t mc[OZ_SMAX];
  mod_consts(s, 1, mc);
  uint8_t idx[64];
  for (int c = 0; c < 16; c++)
    for (int q = 0; q < 4; q++) idx[c * 4 + q] = (uint8_t)(q * 16 + c);
  const __m512i perm = _mm512_loadu_si512(idx);
  #pragma omp parallel
  {
    #pragma omp for schedule(dynamic, 1)
    for (size_t i16 = 0; i16 < ni16; i16++) {
      __mmask8 mk[2][2];
      __m512d sc[2][2];
      for (int bi = 0; bi < 2; bi++) {
        size_t base = r0 + bi * h2 + i16 * 16;  // global row of lane 0
        for (int h = 0; h < 2; h++) {
          __mmask8 mm = 0;
          for (int c = 0; c < 8; c++) {
            size_t il = i16 * 16 + 8 * h + c;
            if (il < h2 && bi * h2 + il < H && base + 8 * h + c < G->m) mm |= (__mmask8)(1u << c);
          }
          mk[bi][h] = mm;
        }
        __m512d a0 = _mm512_setzero_pd(), a1 = a0;
        for (size_t p = 0; p < G->k; p++) {
          __m512d e = _mm512_set1_pd(ek[p]);
          __m512d d0 = _mm512_scalef_pd(_mm512_maskz_loadu_pd(mk[bi][0], A + base + p * lda), e);
          __m512d d1 = _mm512_scalef_pd(_mm512_maskz_loadu_pd(mk[bi][1], A + base + 8 + p * lda), e);
          a0 = _mm512_fmadd_pd(d0, d0, a0);
          a1 = _mm512_fmadd_pd(d1, d1, a1);
        }
        double ss[16], t[16];
        _mm512_storeu_pd(ss, a0); _mm512_storeu_pd(ss + 8, a1);
        for (int c = 0; c < 16; c++) {
          if (!((mk[bi][c / 8] >> (c % 8)) & 1)) { t[c] = 0; continue; }
          size_t ig = base + c;
          int e = (isfinite(ss[c]) && ss[c] > 1e-280) ? L - norm_exp(ss[c], 0) : L - safe_norm_exp_sc(A + ig, lda, G->k, ek, 1);
          sig[ig] = e;
          t[c] = (double)e;
        }
        sc[bi][0] = _mm512_loadu_pd(t); sc[bi][1] = _mm512_loadu_pd(t + 8);
      }
      for (size_t kb = 0; kb < nkb; kb++)
        for (int rr = 0; rr < 16; rr++) {
          split_t x[4][4][2];  // [q][block bi*2+bk][half]
          for (int q = 0; q < 4; q++) {
            size_t kk = kb * 64 + 4 * rr + q;
            for (int bi = 0; bi < 2; bi++)
              for (int bk = 0; bk < 2; bk++) {
                size_t kg = bk * G->kh + kk;
                int ok = kk < G->kh && kg < G->k;
                __m512d e = _mm512_set1_pd(ok ? ek[kg] : 0.0);
                const double *src = A + r0 + bi * h2 + i16 * 16 + kg * lda;
                for (int h = 0; h < 2; h++) {
                  __m512d d = ok ? _mm512_maskz_loadu_pd(mk[bi][h], src + 8 * h) : _mm512_setzero_pd();
                  d = _mm512_roundscale_pd(_mm512_scalef_pd(d, _mm512_add_pd(sc[bi][h], e)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
                  x[q][bi * 2 + bk][h] = split32(d);
                }
              }
          }
          size_t off = amx_tile_off(i16, kb, G->kbp) + rr * 64;
          for (int l = 0; l < s; l++) {
            int p = oz_mod[l];
            __m512i blk[4];
            for (int b = 0; b < 4; b++) {
              __m512i w = _mm512_castsi128_si512(pack16(uresid(x[0][b][0], mc[l].c, mc[l].q, mc[l].p, mc[l].invp),
                                                        uresid(x[0][b][1], mc[l].c, mc[l].q, mc[l].p, mc[l].invp)));
              w = _mm512_inserti32x4(w, pack16(uresid(x[1][b][0], mc[l].c, mc[l].q, mc[l].p, mc[l].invp),
                                               uresid(x[1][b][1], mc[l].c, mc[l].q, mc[l].p, mc[l].invp)), 1);
              w = _mm512_inserti32x4(w, pack16(uresid(x[2][b][0], mc[l].c, mc[l].q, mc[l].p, mc[l].invp),
                                               uresid(x[2][b][1], mc[l].c, mc[l].q, mc[l].p, mc[l].invp)), 2);
              w = _mm512_inserti32x4(w, pack16(uresid(x[3][b][0], mc[l].c, mc[l].q, mc[l].p, mc[l].invp),
                                               uresid(x[3][b][1], mc[l].c, mc[l].q, mc[l].p, mc[l].invp)), 3);
              blk[b] = oz_perm_cq(perm, w);
            }
            __m512i A11 = blk[0], A12 = blk[1], A21 = blk[2], A22 = blk[3];
            __m512i S1 = madd8(A21, A22, p), S2 = msub8(S1, A11, p), S3 = msub8(A11, A21, p), S4 = msub8(A12, S2, p);
            uint8_t **o = out + l * 7;
            _mm512_stream_si512((void *)(o[0] + off), A11);
            _mm512_stream_si512((void *)(o[1] + off), A12);
            _mm512_stream_si512((void *)(o[2] + off), S4);
            _mm512_stream_si512((void *)(o[3] + off), A22);
            _mm512_stream_si512((void *)(o[4] + off), S1);
            _mm512_stream_si512((void *)(o[5] + off), S2);
            _mm512_stream_si512((void *)(o[6] + off), S3);
          }
        }
    }
    _mm_sfence();
  }
}

// ---- right factors (AMX "A" operands, rows = columns of B) for a column panel ----------------
static void wpack_right(int s, int L, const kgeom_t *G, size_t c0, size_t W, const double *B, size_t ldb,
                        const double *ek, int *tau, uint8_t **out, size_t w2p) {
  size_t w2 = (W + 1) / 2, nkb = G->kbp / 64;
  modc_t mc[OZ_SMAX];
  mod_consts(s, 0, mc);
  #pragma omp parallel
  {
    #pragma omp for schedule(dynamic, 4)
    for (size_t j = 0; j < w2p; j++) {
      size_t j16 = j / 16, rr = j % 16;
      const double *col[2];
      int valid[2];
      __m512d vt[2];
      for (int bj = 0; bj < 2; bj++) {
        size_t jg = c0 + bj * w2 + j;
        valid[bj] = j < w2 && bj * w2 + j < W && jg < G->n;
        col[bj] = valid[bj] ? B + jg * ldb : B;
        int tj = 0;
        if (valid[bj]) {
          __m512d acc = _mm512_setzero_pd();
          size_t p = 0;
          for (; p + 8 <= G->k; p += 8) {
            __m512d d = _mm512_scalef_pd(_mm512_loadu_pd(col[bj] + p), _mm512_sub_pd(_mm512_setzero_pd(), _mm512_loadu_pd(ek + p)));
            acc = _mm512_fmadd_pd(d, d, acc);
          }
          if (p < G->k) {
            __mmask8 mm = (__mmask8)((1u << (G->k - p)) - 1);
            __m512d d = _mm512_scalef_pd(_mm512_maskz_loadu_pd(mm, col[bj] + p), _mm512_sub_pd(_mm512_setzero_pd(), _mm512_loadu_pd(ek + p)));
            acc = _mm512_fmadd_pd(d, d, acc);
          }
          double ss = _mm512_reduce_add_pd(acc);
          tj = (isfinite(ss) && ss > 1e-280) ? L - norm_exp(ss, 0) : L - safe_norm_exp_sc(col[bj], 1, G->k, ek, -1);
          tau[jg] = tj;
        }
        vt[bj] = _mm512_set1_pd((double)tj);
      }
      for (size_t kb = 0; kb < nkb; kb++) {
        split_t x[4][8];  // [block bk*2+bj][v]
        for (int bk = 0; bk < 2; bk++)
          for (int bj = 0; bj < 2; bj++)
            for (int v = 0; v < 8; v++) {
              size_t kl = kb * 64 + v * 8, kg = bk * G->kh + kl;
              __mmask8 mm = 0;
              if (valid[bj])
                for (int c = 0; c < 8; c++)
                  if (kl + c < G->kh && kg + c < G->k) mm |= (__mmask8)(1u << c);
              __m512d d = mm ? _mm512_maskz_loadu_pd(mm, col[bj] + kg) : _mm512_setzero_pd();
              __m512d ev = _mm512_maskz_loadu_pd(mm, ek + kg);
              d = _mm512_roundscale_pd(_mm512_scalef_pd(d, _mm512_sub_pd(vt[bj], ev)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
              x[bk * 2 + bj][v] = split32(d);
            }
        size_t off = amx_tile_off(j16, kb, G->kbp) + rr * 64;
        for (int l = 0; l < s; l++) {
          int p = oz_mod[l];
          __m512i blk[4];
          for (int b = 0; b < 4; b++) {
            __m512i w = _mm512_castsi128_si512(pack16(uresid(x[b][0], mc[l].c, 1.0, mc[l].p, mc[l].invp),
                                                      uresid(x[b][1], mc[l].c, 1.0, mc[l].p, mc[l].invp)));
            w = _mm512_inserti32x4(w, pack16(uresid(x[b][2], mc[l].c, 1.0, mc[l].p, mc[l].invp),
                                             uresid(x[b][3], mc[l].c, 1.0, mc[l].p, mc[l].invp)), 1);
            w = _mm512_inserti32x4(w, pack16(uresid(x[b][4], mc[l].c, 1.0, mc[l].p, mc[l].invp),
                                             uresid(x[b][5], mc[l].c, 1.0, mc[l].p, mc[l].invp)), 2);
            w = _mm512_inserti32x4(w, pack16(uresid(x[b][6], mc[l].c, 1.0, mc[l].p, mc[l].invp),
                                             uresid(x[b][7], mc[l].c, 1.0, mc[l].p, mc[l].invp)), 3);
            blk[b] = w;
          }
          __m512i B11 = blk[0], B12 = blk[1], B21 = blk[2], B22 = blk[3];
          __m512i T1 = msub8(B12, B11, p), T2 = msub8(B22, T1, p), T3 = msub8(B22, B12, p), T4 = msub8(T2, B21, p);
          uint8_t **o = out + l * 7;
          _mm512_stream_si512((void *)(o[0] + off), B11);
          _mm512_stream_si512((void *)(o[1] + off), B21);
          _mm512_stream_si512((void *)(o[2] + off), B22);
          _mm512_stream_si512((void *)(o[3] + off), T4);
          _mm512_stream_si512((void *)(o[4] + off), T1);
          _mm512_stream_si512((void *)(o[5] + off), T2);
          _mm512_stream_si512((void *)(o[6] + off), T3);
        }
      }
    }
    _mm_sfence();
  }
}

// ---- epilogue: int32 product (>= 0) mod p -> byte; product planes interleaved by column ----
typedef struct { uint8_t *Y; size_t ldy, rows, cols; double p, invp; } wepi_ctx;
static void wepi(const int32_t *blk, size_t ld, size_t r0, size_t c0, void *vctx) {
  wepi_ctx *e = vctx;  // blk rows = columns j (r0 + rr), cols = rows i (c0 + cc); rows padded: in bounds
  const __m512d vp = _mm512_set1_pd(e->p), vi = _mm512_set1_pd(e->invp), zero = _mm512_setzero_pd();
  for (size_t rr = 0; rr < 32; rr++) {
    size_t j = r0 + rr;
    if (j >= e->cols) break;
    __m128i o[4];
    for (int h = 0; h < 4; h++) {
      __m512d d = _mm512_cvtepi32_pd(_mm256_load_si256((const __m256i *)(blk + rr * ld + 8 * h)));
      __m512d t = _mm512_roundscale_pd(_mm512_mul_pd(d, vi), _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
      d = _mm512_fnmadd_pd(t, vp, d);
      d = _mm512_mask_add_pd(d, _mm512_cmp_pd_mask(d, zero, _CMP_LT_OQ), d, vp);
      d = _mm512_mask_sub_pd(d, _mm512_cmp_pd_mask(d, vp, _CMP_GE_OQ), d, vp);
      o[h] = _mm512_cvtepi64_epi8(_mm512_cvtpd_epi64(d));
    }
    __m256i y = _mm256_set_m128i(_mm_unpacklo_epi64(o[2], o[3]), _mm_unpacklo_epi64(o[0], o[1]));
    _mm256_storeu_si256((__m256i *)(e->Y + c0 + j * e->ldy), y);
  }
}

// ---- reconstruction of a C block: Winograd output combination mod p, then CRT -------------
static void wreconstruct(int s, size_t r0, size_t H, size_t c0, size_t W, size_t m, size_t n, uint8_t **Yl,
                         size_t h2p, const int *sig, const int *tau, double *C, size_t ldc) {
  const oz_const_t *K = &oz_const[s];
  size_t h2 = (H + 1) / 2, w2 = (W + 1) / 2, nch = (h2 + 63) / 64, ldy = 7 * h2p;
  #pragma omp parallel
  {
    uint8_t ys[4][OZ_SMAX][64] __attribute__((aligned(64)));
    #pragma omp for schedule(dynamic, 4) collapse(2)
    for (size_t j = 0; j < w2; j++)
      for (size_t ch = 0; ch < nch; ch++) {
        size_t i0 = ch * 64;
        for (int l = 0; l < s; l++) {
          int p = oz_mod[l];
          const uint8_t *y = Yl[l] + j * ldy + i0;
          __m512i P1 = _mm512_loadu_si512(y), P2 = _mm512_loadu_si512(y + h2p), P3 = _mm512_loadu_si512(y + 2 * h2p),
                  P4 = _mm512_loadu_si512(y + 3 * h2p), P5 = _mm512_loadu_si512(y + 4 * h2p),
                  P6 = _mm512_loadu_si512(y + 5 * h2p), P7 = _mm512_loadu_si512(y + 6 * h2p);
          __m512i U2 = madd8(P1, P6, p), U3 = madd8(U2, P7, p);
          _mm512_store_si512((void *)ys[0][l], madd8(P1, P2, p));                 // C11
          _mm512_store_si512((void *)ys[1][l], madd8(madd8(U2, P5, p), P3, p));   // C12
          _mm512_store_si512((void *)ys[2][l], msub8(U3, P4, p));                 // C21
          _mm512_store_si512((void *)ys[3][l], madd8(U3, P5, p));                 // C22
        }
        for (int bi = 0; bi < 2; bi++)
          for (int bj = 0; bj < 2; bj++) {
            int o = bi * 2 + bj;
            if (bj * w2 + j >= W) continue;
            size_t jg = c0 + bj * w2 + j;
            if (jg >= n) continue;
            for (size_t i = i0; i < i0 + 64 && i < h2; i += 8) {
              if (bi * h2 + i >= H) break;
              size_t ig0 = r0 + bi * h2 + i;
              if (ig0 >= m) break;
              size_t lim = 8;
              if (h2 - i < lim) lim = h2 - i;
              if (H - (bi * h2 + i) < lim) lim = H - (bi * h2 + i);
              if (m - ig0 < lim) lim = m - ig0;
              __mmask8 mk = (__mmask8)((1u << lim) - 1);
              __m512d x = _mm512_setzero_pd(), sh = x, sm = x, sl = x;
              for (int l = 0; l < s; l++) {
                __m512d yv = _mm512_cvtepi64_pd(_mm512_cvtepu8_epi64(_mm_loadl_epi64((const __m128i *)(ys[o][l] + (i - i0)))));
                x = _mm512_fmadd_pd(yv, _mm512_set1_pd(K->invp[l]), x);
                sh = _mm512_fmadd_pd(yv, _mm512_set1_pd(K->wh[l]), sh);
                sm = _mm512_fmadd_pd(yv, _mm512_set1_pd(K->wm[l]), sm);
                sl = _mm512_fmadd_pd(yv, _mm512_set1_pd(K->wl[l]), sl);
              }
              __m512d rr = _mm512_roundscale_pd(x, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
              __m512d Hh = _mm512_fnmadd_pd(rr, _mm512_set1_pd(K->Ph), sh);
              __m512d Mm = _mm512_fnmadd_pd(rr, _mm512_set1_pd(K->Pm), sm);
              __m512d Lo = _mm512_fnmadd_pd(rr, _mm512_set1_pd(K->Pl), sl);
              __m512d cb = _mm512_add_pd(Hh, _mm512_add_pd(Mm, Lo));
              __m256i si = _mm256_maskz_loadu_epi32(mk, sig + ig0);
              __m512d e = _mm512_cvtepi32_pd(_mm256_add_epi32(si, _mm256_set1_epi32(tau[jg])));
              _mm512_mask_storeu_pd(C + ig0 + jg * ldc, mk, _mm512_scalef_pd(cb, _mm512_sub_pd(_mm512_setzero_pd(), e)));
            }
          }
      }
  }
}

void oz_dgemm_w(int s, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B, size_t ldb,
                double *C, size_t ldc, oz_times *tm) {
  static int inited = 0;
  if (!inited) { amx_init(); inited = 1; }
  if (k > KMAX_W || m < 64 || n < 64 || k < 128) { oz_dgemm(s, m, k, n, A, lda, B, ldb, C, ldc, tm); return; }
  int L = oz_bits(s, k);
  if (s < 2 || s > 16 || L > 62) { fprintf(stderr, "oz_dgemm_w: need 2 <= s <= 16\n"); exit(1); }
  oz_nonfinite_seen = 0;
  kgeom_t G = {m, k, n, (k + 1) / 2, 0};
  G.kbp = amx_pad(G.kh, 64);
  // memory blocking as in oz_dgemm, with the 7/4 factor of the Winograd planes
  double avail = oz_mem_available() + (double)oz_workspace_size();
  double budget = getenv("OZ_MEM_GB") ? atof(getenv("OZ_MEM_GB")) * 1e9 : fmin(8.0e9, 0.8 * avail);
  size_t H = m, W = n;
  for (;;) {
    double h2p = amx_pad((H + 1) / 2, AMX_COLPAD), w2p = amx_pad((W + 1) / 2, 32);
    double need = (double)s * 7.0 * (h2p * G.kbp + w2p * G.kbp + h2p * (double)((W + 1) / 2));
    if (need <= budget || (H <= 512 && W <= 512)) break;
    if (W / 2 >= 4096 || (H <= 512 && W > 512)) W = amx_pad((W + 1) / 2, 64);
    else { H = amx_pad((H + 1) / 2, 128); W = n; }
  }
  // The Winograd planes take 7/4 x the memory of the plain emulation; when that forces small blocks
  // the half-size products become inefficient and the plain emulation is faster.
  if ((H < 4096 && H < m) || (W < 4096 && W < n)) { oz_dgemm(s, m, k, n, A, lda, B, ldb, C, ldc, tm); return; }
  size_t h2p = amx_pad((H + 1) / 2, AMX_COLPAD), w2p = amx_pad((W + 1) / 2, 32);
  size_t szL = h2p * G.kbp, szR = w2p * G.kbp, szY = amx_pad(7 * h2p * ((W + 1) / 2), 4096);
  uint8_t *ws = (uint8_t *)oz_workspace((size_t)s * (7 * szL + 7 * szR + szY));
  uint8_t *Lp[OZ_SMAX * 7], *Rp[OZ_SMAX * 7], *Yl[OZ_SMAX];
  for (int l = 0; l < s; l++) {
    uint8_t *b = ws + (size_t)l * (7 * szL + 7 * szR + szY);
    for (int f = 0; f < 7; f++) { Lp[l * 7 + f] = b + f * szL; Rp[l * 7 + f] = b + 7 * szL + f * szR; }
    Yl[l] = b + 7 * szL + 7 * szR;
  }
  int *sig = malloc(m * sizeof(int)), *tau = malloc(n * sizeof(int));
  double t0 = now_sec();
  double *ek = oz_inner_scaling(m, k, n, A, lda, B, ldb, G.kbp * 2 + 64);
  double tconv = 0, tgemm = 0, tcrt = 0, tsc = now_sec() - t0;
  for (size_t r0 = 0; r0 < m; r0 += H) {
    size_t h = m - r0 < H ? m - r0 : H, hh2p = amx_pad((h + 1) / 2, AMX_COLPAD);
    double t1 = now_sec();
    wpack_left(s, L, &G, r0, h, A, lda, ek, sig, Lp, hh2p);
    tconv += now_sec() - t1;
    for (size_t c0 = 0; c0 < n; c0 += W) {
      size_t w = n - c0 < W ? n - c0 : W, ww2 = (w + 1) / 2, ww2p = amx_pad(ww2, 32);
      double t2 = now_sec();
      wpack_right(s, L, &G, c0, w, B, ldb, ek, tau, Rp, ww2p);
      double t3 = now_sec();
      for (int l = 0; l < s; l++)
        for (int f = 0; f < 7; f++) {
          wepi_ctx e = {Yl[l] + (size_t)f * hh2p, 7 * hh2p, hh2p, ww2, (double)oz_mod[l], 1.0 / oz_mod[l]};
          amx_gemm_u8u8(ww2p, hh2p, G.kbp, Rp[l * 7 + f], Lp[l * 7 + f], wepi, &e);
        }
      double t4 = now_sec();
      wreconstruct(s, r0, h, c0, w, m, n, Yl, hh2p, sig, tau, C, ldc);
      double t5 = now_sec();
      tconv += t3 - t2; tgemm += t4 - t3; tcrt += t5 - t4;
    }
  }
  free(sig); free(tau); free(ek);
  if (oz_nonfinite_seen) { oz_nonfinite_seen = 0; dgemm_nn(m, n, k, 1.0, A, lda, B, ldb, 0.0, C, ldc); }
  if (getenv("OZ_VERBOSE") && atoi(getenv("OZ_VERBOSE")))
    fprintf(stderr, "oz_dgemm_w s=%d blocks H=%zu W=%zu workspace %.2f GB\n", s, H, W,
            (double)s * (7 * szL + 7 * szR + szY) / 1e9);
  if (tm) { tm->scale = tsc; tm->convert = tconv; tm->gemm = tgemm; tm->crt = tcrt; }
}
