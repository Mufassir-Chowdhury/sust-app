// Baseline: time cblas_dgemm of the linked BLAS for a list of sizes.
// usage: bench_dgemm reps n1 n2 ...   (threads via env OMP/MKL/BLIS/OPENBLAS_NUM_THREADS)
#include <stdio.h>
#ifdef USE_MKL
#include <mkl_cblas.h>
#else
#include <cblas.h>
#endif
#include "common.h"

int main(int argc, char **argv) {
  if (argc < 3) { fprintf(stderr, "usage: %s reps n...\n", argv[0]); return 1; }
  int reps = atoi(argv[1]);
  for (int a = 2; a < argc; a++) {
    size_t n = (size_t)atol(argv[a]);
    double *A = amalloc(n * n * 8), *B = amalloc(n * n * 8), *C = amalloc(n * n * 8);
    fill_uniform(A, n, n, n, 1); fill_uniform(B, n, n, n, 2); memset(C, 0, n * n * 8);
    int r = reps;
    double flop = 2.0 * n * n * n;
    if (flop * r > 4e13) r = (int)(4e13 / flop) < 3 ? 3 : (int)(4e13 / flop);
    // warm-up
    cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, n, n, n, 1.0, A, n, B, n, 0.0, C, n);
    double t[64];
    if (r > 64) r = 64;
    for (int i = 0; i < r; i++) {
      double t0 = now_sec();
      cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, n, n, n, 1.0, A, n, B, n, 0.0, C, n);
      t[i] = now_sec() - t0;
    }
    double tmin = t[0];
    for (int i = 1; i < r; i++) if (t[i] < tmin) tmin = t[i];
    double tm = median(t, r);
    printf("dgemm n=%zu reps=%d median=%.6f s min=%.6f s  %.1f GFLOP/s (median)\n", n, r, tm, tmin, flop / tm * 1e-9);
    fflush(stdout);
    free(A); free(B); free(C);
  }
  return 0;
}
