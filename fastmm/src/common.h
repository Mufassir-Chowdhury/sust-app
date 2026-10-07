// Common utilities: timing, aligned allocation, random matrices, error metrics.
// All matrices are column-major with an explicit leading dimension.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

static inline double now_sec(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + 1e-9 * ts.tv_nsec;
}

static inline void *amalloc(size_t bytes) {
  void *p = NULL;
  size_t al = 2u << 20;  // 2 MiB alignment helps THP
  bytes = (bytes + al - 1) / al * al;
  if (posix_memalign(&p, al, bytes ? bytes : al)) return NULL;
  return p;
}

// xorshift-style generator, deterministic per seed.
static inline uint64_t rng_next(uint64_t *s) {
  uint64_t x = *s;
  x ^= x << 13; x ^= x >> 7; x ^= x << 17;
  *s = x;
  return x * 0x2545F4914F6CDD1DULL;
}
static inline double rng_unif(uint64_t *s) {  // uniform in [-1,1)
  return ((rng_next(s) >> 11) * (1.0 / 9007199254740992.0)) * 2.0 - 1.0;
}

static inline void fill_uniform(double *A, size_t m, size_t n, size_t lda, uint64_t seed) {
  #pragma omp parallel for schedule(static)
  for (size_t j = 0; j < n; j++) {
    uint64_t s = seed * 0x9E3779B97F4A7C15ULL + j * 0xD1B54A32D192ED03ULL + 1;
    for (int w = 0; w < 4; w++) rng_next(&s);
    for (size_t i = 0; i < m; i++) A[i + j * lda] = rng_unif(&s);
  }
}

static inline int cmp_double(const void *a, const void *b) {
  double x = *(const double *)a, y = *(const double *)b;
  return (x > y) - (x < y);
}
static inline double median(double *v, int n) {
  qsort(v, n, sizeof(double), cmp_double);
  return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}
