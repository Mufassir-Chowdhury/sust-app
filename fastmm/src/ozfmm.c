// FP64 GEMM emulation (Ozaki scheme II style, see ozaki.c) in which every modular product
// Ai*Bi mod p_l is itself computed with one level of a fast bilinear scheme (Strassen-
// Winograd, or any scheme with integer coefficients given as an SLP), *exactly* in Z/p_l.
// Because the scheme is an exact identity over any commutative ring, the integer product
// recovered by the CRT -- and hence the final double result -- is bit-identical to the plain
// emulation with the same moduli: the fast algorithm costs no accuracy at all here.
//
// Layout: A (m x k) is split into M x K blocks of mb x kb, B into K x N blocks of kb x nb
// (mb = ceil(m/M) etc.; entries outside the matrix are zero).  The conversion pass produces,
// per modulus, the r left factors (from A) and r right factors (from B) of the scheme as
// packed int8 AMX operands; r AMX GEMMs give the r products mod p; the reconstruction pass
// evaluates the scheme's output combinations mod p and then the CRT.
#include <omp.h>
#include "oz_internal.h"
#include "amx.h"
#include "slp.h"
#include "ozaki.h"

#define MAXB 16    // max blocks per operand (M*K, K*N, M*N)
#define MAXR 64    // max rank
#define MAXV 160   // max SLP variables

static inline __m512d red_sym(__m512d v, __m512d vp, __m512d vi) {
  // v: exact small integer; returns v mod p in [-p/2, p/2)
  __m512d t = _mm512_roundscale_pd(_mm512_fmadd_pd(v, vi, _mm512_set1_pd(0.5)), _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
  return _mm512_fnmadd_pd(t, vp, v);
}
static inline __m512d resid_pd(split_t x, double c, double q, double p, double invp) {
  __m512d v = q == 1.0 ? x.lo : _mm512_mul_pd(x.lo, _mm512_set1_pd(q));
  v = _mm512_fmadd_pd(x.hi, _mm512_set1_pd(c), v);
  __m512d t = _mm512_fmadd_pd(v, _mm512_set1_pd(invp), _mm512_set1_pd(0.5));
  t = _mm512_roundscale_pd(t, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
  return _mm512_fnmadd_pd(t, _mm512_set1_pd(p), v);
}

// Evaluate an SLP in the residue domain on one 8-lane vector per input.  out[o] in [-p/2, p/2).
static inline void slp_mod(const gslp *s, const __m512d *in, __m512d *out, __m512d vp, __m512d vi) {
  __m512d v[MAXV];
  for (int i = 0; i < s->nin; i++) v[i] = in[i];
  for (int t = 0; t < s->ninstr; t++) {
    const ginstr *I = &s->ins[t];
    __m512d acc = _mm512_mul_pd(_mm512_set1_pd(I->coef[0]), v[I->src[0]]);
    for (int q = 1; q < I->nt; q++) acc = _mm512_fmadd_pd(_mm512_set1_pd(I->coef[q]), v[I->src[q]], acc);
    v[s->nin + t] = red_sym(acc, vp, vi);
  }
  for (int o = 0; o < s->nout; o++) {
    __m512d x = v[s->ovar[o]];
    double c = s->oscale[o];
    out[o] = c == 1.0 ? x : red_sym(_mm512_mul_pd(_mm512_set1_pd(c), x), vp, vi);
  }
}

// Same in the byte domain: residues in [lo, lo+p) (lo = -floor(p/2)) held in 32 int16 lanes.
static inline __m512i red16(__m512i v, int p, int steps) {
  const __m512i vp = _mm512_set1_epi16((short)p), hi = _mm512_set1_epi16((short)(p - 1 - p / 2)),
                lo = _mm512_set1_epi16((short)(-(p / 2)));
  for (int t = 0; t < steps; t++) {
    v = _mm512_mask_sub_epi16(v, _mm512_cmpgt_epi16_mask(v, hi), v, vp);
    v = _mm512_mask_add_epi16(v, _mm512_cmplt_epi16_mask(v, lo), v, vp);
  }
  return v;
}
static inline void slp_mod16(const gslp *s, const __m512i *in, __m512i *out, int p) {
  __m512i v[MAXV];
  for (int i = 0; i < s->nin; i++) v[i] = in[i];
  for (int t = 0; t < s->ninstr; t++) {
    const ginstr *I = &s->ins[t];
    __m512i acc = _mm512_setzero_si512();
    int sumc = 0;
    for (int q = 0; q < I->nt; q++) {
      int c = (int)I->coef[q];
      __m512i x = v[I->src[q]];
      if (c == 1) acc = _mm512_add_epi16(acc, x);
      else if (c == -1) acc = _mm512_sub_epi16(acc, x);
      else acc = _mm512_add_epi16(acc, _mm512_mullo_epi16(x, _mm512_set1_epi16((short)c)));
      sumc += c < 0 ? -c : c;
    }
    v[s->nin + t] = red16(acc, p, (sumc + 1) / 2);
  }
  for (int o = 0; o < s->nout; o++) {
    int c = (int)s->oscale[o];
    __m512i x = v[s->ovar[o]];
    if (c == 1) out[o] = x;
    else {
      x = c == -1 ? _mm512_sub_epi16(_mm512_setzero_si512(), x) : _mm512_mullo_epi16(x, _mm512_set1_epi16((short)c));
      out[o] = red16(x, p, ((c < 0 ? -c : c) + 1) / 2);
    }
  }
}
// widen 64 int8 -> two int16 vectors; narrow back
static inline void widen(__m512i b, __m512i *lo, __m512i *hi) {
  *lo = _mm512_cvtepi8_epi16(_mm512_castsi512_si256(b));
  *hi = _mm512_cvtepi8_epi16(_mm512_extracti64x4_epi64(b, 1));
}
static inline __m512i narrow(__m512i lo, __m512i hi) {
  return _mm512_inserti64x4(_mm512_castsi256_si512(_mm512_cvtepi16_epi8(lo)), _mm512_cvtepi16_epi8(hi), 1);
}

static int scheme_is_integral(const gen_scheme *g) {
  for (int sec = 0; sec < 3; sec++) {
    const gslp *s = &g->s[sec];
    for (int t = 0; t < s->ninstr; t++)
      for (int q = 0; q < s->ins[t].nt; q++) if (s->ins[t].coef[q] != rint(s->ins[t].coef[q])) return 0;
    for (int o = 0; o < s->nout; o++) if (s->oscale[o] != rint(s->oscale[o])) return 0;
  }
  for (int l = 0; l < g->r; l++) if (g->pscale[l] != 1.0) return 0;
  return 1;
}

typedef struct {
  size_t m, k, n, mb, kb, nb, mbp, kbp, nbp;  // true and padded block sizes
  int M, K, N, r;
} geom_t;

// ---- A side: left factors as VNNI operands (plane dims mbp x kbp) --------------------------
static void pack_left(int s, int L, const geom_t *G, const gen_scheme *g, const double *A, size_t lda, const double *ek,
                      int *sig, int8_t **out /* [l * r + prod] */) {
  const int M = G->M, K = G->K, r = G->r, MK = M * K;
  size_t nkb = G->kbp / 64, ni16 = G->mbp / 16;
  modc_t mc[OZ_SMAX];
  mod_consts(s, 1, mc);
  uint8_t idx[64];
  for (int c = 0; c < 16; c++)
    for (int q = 0; q < 4; q++) idx[c * 4 + q] = (uint8_t)(q * 16 + c);
  const __m512i perm = _mm512_loadu_si512(idx);
  #pragma omp parallel
  {
    #pragma omp for schedule(dynamic, 1)
    for (size_t i16 = 0; i16 < ni16; i16++) {
      size_t i0 = i16 * 16;
      __mmask8 mk[MAXB][2];
      const double *base[MAXB];
      __m512d sc[MAXB][2];
      // row masks per block row, norms of the M*16 rows
      for (int bi = 0; bi < M; bi++) {
        for (int h = 0; h < 2; h++) {
          __mmask8 mm = 0;
          for (int c = 0; c < 8; c++) {
            size_t il = i0 + 8 * h + c, ig = bi * G->mb + il;
            if (il < G->mb && ig < G->m) mm |= (__mmask8)(1u << c);
          }
          mk[bi][h] = mm;
        }
        __m512d a0 = _mm512_setzero_pd(), a1 = a0;
        const double *rowp = A + bi * G->mb + i0;
        for (size_t p = 0; p < G->k; p++) {
          __m512d e = _mm512_set1_pd(ek[p]);
          __m512d d0 = _mm512_scalef_pd(_mm512_maskz_loadu_pd(mk[bi][0], rowp + p * lda), e);
          __m512d d1 = _mm512_scalef_pd(_mm512_maskz_loadu_pd(mk[bi][1], rowp + 8 + p * lda), e);
          a0 = _mm512_fmadd_pd(d0, d0, a0);
          a1 = _mm512_fmadd_pd(d1, d1, a1);
        }
        double ss[16], t[16];
        _mm512_storeu_pd(ss, a0); _mm512_storeu_pd(ss + 8, a1);
        for (int c = 0; c < 16; c++) {
          size_t il = i0 + c, ig = bi * G->mb + il;
          if (il >= G->mb || ig >= G->m) { t[c] = 0; continue; }
          int e = (isfinite(ss[c]) && ss[c] > 1e-280) ? L - norm_exp(ss[c], 0) : L - safe_norm_exp_sc(A + ig, lda, G->k, ek, 1);
          sig[ig] = e;
          t[c] = (double)e;
        }
        sc[bi][0] = _mm512_loadu_pd(t); sc[bi][1] = _mm512_loadu_pd(t + 8);
      }
      for (int bi = 0; bi < M; bi++)
        for (int bk = 0; bk < K; bk++) base[bi * K + bk] = A + bi * G->mb + i0 + bk * G->kb * lda;
      for (size_t kb = 0; kb < nkb; kb++)
        for (int rr = 0; rr < 16; rr++) {
          split_t x[4][MAXB][2];  // [q][block][half]
          for (int q = 0; q < 4; q++) {
            size_t kk = kb * 64 + 4 * rr + q;
            for (int bi = 0; bi < M; bi++)
              for (int bk = 0; bk < K; bk++) {
                int b = bi * K + bk;
                int ok = kk < G->kb && bk * G->kb + kk < G->k;
                __m512d e = _mm512_set1_pd(ok ? ek[bk * G->kb + kk] : 0.0);
                for (int h = 0; h < 2; h++) {
                  __m512d d = ok ? _mm512_maskz_loadu_pd(mk[bi][h], base[b] + 8 * h + kk * lda) : _mm512_setzero_pd();
                  d = _mm512_roundscale_pd(_mm512_scalef_pd(d, _mm512_add_pd(sc[bi][h], e)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
                  x[q][b][h] = split32(d);
                }
              }
          }
          for (int l = 0; l < s; l++) {
            // 64-byte VNNI tile rows of every block, then the scheme's left factors in int16
            __m512i blo[MAXB], bhi[MAXB], flo[MAXR], fhi[MAXR];
            for (int b = 0; b < MK; b++) {
              __m512i w = _mm512_setzero_si512();
              __m128i ln[4];
              for (int q = 0; q < 4; q++)
                ln[q] = pack16(resid_split(x[q][b][0], mc[l].c, mc[l].q, mc[l].p, mc[l].invp),
                               resid_split(x[q][b][1], mc[l].c, mc[l].q, mc[l].p, mc[l].invp));
              w = _mm512_castsi128_si512(ln[0]);
              w = _mm512_inserti32x4(w, ln[1], 1);
              w = _mm512_inserti32x4(w, ln[2], 2);
              w = _mm512_inserti32x4(w, ln[3], 3);
              widen(_mm512_permutexvar_epi8(perm, w), &blo[b], &bhi[b]);
            }
            slp_mod16(&g->s[0], blo, flo, (int)mc[l].p);
            slp_mod16(&g->s[0], bhi, fhi, (int)mc[l].p);
            for (int pr = 0; pr < r; pr++)
              _mm512_stream_si512((void *)(out[l * r + pr] + amx_tile_off(i16, kb, G->kbp) + rr * 64), narrow(flo[pr], fhi[pr]));
          }
        }
    }
    _mm_sfence();
  }
}

// ---- B side: right factors as AMX "A" operands (plane dims nbp x kbp) -----------------------
static void pack_right(int s, int L, const geom_t *G, const gen_scheme *g, const double *B, size_t ldb, const double *ek,
                       int *tau, int8_t **out) {
  const int K = G->K, N = G->N, r = G->r, KN = K * N;
  size_t nkb = G->kbp / 64;
  modc_t mc[OZ_SMAX];
  mod_consts(s, 0, mc);
  #pragma omp parallel
  {
    #pragma omp for schedule(dynamic, 4)
    for (size_t j = 0; j < G->nbp; j++) {
      size_t j16 = j / 16, rr = j % 16;
      __m512d vt[MAXB];
      const double *col[MAXB];
      int valid[MAXB];
      for (int bj = 0; bj < N; bj++) {
        size_t jg = bj * G->nb + j;
        int ok = j < G->nb && jg < G->n;
        int tj = 0;
        if (ok) {
          const double *c = B + jg * ldb;
          __m512d acc = _mm512_setzero_pd();
          size_t p = 0;
          for (; p + 8 <= G->k; p += 8) {
            __m512d d = _mm512_scalef_pd(_mm512_loadu_pd(c + p), _mm512_sub_pd(_mm512_setzero_pd(), _mm512_loadu_pd(ek + p)));
            acc = _mm512_fmadd_pd(d, d, acc);
          }
          if (p < G->k) {
            __m512d d = _mm512_maskz_loadu_pd((__mmask8)((1u << (G->k - p)) - 1), c + p);
            d = _mm512_scalef_pd(d, _mm512_sub_pd(_mm512_setzero_pd(), _mm512_loadu_pd(ek + p)));
            acc = _mm512_fmadd_pd(d, d, acc);
          }
          double ss = _mm512_reduce_add_pd(acc);
          tj = (isfinite(ss) && ss > 1e-280) ? L - norm_exp(ss, 0) : L - safe_norm_exp_sc(c, 1, G->k, ek, -1);
          tau[jg] = tj;
        }
        for (int bk = 0; bk < K; bk++) {
          col[bk * N + bj] = ok ? B + jg * ldb + bk * G->kb : NULL;
          valid[bk * N + bj] = ok;
          vt[bk * N + bj] = _mm512_set1_pd((double)tj);
        }
      }
      for (size_t kb = 0; kb < nkb; kb++) {
        split_t x[MAXB][8];
        for (int b = 0; b < KN; b++) {
          int bk = b / N;
          for (int v = 0; v < 8; v++) {
            size_t kl = kb * 64 + v * 8;  // local k within block
            __mmask8 mm = 0;
            if (valid[b])
              for (int c = 0; c < 8; c++)
                if (kl + c < G->kb && bk * G->kb + kl + c < G->k) mm |= (__mmask8)(1u << c);
            __m512d d = mm ? _mm512_maskz_loadu_pd(mm, col[b] + kl) : _mm512_setzero_pd();
            __m512d ev = _mm512_maskz_loadu_pd(mm, ek + bk * G->kb + kl);  // global k indices
            d = _mm512_roundscale_pd(_mm512_scalef_pd(d, _mm512_sub_pd(vt[b], ev)), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
            x[b][v] = split32(d);
          }
        }
        for (int l = 0; l < s; l++) {
          __m512i blo[MAXB], bhi[MAXB], flo[MAXR], fhi[MAXR];
          for (int b = 0; b < KN; b++) {
            __m512i w = _mm512_setzero_si512();
            __m128i ln[4];
            for (int v = 0; v < 4; v++)
              ln[v] = pack16(resid_split(x[b][2 * v], mc[l].c, 1.0, mc[l].p, mc[l].invp),
                             resid_split(x[b][2 * v + 1], mc[l].c, 1.0, mc[l].p, mc[l].invp));
            w = _mm512_castsi128_si512(ln[0]);
            w = _mm512_inserti32x4(w, ln[1], 1);
            w = _mm512_inserti32x4(w, ln[2], 2);
            w = _mm512_inserti32x4(w, ln[3], 3);
            widen(w, &blo[b], &bhi[b]);
          }
          slp_mod16(&g->s[1], blo, flo, (int)mc[l].p);
          slp_mod16(&g->s[1], bhi, fhi, (int)mc[l].p);
          for (int pr = 0; pr < r; pr++)
            _mm512_stream_si512((void *)(out[l * r + pr] + amx_tile_off(j16, kb, G->kbp) + rr * 64), narrow(flo[pr], fhi[pr]));
        }
      }
    }
    _mm_sfence();
  }
}

// ---- epilogue: product mod p -> uint8 plane (dims mbp x nb, column-major) -----------------
typedef struct { uint8_t *Y; size_t ldy, rows, cols; double p, invp; } epi2_ctx;
static void epi2(const int32_t *blk, size_t ld, size_t r0, size_t c0, void *vctx) {
  epi2_ctx *e = vctx;  // blk rows = product columns (r0 + rr), blk cols = product rows (c0 + cc)
  const __m512d vp = _mm512_set1_pd(e->p), vi = _mm512_set1_pd(e->invp), zero = _mm512_setzero_pd();
  for (size_t rr = 0; rr < 32; rr++) {
    size_t j = r0 + rr;
    if (j >= e->cols) break;
    __m128i o[4];
    for (int h = 0; h < 4; h++) {
      __m512d d = _mm512_cvtepi32_pd(_mm256_load_si256((const __m256i *)(blk + rr * ld + 8 * h)));
      __m512d t = _mm512_roundscale_pd(_mm512_mul_pd(d, vi), _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
      d = _mm512_fnmadd_pd(t, vp, d);
      d = _mm512_mask_add_pd(d, _mm512_cmp_pd_mask(d, zero, _CMP_LT_OQ), d, vp);
      d = _mm512_mask_sub_pd(d, _mm512_cmp_pd_mask(d, vp, _CMP_GE_OQ), d, vp);
      o[h] = _mm512_cvtepi64_epi8(_mm512_cvtpd_epi64(d));
    }
    __m256i y = _mm256_set_m128i(_mm_unpacklo_epi64(o[2], o[3]), _mm_unpacklo_epi64(o[0], o[1]));
    _mm256_storeu_si256((__m256i *)(e->Y + c0 + j * e->ldy), y);  // rows padded to mbp: in bounds
  }
}

// ---- reconstruction: output combinations mod p, then CRT ---------------------------------
static void reconstruct2(int s, const geom_t *G, const gen_scheme *g, uint8_t **Y /* [l*r+pr] */, size_t ldy,
                         const int *sig, const int *tau, double *C, size_t ldc) {
  const oz_const_t *Kc = &oz_const[s];
  const int M = G->M, N = G->N, r = G->r, MN = M * N;
  size_t nchunk = (G->mb + 63) / 64;
  #pragma omp parallel
  {
    uint8_t ys[MAXB][OZ_SMAX][64] __attribute__((aligned(64)));
    #pragma omp for schedule(dynamic, 4) collapse(2)
    for (size_t j = 0; j < G->nb; j++)
      for (size_t ch = 0; ch < nchunk; ch++) {
        size_t i0 = ch * 64;
        // 1) output residues of every modulus for 64 rows, via the scheme's C program
        for (int l = 0; l < s; l++) {
          int p = oz_mod[l];
          __m512i lo16[MAXR], hi16[MAXR], olo[MAXB], ohi[MAXB];
          const __m512i vhi = _mm512_set1_epi16((short)(p - 1 - p / 2)), vp = _mm512_set1_epi16((short)p);
          for (int pr = 0; pr < r; pr++) {
            __m512i b = _mm512_loadu_si512((const void *)(Y[l * r + pr] + i0 + j * ldy));  // in [0,p)
            __m512i a0 = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(b)), a1 = _mm512_cvtepu8_epi16(_mm512_extracti64x4_epi64(b, 1));
            lo16[pr] = _mm512_mask_sub_epi16(a0, _mm512_cmpgt_epi16_mask(a0, vhi), a0, vp);  // to symmetric
            hi16[pr] = _mm512_mask_sub_epi16(a1, _mm512_cmpgt_epi16_mask(a1, vhi), a1, vp);
          }
          slp_mod16(&g->s[2], lo16, olo, p);
          slp_mod16(&g->s[2], hi16, ohi, p);
          for (int o = 0; o < MN; o++) {
            __m512i x0 = _mm512_mask_add_epi16(olo[o], _mm512_cmplt_epi16_mask(olo[o], _mm512_setzero_si512()), olo[o], vp);
            __m512i x1 = _mm512_mask_add_epi16(ohi[o], _mm512_cmplt_epi16_mask(ohi[o], _mm512_setzero_si512()), ohi[o], vp);
            _mm512_store_si512((void *)ys[o][l], narrow(x0, x1));
          }
        }
        // 2) CRT for each output block, 8 rows at a time
        for (int bi = 0; bi < M; bi++)
          for (int bj = 0; bj < N; bj++) {
            int o = bi * N + bj;
            size_t jg = bj * G->nb + j;
            if (jg >= G->n) continue;
            for (size_t i = i0; i < i0 + 64 && i < G->mb; i += 8) {
              size_t ig0 = bi * G->mb + i;
              if (ig0 >= G->m) break;
              __m512d x = _mm512_setzero_pd(), sh = x, sm = x, sl = x;
              for (int l = 0; l < s; l++) {
                __m512d y = _mm512_cvtepi64_pd(_mm512_cvtepu8_epi64(_mm_loadl_epi64((const __m128i *)(ys[o][l] + (i - i0)))));
                x = _mm512_fmadd_pd(y, _mm512_set1_pd(Kc->invp[l]), x);
                sh = _mm512_fmadd_pd(y, _mm512_set1_pd(Kc->wh[l]), sh);
                sm = _mm512_fmadd_pd(y, _mm512_set1_pd(Kc->wm[l]), sm);
                sl = _mm512_fmadd_pd(y, _mm512_set1_pd(Kc->wl[l]), sl);
              }
              size_t lim = G->m - ig0 < 8 ? G->m - ig0 : 8;
              if (G->mb - i < lim) lim = G->mb - i;
              __mmask8 mk = (__mmask8)((1u << lim) - 1);
              __m512d rr = _mm512_roundscale_pd(x, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
              __m512d H = _mm512_fnmadd_pd(rr, _mm512_set1_pd(Kc->Ph), sh);
              __m512d Mm = _mm512_fnmadd_pd(rr, _mm512_set1_pd(Kc->Pm), sm);
              __m512d Lo = _mm512_fnmadd_pd(rr, _mm512_set1_pd(Kc->Pl), sl);
              __m512d cb = _mm512_add_pd(H, _mm512_add_pd(Mm, Lo));
              __m256i si = _mm256_maskz_loadu_epi32(mk, sig + ig0);
              __m512d e = _mm512_cvtepi32_pd(_mm256_add_epi32(si, _mm256_set1_epi32(tau[jg])));
              _mm512_mask_storeu_pd(C + ig0 + jg * ldc, mk, _mm512_scalef_pd(cb, _mm512_sub_pd(_mm512_setzero_pd(), e)));
            }
          }
      }
  }
}

void oz_dgemm_fmm(int s, const gen_scheme *g, size_t m, size_t k, size_t n, const double *A, size_t lda,
                  const double *B, size_t ldb, double *C, size_t ldc, oz_times *tm) {
  static int inited = 0;
  if (!inited) { amx_init(); inited = 1; }
  if (!scheme_is_integral(g) || g->M * g->K > MAXB || g->K * g->N > MAXB || g->M * g->N > MAXB || g->r > MAXR ||
      g->s[0].nin + g->s[0].ninstr > MAXV || g->s[1].nin + g->s[1].ninstr > MAXV || g->s[2].nin + g->s[2].ninstr > MAXV) {
    fprintf(stderr, "oz_dgemm_fmm: unsupported scheme\n");
    exit(1);
  }
  if (k > 131071) { oz_dgemm(s, m, k, n, A, lda, B, ldb, C, ldc, tm); return; }  // int32 range: see ozaki.c
  double t0 = now_sec();
  int L = oz_bits(s, k);
  if (s < 2 || s > 16 || L > 62) { fprintf(stderr, "oz_dgemm_fmm: need 2 <= s <= 16\n"); exit(1); }
  oz_nonfinite_seen = 0;
  geom_t G = {m, k, n, 0, 0, 0, 0, 0, 0, g->M, g->K, g->N, g->r};
  G.mb = (m + g->M - 1) / g->M; G.kb = (k + g->K - 1) / g->K; G.nb = (n + g->N - 1) / g->N;
  G.mbp = amx_pad(G.mb, AMX_COLPAD); G.kbp = amx_pad(G.kb, 64); G.nbp = amx_pad(G.nb, 32);
  int *sig = malloc(m * sizeof(int)), *tau = malloc(n * sizeof(int));
  int r = g->r;
  size_t szL = G.mbp * G.kbp, szR = G.nbp * G.kbp;
  // product planes of one modulus are interleaved by column: element (i, j) of product pr is at
  // Ybase_l + (j * r + pr) * mbp + i, so the reconstruction reads r adjacent columns per modulus.
  size_t ldy = (size_t)r * G.mbp, szYl = amx_pad(ldy * G.nb, 4096);
  int8_t *ws = oz_workspace((size_t)s * r * (szL + szR) + (size_t)s * szYl);
  int8_t **Lp = malloc((size_t)s * r * sizeof(int8_t *)), **Rp = malloc((size_t)s * r * sizeof(int8_t *));
  uint8_t **Yp = malloc((size_t)s * r * sizeof(uint8_t *));
  for (int q = 0; q < s * r; q++) {
    Lp[q] = ws + (size_t)q * (szL + szR);
    Rp[q] = Lp[q] + szL;
  }
  uint8_t *ybase = (uint8_t *)(ws + (size_t)s * r * (szL + szR));
  for (int l = 0; l < s; l++)
    for (int pr = 0; pr < r; pr++) Yp[l * r + pr] = ybase + (size_t)l * szYl + (size_t)pr * G.mbp;
  double t1 = now_sec();
  double *ek = oz_inner_scaling(m, k, n, A, lda, B, ldb, amx_pad(k, 64));
  pack_left(s, L, &G, g, A, lda, ek, sig, Lp);
  pack_right(s, L, &G, g, B, ldb, ek, tau, Rp);
  free(ek);
  double t2 = now_sec();
  for (int l = 0; l < s; l++)
    for (int pr = 0; pr < r; pr++) {
      epi2_ctx e = {Yp[l * r + pr], ldy, G.mbp, G.nb, (double)oz_mod[l], 1.0 / oz_mod[l]};
      amx_gemm_s8s8(G.nbp, G.mbp, G.kbp, Rp[l * r + pr], Lp[l * r + pr], epi2, &e);
    }
  double t3 = now_sec();
  reconstruct2(s, &G, g, Yp, ldy, sig, tau, C, ldc);
  double t4 = now_sec();
  free(sig); free(tau); free(Lp); free(Rp); free(Yp);
  if (oz_nonfinite_seen) {  // NaN/Inf: the plain emulation falls back to the BLAS
    oz_nonfinite_seen = 0;
    oz_dgemm(s, m, k, n, A, lda, B, ldb, C, ldc, NULL);
  }
  if (tm) { tm->scale = 0; tm->convert = t2 - t1; tm->gemm = t3 - t2; tm->crt = t4 - t3; (void)t0; }
}
