// Helpers shared by ozaki.c (plain emulation) and ozfmm.c (emulation with a fast bilinear
// scheme applied exactly in the modular domain).
#pragma once
#include <immintrin.h>
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
  double mx = 0;
  for (size_t p = 0; p < len; p++) {
    if (!isfinite(x[p * inc])) { __atomic_store_n(&oz_nonfinite_seen, 1, __ATOMIC_RELAXED); return L_ZERO_ROW; }
    mx = fmax(mx, fabs(ldexp(x[p * inc], sgn * (int)ek[p])));
  }
  if (mx == 0 || !isfinite(mx)) return L_ZERO_ROW;
  int em;
  frexp(mx, &em);
  double ss = 0;
  for (size_t p = 0; p < len; p++) { double v = ldexp(x[p * inc], sgn * (int)ek[p] - em); ss += v * v; }
  return norm_exp(ss, em);
}

// Inner-dimension equilibration (exact powers of two): A' = A*2^ek, B' = 2^-ek*B with
// ek = round(log2(sqrt(||B_k,:|| / ||A_:,k||))).  Returns ek as doubles, zero-padded to Kp.
// Disabled (all zero) with OZ_INNER=0.
static double *oz_inner_scaling(size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B,
                                size_t ldb, size_t Kp) {
  double *ek = calloc(Kp + 8, sizeof(double));
  if (getenv("OZ_INNER") && !atoi(getenv("OZ_INNER"))) return ek;
  double *ca = calloc(k, sizeof(double)), *rb = calloc(k, sizeof(double));
  #pragma omp parallel
  {
    #pragma omp for schedule(static) nowait
    for (size_t p = 0; p < k; p++) {
      const double *c = A + p * lda;
      __m512d acc = _mm512_setzero_pd();
      size_t i = 0;
      for (; i + 8 <= m; i += 8) { __m512d d = _mm512_loadu_pd(c + i); acc = _mm512_fmadd_pd(d, d, acc); }
      if (i < m) { __m512d d = _mm512_maskz_loadu_pd((__mmask8)((1u << (m - i)) - 1), c + i); acc = _mm512_fmadd_pd(d, d, acc); }
      ca[p] = _mm512_reduce_add_pd(acc);
    }
    double *loc = calloc(k, sizeof(double));
    #pragma omp for schedule(static)
    for (size_t j = 0; j < n; j++)
      for (size_t p = 0; p < k; p++) loc[p] += B[p + j * ldb] * B[p + j * ldb];
    #pragma omp critical
    for (size_t p = 0; p < k; p++) rb[p] += loc[p];
    free(loc);
  }
  for (size_t p = 0; p < k; p++)
    if (ca[p] > 0 && rb[p] > 0 && isfinite(ca[p]) && isfinite(rb[p])) ek[p] = (double)lround(0.25 * log2(rb[p] / ca[p]));
  free(ca); free(rb);
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
