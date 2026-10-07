// Correctness + throughput test of the AMX int8 GEMM kernel.
#include <stdio.h>
#include <omp.h>
#include "common.h"
#include "amx.h"
#include <sys/mman.h>
static void *hpalloc(size_t b) { void *p = amalloc(b); madvise(p, (b + (2u<<20) - 1) / (2u<<20) * (2u<<20), MADV_HUGEPAGE); return p; }

// pack column-major int8 A (m x k, lda) into tile rows
static void pack_A(size_t m, size_t k, const int8_t *A, size_t lda, int8_t *Ap, size_t Mp, size_t Kp) {
  memset(Ap, 0, Mp * Kp);
  for (size_t i = 0; i < m; i++)
    for (size_t p = 0; p < k; p++) Ap[amx_tile_off(i / 16, p / 64, Kp) + (i % 16) * 64 + (p % 64)] = A[i + p * lda];
}
// pack column-major int8 B (k x n, ldb) into VNNI tiles
static void pack_B(size_t k, size_t n, const int8_t *B, size_t ldb, int8_t *Bp, size_t Np, size_t Kp) {
  memset(Bp, 0, Np * Kp);
  for (size_t j = 0; j < n; j++)
    for (size_t p = 0; p < k; p++)
      Bp[amx_tile_off(j / 16, p / 64, Kp) + ((p % 64) / 4) * 64 + (j % 16) * 4 + (p % 4)] = B[p + j * ldb];
}

typedef struct { int32_t *C; size_t ldc, m, n; } store_ctx;
static void epi_store(const int32_t *blk, size_t ld, size_t i0, size_t j0, void *vctx) {
  store_ctx *c = vctx;
  for (size_t r = 0; r < 32 && i0 + r < c->m; r++)
    for (size_t q = 0; q < 32 && j0 + q < c->n; q++) c->C[(i0 + r) + (j0 + q) * c->ldc] = blk[r * ld + q];
}
static void epi_none(const int32_t *blk, size_t ld, size_t i0, size_t j0, void *vctx) { (void)blk; (void)ld; (void)i0; (void)j0; (void)vctx; }

int main(int argc, char **argv) {
  if (amx_init()) { fprintf(stderr, "AMX not available\n"); return 1; }
  // correctness on an odd-sized problem
  {
    size_t m = 77, n = 45, k = 200;
    size_t Mp = amx_pad(m, 32), Np = amx_pad(n, AMX_COLPAD), Kp = amx_pad(k, 64);
    int8_t *A = malloc(m * k), *B = malloc(k * n), *Ap = aligned_alloc(4096, Mp * Kp), *Bp = aligned_alloc(4096, Np * Kp);
    int32_t *C = calloc(m * n, 4);
    uint64_t s = 5;
    for (size_t i = 0; i < m * k; i++) A[i] = (int8_t)(rng_next(&s) % 256 - 128);
    for (size_t i = 0; i < k * n; i++) B[i] = (int8_t)(rng_next(&s) % 256 - 128);
    pack_A(m, k, A, m, Ap, Mp, Kp); pack_B(k, n, B, k, Bp, Np, Kp);
    store_ctx ctx = {C, m, m, n};
    amx_gemm_s8s8(Mp, Np, Kp, Ap, Bp, epi_store, &ctx);
    long bad = 0;
    for (size_t i = 0; i < m; i++)
      for (size_t j = 0; j < n; j++) {
        int32_t r = 0;
        for (size_t p = 0; p < k; p++) r += (int32_t)A[i + p * m] * B[p + j * k];
        if (r != C[i + j * m]) bad++;
      }
    printf("correctness: %ld mismatches\n", bad);
  }
  for (int a = 1; a < argc; a++) {
    size_t n = atol(argv[a]);
    size_t Np = amx_pad(n, AMX_COLPAD), Kp = amx_pad(n, 64);
    int8_t *Ap = hpalloc(Np * Kp), *Bp = hpalloc(Np * Kp);
    for (size_t i = 0; i < Np * Kp; i++) { Ap[i] = (int8_t)(i * 31 + 7); Bp[i] = (int8_t)(i * 17 + 3); }
    int32_t *C = aligned_alloc(4096, n * n * 4);
    store_ctx ctx = {C, n, n, n};
    double t[11];
    for (int mode = 0; mode < 2; mode++) {
      for (int r = -1; r < 7; r++) {
        double t0 = now_sec();
        amx_gemm_s8s8(Np, Np, Kp, Ap, Bp, mode ? epi_store : epi_none, &ctx);
        if (r >= 0) t[r] = now_sec() - t0;
      }
      double tm = median(t, 7);
      printf("n=%zu threads=%d %s: %.4f s  %.1f GOP/s\n", n, omp_get_max_threads(), mode ? "store" : "noepi", tm,
             2.0 * n * n * n / tm * 1e-9);
    }
    free(Ap); free(Bp); free(C);
  }
  return 0;
}
