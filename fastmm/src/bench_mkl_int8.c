// Probe: throughput of MKL integer / bf16 GEMMs (do they use AMX here?)
#include <stdio.h>
#include <mkl.h>
#include "common.h"
int main(int argc, char **argv) {
  int reps = 5;
  for (int a = 1; a < argc; a++) {
    size_t n = atol(argv[a]);
    int8_t *A = amalloc(n * n); uint8_t *B = amalloc(n * n); int32_t *C = amalloc(n * n * 4);
    for (size_t i = 0; i < n * n; i++) { A[i] = (int8_t)(i * 7 % 255 - 127); B[i] = (uint8_t)(i * 13 % 256); }
    MKL_INT32 co = 0;
    double t[16];
    for (int r = -1; r < reps; r++) {
      double t0 = now_sec();
      cblas_gemm_s8u8s32(CblasColMajor, CblasNoTrans, CblasNoTrans, CblasFixOffset, n, n, n, 1.0f, A, n, 0, B, n, 0, 0.0f, C, n, &co);
      if (r >= 0) t[r] = now_sec() - t0;
    }
    double tm = median(t, reps);
    printf("s8u8s32 n=%zu %.4f s  %.1f GOP/s\n", n, tm, 2.0 * n * n * n / tm * 1e-9);
    // s16s16s32
    int16_t *A2 = amalloc(n*n*2), *B2 = amalloc(n*n*2);
    for (size_t i = 0; i < n * n; i++) { A2[i] = A[i]; B2[i] = B[i]; }
    for (int r = -1; r < reps; r++) {
      double t0 = now_sec();
      cblas_gemm_s16s16s32(CblasColMajor, CblasNoTrans, CblasNoTrans, CblasFixOffset, n, n, n, 1.0f, A2, n, 0, B2, n, 0, 0.0f, C, n, &co);
      if (r >= 0) t[r] = now_sec() - t0;
    }
    tm = median(t, reps);
    printf("s16s16s32 n=%zu %.4f s  %.1f GOP/s\n", n, tm, 2.0 * n * n * n / tm * 1e-9);
    MKL_BF16 *A3 = amalloc(n*n*2), *B3 = amalloc(n*n*2); float *C3 = amalloc(n*n*4);
    for (size_t i = 0; i < n * n; i++) { A3[i] = 0x3f80; B3[i] = 0x3f80; }
    for (int r = -1; r < reps; r++) {
      double t0 = now_sec();
      cblas_gemm_bf16bf16f32(CblasColMajor, CblasNoTrans, CblasNoTrans, n, n, n, 1.0f, A3, n, B3, n, 0.0f, C3, n);
      if (r >= 0) t[r] = now_sec() - t0;
    }
    tm = median(t, reps);
    printf("bf16bf16f32 n=%zu %.4f s  %.1f GFLOP/s\n", n, tm, 2.0 * n * n * n / tm * 1e-9);
    float *A4 = amalloc(n*n*4), *B4 = amalloc(n*n*4);
    for (size_t i = 0; i < n * n; i++) { A4[i] = 1; B4[i] = 1; }
    for (int r = -1; r < reps; r++) {
      double t0 = now_sec();
      cblas_sgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, n, n, n, 1.0f, A4, n, B4, n, 0.0f, C3, n);
      if (r >= 0) t[r] = now_sec() - t0;
    }
    tm = median(t, reps);
    printf("sgemm n=%zu %.4f s  %.1f GFLOP/s\n", n, tm, 2.0 * n * n * n / tm * 1e-9);
    free(A); free(B); free(C); free(A2); free(B2); free(A3); free(B3); free(C3); free(A4); free(B4);
  }
  return 0;
}
