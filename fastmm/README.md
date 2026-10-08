# fastmm: looking for a dense double-precision matrix multiply faster than Strassen in practice

Research log and code.
* **Measurements:** all speed numbers come from 4 vCPUs of an Intel Xeon 5th-gen ("Emerald Rapids",
  family 6 model 207) with AVX-512 and **AMX-INT8**, 15 GB RAM, KVM guest, 4 threads.
* **Second machine:** a second CPU without AMX was used only for checks, the n = 8000 accuracy rows
  and one int8 measurement (section 1).

## Verdict

**Goal.** A general dense algorithm, accurate in double precision, measurably faster than both a tuned
BLAS (MKL here; OpenBLAS and BLIS are slower on this machine) and a careful Strassen-Winograd, for
n = 500-20000.

**Result: partial. The goal is not met in full.**

1. **No Strassen-type (bilinear) algorithm beats a careful Strassen-Winograd** by more than the
   measurement noise. This covers 59 verified schemes, a validated cost model, fused multi-level and
   task-parallel implementations, and the lower-exponent base cases (<4,4,4;48>, <3,3,6;40>,
   <5,5,5;93>).
2. **What does beat both is a different kind of algorithm**, FP64 GEMM emulated with exact int8
   products on the CPU's AMX matrix units (Ozaki scheme II with the Chinese remainder theorem).
   * It is a published method, and open-source CPU implementations exist. This work is an
     independent implementation, measured against MKL and Strassen.
   * It only pays on CPUs with AMX.
   * **It has a speed/accuracy trade-off that decides the answer:**
     * **14 moduli** is clearly faster than both, 1.2-1.5x MKL at n >= 2000. Its accuracy is only
       normwise per row and column: much worse than DGEMM, entry by entry, on inputs with wide
       exponent ranges.
     * **16 moduli** is DGEMM-accurate or better on well-scaled and diagonally scaled data, at
       1.1-1.3x MKL. That roughly ties the best Strassen-type plan. On inputs where large entries
       meet small ones (random exponents of 2^±48 or wider, checkerboard or diagonal-decay
       structure) it is 1.4 to 10^7 times less accurate than DGEMM.
     * **16 moduli, certified** (new mode, section 5) attaches a rigorous per-entry error bound and
       recomputes every entry it cannot certify with a compensated dot product or DGEMM. **It is
       never less accurate than DGEMM on any input.** On inputs that certify it runs at
       1.01-1.18x MKL: a tie with the best Strassen-type plan at n = 4000-8000, and a loss at
       n = 2000. On inputs that do not certify it runs at about 0.8x MKL.
3. **Below n = 1500 nothing beats MKL** on this machine.

**Measured paired speedups over MKL DGEMM**: AMX machine, 4 threads, median of interleaved rounds.
Ranges cover separate sessions and code versions (section 7). "-" means not measured.

| n | best Strassen-type plan | emulation, 14 moduli | 16 moduli | 16 moduli, certified |
|---|---|---|---|---|
| 500 | 0.83 | 0.54 | 0.48 | - |
| 1000 | 0.93 | 0.88 | 0.75 | - |
| 1500 | 1.00 | 1.04 | 0.97 | - |
| 2000 | 1.25 | 1.35 | 1.17 | 1.05 |
| 3000 | 1.30 | 1.46 | 1.32 | - |
| 4000 | 1.04-1.05 | 1.19-1.33 | 1.10-1.21 | 1.01-1.07 |
| 6000 | 1.07 | 1.25-1.37 | 1.09-1.19 | - |
| 8000 | 1.11-1.12 | 1.36-1.38 | 1.21-1.27 | 1.15-1.18 |
| 10000 | 1.15 | 1.31 | 1.18 | - |
| 12000 | 1.23 | 1.40-1.43 | 1.22-1.25 | - |
| 16000 | 1.27 | 1.33-1.42 | 1.16-1.30 | - |
| 20000 | 1.23 | 1.39-1.41 | 1.18-1.25 | - |

The exact-Strassen-Winograd variant of the emulation (bit-identical results) adds 3-7% at
6000-12000 with 14 moduli: 1.45 at 6000 and 8000, 1.52 at 12000.

**Accuracy relative to DGEMM** (section 6). Each entry is the max componentwise error
|C - C*| / (|A||B|), divided by DGEMM's, over n = 1000, 2000 and 4000 with every entry checked. A
value of 1 means DGEMM's accuracy.

| input | Strassen-Winograd, 1 level / 2 levels | emulation, 14 moduli | 16 moduli | 16 moduli, certified |
|---|---|---|---|---|
| well-scaled (uniform, positive, cancellation) | 0.8-4 / 0.8-15 | 0.2-3 | 0.001-0.1 | 0.001-0.1 |
| rows, columns or inner dimension scaled by up to 2^±32 | 10^5-10^38 (wrong); 2-3 with rescaling | 1-3 | 0.07-0.09 | 0.07-0.09 |
| random exponents 2^±10 | 3-4 / 8-11 | 1.6-2.6 | 0.05-0.07 | 0.05-0.07 |
| random exponents 2^±20 to 2^±32 | 6-300 / 15-700 | 4-100 | 0.05-1 | 0.05-1 |
| random exponents 2^±48 / 2^±64 | 10^2-10^4 / 10^3-10^6 | 10^2-10^4 / 10^3-10^5 | 1.4-28 / 6-1100 | 1 |
| checkerboard exponents 2^8 / 2^16 / 2^32 | 2-11 | 10^2 / 10^4 / 10^9 | 0.6-1.5 / 140-380 / 10^7 | 1 |
| decay away from the diagonal to 2^-8 / 2^-16 / 2^-32 | 10-100 / 10^3 / 10^7-10^8 | 5-14 / 10^3 / 10^7 | 0.04-0.1 / 5-11 / 10^5 | 0.3 / 1 / 1 |

**Plainly:** a general, double-precision-safe algorithm that is clearly faster than both MKL and
Strassen-Winograd was **not** found. The certified emulation is safe and at most about 15-18%
faster than MKL, but not faster than the best Strassen-type plan. The 14-modulus emulation is up to
1.5x faster than MKL, but it is not DGEMM-accurate on all inputs. Neither is Strassen.

**What else to know:**
* **The first call is slow.** It allocates and pre-faults a workspace of up to 8 GB; at n = 8000 that
  call takes 3.6-5.7 s against 2.3-2.9 s afterwards.
* **It needs free memory**, about 3-8 GB of workspace at n >= 8000.
* **The machine changed during the work.** Near the end, the container moved to a CPU without AMX
  (Cascade Lake). There the emulation runs through a portable exact int8 kernel: same results, bit
  for bit, but slow. MKL's int8 GEMM reaches only 1.2-3.2x its DGEMM rate there, so the emulation
  cannot win on such CPUs.
* **One consequence:** the final single-code-version speed sweep could not be run (section 10).
* **Review.** Two independent reviews tried to break the results (`docs/independent_review*.md`).
  The first found real problems, which were fixed or are reported here.
* **Reproduce.** `./run_all.sh` reruns all checks and benchmarks.

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

Two machines were used. Everything up to the last stage ran on the first. Near the end the container
was restarted on the second (`results/machine2/`).

* **AMX machine** (all speed results; accuracy for n <= 4000):
  Intel Xeon 5th generation, family 6 model 207 (Emerald Rapids), 4 vCPUs (KVM), 1 thread per core.
  * AVX-512, AVX512-VNNI/BF16/FP16, AMX-INT8/BF16.
  * L2 2 MB per core. The guest reports a 260 MB L3, but streaming measurements suggest the effective
    share is far smaller.
  * DRAM bandwidth 25-35 GB/s (STREAM-like `src/stream.c`).
  * Cores run at about 2.5 GHz under load. Measured AMX peak is 5.1-5.6 Tera int8-ops/s per core, and
    it does not drop with all 4 cores busy.
* **Second machine** (no AMX):
  * Intel Xeon family 6 model 85 stepping 7 (Cascade Lake), 4 vCPUs at 2.8 GHz, AVX-512 with VNNI,
    no AMX, no VBMI. Same OS image and MKL.
  * The emulation runs here through a portable exact int8 kernel (`src/amx.c`,
    `amx_gemm_portable`: int16 multiply-add with int32 accumulation, the same wrap-around as AMX).
    Its results were verified to match the AMX machine's digit for digit on every statistic tested.
  * Used for: the checks without AMX (`results/machine2/checks.txt`), the n = 8000 accuracy rows,
    and the int8-vs-FP64 measurement in section 9.
  * Timings on this VM were too noisy for speed comparisons: MKL DGEMM varied between 116 and
    201 GFLOP/s at n = 8000.
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
| 11 | Certified emulation: rigorous per-entry error bound, lower bound of abs(A)·abs(B) from one extra int8 GEMM, repair of uncertified entries | the emulation's componentwise failures (found after the first review) | overhead > 10%, or any entry worse than DGEMM | overhead about 3%; never worse than DGEMM; but conservative, and with 16 moduli only 1.0-1.18x MKL |

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

Full tables: `results/accuracy.md` (max componentwise, median relative and normwise errors for every
method; raw data `results/accuracy.txt`; script `bench/accuracy.sh`; summary table below made by
`tools/acc_summary.py`).

**Method.**
* Every entry of C is compared with a double-double reference (error about k·u²·|A||B|).
* Sizes n = 1000, 2000 and 4000, with all methods and all input classes; n = 8000 for a subset.
* The metric is the max componentwise error |C - C*| / (|A||B|), the quantity DGEMM bounds by about
  k·u.
* An earlier version sampled 20000 entries for n > 1000. The first independent review showed that
  sampling understated the emulation's heavy-tailed maximum error (4-13x) far more than DGEMM's
  (1.4-1.8x), so every number now uses all entries.

**Input classes:**
* uniform [-1,1]; positive [0,1]; heavy cancellation (|C| about 2^-30 |A||B|);
* rows and columns scaled by 2^±r, or the inner dimension scaled by 2^±r (diagonal scalings);
* independent random exponents 2^±r per entry;
* checker: exponents 0 and r alternating in a checkerboard, so large entries meet small ones and no
  diagonal scaling can remove it;
* decay: entries decaying away from the diagonal to 2^-r, as in kernel, covariance or
  inverse-operator matrices.

**Columns:**
* sw1, sw2: 1 and 2 memory-lean Winograd levels;
* sc:sw1: sw1 with outside-inside power-of-two scaling;
* P48: one task-parallel <4,4,4;48> level;
* oz14, oz16: the emulation with 14 or 16 moduli;
* ozc16: certified mode.

Bold marks errors more than 10x DGEMM's.

| input (n = 4000) | r | DGEMM | sw1 | sw2 | sc:sw1 | P48 | oz14 | oz16 | ozc16 | ozc16 did |
|---|---|---|---|---|---|---|---|---|---|---|
| uniform [-1,1] | 0 | 9.9e-17 | 3.0e-16 | 9.7e-16 | 3.0e-16 | **1.0e-15** | 3.0e-16 | 7.4e-18 | 7.4e-18 | all certified |
| positive [0,1] | 0 | 1.0e-15 | 1.1e-15 | 9.8e-16 | 1.1e-15 | 6.9e-15 | 3.5e-16 | 1.1e-16 | 1.1e-16 | all certified |
| cancellation | 0 | 1.1e-16 | 5.0e-16 | **1.5e-15** | 5.0e-16 | 7.6e-16 | 2.1e-16 | 4.8e-19 | 4.8e-19 | all certified |
| rows/cols scaled 2^±r | 10 | 9.9e-17 | **6.4e-05** | **1.4e-04** | 3.0e-16 | **1.8e-04** | 3.0e-16 | 7.4e-18 | 7.4e-18 | all certified |
| rows/cols scaled 2^±r | 32 | 9.9e-17 | **1.9e+19** | **3.0e+22** | 3.0e-16 | **1.2e+22** | 3.0e-16 | 7.4e-18 | 7.4e-18 | all certified |
| inner dim. scaled 2^±r | 10 | 9.9e-17 | **1.6e-11** | **5.2e-11** | 3.0e-16 | **5.3e-11** | 3.0e-16 | 7.4e-18 | 7.4e-18 | all certified |
| inner dim. scaled 2^±r | 20 | 9.9e-17 | **1.2e-05** | **3.2e-05** | 3.0e-16 | **3.6e-05** | 3.0e-16 | 7.4e-18 | 7.4e-18 | all certified |
| inner dim. scaled 2^±r | 32 | 9.9e-17 | **1.1e+02** | **4.0e+02** | 3.0e-16 | **6.0e+02** | 3.0e-16 | 7.4e-18 | 7.4e-18 | all certified |
| random exponents 2^±r | 10 | 9.0e-16 | 2.7e-15 | 7.7e-15 | 2.7e-15 | 7.2e-15 | 2.4e-15 | 6.2e-17 | 6.2e-17 | all certified |
| random exponents 2^±r | 20 | 1.8e-15 | 1.0e-14 | **2.9e-14** | 1.0e-14 | **2.9e-14** | 7.8e-15 | 9.5e-17 | 9.5e-17 | 0.1% recomputed |
| random exponents 2^±r | 32 | 1.9e-15 | **3.3e-14** | **1.2e-13** | **4.4e-14** | **1.2e-13** | **3.5e-14** | 3.0e-16 | 1.9e-15 | DGEMM fallback |
| random exponents 2^±r | 48 | 1.7e-15 | **2.0e-13** | **8.4e-13** | **2.0e-13** | **5.5e-13** | **2.1e-13** | 2.3e-15 | 1.7e-15 | DGEMM fallback |
| random exponents 2^±r | 64 | 1.6e-15 | **1.2e-12** | **6.3e-12** | **1.2e-12** | **4.2e-12** | **1.2e-12** | 9.1e-15 | 1.6e-15 | DGEMM fallback |
| checker, spread 2^r | 8 | 1.4e-16 | 3.9e-16 | 1.2e-15 | 3.9e-16 | **1.5e-15** | **2.9e-14** | 2.1e-16 | 1.4e-16 | DGEMM fallback |
| checker, spread 2^r | 16 | 1.4e-16 | 4.3e-16 | 1.2e-15 | 4.3e-16 | 1.1e-15 | **6.2e-12** | **5.4e-14** | 1.4e-16 | DGEMM fallback |
| checker, spread 2^r | 32 | 9.9e-17 | 3.0e-16 | 8.5e-16 | 3.0e-16 | **1.0e-15** | **4.6e-07** | **3.4e-09** | 9.9e-17 | DGEMM fallback |
| decay to 2^-r | 8 | 2.9e-16 | **6.7e-15** | **2.7e-14** | **6.7e-15** | **1.3e-14** | **4.1e-15** | 3.2e-17 | 1.0e-16 | 4.6% recomputed (20 DGEMM tiles) |
| decay to 2^-r | 16 | 4.5e-16 | **7.8e-13** | **2.5e-12** | **7.8e-13** | **1.7e-12** | **6.5e-13** | **5.1e-15** | 4.5e-16 | DGEMM fallback |
| decay to 2^-r | 32 | 7.8e-16 | **2.7e-08** | **7.4e-08** | **2.7e-08** | **7.3e-08** | **1.4e-08** | **1.1e-10** | 7.8e-16 | DGEMM fallback |

**What the table says:**
* **DGEMM** is the only method that is accurate on every class.
* **Strassen-type methods.**
  * Well-scaled data: 2-15x DGEMM's error.
  * Diagonally scaled data: wrong, with errors of 10^5 to 10^38 times DGEMM's (rows or columns
    scaled by 2^±32 give errors of order 10^19-10^22). The outside-inside scaling (sc:) fully
    repairs this.
  * Random exponents: 3-10^6x, depending on the range.
  * Decay: 10^3-10^8x at r >= 16. Rescaling does not help either case.
  * Checker: harmless, 2-11x.
* **Emulation, 16 moduli.**
  * At least as accurate as DGEMM, usually 10-800x more accurate, on well-scaled and diagonally
    scaled data and on random exponents up to 2^±32.
  * Its error bound is normwise per row and column (section 5), so it fails where large entries
    meet small ones:
    * random exponents 2^±48: 1.4-28x DGEMM's error;
    * random exponents 2^±64: 6-1100x;
    * checker 2^16: 140-380x; checker 2^32: 10^7x;
    * decay 2^-16: 5-11x; decay 2^-32: 10^5x.
  * The error shrinks with n on random exponents: at 2^±64 it is 1100x at n = 1000, 61x at 2000
    and 6x at 4000.
* **Emulation, 14 moduli.**
  * About DGEMM's error (0.2-3x) on well-scaled and diagonally scaled data.
  * 2-100x worse on random exponents up to 2^±32, and up to 10^9x worse on the hard classes.
  * On the checker class it is far worse than Strassen.
* **Certified mode.**
  * Never worse than DGEMM: max ratio 1.0 over all 57 class/size combinations.
  * Where everything certifies, it is the 16-modulus result.
  * It certifies everything on uniform, positive, cancellation, diagonally scaled and 2^±10 random
    exponent inputs, and part of the entries for 2^±20 random exponents and decay 2^-8. Elsewhere it
    falls back to DGEMM.
  * The certificate is conservative. On checker 2^8 the plain 16-modulus result is already as
    accurate as DGEMM, but it cannot be certified.
* **Exact modular Winograd inside the emulation** (ozw, ozf in `accuracy.md`) is bit-identical to the
  plain emulation in every case. This is also checked by `bench/checks.sh`.
* **NaN and Inf** (`bin/fmmtest nancheck`, a single NaN or Inf in A).
  * The emulation detects them and calls DGEMM, so the NaN/Inf pattern of C matches DGEMM's.
  * One Strassen-Winograd level spreads a single NaN to 448 entries that DGEMM leaves finite (853
    for an Inf), because its pre-additions mix rows and columns.
* **Long k.** For k > 131071 the int32 accumulators could overflow, so the inner dimension is split.
  Checked at k = 140000: oz16 error 1.6e-18, DGEMM 9.1e-18.
* **LIBXS**, the open-source AMX Ozaki-II code, on the same inputs: `results/libxs_compare.md`.
  Without inner scaling it loses about 10 digits on inner-scaled inputs (error 3.9e-6).
* **n = 8000** (subset of classes, run on the second machine): `results/accuracy.md`. The emulation
  results there are machine-independent (verified bit-identical, section 1); DGEMM and Strassen use
  that machine's MKL.

## 7. Speed: full tables

All numbers: paired speedup over MKL DGEMM (4 threads) on the AMX machine, median over interleaved
rounds; > 1 means faster than MKL. Raw data: `results/sweep.txt`, `results/sweep_oz_final.txt`,
`results/certified_timing_raw.txt`.

**Provenance.** The speed data come from three sessions on the same AMX machine, with three code
versions:

| session | time (UTC) | what | code | differences from the final code |
|---|---|---|---|---|
| S1, main sweep | Oct 7, 18:44-20:39 | all methods, n = 500-20000 | working tree committed in 30c415a | no memory blocking fixes, old (2-norm) inner scaling, no certified mode |
| S2, final emulation runs | Oct 7, 20:45-22:50 | emulation variants, n = 4000-20000, Strassen plans at 4000 and 8000 | 30c415a / fc5857d | old inner scaling; the n = 16000 row predates two blocking fixes |
| S3, post-review | Oct 8, 00:41-00:50 | oz16 vs certified ozc16 at 2000, 4000, 8000 and 4 input classes at 4000 | 22c1a88 / 16502f1 | final emulation, certified mode during tuning (see below) |

* Between sessions only O(n^2) parts changed: the inner scaling (2-norm to max-abs), blocking
  rules, and guards.
* The plain 16-modulus emulation measured in S3 (1.17 at 2000, 1.10-1.12 at 4000, 1.21-1.23 at
  8000) agrees with S1 and S2 (1.17; 1.12-1.21; 1.21-1.27).
* A final sweep of the final code with one version for everything (`bench/sweep_final.sh`) was
  written but could not be run: the container moved to a CPU without AMX before it started
  (section 10).

**S1, main sweep** (`results/sweep.md`, all methods in one session, `bench/sweep.sh`):

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

**S2, emulation variants** (`results/sweep_oz_final.md`, scripts `bench/sweep_final_small.sh` and
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

**Certified mode (S3)**, `results/certified_timing_raw.txt`, verbatim tool outputs recovered from the
session log, with commands and timestamps:

| n, input | oz16 | ozc16 | ozc16 code state |
|---|---|---|---|
| 2000, uniform | 1.17 | 1.05 | first version, scalar certificate epilogue |
| 4000, uniform | 1.10-1.12 | 1.01 / 1.07 | first version / final |
| 8000, uniform | 1.21-1.23 | 1.15 / 1.18 | first version / vectorised epilogue (final) |
| 4000, decay 2^-8 | 1.13 | 1.03 | some tiles recomputed by DGEMM (intermediate per-tile rule; with the final rule 20 of 256 tiles) |
| 4000, checker 2^16 | 1.14 | 0.80 | nothing certifies: the whole product falls back to DGEMM |
| 4000, decay 2^-16 | 1.10 | 0.63 | an intermediate rule (47% of tiles by DGEMM). The final rule falls back to DGEMM once more than 1/8 of tiles need it; that case was not timed. |

* When every entry certifies, the certificate costs about 3% of the plain emulation's time.
* When nothing certifies, the call costs about 1.25x a DGEMM.

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
* **A DGEMM-accurate emulation that is also clearly faster than Strassen (failed).**
  * The emulation's error is bounded per row and column (about 2^-L ||a_i|| ||b_j||), not per entry.
  * The fast 14-modulus version is therefore not DGEMM-accurate on inputs that mix magnitudes. The
    16-modulus version fails on the harder ones (checker, decay, random exponents of 2^±48 or
    more; section 6).
  * More moduli would push the problem further out but never remove it: each modulus adds about
    8 bits, and the checkerboard class needs 2·log2(spread) extra bits.
  * The certified mode makes the result safe on every input. But its rigorous bound is pessimistic
    (a sum of absolute values where the actual errors partly cancel), so it needs 16 moduli and
    still rejects some inputs whose actual error is fine.
  * Its 3% overhead plus 16 moduli leave it at 1.0-1.18x MKL, a tie with Strassen-type plans.
  * A tighter certificate, for example a statistical bound or a second low-precision product,
    could not be made rigorous in the time available.
* **The emulation on CPUs without AMX (measured).** On the second machine (Cascade Lake, AVX-512
  VNNI) MKL's int8 GEMM (`cblas_gemm_s8u8s32`) ran at 244-375 GOP/s against DGEMM's
  116-201 GFLOP/s, 1.2-3.2x in a noisy VM. The hardware peak ratio there is 8x.
  * With 14-16 int8 GEMMs per product the emulation cannot win on such CPUs
    (`results/machine2/int8_vs_fp64.txt`).
  * On the AMX machine the ratio is about 26x.

## 10. What I could not do or verify

* **One AMX machine**, a 4-vCPU VM with noisy neighbours.
  * Absolute numbers and crossovers will differ on bare metal and on more cores (MKL scales
    differently).
  * Small-n differences under about 10% are within the noise.
* **No final single-version speed sweep.** The container moved to a CPU without AMX before the
  final sweep of the final code (`bench/sweep_final.sh`) could run.
  * The speed numbers therefore come from three sessions with three code versions (section 7). The
    changes between them are O(n^2) parts, and the plain emulation's speed agrees across sessions
    within the noise.
  * The certified mode was timed only at n = 2000, 4000 and 8000 on uniform data, and on 3 other
    input classes at n = 4000. Its timing under the final fallback rule on inputs that partly
    certify was not measured.
  * Its speed at n >= 10000 is not measured. From its constant 3% overhead it should be about 3%
    below the plain 16-modulus numbers, but that is not a measurement.
* **No hardware performance counters** (no PMU in the guest, no matching `perf`), so the AMX GEMM
  bottleneck is inferred, not measured.
* **Literature:** paper full texts were not readable from the sandbox (arXiv blocked). Claims about
  prior work are from abstracts, snippets and code; for example, whether Dumas, Pernet, Sedoglavic &
  Tichavsky 2026 contains wall-clock timings for <4,4,4;48> is unverified.
* **Accuracy:**
  * The error study is empirical: 8 input classes at n <= 4000, and 4 classes at n = 8000 (on the
    second machine), with every entry checked. Nothing above n = 8000 was checked.
  * Real application matrices were not tested; the decay class is a stand-in for kernel and
    covariance matrices.
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
./run_all.sh          # build, checks, accuracy study, speed sweeps, cold start (many hours: n up to 20000)
./run_all.sh quick    # same with the main sweep limited to n <= 4000 and no final sweep
bench/checks.sh       # correctness only (~5-10 min); also runs without AMX (portable kernel)
```

Requirements:
* gcc >= 13 and Python 3; Linux >= 5.16.
* An Intel CPU with AMX-INT8 (Sapphire Rapids or later) for the speed results. Any AVX-512BW CPU
  runs everything else, slowly and with identical results.
* MKL (`pip install mkl mkl-devel`, installs to /usr/local), OpenBLAS and BLIS (`apt install
  libopenblas-openmp-dev libblis-openmp-dev`). The Makefile hard-codes those paths.

Methods in `bin/fmmtest` (`acc`, `time`, `bitcmp`, `cold`, `edge`, `refcheck`, `nancheck` modes):
* `dgemm`: MKL.
* `swD`: D memory-lean Winograd levels.
* `g:<slp>:<depth>:<dfs>:<bfs>`: generic scheme engine.
* `swD+<method>`: lean top levels over another method.
* `sc:<method>`: outside-inside scaling.
* `ozS`: emulation with S moduli.
* `ozwS`: the same with exact Winograd per modulus.
* `ozfS:<slp>`: the same with any integer scheme per modulus.
* `ozcS`: certified (theta from `OZ_CERT_THETA`, default 4).
* `time` mode uses uniform inputs unless `FMM_TIME_TYPE` / `FMM_TIME_R` are set.

| path | what |
|---|---|
| `src/common.h`, `src/blas.h` | timing, allocation, BLAS shim |
| `src/sw.c` | memory-lean Strassen-Winograd (Boyer et al. schedule, fused post-additions), optional generic leaves |
| `src/gen.c`, `src/slp.h` | generic bilinear-scheme engine (SLP-driven fused passes, DFS/BFS, peeling) |
| `src/amx.c` | AMX int8 GEMM (s8 x s8 and u8 x u8), 1x4 tile kernel, macro tiling; portable exact fallback without AMX |
| `src/ozaki.c`, `src/oz_internal.h`, `src/oz_consts.h` | FP64 emulation (Ozaki II/CRT): scaling, residues, CRT, blocking, guards, certified mode |
| `src/ozw.c` | emulation with one exact Strassen-Winograd level per modulus (specialised, unsigned residues) |
| `src/ozfmm.c` | emulation with any integer-coefficient scheme per modulus (generic, slower) |
| `src/fmmtest.c` | driver (see above) |
| `src/testmat.h` | 8 input classes, double-double reference (per entry and blocked for all entries), error metrics |
| `src/bench_*.c`, `src/stream.c` | DGEMM, AMX, oneDNN, MKL int8, concurrency and bandwidth probes |
| `tools/scheme_check.py`, `tools/import_schemes.py`, `tools/fetch_scheme_sources.sh` | exact verification and import of the scheme library |
| `tools/slp.py` | scheme to straight-line programs (CSE, composition) |
| `tools/gen_oz_consts.py` | exact CRT constants |
| `tools/costmodel.py`, `calibrate.py`, `validate_model.py`, `rank_plans.py` | cost model, calibration, validation, search |
| `tools/acc_table.py`, `acc_summary.py`, `sweep_table.py`, `oz_table.py`, `final_table.py` | result tables |
| `bench/` | `checks.sh`, `accuracy.sh`, `sweep.sh`, `sweep_final*.sh`, `cold.sh` |
| `schemes/` | 59 verified schemes (JSON) + `INDEX.md` |
| `results/` | raw outputs and tables; `results/machine2/`: second machine (no AMX) |
| `review/`, `review2/` | test code written by the independent reviewers |
| `docs/` | literature surveys, novelty checks, cost model, ideas, independent reviews |
