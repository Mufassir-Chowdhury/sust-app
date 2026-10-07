# LIBXS Ozaki DGEMM vs this project's emulation vs MKL (same machine, 4 threads)

LIBXS (H. Pabst, https://github.com/hfp/libxs) commit 6d264dffe880 (2026-10-07), built with its
default Makefiles (`make -j4` at the root, then in `samples/ozaki`; `-march=native` was used), and
used as an LD_PRELOAD replacement of the Fortran `dgemm_` called by `src/bench_fortran.c` (MKL
underneath). Settings tried: `OZAKI=2 OZAKI_N=14/16` (CRT, scheme II), `OZAKI=1` (slicing, the CPU
default on AMX), `OZAKI_AMX=0/1`, `OMP_PLACES=cores OMP_PROC_BIND=true`. Same input generator and
error metrics as `fmmtest acc` (double-double reference). **Caveat:** I may not have found LIBXS's best
configuration; its own driver (`dgemm-wrap.x 4000`) reports the same rate (66.5 GFLOP/s vs 208.7 for
the statically linked OpenBLAS), so the build itself does not look broken.

## Speed (effective GFLOP/s = 2n^3 / time; median of 3-5 calls)

| n | MKL | LIBXS CRT 14 | LIBXS CRT 16 | LIBXS slicing | this work oz14 | this work oz16 |
|---|---|---|---|---|---|---|
| 2000 | 179-224 | 41 | 32-36 | 46-54 | ~270 (1.35x MKL) | ~235 (1.17x) |
| 4000 | 285 | 67 | 60-62 | 63-69 | ~360 (1.2-1.3x) | ~320 (1.1-1.2x) |

## Accuracy at n = 2000 (max componentwise |C-C*|/(|A||B|); median relative error)

| input | MKL | LIBXS CRT 16 | LIBXS slicing | this work oz14 | this work oz16 |
|---|---|---|---|---|---|
| uniform | 1.3e-16; 6.1e-16 | 2.0e-17; 4.8e-17 | 4.7e-17; 1.1e-16 | 1.5e-16; 1.3e-15 | 7.4e-18; 3.8e-17 |
| random exponents 2^+-20 | 1.2e-15; 4.9e-16 | 4.1e-15; 9.2e-16 | 4.1e-15; 9.4e-16 | 6.9e-15; 1.9e-15 | 1.0e-16; 4.5e-17 |
| inner (k) scaling 2^+-20 | 1.3e-16; 6.1e-16 | **3.9e-06; 3.0e-05** | **3.9e-06; 3.1e-05** | 1.5e-16; 1.3e-15 | 7.4e-18; 3.8e-17 |
| row/col scaling 2^+-32 | 1.3e-16; 6.1e-16 | 2.0e-17; 4.8e-17 | 4.7e-17; 1.1e-16 | 1.5e-16; 1.3e-15 | 7.4e-18; 3.8e-17 |
| cancellation (C ~ 2^-30 A B) | 1.4e-16; 9.0e-07 | 4.8e-18; 6.3e-08 | 4.8e-18; 6.3e-08 | 1.0e-16; 1.3e-06 | 1.9e-19; 2.1e-09 |

Reading: on this machine the open-source LIBXS Ozaki path (CPU, AMX) is about 3-6x slower than MKL
DGEMM, while this project's implementation of the same scheme is 1.1-1.5x faster than MKL. The
difference is engineering (int8 GEMM efficiency, conversion), not the method. LIBXS has no inner
(k-dimension) scaling and loses ~10 digits on inner-scaled inputs; with it (this work) the error stays
below MKL's.
