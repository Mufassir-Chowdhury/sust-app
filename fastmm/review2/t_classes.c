// Sweep: all testmat classes, several r, shapes; every entry checked.
#include "certlib.h"
int main(int argc, char **argv) {
  size_t N = argc > 1 ? atol(argv[1]) : 400;
  int rs[] = {0, 8, 16, 32, 48, 64, 100};
  for (int type = 0; type < 8; type++)
    for (int ri = 0; ri < 7; ri++) {
      int r = rs[ri];
      if ((type == 0 || type == 1 || type == 5) && r) continue;
      size_t m = N, k = N + 37, n = N - 21;
      double *A = malloc(m * k * 8), *B = malloc(k * n * 8);
      testmat_fill(type, r, m, k, n, A, B, 42 + r);
      char nm[64]; snprintf(nm, sizeof nm, "%s r=%d", testmat_name[type], r);
      check(nm, m, k, n, A, m, B, k, 4.0, 0, 0, 1);
      free(A); free(B);
    }
  return 0;
}
