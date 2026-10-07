// Minimal STREAM-style bandwidth probe (copy / add / triad) on large arrays.
#include <stdio.h>
#include <omp.h>
#include "common.h"
int main(int argc, char **argv) {
  size_t n = argc > 1 ? atol(argv[1]) : 100000000;  // 800 MB per array
  double *a = amalloc(n * 8), *b = amalloc(n * 8), *c = amalloc(n * 8);
  #pragma omp parallel for
  for (size_t i = 0; i < n; i++) { a[i] = 1; b[i] = 2; c[i] = 0; }
  double best_add = 1e9, best_copy = 1e9, best_sc = 1e9;
  for (int r = 0; r < 7; r++) {
    double t0 = now_sec();
    #pragma omp parallel for
    for (size_t i = 0; i < n; i++) c[i] = a[i];
    double t1 = now_sec();
    #pragma omp parallel for
    for (size_t i = 0; i < n; i++) c[i] = a[i] + b[i];
    double t2 = now_sec();
    #pragma omp parallel for
    for (size_t i = 0; i < n; i++) a[i] += 0.5 * b[i];
    double t3 = now_sec();
    if (t1 - t0 < best_copy) best_copy = t1 - t0;
    if (t2 - t1 < best_add) best_add = t2 - t1;
    if (t3 - t2 < best_sc) best_sc = t3 - t2;
  }
  printf("threads=%d copy %.1f GB/s  add(c=a+b) %.1f GB/s  axpy(a+=sb) %.1f GB/s (counting reads+writes, no write-allocate)\n",
         omp_get_max_threads(), 16.0 * n / best_copy * 1e-9, 24.0 * n / best_add * 1e-9, 24.0 * n / best_sc * 1e-9);
  return 0;
}
