// Helpers shared by ozaki.c (plain emulation) and ozfmm.c (emulation with a fast bilinear
// scheme applied exactly in the modular domain).
#pragma once
#include <immintrin.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "oz_consts.h"

int8_t *oz_workspace(size_t bytes);  // persistent buffer pool, defined in ozaki.c
size_t oz_workspace_size(void);

static void *hp_alloc(size_t bytes) {
  void *p = amalloc(bytes);
  if (p) madvise(p, (bytes + (2u << 20) - 1) / (2u << 20) * (2u << 20), MADV_HUGEPAGE);
  return p;
}


#define L_ZERO_ROW 0  // zero rows/cols: any scaling works (all residues are 0)

// Largest L with (2^L + sqrt(k)/2)^2 <= P/4 (with a relative safety margin).
static int oz_bits(int s, size_t k) {
  double sq = exp2((oz_const[s].log2P - 2.0) / 2.0) * (1.0 - 1e-9) - 0.5 * sqrt((double)k);
  return (int)floor(log2(sq));
}

// Exponent e with ||x||_2 < 2^e (e = INT_MIN-ish marker for a zero vector).
static int norm_exp(double ss_scaled, int scale_exp) {
  if (ss_scaled == 0.0) return L_ZERO_ROW;
  int e;
  frexp(sqrt(ss_scaled) * (1.0 + 1e-9), &e);  // sqrt(ss) < 2^e
  return e + scale_exp;
}

// Overflow/underflow-safe version: scale by the max-abs exponent first. Zero vector -> 0.
static int safe_norm_exp(const double *x, size_t inc, size_t len) {
  double mx = 0;
  for (size_t p = 0; p < len; p++) mx = fmax(mx, fabs(x[p * inc]));
  if (mx == 0 || !isfinite(mx)) return L_ZERO_ROW;
  int em;
  frexp(mx, &em);
  double ss = 0;
  for (size_t p = 0; p < len; p++) { double v = ldexp(x[p * inc], -em); ss += v * v; }
  return norm_exp(ss, em);
}

// ---- residues -------------------------------------------------------------------------
// Inputs are exact integers x (|x| < 2^63) held in doubles.  We split x = xh*2^32 + xl
// (|xh|, |xl| <= 2^31, both exact) once, and per modulus form v = xh*c + xl*q with
// c = (2^32 q) mod p, which is exact (|v| < 2^40) and congruent to x*q.  Then
// t = floor(v/p + 1/2) is computed exactly by one FMA (for odd p, v/p + 1/2 is never closer
// than 1/(2p) to an integer, far above the 2^-19 rounding error; p = 256 is exact) and
// r = v - t*p lies in [-p/2, p/2), i.e. in int8 range.
typedef struct { __m512d hi, lo; } split_t;
static inline split_t split32(__m512d x) {
  split_t s;
  s.hi = _mm512_roundscale_pd(_mm512_mul_pd(x, _mm512_set1_pd(0x1p-32)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  s.lo = _mm512_fnmadd_pd(s.hi, _mm512_set1_pd(0x1p32), x);
  return s;
}
// 8 residues as int32 in a ymm
static inline __m256i resid_split(split_t x, double c, double q, double p, double invp) {
  __m512d v = q == 1.0 ? x.lo : _mm512_mul_pd(x.lo, _mm512_set1_pd(q));
  v = _mm512_fmadd_pd(x.hi, _mm512_set1_pd(c), v);
  __m512d t = _mm512_fmadd_pd(v, _mm512_set1_pd(invp), _mm512_set1_pd(0.5));
  t = _mm512_roundscale_pd(t, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
  v = _mm512_fnmadd_pd(t, _mm512_set1_pd(p), v);
  return _mm512_cvtpd_epi32(v);
}
static inline __m128i pack16(__m256i lo, __m256i hi) {
  return _mm512_cvtepi32_epi8(_mm512_inserti64x4(_mm512_castsi256_si512(lo), hi, 1));
}

typedef struct { double p, invp, c, q; } modc_t;
static void mod_consts(int s, int with_q, modc_t *mc) {
  for (int l = 0; l < s; l++) {
    int p = oz_mod[l], q = with_q ? oz_const[s].q[l] : 1;
    long c = (long)(((unsigned long)1 << 32) % (unsigned long)p) * q % p;
    mc[l] = (modc_t){(double)p, 1.0 / p, (double)c, (double)q};
  }
}


// Scaling-aware variant of safe_norm_exp: norm of x_p * 2^(sgn*ek[p]).
// Set when a NaN or Inf is met; the caller then falls back to the BLAS (IEEE propagation).
extern int oz_nonfinite_seen;
static int safe_norm_exp_sc(const double *x, size_t inc, size_t len, const double *ek, int sgn) {
  // Work on exponents so that x_p * 2^(sgn*ek[p]) is never formed (it may underflow or overflow
  // although the final scaled integer x_p * 2^(sgn*ek[p] + sigma) is representable).
  int emax = INT_MIN;
  for (size_t p = 0; p < len; p++) {
    double v = x[p * inc];
    if (!isfinite(v)) { __atomic_store_n(&oz_nonfinite_seen, 1, __ATOMIC_RELAXED); return L_ZERO_ROW; }
    if (v != 0) { int ex; frexp(v, &ex); ex += sgn * (int)ek[p]; if (ex > emax) emax = ex; }
  }
  if (emax == INT_MIN) return L_ZERO_ROW;
  double ss = 0;
  for (size_t p = 0; p < len; p++) {
    double v = x[p * inc];
    if (v == 0) continue;
    int ex;
    double f = frexp(v, &ex);
    double w = ldexp(f, ex + sgn * (int)ek[p] - emax);  // |w| < 1; negligible terms may flush to 0
    ss += w * w;
  }
  return norm_exp(ss, emax);
}

// Inner-dimension equilibration (exact powers of two): A' = A*2^ek, B' = 2^-ek*B with
// ek = round(log2(sqrt(max|B_k,:| / max|A_:,k|))), clamped so that no nonzero entry of A' or B'
// leaves the normal range (the scaling then loses nothing; subnormal entries are never scaled
// down).  Max-abs values are order-independent, so ek does not depend on the thread count or
// schedule (bitwise reproducible).  Returns ek as doubles, zero-padded to Kp.
// Disabled (all zero) with OZ_INNER=0.
static double *oz_inner_scaling(size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B,
                                size_t ldb, size_t Kp) {
  double *ek = calloc(Kp + 8, sizeof(double));
  if (getenv("OZ_INNER") && !atoi(getenv("OZ_INNER"))) return ek;
  // per inner index p: max |A_:,p|, min nonzero |A_:,p|, max |B_p,:|, min nonzero |B_p,:|
  double *amx = calloc(k, sizeof(double)), *amn = malloc(k * sizeof(double));
  double *bmx = calloc(k, sizeof(double)), *bmn = malloc(k * sizeof(double));
  for (size_t p = 0; p < k; p++) amn[p] = bmn[p] = INFINITY;
  const __m512d inf = _mm512_set1_pd(INFINITY);
  #pragma omp parallel
  {
    #pragma omp for schedule(static) nowait
    for (size_t p = 0; p < k; p++) {
      const double *c = A + p * lda;
      __m512d mx = _mm512_setzero_pd(), mn = inf;
      for (size_t i = 0; i < m; i += 8) {
        __mmask8 mk = m - i >= 8 ? 0xFF : (__mmask8)((1u << (m - i)) - 1);
        __m512d d = _mm512_abs_pd(_mm512_maskz_loadu_pd(mk, c + i));
        mx = _mm512_max_pd(mx, d);
        __mmask8 nz = _mm512_mask_cmp_pd_mask(mk, d, _mm512_setzero_pd(), _CMP_NEQ_OQ);
        mn = _mm512_mask_min_pd(mn, nz, mn, d);
      }
      amx[p] = _mm512_reduce_max_pd(mx);
      amn[p] = _mm512_reduce_min_pd(mn);
    }
    // rows of B in blocks of 64 indices p, each block scanned over all columns j: every value
    // is reduced by one thread in a fixed order
    #pragma omp for schedule(static)
    for (size_t p0 = 0; p0 < k; p0 += 64) {
      size_t len = k - p0 < 64 ? k - p0 : 64;
      __m512d mx[8], mn[8];
      for (int v = 0; v < 8; v++) { mx[v] = _mm512_setzero_pd(); mn[v] = inf; }
      for (size_t j = 0; j < n; j++) {
        const double *c = B + p0 + j * ldb;
        for (int v = 0; v < 8; v++) {
          size_t o = (size_t)v * 8;
          if (o >= len) break;
          __mmask8 mk = len - o >= 8 ? 0xFF : (__mmask8)((1u << (len - o)) - 1);
          __m512d d = _mm512_abs_pd(_mm512_maskz_loadu_pd(mk, c + o));
          mx[v] = _mm512_max_pd(mx[v], d);
          __mmask8 nz = _mm512_mask_cmp_pd_mask(mk, d, _mm512_setzero_pd(), _CMP_NEQ_OQ);
          mn[v] = _mm512_mask_min_pd(mn[v], nz, mn[v], d);
        }
      }
      double tx[64], tn[64];
      for (int v = 0; v < 8; v++) { _mm512_storeu_pd(tx + 8 * v, mx[v]); _mm512_storeu_pd(tn + 8 * v, mn[v]); }
      for (size_t q = 0; q < len; q++) { bmx[p0 + q] = tx[q]; bmn[p0 + q] = tn[q]; }
    }
  }
  for (size_t p = 0; p < k; p++) {
    if (!(amx[p] > 0 && bmx[p] > 0 && isfinite(amx[p]) && isfinite(bmx[p]))) continue;  // zero or NaN/Inf
    long e = lround(0.5 * (log2(bmx[p]) - log2(amx[p])));
    // exact and normal: -1022 <= ilogb(x) + e <= 1023 for every nonzero x of column p of A, and
    // -1022 <= ilogb(y) - e <= 1023 for row p of B (subnormals count as ilogb = -1022: never scaled down)
    int ea_hi = ilogb(amx[p]), ea_lo = ilogb(amn[p]) < -1022 ? -1022 : ilogb(amn[p]);
    int eb_hi = ilogb(bmx[p]), eb_lo = ilogb(bmn[p]) < -1022 ? -1022 : ilogb(bmn[p]);
    long lo = -1022 - ea_lo, hi = 1023 - ea_hi;
    if (eb_hi - 1023 > lo) lo = eb_hi - 1023;
    if (eb_lo + 1022 < hi) hi = eb_lo + 1022;
    if (e < lo) e = lo;
    if (e > hi) e = hi;
    ek[p] = (double)e;
  }
  free(amx); free(amn); free(bmx); free(bmn);
  return ek;
}

// Bytes the kernel reports as available (MemAvailable: free + reclaimable cache); falls back to free pages.
#include <unistd.h>
static double oz_mem_available(void) {
  FILE *f = fopen("/proc/meminfo", "r");
  char line[256];
  double kb = -1;
  while (f && fgets(line, sizeof line, f))
    if (sscanf(line, "MemAvailable: %lf kB", &kb) == 1) break;
  if (f) fclose(f);
  double avail = kb > 0 ? kb * 1024.0 : (double)sysconf(_SC_AVPHYS_PAGES) * (double)sysconf(_SC_PAGESIZE);
  // Respect a cgroup memory limit if there is one (v1: memory.limit_in_bytes, v2: memory.max):
  // headroom = limit - (usage - inactive file cache).
  char line2[512], cgpath[400] = "";
  int v2 = 0;
  FILE *cg = fopen("/proc/self/cgroup", "r");
  while (cg && fgets(line2, sizeof line2, cg)) {
    char *p1 = strstr(line2, ":memory:");
    if (p1) { sscanf(p1 + 8, "%399s", cgpath); v2 = 0; break; }
    if (!strncmp(line2, "0::", 3)) { sscanf(line2 + 3, "%399s", cgpath); v2 = 1; }
  }
  if (cg) fclose(cg);
  char fn[600];
  double lim = -1, use = -1, inact = 0;
  snprintf(fn, sizeof fn, v2 ? "/sys/fs/cgroup%s/memory.max" : "/sys/fs/cgroup/memory%s/memory.limit_in_bytes", cgpath);
  FILE *fm = fopen(fn, "r");
  if (fm) { if (fscanf(fm, "%lf", &lim) != 1) lim = -1; fclose(fm); }
  snprintf(fn, sizeof fn, v2 ? "/sys/fs/cgroup%s/memory.current" : "/sys/fs/cgroup/memory%s/memory.usage_in_bytes", cgpath);
  FILE *fu = fopen(fn, "r");
  if (fu) { if (fscanf(fu, "%lf", &use) != 1) use = -1; fclose(fu); }
  snprintf(fn, sizeof fn, v2 ? "/sys/fs/cgroup%s/memory.stat" : "/sys/fs/cgroup/memory%s/memory.stat", cgpath);
  FILE *fs = fopen(fn, "r");
  while (fs && fgets(line2, sizeof line2, fs)) {
    double v;
    if (sscanf(line2, v2 ? "inactive_file %lf" : "total_inactive_file %lf", &v) == 1) inact = v;
  }
  if (fs) fclose(fs);
  if (lim > 0 && lim < 1e18 && use >= 0) {
    double head = lim - (use - inact);
    if (head < avail) avail = head;
  }
  return avail;
}
