#include "certlib.h"
int main(int argc, char **argv) {
  size_t N = argc > 1 ? atol(argv[1]) : 1500;
  int cases[][2] = {{4, 10}, {4, 12}, {2, 300}, {3, 300}, {7, 8}, {7, 10}, {7, 12}, {6, 3}, {6, 5}, {4, 8}};
  int ncases = argc > 2 ? atoi(argv[2]) : 10;
  const char *mems[] = {NULL, "0.01"};
  for (int c = 0; c < ncases; c++)
    for (int mi = 0; mi < 2; mi++) {
      size_t m = N, k = N - 77, n = N + 13;
      double *A = malloc(m * k * 8), *B = malloc(k * n * 8);
      testmat_fill(cases[c][0], cases[c][1], m, k, n, A, B, 42);
      if (mems[mi]) setenv("OZ_MEM_GB", mems[mi], 1); else unsetenv("OZ_MEM_GB");
      char nm[64]; snprintf(nm, sizeof nm, "%s r=%d mem=%s", testmat_name[cases[c][0]], cases[c][1], mems[mi] ? mems[mi] : "-");
      check(nm, m, k, n, A, m, B, k, 4.0, 0, 0, 1);
      free(A); free(B);
    }
  return 0;
}
