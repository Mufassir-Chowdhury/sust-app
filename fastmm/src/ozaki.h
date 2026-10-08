#pragma once
#include <stddef.h>
typedef struct { double scale, convert, gemm, crt; } oz_times;
int oz_max_moduli(void);
// C = A*B (column-major doubles) emulated with s int8 AMX GEMMs (2 <= s <= oz_max_moduli()).
void oz_dgemm(int s, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B, size_t ldb,
              double *C, size_t ldc, oz_times *tm);
void oz_release(void);  // free the persistent workspace
// Certified mode (theta > 0; 0 = off): every entry of C is either certified to satisfy
// |c^_ij - c_ij| <= (theta + 1) u sum_k |a_ik||b_kj| to first order (bound of the emulation error
// against a rigorous lower bound of |A||B| from one extra int8 GEMM), recomputed with a compensated dot
// product (isolated entries), or computed by the BLAS DGEMM (256 x 256 tiles holding more than 256
// uncertified entries; the whole block when more than 1/8 of its tiles do).
void oz_set_certify(double theta);
typedef struct { size_t entries, flagged, recomputed, dgemm_tiles, blocks, fallback_blocks; } oz_cert_stats;
oz_cert_stats oz_get_cert_stats(void);  // cumulative since program start
#include "gen.h"
// Same emulation, but each modular product is computed with one level of the bilinear scheme g
// (integer coefficients) exactly in Z/p: the result is bit-identical to oz_dgemm(s, ...).
void oz_dgemm_fmm(int s, const gen_scheme *g, size_t m, size_t k, size_t n, const double *A, size_t lda,
                  const double *B, size_t ldb, double *C, size_t ldc, oz_times *tm);
// Specialised: one exact Strassen-Winograd level per modulus (unsigned residues, AMX u8 x u8),
// memory-blocked; bit-identical to oz_dgemm(s, ...).
void oz_dgemm_w(int s, size_t m, size_t k, size_t n, const double *A, size_t lda, const double *B, size_t ldb,
                double *C, size_t ldc, oz_times *tm);
