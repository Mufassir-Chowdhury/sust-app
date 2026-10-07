// Independent correctness tests for the AMX Ozaki-II emulation (review code, not part of the project).
// Calls oz_dgemm / oz_dgemm_w / oz_dgemm_fmm directly with padded leading dimensions, sentinel
// values in the padding, odd shapes and edge-case inputs, and compares with a double-double reference.
//   oztest shapes | values | bound | underflow | nondet | lda
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <omp.h>
#include "common.h"
#include "blas.h"
#include "ozaki.h"
#include "gen.h"
#include "oz_internal.h"

static gen_scheme *gW1;

typedef struct { double maxcw; size_t nan_bad, pad_bad, unwritten; } res_t;

// double-double reference (TwoProd via fma, TwoSum)
static void ref(size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B, size_t ldb, double *R,
                double *Rlo, double *AB) {
  #pragma omp parallel for collapse(2) schedule(dynamic, 64)
  for (size_t j = 0; j < n; j++)
    for (size_t i = 0; i < m; i++) {
      double hi = 0, lo = 0, ab = 0;
      for (size_t p = 0; p < k; p++) {
        double a = A[i + p * lda], b = B[p + j * ldb], pr = a * b, pe = fma(a, b, -pr);
        double s = hi + pr, bb = s - hi, e = (hi - (s - bb)) + (pr - bb);
        hi = s; lo += e + pe; ab += fabs(pr);
      }
      double s = hi + lo;
      R[i + j * m] = s; Rlo[i + j * m] = lo - (s - hi); AB[i + j * m] = ab;
    }
}

static const double SENT = 12345.678;
static int run_method(const char *meth, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B,
                      size_t ldb, double *C, size_t ldc) {
  if (!strcmp(meth, "dgemm")) dgemm_nn(m, n, k, 1.0, A, lda, B, ldb, 0.0, C, ldc);
  else if (!strncmp(meth, "ozw", 3)) oz_dgemm_w(atoi(meth + 3), m, k, n, A, lda, B, ldb, C, ldc, NULL);
  else if (!strncmp(meth, "ozf", 3)) oz_dgemm_fmm(atoi(meth + 3), gW1, m, k, n, A, lda, B, ldb, C, ldc, NULL);
  else if (!strncmp(meth, "oz", 2)) oz_dgemm(atoi(meth + 2), m, k, n, A, lda, B, ldb, C, ldc, NULL);
  else return -1;
  return 0;
}

static res_t evaluate(size_t m, size_t n, const double *C, size_t ldc, const double *R, const double *Rlo,
                      const double *AB) {
  res_t r = {0, 0, 0, 0};
  for (size_t j = 0; j < n; j++) {
    for (size_t i = 0; i < m; i++) {
      double c = C[i + j * ldc], rr = R[i + j * m];
      if (c == -777.25) r.unwritten++;
      if (isnan(rr) || isinf(rr)) { if (!(isnan(c) == isnan(rr) && isinf(c) == isinf(rr))) r.nan_bad++; continue; }
      if (!isfinite(c)) { r.nan_bad++; continue; }
      double d = fabs((c - rr) - Rlo[i + j * m]);
      double cw = AB[i + j * m] > 0 ? d / AB[i + j * m] : (d > 0 ? INFINITY : 0);
      if (cw > r.maxcw) r.maxcw = cw;
    }
    for (size_t i = m; i < ldc; i++) if (C[i + j * ldc] != SENT) r.pad_bad++;
  }
  return r;
}

// Run all methods on one problem; prints a line per method; returns number of failures.
static const char *METHS[] = {"dgemm", "oz14", "ozw14", "ozf14", "oz16", "ozw16", "ozf16"};
#define NMETH 7
static int verbose_all = 0;
static int check(const char *label, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B,
                 size_t ldb, size_t ldc, double lim14, double lim16) {
  double *R = malloc(m * n * 8 + 8), *Rlo = malloc(m * n * 8 + 8), *AB = malloc(m * n * 8 + 8);
  ref(m, k, n, A, lda, B, ldb, R, Rlo, AB);
  double *C[NMETH];
  int fails = 0;
  res_t rs[NMETH];
  for (int q = 0; q < NMETH; q++) {
    C[q] = malloc(ldc * n * 8 + 8);
    for (size_t j = 0; j < n; j++)
      for (size_t i = 0; i < ldc; i++) C[q][i + j * ldc] = i < m ? -777.25 : SENT;
    run_method(METHS[q], m, k, n, A, lda, B, ldb, C[q], ldc);
    rs[q] = evaluate(m, n, C[q], ldc, R, Rlo, AB);
  }
  size_t bd[NMETH] = {0};
  for (int q = 2; q < NMETH; q++) {
    int base = q <= 3 ? 1 : 4;
    if (q == 4) continue;
    for (size_t j = 0; j < n; j++)
      for (size_t i = 0; i < m; i++) bd[q] += memcmp(&C[q][i + j * ldc], &C[base][i + j * ldc], 8) != 0;
  }
  for (int q = 0; q < NMETH; q++) {
    double lim = q == 0 ? INFINITY : (q <= 3 ? lim14 : lim16);
    int bad = rs[q].maxcw > lim || rs[q].nan_bad || rs[q].pad_bad || rs[q].unwritten || bd[q];
    fails += bad;
    if (bad || verbose_all)
      printf("%-28s m=%zu k=%zu n=%zu lda=%zu ldb=%zu ldc=%zu %-6s maxcw=%.2e nanbad=%zu padbad=%zu unwritten=%zu bitdiff=%zu %s\n",
             label, m, k, n, lda, ldb, ldc, METHS[q], rs[q].maxcw, rs[q].nan_bad, rs[q].pad_bad, rs[q].unwritten,
             bd[q], bad ? "FAIL" : "ok");
  }
  for (int q = 0; q < NMETH; q++) free(C[q]);
  free(R); free(Rlo); free(AB);
  return fails;
}

static double *mk(size_t rows, size_t cols, size_t ld, double padval) {
  double *X = malloc(ld * cols * 8 + 64);
  for (size_t j = 0; j < cols; j++)
    for (size_t i = 0; i < ld; i++) X[i + j * ld] = padval;
  return X;
}
static void fillu(double *X, size_t rows, size_t cols, size_t ld, uint64_t seed) {
  for (size_t j = 0; j < cols; j++) {
    uint64_t s = seed * 1000003 + j * 7919 + 1;
    for (size_t i = 0; i < rows; i++) X[i + j * ld] = rng_unif(&s);
  }
}

int main(int argc, char **argv) {
  gW1 = gen_load("slp/winograd.slp");
  if (!gW1) { fprintf(stderr, "run from the fastmm directory\n"); return 1; }
  const char *mode = argc > 1 ? argv[1] : "shapes";
  int fails = 0, cases = 0;
  if (!strcmp(mode, "shapes")) {
    // padded leading dims, NaN in the padding of A and B (must never be read)
    size_t ms[] = {1, 2, 15, 17, 33, 64, 65, 129, 257}, ks[] = {1, 2, 63, 64, 65, 127, 128, 129, 513};
    for (size_t a = 0; a < sizeof ms / 8; a++)
      for (size_t b = 0; b < sizeof ks / 8; b++)
        for (size_t c = 0; c < sizeof ms / 8; c++) {
          size_t m = ms[a], k = ks[b], n = ms[(c + a) % (sizeof ms / 8)];
          size_t lda = m + (b % 3) * 5, ldb = k + (c % 2) * 3, ldc = m + (a % 2) * 7;
          double *A = mk(m, k, lda, NAN), *B = mk(k, n, ldb, NAN);
          fillu(A, m, k, lda, a * 100 + b), fillu(B, k, n, ldb, c * 1000 + b + 7);
          char lab[64]; snprintf(lab, sizeof lab, "shape");
          fails += check(lab, m, k, n, A, lda, B, ldb, ldc, 1e-14, 1e-15);
          cases++;
          free(A); free(B);
        }
    // long / thin / around the ozw thresholds and the k limits
    size_t sh[][3] = {{1, 100000, 1}, {2, 131071, 3}, {3, 131072, 2}, {5, 140001, 4}, {64, 66000, 64}, {64, 66001, 64},
                      {65, 65999, 67}, {64, 128, 64}, {64, 127, 64}, {63, 128, 64}, {64, 128, 63}, {2000, 1, 2000},
                      {3000, 2, 7}, {7, 3, 3000}, {4097, 33, 65}, {129, 4097, 65}, {1000, 1000, 1}, {1, 1000, 1000}};
    for (size_t t = 0; t < sizeof sh / sizeof sh[0]; t++) {
      size_t m = sh[t][0], k = sh[t][1], n = sh[t][2], lda = m + 3, ldb = k + 1, ldc = m + 1;
      double *A = mk(m, k, lda, NAN), *B = mk(k, n, ldb, NAN);
      fillu(A, m, k, lda, t + 11), fillu(B, k, n, ldb, t + 13);
      fails += check("thin", m, k, n, A, lda, B, ldb, ldc, 1e-13, 1e-15);
      cases++;
      free(A); free(B);
    }
  } else if (!strcmp(mode, "values")) {
    verbose_all = 1;
    size_t m = 150, k = 301, n = 130, lda = 157, ldb = 305, ldc = 151;
    for (int cls = 0; cls < 13; cls++) {
      double *A = mk(m, k, lda, NAN), *B = mk(k, n, ldb, NAN);
      fillu(A, m, k, lda, 5 + cls), fillu(B, k, n, ldb, 9 + cls);
      const char *lab = "";
      uint64_t s = 99 + cls;
      switch (cls) {
        case 0: lab = "zero rows/cols";
          for (size_t p = 0; p < k; p++) A[3 + p * lda] = 0, A[m - 1 + p * lda] = 0;
          for (size_t i = 0; i < m; i++) A[i + 5 * lda] = 0;
          for (size_t p = 0; p < k; p++) B[p + 7 * ldb] = 0;
          for (size_t j = 0; j < n; j++) B[10 + j * ldb] = 0;
          break;
        case 1: lab = "A~1e300 B~1e-300";
          for (size_t p = 0; p < k; p++) for (size_t i = 0; i < m; i++) A[i + p * lda] *= 1e300;
          for (size_t j = 0; j < n; j++) for (size_t p = 0; p < k; p++) B[p + j * ldb] *= 1e-300;
          break;
        case 2: lab = "A~1e-300 B~1e300";
          for (size_t p = 0; p < k; p++) for (size_t i = 0; i < m; i++) A[i + p * lda] *= 1e-300;
          for (size_t j = 0; j < n; j++) for (size_t p = 0; p < k; p++) B[p + j * ldb] *= 1e300;
          break;
        case 3: lab = "A subnormal B~1e300";
          for (size_t p = 0; p < k; p++) for (size_t i = 0; i < m; i++) A[i + p * lda] *= 1e-310;
          for (size_t j = 0; j < n; j++) for (size_t p = 0; p < k; p++) B[p + j * ldb] *= 1e300;
          break;
        case 4: lab = "A~1e150 B~1e150";
          for (size_t p = 0; p < k; p++) for (size_t i = 0; i < m; i++) A[i + p * lda] *= 1e150;
          for (size_t j = 0; j < n; j++) for (size_t p = 0; p < k; p++) B[p + j * ldb] *= 1e150;
          break;
        case 5: lab = "rows 2^+-500, cols 2^+-500";
          for (size_t i = 0; i < m; i++) { int e = (int)(rng_next(&s) % 1001) - 500; for (size_t p = 0; p < k; p++) A[i + p * lda] = ldexp(A[i + p * lda], e); }
          for (size_t j = 0; j < n; j++) { int e = (int)(rng_next(&s) % 1001) - 500; for (size_t p = 0; p < k; p++) B[p + j * ldb] = ldexp(B[p + j * ldb], e); }
          break;
        case 6: lab = "inner 2^+-400";
          for (size_t p = 0; p < k; p++) { int e = (int)(rng_next(&s) % 801) - 400;
            for (size_t i = 0; i < m; i++) A[i + p * lda] = ldexp(A[i + p * lda], e);
            for (size_t j = 0; j < n; j++) B[p + j * ldb] = ldexp(B[p + j * ldb], -e); }
          break;
        case 7: lab = "negative zeros";
          for (size_t p = 0; p < k; p++) for (size_t i = 0; i < m; i++) A[i + p * lda] = -0.0;
          break;
        case 8: lab = "exact cancel [U U][V;-V]";
          for (size_t p = 0; p < k / 2; p++) for (size_t i = 0; i < m; i++) A[i + (k / 2 + p) * lda] = A[i + p * lda];
          for (size_t j = 0; j < n; j++) for (size_t p = 0; p < k / 2; p++) B[k / 2 + p + j * ldb] = -B[p + j * ldb];
          for (size_t j = 0; j < n; j++) B[k - 1 + j * ldb] = 0;
          break;
        case 9: lab = "NaN in A"; A[17 + 33 * lda] = NAN; break;
        case 10: lab = "Inf in B, 0 row"; B[40 + 9 * ldb] = INFINITY; for (size_t i = 0; i < m; i++) A[i + 40 * lda] = 0; break;
        case 11: lab = "integers (exact in dgemm)";
          for (size_t p = 0; p < k; p++) for (size_t i = 0; i < m; i++) A[i + p * lda] = (double)((int)(rng_next(&s) % 2001) - 1000);
          for (size_t j = 0; j < n; j++) for (size_t p = 0; p < k; p++) B[p + j * ldb] = (double)((int)(rng_next(&s) % 2001) - 1000);
          break;
        case 12: lab = "row 1e-300 in big column";
          // column p of A big, row 3 of A tiny everywhere, B small: inner scaling pushes row 3 below 2^-1074
          for (size_t p = 0; p < k; p++) { A[0 + p * lda] = 1e150; A[3 + p * lda] = 1e-300 * (1 + 0.5 * rng_unif(&s)); }
          for (size_t j = 0; j < n; j++) for (size_t p = 0; p < k; p++) B[p + j * ldb] *= 1e100;
          break;
      }
      printf("-- class %d: %s\n", cls, lab);
      fails += check(lab, m, k, n, A, lda, B, ldb, ldc, 1e-13, 2e-15);
      if (cls == 7 || cls == 8) {  // sign of zero / exact zero
        double *C1 = mk(m, n, ldc, 0), *C2 = mk(m, n, ldc, 0);
        run_method("dgemm", m, k, n, A, lda, B, ldb, C1, ldc);
        run_method("oz16", m, k, n, A, lda, B, ldb, C2, ldc);
        size_t z1 = 0, z2 = 0, nz = 0, sgn = 0;
        for (size_t j = 0; j < n; j++) for (size_t i = 0; i < m; i++) {
          double a = C1[i + j * ldc], b = C2[i + j * ldc];
          z1 += a == 0; z2 += b == 0; nz += b != 0; sgn += (a == 0 && b == 0 && signbit(a) != signbit(b));
        }
        printf("   zeros: dgemm %zu, oz16 %zu, oz16 nonzero %zu, sign-of-zero mismatches %zu\n", z1, z2, nz, sgn);
        free(C1); free(C2);
      }
      cases++;
      free(A); free(B);
    }
  } else if (!strcmp(mode, "underflow")) {
    // minimal 2x1x1 example and a 64x200x64 one
    double A[2] = {1e150, 1e-300}, B[1] = {1e100}, C[2], D[2];
    oz_dgemm(16, 2, 1, 1, A, 2, B, 1, C, 2, NULL);
    dgemm_nn(2, 1, 1, 1.0, A, 2, B, 1, 0.0, D, 2);
    printf("A=[1e150;1e-300], B=[1e100]: dgemm C=[%.17g, %.17g]  oz16 C=[%.17g, %.17g]\n", D[0], D[1], C[0], C[1]);
    setenv("OZ_INNER", "0", 1);
    oz_dgemm(16, 2, 1, 1, A, 2, B, 1, C, 2, NULL);
    printf("  same with OZ_INNER=0: oz16 C=[%.17g, %.17g]\n", C[0], C[1]);
    unsetenv("OZ_INNER");
    double A2[2] = {1.0, 1e-300}, B2[1] = {1e-60};
    oz_dgemm(16, 2, 1, 1, A2, 2, B2, 1, C, 2, NULL);
    dgemm_nn(2, 1, 1, 1.0, A2, 2, B2, 1, 0.0, D, 2);
    printf("A=[1;1e-300], B=[1e-60]: dgemm C=[%.17g, %.17g]  oz16 C=[%.17g, %.17g]\n", D[0], D[1], C[0], C[1]);
  } else if (!strcmp(mode, "bound")) {
    // every s = 2..16, OZ_INNER=0, check |err| <= 4 sqrt(k) 2^-L ||a_i|| ||b_j|| + 2u|C| on all entries;
    // adversarial: B = +-A^T with row norms just below a power of two (Cauchy-Schwarz equality).
    setenv("OZ_INNER", "0", 1);
    for (int s = 2; s <= 16; s++)
      for (int cls = 0; cls < 4; cls++) {
        size_t m = 97, k = cls == 3 ? 4 : 300, n = 97;
        double *A = mk(m, k, m, 0), *B = mk(k, n, k, 0);
        fillu(A, m, k, m, s * 10 + cls);
        uint64_t sd = 1234 + s;
        if (cls == 1) for (size_t q = 0; q < m * k; q++) A[q] = ldexp(A[q], (int)(rng_next(&sd) % 41) - 20);
        if (cls >= 2) {  // constant rows c with ||row|| = 2^e (1 - 2^-28)
          for (size_t i = 0; i < m; i++) {
            double c = ldexp(1.0 - ldexp(1, -28), (int)i % 7 - 3) / sqrt((double)k);
            for (size_t p = 0; p < k; p++) A[i + p * m] = ((p + i) % 3 ? 1 : -1) * c;
          }
        }
        for (size_t j = 0; j < n; j++)  // B = A^T (or -A^T)
          for (size_t p = 0; p < k; p++) B[p + j * k] = (cls == 2 && j % 2 ? -1 : 1) * A[j + p * m];
        double *C = mk(m, n, m, 0), *R = malloc(m * n * 8), *Rl = malloc(m * n * 8), *AB = malloc(m * n * 8);
        ref(m, k, n, A, m, B, k, R, Rl, AB);
        oz_dgemm(s, m, k, n, A, m, B, k, C, m, NULL);
        int L = oz_bits(s, k);
        double worst = 0;
        for (size_t j = 0; j < n; j++) {
          double nb = 0; for (size_t p = 0; p < k; p++) nb += B[p + j * k] * B[p + j * k];
          for (size_t i = 0; i < m; i++) {
            double na = 0; for (size_t p = 0; p < k; p++) na += A[i + p * m] * A[i + p * m];
            double bnd = 4 * sqrt((double)k) * ldexp(1, -L) * sqrt(na) * sqrt(nb) * 1.01 + 2.3e-16 * fabs(R[i + j * m]);
            double e = fabs((C[i + j * m] - R[i + j * m]) - Rl[i + j * m]);
            if (e / bnd > worst) worst = e / bnd;
          }
        }
        printf("bound s=%2d L=%2d class=%d k=%zu: max err/bound = %.3g %s\n", s, L, cls, k, worst, worst <= 1 ? "ok" : "FAIL");
        fails += worst > 1; cases++;
        free(A); free(B); free(C); free(R); free(Rl); free(AB);
      }
  } else if (!strcmp(mode, "nondet")) {
    // inner scaling: rb[p] (sum over columns of B^2) is reduced across threads in a critical section, in
    // arrival order.  Make rb[0]/ca[0] straddle 4 (the lround(0.25*log2) boundary) depending on that order.
    size_t m = 64, k = 128, n = 64;
    double *A = mk(m, k, m, 0), *B = mk(k, n, k, 0);
    fillu(A, m, k, m, 3); fillu(B, k, n, k, 4);
    double ca = 0;  // make ca[0] = 1 exactly: column 0 of A = (1, 0, ...)
    for (size_t i = 0; i < m; i++) A[i] = i == 0 ? 1.0 : 0.0;
    ca = 1.0;
    // row 0 of B: one entry per thread chunk (static schedule, 16 columns per thread)
    for (size_t j = 0; j < n; j++) B[0 + j * k] = 0;
    double x0 = nextafter(2.0, 0.0);              // x0^2 rounds to 4 - 2^-50 (2 ulps below 4)
    double t = sqrt(0.6 * ldexp(1.0, -51));        // t^2 ~ 0.6 ulp
    B[0 + 0 * k] = x0; B[0 + 16 * k] = t; B[0 + 32 * k] = t;
    double l0 = x0 * x0, l1 = t * t;
    printf("loc values: %.17g %.17g; (l0+l1)+l1 = %.17g, (l1+l1)+l0 = %.17g, l1+(l0+l1)=%.17g (ca=%g)\n", l0, l1,
           (l0 + l1) + l1, (l1 + l1) + l0, l1 + (l0 + l1), ca);
    double *C0 = mk(m, n, m, 0), *C1 = mk(m, n, m, 0);
    oz_dgemm(14, m, k, n, A, m, B, k, C0, m, NULL);
    int differ_runs = 0;
    for (int rep = 0; rep < 400; rep++) {
      oz_dgemm(14, m, k, n, A, m, B, k, C1, m, NULL);
      if (memcmp(C0, C1, m * n * 8)) {
        differ_runs++;
        if (differ_runs == 1) {
          size_t ne = 0; double mx = 0;
          for (size_t q = 0; q < m * n; q++) if (C0[q] != C1[q]) { ne++; double r = fabs(C0[q] - C1[q]) / fabs(C0[q]); if (r > mx) mx = r; }
          printf("first differing run: %zu of %zu entries differ, max relative difference %.2e\n", ne, m * n, mx);
        }
      }
    }
    printf("oz14: %d of 400 repeated runs differ bitwise from the first run\n", differ_runs);
    int dw = 0;
    for (int rep = 0; rep < 200; rep++) {
      oz_dgemm(14, m, k, n, A, m, B, k, C0, m, NULL);
      oz_dgemm_w(14, m, k, n, A, m, B, k, C1, m, NULL);
      dw += memcmp(C0, C1, m * n * 8) != 0;
    }
    printf("oz14 vs ozw14: %d of 200 paired runs differ bitwise\n", dw);
  }
  printf("%s: %d cases, %d method failures\n", mode, cases, fails);
  return fails != 0;
}
