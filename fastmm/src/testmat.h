// Test-matrix generators and a double-double reference for sampled entries.
#pragma once
#include "common.h"

// Input classes used in every accuracy experiment.
//  0 unif   : entries uniform in [-1,1)
//  1 pos    : entries uniform in [0,1)
//  2 rowcol : A = D1*U, B = U*D2, D diagonal 2^e, e uniform integer in [-r,r]  (outer scaling)
//  3 inner  : A = U*D,  B = D^-1*U (scaling along the inner dimension; rows of A span 2^+-r)
//  4 randexp: each entry u*2^e with e uniform integer in [-r,r], independently (wide range everywhere)
//  5 cancel : A = [U U], B = [V; -V + 2^-30 W]  (|C| ~ 2^-30 |A||B|: heavy cancellation)
static const char *testmat_name[] = {"unif", "pos", "rowcol", "inner", "randexp", "cancel"};
#define TESTMAT_COUNT 6

static inline int rnd_exp(uint64_t *s, int r) { return r ? (int)(rng_next(s) % (2 * r + 1)) - r : 0; }

// fills A (m x k) and B (k x n), column-major, tightly packed
static void testmat_fill(int type, int r, size_t m, size_t k, size_t n, double *A, double *B, uint64_t seed) {
  fill_uniform(A, m, k, m, seed * 2 + 1);
  fill_uniform(B, k, n, k, seed * 2 + 2);
  uint64_t s = seed * 7919 + 17;
  if (type == 1) {
    #pragma omp parallel for
    for (size_t j = 0; j < k; j++) for (size_t i = 0; i < m; i++) A[i + j * m] = 0.5 * (A[i + j * m] + 1.0);
    #pragma omp parallel for
    for (size_t j = 0; j < n; j++) for (size_t i = 0; i < k; i++) B[i + j * k] = 0.5 * (B[i + j * k] + 1.0);
  } else if (type == 5) {
    // A = [U U], B = [V; -V + 2^-30 W]: C = 2^-30 U W, while |A||B| ~ 2|U||V| (heavy cancellation)
    size_t h = k / 2;
    #pragma omp parallel for
    for (size_t p = 0; p < h; p++) for (size_t i = 0; i < m; i++) A[i + (h + p) * m] = A[i + p * m];
    #pragma omp parallel for
    for (size_t j = 0; j < n; j++)
      for (size_t p = 0; p < h; p++) B[h + p + j * k] = -B[p + j * k] + ldexp(B[h + p + j * k], -30);
  } else if (type == 2) {
    int *e1 = malloc(m * sizeof(int)), *e2 = malloc(n * sizeof(int));
    for (size_t i = 0; i < m; i++) e1[i] = rnd_exp(&s, r);
    for (size_t j = 0; j < n; j++) e2[j] = rnd_exp(&s, r);
    #pragma omp parallel for
    for (size_t j = 0; j < k; j++) for (size_t i = 0; i < m; i++) A[i + j * m] = ldexp(A[i + j * m], e1[i]);
    #pragma omp parallel for
    for (size_t j = 0; j < n; j++) for (size_t i = 0; i < k; i++) B[i + j * k] = ldexp(B[i + j * k], e2[j]);
    free(e1); free(e2);
  } else if (type == 3) {
    int *e = malloc(k * sizeof(int));
    for (size_t p = 0; p < k; p++) e[p] = rnd_exp(&s, r);
    #pragma omp parallel for
    for (size_t p = 0; p < k; p++) for (size_t i = 0; i < m; i++) A[i + p * m] = ldexp(A[i + p * m], e[p]);
    #pragma omp parallel for
    for (size_t j = 0; j < n; j++) for (size_t p = 0; p < k; p++) B[p + j * k] = ldexp(B[p + j * k], -e[p]);
    free(e);
  } else if (type == 4) {
    #pragma omp parallel for
    for (size_t j = 0; j < k; j++) {
      uint64_t t = seed * 31 + j * 1000003 + 5;
      for (size_t i = 0; i < m; i++) A[i + j * m] = ldexp(A[i + j * m], rnd_exp(&t, r));
    }
    #pragma omp parallel for
    for (size_t j = 0; j < n; j++) {
      uint64_t t = seed * 37 + j * 1000033 + 7;
      for (size_t i = 0; i < k; i++) B[i + j * k] = ldexp(B[i + j * k], rnd_exp(&t, r));
    }
  }
}

// double-double accumulation of sum_p A[i,p]*B[p,j]; also returns sum |A||B| in *absdot
static inline void dd_add(double *hi, double *lo, double a) {
  double s = *hi + a, bb = s - *hi, e = (*hi - (s - bb)) + (a - bb);
  *hi = s; *lo += e;
}
static double ref_entry(size_t m, size_t k, const double *A, const double *B, size_t i, size_t j, double *lo_out,
                        double *absdot) {
  double hi = 0, lo = 0, ab = 0;
  for (size_t p = 0; p < k; p++) {
    double a = A[i + p * m], b = B[p + j * k];
    double pr = a * b, pe = fma(a, b, -pr);
    dd_add(&hi, &lo, pr);
    lo += pe;
    ab += fabs(pr);
  }
  double s = hi + lo;
  *lo_out = lo - (s - hi);
  *absdot = ab;
  return s;
}

// Reference for all entries at once, with the same arithmetic and summation order as ref_entry
// (bitwise identical results), blocked so that it vectorises over i and reuses the A block.
typedef struct { double *r, *lo, *ab; } refall_t;
static refall_t *g_ref;  // when set, err_sampled(ns = 0) reads the reference from here
static refall_t *ref_full(size_t m, size_t k, size_t n, const double *A, const double *B) {
  refall_t *R = malloc(sizeof *R);
  R->r = malloc(m * n * 8); R->lo = malloc(m * n * 8); R->ab = malloc(m * n * 8);
  const size_t IB = 256, JB = 16;
  size_t ni = (m + IB - 1) / IB, nj = (n + JB - 1) / JB;
  #pragma omp parallel for schedule(dynamic, 1)
  for (size_t t = 0; t < ni * nj; t++) {
    size_t i0 = (t % ni) * IB, j0 = (t / ni) * JB;
    size_t ilen = m - i0 < IB ? m - i0 : IB, jlen = n - j0 < JB ? n - j0 : JB;
    double hi[16][256], lo[16][256], ab[16][256];
    for (size_t jj = 0; jj < jlen; jj++)
      for (size_t i = 0; i < ilen; i++) hi[jj][i] = lo[jj][i] = ab[jj][i] = 0;
    for (size_t p = 0; p < k; p++) {
      const double *a = A + i0 + p * m;
      for (size_t jj = 0; jj < jlen; jj++) {
        double b = B[p + (j0 + jj) * k];
        double *h = hi[jj], *l = lo[jj], *x = ab[jj];
        #pragma omp simd
        for (size_t i = 0; i < ilen; i++) {
          double pr = a[i] * b, pe = fma(a[i], b, -pr);
          double sm = h[i] + pr, bb = sm - h[i], e = (h[i] - (sm - bb)) + (pr - bb);
          h[i] = sm;
          l[i] = (l[i] + e) + pe;
          x[i] += fabs(pr);
        }
      }
    }
    for (size_t jj = 0; jj < jlen; jj++)
      for (size_t i = 0; i < ilen; i++) {
        size_t o = i0 + i + (j0 + jj) * m;
        double sm = hi[jj][i] + lo[jj][i];
        R->r[o] = sm; R->lo[o] = lo[jj][i] - (sm - hi[jj][i]); R->ab[o] = ab[jj][i];
      }
  }
  return R;
}

typedef struct { double max_cw, med_cw, max_rel, med_rel, nrm; } errstats;

// Error of C against the reference on `ns` sampled entries (all entries if ns == 0).
// cw  = |C - R| / (|A||B|)   componentwise (the quantity classical GEMM bounds by ~k*u)
// rel = |C - R| / |R|        (only over entries with |R| > 0)
// nrm = max |C - R| / (max_i ||a_i||_2 * max_j ||b_j||_2)   (normwise-style)
static errstats err_sampled(size_t m, size_t k, size_t n, const double *A, const double *B, const double *C,
                            size_t ns, uint64_t seed) {
  size_t total = ns ? ns : m * n;
  double *cw = malloc(total * sizeof(double)), *rel = malloc(total * sizeof(double)), *ae = malloc(total * sizeof(double));
  size_t nrel = 0;
  #pragma omp parallel for schedule(dynamic, 16) reduction(+ : nrel)
  for (size_t t = 0; t < total; t++) {
    size_t i, j;
    if (ns) {
      uint64_t s = seed * 1000003 + t * 7777 + 1;
      rng_next(&s);
      i = rng_next(&s) % m; j = rng_next(&s) % n;
    } else { i = t % m; j = t / m; }
    double lo, ab, r;
    if (!ns && g_ref) { r = g_ref->r[i + j * m]; lo = g_ref->lo[i + j * m]; ab = g_ref->ab[i + j * m]; }
    else r = ref_entry(m, k, A, B, i, j, &lo, &ab);
    double d = fabs((C[i + j * m] - r) - lo);
    ae[t] = d;
    cw[t] = ab > 0 ? d / ab : 0;
    rel[t] = r != 0 ? d / fabs(r) : -1;
  }
  double na = 0, nb = 0;
  for (size_t i = 0; i < m; i++) { double s = 0; for (size_t p = 0; p < k; p++) s += A[i + p * m] * A[i + p * m]; if (s > na) na = s; }
  for (size_t j = 0; j < n; j++) { double s = 0; for (size_t p = 0; p < k; p++) s += B[p + j * k] * B[p + j * k]; if (s > nb) nb = s; }
  errstats e;
  double mae = 0;
  for (size_t t = 0; t < total; t++) if (ae[t] > mae) mae = ae[t];
  e.nrm = mae / (sqrt(na) * sqrt(nb));
  size_t w = 0;
  for (size_t t = 0; t < total; t++) if (rel[t] >= 0) rel[w++] = rel[t];
  nrel = w;
  e.max_cw = 0; for (size_t t = 0; t < total; t++) if (cw[t] > e.max_cw) e.max_cw = cw[t];
  e.med_cw = median(cw, (int)total);
  e.max_rel = 0; for (size_t t = 0; t < nrel; t++) if (rel[t] > e.max_rel) e.max_rel = rel[t];
  e.med_rel = nrel ? median(rel, (int)nrel) : 0;
  free(cw); free(rel); free(ae);
  return e;
}
