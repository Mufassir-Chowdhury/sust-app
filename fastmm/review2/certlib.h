// Independent check of the certified mode (ozc16).  ozaki.c is #included so that its calls of
// dgemm_nn can be intercepted: every entry written by a DGEMM call (tile, block or NaN fallback)
// is marked, and all other entries (certified emulation or Dot2) must satisfy
//     |c^ - c| <= (theta + 1) u sum_k |a_ik||b_kj|.
#include "blas.h"
#include <omp.h>
static void hook_dgemm(size_t m, size_t n, size_t k, double alpha, const double *A, size_t lda, const double *B,
                       size_t ldb, double beta, double *C, size_t ldc);
#define dgemm_nn hook_dgemm
#include "../src/ozaki.c"
#undef dgemm_nn
#include "testmat.h"

static double *g_Cbase = NULL;
static size_t g_ldc = 0, g_m = 0, g_n = 0;
static uint8_t *g_route = NULL;  // 1 = written by a DGEMM call
static long g_hook_calls = 0;
static void hook_dgemm(size_t m, size_t n, size_t k, double alpha, const double *A, size_t lda, const double *B,
                       size_t ldb, double beta, double *C, size_t ldc) {
  dgemm_nn(m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);
  if (g_route && g_Cbase && C >= g_Cbase && C < g_Cbase + g_ldc * g_n) {
    size_t off = (size_t)(C - g_Cbase), i0 = off % g_ldc, j0 = off / g_ldc;
    #pragma omp critical(hook)
    {
      g_hook_calls++;
      for (size_t j = j0; j < j0 + n && j < g_n; j++)
        for (size_t i = i0; i < i0 + m && i < g_m; i++) g_route[i + j * g_m] = 1;
    }
  }
}

// double-double reference with leading dimensions; a, b scaled by 2^sa, 2^sb (exact unless they
// leave the normal range; used to move underflowing / overflowing products into range)
static double refd(size_t k, const double *A, size_t lda, const double *B, size_t ldb, size_t i, size_t j, int sa,
                   int sb, double *lo_out, double *absdot) {
  double hi = 0, lo = 0, ab = 0;
  for (size_t p = 0; p < k; p++) {
    double a = ldexp(A[i + p * lda], sa), b = ldexp(B[p + j * ldb], sb);
    double pr = a * b, pe = fma(a, b, -pr);
    double s = hi + pr, bb = s - hi, e = (hi - (s - bb)) + (pr - bb);
    hi = s; lo += e + pe;
    ab += fabs(pr);
  }
  double s = hi + lo;
  *lo_out = lo - (s - hi);
  *absdot = ab;
  return s;
}

typedef struct {
  double max_emu_u, max_dg_route_u, max_dgemm_u, worst_ratio_vs_dgemm;
  size_t viol, n_dg_route, n_worse_than_dgemm, nnan_mismatch, pad_bad;
  oz_cert_stats st;
} res_t;

static double U = 0x1p-53;

// Runs ozc16 (theta) and DGEMM on (m,k,n) with lda/ldb, ldc = m + padc; checks every entry.
static res_t check(const char *name, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B,
                   size_t ldb, double theta, int sa, int sb, int verbose) {
  size_t padc = 3, ldc = m + padc;
  double *C = malloc((ldc * n + 1) * sizeof(double)), *D = malloc((m * n + 1) * sizeof(double));
  for (size_t t = 0; t < ldc * n; t++) C[t] = 12345.678;
  g_route = calloc(m * n + 1, 1);
  g_Cbase = C; g_ldc = ldc; g_m = m; g_n = n; g_hook_calls = 0;
  oz_cert_stats s0 = oz_get_cert_stats();
  oz_set_certify(theta);
  oz_dgemm(16, m, k, n, A, lda, B, ldb, C, ldc, NULL);
  oz_set_certify(0);
  oz_cert_stats s1 = oz_get_cert_stats();
  g_Cbase = NULL;
  dgemm_nn(m, n, k, 1.0, A, lda, B, ldb, 0.0, D, m);
  res_t R;
  memset(&R, 0, sizeof R);
  R.st.entries = s1.entries - s0.entries; R.st.flagged = s1.flagged - s0.flagged;
  R.st.recomputed = s1.recomputed - s0.recomputed; R.st.dgemm_tiles = s1.dgemm_tiles - s0.dgemm_tiles;
  R.st.blocks = s1.blocks - s0.blocks; R.st.fallback_blocks = s1.fallback_blocks - s0.fallback_blocks;
  double bound = (theta + 1) * U;
  size_t wi = 0, wj = 0;
  #pragma omp parallel for schedule(dynamic, 4)
  for (size_t j = 0; j < n; j++) {
    double l_emu = 0, l_dgr = 0, l_dg = 0, l_ratio = 0;
    size_t l_viol = 0, l_ndg = 0, l_worse = 0, l_nan = 0, l_pad = 0, li = 0;
    for (size_t i = m; i < ldc; i++) l_pad += C[i + j * ldc] != 12345.678;
    for (size_t i = 0; i < m; i++) {
      double c = C[i + j * ldc], d = D[i + j * m];
      if (isnan(c) || isnan(d) || isinf(c) || isinf(d)) {
        l_nan += (isnan(c) != isnan(d)) || (isinf(c) != isinf(d)) || (isinf(c) && c != d);
        continue;
      }
      double lo, ab, r = refd(k, A, lda, B, ldb, i, j, sa, sb, &lo, &ab);
      double ec = fabs((ldexp(c, sa + sb) - r) - lo), ed = fabs((ldexp(d, sa + sb) - r) - lo);
      double uc = ab > 0 ? ec / ab / U : (ec > 0 ? INFINITY : 0), ud = ab > 0 ? ed / ab / U : (ed > 0 ? INFINITY : 0);
      if (ud > l_dg) l_dg = ud;
      if (g_route[i + j * m]) {
        l_ndg++;
        if (uc > l_dgr) l_dgr = uc;
      } else {
        if (uc > l_emu) { l_emu = uc; li = i; }
        if (uc > bound / U) l_viol++;
      }
      if (ec > ed) { l_worse++; double q = ed > 0 ? ec / ed : INFINITY; if (q > l_ratio) l_ratio = q; }
    }
    #pragma omp critical(res)
    {
      if (l_emu > R.max_emu_u) { R.max_emu_u = l_emu; wi = li; wj = j; }
      if (l_dgr > R.max_dg_route_u) R.max_dg_route_u = l_dgr;
      if (l_dg > R.max_dgemm_u) R.max_dgemm_u = l_dg;
      if (l_ratio > R.worst_ratio_vs_dgemm) R.worst_ratio_vs_dgemm = l_ratio;
      R.viol += l_viol; R.n_dg_route += l_ndg; R.n_worse_than_dgemm += l_worse; R.nnan_mismatch += l_nan; R.pad_bad += l_pad;
    }
  }
  if (verbose)
    printf("%-34s m=%zu k=%zu n=%zu th=%g | emu/dot2 max %.3fu (at %zu,%zu) VIOL=%zu | dgemm-routed %zu (max %.2fu) | "
           "DGEMM max %.2fu | worse-than-DGEMM %zu | flag=%zu dot2=%zu dgt=%zu fb=%zu/%zu | nan_mism=%zu pad=%zu\n",
           name, m, k, n, theta, R.max_emu_u, wi, wj, R.viol, R.n_dg_route, R.max_dg_route_u, R.max_dgemm_u,
           R.n_worse_than_dgemm, R.st.flagged, R.st.recomputed, R.st.dgemm_tiles, R.st.fallback_blocks, R.st.blocks,
           R.nnan_mismatch, R.pad_bad);
  fflush(stdout);
  free(C); free(D); free(g_route); g_route = NULL;
  return R;
}
