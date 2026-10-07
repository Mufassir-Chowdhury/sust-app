// Review timing harness (not part of the project): interleaved rounds, paired ratios vs the first
// method, plus ratio of medians and of minimums.  Includes a textbook Strassen-Winograd written for
// this review (7 MKL calls per level, plain temporaries, OpenMP additions).
//   speed <reps> <n> method...    methods: dgemm | dgemmB (a second, identical dgemm) | tsw1 | tsw2
//                                          | sw1 (project) | W2 (project, task-parallel <4,4,4;49>) | oz14 | oz16 | ozw14
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include "common.h"
#include "blas.h"
#include "fmm.h"
#include "ozaki.h"
#include "gen.h"

// ---------------- textbook Strassen-Winograd ----------------
static void add(size_t m, size_t n, double a, const double *X, size_t ldx, double b, const double *Y, size_t ldy, double *Z,
                size_t ldz) {
  #pragma omp parallel for schedule(static)
  for (size_t j = 0; j < n; j++)
    for (size_t i = 0; i < m; i++) Z[i + j * ldz] = a * X[i + j * ldx] + b * Y[i + j * ldy];
}
// C = A*B, n x n, n divisible by 2^depth
static void tsw(int depth, size_t n, const double *A, size_t lda, const double *B, size_t ldb, double *C, size_t ldc) {
  if (depth == 0) { dgemm_nn(n, n, n, 1.0, A, lda, B, ldb, 0.0, C, ldc); return; }
  size_t h = n / 2, hh = h * h;
  const double *A11 = A, *A21 = A + h, *A12 = A + h * lda, *A22 = A + h + h * lda;
  const double *B11 = B, *B21 = B + h, *B12 = B + h * ldb, *B22 = B + h + h * ldb;
  double *C11 = C, *C21 = C + h, *C12 = C + h * ldc, *C22 = C + h + h * ldc;
  static double *wb[8];
  static size_t wsz[8];
  if (wsz[depth] < 6 * hh) {  // persistent per-level workspace, pre-faulted once
    free(wb[depth]); wb[depth] = amalloc(6 * hh * 8); wsz[depth] = 6 * hh;
    memset(wb[depth], 0, 6 * hh * 8);
  }
  double *W = wb[depth];
  double *S = W, *T = W + hh, *P = W + 2 * hh, *Q = W + 3 * hh, *U = W + 4 * hh;
  // C11 = A11 B11 + A12 B21
  tsw(depth - 1, h, A11, lda, B11, ldb, P, h);                              // P1
  tsw(depth - 1, h, A12, lda, B21, ldb, C11, ldc);                          // P2
  add(h, h, 1, P, h, 1, C11, ldc, C11, ldc);                                // C11 = P1 + P2
  add(h, h, 1, A21, lda, 1, A22, lda, S, h);                                // S1
  add(h, h, 1, B12, ldb, -1, B11, ldb, T, h);                               // T1
  tsw(depth - 1, h, S, h, T, h, Q, h);                                      // P5 = S1 T1
  add(h, h, 1, S, h, -1, A11, lda, S, h);                                   // S2 = S1 - A11
  add(h, h, 1, B22, ldb, -1, T, h, T, h);                                   // T2 = B22 - T1
  tsw(depth - 1, h, S, h, T, h, U, h);                                      // P6 = S2 T2
  add(h, h, 1, P, h, 1, U, h, P, h);                                        // U2 = P1 + P6  (in P)
  add(h, h, 1, A12, lda, -1, S, h, S, h);                                   // S4 = A12 - S2
  tsw(depth - 1, h, S, h, B22, ldb, U, h);                                  // P3 = S4 B22
  add(h, h, 1, P, h, 1, Q, h, C12, ldc);                                    // U4 = U2 + P5
  add(h, h, 1, C12, ldc, 1, U, h, C12, ldc);                                // C12 = U4 + P3
  add(h, h, 1, T, h, -1, B21, ldb, T, h);                                   // T4 = T2 - B21
  tsw(depth - 1, h, A22, lda, T, h, U, h);                                  // P4 = A22 T4
  add(h, h, 1, A11, lda, -1, A21, lda, S, h);                               // S3
  add(h, h, 1, B22, ldb, -1, B12, ldb, T, h);                               // T3
  double *P7 = W + 5 * hh;
  tsw(depth - 1, h, S, h, T, h, P7, h);                                     // P7 = S3 T3
  add(h, h, 1, P, h, 1, P7, h, P, h);                                       // U3 = U2 + P7
  add(h, h, 1, P, h, -1, U, h, C21, ldc);                                   // C21 = U3 - P4
  add(h, h, 1, P, h, 1, Q, h, C22, ldc);                                    // C22 = U3 + P5
}

static gen_scheme *gW2;
static double *gwork;
static size_t gwork_sz;
static void run(const char *me, size_t n, const double *A, const double *B, double *C) {
  if (!strncmp(me, "dgemm", 5)) dgemm_nn(n, n, n, 1.0, A, n, B, n, 0.0, C, n);
  else if (!strcmp(me, "tsw1")) tsw(1, n, A, n, B, n, C, n);
  else if (!strcmp(me, "tsw2")) tsw(2, n, A, n, B, n, C, n);
  else if (!strcmp(me, "sw1")) {
    size_t ws = sw_workspace(n, n, n, 1);
    if (ws > gwork_sz) { free(gwork); gwork = amalloc(ws * 8); gwork_sz = ws; }
    sw_dgemm(1, n, n, n, A, n, B, n, C, n, gwork);
  } else if (!strcmp(me, "W2")) {
    size_t ws = gen_workspace(gW2, n, n, n, 1, 0, 1);
    if (ws > gwork_sz) { free(gwork); gwork = amalloc(ws * 8); gwork_sz = ws; }
    gen_dgemm(gW2, 1, 0, 1, n, n, n, A, n, B, n, C, n, gwork);
  } else if (!strncmp(me, "ozw", 3)) oz_dgemm_w(atoi(me + 3), n, n, n, A, n, B, n, C, n, NULL);
  else if (!strncmp(me, "oz", 2)) oz_dgemm(atoi(me + 2), n, n, n, A, n, B, n, C, n, NULL);
}

int main(int argc, char **argv) {
  int reps = atoi(argv[1]);
  size_t n = atol(argv[2]);
  int nm = argc - 3;
  gW2 = gen_load("slp/winograd-squared.slp");
  printf("# mkl_get_max_threads=%d mkl_get_dynamic=%d omp_get_max_threads=%d\n", mkl_get_max_threads(), mkl_get_dynamic(),
         omp_get_max_threads());
  double *A = amalloc(n * n * 8), *B = amalloc(n * n * 8), *C = amalloc(n * n * 8), *R = amalloc(n * n * 8);
  fill_uniform(A, n, n, n, 1); fill_uniform(B, n, n, n, 2);
  dgemm_nn(n, n, n, 1.0, A, n, B, n, 0.0, R, n);
  double t[16][64];
  for (int a = 0; a < nm; a++) {  // warm-up + sanity check against MKL
    run(argv[3 + a], n, A, B, C);
    double mx = 0;
    for (size_t q = 0; q < n * n; q += 7) { double d = fabs(C[q] - R[q]); if (d > mx) mx = d; }
    printf("# %s warm-up max |C - C_mkl| (sampled) = %.2e\n", argv[3 + a], mx);
  }
  for (int i = 0; i < reps; i++)
    for (int a = 0; a < nm; a++) {
      double t0 = now_sec();
      run(argv[3 + a], n, A, B, C);
      t[a][i] = now_sec() - t0;
    }
  double med0, min0;
  for (int a = 0; a < nm; a++) {
    double tc[64], rat[64];
    memcpy(tc, t[a], reps * 8);
    double med = median(tc, reps), mn = tc[0];
    if (a == 0) { med0 = med; min0 = mn; }
    for (int i = 0; i < reps; i++) rat[i] = t[0][i] / t[a][i];
    double rmed = median(rat, reps);
    printf("n=%zu %-6s median=%.4f s (%.0f GF/s) min=%.4f  paired=%.3f [%.3f-%.3f]  med-ratio=%.3f min-ratio=%.3f  times:", n,
           argv[3 + a], med, 2.0 * n * n * n / med * 1e-9, mn, rmed, rat[reps / 4], rat[(3 * reps) / 4], med0 / med, min0 / mn);
    for (int i = 0; i < reps; i++) printf(" %.3f", t[a][i]);
    printf("\n");
    fflush(stdout);
  }
  return 0;
}
