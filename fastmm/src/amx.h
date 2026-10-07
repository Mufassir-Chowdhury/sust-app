// Intel AMX-INT8 GEMM on pre-packed operands.
//   Packed A ("tile rows"):  for each 16-row block i16 and 64-wide k block kb, a 1 KiB tile
//                            A_tile[r][c] = A[16*i16 + r][64*kb + c]           (int8)
//   Packed B ("VNNI tiles"): for each 16-col block j16 and 64-wide k block kb, a 1 KiB tile
//                            B_tile[r][4*c + q] = B[64*kb + 4*r + q][16*j16 + c] (int8)
// Dimensions are padded with zeros: rows (Mp) to multiples of 32, columns (Np) to multiples of
// AMX_COLPAD = 64, and k to multiples of 64.
#define AMX_COLPAD 64
#pragma once
#include <stdint.h>
#include <stddef.h>

int amx_init(void);  // request permission from the kernel; returns 0 on success

static inline size_t amx_pad(size_t x, size_t q) { return (x + q - 1) / q * q; }

// Byte offset of tile (blk16, kb) in a packed operand with Kp = padded k.
static inline size_t amx_tile_off(size_t blk16, size_t kb, size_t Kp) { return (blk16 * (Kp / 64) + kb) * 1024; }

// Epilogue callback: receives a 32x32 int32 block (row-major, row stride ld elements) of C = A*B
// for rows [i0, i0+32) and cols [j0, j0+32), with the full k range accumulated.
typedef void (*amx_epilogue_fn)(const int32_t *blk, size_t ld, size_t i0, size_t j0, void *ctx);

// C = A*B over full K (Kp multiple of 64, Mp multiple of 32, Np multiple of 64), parallel over threads.
// Every 32x32 block of C is handed to `epi`.
void amx_gemm_s8s8(size_t Mp, size_t Np, size_t Kp, const int8_t *Ap, const int8_t *Bp, amx_epilogue_fn epi,
                   void *ctx);
// Same with unsigned 8-bit operands (u8 x u8 -> int32 exact while Kp * 255^2 < 2^31).
void amx_gemm_u8u8(size_t Mp, size_t Np, size_t Kp, const uint8_t *Ap, const uint8_t *Bp, amx_epilogue_fn epi,
                   void *ctx);
