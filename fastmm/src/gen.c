// Generic fast matrix multiplication engine driven by straight-line programs (SLPs)
// produced by tools/slp.py from a scheme (U,V,W).  One recursion level:
//   phase A: one fused pass over the M*K blocks of A writes all non-trivial left factors,
//   phase B: same for B,
//   products: r recursive calls (DFS: one after another with multithreaded BLAS at the
//             leaves; BFS: as OpenMP tasks with single-threaded BLAS leaves),
//   phase C: one fused pass reads the r products and writes the M*N blocks of C.
// Fused passes read every input block once and write every output once; intermediate
// SLP variables live in a small per-thread scratch buffer.
// Remainders (dimensions not divisible by the base case) are handled by dynamic peeling.
#include <stdio.h>
#include <string.h>
#include <omp.h>
#include "gen.h"
#include "blas.h"

#define RB 256  // rows per chunk in fused passes

#include "slp.h"

static int read_slp(FILE *f, gslp *s) {
  if (fscanf(f, "%d %d %d", &s->nin, &s->ninstr, &s->nout) != 3) return -1;
  s->ins = calloc(s->ninstr, sizeof(ginstr));
  for (int i = 0; i < s->ninstr; i++) {
    if (fscanf(f, "%d", &s->ins[i].nt) != 1) return -1;
    s->ins[i].src = malloc(s->ins[i].nt * sizeof(int));
    s->ins[i].coef = malloc(s->ins[i].nt * sizeof(double));
    for (int t = 0; t < s->ins[i].nt; t++)
      if (fscanf(f, "%d %lf", &s->ins[i].src[t], &s->ins[i].coef[t]) != 2) return -1;
  }
  s->ovar = malloc(s->nout * sizeof(int));
  s->oscale = malloc(s->nout * sizeof(double));
  for (int o = 0; o < s->nout; o++)
    if (fscanf(f, "%d %lf", &s->ovar[o], &s->oscale[o]) != 2) return -1;
  return 0;
}

gen_scheme *gen_load(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) { perror(path); return NULL; }
  gen_scheme *g = calloc(1, sizeof *g);
  char tag[16];
  if (fscanf(f, "%15s %d %d %d %d", tag, &g->M, &g->K, &g->N, &g->r) != 5) goto bad;
  g->pscale = malloc(g->r * sizeof(double));
  if (fscanf(f, "%15s", tag) != 1) goto bad;
  for (int l = 0; l < g->r; l++) if (fscanf(f, "%lf", &g->pscale[l]) != 1) goto bad;
  for (int sec = 0; sec < 3; sec++) {
    char w1[16], w2[16];
    if (fscanf(f, "%15s %15s", w1, w2) != 2) goto bad;
    if (read_slp(f, &g->s[sec])) goto bad;
  }
  fclose(f);
  for (int l = 0; l < g->r; l++) {
    if (g->s[0].ovar[l] >= g->s[0].nin || g->s[0].oscale[l] != 1.0) g->nbufA++;
    if (g->s[1].ovar[l] >= g->s[1].nin || g->s[1].oscale[l] != 1.0) g->nbufB++;
  }
  return g;
bad:
  fprintf(stderr, "bad slp file %s\n", path);
  fclose(f);
  return NULL;
}

void gen_dims(const gen_scheme *g, int *M, int *K, int *N, int *r) { *M = g->M; *K = g->K; *N = g->N; *r = g->r; }

// ---- fused SLP evaluation ------------------------------------------------------------
typedef struct { const double *p; size_t ld; } cview;
typedef struct { double *p; size_t ld; } view;

// Evaluate slp on blocks of size rows x cols.  in[v] for inputs; out[o].p == NULL means skip.
// extra_scale (may be NULL): per-input scale applied on read (used for product scales in C).
static void slp_eval(const gslp *s, size_t rows, size_t cols, const cview *in, const double *in_scale,
                     const view *out, int parallel) {
  int nv = s->nin + s->ninstr;
  #pragma omp parallel if (parallel)
  {
    double *scr = amalloc((size_t)s->ninstr * RB * sizeof(double) + 64);
    const double **ptr = malloc(nv * sizeof(double *));
    #pragma omp for schedule(static) collapse(2)
    for (size_t j = 0; j < cols; j++)
      for (size_t i0 = 0; i0 < rows; i0 += RB) {
        size_t len = rows - i0 < RB ? rows - i0 : RB;
        for (int v = 0; v < s->nin; v++) ptr[v] = in[v].p ? in[v].p + i0 + j * in[v].ld : NULL;
        for (int t = 0; t < s->ninstr; t++) {
          const ginstr *I = &s->ins[t];
          double *d = scr + (size_t)t * RB;
          const double *s0 = ptr[I->src[0]];
          double c0 = I->coef[0] * (I->src[0] < s->nin && in_scale ? in_scale[I->src[0]] : 1.0);
          #pragma omp simd
          for (size_t i = 0; i < len; i++) d[i] = c0 * s0[i];
          for (int q = 1; q < I->nt; q++) {
            const double *sq = ptr[I->src[q]];
            double cq = I->coef[q] * (I->src[q] < s->nin && in_scale ? in_scale[I->src[q]] : 1.0);
            #pragma omp simd
            for (size_t i = 0; i < len; i++) d[i] += cq * sq[i];
          }
          ptr[s->nin + t] = d;
        }
        for (int o = 0; o < s->nout; o++) {
          if (!out[o].p) continue;
          int v = s->ovar[o];
          double c = s->oscale[o] * (v < s->nin && in_scale ? in_scale[v] : 1.0);
          double *d = out[o].p + i0 + j * out[o].ld;
          const double *src = ptr[v];
          if (c == 1.0) memcpy(d, src, len * sizeof(double));
          else {
            #pragma omp simd
            for (size_t i = 0; i < len; i++) d[i] = c * src[i];
          }
        }
      }
    free(scr);
    free((void *)ptr);
  }
}

// ---- recursion -----------------------------------------------------------------------
// workspace needed by one call at this level (excluding children)
static size_t level_ws(const gen_scheme *g, size_t m, size_t k, size_t n) {
  size_t mb = m / g->M, kb = k / g->K, nb = n / g->N;
  return g->nbufA * mb * kb + g->nbufB * kb * nb + (size_t)g->r * mb * nb;
}

// plan: the top `dfs` levels run products one after another (multithreaded BLAS leaves and
// parallel passes), the next `bfs` levels spawn their products as OpenMP tasks, and any deeper
// levels run serially inside the task that owns them.
size_t gen_workspace(const gen_scheme *g, size_t m, size_t k, size_t n, int depth, int dfs, int bfs) {
  if (depth <= 0 || m < (size_t)g->M || k < (size_t)g->K || n < (size_t)g->N) return 0;
  size_t mb = m / g->M, kb = k / g->K, nb = n / g->N;
  int cd = dfs > 0 ? dfs - 1 : 0, cb = dfs > 0 ? bfs : (bfs > 0 ? bfs - 1 : 0);
  size_t child = gen_workspace(g, mb, kb, nb, depth - 1, cd, cb);
  int tasks = dfs <= 0 && bfs > 0;
  return level_ws(g, m, k, n) + (tasks ? (size_t)g->r : 1) * child;
}

static void gen_rec(const gen_scheme *g, int depth, int dfs, int bfs, int in_task, size_t m, size_t k, size_t n,
                    const double *A, size_t lda, const double *B, size_t ldb, double *C, size_t ldc, double *W) {
  if (depth <= 0 || m < (size_t)g->M || k < (size_t)g->K || n < (size_t)g->N) {
    if (in_task) blas_set_threads_local(1);
    dgemm_nn(m, n, k, 1.0, A, lda, B, ldb, 0.0, C, ldc);
    if (in_task) blas_set_threads_local(0);
    return;
  }
  const int M = g->M, K = g->K, N = g->N, r = g->r;
  size_t mb = m / M, kb = k / K, nb = n / N;
  int par = !in_task;
  if (M * K > 64 || K * N > 64 || M * N > 64) { fprintf(stderr, "scheme too large\n"); exit(1); }
  cview ina[64], inb[64];
  view outc[64];
  view *outa = malloc(r * sizeof(view)), *outb = malloc(r * sizeof(view));
  cview *inc = malloc(r * sizeof(cview)), *fa = malloc(r * sizeof(cview)), *fb = malloc(r * sizeof(cview));
  for (int i = 0; i < M; i++)
    for (int q = 0; q < K; q++) ina[i * K + q] = (cview){A + i * mb + q * kb * lda, lda};
  for (int q = 0; q < K; q++)
    for (int j = 0; j < N; j++) inb[q * N + j] = (cview){B + q * kb + j * nb * ldb, ldb};
  double *w = W;
  // left / right factors: either a view of an input block or a new buffer
  for (int l = 0; l < r; l++) {
    int v = g->s[0].ovar[l];
    if (v < g->s[0].nin && g->s[0].oscale[l] == 1.0) { fa[l] = ina[v]; outa[l].p = NULL; }
    else { outa[l] = (view){w, mb}; fa[l] = (cview){w, mb}; w += mb * kb; }
    v = g->s[1].ovar[l];
    if (v < g->s[1].nin && g->s[1].oscale[l] == 1.0) { fb[l] = inb[v]; outb[l].p = NULL; }
    else { outb[l] = (view){w, kb}; fb[l] = (cview){w, kb}; w += kb * nb; }
  }
  double *prod = w;
  w += (size_t)r * mb * nb;
  slp_eval(&g->s[0], mb, kb, ina, NULL, outa, par);
  slp_eval(&g->s[1], kb, nb, inb, NULL, outb, par);
  int cd = dfs > 0 ? dfs - 1 : 0, cb = dfs > 0 ? bfs : (bfs > 0 ? bfs - 1 : 0);
  size_t cws = gen_workspace(g, mb, kb, nb, depth - 1, cd, cb);
  if (dfs <= 0 && bfs > 0) {
    // products as tasks
    if (in_task) {
      for (int l = 0; l < r; l++) {
        #pragma omp task firstprivate(l)
        gen_rec(g, depth - 1, cd, cb, 1, mb, kb, nb, fa[l].p, fa[l].ld, fb[l].p, fb[l].ld, prod + l * mb * nb, mb,
                w + l * cws);
      }
      #pragma omp taskwait
    } else {
      #pragma omp parallel
      #pragma omp single
      {
        for (int l = 0; l < r; l++) {
          #pragma omp task firstprivate(l)
          gen_rec(g, depth - 1, cd, cb, 1, mb, kb, nb, fa[l].p, fa[l].ld, fb[l].p, fb[l].ld, prod + l * mb * nb, mb,
                  w + l * cws);
        }
        #pragma omp taskwait
      }
    }
  } else {
    for (int l = 0; l < r; l++)
      gen_rec(g, depth - 1, cd, cb, in_task, mb, kb, nb, fa[l].p, fa[l].ld, fb[l].p, fb[l].ld, prod + l * mb * nb, mb,
              w);
  }
  for (int l = 0; l < r; l++) inc[l] = (cview){prod + l * mb * nb, mb};
  for (int i = 0; i < M; i++)
    for (int j = 0; j < N; j++) outc[i * N + j] = (view){C + i * mb + j * nb * ldc, ldc};
  slp_eval(&g->s[2], mb, nb, inc, g->pscale, outc, par);

  // dynamic peeling of remainders
  size_t mc = M * mb, kc = K * kb, nc = N * nb;
  if (in_task) blas_set_threads_local(1);
  if (k > kc) dgemm_nn(mc, nc, k - kc, 1.0, A + kc * lda, lda, B + kc, ldb, 1.0, C, ldc);
  if (m > mc) dgemm_nn(m - mc, nc, k, 1.0, A + mc, lda, B, ldb, 0.0, C + mc, ldc);
  if (n > nc) dgemm_nn(m, n - nc, k, 1.0, A, lda, B + nc * ldb, ldb, 0.0, C + nc * ldc, ldc);
  if (in_task) blas_set_threads_local(0);
  free(outa); free(outb); free(inc); free(fa); free(fb);
}

void gen_dgemm(const gen_scheme *g, int depth, int dfs, int bfs, size_t m, size_t k, size_t n, const double *A,
               size_t lda, const double *B, size_t ldb, double *C, size_t ldc, double *work) {
  size_t ws = gen_workspace(g, m, k, n, depth, dfs, bfs);
  double *W = work;
  if (!W && ws) W = amalloc(ws * sizeof(double));
  gen_rec(g, depth, dfs, bfs, 0, m, k, n, A, lda, B, ldb, C, ldc, W);
  if (W != work) free(W);
}

// Calibration helper: time the three fused passes of one level on blocks of an m x k x n
// problem (data uniform random), returns seconds for phase A, B, C in t[0..2].
void gen_bench_passes(const gen_scheme *g, size_t m, size_t k, size_t n, int reps, double *t) {
  size_t mb = m / g->M, kb = k / g->K, nb = n / g->N;
  double *A = amalloc(m * k * 8), *B = amalloc(k * n * 8), *C = amalloc(m * n * 8);
  fill_uniform(A, m, k, m, 3); fill_uniform(B, k, n, k, 4);
  double *bufA = amalloc((size_t)g->nbufA * mb * kb * 8 + 64), *bufB = amalloc((size_t)g->nbufB * kb * nb * 8 + 64);
  double *prod = amalloc((size_t)g->r * mb * nb * 8);
  fill_uniform(prod, mb * nb, g->r, mb * nb, 5);
  int M = g->M, K = g->K, N = g->N, r = g->r;
  cview ina[64], inb[64], *inc = malloc(r * sizeof(cview));
  view *outa = malloc(r * sizeof(view)), *outb = malloc(r * sizeof(view)), outc[64];
  for (int i = 0; i < M; i++) for (int q = 0; q < K; q++) ina[i * K + q] = (cview){A + i * mb + q * kb * m, m};
  for (int q = 0; q < K; q++) for (int j = 0; j < N; j++) inb[q * N + j] = (cview){B + q * kb + j * nb * k, k};
  double *wa = bufA, *wb = bufB;
  for (int l = 0; l < r; l++) {
    int v = g->s[0].ovar[l];
    if (v < g->s[0].nin && g->s[0].oscale[l] == 1.0) outa[l].p = NULL; else { outa[l] = (view){wa, mb}; wa += mb * kb; }
    v = g->s[1].ovar[l];
    if (v < g->s[1].nin && g->s[1].oscale[l] == 1.0) outb[l].p = NULL; else { outb[l] = (view){wb, kb}; wb += kb * nb; }
    inc[l] = (cview){prod + l * mb * nb, mb};
  }
  for (int i = 0; i < M; i++) for (int j = 0; j < N; j++) outc[i * N + j] = (view){C + i * mb + j * nb * m, m};
  double tt[3][32];
  if (reps > 32) reps = 32;
  for (int it = -1; it < reps; it++) {
    double t0 = now_sec();
    slp_eval(&g->s[0], mb, kb, ina, NULL, outa, 1);
    double t1 = now_sec();
    slp_eval(&g->s[1], kb, nb, inb, NULL, outb, 1);
    double t2 = now_sec();
    slp_eval(&g->s[2], mb, nb, inc, g->pscale, outc, 1);
    double t3 = now_sec();
    if (it >= 0) { tt[0][it] = t1 - t0; tt[1][it] = t2 - t1; tt[2][it] = t3 - t2; }
  }
  for (int q = 0; q < 3; q++) t[q] = median(tt[q], reps);
  free(A); free(B); free(C); free(bufA); free(bufB); free(prod); free(inc); free(outa); free(outb);
}
