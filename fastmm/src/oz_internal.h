// Helpers shared by ozaki.c (plain emulation) and ozfmm.c (emulation with a fast bilinear
// scheme applied exactly in the modular domain).
#pragma once
#include <immintrin.h>
#include <stdio.h>
#include <sys/mman.h>
#include "common.h"
#include "oz_consts.h"

int8_t *oz_workspace(size_t bytes);  // persistent buffer pool, defined in ozaki.c

static void *hp_alloc(size_t bytes) {
  void *p = amalloc(bytes);
  if (p) madvise(p, (bytes + (2u << 20) - 1) / (2u << 20) * (2u << 20), MADV_HUGEPAGE);
  return p;
}


#define L_ZERO_ROW 0  // zero rows/cols: any scaling works (all residues are 0)

// Largest L with (2^L + sqrt(k)/2)^2 <= P/4 (with a relative safety margin).
static int oz_bits(int s, size_t k) {
  double sq = exp2((oz_const[s].log2P - 2.0) / 2.0) * (1.0 - 1e-9) - 0.5 * sqrt((double)k);
  return (int)floor(log2(sq));
}

// Exponent e with ||x||_2 < 2^e (e = INT_MIN-ish marker for a zero vector).
static int norm_exp(double ss_scaled, int scale_exp) {
  if (ss_scaled == 0.0) return L_ZERO_ROW;
  int e;
  frexp(sqrt(ss_scaled) * (1.0 + 1e-9), &e);  // sqrt(ss) < 2^e
  return e + scale_exp;
}

// Overflow/underflow-safe version: scale by the max-abs exponent first. Zero vector -> 0.
static int safe_norm_exp(const double *x, size_t inc, size_t len) {
  double mx = 0;
  for (size_t p = 0; p < len; p++) mx = fmax(mx, fabs(x[p * inc]));
  if (mx == 0 || !isfinite(mx)) return L_ZERO_ROW;
  int em;
  frexp(mx, &em);
  double ss = 0;
  for (size_t p = 0; p < len; p++) { double v = ldexp(x[p * inc], -em); ss += v * v; }
  return norm_exp(ss, em);
}

// ---- residues -------------------------------------------------------------------------
// Inputs are exact integers x (|x| < 2^63) held in doubles.  We split x = xh*2^32 + xl
// (|xh|, |xl| <= 2^31, both exact) once, and per modulus form v = xh*c + xl*q with
// c = (2^32 q) mod p, which is exact (|v| < 2^40) and congruent to x*q.  Then
// t = floor(v/p + 1/2) is computed exactly by one FMA (for odd p, v/p + 1/2 is never closer
// than 1/(2p) to an integer, far above the 2^-19 rounding error; p = 256 is exact) and
// r = v - t*p lies in [-p/2, p/2), i.e. in int8 range.
typedef struct { __m512d hi, lo; } split_t;
static inline split_t split32(__m512d x) {
  split_t s;
  s.hi = _mm512_roundscale_pd(_mm512_mul_pd(x, _mm512_set1_pd(0x1p-32)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  s.lo = _mm512_fnmadd_pd(s.hi, _mm512_set1_pd(0x1p32), x);
  return s;
}
// 8 residues as int32 in a ymm
static inline __m256i resid_split(split_t x, double c, double q, double p, double invp) {
  __m512d v = q == 1.0 ? x.lo : _mm512_mul_pd(x.lo, _mm512_set1_pd(q));
  v = _mm512_fmadd_pd(x.hi, _mm512_set1_pd(c), v);
  __m512d t = _mm512_fmadd_pd(v, _mm512_set1_pd(invp), _mm512_set1_pd(0.5));
  t = _mm512_roundscale_pd(t, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
  v = _mm512_fnmadd_pd(t, _mm512_set1_pd(p), v);
  return _mm512_cvtpd_epi32(v);
}
static inline __m128i pack16(__m256i lo, __m256i hi) {
  return _mm512_cvtepi32_epi8(_mm512_inserti64x4(_mm512_castsi256_si512(lo), hi, 1));
}

typedef struct { double p, invp, c, q; } modc_t;
static void mod_consts(int s, int with_q, modc_t *mc) {
  for (int l = 0; l < s; l++) {
    int p = oz_mod[l], q = with_q ? oz_const[s].q[l] : 1;
    long c = (long)(((unsigned long)1 << 32) % (unsigned long)p) * q % p;
    mc[l] = (modc_t){(double)p, 1.0 / p, (double)c, (double)q};
  }
}

