// Driver calling the Fortran BLAS symbol dgemm_ (so that an LD_PRELOADed DGEMM replacement such as
// LIBXS's Ozaki wrapper intercepts it). Reports median time and sampled error vs a double-double
// reference, with the same input classes and metrics as fmmtest.
// usage: bench_fortran reps type r n [n ...]
#include <stdio.h>
#include "common.h"
#include "testmat.h"
extern void dgemm_(const char *ta, const char *tb, const int *m, const int *n, const int *k, const double *alpha,
                   const double *A, const int *lda, const double *B, const int *ldb, const double *beta, double *C,
                   const int *ldc);
int main(int argc, char **argv) {
  int reps = atoi(argv[1]), type = atoi(argv[2]), r = atoi(argv[3]);
  for (int a = 4; a < argc; a++) {
    int n = atoi(argv[a]);
    size_t N = (size_t)n * n;
    double *A = amalloc(N * 8), *B = amalloc(N * 8), *C = amalloc(N * 8);
    testmat_fill(type, r, n, n, n, A, B, 42);
    const double one = 1, zero = 0;
    double t[64];
    if (reps > 64) reps = 64;
    dgemm_("N", "N", &n, &n, &n, &one, A, &n, B, &n, &zero, C, &n);  // warm-up
    for (int i = 0; i < reps; i++) {
      double t0 = now_sec();
      dgemm_("N", "N", &n, &n, &n, &one, A, &n, B, &n, &zero, C, &n);
      t[i] = now_sec() - t0;
    }
    double tm = median(t, reps);
    errstats e = err_sampled(n, n, n, A, B, C, n > 1000 ? 20000 : 0, 7);
    printf("fortran n=%d type=%s r=%d median=%.5f gflops=%.1f max_cw=%.3e med_cw=%.3e med_rel=%.3e\n", n,
           testmat_name[type], r, tm, 2.0 * n * n * (double)n / tm * 1e-9, e.max_cw, e.med_cw, e.med_rel);
    fflush(stdout);
    free(A); free(B); free(C);
  }
  return 0;
}
