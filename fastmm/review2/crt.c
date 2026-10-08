// Exhaustive-ish check of the CRT low-part error of reconstruct() (s = 16) against the
// 2^(2L-80) term used in the certificate's E_ij.  Includes ozaki.c to reach its static functions.
#include "../src/ozaki.c"
#include <stdint.h>

typedef __int128 i128;
static uint64_t rs = 88172645463325252ULL;
static uint64_t xr(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }

int main(int argc, char **argv) {
  int s = 16;
  long N = argc > 1 ? atol(argv[1]) : 2000000;
  int L = oz_bits(s, 8192);
  i128 P = 1;
  for (int l = 0; l < s; l++) P *= oz_mod[l];
  i128 P4 = P / 4;
  size_t m = 8;  // 8 values per column, 1 column per trial batch
  uint8_t *Y[16];
  for (int l = 0; l < s; l++) Y[l] = malloc(m);
  int sig[8] = {0}, tau[1] = {0};
  double C[8];
  double worst_abs_small = 0, worst_rel_excess = 0;
  long cnt_exceed = 0, nmismatch = 0; int mode_of[8]; long double worst_low[3] = {0, 0, 0};
  for (long t = 0; t < N; t++) {
    i128 cb[8];
    for (int q = 0; q < 8; q++) {
      i128 v;
      int mode = (t + q) % 3;
      if (mode == 0) {  // full range
        v = (i128)((((unsigned __int128)xr() << 64) | xr()) % (unsigned __int128)(2 * P4 + 1)) - P4;
      } else if (mode == 1) {  // small values: the final rounding u|c| is tiny, the low part dominates
        v = (i128)(int64_t)(xr() >> 4) - (i128)(1LL << 59);
      } else {
        v = (i128)((int64_t)(xr() % 2001) - 1000);
      }
      cb[q] = v; mode_of[q] = mode;
      for (int l = 0; l < s; l++) {
        i128 r = ((v % oz_mod[l]) * oz_const[s].q[l]) % oz_mod[l];
        if (r < 0) r += oz_mod[l];
        Y[l][q] = (uint8_t)r;
      }
    }
    reconstruct(s, m, 1, Y, m, sig, tau, C, m);
    for (int q = 0; q < 8; q++) {
      double c = C[q];
      double fl = floor(c);
      i128 ci = (i128)fl;
      long double diff = (long double)(ci - cb[q]) + (long double)(c - fl);
      double ad = fabsl(diff);
      double allowed = ldexp(fabs(c), -53) + ldexp(1.0, 2 * L - 80);
      if (ad > allowed) cnt_exceed++;
      double lowpart = ad - ldexp(fabs(c), -53);
      {  // scalar replica of reconstruct (one lane), split into H and s = fl(M+L)
        const oz_const_t *K = &oz_const[s];
        double x = 0, sh = 0, sm = 0, sl = 0;
        for (int l = 0; l < s; l++) { double y = Y[l][q]; x = fma(y, K->invp[l], x); sh = fma(y, K->wh[l], sh); sm = fma(y, K->wm[l], sm); sl = fma(y, K->wl[l], sl); }
        double r = nearbyint(x), H = fma(-r, K->Ph, sh), M = fma(-r, K->Pm, sm), Lw = fma(-r, K->Pl, sl);
        double sML = M + Lw, cb2 = H + sML;
        if (memcmp(&cb2, &c, 8)) nmismatch++;
        // exact |H + sML - Cbar|: H is an integer < 2^127
        i128 Hi = (i128)H;
        double fl2 = floor(sML); i128 si = (i128)fl2;
        long double lowerr = fabsl((long double)(Hi + si - cb[q]) + (long double)(sML - fl2));
        if (lowerr > worst_low[mode_of[q]]) worst_low[mode_of[q]] = lowerr;
      }
      if (lowpart > worst_abs_small) worst_abs_small = lowpart;
      if (ad / allowed > worst_rel_excess) worst_rel_excess = ad / allowed;
    }
  }
  printf("s=%d L=%d  2^(2L-80)=2^%d  trials=%ld\n", s, L, 2 * L - 80, N * 8);
  printf("max(|cb-Cbar| - u|cb|) = %.4g = 2^%.2f\n", worst_abs_small, log2(worst_abs_small));
  printf("entries with |cb-Cbar| > u|cb| + 2^(2L-80): %ld   worst ratio %.3f\n", cnt_exceed, worst_rel_excess);
  printf("scalar replica mismatches: %ld\n", nmismatch);
  for (int md = 0; md < 3; md++) printf("mode %d (0 full range, 1 |C|<2^59, 2 |C|<=1000): max |H + fl(M+L) - Cbar| = 2^%.3f\n", md, (double)log2l(worst_low[md]));
  return 0;
}
