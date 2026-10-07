#pragma once
#include "common.h"
typedef struct gen_scheme gen_scheme;
gen_scheme *gen_load(const char *slp_path);
void gen_dims(const gen_scheme *g, int *M, int *K, int *N, int *r);
// depth levels of the scheme; top `dfs` levels depth-first (threaded BLAS), next `bfs` levels as tasks.
size_t gen_workspace(const gen_scheme *g, size_t m, size_t k, size_t n, int depth, int dfs, int bfs);
void gen_dgemm(const gen_scheme *g, int depth, int dfs, int bfs, size_t m, size_t k, size_t n, const double *A,
               size_t lda, const double *B, size_t ldb, double *C, size_t ldc, double *work);
void gen_bench_passes(const gen_scheme *g, size_t m, size_t k, size_t n, int reps, double *t);
