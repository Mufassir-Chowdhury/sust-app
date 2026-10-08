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

// ---- certification (optional) ------------------------------------------------------------
// With certification on, the packers also produce, per row of A (column of B): the 1-norm of the
// rounded scaled integers (n1), an exponent ex with max|x| * 2^ex in [64, 128), a zero flag, and a
// plane q = floor(|x| * 2^ex) in [0, 127] (same AMX layout as the residues).  One more int8 GEMM
// of the q planes gives a rigorous lower bound of sum_k |a_ik||b_kj| for every entry (section
// "certificate" below).
typedef struct { double *n1; int *ex; uint8_t *zr; int8_t *q; } cert_pack;
static double g_cert_theta = 0;
static oz_cert_stats g_cert_stats;
void oz_set_certify(double theta) { g_cert_theta = theta; }
oz_cert_stats oz_get_cert_stats(void) { return g_cert_stats; }

// Pack B (k x n) as the AMX "A" operand (rows = columns j of B, 64 k per tile row), computing
// the column scaling exponent tau_j in the same pass (column j stays in L1/L2 between the two
// uses).  Out_l = packed residues for modulus l (no q factor).  Np = pad(n,32), Kp = pad(k,64).
static void pack_B_res(int s, int L, size_t k, size_t n, const double *B, size_t ldb, const double *ek, int *tau,
                       int8_t **out, size_t Np, size_t Kp, cert_pack *cp) {
  size_t nkb = Kp / 64;
  modc_t mc[OZ_SMAX];
  mod_consts(s, 0, mc);
  #pragma omp parallel
  {
    #pragma omp for schedule(dynamic, 4)
    for (size_t j = 0; j < Np; j++) {
      size_t j16 = j / 16, r = j % 16;
      const double *col = B + (j < n ? j : 0) * ldb;
      int tj = 0, xj = 0;
      if (j < n) {
        __m512d acc = _mm512_setzero_pd(), mxv = acc;
        size_t p = 0;
        for (; p + 8 <= k; p += 8) {
          __m512d d = _mm512_scalef_pd(_mm512_loadu_pd(col + p), _mm512_sub_pd(_mm512_setzero_pd(), _mm512_loadu_pd(ek + p)));
          acc = _mm512_fmadd_pd(d, d, acc);
          mxv = _mm512_max_pd(mxv, _mm512_abs_pd(d));
        }
        if (p < k) {
          __m512d d = _mm512_maskz_loadu_pd((__mmask8)((1u << (k - p)) - 1), col + p);
          d = _mm512_scalef_pd(d, _mm512_sub_pd(_mm512_setzero_pd(), _mm512_loadu_pd(ek + p)));
          acc = _mm512_fmadd_pd(d, d, acc);
          mxv = _mm512_max_pd(mxv, _mm512_abs_pd(d));
        }
        double ss = _mm512_reduce_add_pd(acc);
        tj = (isfinite(ss) && ss > 1e-280) ? L - norm_exp(ss, 0) : L - safe_norm_exp_sc(col, 1, k, ek, -1);
        tau[j] = tj;
        if (cp) {
          double mx = _mm512_reduce_max_pd(mxv);  // exact: the inner scaling keeps entries normal
          cp->zr[j] = !(mx > 0);
          xj = mx > 0 && isfinite(mx) ? 6 - ilogb(mx) : 0;
          cp->ex[j] = xj;
        }
      }
      __m512d vt = _mm512_set1_pd((double)tj), vx = _mm512_set1_pd((double)xj), nb = _mm512_setzero_pd();
      for (size_t kb = 0; kb < nkb; kb++) {
        split_t x[8];
        __m256i qv[8] = {0};
        for (int v = 0; v < 8; v++) {
          size_t k0 = kb * 64 + v * 8;
          __mmask8 mk = k0 >= k ? 0 : (k - k0 >= 8 ? 0xFF : (__mmask8)((1u << (k - k0)) - 1));
          if (j >= n) mk = 0;
          __m512d raw = _mm512_maskz_loadu_pd(mk, col + k0);
          __m512d ekv = _mm512_loadu_pd(ek + k0);  // ek is padded to Kp with zeros
          __m512d sh = _mm512_sub_pd(vt, ekv);
          __m512d d = _mm512_roundscale_pd(_mm512_scalef_pd(raw, sh), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
          x[v] = split32(d);
          if (cp) {
            nb = _mm512_add_pd(nb, _mm512_abs_pd(d));
            __m512d q = _mm512_roundscale_pd(_mm512_scalef_pd(_mm512_abs_pd(raw), _mm512_sub_pd(vx, ekv)),
                                             _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
            qv[v] = _mm512_cvtpd_epi32(q);
          }
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
        if (cp) {
          __m512i z = _mm512_setzero_si512();
          for (int v = 0; v < 8; v += 2) z = _mm512_inserti32x4(z, pack16(qv[v], qv[v + 1]), v / 2);
          _mm512_stream_si512((void *)(cp->q + amx_tile_off(j16, kb, Kp) + r * 64), z);
        }
      }
      if (cp && j < n) cp->n1[j] = _mm512_reduce_add_pd(nb);
    }
    _mm_sfence();
  }
}

// Pack A (m x k) as the AMX VNNI operand ("columns" = rows i of A): tile (i16, kb), tile row r
// holds for c = 0..15 the 4 bytes A[16 i16 + c, 64 kb + 4 r + 0..3].  Includes the q_l factor.
// Works on 16-row strips: first the 16 row norms (strip stays in L2), then the conversion.
static void pack_A_res(int s, int L, size_t m, size_t k, const double *A, size_t lda, const double *ek, int *sig,
                       int8_t **out, size_t Mp, size_t Kp, cert_pack *cp) {
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
      __m512d a0 = _mm512_setzero_pd(), a1 = a0, x0 = a0, x1 = a0;
      for (size_t p = 0; p < k; p++) {
        __m512d e = _mm512_set1_pd(ek[p]);
        __m512d d0 = _mm512_scalef_pd(_mm512_maskz_loadu_pd(m0, A + i0 + p * lda), e);
        __m512d d1 = _mm512_scalef_pd(_mm512_maskz_loadu_pd(m1, A + i0 + 8 + p * lda), e);
        a0 = _mm512_fmadd_pd(d0, d0, a0);
        a1 = _mm512_fmadd_pd(d1, d1, a1);
        if (cp) { x0 = _mm512_max_pd(x0, _mm512_abs_pd(d0)); x1 = _mm512_max_pd(x1, _mm512_abs_pd(d1)); }
      }
      double ss[16], t[16], mxa[16], tx[16];
      _mm512_storeu_pd(ss, a0); _mm512_storeu_pd(ss + 8, a1);
      _mm512_storeu_pd(mxa, x0); _mm512_storeu_pd(mxa + 8, x1);
      for (int c = 0; c < 16; c++) {
        size_t i = i0 + c;
        tx[c] = 0.0;
        if (i >= m) { t[c] = 0.0; continue; }
        int e = (isfinite(ss[c]) && ss[c] > 1e-280) ? L - norm_exp(ss[c], 0) : L - safe_norm_exp_sc(A + i, lda, k, ek, 1);
        sig[i] = e;
        t[c] = (double)e;
        if (cp) {
          int xe = mxa[c] > 0 && isfinite(mxa[c]) ? 6 - ilogb(mxa[c]) : 0;
          cp->ex[i] = xe; cp->zr[i] = !(mxa[c] > 0);
          tx[c] = (double)xe;
        }
      }
      __m512d sc0 = _mm512_loadu_pd(t), sc1 = _mm512_loadu_pd(t + 8);
      __m512d xs0 = _mm512_loadu_pd(tx), xs1 = _mm512_loadu_pd(tx + 8), na0 = _mm512_setzero_pd(), na1 = na0;
      for (size_t kb = 0; kb < nkb; kb++)
        for (int r = 0; r < 16; r++) {
          split_t x[8];  // x[2q], x[2q+1]: column 64kb+4r+q, rows i0..i0+7 / i0+8..i0+15
          __m256i qv[8] = {0};
          for (int q = 0; q < 4; q++) {
            size_t kk = kb * 64 + 4 * r + q;
            __m512d d0 = _mm512_setzero_pd(), d1 = d0;
            if (kk < k) {
              d0 = _mm512_maskz_loadu_pd(m0, A + i0 + kk * lda);
              d1 = _mm512_maskz_loadu_pd(m1, A + i0 + 8 + kk * lda);
            }
            __m512d e = _mm512_set1_pd(ek[kk]);
            __m512d r0 = _mm512_roundscale_pd(_mm512_scalef_pd(d0, _mm512_add_pd(sc0, e)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
            __m512d r1 = _mm512_roundscale_pd(_mm512_scalef_pd(d1, _mm512_add_pd(sc1, e)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
            x[2 * q] = split32(r0);
            x[2 * q + 1] = split32(r1);
            if (cp) {
              na0 = _mm512_add_pd(na0, _mm512_abs_pd(r0));
              na1 = _mm512_add_pd(na1, _mm512_abs_pd(r1));
              qv[2 * q] = _mm512_cvtpd_epi32(_mm512_roundscale_pd(_mm512_scalef_pd(_mm512_abs_pd(d0), _mm512_add_pd(xs0, e)),
                                                                  _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC));
              qv[2 * q + 1] = _mm512_cvtpd_epi32(_mm512_roundscale_pd(_mm512_scalef_pd(_mm512_abs_pd(d1), _mm512_add_pd(xs1, e)),
                                                                      _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC));
            }
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
          if (cp) {
            __m512i z = _mm512_setzero_si512();
            for (int q = 0; q < 4; q++) z = _mm512_inserti32x4(z, pack16(qv[2 * q], qv[2 * q + 1]), q);
            z = _mm512_permutexvar_epi8(perm, z);
            _mm512_stream_si512((void *)(cp->q + amx_tile_off(i16, kb, Kp) + r * 64), z);
          }
        }
      if (cp) {
        double n1[16];
        _mm512_storeu_pd(n1, na0); _mm512_storeu_pd(n1 + 8, na1);
        for (int c = 0; c < 16 && i0 + c < m; c++) cp->n1[i0 + c] = n1[c];
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

// ---- certificate -------------------------------------------------------------------------
// Scaled integers: Ai = round(a'_ik 2^sig_i), Bi = round(b'_kj 2^tau_j) with a' = a 2^e_k,
// b' = 2^-e_k b, rounding errors |dA|, |dB| <= 1/2.  The CRT recovers sum_k Ai Bi exactly (up to
// a low-part error < 2^(2L-80)), so the emulated c_ij differs from the exact product by at most
//     E_ij 2^-(sig_i+tau_j),  E_ij = n1A_i/2 + n1B_j/2 + k/4 + 2^(2L-80),
// with n1A_i = sum_k |Ai_ik|, n1B_j = sum_k |Bi_kj|; the final rounding to double adds u|c_ij|.
// The q planes give LB_ij = 2^-(exA_i+exB_j) sum_k qA qB <= sum_k |a_ik||b_kj|.  An entry is
// certified when E_ij 2^-(sig_i+tau_j) <= theta u LB_ij; then |c^ - c| <= (theta+1) u |a_i||b_j|
// (to first order).  Uncertified entries are recomputed with a compensated dot product (Dot2,
// error <= u|c| + O(k^2 u^2)|a_i||b_j|), or, if there are many, the block falls back to DGEMM.
typedef struct {
  uint8_t *F;           // flags, column-major h x w
  size_t h, w;
  const double *n1A, *n1B, *fa, *fb;  // fa_i = 2^(sig_i - exA_i), fb_j = theta 2^(tau_j - exB_j - 53)
  const uint8_t *zrA, *zrB;
  double ek0;           // k/4 + 2^(2L-80)
  size_t count;
  size_t *tcnt, ntr;    // uncertified entries per CERT_T x CERT_T tile (ntr tiles along i)
} cert_ctx;
#define CERT_T 256        // tile of C recomputed by DGEMM when it holds many uncertified entries
#define CERT_TILE_MAX 256  // ... more than this many (otherwise: Dot2 per entry, cheaper below ~400)

static void epi_cert(const int32_t *blk, size_t ld, size_t r0, size_t c0, void *vctx) {
  cert_ctx *e = vctx;
  if (c0 >= e->h) return;
  size_t cnt = e->h - c0 < 32 ? e->h - c0 : 32, flagged = 0;
  const __m512d half = _mm512_set1_pd(0.5), ek0 = _mm512_set1_pd(e->ek0);
  const __m512d safety = _mm512_set1_pd(1.0 + 0x1p-30);  // rounding in the double sums n1A, n1B and here
  __m512d n1a[4], fa[4];
  __mmask8 mk[4], zr[4];
  for (int g = 0; g < 4; g++) {
    size_t o = 8 * (size_t)g;
    mk[g] = o >= cnt ? 0 : (cnt - o >= 8 ? 0xFF : (__mmask8)((1u << (cnt - o)) - 1));
    n1a[g] = _mm512_maskz_loadu_pd(mk[g], e->n1A + c0 + o);
    fa[g] = _mm512_maskz_loadu_pd(mk[g], e->fa + c0 + o);
    __m128i z = _mm_maskz_loadu_epi8(mk[g], e->zrA + c0 + o);
    zr[g] = _mm_test_epi8_mask(z, z);
  }
  for (size_t rr = 0; rr < 32; rr++) {
    size_t j = r0 + rr;
    if (j >= e->w) break;
    uint8_t *f = e->F + c0 + j * e->h;
    __m512d nb = _mm512_set1_pd(e->n1B[j]), fb = _mm512_set1_pd(e->fb[j]);
    for (int g = 0; g < 4; g++) {
      if (!mk[g]) break;
      __m512d E = _mm512_mul_pd(_mm512_fmadd_pd(half, _mm512_add_pd(n1a[g], nb), ek0), safety);
      __m512d lb = _mm512_cvtepi32_pd(_mm256_loadu_si256((const __m256i *)(blk + rr * ld + 8 * g)));
      __m512d rhs = _mm512_mul_pd(_mm512_mul_pd(lb, fa[g]), fb);
      __mmask8 bad = _mm512_mask_cmp_pd_mask(mk[g], E, rhs, _CMP_NLE_UQ) & (__mmask8)~zr[g];
      if (e->zrB[j]) bad = 0;
      _mm_mask_storeu_epi8(f + 8 * g, mk[g], _mm_maskz_set1_epi8(bad, 1));
      flagged += (size_t)__builtin_popcount(bad);
    }
  }
  if (flagged) {
    __atomic_add_fetch(&e->count, flagged, __ATOMIC_RELAXED);
    __atomic_add_fetch(&e->tcnt[c0 / CERT_T + (r0 / CERT_T) * e->ntr], flagged, __ATOMIC_RELAXED);
  }
}

// Compensated dot product (Ogita, Rump & Oishi 2005, Dot2) in 4 interleaved chains, combined with
// TwoSum: result error <= u|x.y| + O(k^2 u^2) sum|x_p y_p|.
static double dot2(size_t k, const double *x, size_t incx, const double *y) {
  double p[4] = {0, 0, 0, 0}, sg[4] = {0, 0, 0, 0};
  size_t q = 0;
  for (; q + 4 <= k; q += 4)
    for (int c = 0; c < 4; c++) {
      double a = x[(q + c) * incx], b = y[q + c];
      double h = a * b, r = fma(a, b, -h);
      double t = p[c] + h, z = t - p[c], e = (p[c] - (t - z)) + (h - z);
      p[c] = t; sg[c] += e + r;
    }
  for (; q < k; q++) {
    double a = x[q * incx], b = y[q];
    double h = a * b, r = fma(a, b, -h);
    double t = p[0] + h, z = t - p[0], e = (p[0] - (t - z)) + (h - z);
    p[0] = t; sg[0] += e + r;
  }
  double P = p[0], S = sg[0] + sg[1] + sg[2] + sg[3];
  for (int c = 1; c < 4; c++) {
    double t = P + p[c], z = t - P, e = (P - (t - z)) + (p[c] - z);
    P = t; S += e;
  }
  return P + S;
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
  const int cert = g_cert_theta > 0;  // one extra plane: q planes of A and B, and the flags
  const int np = s + cert;
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
    double need = (double)np * ((double)amx_pad(H, AMX_COLPAD) * Kp + (double)amx_pad(W, 32) * Kp + (double)H * W);
    if (need <= budget || (H <= 256 && W <= 256)) break;
    if (W / 2 >= 4096 || (H <= 256 && W > 256)) W = amx_pad((W + 1) / 2, 32);
    else { H = amx_pad((H + 1) / 2, AMX_COLPAD); W = n; }
  }
  size_t Hp = amx_pad(H, AMX_COLPAD), Wp = amx_pad(W, 32);
  int *sig = malloc(m * sizeof(int)), *tau = malloc(n * sizeof(int));
  double t_inner0 = now_sec();
  double *ek = oz_inner_scaling(m, k, n, A, lda, B, ldb, Kp);
  double t_inner = now_sec() - t_inner0;
  int8_t *Ares[OZ_SMAX + 1], *Bres[OZ_SMAX + 1];
  uint8_t *Y[OZ_SMAX + 1];
  // Persistent workspace (kept between calls, like a BLAS library's buffer pool): re-faulting
  // gigabytes of fresh pages on every call would otherwise dominate the conversion time.
  size_t per = Hp * Kp + Wp * Kp + amx_pad(H * W, 4096);
  int8_t *ws = oz_workspace((size_t)np * per);
  for (int l = 0; l < np; l++) {
    Ares[l] = ws + (size_t)l * per;
    Bres[l] = Ares[l] + Hp * Kp;
    Y[l] = (uint8_t *)(Bres[l] + Wp * Kp);
  }
  double tconv = 0, tgemm = 0, tcrt = 0;
  double *n1A = NULL, *n1B = NULL, *fa = NULL, *fb = NULL;
  size_t *tcnt = NULL, ntr = 0, ntc = 0;
  int *exA = NULL, *exB = NULL;
  uint8_t *zrA = NULL, *zrB = NULL;
  if (cert) {
    n1A = malloc(m * sizeof(double)); fa = malloc(m * sizeof(double)); exA = malloc(m * sizeof(int)); zrA = malloc(m);
    n1B = malloc(n * sizeof(double)); fb = malloc(n * sizeof(double)); exB = malloc(n * sizeof(int)); zrB = malloc(n);
  }
  for (size_t i0 = 0; i0 < m; i0 += H) {
    size_t h = m - i0 < H ? m - i0 : H, hp = amx_pad(h, AMX_COLPAD);
    double t0 = now_sec();
    cert_pack cpA = {n1A + i0, exA + i0, zrA + i0, Ares[s]};
    pack_A_res(s, L, h, k, A + i0, lda, ek, sig + i0, Ares, hp, Kp, cert ? &cpA : NULL);
    if (cert)
      for (size_t i = i0; i < i0 + h; i++) fa[i] = ldexp(1.0, sig[i] - exA[i]);
    tconv += now_sec() - t0;
    for (size_t j0 = 0; j0 < n; j0 += W) {
      size_t w = n - j0 < W ? n - j0 : W, wp = amx_pad(w, 32);
      double t1 = now_sec();
      cert_pack cpB = {n1B + j0, exB + j0, zrB + j0, Bres[s]};
      pack_B_res(s, L, k, w, B + j0 * ldb, ldb, ek, tau + j0, Bres, wp, Kp, cert ? &cpB : NULL);
      double t2 = now_sec();
      size_t nflag = 0;
      if (cert) {
        for (size_t j = j0; j < j0 + w; j++) fb[j] = g_cert_theta * ldexp(1.0, tau[j] - exB[j] - 53);
        ntr = (h + CERT_T - 1) / CERT_T; ntc = (w + CERT_T - 1) / CERT_T;
        tcnt = realloc(tcnt, ntr * ntc * sizeof(size_t));
        memset(tcnt, 0, ntr * ntc * sizeof(size_t));
        cert_ctx cc = {Y[s], h, w, n1A + i0, n1B + j0, fa + i0, fb + j0, zrA + i0, zrB + j0,
                       0.25 * (double)k + ldexp(1.0, 2 * L - 80), 0, tcnt, ntr};
        amx_gemm_s8s8(wp, hp, Kp, Bres[s], Ares[s], epi_cert, &cc);
        nflag = cc.count;
        size_t ndg = 0;
        for (size_t t = 0; t < ntr * ntc; t++) ndg += tcnt[t] > CERT_TILE_MAX;
        g_cert_stats.entries += h * w; g_cert_stats.flagged += nflag; g_cert_stats.blocks++;
        // emulation + DGEMM on more than 1/8 of the tiles costs more than DGEMM on the whole block
        if (8 * ndg > ntr * ntc) {
          g_cert_stats.fallback_blocks++;
          dgemm_nn(h, w, k, 1.0, A + i0, lda, B + j0 * ldb, ldb, 0.0, C + i0 + j0 * ldc, ldc);
          double t3 = now_sec();
          tconv += t2 - t1; tgemm += t3 - t2;
          continue;
        }
      }
      for (int l = 0; l < s; l++) {
        epi_ctx e = {Y[l], h, h, w, (double)oz_mod[l], 1.0 / oz_mod[l]};
        amx_gemm_s8s8(wp, hp, Kp, Bres[l], Ares[l], epi_mod, &e);
      }
      double t3 = now_sec();
      reconstruct(s, h, w, Y, h, sig + i0, tau + j0, C + i0 + j0 * ldc, ldc);
      if (nflag) {  // uncertified entries: tiles with many of them by DGEMM, the others by Dot2
        const uint8_t *F = Y[s];
        size_t nrec = 0, ntd = 0;
        #pragma omp parallel for schedule(dynamic, 1) reduction(+ : nrec, ntd)
        for (size_t t = 0; t < ntr * ntc; t++) {
          if (!tcnt[t]) continue;
          size_t ti = (t % ntr) * CERT_T, tj = (t / ntr) * CERT_T;
          size_t th = h - ti < CERT_T ? h - ti : CERT_T, tw = w - tj < CERT_T ? w - tj : CERT_T;
          if (tcnt[t] > CERT_TILE_MAX) {
            int old = blas_set_threads_local(1);
            dgemm_nn(th, tw, k, 1.0, A + i0 + ti, lda, B + (j0 + tj) * ldb, ldb, 0.0, C + i0 + ti + (j0 + tj) * ldc, ldc);
            blas_set_threads_local(old);
            ntd++;
          } else {
            for (size_t j = tj; j < tj + tw; j++)
              for (size_t i = ti; i < ti + th; i++)
                if (F[i + j * h]) { C[i0 + i + (j0 + j) * ldc] = dot2(k, A + i0 + i, lda, B + (j0 + j) * ldb); nrec++; }
          }
        }
        g_cert_stats.recomputed += nrec; g_cert_stats.dgemm_tiles += ntd;
      }
      double t4 = now_sec();
      tconv += t2 - t1; tgemm += t3 - t2; tcrt += t4 - t3;
    }
  }
  if (cert) { free(n1A); free(fa); free(exA); free(zrA); free(n1B); free(fb); free(exB); free(zrB); free(tcnt); }
  free(sig); free(tau); free(ek);
  if (oz_nonfinite_seen) {  // NaN/Inf in the input: recompute with the BLAS so they propagate as in IEEE
    dgemm_nn(m, n, k, 1.0, A, lda, B, ldb, 0.0, C, ldc);
    oz_nonfinite_seen = 0;
  }
  if (getenv("OZ_VERBOSE") && atoi(getenv("OZ_VERBOSE")))
    fprintf(stderr, "oz_dgemm s=%d blocks H=%zu W=%zu workspace %.2f GB\n", s, H, W, (double)np * per / 1e9);
  if (tm) { tm->scale = t_inner; tm->convert = tconv; tm->gemm = tgemm; tm->crt = tcrt; }
}
