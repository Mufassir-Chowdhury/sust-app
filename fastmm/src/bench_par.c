// Is MKL's multithreaded DGEMM efficient at medium sizes? Compare one 4-thread dgemm with
// T concurrent single-threaded dgemms (each on its own matrices), same n.
#include <stdio.h>
#include <omp.h>
#include "blas.h"
#include "common.h"
int main(int argc, char **argv) {
  int T = omp_get_max_threads();
  for (int a = 1; a < argc; a++) {
    size_t n = atol(argv[a]);
    double *A[8], *B[8], *C[8];
    for (int t = 0; t < T; t++) { A[t] = amalloc(n*n*8); B[t] = amalloc(n*n*8); C[t] = amalloc(n*n*8);
      fill_uniform(A[t], n, n, n, t+1); fill_uniform(B[t], n, n, n, t+11); memset(C[t], 0, n*n*8); }
    double tm[2][15];
    int R = 11;
    for (int r = -1; r < R; r++) {
      mkl_set_num_threads(T);
      double t0 = now_sec();
      dgemm_nn(n, n, n, 1.0, A[0], n, B[0], n, 0.0, C[0], n);
      double t1 = now_sec();
      #pragma omp parallel num_threads(T)
      {
        mkl_set_num_threads_local(1);
        int t = omp_get_thread_num();
        dgemm_nn(n, n, n, 1.0, A[t], n, B[t], n, 0.0, C[t], n);
        mkl_set_num_threads_local(0);
      }
      double t2 = now_sec();
      if (r >= 0) { tm[0][r] = t1 - t0; tm[1][r] = (t2 - t1) / T; }
    }
    double f = 2.0*n*n*n*1e-9;
    printf("n=%zu  MKL %dT: %.1f GF/s   %d x 1T concurrent: %.1f GF/s aggregate\n", n, T, f/median(tm[0],R), T, f/median(tm[1],R));
    for (int t = 0; t < T; t++) { free(A[t]); free(B[t]); free(C[t]); }
  }
}
