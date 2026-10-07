// FP64 GEMM emulated with exact int8 products on Intel AMX (Ozaki scheme II style:
// Ozaki, Uchino & Imamura 2025, arXiv:2504.08009).  C = A*B, column-major doubles.
//
//  1. Row i of A is scaled by 2^sig_i and column j of B by 2^tau_j (powers of two, exact),
//     then rounded to integers Ai, Bi with ||Ai_row||_2, ||Bi_col||_2 < 2^L, so that by
//     Cauchy-Schwarz every entry of the integer product Ai*Bi lies in (-P/4, P/4).
//  2. For each modulus p_l (pairwise coprime, <= 256) the residues are int8 matrices and
//     Ai*Bi mod p_l is computed exactly by one AMX int8 GEMM with int32 accumulation.
//  3. The Chinese remainder theorem recovers the integer product, which is rescaled.
// The only rounding errors are the rounding of A and B to L-bit integers (step 1) and
// the final conversion of the reconstructed integer to double.
//
// Implementation notes: we compute C^T = B^T A^T on the tiles, so that B^T is the AMX
// "A" operand (rows of 64 k-values, read straight from B's columns) and A^T is the VNNI
// operand (4 consecutive k of 16 consecutive rows of A); tile rows of the result are
// columns of C.
#include <immintrin.h>
#include <stdio.h>
#include <sys/mman.h>
#include <omp.h>
#include "common.h"
#include "amx.h"
#include "ozaki.h"
#include "oz_consts.h"

#include "oz_internal.h"
#include <unistd.h>
#include "blas.h"
#define OZ_KMAX 131071  // (2^31 - 1) / 128^2

static int8_t *g_ws = NULL;
static size_t g_ws_size = 0;
int8_t *oz_workspace(size_t bytes) {
  if (bytes > g_ws_size) {
    free(g_ws);
    g_ws = hp_alloc(bytes);
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < bytes; i += 4096) g_ws[i] = 0;  // pre-fault
    g_ws_size = bytes;
  }
  return g_ws;
}
void oz_release(void) { free(g_ws); g_ws = NULL; g_ws_size = 0; }
size_t oz_workspace_size(void) { return g_ws_size; }
int oz_nonfinite_seen = 0;

int oz_max_moduli(void) { return 16; }  // split32 residue path needs |scaled entries| < 2^63

// Pack B (k x n) as the AMX "A" operand (rows = columns j of B, 64 k per tile row), computing
// the column scaling exponent tau_j in the same pass (column j stays in L1/L2 between the two
// uses).  Out_l = packed residues for modulus l (no q factor).  Np = pad(n,32), Kp = pad(k,64).
static void pack_B_res(int s, int L, size_t k, size_t n, const double *B, size_t ldb, const double *ek, int *tau,
                       int8_t **out, size_t Np, size_t Kp) {
  size_t nkb = Kp / 64;
  modc_t mc[OZ_SMAX];
  mod_consts(s, 0, mc);
  #pragma omp parallel
  {
    #pragma omp for schedule(dynamic, 4)
    for (size_t j = 0; j < Np; j++) {
      size_t j16 = j / 16, r = j % 16;
      const double *col = B + (j < n ? j : 0) * ldb;
      int tj = 0;
      if (j < n) {
        __m512d acc = _mm512_setzero_pd();
        size_t p = 0;
        for (; p + 8 <= k; p += 8) {
          __m512d d = _mm512_scalef_pd(_mm512_loadu_pd(col + p), _mm512_sub_pd(_mm512_setzero_pd(), _mm512_loadu_pd(ek + p)));
          acc = _mm512_fmadd_pd(d, d, acc);
        }
        if (p < k) {
          __m512d d = _mm512_maskz_loadu_pd((__mmask8)((1u << (k - p)) - 1), col + p);
          d = _mm512_scalef_pd(d, _mm512_sub_pd(_mm512_setzero_pd(), _mm512_loadu_pd(ek + p)));
          acc = _mm512_fmadd_pd(d, d, acc);
        }
        double ss = _mm512_reduce_add_pd(acc);
        tj = (isfinite(ss) && ss > 1e-280) ? L - norm_exp(ss, 0) : L - safe_norm_exp_sc(col, 1, k, ek, -1);
        tau[j] = tj;
      }
      __m512d vt = _mm512_set1_pd((double)tj);
      for (size_t kb = 0; kb < nkb; kb++) {
        split_t x[8];
        for (int v = 0; v < 8; v++) {
          size_t k0 = kb * 64 + v * 8;
          __mmask8 mk = k0 >= k ? 0 : (k - k0 >= 8 ? 0xFF : (__mmask8)((1u << (k - k0)) - 1));
          if (j >= n) mk = 0;
          __m512d d = _mm512_maskz_loadu_pd(mk, col + k0);
          __m512d sh = _mm512_sub_pd(vt, _mm512_loadu_pd(ek + k0));  // ek is padded to Kp with zeros
          d = _mm512_roundscale_pd(_mm512_scalef_pd(d, sh), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
          x[v] = split32(d);
        }
        for (int l = 0; l < s; l++) {
          int8_t *dst = out[l] + amx_tile_off(j16, kb, Kp) + r * 64;
          __m512i z = _mm512_setzero_si512();
          for (int v = 0; v < 8; v += 2) {
            __m128i bb = pack16(resid_split(x[v], mc[l].c, 1.0, mc[l].p, mc[l].invp),
                                resid_split(x[v + 1], mc[l].c, 1.0, mc[l].p, mc[l].invp));
            z = _mm512_inserti32x4(z, bb, v / 2);
          }
          _mm512_stream_si512((void *)dst, z);
        }
      }
    }
    _mm_sfence();
  }
}

// Pack A (m x k) as the AMX VNNI operand ("columns" = rows i of A): tile (i16, kb), tile row r
// holds for c = 0..15 the 4 bytes A[16 i16 + c, 64 kb + 4 r + 0..3].  Includes the q_l factor.
// Works on 16-row strips: first the 16 row norms (strip stays in L2), then the conversion.
static void pack_A_res(int s, int L, size_t m, size_t k, const double *A, size_t lda, const double *ek, int *sig,
                       int8_t **out, size_t Mp, size_t Kp) {
  size_t nkb = Kp / 64, ni16 = Mp / 16;
  modc_t mc[OZ_SMAX];
  mod_consts(s, 1, mc);
  // byte permutation: source bytes laid out as [q][c] (q = k offset 0..3, c = row 0..15),
  // destination [c][q]
  uint8_t idx[64];
  for (int c = 0; c < 16; c++)
    for (int q = 0; q < 4; q++) idx[c * 4 + q] = (uint8_t)(q * 16 + c);
  const __m512i perm = _mm512_loadu_si512(idx);
  #pragma omp parallel
  {
    #pragma omp for schedule(dynamic, 1)
    for (size_t i16 = 0; i16 < ni16; i16++) {
      size_t i0 = i16 * 16;
      __mmask8 m0 = i0 >= m ? 0 : (m - i0 >= 8 ? 0xFF : (__mmask8)((1u << (m - i0)) - 1));
      __mmask8 m1 = i0 + 8 >= m ? 0 : (m - i0 - 8 >= 8 ? 0xFF : (__mmask8)((1u << (m - i0 - 8)) - 1));
      // row norms of the strip
      __m512d a0 = _mm512_setzero_pd(), a1 = a0;
      for (size_t p = 0; p < k; p++) {
        __m512d e = _mm512_set1_pd(ek[p]);
        __m512d d0 = _mm512_scalef_pd(_mm512_maskz_loadu_pd(m0, A + i0 + p * lda), e);
        __m512d d1 = _mm512_scalef_pd(_mm512_maskz_loadu_pd(m1, A + i0 + 8 + p * lda), e);
        a0 = _mm512_fmadd_pd(d0, d0, a0);
        a1 = _mm512_fmadd_pd(d1, d1, a1);
      }
      double ss[16], t[16];
      _mm512_storeu_pd(ss, a0); _mm512_storeu_pd(ss + 8, a1);
      for (int c = 0; c < 16; c++) {
        size_t i = i0 + c;
        if (i >= m) { t[c] = 0.0; continue; }
        int e = (isfinite(ss[c]) && ss[c] > 1e-280) ? L - norm_exp(ss[c], 0) : L - safe_norm_exp_sc(A + i, lda, k, ek, 1);
        sig[i] = e;
        t[c] = (double)e;
      }
      __m512d sc0 = _mm512_loadu_pd(t), sc1 = _mm512_loadu_pd(t + 8);
      for (size_t kb = 0; kb < nkb; kb++)
        for (int r = 0; r < 16; r++) {
          split_t x[8];  // x[2q], x[2q+1]: column 64kb+4r+q, rows i0..i0+7 / i0+8..i0+15
          for (int q = 0; q < 4; q++) {
            size_t kk = kb * 64 + 4 * r + q;
            __m512d d0 = _mm512_setzero_pd(), d1 = d0;
            if (kk < k) {
              d0 = _mm512_maskz_loadu_pd(m0, A + i0 + kk * lda);
              d1 = _mm512_maskz_loadu_pd(m1, A + i0 + 8 + kk * lda);
            }
            __m512d e = _mm512_set1_pd(ek[kk]);
            x[2 * q] = split32(_mm512_roundscale_pd(_mm512_scalef_pd(d0, _mm512_add_pd(sc0, e)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
            x[2 * q + 1] = split32(_mm512_roundscale_pd(_mm512_scalef_pd(d1, _mm512_add_pd(sc1, e)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC));
          }
          for (int l = 0; l < s; l++) {
            __m512i z = _mm512_setzero_si512();
            for (int q = 0; q < 4; q++) {
              __m128i bb = pack16(resid_split(x[2 * q], mc[l].c, mc[l].q, mc[l].p, mc[l].invp),
                                  resid_split(x[2 * q + 1], mc[l].c, mc[l].q, mc[l].p, mc[l].invp));
              z = _mm512_inserti32x4(z, bb, q);
            }
            z = _mm512_permutexvar_epi8(perm, z);
            _mm512_stream_si512((void *)(out[l] + amx_tile_off(i16, kb, Kp) + r * 64), z);
          }
        }
    }
    _mm_sfence();
  }
}

// ---- epilogue: c mod p -> uint8 plane --------------------------------------------------
typedef struct {
  uint8_t *Y;      // plane for this modulus, column-major m x n, ld = ldy
  size_t ldy, m, n;
  double p, invp;
} epi_ctx;

static void epi_mod(const int32_t *blk, size_t ld, size_t r0, size_t c0, void *vctx) {
  // blk rows = columns j of C (r0 + rr), blk cols = rows i of C (c0 + cc)
  epi_ctx *e = vctx;
  const __m512d vp = _mm512_set1_pd(e->p), vi = _mm512_set1_pd(e->invp), zero = _mm512_setzero_pd();
  for (size_t rr = 0; rr < 32; rr++) {
    size_t j = r0 + rr;
    if (j >= e->n) break;
    __m128i out[4];
    for (int h = 0; h < 4; h++) {
      __m512d d = _mm512_cvtepi32_pd(_mm256_load_si256((const __m256i *)(blk + rr * ld + 8 * h)));
      __m512d t = _mm512_roundscale_pd(_mm512_mul_pd(d, vi), _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
      d = _mm512_fnmadd_pd(t, vp, d);
      d = _mm512_mask_add_pd(d, _mm512_cmp_pd_mask(d, zero, _CMP_LT_OQ), d, vp);
      d = _mm512_mask_sub_pd(d, _mm512_cmp_pd_mask(d, vp, _CMP_GE_OQ), d, vp);
      out[h] = _mm512_cvtepi64_epi8(_mm512_cvtpd_epi64(d));
    }
    __m256i y = _mm256_set_m128i(_mm_unpacklo_epi64(out[2], out[3]), _mm_unpacklo_epi64(out[0], out[1]));
    if (c0 >= e->m) return;  // block lies entirely in the zero padding (Mp is a multiple of 64)
    uint8_t *dst = e->Y + c0 + j * e->ldy;
    size_t cnt = e->m - c0 < 32 ? e->m - c0 : 32;
    if (cnt == 32) _mm256_storeu_si256((__m256i *)dst, y);
    else _mm256_mask_storeu_epi8(dst, (__mmask32)((1ull << cnt) - 1), y);
  }
}

// ---- CRT reconstruction ----------------------------------------------------------------
static void reconstruct(int s, size_t m, size_t n, uint8_t **Y, size_t ldy, const int *sig, const int *tau,
                        double *C, size_t ldc) {
  const oz_const_t *K = &oz_const[s];
  #pragma omp parallel for schedule(static)
  for (size_t j = 0; j < n; j++) {
    for (size_t i = 0; i < m; i += 8) {
      __mmask8 mk = m - i >= 8 ? 0xFF : (__mmask8)((1u << (m - i)) - 1);
      __m512d x = _mm512_setzero_pd(), sh = x, sm = x, sl = x;
      for (int l = 0; l < s; l++) {
        __m128i b = _mm_maskz_loadu_epi8(mk, Y[l] + i + j * ldy);
        __m512d y = _mm512_cvtepi64_pd(_mm512_cvtepu8_epi64(b));
        x = _mm512_fmadd_pd(y, _mm512_set1_pd(K->invp[l]), x);
        sh = _mm512_fmadd_pd(y, _mm512_set1_pd(K->wh[l]), sh);
        sm = _mm512_fmadd_pd(y, _mm512_set1_pd(K->wm[l]), sm);
        sl = _mm512_fmadd_pd(y, _mm512_set1_pd(K->wl[l]), sl);
      }
      __m512d r = _mm512_roundscale_pd(x, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
      __m512d H = _mm512_fnmadd_pd(r, _mm512_set1_pd(K->Ph), sh);
      __m512d M = _mm512_fnmadd_pd(r, _mm512_set1_pd(K->Pm), sm);
      __m512d L = _mm512_fnmadd_pd(r, _mm512_set1_pd(K->Pl), sl);
      __m512d cb = _mm512_add_pd(H, _mm512_add_pd(M, L));
      // scale by 2^-(sig_i + tau_j) in one exact step
      __m256i si = _mm256_maskz_loadu_epi32(mk, sig + i);
      __m512d e = _mm512_cvtepi32_pd(_mm256_add_epi32(si, _mm256_set1_epi32(tau[j])));
      __m512d c = _mm512_scalef_pd(cb, _mm512_sub_pd(_mm512_setzero_pd(), e));
      _mm512_mask_storeu_pd(C + i + j * ldc, mk, c);
    }
  }
}

// ---- driver ----------------------------------------------------------------------------
void oz_dgemm(int s, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B, size_t ldb,
              double *C, size_t ldc, oz_times *tm) {
  static int inited = 0;
  if (!inited) { amx_init(); inited = 1; }
  // int32 accumulation of int8 residues (|r| <= 128) is exact for k <= 131071: longer inner
  // dimensions are split and the partial products summed in double (one rounding per chunk).
  if (k > OZ_KMAX) {
    double *T = amalloc(m * n * sizeof(double));
    for (size_t p0 = 0; p0 < k; p0 += OZ_KMAX) {
      size_t kk = k - p0 < OZ_KMAX ? k - p0 : OZ_KMAX;
      oz_dgemm(s, m, kk, n, A + p0 * lda, lda, B + p0, ldb, p0 ? T : C, p0 ? m : ldc, tm);
      if (p0)
        #pragma omp parallel for
        for (size_t j = 0; j < n; j++) for (size_t i = 0; i < m; i++) C[i + j * ldc] += T[i + j * m];
    }
    free(T);
    return;
  }
  int L = oz_bits(s, k);
  if (s < 2 || s > 16 || L > 62) { fprintf(stderr, "oz_dgemm: need 2 <= s <= 16 (got %d)\n", s); exit(1); }
  oz_nonfinite_seen = 0;
  // Memory blocking: C is computed in H x W blocks; the residues of a row panel of A (H rows)
  // are reused for every column panel of B.  Workspace = s * (Hp*Kp + Wp*Kp + H*W) bytes.
  // Budget: OZ_MEM_GB if set, else 80 % of the currently available physical memory, capped at 8 GB.
  double avail = oz_mem_available() + (double)oz_workspace_size();
  double budget = getenv("OZ_MEM_GB") ? atof(getenv("OZ_MEM_GB")) * 1e9 : fmin(8.0e9, 0.8 * avail);
  // Shrink the column panel first: with a single row panel (H = m) every residue of A and of B
  // is computed exactly once; only if A's residues alone do not fit is the row panel split.
  // Column panels narrower than ~4096 make the AMX GEMM re-stream its row operand too often, so
  // W is not halved below 4096 (unless the matrix is narrower); instead the row panel is halved.
  size_t Kp = amx_pad(k, 64), H = m, W = n;
  for (;;) {
    double need = (double)s * ((double)amx_pad(H, AMX_COLPAD) * Kp + (double)amx_pad(W, 32) * Kp + (double)H * W);
    if (need <= budget || (H <= 256 && W <= 256)) break;
    if (W / 2 >= 4096 || (H <= 256 && W > 256)) W = amx_pad((W + 1) / 2, 32);
    else { H = amx_pad((H + 1) / 2, AMX_COLPAD); W = n; }
  }
  size_t Hp = amx_pad(H, AMX_COLPAD), Wp = amx_pad(W, 32);
  int *sig = malloc(m * sizeof(int)), *tau = malloc(n * sizeof(int));
  double t_inner0 = now_sec();
  double *ek = oz_inner_scaling(m, k, n, A, lda, B, ldb, Kp);
  double t_inner = now_sec() - t_inner0;
  int8_t *Ares[OZ_SMAX], *Bres[OZ_SMAX];
  uint8_t *Y[OZ_SMAX];
  // Persistent workspace (kept between calls, like a BLAS library's buffer pool): re-faulting
  // gigabytes of fresh pages on every call would otherwise dominate the conversion time.
  size_t per = Hp * Kp + Wp * Kp + amx_pad(H * W, 4096);
  int8_t *ws = oz_workspace((size_t)s * per);
  for (int l = 0; l < s; l++) {
    Ares[l] = ws + (size_t)l * per;
    Bres[l] = Ares[l] + Hp * Kp;
    Y[l] = (uint8_t *)(Bres[l] + Wp * Kp);
  }
  double tconv = 0, tgemm = 0, tcrt = 0;
  for (size_t i0 = 0; i0 < m; i0 += H) {
    size_t h = m - i0 < H ? m - i0 : H, hp = amx_pad(h, AMX_COLPAD);
    double t0 = now_sec();
    pack_A_res(s, L, h, k, A + i0, lda, ek, sig + i0, Ares, hp, Kp);
    tconv += now_sec() - t0;
    for (size_t j0 = 0; j0 < n; j0 += W) {
      size_t w = n - j0 < W ? n - j0 : W, wp = amx_pad(w, 32);
      double t1 = now_sec();
      pack_B_res(s, L, k, w, B + j0 * ldb, ldb, ek, tau + j0, Bres, wp, Kp);
      double t2 = now_sec();
      for (int l = 0; l < s; l++) {
        epi_ctx e = {Y[l], h, h, w, (double)oz_mod[l], 1.0 / oz_mod[l]};
        amx_gemm_s8s8(wp, hp, Kp, Bres[l], Ares[l], epi_mod, &e);
      }
      double t3 = now_sec();
      reconstruct(s, h, w, Y, h, sig + i0, tau + j0, C + i0 + j0 * ldc, ldc);
      double t4 = now_sec();
      tconv += t2 - t1; tgemm += t3 - t2; tcrt += t4 - t3;
    }
  }
  free(sig); free(tau); free(ek);
  if (oz_nonfinite_seen) {  // NaN/Inf in the input: recompute with the BLAS so they propagate as in IEEE
    dgemm_nn(m, n, k, 1.0, A, lda, B, ldb, 0.0, C, ldc);
    oz_nonfinite_seen = 0;
  }
  if (getenv("OZ_VERBOSE") && atoi(getenv("OZ_VERBOSE")))
    fprintf(stderr, "oz_dgemm s=%d blocks H=%zu W=%zu workspace %.2f GB\n", s, H, W, (double)s * per / 1e9);
  if (tm) { tm->scale = t_inner; tm->convert = tconv; tm->gemm = tgemm; tm->crt = tcrt; }
}
