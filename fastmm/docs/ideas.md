# Candidate ideas, ranked, with kill tests and outcomes

Written after the baselines and the first Strassen measurements (so the "weakness" column is
based on measurement), and updated with the outcome of each test. Chronology note: ideas 1, 2 and 4
were tested roughly in parallel; the AMX ratio (idea 1) was measured early because it was the
cheapest kill test.

Measured facts that drive the ranking (4 cores, Xeon "Emerald Rapids", MKL 2026.1):
* MKL DGEMM: 61-74 GFLOP/s on one core, ~280 GFLOP/s on four for n >= 4000 (~88 % of the
  ~320 GFLOP/s peak at the observed ~2.5 GHz), but only ~190-240 GFLOP/s for 1000 <= n <= 3000.
* Memory bandwidth: ~25-35 GB/s from DRAM, ~70-120 GB/s for L3-sized data.
* Strassen-Winograd, one level, memory-lean: 0.78-0.84x of MKL at n = 4000, 1.10x at 8000,
  1.10-1.14x at 12000-16000. Two levels: 1.26x only at n = 16000.
* AMX-INT8 (TDPBSSD): 5.1-5.6 Tera-int-ops/s per core in registers, ~7-7.5 Tops/s for a full
  int8 GEMM on 4 cores (same as oneDNN 3.9 on this machine) = ~26x the FP64 DGEMM rate.

| # | idea | Strassen weakness attacked / mechanism | why it might work | cost | kill test | outcome |
|---|---|---|---|---|---|---|
| 1 | FP64 emulation on AMX-INT8 via CRT (Ozaki scheme II) | not a Strassen fix: replaces the FP64 FMA bottleneck by int8 matrix units | int8 GEMM ~26x faster per op than DGEMM here; needs ~14-16 int8 GEMMs for FP64 accuracy | high (kernels, conversion, CRT) | measured int8/FP64 throughput ratio < ~16 -> dead | **ratio ~26 -> alive; 1.2-1.5x over MKL at 2000-20000 with 14 moduli, 1.1-1.3x with 16** (known method; see novelty) |
| 2 | Fused multi-level Strassen: 2 levels as one <4,4,4;49> level with single-pass operand sums, task-parallel leaves | addition memory traffic; MKL's weak multithreaded efficiency at medium sizes | one pass reads 16 blocks once instead of 2 levels of pairwise passes; single-threaded leaves run at ~250-265 GFLOP/s aggregate | medium (generic SLP engine) | no gain over 1-level memory-lean Strassen at n = 8000 -> dead | small gain: 1.13-1.25x at 2000-8000 where plain Strassen gives 0.8-1.1x; within noise of other plans at large n |
| 3 | Better-exponent / lower-rank base cases (<4,4,4;48> rational, <3,3,6;40>, <5,5,5;93>, <3,3,3;23>) | multiplication count per level | 48/64 vs 49/64 (2 %), 40/54 vs (7/8)^~2.3 | low with the generic engine + scheme library | cost model + one measurement at 8000/12000 | <4,4,4;48> best measured bilinear plan at 12000 (1.20x vs 1.16x for 49) - within noise; others no better; big ones do not fit in memory |
| 4 | Alternative basis (Karstadt-Schwartz) / sparsified schemes | number of additions | 12 instead of 15 additions per level | medium | cost model sensitivity: what if passes were free? | **killed by the model**: removing all operand buffers gains < 4 % (passes are memory-bound and second order); not implemented |
| 5 | Exact modular Strassen inside the emulation | none of Strassen's numerical weakness exists over Z/p | saves 12.5 % of the AMX work per level with bit-identical results | medium | measured AMX time drops by ~1/8 and O(n^2) overhead < saving | generic version (`ozfmm.c`): AMX time -13 % but overhead larger -> slower. Specialised version (`ozw.c`, unsigned residues, byte-level modular sums, fused output combination): **bit-identical, 3-7 % faster than the plain emulation for 6000 <= n <= 16000 with 14 moduli**; slower at n <= 4000 (per-call overhead of 7x more, smaller GEMMs) and no better when its 7/4x larger buffers force memory blocking (16 moduli: tie at n = 16000, falls back to the plain emulation at 20000) |
| 6 | Floating-point Strassen on top of emulated leaves | uses both savings | multiplicative speedups | low | speed at 8000 and accuracy | no gain at 8000 (leaves at 4000 less efficient); accuracy is Strassen's normwise behaviour unless scaled |
| 7 | Border-rank (APA) schemes, e.g. Bini <2,2,3> border rank 10 | multiplication count | 10 instead of 11 | low | error analysis: error ~ eps*|A||B| + u/eps -> best ~ sqrt(u) ~ 1e-8 | **killed by analysis**: cannot reach double precision (8 digits lost); exact conversions cost more than they save |
| 8 | Laser method / asymptotic constructions | exponent | - | - | constants (the brief's earlier finding: wins only near n ~ 1e11) | not pursued (per brief) |
| 9 | Better AMX int8 kernel | dominant cost of idea 1 | microkernel loop sustains ~3.3 Tops/core when data is in L2, the GEMM only ~1.8 | high | beat oneDNN's AMX matmul (7.0 Tops/s here) | matched oneDNN (7.2-7.6 Tops/s), not beaten; ~1.7x head-room remains unexploited |
| 10 | Outside-inside power-of-two scaling | Strassen's (and the emulation's) sensitivity to badly scaled inputs | exact, O(n^2) | low | accuracy on row/col- and inner-scaled inputs | fixes both families on diagonally scaled inputs; built into the emulation (3-4 % cost); does not help entrywise-random exponents |
| 11 | Certified emulation (per-entry rigorous bound vs a rigorous lower bound of abs(A)·abs(B) from one extra int8 GEMM; Dot2 / DGEMM repair) | componentwise failures of the emulation on inputs mixing magnitudes (checker, decay, wide random exponents), found after the first independent review | the plain emulation's error bound is only normwise per row/column | medium | overhead > 10 %, or any entry worse than DGEMM | **overhead ~3 %, never worse than DGEMM** on all 57 tested class/size combinations; conservative (needs 16 moduli, rejects some accurate results); 1.01-1.18x MKL when everything certifies, ~0.8x when nothing does |
