# Novelty check (2026-10-07)

Tags: **[V]** I read the primary source (for GitHub code, through a fetch summary). **[A]** Abstract or search snippet only. **[U]** Unverified, from memory or title only.

**Limits.** arxiv.org, alphaxiv, Semantic Scholar, HAL, lip6, cornell.edu and utexas.edu were blocked. GitHub was reachable. The shared web-search budget (200 calls) ran out during item 5, so items 5 and 6 are the least checked. "Found nothing" means only that my search missed it.

---

## 1. FP64 GEMM emulated with Ozaki II on AMX-INT8, compared with MKL DGEMM

**Verdict: partially known.** Ozaki II on AMX-INT8 on Emerald Rapids is already published, and open-source CPU code exists. I found no published timing of Ozaki II at 53-bit accuracy against **MKL** DGEMM.

- **Kouya, T.** "Ozaki Scheme II Is Fast on CPUs Too: Multiple-Precision Matrix Multiplication on Intel AMX-INT8 and Arm SVE2-i8mm", arXiv:2609.27831 (submitted Aug 2026). https://arxiv.org/abs/2609.27831 **[A]**
  - The output is MPFR, at 53 to 2048 bits, for N = 256 to 8192.
  - Hardware: a two-socket Xeon Gold 6526Y (Emerald Rapids, 32 cores), plus an Arm port to NVIDIA GB10 using SVE2-i8mm (SMMLA).
  - It has two backends that share one CRT reconstruction: an exact AMX INT8→INT32 backend and a binary64 DGEMM backend.
  - Accuracy: always within 1 ulp of MPFR.
  - Baselines are naive MPFR (up to 167× slower), BNCmatmul Strassen (up to 588× slower) and Ozaki I with **OpenBLAS** DGEMM (9 to 78× slower).
  - The INT8 GEMM rate is 51.6 Top/s, against 2.3 TFlop/s for OpenBLAS DGEMM. The AMX backend needs 3 to 4.5× more moduli than the binary64 backend.
  - The break-even between the two backends is N≈2048.
  - One snippet gives AMX-to-binary64 time ratios of 1.28 at N=1024, 0.93 at 2048, 0.68 at 4096 and 0.47 at 8192. It may say these are at p=53. **[A, attribution uncertain]**
  - **No snippet shows a comparison with MKL DGEMM at 53 bits.** The DGEMM baseline is OpenBLAS.
- **LIBXS, H. Pabst.** https://github.com/hfp/libxs, `samples/ozaki/`, docs `documentation/ozaki/index.md`. **[V]**
  - It intercepts DGEMM, SGEMM, ZGEMM and CGEMM through LD_PRELOAD and runs Ozaki I (8 int8 slices for fp64) or Ozaki II (CRT, **16 moduli for fp64 by default**, from a table of 20 coprime moduli ≤256: 256, 251, 243, …, 163).
  - It uses the AMX instruction `_tile_dpbuud` (u8×u8). The default switches Scheme 2 to AMX from N≈640.
  - Scaling is one power-of-two exponent per row of A and per column of B.
  - The docs give no comparison with MKL. They report accuracy against the graded-BLAS componentwise criterion.
  - **This is open-source prior art for the implementation itself.**
- **Cui, B., Liu, Y.** "BF16 Component-Product Emulation of FP32 and FP64 GEMM on Intel AMX", arXiv:2609.04663. https://arxiv.org/abs/2609.04663 **[A]**
  - FP64 inputs are split into a fixed six BF16 slices, keeping 6, 10, 15 or 21 products. It only works for inputs inside the BF16 exponent range.
  - It reports up to 1.72× over oneMKL DGEMM. That is not full FP64 accuracy. My own inference: six 8-bit slices hold about 48 bits, fewer than 53.
- **Ozaki, K., Uchino, Y., Imamura, T.** "Ozaki Scheme II: A GEMM-oriented emulation of floating-point matrix multiplication using an integer modular technique", IJHPCA (2026), DOI 10.1177/10943420261467787; arXiv:2504.08009. **[A]** INT8 results are on GPUs only. CPU results use a DGEMM backend, with up to 2.3× over Ozaki I for quadruple precision.
- **GPU work, none of it on CPUs:**
  - Uchino, Ozaki, Imamura, "High-Performance and Power-Efficient Emulation of Matrix Multiplication using INT8 Matrix Engines", SC'25 ScalAH workshop, arXiv:2508.03984. Reports 1.4× over native DGEMM on GH200. **[A]**
  - GEMMul8 (https://github.com/RIKEN-RCCS/GEMMul8) is CUDA/HIP only. **[V]**
  - Ootomo, Ozaki, Yokota, ozIMMU, IJHPCA 38 (2024), arXiv:2306.11975. **[A]**
  - Schwarz et al., ADP/ESC, SCA/HPCAsia'26, arXiv:2511.13778. **[A]**
- **Unchecked:** "Optimization of a GEMM Implementation using Intel AMX", SCA/HPCAsia'26, DOI 10.1145/3773656.3773660. I saw the title only. **[U]**

**Safe claim:** a 53-bit-accurate Ozaki II DGEMM on AMX-INT8 for Sapphire or Emerald Rapids, timed against MKL DGEMM. Cite Kouya and LIBXS as prior implementations.

## 2. Strassen-Winograd applied exactly per modulus inside Ozaki II

**Verdict: partially known.** The principle that a fast bilinear algorithm adds no error inside an exact modular product is standard. Fast *complex* bilinear tricks are already used inside Ozaki II. I found no paper that applies Strassen or Strassen-Winograd over the *real block structure* inside Ozaki I or II.

- **Multimodular integer products with Strassen per prime:**
  - FLINT `fmpz_mat_mul_multi_mod` calls `nmod_mat_mul` for each prime. `nmod_mat_mul` picks Strassen-Winograd automatically and has byte kernels for moduli ≤255. **[V]** (`src/fmpz_mat/mul_multi_mod.c`, `doc/source/nmod_mat.rst`)
  - FFLAS-FFPACK `fgemm_classical_mp.inl` (P. Giorgi) works over `rns_double` and calls `fgemm` per modulus with an `MMHelperAlgo::Winograd` helper. **[V]** One comment says Winograd "switches to Classic" for some mode traits, so whether Winograd actually runs depends on the mode.
  - Dumas, Giorgi, Pernet, "Dense linear algebra over word-size prime fields: the FFLAS and FFPACK packages", ACM TOMS 35(3), 2008. Exact Strassen-Winograd over Z/pZ. **[A]**
  - Ozaki II is "scale to integers, multimodular product, CRT, rescale", so moving this to Ozaki II is a direct step.
- **Fast bilinear algorithms already inside Ozaki II:**
  - Uchino, Ma, Imamura, Ozaki, Gutsche, "Emulation of Complex Matrix Multiplication based on the Chinese Remainder Theorem", arXiv:2512.08321. Applies Karatsuba (3M) to the INT8 residues for ZGEMM and CGEMM; 4.0 to 5.6× over cuBLAS ZGEMM on B200. **[A]**
  - Caday (NVIDIA), "The 2M Multiplication Algorithm for Complex Matrices", arXiv:2609.05419. Pairs 2M with Ozaki II and states that 2M, 3M and 4M "do not affect accuracy as multiplications are always performed in exact integer arithmetic". **[A]**
  - Uchino, Ozaki, Imamura, Ozaki II on FP8, arXiv:2603.10634. Uses Karatsuba to build the integer products from FP8 pieces. **[A]**
- **Not combined anywhere I found.** Kouya *compares* Ozaki II with an MPFR Strassen but does not combine them. One snippet says an emulation paper "assumes divide-and-conquer methods like Strassen's and Winograd's are not applied". **[A, source unclear]**
- LIBXS has no Strassen or Karatsuba in its Ozaki II path. **[V]**

**Safe claim:** this is a transfer of known multimodular Strassen-Winograd practice into FP64 emulation on INT8 hardware, with output bit-identical to plain Ozaki II. The INT8-specific work is engineering: re-reducing operand sums back into the int8 range, or keeping a residue bound that leaves room for them.

## 3. Power-of-two diagonal scaling along k (A·D, D⁻¹·B) inside Ozaki

**Verdict: partially known.** The technique is known for fast matrix multiplication. I found nobody using it for Ozaki emulation.

- **Ballard, Benson, Druinsky, Lipshitz, Schwartz**, "Improving the numerical stability of fast matrix multiplication", SIMAX 37(4), 2016, arXiv:1507.00687. **[A]**
  - Defines outside scaling (rows of A, columns of B; credited to Dumitrescu, Numer. Math. 1998) and inside scaling (A·D, D⁻¹·B), and their combinations.
  - From memory: powers of two avoid rounding. **[U]**
  - Later work extends this to alternative-basis algorithms. **[A]**
- **Ozaki family:** scaling is always per row of A and per column of B, as D·A and B·E with power-of-two D and E. **[A]**
  - Ozaki II.
  - Kawakami and Takahashi, "Improved Scaling for Fast Mode of Ozaki Scheme II", arXiv:2606.29129, a scale-invariant fix to the Cauchy-Schwarz bound. **[A]**
  - GEMMul8 docs: fast and accurate modes, no k scaling. **[V]**
  - LIBXS: per row and column only. Its `OZAKI_DECAY` reorders along k "for smoothness", which is a permutation, not a scaling. **[V]**
- **Abdelfattah, Dongarra, Fasi, Mikaitis, Tisseur**, "Analysis of Floating-Point Matrix Multiplication Computed via Integer Arithmetic", arXiv:2506.11277 (SISC 2026). **[A]**
  - Shows that badly scaled rows of A or columns of B (wide range along k) need more slices: about 7 slices at φ=0 versus more than 20 at φ=100.
  - I found no k-scaling remedy in it. **[U, absence]**
- **Schwarz et al., ADP/ESC**: estimates the number of slices from the data and can fall back to native. No k scaling found. **[A]**
- **Caveat:** inside scaling only removes a rank-1 exponent pattern (a function of k alone). It cannot help when exponents vary randomly entry by entry, consistent with `ideas.md` #10.

## 4. Two- or three-level Strassen-Winograd as one ⟨4,4,4;49⟩ or ⟨8,8,8;343⟩ step, with streaming additions, vendor BLAS and BFS tasks

**Verdict: mostly known. The parts exist separately; I did not find this exact combination.**

- **Benson, A.R., Ballard, G.**, "A framework for practical parallel fast matrix multiplication", PPoPP 2015, arXiv:1409.2908; code at https://github.com/arbenson/fast-matmul. **[V code]**
  - Additions: pairwise, write-once (default) or streaming, with common-subexpression elimination.
  - Parallelism: DFS, BFS or HYBRID, using OpenMP tasks. Leaf products call MKL.
  - Multi-level is plain recursion with the same algorithm, and each level does its own additions. **The code cannot merge levels into one step.** Its input is any ⟨U,V,W⟩, so a Kronecker-composed ⟨4,4,4;49⟩ could still be fed in.
- **Huang, J., Rice, L., Matthews, D.A., van de Geijn, R.A.**, "Generating Families of Practical Fast Matrix Multiplication Algorithms", IPDPS 2017, arXiv:1611.01120 (FLAWN #82). **[A]**
  - Builds multi-level algorithms as Kronecker products of [U,V,W], so two levels of Strassen become a single ⟨4,4,4;49⟩ step.
  - Additions are fused into BLIS packing and the micro-kernel. Parallelism is data-parallel, not task-based. **[V, README]**
- **Huang, Smith, Henry, van de Geijn**, "Strassen's algorithm reloaded", SC16. Covers one and two levels inside BLIS. **[U]**
- **D'Alberto, Bodrato, Nicolau**, ACM TOMS 38(1):2, 2011. Software-pipelined matrix additions and thread allocation for hybrid Strassen and Winograd. **[A]**

**What remains:** the Kronecker-flattened multi-level step, with single-pass streaming additions, on an unmodified vendor BLAS and with BFS task leaves. That is an engineering combination, not a new idea.

## 5. Wall-clock timings for ⟨4,4,4;48⟩ (rational or AlphaEvolve complex) in floating point

**Verdict: found nothing, with low confidence because the search budget ran out mid-item.**

- **Dumas, Pernet, Sedoglavic**, arXiv:2506.13242. A rational scheme valid outside characteristic 2. Operation counts are 347/32·n^{log₄48}, or 7·n^{log₄48} with an alternative basis. **[A]**
- **Dumas, Pernet, Sedoglavic, Tichavský**, "Fast matrix multiplication via recursive ⟨4×4×4:48⟩ algorithms into practice", arXiv:2609.12027. 48 multiplications and 216 additions; leading constant 7.75, or 6.5 with an alternative basis. **[A]** The title suggests an implementation, but **I could not confirm whether it reports timings, on what hardware, or with what speedups.** Read it before claiming novelty.
- arXiv:2603.18699 ("A more accurate rational …") and arXiv:2602.13171 ("Complex to Rational Fast Matrix Multiplication"): I checked titles only. **[U]**
- The PLinOpt repo (jgdumas/plinopt) has no benchmarks. **[V]**
- fflas-ffpack master has only Winograd and Bini schedules, no 4×4×4:48 one. **[V]**
- AlphaEvolve (Novikov et al., 2025): I did not check for timings. **[U]**

## 6. Cost models for practical fast matrix multiplication

- **Huang et al. (SC16, IPDPS 2017)**: time is modelled as arithmetic time plus memory time. **[A]**
  - Arithmetic: leaf multiplication flops and extra addition flops at rate τ_a.
  - Memory: packing and C-update traffic at bandwidth τ_b, with counts taken from the nonzeros of U, V and W at each level.
  - Used to rank 20+ algorithms and level counts. It ignores task scheduling and measured efficiency as a function of shape.
- **Benson and Ballard (2015)**: an empirical "effective GFLOPS" analysis. **[U, from memory]**
  - It explains results through multiplication savings, memory-bound additions, and DFS/BFS/HYBRID load balance.
  - HYBRID runs BFS on ⌊R/P⌋·P subproblems and then DFS on the remainder.
  - As far as I know there is no closed-form model.
- **D'Alberto and Nicolau**, "Adaptive Strassen's matrix multiplication", ICS 2007, and "Adaptive Winograd's matrix multiplications", ACM TOMS 36(1), 2009. **[U]**
  - The recursion cutoff is set at install time from measured leaf BLAS speed versus addition cost.
  - The 2011 TOMS paper adds pipelined additions and thread allocation.
- **Schwartz and Vaknin**, "Pebbling game and alternative basis for high performance matrix multiplication", SISC 45(6), 2023. Uses a pebbling game to model I/O and memory footprint of the encode/decode (addition) phases. **[A]**
- **Ballard, Demmel, Holtz, Lipshitz, Schwartz**, CAPS, SPAA 2012. A BFS/DFS communication-cost model for parallel Strassen. **[U]**
- **Gap:** I found no model that combines all three of measured leaf BLAS efficiency by shape, addition memory traffic, and task-schedule makespan. Huang et al. covers the first two in idealised form. **[absence, U]**
