// Strassen-Winograd (Winograd's 7-multiplication, 15-addition variant), column-major,
// C = A*B. Memory-lean schedule after Boyer, Dumas, Pernet & Zhou (ISSAC 2009) with
// two temporaries per level; the post-additions are fused into one streaming pass and
// two of the products accumulate directly into C through the BLAS beta argument.
// Odd dimensions are handled by dynamic peeling (rank-1 / gemv fix-ups).
#include <stdio.h>
#include "fmm.h"
#include "blas.h"

#define PAR_MIN 65536  // elements below which additions run serially

// Z = a*X + b*Y   (m x n blocks, arbitrary leading dims; Z may alias X or Y)
static void axpby2(size_t m, size_t n, double a, const double *X, size_t ldx, double b, const double *Y,
                   size_t ldy, double *Z, size_t ldz) {
  #pragma omp parallel for schedule(static) if (m * n > PAR_MIN)
  for (size_t j = 0; j < n; j++) {
    const double *x = X + j * ldx, *y = Y + j * ldy;
    double *z = Z + j * ldz;
    #pragma omp simd
    for (size_t i = 0; i < m; i++) z[i] = a * x[i] + b * y[i];
  }
}

static void copyblk(size_t m, size_t n, const double *X, size_t ldx, double *Z, size_t ldz) {
  #pragma omp parallel for schedule(static) if (m * n > PAR_MIN)
  for (size_t j = 0; j < n; j++) memcpy(Z + j * ldz, X + j * ldx, m * sizeof(double));
}

size_t sw_workspace(size_t m, size_t k, size_t n, int depth) {
  if (depth <= 0 || m < 2 || k < 2 || n < 2) return 0;
  size_t m2 = m / 2, k2 = k / 2, n2 = n / 2;
  size_t here = m2 * (k2 > n2 ? k2 : n2) + k2 * n2 + m2 * n2;  // X, Y, Z
  return here + sw_workspace(m2, k2, n2, depth - 1);
}

static void sw_rec(int depth, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B,
                   size_t ldb, double *C, size_t ldc, double *W);

// Optional leaf multiply (C = A*B) used instead of dgemm when the recursion bottoms out;
// lets the memory-lean top levels hand sub-products to another algorithm.
static sw_leaf_fn g_leaf = NULL;
static void *g_leaf_ctx = NULL;
void sw_set_leaf(sw_leaf_fn f, void *ctx) { g_leaf = f; g_leaf_ctx = ctx; }
static void leaf_mul(size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B, size_t ldb, double *C,
                     size_t ldc) {
  if (g_leaf) g_leaf(m, k, n, A, lda, B, ldb, C, ldc, g_leaf_ctx);
  else dgemm_nn(m, n, k, 1.0, A, lda, B, ldb, 0.0, C, ldc);
}

// C = alpha*A*B + beta*C using recursion depth d (Z is scratch m x n when needed)
static void mul(int d, double alpha, size_t m, size_t k, size_t n, const double *A, size_t lda,
                const double *B, size_t ldb, double beta, double *C, size_t ldc, double *Z, double *W) {
  if (d <= 0 && !g_leaf) { dgemm_nn(m, n, k, alpha, A, lda, B, ldb, beta, C, ldc); return; }
  if (d <= 0) {
    if (alpha == 1.0 && beta == 0.0) { leaf_mul(m, k, n, A, lda, B, ldb, C, ldc); return; }
    leaf_mul(m, k, n, A, lda, B, ldb, Z, m);
    axpby2(m, n, alpha, Z, m, beta, C, ldc, C, ldc);
    return;
  }
  if (alpha == 1.0 && beta == 0.0) { sw_rec(d, m, k, n, A, lda, B, ldb, C, ldc, W); return; }
  sw_rec(d, m, k, n, A, lda, B, ldb, Z, m, W);
  axpby2(m, n, alpha, Z, m, beta, C, ldc, C, ldc);
}

static void sw_rec(int depth, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B,
                   size_t ldb, double *C, size_t ldc, double *W) {
  if (depth <= 0 || m < 2 || k < 2 || n < 2) { leaf_mul(m, k, n, A, lda, B, ldb, C, ldc); return; }
  size_t m2 = m / 2, k2 = k / 2, n2 = n / 2;
  const double *A11 = A, *A21 = A + m2, *A12 = A + k2 * lda, *A22 = A + m2 + k2 * lda;
  const double *B11 = B, *B21 = B + k2, *B12 = B + n2 * ldb, *B22 = B + k2 + n2 * ldb;
  double *C11 = C, *C21 = C + m2, *C12 = C + n2 * ldc, *C22 = C + m2 + n2 * ldc;
  size_t ldx = m2;
  double *X = W, *Y = X + m2 * (k2 > n2 ? k2 : n2), *Z = Y + k2 * n2, *Wn = Z + m2 * n2;
  size_t ldy = k2;
  int d = depth - 1;

  axpby2(m2, k2, 1, A11, lda, -1, A21, lda, X, ldx);          // S3 = A11 - A21
  axpby2(k2, n2, 1, B22, ldb, -1, B12, ldb, Y, ldy);          // T3 = B22 - B12
  mul(d, 1, m2, k2, n2, X, ldx, Y, ldy, 0, C21, ldc, Z, Wn);  // P7
  axpby2(m2, k2, 1, A21, lda, 1, A22, lda, X, ldx);           // S1 = A21 + A22
  axpby2(k2, n2, 1, B12, ldb, -1, B11, ldb, Y, ldy);          // T1 = B12 - B11
  mul(d, 1, m2, k2, n2, X, ldx, Y, ldy, 0, C22, ldc, Z, Wn);  // P5
  axpby2(m2, k2, 1, X, ldx, -1, A11, lda, X, ldx);            // S2 = S1 - A11
  axpby2(k2, n2, 1, B22, ldb, -1, Y, ldy, Y, ldy);            // T2 = B22 - T1
  mul(d, 1, m2, k2, n2, X, ldx, Y, ldy, 0, C12, ldc, Z, Wn);  // P6
  axpby2(m2, k2, 1, A12, lda, -1, X, ldx, X, ldx);            // S4 = A12 - S2
  mul(d, 1, m2, k2, n2, X, ldx, B22, ldb, 0, C11, ldc, Z, Wn);  // P3
  mul(d, 1, m2, k2, n2, A11, lda, B11, ldb, 0, X, ldx, Z, Wn);  // P1 (X now m2 x n2)
  // Fused post-additions: C11<-P1, C12<-P1+P6+P5+P3, C21<-P1+P6+P7, C22<-P1+P6+P7+P5
  #pragma omp parallel for schedule(static) if (m2 * n2 > PAR_MIN)
  for (size_t j = 0; j < n2; j++) {
    double *x = X + j * ldx, *c11 = C11 + j * ldc, *c12 = C12 + j * ldc, *c21 = C21 + j * ldc, *c22 = C22 + j * ldc;
    #pragma omp simd
    for (size_t i = 0; i < m2; i++) {
      double p1 = x[i], p3 = c11[i], p6 = c12[i], p7 = c21[i], p5 = c22[i];
      double u2 = p1 + p6, u3 = u2 + p7;
      c22[i] = u3 + p5;
      c12[i] = (u2 + p5) + p3;
      c21[i] = u3;
      c11[i] = p1;
    }
  }
  axpby2(k2, n2, 1, Y, ldy, -1, B21, ldb, Y, ldy);               // T4 = T2 - B21
  mul(d, -1, m2, k2, n2, A22, lda, Y, ldy, 1, C21, ldc, Z, Wn);  // C21 = U3 - P4
  mul(d, 1, m2, k2, n2, A12, lda, B21, ldb, 1, C11, ldc, Z, Wn); // C11 = P1 + P2

  // Dynamic peeling for odd dimensions.
  if (k & 1) {  // C[0:2m2,0:2n2] += A[0:2m2,k-1] * B[k-1,0:2n2]
    dgemm_nn(2 * m2, 2 * n2, 1, 1.0, A + (k - 1) * lda, lda, B + (k - 1), ldb, 1.0, C, ldc);
  }
  if (m & 1) {  // last row of C over the first 2*n2 columns
    dgemm_nn(1, 2 * n2, k, 1.0, A + (m - 1), lda, B, ldb, 0.0, C + (m - 1), ldc);
  }
  if (n & 1) {  // last column of C, all rows
    dgemm_nn(m, 1, k, 1.0, A, lda, B + (n - 1) * ldb, ldb, 0.0, C + (n - 1) * ldc, ldc);
  }
}

void sw_dgemm(int depth, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B,
              size_t ldb, double *C, size_t ldc, double *work) {
  size_t ws = sw_workspace(m, k, n, depth);
  double *W = work;
  if (!W && ws) W = amalloc(ws * sizeof(double));
  sw_rec(depth, m, k, n, A, lda, B, ldb, C, ldc, W);
  if (W != work) free(W);
}
