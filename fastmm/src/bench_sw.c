// Benchmark + error check for Strassen-Winograd vs the linked BLAS dgemm.
// usage: bench_sw reps maxdepth n1 [n2 ...]
// For each n: times dgemm and sw at depth 1..maxdepth, interleaved (round-robin) to
// reduce the effect of machine drift; reports medians and max-norm relative error.
#include <stdio.h>
#include "fmm.h"
#include "blas.h"

static double maxrel(size_t n, const double *C, const double *R) {
  double e = 0, s = 0;
  for (size_t i = 0; i < n * n; i++) {
    double d = fabs(C[i] - R[i]);
    if (d > e) e = d;
    if (fabs(R[i]) > s) s = fabs(R[i]);
  }
  return e / s;
}

int main(int argc, char **argv) {
  if (argc < 4) { fprintf(stderr, "usage: %s reps maxdepth n...\n", argv[0]); return 1; }
  int reps = atoi(argv[1]), maxd = atoi(argv[2]);
  for (int a = 3; a < argc; a++) {
    size_t n = atol(argv[a]);
    double *A = amalloc(n * n * 8), *B = amalloc(n * n * 8), *R = amalloc(n * n * 8), *C = amalloc(n * n * 8);
    fill_uniform(A, n, n, n, 11); fill_uniform(B, n, n, n, 12);
    size_t ws = sw_workspace(n, n, n, maxd);
    double *W = amalloc((ws + 1) * 8);
    double flop = 2.0 * n * n * n;
    int r = reps;
    if (flop * r * (maxd + 1) > 6e13) r = (int)(6e13 / flop / (maxd + 1));
    if (r < 3) r = 3;
    if (r > 64) r = 64;
    double t[8][64];
    dgemm_nn(n, n, n, 1.0, A, n, B, n, 0.0, R, n);  // warm-up + reference
    for (int d = 1; d <= maxd; d++) sw_dgemm(d, n, n, n, A, n, B, n, C, n, W);  // warm-up
    for (int i = 0; i < r; i++) {
      for (int d = 0; d <= maxd; d++) {
        double t0 = now_sec();
        if (d == 0) dgemm_nn(n, n, n, 1.0, A, n, B, n, 0.0, C, n);
        else sw_dgemm(d, n, n, n, A, n, B, n, C, n, W);
        t[d][i] = now_sec() - t0;
      }
    }
    double t0m = median(t[0], r);
    printf("n=%zu reps=%d dgemm %.4fs (%.1f GF/s)", n, r, t0m, flop / t0m * 1e-9);
    for (int d = 1; d <= maxd; d++) {
      sw_dgemm(d, n, n, n, A, n, B, n, C, n, W);
      double err = maxrel(n, C, R);
      double tm = median(t[d], r);
      printf(" | d=%d %.4fs x%.3f err %.1e", d, tm, t0m / tm, err);
    }
    printf("\n");
    fflush(stdout);
    free(A); free(B); free(R); free(C); free(W);
  }
  return 0;
}
