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

typedef struct { char kind[16]; int param, dfs, bfs; gen_scheme *g; } method;

static method parse(const char *s) {
  method m = {"", 0, 0, 0, NULL};
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
  if (!strcmp(s, "dgemm")) { strcpy(m.kind, "dgemm"); return m; }
  if (!strncmp(s, "sw", 2)) { strcpy(m.kind, "sw"); m.param = atoi(s + 2); return m; }
  if (!strncmp(s, "oz", 2)) { strcpy(m.kind, "oz"); m.param = atoi(s + 2); return m; }
  fprintf(stderr, "unknown method %s\n", s);
  exit(1);
}

static double *g_work = NULL;
static size_t g_work_sz = 0;
static oz_times g_ozt;

static void run(method me, size_t m, size_t k, size_t n, const double *A, const double *B, double *C) {
  if (!strcmp(me.kind, "dgemm")) dgemm_nn(m, n, k, 1.0, A, m, B, k, 0.0, C, m);
  else if (!strcmp(me.kind, "sw")) {
    size_t ws = sw_workspace(m, k, n, me.param);
    if (ws > g_work_sz) { free(g_work); g_work = amalloc(ws * 8); g_work_sz = ws; }
    sw_dgemm(me.param, m, k, n, A, m, B, k, C, m, g_work);
  } else if (!strcmp(me.kind, "oz")) oz_dgemm(me.param, m, k, n, A, m, B, k, C, m, &g_ozt);
  else if (!strcmp(me.kind, "ozf")) oz_dgemm_fmm(me.param, me.g, m, k, n, A, m, B, k, C, m, &g_ozt);
  else if (!strcmp(me.kind, "gen")) {
    size_t ws = gen_workspace(me.g, m, k, n, me.param, me.dfs, me.bfs);
    if (ws > g_work_sz) { free(g_work); g_work = amalloc(ws * 8); g_work_sz = ws; }
    gen_dgemm(me.g, me.param, me.dfs, me.bfs, m, k, n, A, m, B, k, C, m, g_work);
  }
}

int main(int argc, char **argv) {
  if (argc < 3) { fprintf(stderr, "see source for usage\n"); return 1; }
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
      if (!strcmp(me[a].kind, "oz") || !strcmp(me[a].kind, "ozf")) {
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
