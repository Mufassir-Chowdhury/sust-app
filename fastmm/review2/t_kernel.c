// amx_gemm_s8s8 / amx_gemm_u8u8 (portable path on this machine) vs a naive mod-2^32 product.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "amx.h"
typedef struct { int32_t *C; size_t Np; long calls; } col_t;
static void epi(const int32_t *blk, size_t ld, size_t i0, size_t j0, void *ctx) {
  col_t *c = ctx;
  for (size_t r = 0; r < 32; r++) for (size_t q = 0; q < 32; q++) c->C[(i0 + r) * c->Np + j0 + q] = blk[r * ld + q];
  __atomic_add_fetch(&c->calls, 1, __ATOMIC_RELAXED);
}
static uint64_t rs = 12345;
static uint64_t xr(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs * 0x2545F4914F6CDD1DULL; }
// mode 0 random, 1 extremes only, 2 all -128 (s8) / 255 (u8)
static int run(int uns, size_t M, size_t N, size_t K, int mode) {
  size_t Mp = amx_pad(M, 32), Np = amx_pad(N, 64), Kp = amx_pad(K, 64);
  int8_t *a = calloc(M * K, 1), *b = calloc(K * N, 1);  // a[i*K+p], b[p*N+j]
  for (size_t t = 0; t < M * K; t++) a[t] = mode == 2 ? (uns ? (int8_t)255 : -128) : mode == 1 ? (int8_t)((xr() & 1) ? (uns ? 255 : -128) : 127) : (int8_t)xr();
  for (size_t t = 0; t < K * N; t++) b[t] = mode == 2 ? (uns ? (int8_t)255 : -128) : mode == 1 ? (int8_t)((xr() & 1) ? (uns ? 255 : -128) : 127) : (int8_t)xr();
  int8_t *Ap = aligned_alloc(64, Mp * Kp), *Bp = aligned_alloc(64, Np * Kp);
  memset(Ap, 0, Mp * Kp); memset(Bp, 0, Np * Kp);
  for (size_t i = 0; i < M; i++) for (size_t p = 0; p < K; p++) Ap[amx_tile_off(i / 16, p / 64, Kp) + (i % 16) * 64 + p % 64] = a[i * K + p];
  for (size_t p = 0; p < K; p++) for (size_t j = 0; j < N; j++)
    Bp[amx_tile_off(j / 16, p / 64, Kp) + ((p % 64) / 4) * 64 + 4 * (j % 16) + p % 4] = b[p * N + j];
  col_t c = {calloc(Mp * Np, 4), Np, 0};
  for (size_t t = 0; t < Mp * Np; t++) c.C[t] = 0x5a5a5a5a;
  if (uns) amx_gemm_u8u8(Mp, Np, Kp, (uint8_t *)Ap, (uint8_t *)Bp, epi, &c); else amx_gemm_s8s8(Mp, Np, Kp, Ap, Bp, epi, &c);
  size_t bad = 0, wraps = 0;
  for (size_t i = 0; i < Mp; i++) for (size_t j = 0; j < Np; j++) {
    int64_t s = 0;
    if (i < M && j < N) for (size_t p = 0; p < K; p++) {
      int64_t x = uns ? (uint8_t)a[i * K + p] : a[i * K + p], y = uns ? (uint8_t)b[p * N + j] : b[p * N + j];
      s += x * y;
    }
    if (s > INT32_MAX || s < INT32_MIN) wraps++;
    int32_t ref = (int32_t)(uint32_t)(uint64_t)s;
    bad += c.C[i * Np + j] != ref;
  }
  printf("%s M=%zu N=%zu K=%zu (Mp=%zu Np=%zu Kp=%zu) mode=%d: epilogue calls %ld (expect %zu), mismatches %zu, entries that wrapped %zu\n",
         uns ? "u8u8" : "s8s8", M, N, K, Mp, Np, Kp, mode, c.calls, (Mp / 32) * (Np / 32), bad, wraps);
  free(a); free(b); free(Ap); free(Bp); free(c.C);
  return bad != 0;
}
int main(void) {
  amx_init();
  printf("amx_hardware() = %d\n", amx_hardware());
  int f = 0;
  size_t sh[][3] = {{1, 1, 1}, {31, 63, 65}, {33, 65, 127}, {95, 129, 600}, {130, 190, 1000}, {32, 64, 64}, {17, 200, 4097}};
  for (int u = 0; u < 2; u++)
    for (int t = 0; t < 7; t++)
      for (int md = 0; md < 2; md++) f += run(u, sh[t][0], sh[t][1], sh[t][2], md);
  f += run(0, 20, 40, 131073 + 64, 2);  // s8s8 all -128: 16384 * 131137 > 2^31, wraps
  f += run(1, 20, 40, 33100, 2);        // u8u8 all 255: 65025 * 33100 > 2^31
  f += run(1, 20, 40, 66000, 1);
  printf("%s\n", f ? "FAIL" : "all match");
  return f;
}
