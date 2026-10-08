// Value classes: subnormal outputs, overflow, NaN/Inf, integers, zero rows/cols, huge single entries,
// straddling quantisation, k = 0, theta not a power of two.
#include "certlib.h"

static void fill(double *X, size_t len, uint64_t seed) { uint64_t s = seed; for (size_t t = 0; t < len; t++) X[t] = rng_unif(&s); }

int main(void) {
  size_t m = 300, k = 257, n = 290;
  double *A = malloc(m * k * 8), *B = malloc(k * n * 8);
  // 1. subnormal outputs: entries ~2^-540 -> products ~2^-1080 (below the normal range)
  for (int e = 520; e <= 560; e += 20) {
    fill(A, m * k, 1); fill(B, k * n, 2);
    for (size_t t = 0; t < m * k; t++) A[t] = ldexp(A[t], -e);
    for (size_t t = 0; t < k * n; t++) B[t] = ldexp(B[t], -e);
    char nm[64]; snprintf(nm, sizeof nm, "subnormal output 2^-%d each", e);
    check(nm, m, k, n, A, m, B, k, 4.0, e, e, 1);
  }
  // 2. subnormal inputs with normal outputs
  fill(A, m * k, 3); fill(B, k * n, 4);
  for (size_t t = 0; t < m * k; t++) A[t] = ldexp(A[t], -1060);
  for (size_t t = 0; t < k * n; t++) B[t] = ldexp(B[t], 1000);
  check("subnormal A (2^-1060), B 2^1000", m, k, n, A, m, B, k, 4.0, 100, -100, 1);
  // 3. overflow region: products overflow, c finite (B column alternates sign: exact cancellation pairs)
  fill(A, m * k, 5); fill(B, k * n, 6);
  for (size_t t = 0; t < m * k; t++) A[t] = ldexp(A[t], 1000);
  for (size_t t = 0; t < k * n; t++) B[t] = ldexp(B[t], 30);
  check("products overflow (2^1000 x 2^30)", m, k, n, A, m, B, k, 4.0, -100, 0, 1);
  // 4. near DBL_MAX outputs, finite products
  fill(A, m * k, 7); fill(B, k * n, 8);
  for (size_t t = 0; t < m * k; t++) A[t] = ldexp(fabs(A[t]), 511);
  for (size_t t = 0; t < k * n; t++) B[t] = ldexp(fabs(B[t]), 503);
  check("outputs near DBL_MAX", m, k, n, A, m, B, k, 4.0, -10, 0, 1);
  // 5. NaN / Inf
  for (int t = 0; t < 3; t++) {
    fill(A, m * k, 9); fill(B, k * n, 10);
    if (t == 0) A[17 + 33 * m] = NAN;
    if (t == 1) B[5 + 200 * k] = INFINITY;
    if (t == 2) { A[17 + 33 * m] = INFINITY; B[33 + 7 * k] = 0.0; }  // Inf * 0
    check(t == 0 ? "NaN in A" : t == 1 ? "Inf in B" : "Inf*0", m, k, n, A, m, B, k, 4.0, 0, 0, 1);
  }
  // 6. integers
  { uint64_t s = 11;
    for (size_t t = 0; t < m * k; t++) A[t] = (double)((int64_t)(rng_next(&s) % 2000001) - 1000000);
    for (size_t t = 0; t < k * n; t++) B[t] = (double)((int64_t)(rng_next(&s) % 2000001) - 1000000);
    check("integers |x|<=1e6", m, k, n, A, m, B, k, 4.0, 0, 0, 1); }
  // 7. zero rows / columns, one huge entry per row, a row of tiny entries
  fill(A, m * k, 12); fill(B, k * n, 13);
  for (size_t p = 0; p < k; p++) { A[5 + p * m] = 0; A[200 + p * m] = 0; }
  for (size_t p = 0; p < k; p++) { B[p + 7 * k] = 0; }
  for (size_t i = 20; i < 60; i++) A[i + ((i * 7) % k) * m] = ldexp(1.0, 60);
  for (size_t p = 0; p < k; p++) A[100 + p * m] = ldexp(A[100 + p * m], -900);
  check("zero rows/cols, huge entries, tiny row", m, k, n, A, m, B, k, 4.0, 0, 0, 1);
  // 8. straddling the 7-bit quantisation: |a| = (q + 1 - 2^-40)/64 and q/64 exactly, mixed with 127.99/64
  { uint64_t s = 14;
    for (size_t t = 0; t < m * k; t++) { int q = rng_next(&s) % 128; A[t] = (rng_next(&s) & 1 ? q / 64.0 : (q + 1 - ldexp(1, -40)) / 64.0) * (rng_next(&s) & 1 ? 1 : -1); }
    for (size_t t = 0; t < k * n; t++) { int q = rng_next(&s) % 128; B[t] = (rng_next(&s) & 1 ? q / 64.0 : (q + 1 - ldexp(1, -40)) / 64.0); }
    check("quantisation boundaries", m, k, n, A, m, B, k, 4.0, 0, 0, 1); }
  // 9. theta not a power of two, and tiny theta
  fill(A, m * k, 15); fill(B, k * n, 16);
  check("theta=3", m, k, n, A, m, B, k, 3.0, 0, 0, 1);
  check("theta=0.3", m, k, n, A, m, B, k, 0.3, 0, 0, 1);
  check("theta=1e-3", m, k, n, A, m, B, k, 1e-3, 0, 0, 1);
  // 10. k = 0
  { double C[4] = {7, 7, 7, 7}; oz_set_certify(4.0); oz_dgemm(16, 2, 0, 2, A, 2, B, 1, C, 2, NULL); oz_set_certify(0);
    printf("k=0: C = %g %g %g %g\n", C[0], C[1], C[2], C[3]); }
  // 11. randexp very wide and rows mixing 2^+-1000
  testmat_fill(4, 1000, m, k, n, A, B, 3);
  check("randexp r=1000", m, k, n, A, m, B, k, 4.0, 0, 0, 1);
  return 0;
}
