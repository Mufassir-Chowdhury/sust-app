// AMX-INT8 GEMM on pre-packed operands (see amx.h for the layouts).
// Two register blockings (compile-time AMX_KERNEL):
//   22: 2x2 C tiles (32x32 int32), 2 A + 2 B tiles per 64-wide k step;
//   14: 1x4 C tiles (16x64 int32), 1 A + 4 B tiles per k step (B panel 64 cols x KC kept in L1,
//       A streamed from L2 at half the rate of the 2x2 kernel).
// Cache blocking: macro tile MC x NC of C kept as int32 (row-major, ld = NC) in a per-thread
// buffer; k in chunks of KC.  Column macro-blocks are processed one after another and threads
// share the row blocks, so the K x NC slab of B is reused through L3.
#include <immintrin.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdlib.h>
#include <omp.h>
#include "amx.h"

#define ARCH_REQ_XCOMP_PERM 0x1023
#define XFEATURE_XTILEDATA 18

#ifndef AMX_MC
#define AMX_MC 128
#endif
#ifndef AMX_NC
#define AMX_NC 256
#endif
#ifndef AMX_KC
#define AMX_KC 512
#endif
#ifndef AMX_KERNEL
#define AMX_KERNEL 14
#endif

typedef struct {
  uint8_t palette_id, start_row, reserved[14];
  uint16_t colsb[16];
  uint8_t rows[16];
} __attribute__((packed)) tilecfg_t;

int amx_init(void) { return (int)syscall(SYS_arch_prctl, ARCH_REQ_XCOMP_PERM, XFEATURE_XTILEDATA); }

static void tile_config(void) {
  tilecfg_t cfg;
  memset(&cfg, 0, sizeof cfg);
  cfg.palette_id = 1;
  for (int t = 0; t < 8; t++) { cfg.rows[t] = 16; cfg.colsb[t] = 64; }
  _tile_loadconfig(&cfg);
}

#define CST (AMX_NC * 4)  // byte stride of a cbuf row

// 2x2: C tiles 0..3, A tiles 4,5, B tiles 6,7.  c points at the 32x32 corner in cbuf.
static inline void kernel_22(const int8_t *a0, const int8_t *a1, const int8_t *b0, const int8_t *b1, size_t nkb,
                             int32_t *c, int load_c) {
  if (load_c) {
    _tile_loadd(0, c, CST); _tile_loadd(1, c + 16, CST);
    _tile_loadd(2, c + 16 * AMX_NC, CST); _tile_loadd(3, c + 16 * AMX_NC + 16, CST);
  } else { _tile_zero(0); _tile_zero(1); _tile_zero(2); _tile_zero(3); }
  _tile_loadd(4, a0, 64); _tile_loadd(6, b0, 64); _tile_loadd(7, b1, 64); _tile_loadd(5, a1, 64);
  for (size_t kb = 0; kb + 1 < nkb; kb++) {
    size_t nx = (kb + 1) * 1024;
    _tile_dpbssd(0, 4, 6); _tile_dpbssd(1, 4, 7);
    _tile_loadd(4, a0 + nx, 64);
    _tile_dpbssd(2, 5, 6);
    _tile_loadd(6, b0 + nx, 64);
    _tile_dpbssd(3, 5, 7);
    _tile_loadd(7, b1 + nx, 64); _tile_loadd(5, a1 + nx, 64);
  }
  _tile_dpbssd(0, 4, 6); _tile_dpbssd(1, 4, 7); _tile_dpbssd(2, 5, 6); _tile_dpbssd(3, 5, 7);
  _tile_stored(0, c, CST); _tile_stored(1, c + 16, CST);
  _tile_stored(2, c + 16 * AMX_NC, CST); _tile_stored(3, c + 16 * AMX_NC + 16, CST);
}

// 1x4: C tiles 0..3 (16 x 64), A tile 4, B tiles 6/7 alternating.  bq = first tile of B
// panel q (consecutive kb tiles 1 KiB apart).
static inline void kernel_14(const int8_t *a, const int8_t *b0, const int8_t *b1, const int8_t *b2,
                             const int8_t *b3, size_t nkb, int32_t *c, int load_c) {
  if (load_c) {
    _tile_loadd(0, c, CST); _tile_loadd(1, c + 16, CST); _tile_loadd(2, c + 32, CST); _tile_loadd(3, c + 48, CST);
  } else { _tile_zero(0); _tile_zero(1); _tile_zero(2); _tile_zero(3); }
  for (size_t kb = 0; kb < nkb; kb++) {
    size_t o = kb * 1024;
#ifdef AMX_PF
    // prefetch the A tile AMX_PF steps ahead (it streams from L2) into L1
    for (int l = 0; l < 1024; l += 64) _mm_prefetch((const char *)a + o + AMX_PF * 1024 + l, _MM_HINT_T0);
#endif
    _tile_loadd(4, a + o, 64);
    _tile_loadd(6, b0 + o, 64); _tile_dpbssd(0, 4, 6);
    _tile_loadd(7, b1 + o, 64); _tile_dpbssd(1, 4, 7);
    _tile_loadd(6, b2 + o, 64); _tile_dpbssd(2, 4, 6);
    _tile_loadd(7, b3 + o, 64); _tile_dpbssd(3, 4, 7);
  }
  _tile_stored(0, c, CST); _tile_stored(1, c + 16, CST); _tile_stored(2, c + 32, CST); _tile_stored(3, c + 48, CST);
}

void amx_gemm_s8s8(size_t Mp, size_t Np, size_t Kp, const int8_t *Ap, const int8_t *Bp, amx_epilogue_fn epi,
                   void *ctx) {
  size_t nkbt = Kp / 64;
  size_t nmt = (Mp + AMX_MC - 1) / AMX_MC, nnt = (Np + AMX_NC - 1) / AMX_NC;
  #pragma omp parallel
  {
    tile_config();
    // per-thread C buffer, kept across calls (avoids an mmap/munmap pair per call)
    static __thread int32_t *cbuf = NULL;
    if (!cbuf) cbuf = aligned_alloc(4096, (size_t)AMX_MC * AMX_NC * 4);
    for (size_t nt = 0; nt < nnt; nt++) {
      #pragma omp for schedule(dynamic, 1)
      for (size_t mt = 0; mt < nmt; mt++) {
        size_t i0 = mt * AMX_MC, j0 = nt * AMX_NC;
        size_t mc = Mp - i0 < AMX_MC ? Mp - i0 : AMX_MC;
        size_t nc = Np - j0 < AMX_NC ? Np - j0 : AMX_NC;
        for (size_t kb0 = 0; kb0 < nkbt; kb0 += AMX_KC / 64) {
          size_t nkb = nkbt - kb0 < AMX_KC / 64 ? nkbt - kb0 : AMX_KC / 64;
#if AMX_KERNEL == 22
          for (size_t bj = 0; bj < nc / 32; bj++) {
            size_t j16 = (j0 + 32 * bj) / 16;
            const int8_t *b0 = Bp + amx_tile_off(j16, kb0, Kp), *b1 = Bp + amx_tile_off(j16 + 1, kb0, Kp);
            for (size_t bi = 0; bi < mc / 32; bi++) {
              size_t i16 = (i0 + 32 * bi) / 16;
              kernel_22(Ap + amx_tile_off(i16, kb0, Kp), Ap + amx_tile_off(i16 + 1, kb0, Kp), b0, b1, nkb,
                        cbuf + 32 * bi * AMX_NC + 32 * bj, kb0 > 0);
            }
          }
#else
          for (size_t bj = 0; bj < nc / 64; bj++) {
            size_t j16 = (j0 + 64 * bj) / 16;
            const int8_t *b0 = Bp + amx_tile_off(j16, kb0, Kp), *b1 = Bp + amx_tile_off(j16 + 1, kb0, Kp);
            const int8_t *b2 = Bp + amx_tile_off(j16 + 2, kb0, Kp), *b3 = Bp + amx_tile_off(j16 + 3, kb0, Kp);
            for (size_t bi = 0; bi < mc / 16; bi++)
              kernel_14(Ap + amx_tile_off((i0 + 16 * bi) / 16, kb0, Kp), b0, b1, b2, b3, nkb,
                        cbuf + 16 * bi * AMX_NC + 64 * bj, kb0 > 0);
          }
#endif
        }
        for (size_t bi = 0; bi < mc / 32; bi++)
          for (size_t bj = 0; bj < nc / 32; bj++)
            epi(cbuf + 32 * bi * AMX_NC + 32 * bj, AMX_NC, i0 + 32 * bi, j0 + 32 * bj, ctx);
      }
    }
    _tile_release();
  }
}
