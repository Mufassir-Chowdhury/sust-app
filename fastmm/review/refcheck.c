// Dump sampled reference entries (ref_entry from src/testmat.h) with their A row / B column so that
// review/refcheck.py can recompute them exactly. Review code.
#include <stdio.h>
#include "testmat.h"
int main(int argc, char **argv) {
  int type = atoi(argv[1]), r = atoi(argv[2]); size_t n = atol(argv[3]); int cnt = atoi(argv[4]);
  double *A = amalloc(n * n * 8), *B = amalloc(n * n * 8);
  testmat_fill(type, r, n, n, n, A, B, 42);
  FILE *f = fopen(argv[5], "wb");
  uint64_t s = 77;
  fwrite(&n, 8, 1, f);
  for (int t = 0; t < cnt; t++) {
    size_t i = rng_next(&s) % n, j = rng_next(&s) % n;
    double lo, ab, hi = ref_entry(n, n, A, B, i, j, &lo, &ab);
    for (size_t p = 0; p < n; p++) fwrite(&A[i + p * n], 8, 1, f);
    fwrite(&B[j * n], 8, n, f);
    fwrite(&hi, 8, 1, f); fwrite(&lo, 8, 1, f); fwrite(&ab, 8, 1, f);
  }
  fclose(f);
  return 0;
}
