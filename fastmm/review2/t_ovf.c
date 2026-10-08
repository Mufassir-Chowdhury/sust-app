#include "certlib.h"
int main(void) {
  size_t m = 300, k = 257, n = 290;
  double *A = malloc(m * k * 8), *B = malloc(k * n * 8), *C = malloc(m * n * 8), *D = malloc(m * n * 8);
  uint64_t s = 5; for (size_t t = 0; t < m * k; t++) A[t] = ldexp(rng_unif(&s), 1000);
  s = 6; for (size_t t = 0; t < k * n; t++) B[t] = ldexp(rng_unif(&s), 30);
  oz_set_certify(4.0); oz_dgemm(16, m, k, n, A, m, B, k, C, m, NULL); oz_set_certify(0);
  oz_cert_stats st = oz_get_cert_stats();
  dgemm_nn(m, n, k, 1.0, A, m, B, k, 0.0, D, m);
  size_t cfin = 0, dnonfin = 0, cinf = 0; double worst = 0;
  for (size_t j = 0; j < n; j++) for (size_t i = 0; i < m; i++) {
    double lo, ab, r = refd(k, A, m, B, k, i, j, -100, 0, &lo, &ab);
    double c = C[i + j * m];
    if (!isfinite(D[i + j * m])) dnonfin++;
    int rfin = fabs(r) < ldexp(1, 923);  // true value representable
    if (isfinite(c)) { cfin++; double e = fabs((ldexp(c, -100) - r) - lo) / ab / U; if (e > worst) worst = e; }
    else if (rfin) cinf++;
  }
  printf("overflowing products: ozc16 finite %zu/%zu, DGEMM non-finite %zu, ozc16 Inf where true value finite %zu, max ozc16 err %.3fu  (flagged so far %zu)\n",
         cfin, m * n, dnonfin, cinf, worst, st.flagged);
  return 0;
}
