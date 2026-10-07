# fastmm: looking for a dense double-precision matrix multiply faster than Strassen in practice

Research log and code. One machine: 4 vCPUs of an Intel Xeon 5th-gen ("Emerald Rapids",
family 6 model 207) with AVX-512 and **AMX-INT8**, 15 GB RAM, KVM guest. All numbers below were
measured there with 4 threads unless stated.

## Verdict

*Draft: final numbers for n = 16000 and 20000 and the LIBXS comparison are filled in below.*

**Goal:** a general dense algorithm, double-precision accurate, measurably faster than both tuned BLAS
(MKL here; OpenBLAS and BLIS are slower on this machine) and a careful Strassen-Winograd, for
n = 500-20000.

**What I achieved:**

1. **No bilinear (Strassen-type) algorithm beats Strassen-Winograd meaningfully.**
   * The best plans I found are fused multi-level Winograd, or one memory-lean Winograd level over a
     <4,4,4;48> rational scheme with task-parallel leaves.
   * They reach 1.1-1.3x over MKL for n >= 2000. They are slower than MKL at n = 4000-6000 and at
     n <= 1000.
   * They are always within the noise of plain Strassen-Winograd plans.
   * The lower-exponent schemes I tried (<4,4,4;48>, <3,3,6;40>, <5,5,5;93>) do not change this at
     n <= 20000. My cost model says why: multiplication count and leaf BLAS efficiency dominate, and
     the bigger base cases do not fit in memory in a fused implementation.
2. **What does beat both is a different kind of algorithm: FP64 GEMM emulated with exact int8 products
   on the AMX units** (Ozaki scheme II: integer scaling, Chinese remainder theorem, 14-16 int8 GEMMs).
   * The method is published and open-source prior art exists (LIBXS, Kouya 2026). What is mine is
     this implementation and its measurement against MKL and Strassen.
   * Measured paired speedup over MKL DGEMM: 1.2-1.5x with 14 moduli and 1.1-1.3x with 16 moduli,
     for 2000 <= n <= 12000.
   * At n <= 1000 it is slower than MKL. At n >= 16000, two levels of Strassen-Winograd catch up.
3. **Accuracy of the emulation:**
   * With 16 moduli and the built-in power-of-two scaling of rows, columns and the inner dimension, the
     componentwise error is at or below MKL's on every input class I tested. The exception is matrices
     whose entries have independently random exponents spanning more than about 2^±32; there it
     degrades, while DGEMM does not.
   * With 14 moduli it is DGEMM-level on well-scaled data (median relative error about 2x DGEMM's) and
     up to 100x worse on wide exponent ranges.
   * Strassen-type algorithms are 2-1000x worse than DGEMM on these tests, and catastrophically wrong
     on row- or column-scaled inputs unless they are rescaled.
4. **It is hardware-specific.** The gain exists only because int8 matrix units are about 26x faster
   per operation than FP64 FMA here. On a CPU without AMX (AVX512-VNNI only: about 8x) the emulation
   loses.
5. **Strassen applied exactly inside the modular int8 products** gives bit-identical results to the
   plain emulation (verified) and cuts the int8 work by 13%. My implementation of the modular-domain
   additions costs more than that saves at n <= 8000, so it is not faster yet.

So: a partial success. On this hardware the emulation is faster than tuned BLAS and than my best
Strassen for roughly 2000 <= n <= 12000, with DGEMM-level accuracy except on adversarial exponent
ranges. It is not a new matrix multiplication algorithm, and it does not lower the exponent.

(Full tables, method and caveats below. Everything can be re-run with `./run_all.sh`.)

## Contents

1. Machine, libraries, method of measurement
2. Baselines, and what limits Strassen here
3. Ideas, ranked, with kill tests (`docs/ideas.md`)
4. Bilinear schemes: library, exact checks, engine, cost model, search
5. FP64 emulation on AMX-INT8
6. Accuracy
7. Speed: full table
8. What is and is not new
9. What failed, and why
10. What I could not do or verify
11. Reproduce; code map

## 1. Machine, libraries, method

* **CPU:** Intel Xeon 5th generation, family 6 model 207 (Emerald Rapids), 4 vCPUs (KVM), 1 thread per core.
  * AVX-512, AVX512-VNNI/BF16/FP16, AMX-INT8/BF16.
  * L2 2 MB per core. The guest reports a 260 MB L3, but streaming measurements suggest the effective
    share is far smaller.
  * DRAM bandwidth 25-35 GB/s (STREAM-like `src/stream.c`).
  * Cores run at about 2.5 GHz under load. Measured AMX peak is 5.1-5.6 Tera int8-ops/s per core, and
    it does not drop with all 4 cores busy.
* **Software:** Ubuntu 24.04, gcc 13.3, `-O3 -march=native -fopenmp`.
  * **MKL 2026.1** (pip `mkl`, GNU OpenMP threading layer) is the reference BLAS. It is by far the
    fastest DGEMM here: about 280 GFLOP/s on 4 cores for n >= 4000, against OpenBLAS 0.3.26 at about
    200-215 and BLIS 0.9.0 (Debian) at about 120.
  * oneDNN 3.9.2, built from source, is used only as a reference for the AMX int8 GEMM speed.
* **Timing protocol** (`bin/fmmtest time`):
  * Same process and same input data; one warm-up call per method.
  * Methods are run round-robin (interleaved) for R rounds: R = 15 for n <= 2000, 9 up to 4000,
    5 up to 8000, 3 above.
  * Reported number is the median over rounds of the per-round ratio t_MKL / t_method ("paired
    speedup"), with its interquartile range.
  * Paired ratios are robust to the slow drift of this shared VM. Medians of single runs swing by up to
    30% for n <= 3000 (MKL at n = 2000 measured anywhere from 150 to 240 GFLOP/s across sessions);
    for n >= 4000 they agree within about 3%. Differences below about 5-10% at small n are noise.
  * All threads: `OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 OMP_PROC_BIND=close`.
* **Accuracy protocol** (`bin/fmmtest acc`):
  * The reference is a double-double dot product, with error about k*u^2*|A||B|.
  * Error is computed on all entries (n = 1000) or 20000 sampled entries (n > 1000).
  * Metrics: max componentwise |C - C*| / (|A||B|), the quantity DGEMM bounds by about k*u; median
    relative error; and a normwise metric.
  * Six input classes: uniform, positive, heavy cancellation, outside (row/column) scaling, inside
    (k-dimension) scaling, and independent random exponents 2^e with e uniform in [-r, r].

## 2. Baselines, and what limits Strassen on this machine

MKL DGEMM, GFLOP/s (calibration medians, `results/calib.json`):

| n | 500 | 1000 | 1500 | 2000 | 3000 | 4000 | 6000 | 8000 |
|---|---|---|---|---|---|---|---|---|
| 4 threads | 202 | 210 | 219 | 203 | 234 | 282 | 288 | 283 |
| 1 thread, alone | 70 | 74 | 66 | 68 | 67 | 70 | | |
| 1 thread, 4 running concurrently | 44 | 53 | 63 | 61 | 66 | 66 | | |

Peak is about 320 GFLOP/s (4 cores x 32 flop/cycle x about 2.5 GHz), so MKL reaches about 88% for
n >= 4000 and only 65-75% for 1000 <= n <= 3000.

What limits Strassen-Winograd here, in plain words, with the measurements behind each point:

* **Leaf efficiency (the main limit).**
  * One Strassen level saves 12.5% of the flops but moves the work to half-size products. For
    n <= 6000 these fall in the range where MKL is 15-30% less efficient.
  * Measured, one memory-lean level: 0.79x of MKL at n = 4000, 0.76x at 6000, 1.04-1.10x at 8000,
    1.11x at 10000-16000.
  * Two levels pay only when the leaves are >= 4000 (n = 16000: 1.21x). Three never pay
    (n = 16000: 1.00x).
* **Parallel scaling.** Running the 7 (or 49) sub-products as single-threaded tasks avoids MKL's weak
  multithreaded efficiency at medium sizes, but has costs:
  * 4 concurrent single-threaded DGEMMs reach only 250-265 GFLOP/s in total, not 4 x 70;
  * 7 tasks on 4 cores leave 1/8 idle;
  * this is what lets two-level Winograd as one task-parallel <4,4,4;49> level reach 1.25x at
    n = 2000-3000.
* **Additions and memory traffic (second order).**
  * A fused level moves 7.5n^2 doubles; my memory-lean level moves about 8n^2.
  * At about 41 GB/s measured that is 0.1-0.2 s at n = 8000 (3-6%) and 0.03 s at n = 4000 (6%).
  * Alternative bases or sparser schemes attack this term. The model says removing all of it would
    gain under 4% (section 4).
* **Memory.**
  * The memory-lean schedule needs (2/3)n^2 extra.
  * Fused or task-parallel plans need 3-16 n^2. At n = 16000-20000 this, not speed, rules out the
    larger base cases.
* **Stability** (section 6).
  * Componentwise error is 2-4x DGEMM's per level on well-scaled data and 10-1000x on data with
    wide exponent ranges.
  * It reaches 1e+17 to 1e+21 (garbage) for rows or columns scaled by 2^±32.
  * Outside-inside power-of-two scaling (Ballard et al. 2016) repairs the diagonally scaled cases
    but not entrywise exponent spread.

## 3. Ideas, ranked, with kill tests

Full table: `docs/ideas.md`. In short:

| # | Idea | Weakness it attacks | Kill test | Outcome |
|---|---|---|---|---|
| 1 | FP64 emulation on AMX-INT8 (Ozaki II) | FP64 FMA throughput itself | int8:FP64 throughput ratio < ~16 | ratio about 26; **1.2-1.5x over MKL** for 2000-12000 |
| 2 | Fused multi-level Winograd (one <4,4,4;49> level), task-parallel leaves | traffic, MKL's mid-size threading | no gain over 1-level Strassen at n = 8000 | small, within noise of Winograd plans |
| 3 | Lower-rank base cases (<4,4,4;48>, <3,3,6;40>, <5,5,5;93>, <3,3,3;23>) | multiplication count | cost model, then measurement at 8000/12000 | <4,4,4;48> best bilinear plan at 12000 (1.20x vs 1.16x), within noise |
| 4 | Alternative basis / sparsified schemes | additions | model sensitivity | killed: under 4% even if the passes were free |
| 5 | Exact modular Strassen inside the emulation | Strassen's instability (absent over Z/p) | int8 work down by about 1/8 and overhead smaller | bit-identical; generic version slower; specialised Winograd version 3-7% faster than the plain emulation at 6000-12000 (14 moduli), slower at n <= 4000 |
| 6 | Floating-point Strassen over emulated leaves | both | speed and accuracy at 8000 | no gain |
| 7 | Border-rank (APA) schemes | multiplication count | error analysis | killed: about sqrt(u), 8 digits lost |
| 9 | Faster AMX kernel | dominant cost of #1 | beat oneDNN (7.0 Tops/s) | matched (7.2-7.6), not beaten |
| 10 | Power-of-two outside-inside scaling | sensitivity to badly scaled data | accuracy tests | fixes diagonal scalings for both families |

## 4. Bilinear schemes: library, exactness, engine, cost model, search

* **Library** (`schemes/`, index `schemes/INDEX.md`):
  * 59 published schemes, from <2,2,2;7> to <5,5,5;93>, including the rational <4,4,4;48> of
    Dumas-Pernet-Sedoglavic (via PLinOpt), AlphaTensor and AlphaEvolve schemes, Smirnov's
    <3,3,6;40>, flip-graph schemes and Benson-Ballard's base cases.
  * Machine-readable JSON (U, V, W).
* **Exactness.** `tools/scheme_check.py` verifies every scheme against the matrix multiplication
  tensor in exact rational (or Gaussian-rational) arithmetic: sum_l U V W = delta delta delta. All 59
  pass. Composed schemes (e.g. Winograd x Winograd) are re-verified before use (`tools/slp.py`).
* **Engine.**
  * `tools/slp.py` turns (U,V,W) into three straight-line programs with greedy pairwise CSE. It finds
    Winograd's 15 additions; Strassen's needs 18.
  * `src/gen.c` executes any scheme recursively with fused single-pass additions, in depth-first or
    task-parallel (OpenMP tasks) mode, with dynamic peeling for odd sizes.
  * `src/sw.c` is a separate memory-lean Strassen-Winograd: Boyer et al. schedule, two temporaries,
    fused post-additions, two products accumulated through BLAS beta. It can hand its sub-products to
    the generic engine.
* **Cost model** (`docs/costmodel.md`, `tools/costmodel.py`):
  * Time = leaf DGEMM time (from measured tables, including concurrent single-threaded runs) + fused
    pass bytes / measured bandwidth + task-schedule makespan.
  * Validated on 30 plans at n = 2000/4000/8000 (`results/model_validation.md`): mean |error| 9.4%,
    max 25%. Depth-first plans are within 8%; task-parallel plans are over-predicted by 6-25%.
  * A memory model marks plans that do not fit in RAM.
* **Search.**
  * `tools/rank_plans.py` ranks every scheme in the library as a depth-first, task-parallel, or
    below-a-memory-lean-Winograd level, at 6 sizes (`results/plan_ranking.md`).
  * The top predictions were measured. At n = 12000 the predicted best feasible plan (memory-lean
    Winograd over a task-parallel <4,4,4;48>, predicted 1.21x) measured 1.20x, the best bilinear
    result at that size (Winograd x Winograd: 1.16x, <3,3,6;40>: 1.12x).
  * A search for new schemes (flip graphs, alternating least squares, SAT) was **not** run. The model
    shows the free parameters such a search could improve for a known rank (sparsity, number of
    operand buffers) are worth under 4% here. Only a lower rank would matter, and the known ranks for
    these formats come from very large searches; even a hypothetical <4,4,4;47> would be worth
    about 2%.

## 5. FP64 emulation on AMX-INT8 (Ozaki scheme II, CRT)

`src/ozaki.c`, `src/amx.c`, `src/oz_internal.h`, `tools/gen_oz_consts.py`.

1. **Scale.** Each row of A gets a power of two 2^sigma_i, each column of B 2^tau_j, and the inner
   dimension a balancing 2^e_k, with A' = A·2^e and B' = 2^-e·B, exact. The scaled entries are rounded
   to integers with ||row||_2, ||col||_2 < 2^L. By Cauchy-Schwarz every entry of the integer product is
   below P/4, where P is the product of the moduli.
2. **Residues.** For s pairwise coprime moduli <= 256, the residues are int8. They are computed
   exactly from a 32-bit split of each integer, with one FMA-based floor per modulus, and written
   straight into AMX tile layout with non-temporal stores.
3. **Products.** One AMX int8 GEMM per modulus (`src/amx.c`: 1x4 tile kernel, 128x256 macro tiles,
   K chunks of 512). The int32 accumulation is exact for k <= 131071; longer k is split. The epilogue
   reduces mod p to bytes.
4. **CRT.** Reconstruction in double with a three-way split of the CRT weights. The constants are
   generated and checked with exact integers. The high and middle sums are exact, then the result is
   rescaled with `scalef`.
5. **Guards.** Memory blocking keeps the workspace under 80% of free RAM (n = 20000 fits in 15 GB). A
   persistent pre-faulted workspace is used. NaN/Inf inputs fall back to MKL.

* The only rounding errors are the rounding of the scaled inputs to L-bit integers and the final
  rounding to double.
* L is about 54 bits for 14 moduli, 57 for 15, and 61 for 16, at k = 8192.
* Throughput: the AMX GEMM reaches 7.2-7.6 Tops/s on 4 cores, against oneDNN 3.9.2's 7.0 Tops/s and
  MKL's `cblas_gemm_s8u8s32` at 5.2 Tops/s on this machine. That is about 26x the DGEMM rate but only
  about 35% of the AMX peak.
* Time split at n = 8000 with 14 moduli: conversion 0.24 s, int8 GEMMs 2.4-2.6 s, CRT 0.08 s, scaling
  0.02 s.

## 6. Accuracy

Full tables: `results/accuracy.md` (raw: `results/accuracy.txt`, script `bench/accuracy.sh`). Sizes
n = 1000 (all entries checked), 2000 and 4000 (20000 sampled entries). Reference: double-double dot
products. Metric shown here: max componentwise |C - C*| / (|A||B|), the quantity DGEMM bounds by
about k*u; `accuracy.md` also has the median relative error and a normwise metric.

Columns: `sw1-3` = 1-3 memory-lean Winograd levels; `sc:` = the same with outside-inside power-of-two
scaling; `W2`, `P48` = one task-parallel <4,4,4;49> / <4,4,4;48> level; `ozS` = emulation with S
moduli (built-in row, column and inner scaling).

| n | input | DGEMM | sw1 | sw2 | sc:sw1 | P48 | oz14 | oz15 | oz16 |
|---|---|---|---|---|---|---|---|---|---|
| 2000 | uniform [-1,1] | 1.3e-16 | 3.3e-16 | 9.8e-16 | 3.3e-16 | 8.0e-16 | 1.5e-16 | 7.4e-18 | 7.4e-18 |
| 2000 | positive | 1.1e-15 | 1.0e-15 | 7.1e-16 | 1.0e-15 | 5.6e-15 | 2.2e-16 | 1.2e-16 | 1.1e-16 |
| 2000 | cancellation | 1.4e-16 | 4.9e-16 | 1.1e-15 | 4.9e-16 | 7.7e-16 | 1.0e-16 | 1.2e-17 | 1.9e-19 |
| 2000 | row/col scaled 2^±32 | 1.3e-16 | **1.8e+17** | **1.5e+21** | 3.3e-16 | **2.3e+20** | 1.5e-16 | 7.4e-18 | 7.4e-18 |
| 2000 | inner scaled 2^±20 | 1.3e-16 | **8.7e-06** | **3.1e-05** | 3.3e-16 | **3.2e-05** | 1.5e-16 | 7.4e-18 | 7.4e-18 |
| 2000 | random exponents 2^±20 | 1.2e-15 | 1.3e-14 | 2.2e-14 | 1.3e-14 | 2.4e-14 | 6.9e-15 | 9.5e-16 | 1.0e-16 |
| 2000 | random exponents 2^±32 | 1.1e-15 | 4.8e-14 | 1.7e-13 | 4.8e-14 | 8.9e-14 | 3.3e-14 | 3.8e-15 | 3.5e-16 |
| 2000 | random exponents 2^±48 | 1.3e-15 | 2.2e-13 | 3.6e-13 | 2.0e-13 | 4.4e-13 | 1.4e-13 | 1.8e-14 | 9.4e-16 |
| 2000 | random exponents 2^±64 | 1.0e-15 | 1.3e-12 | 3.2e-12 | 1.3e-12 | 7.1e-12 | 1.0e-12 | 1.3e-13 | **1.0e-14** |
| 1000 | random exponents 2^±48 | 2.1e-15 | 1.1e-11 | 4.1e-11 | 1.3e-11 | 4.3e-11 | 7.7e-12 | 1.4e-12 | **5.8e-14** |
| 1000 | random exponents 2^±64 | 1.3e-15 | 2.8e-10 | 2.2e-09 | 2.6e-10 | 8.8e-10 | 1.0e-10 | 2.3e-11 | **1.3e-12** |
| 4000 | random exponents 2^±64 | 1.0e-15 | 1.4e-13 | 3.1e-13 | 1.4e-13 | 5.8e-13 | 1.3e-13 | 2.5e-14 | 7.1e-16 |

What the tables say:

* **Emulation, 16 moduli.**
  * At or below DGEMM's componentwise error on every tested size for uniform, positive,
    cancellation, row/column-scaled, inner-scaled inputs, and random exponents up to 2^±32. The
    median relative error is 5x to several hundred times *below* DGEMM's on these classes.
  * Random exponents 2^±48: at n = 2000 and 4000 about DGEMM's (0.6-0.7x), but **28x worse at
    n = 1000**. Random exponents 2^±64: **10x worse at n = 2000 and 1000x worse at n = 1000**
    (0.7x at n = 4000).
  * Why: the error is about 2^-L ||a_i|| ||b_j|| per entry (L is about 61-62 bits for 16 moduli at these k),
    which is far below k·u·|a_i||b_j| unless the entries of a row span many binades and the large
    entries of a_i and b_j sit at different k. DGEMM's error does not depend on that.
    The normwise error stays at DGEMM's level or below in all cases.
* **Emulation, 14 moduli.** About DGEMM's componentwise error on uniform, positive, cancellation and
  diagonally scaled inputs; the median relative error is 2-4x DGEMM's. On random exponents it is
  5-100x worse at 2^±20-2^±32, and up to 10^5 x worse at 2^±64 (n = 1000). **14 moduli is not a
  drop-in DGEMM replacement for badly scaled data; 16 moduli is close to one, with the exception
  above.**
* **Exact modular Winograd inside the emulation** (`oz14+modW` in `accuracy.md`) is bit-identical
  to `oz14` in every case (also checked by `bench/checks.sh`, section 5).
* **Strassen-type algorithms.**
  * Well-scaled data: 2-3x DGEMM's error per level (sw1 2.5x, sw2 7.5x, sw3 up to 19x; the
    task-parallel 49/48 levels 4-8x).
  * Random exponents: 4-20x worse (2^±10), up to 10^5-10^6 x worse (2^±64, n = 1000).
  * Row/column scaling 2^±32: completely wrong (10^17-10^21). Inner scaling: errors 1e-11 at 2^±10,
    1e-5 at 2^±20 (11 digits lost), above 1 at 2^±32.
  * The outside-inside power-of-two scaling `sc:` (Ballard et al. 2016) repairs the diagonally scaled
    cases fully (sc:sw1 = sw1 on unscaled data) but not random exponents.
* **NaN and Inf** (`bin/fmmtest nancheck`, a single NaN or Inf in A): the emulation detects them
  and calls DGEMM, so the NaN/Inf pattern of C matches DGEMM exactly. One Strassen-Winograd level
  spreads a single NaN to 448 entries that DGEMM leaves finite (853 for an Inf), because the
  pre-additions mix rows and columns that the classical product keeps apart.
* **Long k.** For k > 131071 the int32 accumulators could overflow; the emulation splits k
  (checked at k = 140000: oz16 error 1.6e-18, DGEMM 9.1e-18).
* **LIBXS** (the open-source AMX Ozaki-II code) on the same inputs: `results/libxs_compare.md`.
  Without inner scaling it loses about 10 digits on inner-scaled inputs (3.9e-6).

## 7. Speed: full tables

All numbers: paired speedup over MKL DGEMM (4 threads), median over interleaved rounds; > 1 means
faster than MKL. Raw data: `results/sweep.txt`, `results/sweep_oz_final.txt`.

**Main sweep** (`results/sweep.md`, all methods in one session, `bench/sweep.sh`):

| n | MKL GFLOP/s | OpenBLAS GF/s | BLIS GF/s | best Strassen-type plan | its speedup | emulation 14 moduli | 15 | 16 |
|---|---|---|---|---|---|---|---|---|
| 500 | 191 | 59 | 75 | W1 task-parallel | 0.83 | 0.54 | 0.52 | 0.48 |
| 750 | 219 | 167 | 96 | W1 task-parallel | 0.85 | 0.74 | 0.62 | 0.57 |
| 1000 | 197 | 189 | 134 | 1 lean level | 0.93 | 0.88 | 0.79 | 0.75 |
| 1500 | 187 | 189 | 134 | W1 task-parallel | 1.00 | **1.04** | 0.96 | 0.97 |
| 2000 | 203 | 186 | 121 | <4,4,4;49> task-par. | 1.25 | **1.35** | 1.25 | 1.17 |
| 3000 | 212 | 198 | 114 | <4,4,4;48> task-par. | 1.30 | 1.46 | **1.51** | 1.32 |
| 4000 | 265 | 213 | 127 | <4,4,4;48> task-par. | 1.04 | **1.19** | 1.10 | 1.12 |
| 6000 | 271 | 200 | 119 | <4,4,4;49> task-par. | 1.07 | **1.25** | **1.25** | 1.09 |
| 8000 | 275 | 212 | 124 | <4,4,4;49> task-par. | 1.12 | **1.38** | 1.26 | 1.21 |
| 10000 | 271 | 204 | 121 | lean + <4,4,4;48> | 1.15 | **1.31** | 1.28 | 1.18 |
| 12000 | 280 | 215 | 126 | lean + <4,4,4;48> | 1.23 | **1.40** | 1.34 | 1.22 |
| 16000 | 272 | 216 | 127 | lean + <4,4,4;48> | 1.27 | **1.33** | 1.22 | 1.16 |
| 20000 | 273 | 221 | 112 | 2 lean levels | 1.23 | **1.41** | 1.33 | 1.25 |

**Final emulation code** (`results/sweep_oz_final.md`, `bench/sweep_final_oz.sh`; run after the
sweep). Changes since the sweep: memory blocking that converts each operand once where it fits and
respects the container's cgroup memory limit, NaN/Inf and long-k guards, and the exact-Winograd
variant `ozw`. Each cell is paired against MKL in its own session.

| n | oz14 (plain) | ozw14 (exact Winograd) | oz16 (plain) | ozw16 (exact Winograd) |
|---|---|---|---|---|
| 4000 | 1.34 | 1.13 | 1.09 | 0.93 |
| 6000 | 1.37 | **1.45** | 1.19 | 1.21 |
| 8000 | 1.42 | **1.47** | 1.24-1.30 | 1.31-1.38 |
| 12000 | 1.43 | **1.52** | 1.25 | 1.25 |
| 16000 | 1.42 | 1.44 | **1.30** | 1.17 (memory-blocked) |
| 20000 | **1.39** | 1.32 | 1.18 | falls back to oz16 |

(The 4000 and 8000 rows come from the interleaved runs in sections 5 and 7 of the log, i.e.
`results/` raw outputs of `fmmtest time`; the 8000 ranges are two separate sessions.)

**Cold first call** (`results/cold.txt`): the first call of the emulation allocates and pre-faults its
workspace. See the file for the measured first-call versus steady-state times; the steady-state numbers
above exclude this one-off cost, as is usual for library buffer pools.

### Speed ceiling of the emulation on this machine

| quantity | value |
|---|---|
| AMX int8 vs FP64 FMA, instruction peak per core | 2048 vs 32 ops/cycle = **64x** |
| achieved: int8 GEMM (7.2-7.6 Tops/s) vs MKL DGEMM (about 280 GFLOP/s) | **about 26x** |
| int8 GEMMs needed for FP64 accuracy | s = 14-16 (each gives about 7.9 bits of the CRT modulus) |
| upper bound on speed-up, GEMMs only | 26/14 = 1.86x (s = 14), 26/16 = 1.63x (s = 16) |
| with one exact Winograd level per modulus | x 8/7 -> 2.1x / 1.86x |
| measured, including conversion and CRT (2000 <= n <= 12000) | 1.2-1.5x (s = 14), 1.1-1.3x (s = 16) |
| if the AMX GEMM reached 60 % of its peak (about 12 Tops/s) | about 3x (s = 14) |

## 8. What is and is not new

Searched by two literature agents and a novelty-check agent (`docs/lit_bilinear.md`,
`docs/lit_emulation.md`, `docs/novelty_check.md`). arXiv and publisher sites were blocked from this
sandbox, so most paper-level facts come from abstracts, snippets and GitHub sources, and are tagged
as such in those files.

* **Ozaki scheme II on AMX-INT8 is not new.**
  * Method: Ozaki, Uchino & Imamura 2025.
  * CPU AMX implementations exist: LIBXS by H. Pabst (open source, LD_PRELOAD DGEMM replacement,
    16 moduli by default), and Kouya 2026 (arXiv:2609.27831, multiple precision on Emerald Rapids,
    compared against MPFR and OpenBLAS-based Ozaki I).
  * What I did not find published is a 53-bit-accurate comparison against **MKL DGEMM** and a careful
    Strassen on the same machine. That measurement, the accuracy study, and the implementation choices
    (exact 32-bit-split residues, built-in inner-dimension scaling) are this project's contribution.
    None is a new algorithm.
* **Inner-dimension (k) power-of-two scaling** is Ballard et al.'s inside scaling for fast matrix
  multiplication (SIMAX 2016). I did not find it used inside Ozaki-type emulation, where published
  codes scale only rows and columns. Transfer of a known idea.
* **Exact modular Strassen inside the emulation.**
  * Strassen-Winograd per prime is standard in computer algebra (FLINT, FFLAS-FFPACK).
  * Complex Karatsuba/3M/2M tricks are already used on int8 residues inside Ozaki II, with the explicit
    remark that they cost no accuracy (Uchino et al. 2025, Caday 2026).
  * Real block Strassen inside Ozaki II was not found. It is a direct transfer; the specialised
    version gains 3-7% at 6000-12000 and loses elsewhere.
* **Fused multi-level Strassen with task-parallel leaves on a vendor BLAS** is engineering on
  Benson & Ballard 2015 and Huang et al. 2017.
* **The cost model** combines measured leaf efficiency (including concurrent single-threaded leaves),
  fused-pass traffic and task makespan, and is validated against 30 measured plans. Similar models
  exist (Huang et al.; Benson & Ballard; D'Alberto & Nicolau). I found none with exactly these
  ingredients, but this is a minor point.
* **No new scheme was found** and none was searched for (section 4 explains why).

## 9. What failed, and why

* **Strassen-type algorithms at n <= 6000 on 4 cores.** MKL is about 88% efficient at n >= 4000 and
  about 70% at 1000-3000. A level of recursion moves the work into the less efficient range and loses
  more than the 12.5% it saves. Task-parallel leaves fix part of this, which gives 1.1-1.3x at
  n = 2000-3000, but only where MKL itself is weak.
* **Lower-exponent schemes.**
  * One level of <4,4,4;48> saves 2% of the multiplications compared with two Winograd levels.
  * <3,3,6;40> and <5,5,5;93> save 3-4% more, but need 18-30 GB of buffers at the sizes where they
    could pay.
  * Within the noise and the model error, they tie with Winograd.
* **Faster AMX kernel.** The microkernel sustains about 3.3 Tops/s per core when data sit in L2, but
  the full GEMM gets about 1.8. Register blockings (2x2, 1x4), macro-tile sizes, software pipelining
  and prefetch all landed at 6-7.6 Tops/s on 4 cores, the same as oneDNN. With no PMU access in this
  VM I could not find the bottleneck. The emulation's speedup is roughly proportional to this number,
  so it is the main open opportunity.
* **Modular Strassen inside the emulation (partly failed).** The predicted 1/8 cut in AMX work
  happens, and the output is bit-identical. The generic version (`src/ozfmm.c`, any integer scheme,
  interpreted modular additions in int16) adds about 0.3 s at n = 8000 against 0.3 s saved, so it is
  not faster. The specialised Winograd version (`src/ozw.c`: unsigned residues, byte-level modular
  sums, output combination fused into the CRT) is 3-7% faster than the plain emulation at
  6000 <= n <= 12000 with 14 moduli. It loses at n <= 4000, where seven half-size GEMMs per modulus
  are less efficient than one full-size GEMM and the per-call overhead is 7x larger, and when its
  7/4 x larger residue planes force memory blocking (16 moduli at n >= 16000, where it now falls back
  to the plain emulation). The gain is far below the 12.5% of int8 work it removes because the AMX
  GEMM runs at only about 35% of peak, so the half-size products lose some of their efficiency.
* **Floating-point Strassen over emulated leaves.** No gain at n = 8000: the emulation is less
  efficient at n/2, and the accuracy reverts to Strassen's behaviour unless the inputs are scaled.
* **Border-rank schemes.** Rejected by error analysis (Bini-Lotti): an O(eps) truncation plus
  O(u/eps) rounding gives at best about sqrt(u) = 1e-8, which is not double precision.

## 10. What I could not do or verify

* **One machine only**, a 4-vCPU VM with noisy neighbours.
  * Absolute numbers and crossovers will differ on bare metal, on more cores (MKL scales differently),
    and certainly on CPUs without AMX, where the emulation does not pay.
  * Small-n differences under about 10% are within the noise.
* **No hardware performance counters** (no PMU in the guest, no matching `perf`), so the AMX GEMM
  bottleneck is inferred, not measured.
* **Literature:** paper full texts were not readable from the sandbox (arXiv blocked). Claims about
  prior work are from abstracts, snippets and code; for example, whether Dumas, Pernet, Sedoglavic &
  Tichavsky 2026 contains wall-clock timings for <4,4,4;48> is unverified.
* **Accuracy:**
  * The error study is empirical: 6 input classes, n <= 4000 with sampled entries above 1000.
  * The emulation's error bound is analysed (Cauchy-Schwarz range bound, rounding of the scaled
    inputs) but not formally proved here.
  * The accuracy of the emulation on entrywise wide exponent ranges beyond 2^±64 was not tested.
* **Not run:** no flip-graph, ALS or SAT search for new schemes (justified by the model, but still not
  done), and no Karstadt-Schwartz alternative-basis implementation.
* **First call:** the emulation keeps a persistent, pre-faulted workspace of up to 8 GB, as a BLAS
  buffer pool would. The first call pays for allocating and faulting it; that cost is reported
  separately in section 7 and excluded from the steady-state numbers.

## 11. Reproduce; code map

```
./run_all.sh          # build, checks, accuracy study, full speed sweep (several hours: n up to 20000)
./run_all.sh quick    # same with the sweep limited to n <= 4000
bench/checks.sh       # correctness only (~5 min)
```

Requirements:
* gcc >= 13 and Python 3.
* An Intel CPU with AMX-INT8 (Sapphire Rapids or later) and Linux >= 5.16.
* MKL (`pip install mkl mkl-devel`, installs to /usr/local), OpenBLAS and BLIS (`apt install
  libopenblas-openmp-dev libblis-openmp-dev`).
* The Makefile hard-codes those paths.

| path | what |
|---|---|
| `src/common.h`, `src/blas.h` | timing, allocation, BLAS shim |
| `src/sw.c` | memory-lean Strassen-Winograd (Boyer et al. schedule, fused post-additions), optional generic leaves |
| `src/gen.c`, `src/slp.h` | generic bilinear-scheme engine (SLP-driven fused passes, DFS/BFS, peeling) |
| `src/amx.c` | AMX int8 GEMM (s8 x s8 and u8 x u8), 1x4 tile kernel, macro tiling |
| `src/ozaki.c`, `src/oz_internal.h`, `src/oz_consts.h` | FP64 emulation (Ozaki II/CRT): scaling, residues, CRT, blocking, guards |
| `src/ozw.c` | emulation with one exact Strassen-Winograd level per modulus (specialised, unsigned residues) |
| `src/ozfmm.c` | emulation with any integer-coefficient scheme per modulus (generic, slower) |
| `src/fmmtest.c` | driver: `acc`, `time` (interleaved, paired), `bitcmp`, `cold`, `passes` |
| `src/testmat.h` | input classes, double-double reference, error metrics |
| `src/bench_*.c`, `src/stream.c` | DGEMM, AMX, oneDNN, concurrency and bandwidth probes |
| `tools/scheme_check.py`, `tools/import_schemes.py`, `tools/fetch_scheme_sources.sh` | exact verification and import of the scheme library |
| `tools/slp.py` | scheme to straight-line programs (CSE, composition) |
| `tools/gen_oz_consts.py` | exact CRT constants |
| `tools/costmodel.py`, `calibrate.py`, `validate_model.py`, `rank_plans.py` | cost model, calibration, validation, search |
| `tools/acc_table.py`, `tools/sweep_table.py` | result tables |
| `schemes/` | 59 verified schemes (JSON) + `INDEX.md` |
| `results/` | raw outputs and tables (`sweep.md`, `accuracy.md`, `model_validation.md`, `plan_ranking.md`, ...) |
| `docs/` | literature surveys, novelty check, cost model, ideas, independent review |
