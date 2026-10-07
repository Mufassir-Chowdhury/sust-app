#pragma once
#include <stddef.h>
typedef struct { double scale, convert, gemm, crt; } oz_times;
int oz_max_moduli(void);
// C = A*B (column-major doubles) emulated with s int8 AMX GEMMs (2 <= s <= oz_max_moduli()).
void oz_dgemm(int s, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B, size_t ldb,
              double *C, size_t ldc, oz_times *tm);
void oz_release(void);  // free the persistent workspace
#include "gen.h"
// Same emulation, but each modular product is computed with one level of the bilinear scheme g
// (integer coefficients) exactly in Z/p: the result is bit-identical to oz_dgemm(s, ...).
void oz_dgemm_fmm(int s, const gen_scheme *g, size_t m, size_t k, size_t n, const double *A, size_t lda,
                  const double *B, size_t ldb, double *C, size_t ldc, oz_times *tm);
// Specialised: one exact Strassen-Winograd level per modulus (unsigned residues, AMX u8 x u8),
// memory-blocked; bit-identical to oz_dgemm(s, ...).
void oz_dgemm_w(int s, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B, size_t ldb,
                double *C, size_t ldc, oz_times *tm);
