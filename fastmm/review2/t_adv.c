// Adversarial inputs for the certified mode.
//   t_adv near   : rounding errors of the scaled integers all +1/2, aligned with the large entries of
//                  the other operand, LB exact (quantised values exact): error close to the bound
//   t_adv exact  : same, big entries powers of two and tails that cancel in DGEMM: DGEMM ~ exact
//   t_adv split N: k = N chunks of 131071 (split path), each chunk certified near its bound
//   t_adv misc   : subnormal output, overflow, NaN/Inf, integers, zero rows, one huge entry, ...
#include "certlib.h"

// Checker design.  For (i+p) even A_ip = qa/64 (big), else small; for (p+j) odd B_pj = qa/64, else small.
// Scaled (sig = kexp + 6) small entries are X = qs*2^kexp + D + tail with tail = +1/2 (or -3/2): round
// half to even gives the rounding error +1/2 exactly; floor(64*small) = qs exactly.
static void build(double *A, double *B, size_t m, size_t k, size_t n, size_t chunk, size_t keff, int qa, int qs,
                  int kexp, int tailmode, uint64_t seed, int constD) {
  uint64_t s = seed;
  for (size_t p = 0; p < k; p++) {
    size_t pc = p % chunk;
    for (size_t i = 0; i < m; i++) {
      double v = 0;
      if (pc < keff) {
        if ((i + p) % 2 == 0) v = qa / 64.0;
        else {
          double D = constD ? ldexp(1, 20) : 2.0 * (double)(rng_next(&s) >> 30);  // even, < 2^35
          double tail = (tailmode == 1 && (pc / 2) % 4 == 3) ? -1.5 : 0.5;
          v = ldexp(qs * ldexp(1, kexp) + D + tail, -(kexp + 6));
        }
      }
      A[i + p * m] = v;
    }
  }
  for (size_t j = 0; j < n; j++)
    for (size_t p = 0; p < k; p++) {
      size_t pc = p % chunk;
      double v = 0;
      if (pc < keff) {
        if ((p + j) % 2 == 1) v = qa / 64.0;
        else {
          double D = constD ? ldexp(1, 20) : 2.0 * (double)(rng_next(&s) >> 30);
          double tail = (tailmode == 1 && (pc / 2) % 4 == 3) ? -1.5 : 0.5;
          v = ldexp(qs * ldexp(1, kexp) + D + tail, -(kexp + 6));
        }
      }
      B[p + j * k] = v;
    }
}

static void print_sig(size_t m, size_t k, const double *A, size_t chunk) {
  double ss = 0;
  size_t kk = k < chunk ? k : chunk;
  for (size_t p = 0; p < kk; p++) ss += A[0 + p * m] * A[0 + p * m];
  int L = oz_bits(16, kk);
  printf("  (row 0: L=%d, predicted sigma=%d)\n", L, L - norm_exp(ss, 0));
}

int main(int argc, char **argv) {
  const char *what = argc > 1 ? argv[1] : "near";
  if (!strcmp(what, "near")) {
    size_t m = 64, n = 64, k = 16384;
    double *A = malloc(m * k * 8), *B = malloc(k * n * 8);
    int cfg[][3] = {{127, 9, 47}, {64, 5, 48}, {127, 19, 46}, {101, 19, 46}};
    for (int c = 0; c < 4; c++) {
      build(A, B, m, k, n, k, k, cfg[c][0], cfg[c][1], cfg[c][2], 0, 7 + c, 0);
      char nm[64]; snprintf(nm, sizeof nm, "near qa=%d qs=%d kappa=2^%d", cfg[c][0], cfg[c][1], cfg[c][2]);
      print_sig(m, k, A, k);
      check(nm, m, k, n, A, m, B, k, 4.0, 0, 0, 1);
    }
  } else if (!strcmp(what, "exact")) {
    size_t m = 16, n = 16, k = 16384;
    double *A = malloc(m * k * 8), *B = malloc(k * n * 8);
    build(A, B, m, k, n, k, k, 64, 5, 48, 1, 3, 1);
    print_sig(m, k, A, k);
    check("dgemm-exact design qa=64 qs=5", m, k, n, A, m, B, k, 4.0, 0, 0, 1);
    // print one entry
    double *C = malloc(m * n * 8), *D = malloc(m * n * 8), lo, ab;
    oz_set_certify(4.0); oz_dgemm(16, m, k, n, A, m, B, k, C, m, NULL); oz_set_certify(0);
    dgemm_nn(m, n, k, 1.0, A, m, B, k, 0.0, D, m);
    double r = refd(k, A, m, B, k, 0, 0, 0, 0, &lo, &ab);
    printf("  entry (0,0): exact %.17g (+%.3g)  ozc16 %.17g  err %.3fu|A||B|   DGEMM %.17g  err %.3fu|A||B|\n", r, lo,
           C[0], fabs(C[0] - r - lo) / ab / U, D[0], fabs(D[0] - r - lo) / ab / U);
  } else if (!strcmp(what, "split")) {
    int nch = argc > 2 ? atoi(argv[2]) : 2;
    size_t m = argc > 3 ? atol(argv[3]) : 32, n = m, chunk = 131071, k = (size_t)nch * chunk;
    int qa = 127, qs = 9, kexp = 47;
    size_t keff = 16384;
    double *A = malloc(m * k * 8), *B = malloc(k * n * 8);
    build(A, B, m, k, n, chunk, keff, qa, qs, kexp, 0, 11, 0);
    print_sig(m, k, A, chunk);
    char nm[64]; snprintf(nm, sizeof nm, "split %d chunks qa=%d qs=%d", nch, qa, qs);
    check(nm, m, k, n, A, m, B, k, 4.0, 0, 0, 1);
    if (argc > 5) {  // dump entry (i, j): row i of A, column j of B, ozc16 and DGEMM values
      size_t i = atol(argv[4]), j = atol(argv[5]);
      double *C = malloc(m * n * 8), *D = malloc(m * n * 8);
      oz_set_certify(4.0); oz_dgemm(16, m, k, n, A, m, B, k, C, m, NULL); oz_set_certify(0);
      dgemm_nn(m, n, k, 1.0, A, m, B, k, 0.0, D, m);
      FILE *f = fopen("/tmp/claude-0/-home-user-sust-app/5d2e18a6-ecf5-578e-b4c6-84a644836956/scratchpad/split_entry.bin", "wb");
      fwrite(&k, 8, 1, f);
      for (size_t p = 0; p < k; p++) fwrite(&A[i + p * m], 8, 1, f);
      fwrite(&B[j * k], 8, k, f);
      fwrite(&C[i + j * m], 8, 1, f); fwrite(&D[i + j * m], 8, 1, f);
      fclose(f);
    }
    // same data as one chunk of k_eff*nch nonzeros?  (control: k <= 131071, no split)
  }
  return 0;
}
