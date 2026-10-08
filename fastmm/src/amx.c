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

// AMX is used when the CPU has AMX-TILE and AMX-INT8 (CPUID 7.0:EDX bits 24, 25), the kernel grants the
// tile state, and AMX_DISABLE is not set.  Otherwise a portable AVX-512BW kernel computes the same
// exact int32 products (bit-identical results, much slower): see amx_gemm_portable below.
#include <cpuid.h>
static int g_amx = -1;
static int amx_available(void) {
  if (g_amx < 0) {
    unsigned a, b, c, d;
    int has = __get_cpuid_count(7, 0, &a, &b, &c, &d) && ((d >> 24) & 1) && ((d >> 25) & 1);
    const char *e = getenv("AMX_DISABLE");
    if (e && atoi(e)) has = 0;
    if (has && syscall(SYS_arch_prctl, ARCH_REQ_XCOMP_PERM, XFEATURE_XTILEDATA) != 0) has = 0;
    g_amx = has;
  }
  return g_amx;
}
int amx_init(void) { return amx_available() ? 0 : -1; }
int amx_hardware(void) { return amx_available(); }

// Portable exact int8 GEMM on the same packed layouts: int8 -> int16, vpmaddwd (pairs of k), int32
// accumulation with the same two's-complement wrap-around as the AMX dot products.
static void amx_gemm_portable(int uns, size_t Mp, size_t Np, size_t Kp, const int8_t *Ap, const int8_t *Bp,
                              amx_epilogue_fn epi, void *ctx) {
  size_t nkb = Kp / 64, nbi = Mp / 32, nbj = Np / 32;
  #pragma omp parallel
  {
    int32_t blk[32 * 32] __attribute__((aligned(64)));
    int16_t a16[16 * 64] __attribute__((aligned(64)));
    __m512i b16[16][2];
    #pragma omp for schedule(dynamic, 1) collapse(2)
    for (size_t bi = 0; bi < nbi; bi++)
      for (size_t bj = 0; bj < nbj; bj++) {
        for (int ti = 0; ti < 2; ti++)
          for (int tj = 0; tj < 2; tj++) {
            size_t i16 = 2 * bi + ti, j16 = 2 * bj + tj;
            __m512i acc[16][2];
            for (int r = 0; r < 16; r++) acc[r][0] = acc[r][1] = _mm512_setzero_si512();
            for (size_t kb = 0; kb < nkb; kb++) {
              const int8_t *at = Ap + amx_tile_off(i16, kb, Kp), *bt = Bp + amx_tile_off(j16, kb, Kp);
              for (int r = 0; r < 16; r++)
                for (int h = 0; h < 2; h++) {
                  __m256i a8 = _mm256_loadu_si256((const __m256i *)(at + 64 * r + 32 * h));
                  __m256i b8 = _mm256_loadu_si256((const __m256i *)(bt + 64 * r + 32 * h));
                  _mm512_store_si512(a16 + 64 * r + 32 * h, uns ? _mm512_cvtepu8_epi16(a8) : _mm512_cvtepi8_epi16(a8));
                  b16[r][h] = uns ? _mm512_cvtepu8_epi16(b8) : _mm512_cvtepi8_epi16(b8);
                }
              for (int r = 0; r < 16; r++)  // row r of the A tile = output row
                for (int rb = 0; rb < 16; rb++) {  // row rb of the B tile = k values 4rb..4rb+3 of 16 columns
                  long long w;
                  memcpy(&w, a16 + 64 * r + 4 * rb, 8);
                  __m512i bc = _mm512_set1_epi64(w);
                  acc[r][0] = _mm512_add_epi32(acc[r][0], _mm512_madd_epi16(b16[rb][0], bc));
                  acc[r][1] = _mm512_add_epi32(acc[r][1], _mm512_madd_epi16(b16[rb][1], bc));
                }
            }
            for (int r = 0; r < 16; r++)
              for (int h = 0; h < 2; h++) {
                __m512i sum = _mm512_add_epi32(acc[r][h], _mm512_srli_epi64(acc[r][h], 32));
                _mm256_storeu_si256((__m256i *)(blk + (16 * ti + r) * 32 + 16 * tj + 8 * h), _mm512_cvtepi64_epi32(sum));
              }
          }
        epi(blk, 32, 32 * bi, 32 * bj, ctx);
      }
  }
}

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

// unsigned (u8 x u8) twin of kernel_14.
// 1x4: C tiles 0..3 (16 x 64), A tile 4, B tiles 6/7 alternating.  bq = first tile of B
// panel q (consecutive kb tiles 1 KiB apart).
static inline void kernel_14u(const int8_t *a, const int8_t *b0, const int8_t *b1, const int8_t *b2,
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
    _tile_loadd(6, b0 + o, 64); _tile_dpbuud(0, 4, 6);
    _tile_loadd(7, b1 + o, 64); _tile_dpbuud(1, 4, 7);
    _tile_loadd(6, b2 + o, 64); _tile_dpbuud(2, 4, 6);
    _tile_loadd(7, b3 + o, 64); _tile_dpbuud(3, 4, 7);
  }
  _tile_stored(0, c, CST); _tile_stored(1, c + 16, CST); _tile_stored(2, c + 32, CST); _tile_stored(3, c + 48, CST);
}

static void amx_gemm_impl(int uns, size_t Mp, size_t Np, size_t Kp, const int8_t *Ap, const int8_t *Bp,
                          amx_epilogue_fn epi, void *ctx) {
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
            for (size_t bi = 0; bi < mc / 16; bi++) {
              if (uns)
                kernel_14u(Ap + amx_tile_off((i0 + 16 * bi) / 16, kb0, Kp), b0, b1, b2, b3, nkb,
                           cbuf + 16 * bi * AMX_NC + 64 * bj, kb0 > 0);
              else
                kernel_14(Ap + amx_tile_off((i0 + 16 * bi) / 16, kb0, Kp), b0, b1, b2, b3, nkb,
                          cbuf + 16 * bi * AMX_NC + 64 * bj, kb0 > 0);
            }
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

void amx_gemm_s8s8(size_t Mp, size_t Np, size_t Kp, const int8_t *Ap, const int8_t *Bp, amx_epilogue_fn epi,
                   void *ctx) {
  if (!amx_available()) { amx_gemm_portable(0, Mp, Np, Kp, Ap, Bp, epi, ctx); return; }
  amx_gemm_impl(0, Mp, Np, Kp, Ap, Bp, epi, ctx);
}
void amx_gemm_u8u8(size_t Mp, size_t Np, size_t Kp, const uint8_t *Ap, const uint8_t *Bp, amx_epilogue_fn epi,
                   void *ctx) {
#if AMX_KERNEL == 22
#error "unsigned products are only implemented for the 1x4 kernel"
#endif
  if (!amx_available()) { amx_gemm_portable(1, Mp, Np, Kp, (const int8_t *)Ap, (const int8_t *)Bp, epi, ctx); return; }
  amx_gemm_impl(1, Mp, Np, Kp, (const int8_t *)Ap, (const int8_t *)Bp, epi, ctx);
}
