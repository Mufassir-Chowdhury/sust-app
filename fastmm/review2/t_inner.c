// 4 rows of A large (2^30) at even p, 4 columns of B large at odd p, nothing else: only 16 entries pair
// large with small.  Compare flagged counts with and without the inner scaling.
#include "certlib.h"
int main(void) {
  size_t m = 600, k = 300, n = 600;
  double *A = malloc(m * k * 8), *B = malloc(k * n * 8);
  for (int inner = 1; inner >= 0; inner--) {
    uint64_t s = 1;
    for (size_t t = 0; t < m * k; t++) A[t] = rng_unif(&s);
    for (size_t t = 0; t < k * n; t++) B[t] = rng_unif(&s);
    for (int r = 0; r < 4; r++) {
      for (size_t p = 0; p < k; p += 2) A[(37 + 150 * r) + p * m] *= 0x1p30;
      for (size_t p = 1; p < k; p += 2) B[p + (11 + 150 * r) * k] *= 0x1p30;
    }
    if (!inner) setenv("OZ_INNER", "0", 1); else unsetenv("OZ_INNER");
    check(inner ? "4 bad rows/cols, inner scaling on" : "4 bad rows/cols, OZ_INNER=0", m, k, n, A, m, B, k, 4.0, 0, 0, 1);
  }
  return 0;
}
