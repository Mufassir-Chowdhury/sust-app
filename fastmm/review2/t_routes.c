// Repair routes (Dot2, tile DGEMM, block fallback) under memory blocking, odd shapes, lda/ldb > m/k.
#include "certlib.h"
// Make entry (i,j) fail certification: row i of A large (2^g) at even p, column j of B large at odd p.
static void bad_row(double *A, size_t lda, size_t k, size_t i, int g) { for (size_t p = 0; p < k; p += 2) A[i + p * lda] = ldexp(A[i + p * lda], g); }
static void bad_col(double *B, size_t ldb, size_t k, size_t j, int g) { for (size_t p = 1; p < k; p += 2) B[p + j * ldb] = ldexp(B[p + j * ldb], g); }

static void scenario(size_t m, size_t k, size_t n, size_t padA, size_t padB, int ndense, int nisol, const char *mem, uint64_t seed) {
  size_t lda = m + padA, ldb = k + padB;
  double *A = malloc(lda * k * 8 + 8), *B = malloc(ldb * n * 8 + 8);
  for (size_t t = 0; t < lda * k; t++) A[t] = NAN;  // padding stays NaN
  for (size_t t = 0; t < ldb * n; t++) B[t] = NAN;
  uint64_t s = seed;
  for (size_t p = 0; p < k; p++) for (size_t i = 0; i < m; i++) A[i + p * lda] = rng_unif(&s);
  for (size_t j = 0; j < n; j++) for (size_t p = 0; p < k; p++) B[p + j * ldb] = rng_unif(&s);
  // anchors keep every column max of A and row max of B at 2^30 (no inner scaling): row m-1 large at odd p,
  // column n-1 large at even p.  Bad rows are large at even p, bad columns at odd p: bad x bad entries fail.
  if (m > 1 && n > 1) {
  for (size_t p = 1; p < k; p += 2) A[m - 1 + p * lda] = ldexp(A[m - 1 + p * lda], 30);
  for (size_t p = 0; p < k; p += 2) B[p + (n - 1) * ldb] = ldexp(B[p + (n - 1) * ldb], 30);
  for (int t = 0; t < ndense; t++) { if (260 + t < m - 1) bad_row(A, lda, k, 260 + t, 30); if (300 + t < n - 1) bad_col(B, ldb, k, 300 + t, 30); }
  for (int t = 0; t < nisol; t++) {  // rows/cols in different 256-tiles: a grid of isolated failures
    size_t i = (37 + 523 * (size_t)t) % (m - 1), j = (11 + 701 * (size_t)t) % (n - 1);
    bad_row(A, lda, k, i, 30); bad_col(B, ldb, k, j, 30);
  }
  }
  if (mem) setenv("OZ_MEM_GB", mem, 1); else unsetenv("OZ_MEM_GB");
  char nm[96];
  snprintf(nm, sizeof nm, "routes dense=%d isol=%d mem=%s lda+%zu", ndense, nisol, mem ? mem : "-", padA);
  check(nm, m, k, n, A, lda, B, ldb, 4.0, 0, 0, 1);
  unsetenv("OZ_MEM_GB");
  free(A); free(B);
}
int main(void) {
  setenv("OZ_VERBOSE", "1", 1);
  scenario(1100, 300, 1037, 5, 3, 0, 4, NULL, 1);
  scenario(1100, 300, 1037, 5, 3, 17, 4, NULL, 2);
  scenario(1100, 300, 1037, 5, 3, 17, 4, "0.002", 3);
  scenario(1100, 300, 1037, 5, 3, 17, 4, "0.0005", 4);
  scenario(1100, 300, 1037, 0, 0, 40, 6, NULL, 5);    // several dense tiles
  scenario(700, 129, 650, 1, 1, 17, 3, "0.0003", 6);
  scenario(33, 1, 31, 2, 1, 0, 2, NULL, 7);             // k = 1
  scenario(17, 2, 5, 2, 1, 0, 1, NULL, 8);             // k = 2, tiny
  scenario(1, 70, 300, 0, 0, 0, 1, NULL, 9);           // m = 1
  scenario(300, 70, 1, 0, 0, 0, 1, NULL, 10);          // n = 1
  scenario(290, 513, 270, 7, 9, 20, 2, "0.0002", 11);
  return 0;
}
