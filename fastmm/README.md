# fastmm: looking for a dense double-precision matrix multiply faster than Strassen in practice

Research log and code. One machine: 4 vCPUs of an Intel Xeon 5th-gen ("Emerald Rapids",
family 6 model 207) with AVX-512 and **AMX-INT8**, 15 GB RAM, KVM guest. All numbers below were
measured there with 4 threads unless stated.

## Verdict

**Goal:** a general dense algorithm, accurate in double precision, measurably faster than both a tuned
BLAS (MKL here; OpenBLAS and BLIS are slower on this machine) and a careful Strassen-Winograd, for
n = 500-20000.

**Result: a partial success, reached by changing the kind of algorithm, and the method is not new.**

* **Strassen-type algorithms.** No bilinear (Strassen-type) algorithm I built or found beats a
  careful Strassen-Winograd by more than the measurement noise.
* **What beats both.** FP64 GEMM emulated with exact int8 products on the CPU's AMX matrix units
  (Ozaki scheme II with the Chinese remainder theorem) beats both MKL and the best Strassen-type
  plan from n = 1500 to 20000.
* **Not new.** The method is published, and open-source CPU implementations exist. This is an
  independent implementation and measurement.
* **Limits.**
  * It does not help at n <= 1000.
  * It needs AMX-class int8 hardware.
  * Matching DGEMM's accuracy takes 16 moduli, and even then not on every input.

Measured on an Intel Xeon (Emerald Rapids), 4 cores, MKL 2026.1, 4 threads. Each entry is the
paired speedup over MKL DGEMM: the median of interleaved rounds, where > 1 means faster than MKL.
Where two numbers are given, they come from two separate sessions: the main sweep and the
final-code runs.

| n | best Strassen-type plan | emulation, 14 moduli | emulation, 16 moduli |
|---|---|---|---|
| 500 | 0.83 | 0.54 | 0.48 |
| 1000 | 0.93 | 0.88 | 0.75 |
| 1500 | 1.00 | 1.04 | 0.97 |
| 2000 | 1.25 | 1.35 | 1.17 |
| 3000 | 1.30 | 1.46 | 1.32 |
| 4000 | 1.04-1.05 | 1.19-1.33 | 1.12-1.21 |
| 6000 | 1.07 | 1.25-1.37 (exact-Winograd variant: 1.45) | 1.09-1.19 |
| 8000 | 1.12 | 1.36-1.38 (1.45) | 1.21-1.27 |
| 10000 | 1.15 | 1.31 | 1.18 |
| 12000 | 1.23 | 1.40-1.43 (1.52) | 1.22-1.25 |
| 16000 | 1.27 | 1.33-1.42 | 1.16-1.30 |
| 20000 | 1.23 | 1.39-1.41 | 1.18-1.25 |

What these numbers mean against the goal:

1. **Speed, 14 moduli.**
   * Faster than MKL and than the best Strassen-type plan at every measured size from 1500 to
     20000.
   * The margin over the Strassen-type plan is 8-28% for 2000 <= n <= 12000 and 5-15% at
     16000-20000. At 1500 it is 4%, which is within noise.
   * Accuracy:
     * About DGEMM's on well-scaled and diagonally scaled data.
     * 5-100x worse than DGEMM, componentwise, on entries with random exponents in 2^±20 to
       2^±32, and up to 10^5 x worse at 2^±64.
     * On every tested input class it is as accurate as or more accurate than the two-level
       Strassen-type plans it is compared with, and close to one Strassen-Winograd level.
2. **Speed, 16 moduli.** This is the variant that is DGEMM-accurate.
   * Faster than MKL for n >= 2000.
   * Faster than the best Strassen-type plan by 8-15% only at n = 4000 and 8000.
   * At 3000, 10000, 12000 and 20000 it is within ±5% of that plan (a tie).
   * At 6000 it is 2-11% faster and at 16000 9% slower to 2% faster, depending on the session.
   * At 2000 it is slower (1.17 vs 1.25).
3. **Accuracy, 16 moduli** (section 6).
   * Componentwise error at or below MKL's on every class tested: uniform, positive, cancellation,
     rows or columns scaled by 2^±32, inner dimension scaled by 2^±20, and random exponents up
     to 2^±32.
   * The exception is entries with independently random exponents of 2^±48 or wider at small n:
     28x worse than DGEMM at 2^±48 and 1000x worse at 2^±64, both at n = 1000.
   * The Strassen-type plans are 2-20x worse than DGEMM on well-scaled data, up to 10^5-10^6 x
     worse at 2^±64 (n = 1000), and completely wrong on row- or column-scaled data unless the
     inputs are rescaled.
4. **Below n = 1500, nothing beats MKL** on this machine: neither Strassen-type plans nor the
   emulation.
5. **The first call is slow.** The emulation allocates and pre-faults a persistent workspace of up
   to 8 GB. At n = 8000 the first call takes 3.6-5.7 s, against 2.3-2.9 s afterwards and 3.6 s for
   MKL. A single isolated product is therefore not faster; repeated products are.
6. **It is hardware-specific.** The gain exists only because int8 matrix units are about 26x faster
   per operation than FP64 FMA here. On a CPU without AMX (AVX512-VNNI: about 8x) the emulation
   loses.
7. **Strassen applied exactly inside the modular int8 products** gives bit-identical results to the
   plain emulation. The specialised version adds 3-7% at 6000 <= n <= 12000 with 14 moduli and
   loses at n <= 4000.
8. **The existing open-source AMX Ozaki code (LIBXS)**, as I configured it, was 3-6x slower than MKL
   on this machine. It loses about 10 digits on inputs whose inner dimension is badly scaled
   (`results/libxs_compare.md`). The speed and the robustness reported here come from engineering
   (int8 kernel, conversion, inner scaling), not from a new method.

It is not a new matrix multiplication algorithm, and it does not lower the exponent. Everything was
measured on one 4-core VM. Full tables, method and caveats follow below. Everything can be re-run
with `./run_all.sh`.

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
  * Error is computed on **every entry** of C. An earlier version sampled 20000 entries for n > 1000;
    the independent review showed that this understated the emulation's heavy-tailed maximum error
    much more than DGEMM's, so all tables now use all entries.
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
| 1 | FP64 emulation on AMX-INT8 (Ozaki II) | FP64 FMA throughput itself | int8:FP64 throughput ratio < ~16 | ratio about 26; **1.2-1.5x over MKL** for 2000-20000 (14 moduli) |
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
   dimension a balancing 2^e_k, with A' = A·2^e and B' = 2^-e·B.
   * e_k balances max|A_:,k| against max|B_k,:|. It is clamped so that no nonzero entry leaves the
     normal range, so the scaling is exact.
   * Norms are computed on exponents, so nothing in them can overflow or underflow.
   * The scaled entries are rounded to integers with ||row||_2, ||col||_2 < 2^L.
   * By Cauchy-Schwarz every entry of the integer product is below P/4, where P is the product of the
     moduli. P/2 would suffice; the extra factor of 2 is a safety margin that costs one bit of L at
     s = 15 and 16.
2. **Residues.** For s pairwise coprime moduli <= 256, the residues are int8. They are computed
   exactly from a 32-bit split of each integer, with one FMA-based floor per modulus, and written
   straight into AMX tile layout with non-temporal stores.
3. **Products.** One AMX int8 GEMM per modulus (`src/amx.c`: 1x4 tile kernel, 128x256 macro tiles,
   K chunks of 512). The int32 accumulation is exact for k <= 131071; longer k is split. The epilogue
   reduces mod p to bytes.
4. **CRT.** Reconstruction in double with a three-way split of the CRT weights. The constants are
   generated and checked with exact integers. The high and middle sums are exact, then the result is
   rescaled with `scalef`.
5. **Guards.** Memory blocking keeps the workspace under 80% of the memory actually available, the
   smaller of the kernel's MemAvailable and the cgroup limit (n = 20000 fits in 15 GB). A
   persistent pre-faulted workspace is used. NaN/Inf inputs fall back to MKL.
6. **Reproducibility.** The result is bitwise identical from run to run and for any number of threads.
   Every reduction runs in a fixed order, and the int32 accumulation is exact. This is checked by
   `fmmtest edge`.

* The errors are:
  * the rounding of the scaled inputs to L-bit integers (the dominant term);
  * the final rounding to double;
  * a term of at most about 1.7e-25·P in the low part of the CRT reconstruction (negligible);
  * for k > 131071, one rounding per k-chunk when the chunk results are added.
* L is about 54 bits for 14 moduli, 57 for 15, and 61 for 16, at k = 8192.
* Throughput: the AMX GEMM reaches 7.2-7.6 Tops/s on 4 cores, against oneDNN 3.9.2's 7.0 Tops/s and
  MKL's `cblas_gemm_s8u8s32` at 5.2 Tops/s on this machine. That is about 26x the DGEMM rate but only
  about 35% of the AMX peak.
* Time split at n = 8000 with 14 moduli: conversion 0.25 s, int8 GEMMs 2.2-2.6 s, CRT 0.06-0.08 s, scaling
  0.02 s.

### What the error bound is, and the certified mode (`ozc16`)

**The error bound is normwise per row and column, not componentwise.** The emulation's error in c_ij
is at most about 2^-L (||a_i||_1 ||b_j||_2 + ||a_i||_2 ||b_j||_1) / 2 (a_i: row i of A after the inner
scaling, b_j: column j of B). DGEMM's error is bounded by k·u·sum_k |a_ik||b_kj|.

* When the large entries of a_i meet large entries of b_j, the two bounds are similar and 16 moduli
  (L about 61) beat DGEMM.
* When large entries of a_i meet only small entries of b_j, sum_k |a_ik||b_kj| can be far below
  ||a_i|| ||b_j||. The emulation's componentwise error then grows without limit, while DGEMM's does
  not. A 2x2 example with entries 2^g and 1/3 gives an oz16 error of 4e-14 at g = 16 and 6e-10 at
  g = 32; DGEMM is exact.
* No diagonal (row, column or inner) scaling removes this, because it is a property of the
  individual entries. This weakness of fixed-precision Ozaki-type emulation is known: Abdelfattah,
  Dongarra, Fasi, Mikaitis & Tisseur 2025, and the LIBXS documentation; see
  `docs/novelty_certificate.md`.
* Section 6 measures it with two input classes:
  * `checker`: exponents alternate in a checkerboard;
  * `decay`: entries decay away from the diagonal, as in kernel or covariance matrices.

**Certified mode** (`oz_set_certify(theta)`, method `ozcS`; the default theta = 4 gives a 5u bound).
Every entry of C is either certified or recomputed:

1. **Rigorous error bound.** The packers also compute the 1-norms of the rounded scaled integer rows
   and columns, n1A_i and n1B_j. The emulation error of c_ij is then at most
   (n1A_i/2 + n1B_j/2 + k/4 + 2^(2L-80)) · 2^-(sigma_i + tau_j), a rigorous bound.
2. **Rigorous lower bound of sum_k |a_ik||b_kj|.**
   * |A| and |B| are quantised by floor to 7 bits, with each row or column scaled by its own power
     of two.
   * One extra int8 AMX GEMM gives the lower bound for every entry; it is 1/17 of the GEMM work.
3. **Certificate.** An entry passes when its bound is at most theta·u times the lower bound. Then
   |c^ - c| <= (theta + 1)·u·sum_k |a_ik||b_kj|, which is **5u|A||B| for theta = 4**. That is
   below DGEMM's own worst-case bound (k·u|A||B|) and close to DGEMM's typical error.
4. **Repair of the entries that fail.** Failing entries are recomputed with a compensated dot
   product (Dot2, Ogita-Rump-Oishi 2005; error <= u|c| + O(k^2 u^2)|A||B|).
   * A 256x256 tile with more than 256 failing entries is recomputed by MKL DGEMM instead.
   * The whole block goes to DGEMM when more than 1/8 of its tiles need DGEMM.
5. **Guarantee.** Every entry is certified to 5u|A||B|, computed by Dot2, or computed by MKL's DGEMM.
   **No input can make it less accurate than DGEMM.**

What it costs:
* About 3% when everything certifies (uniform data at n = 8000: 1.18x MKL against 1.21x for oz16).
* On inputs that fail everywhere, the emulation's conversion and the extra GEMM are wasted and the
  call costs about 1.25x a DGEMM.
* The bound is a worst case. It does not pass for some inputs where the actual emulation error is
  fine, for example decay with r = 8, where oz16 is 10x below DGEMM but some tiles are recomputed.
* It needs 16 moduli. With 14 or 15 the bound (about 2^-54 to 2^-57) is above u·|A||B| for typical
  data, so almost nothing would certify.

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

**Final emulation code** (`results/sweep_oz_final.md`, scripts `bench/sweep_final_small.sh` and
`bench/sweep_final_oz.sh`, run after the main sweep). What changed since the sweep:
* memory blocking that converts each operand once where it fits and respects the container's cgroup
  memory limit;
* the NaN/Inf and long-k guards;
* the exact-Winograd variant `ozw`.

At n = 4000-12000 all variants are interleaved in one session. At n = 4000 and 8000 that session
also includes the Strassen-type plans. At 16000 and 20000 each variant is paired with MKL in its own
session.

| n | oz14 (plain) | ozw14 (exact Winograd) | oz16 (plain) | ozw16 (exact Winograd) | sw1 | W2 task-par. | P48 task-par. |
|---|---|---|---|---|---|---|---|
| 4000 | **1.33** | 1.17 | 1.21 | 1.03 | 0.83 | 1.05 | 1.02 |
| 6000 | 1.37 | **1.45** | 1.19 | 1.21 | | | |
| 8000 | 1.36 | **1.45** | 1.27 | 1.31 | 1.07 | 1.11 | 1.12 |
| 12000 | 1.43 | **1.52** | 1.25 | 1.25 | | | |
| 16000 | 1.42 | **1.44** | 1.30 | 1.33 (memory-blocked) | | | |
| 20000 | **1.39** | 1.32 | 1.18 | 1.17 (falls back to oz16) | | | |

Time split of the emulation, in seconds: conversion / int8 GEMMs / CRT.

| n | oz14 | ozw14 | oz16 |
|---|---|---|---|
| 8000 | 0.25 / 2.20 / 0.06 | 0.30 / 2.02 / 0.14 | 0.25 / 2.49 / 0.07 |
| 12000 | 0.63 / 7.65 / 0.18 | 0.67 / 6.70 / 0.36 | 0.60 / 8.97 / 0.17 |
| 20000 | 3.21 / 37.62 / 0.46 | 6.68 / 35.99 / 1.12 | 3.62 / 45.29 / 0.51 |

The int8 GEMMs take 85-91% of the time.
* At n = 6000-12000 the exact Winograd level removes 8-12% of the GEMM time.
* At 16000-20000, where the larger residue planes force memory blocking, it removes 4% or nothing.
* It always doubles the CRT and output-combination time.

**Cold first call** (`results/cold.txt`, `bench/cold.sh`): a fresh process, then the first call and
the next three, in seconds.

| n | MKL first / then | oz14 | oz16 | ozw14 | sw1 |
|---|---|---|---|---|---|
| 2000 | 0.071 / 0.065-0.075 | 0.073 / 0.055-0.057 | 0.078 / 0.059-0.062 | 0.082 / 0.070-0.081 | 0.067 / 0.063-0.079 |
| 8000 | 3.64 / 3.57-3.63 | 5.68 / 2.55-2.62 | 3.61 / 2.76-2.98 | 4.37 / 2.24-2.34 | 3.32 / 3.33-3.41 |

The first call allocates the workspace (up to 8 GB) and faults its pages in. At n = 8000 this
makes the first call as slow as MKL (oz16) or slower (oz14: 5.7 s against 3.6 s). From the second
call on, the steady-state numbers in the tables above apply. A library would keep this workspace
as a buffer pool, so the cost is paid once per process, but a program that does one large product
and exits sees no gain.

### Speed ceiling of the emulation on this machine

| quantity | value |
|---|---|
| AMX int8 vs FP64 FMA, instruction peak per core | 2048 vs 32 ops/cycle = **64x** |
| achieved: int8 GEMM (7.2-7.6 Tops/s) vs MKL DGEMM (about 280 GFLOP/s) | **about 26x** |
| int8 GEMMs needed for FP64 accuracy | s = 14-16 (each gives about 7.9 bits of the CRT modulus) |
| upper bound on speed-up, GEMMs only | 26/14 = 1.86x (s = 14), 26/16 = 1.63x (s = 16) |
| with one exact Winograd level per modulus | x 8/7 -> 2.1x / 1.86x |
| measured, including conversion and CRT (2000 <= n <= 20000) | 1.2-1.5x (s = 14), 1.1-1.3x (s = 16) |
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
* **The certified mode** (per-entry rigorous certificate plus selective repair). Its components are
  known:
  * an extra int8 GEMM of |A| and |B| is used by GEMMul8's "accurate mode" (Uchino et al. 2025),
    but as an *upper* bound and only to choose the scaling;
  * Rump-style verified products bound errors through |A||B|;
  * Dot2 is Ogita-Rump-Oishi 2005;
  * libraries that adapt to the input (cuBLAS's FP64 emulation, ozIMMU) decide once per call and
    fall back to FP64 for the whole call.

  What I did not find is a per-entry certificate (floor-quantised lower bound against the
  emulation's rigorous error bound) with repair of only the failing entries or tiles. The search
  could not read the papers themselves; Ozaki, Ogita & Oishi (NLAA 2016) on a posteriori validation
  of Ozaki scheme I is the closest candidate and should be checked first. So this is at most a new
  combination of known pieces (`docs/novelty_certificate.md`).
* **The accuracy weakness** of the emulation (componentwise failure when large entries meet small
  ones) is already published; my `checker` and `decay` classes are further instances of it.
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
  are less efficient than one full-size GEMM and the per-call overhead is 7x larger. It gains nothing
  when its 7/4 x larger residue planes force memory blocking: at n = 16000 with 16 moduli it ties the
  plain emulation (1.33 vs 1.30), and at 20000 it falls back to it. The gain is far below the 12.5% of int8 work it removes because the AMX
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
  * The error study is empirical: 6 input classes at n <= 4000, and 2 classes at n = 8000, with every
    entry checked. Nothing above n = 8000 was checked.
  * The emulation's error bound is analysed (Cauchy-Schwarz range bound, rounding of the scaled
    inputs) but not formally proved here.
  * The accuracy of the emulation on entrywise wide exponent ranges beyond 2^±64 was not tested.
* **Not run:** no flip-graph, ALS or SAT search for new schemes (justified by the model, but still not
  done), and no Karstadt-Schwartz alternative-basis implementation.
* **First call:** the emulation keeps a persistent, pre-faulted workspace of up to 8 GB, as a BLAS
  buffer pool would. The first call pays for allocating and faulting it.
  * That cost is reported separately in section 7 and excluded from the steady-state numbers.
  * The cold timings are single samples per method; the review's repeat runs varied by 2x.
* **Free memory:** the speed-ups assume that the workspace fits, which takes about 3-8 GB at
  n >= 8000. With less free memory the blocking shrinks and the gain disappears. At n = 8000 the
  review measured oz14 at 1.22x MKL with a 1.5 GB budget and 0.6-0.7x with 0.25-0.6 GB.
* **Not production code:**
  * The emulation keeps a process-wide workspace and flag, so it must not be called from several
    threads at once.
  * There is no transposed-operand or alpha/beta interface: it computes C = A·B, column-major, with
    leading dimensions.

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
