#pragma once
#include "common.h"

// Strassen-Winograd, C = A*B, `depth` recursion levels then BLAS dgemm.
size_t sw_workspace(size_t m, size_t k, size_t n, int depth);
void sw_dgemm(int depth, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B,
              size_t ldb, double *C, size_t ldc, double *work);
