// Unified accuracy / timing driver for all methods on the same BLAS.
//   fmmtest acc  <type> <r> <m> <k> <n> <nsample> method...
//   fmmtest time <reps> <m> <k> <n> method...
// methods: dgemm | swD (Strassen-Winograd, D levels) | ozS (AMX int8 emulation, S moduli)
//          | genNAME:D (generic bilinear scheme from schemes/NAME.txt, D levels; if built with it)
// Timing runs the methods round-robin (interleaved) and reports median and min per method.
#include <stdio.h>
#include <string.h>
#include "fmm.h"
#include "blas.h"
#include "ozaki.h"
#include "testmat.h"
#include "gen.h"

typedef struct { char kind[16]; int param, dfs, bfs; gen_scheme *g; int swtop, scaled; } method;

static method parse(const char *s) {
  method m = {"", 0, 0, 0, NULL, 0, 0};
  if (!strncmp(s, "sc:", 3)) {  // outside-inside power-of-two scaling around any method
    method in = parse(s + 3);
    in.scaled = 1;
    return in;
  }
  if (!strncmp(s, "sw", 2) && strchr(s, '+')) {  // swD+g:...  memory-lean Strassen top levels, generic leaves
    method in = parse(strchr(s, '+') + 1);
    in.swtop = atoi(s + 2);
    return in;
  }
  if (!strncmp(s, "g:", 2)) {  // g:<file.slp>:<depth>:<dfs>:<bfs>
    char path[512];
    strcpy(m.kind, "gen");
    const char *c1 = strchr(s + 2, ':');
    size_t L = c1 - (s + 2);
    memcpy(path, s + 2, L); path[L] = 0;
    if (sscanf(c1 + 1, "%d:%d:%d", &m.param, &m.dfs, &m.bfs) != 3) { fprintf(stderr, "bad %s\n", s); exit(1); }
    m.g = gen_load(path);
    if (!m.g) exit(1);
    return m;
  }
  if (!strncmp(s, "ozf", 3)) {  // ozf<s>:<file.slp>
    strcpy(m.kind, "ozf");
    m.param = atoi(s + 3);
    const char *c1 = strchr(s, ':');
    m.g = gen_load(c1 + 1);
    if (!m.g) exit(1);
    return m;
  }
  if (!strncmp(s, "ozw", 3)) { strcpy(m.kind, "ozw"); m.param = atoi(s + 3); return m; }
  if (!strcmp(s, "dgemm")) { strcpy(m.kind, "dgemm"); return m; }
  if (!strncmp(s, "sw", 2)) { strcpy(m.kind, "sw"); m.param = atoi(s + 2); return m; }
  if (!strncmp(s, "oz", 2)) { strcpy(m.kind, "oz"); m.param = atoi(s + 2); return m; }
  fprintf(stderr, "unknown method %s\n", s);
  exit(1);
}

static double *g_work = NULL;
static size_t g_work_sz = 0;
static oz_times g_ozt;

static method g_leafm;
static double *g_lwork = NULL;
static size_t g_lwork_sz = 0;
static void gen_leaf(size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B, size_t ldb, double *C,
                     size_t ldc, void *ctx) {
  (void)ctx;
  if (!strcmp(g_leafm.kind, "oz")) { oz_dgemm(g_leafm.param, m, k, n, A, lda, B, ldb, C, ldc, NULL); return; }
  size_t ws = gen_workspace(g_leafm.g, m, k, n, g_leafm.param, g_leafm.dfs, g_leafm.bfs);
  if (ws > g_lwork_sz) { free(g_lwork); g_lwork = amalloc(ws * 8); g_lwork_sz = ws; }
  gen_dgemm(g_leafm.g, g_leafm.param, g_leafm.dfs, g_leafm.bfs, m, k, n, A, lda, B, ldb, C, ldc, g_lwork);
}

static void run(method me, size_t m, size_t k, size_t n, const double *A, const double *B, double *C);

// Outside-inside scaling (powers of two, exact): A' = Dr A Dk, B' = Dk^-1 B Dc, C = Dr^-1 (A'B') Dc^-1.
// Dk balances column norms of A against row norms of B; Dr, Dc then normalise rows of A' and
// columns of B'.  (Cf. Ballard, Benson, Druinsky, Lipshitz, Schwartz, SIMAX 2016.)
static void run_scaled(method me, size_t m, size_t k, size_t n, const double *A, const double *B, double *C) {
  double *As = amalloc(m * k * 8), *Bs = amalloc(k * n * 8);
  int *ek = calloc(k, sizeof(int)), *fr = calloc(m, sizeof(int)), *gc = calloc(n, sizeof(int));
  double *ca = calloc(k, 8), *rb = calloc(k, 8), *ra = calloc(m, 8);
  #pragma omp parallel for
  for (size_t p = 0; p < k; p++) { double s = 0; for (size_t i = 0; i < m; i++) s += A[i + p * m] * A[i + p * m]; ca[p] = s; }
  for (size_t j = 0; j < n; j++) for (size_t p = 0; p < k; p++) rb[p] += B[p + j * k] * B[p + j * k];
  for (size_t p = 0; p < k; p++) ek[p] = (ca[p] > 0 && rb[p] > 0) ? (int)lround(0.25 * log2(rb[p] / ca[p])) : 0;
  #pragma omp parallel for
  for (size_t p = 0; p < k; p++) for (size_t i = 0; i < m; i++) As[i + p * m] = ldexp(A[i + p * m], ek[p]);
  for (size_t p = 0; p < k; p++) for (size_t i = 0; i < m; i++) ra[i] += As[i + p * m] * As[i + p * m];
  for (size_t i = 0; i < m; i++) fr[i] = ra[i] > 0 ? -(int)lround(0.5 * log2(ra[i])) : 0;
  #pragma omp parallel for
  for (size_t p = 0; p < k; p++) for (size_t i = 0; i < m; i++) As[i + p * m] = ldexp(As[i + p * m], fr[i]);
  #pragma omp parallel for
  for (size_t j = 0; j < n; j++) {
    double s = 0;
    for (size_t p = 0; p < k; p++) { double v = ldexp(B[p + j * k], -ek[p]); Bs[p + j * k] = v; s += v * v; }
    gc[j] = s > 0 ? -(int)lround(0.5 * log2(s)) : 0;
    for (size_t p = 0; p < k; p++) Bs[p + j * k] = ldexp(Bs[p + j * k], gc[j]);
  }
  method in = me;
  in.scaled = 0;
  run(in, m, k, n, As, Bs, C);
  #pragma omp parallel for
  for (size_t j = 0; j < n; j++) for (size_t i = 0; i < m; i++) C[i + j * m] = ldexp(C[i + j * m], -fr[i] - gc[j]);
  free(As); free(Bs); free(ek); free(fr); free(gc); free(ca); free(rb); free(ra);
}

static void run(method me, size_t m, size_t k, size_t n, const double *A, const double *B, double *C) {
  if (me.scaled) { run_scaled(me, m, k, n, A, B, C); return; }
  if (me.swtop > 0) {
    g_leafm = me;
    sw_set_leaf(gen_leaf, NULL);
    size_t ws = sw_workspace(m, k, n, me.swtop);
    if (ws > g_work_sz) { free(g_work); g_work = amalloc(ws * 8); g_work_sz = ws; }
    sw_dgemm(me.swtop, m, k, n, A, m, B, k, C, m, g_work);
    sw_set_leaf(NULL, NULL);
    return;
  }
  if (!strcmp(me.kind, "dgemm")) dgemm_nn(m, n, k, 1.0, A, m, B, k, 0.0, C, m);
  else if (!strcmp(me.kind, "sw")) {
    size_t ws = sw_workspace(m, k, n, me.param);
    if (ws > g_work_sz) { free(g_work); g_work = amalloc(ws * 8); g_work_sz = ws; }
    sw_dgemm(me.param, m, k, n, A, m, B, k, C, m, g_work);
  } else if (!strcmp(me.kind, "oz")) oz_dgemm(me.param, m, k, n, A, m, B, k, C, m, &g_ozt);
  else if (!strcmp(me.kind, "ozf")) oz_dgemm_fmm(me.param, me.g, m, k, n, A, m, B, k, C, m, &g_ozt);
  else if (!strcmp(me.kind, "ozw")) oz_dgemm_w(me.param, m, k, n, A, m, B, k, C, m, &g_ozt);
  else if (!strcmp(me.kind, "gen")) {
    size_t ws = gen_workspace(me.g, m, k, n, me.param, me.dfs, me.bfs);
    if (ws > g_work_sz) { free(g_work); g_work = amalloc(ws * 8); g_work_sz = ws; }
    gen_dgemm(me.g, me.param, me.dfs, me.bfs, m, k, n, A, m, B, k, C, m, g_work);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) { fprintf(stderr, "see source for usage\n"); return 1; }
  if (!strcmp(argv[1], "acc")) {
    int type = atoi(argv[2]), r = atoi(argv[3]);
    size_t m = atol(argv[4]), k = atol(argv[5]), n = atol(argv[6]), ns = atol(argv[7]);
    double *A = amalloc(m * k * 8), *B = amalloc(k * n * 8), *C = amalloc(m * n * 8);
    testmat_fill(type, r, m, k, n, A, B, 42);
    for (int a = 8; a < argc; a++) {
      method me = parse(argv[a]);
      memset(C, 0, m * n * 8);
      run(me, m, k, n, A, B, C);
      errstats e = err_sampled(m, k, n, A, B, C, ns, 7);
      printf("acc type=%s r=%d m=%zu k=%zu n=%zu method=%s max_cw=%.3e med_cw=%.3e max_rel=%.3e med_rel=%.3e nrm=%.3e\n",
             testmat_name[type], r, m, k, n, argv[a], e.max_cw, e.med_cw, e.max_rel, e.med_rel, e.nrm);
      fflush(stdout);
    }
    return 0;
  }
  if (!strcmp(argv[1], "bitcmp")) {  // bitcmp <type> <r> <m> <k> <n> methodA methodB : bitwise equality
    int type = atoi(argv[2]), r = atoi(argv[3]);
    size_t m = atol(argv[4]), k = atol(argv[5]), n = atol(argv[6]);
    double *A = amalloc(m * k * 8), *B = amalloc(k * n * 8), *C1 = amalloc(m * n * 8), *C2 = amalloc(m * n * 8);
    testmat_fill(type, r, m, k, n, A, B, 42);
    method a = parse(argv[7]), b = parse(argv[8]);
    run(a, m, k, n, A, B, C1);
    run(b, m, k, n, A, B, C2);
    size_t diff = 0;
    for (size_t i = 0; i < m * n; i++) if (memcmp(&C1[i], &C2[i], 8)) diff++;
    printf("bitcmp type=%s r=%d m=%zu k=%zu n=%zu %s vs %s: %zu of %zu entries differ\n", testmat_name[type], r, m, k,
           n, argv[7], argv[8], diff, m * n);
    return diff != 0;
  }
  if (!strcmp(argv[1], "nancheck")) {  // NaN/Inf must propagate like in the BLAS
    size_t n = 300;
    double *A = amalloc(n * n * 8), *B = amalloc(n * n * 8), *C1 = amalloc(n * n * 8), *C2 = amalloc(n * n * 8);
    const char *meths[] = {"oz14", "oz16", "ozw14", "sw1"};
    int bad = 0;
    for (int t = 0; t < 2; t++) {
      testmat_fill(0, 0, n, n, n, A, B, 42);
      A[17 + 33 * n] = t ? INFINITY : NAN;  // row 17
      B[5 + 200 * n] = t ? -INFINITY : NAN; // column 200
      run(parse("dgemm"), n, n, n, A, B, C1);
      for (int q = 0; q < 4; q++) {
        run(parse(meths[q]), n, n, n, A, B, C2);
        size_t mism = 0;
        for (size_t i = 0; i < n * n; i++) mism += (isnan(C1[i]) != isnan(C2[i])) || (isinf(C1[i]) != isinf(C2[i]));
        printf("nancheck %s %s: %zu entries with different NaN/Inf status than dgemm\n", t ? "inf" : "nan", meths[q], mism);
        bad |= mism && strcmp(meths[q], "sw1");  // Strassen may legitimately turn Inf into NaN (Inf - Inf)
      }
    }
    return bad;
  }
  if (!strcmp(argv[1], "cold")) {  // cold <n> method: time of the first call in a fresh process vs the next calls
    size_t n = atol(argv[2]);
    method me = parse(argv[3]);
    double *A = amalloc(n * n * 8), *B = amalloc(n * n * 8), *C = amalloc(n * n * 8);
    testmat_fill(0, 0, n, n, n, A, B, 42);
    memset(C, 0, n * n * 8);
    double t[4];
    for (int i = 0; i < 4; i++) { double t0 = now_sec(); run(me, n, n, n, A, B, C); t[i] = now_sec() - t0; }
    printf("cold n=%zu method=%s first=%.4f then=%.4f %.4f %.4f\n", n, argv[3], t[0], t[1], t[2], t[3]);
    return 0;
  }
  if (!strcmp(argv[1], "passes")) {  // passes <slp> <reps> n...
    gen_scheme *g = gen_load(argv[2]);
    int reps = atoi(argv[3]);
    int M, K, N, r;
    gen_dims(g, &M, &K, &N, &r);
    for (int a = 4; a < argc; a++) {
      size_t n = atol(argv[a]);
      double t[3];
      gen_bench_passes(g, n, n, n, reps, t);
      printf("passes slp=%s n=%zu tA=%.5f tB=%.5f tC=%.5f\n", argv[2], n, t[0], t[1], t[2]);
      fflush(stdout);
    }
    return 0;
  }
  if (!strcmp(argv[1], "time")) {
    int reps = atoi(argv[2]);
    size_t m = atol(argv[3]), k = atol(argv[4]), n = atol(argv[5]);
    int nm = argc - 6;
    method me[32];
    for (int a = 0; a < nm; a++) me[a] = parse(argv[6 + a]);
    double *A = amalloc(m * k * 8), *B = amalloc(k * n * 8), *C = amalloc(m * n * 8);
    testmat_fill(0, 0, m, k, n, A, B, 42);
    double flop = 2.0 * m * n * k;
    for (int a = 0; a < nm; a++) run(me[a], m, k, n, A, B, C);  // warm-up
    double (*t)[64] = calloc(nm, sizeof *t);
    oz_times (*ozt)[64] = calloc(nm, sizeof *ozt);
    if (reps > 64) reps = 64;
    for (int i = 0; i < reps; i++)
      for (int a = 0; a < nm; a++) {
        double t0 = now_sec();
        run(me[a], m, k, n, A, B, C);
        t[a][i] = now_sec() - t0;
        ozt[a][i] = g_ozt;
      }
    double tref = 0;
    for (int a = 0; a < nm; a++) {
      double tmin = t[a][0], tc[64];
      for (int i = 1; i < reps; i++) if (t[a][i] < tmin) tmin = t[a][i];
      memcpy(tc, t[a], sizeof tc);
      double tmed = median(tc, reps);  // median() sorts its argument: use a copy
      if (a == 0) tref = tmed;
      // paired per-round ratio t_first / t_this (robust to slow drift of the machine)
      double rat[64];
      for (int i = 0; i < reps; i++) rat[i] = t[0][i] / t[a][i];
      double rmed = median(rat, reps), rlo = rat[reps / 4], rhi = rat[(3 * reps) / 4];  // rat sorted now
      printf("time m=%zu k=%zu n=%zu method=%s reps=%d median=%.5f min=%.5f eff_gflops=%.1f speedup_vs_first=%.3f paired=%.3f [IQR %.3f-%.3f]",
             m, k, n, argv[6 + a], reps, tmed, tmin, flop / tmed * 1e-9, tref / tmed, rmed, rlo, rhi);
      if (!strcmp(me[a].kind, "oz") || !strcmp(me[a].kind, "ozf") || !strcmp(me[a].kind, "ozw")) {
        oz_times o = ozt[a][reps - 1];
        printf(" [scale %.4f conv %.4f gemm %.4f crt %.4f]", o.scale, o.convert, o.gemm, o.crt);
      }
      printf("\n");
      fflush(stdout);
    }
    return 0;
  }
  return 1;
}
