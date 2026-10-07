// Thin BLAS abstraction so every algorithm links against the same library.
#pragma once
#include <stddef.h>
#ifdef USE_MKL
#include <mkl.h>
#else
#include <cblas.h>
#endif

// C = alpha*A*B + beta*C, column-major, no transposes.
static inline void dgemm_nn(size_t m, size_t n, size_t k, double alpha, const double *A, size_t lda,
                            const double *B, size_t ldb, double beta, double *C, size_t ldc) {
  if (m == 0 || n == 0) return;
  if (k == 0) {
    for (size_t j = 0; j < n; j++)
      for (size_t i = 0; i < m; i++) C[i + j * ldc] = beta == 0.0 ? 0.0 : beta * C[i + j * ldc];
    return;
  }
  cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, (int)m, (int)n, (int)k, alpha, A, (int)lda, B,
              (int)ldb, beta, C, (int)ldc);
}

static inline void blas_set_threads(int t) {
#ifdef USE_MKL
  mkl_set_num_threads(t);
#elif defined(BLIS_CBLAS)
  (void)t;  // BLIS: controlled by BLIS_NUM_THREADS
#else
  openblas_set_num_threads(t);
#endif
}

#ifdef USE_MKL
static inline int blas_set_threads_local(int t) { return mkl_set_num_threads_local(t); }
#else
static inline int blas_set_threads_local(int t) { (void)t; return 0; }
#endif
