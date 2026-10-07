# FP64 GEMM emulation with low-precision units (Ozaki schemes): literature survey

Date: 2026-10-07. Goal: decide whether an Ozaki-type FP64 emulation on Intel AMX can beat MKL/OpenBLAS DGEMM in wall-clock time at n = 500 to 20,000.

**How this was checked.** The sandbox blocked arxiv.org, publishers, intel.com and nvidia.com. GitHub was reachable, so I read the GEMMul8 and ozIMMU source code directly. Everything else comes from search-engine snippets of abstracts and HTML pages.

| Tag | Meaning |
|---|---|
| **[V]** | Verified from the primary source (code or README I read) |
| **[A]** | Abstract or paper text, seen only through search snippets |
| **[U]** | Unverified, or the snippets conflict |
| **[D]** | My own derivation, not taken from a paper |

"Measured" means a number the authors report from a benchmark. "Claim" means a qualitative statement.

---

## TL;DR for this project

1. **Ozaki II on AMX-INT8 on a CPU has been done**, on the same CPU generation as the target. Kouya, arXiv:2609.27831 (2026), used a two-socket Xeon Gold 6526Y, which is Emerald Rapids. The paper targets *multiple precision* (53 to 2048 bits). Its comparisons are against MPFR and Ozaki I, not against MKL DGEMM at 53 bits **[A]**.
2. **Theoretical ceiling [D].**
   - Per core per cycle, AMX-INT8 does 1024 MACs and AVX-512 FP64 does 16 FMAs, a ratio of **64×**.
   - Ozaki II needs about 14 to 16 INT8 GEMMs for FP64-level accuracy, so the best possible gain is **4.0 to 4.6× over DGEMM** before overheads.
   - AVX512-VNNI is only 8× the FP64 rate (128 MACs per cycle), so it **cannot win**. AMX-BF16 is 32×.
3. **Measured on CPU so far [A].**
   - AMX-INT8 GEMM inside the Ozaki II pipeline sustained 39 to 43 Top/s at N=K=4096 and up to **51.6 Top/s at N=K=8192** on 32 Emerald Rapids cores. An isolated GEMM reached only about 20 Top/s at N=4096 (Kouya).
   - With 15 moduli, 51.6 Top/s is about **3.4 TFLOP/s of FP64-equivalent work [D]**. A rough DGEMM figure for 32 cores at about 2 GHz is 32 flop/cycle × 32 × 2.0 GHz ≈ **2 TFLOP/s [D, frequency U]**. So the realistic margin is **at most about 1.5 to 1.7×**, and only at large n.
   - A BF16-AMX Ozaki-I variant reached **up to 1.72× over oneMKL DGEMM**, but only with 6 retained products and a narrow exponent range. Retaining 10, 15 or 21 products gave "diminishing returns or no improvement" (Cui & Liu, arXiv:2609.04663) **[A]**.
4. **The accuracy limitation is structural.** Ozaki II uses one power-of-two scale per row of A and per column of B. Its error is therefore bounded **row/column-normwise**, not componentwise as DGEMM's is. Accuracy drops when exponents spread widely inside a row of A or a column of B. NVIDIA handles this by estimating the needed bits (ESC) and falling back to native FP64 (ADP) **[A]**.

---

## Q1. Ozaki scheme I (Ozaki, Ogita, Oishi, Rump, 2012)

**Citation.** *Numer. Algorithms* 59(1):95–118, 2012. DOI [10.1007/s11075-011-9478-1](https://doi.org/10.1007/s11075-011-9478-1).

**Idea [A].**
- A fast error-free splitting of floating-point numbers turns A·B into an unevaluated sum of products of floating-point matrices.
- Each slice product is computed exactly by an ordinary BLAS GEMM.
- Applying the transformation only partially gives an accurate approximation with an *a priori* error estimate. The dominant cost is BLAS GEMMs.

**Slice width.** Every slice product must be exact in the working unit, so a slice can carry only about (t − log2 k)/2 significant bits, where t is the unit's precision and k the inner dimension **[D]**. The slices are aligned to a *per-row* (for A) or *per-column* (for B) maximum exponent.

**Verified example, ozIMMU (INT8 Ozaki I) [V, source code].**
- Bits per INT8 slice = `min(7, (31 − ceil(log2 k))/2)`, which is 7 bits plus sign for k ≤ 2^17.
- Element a_ik loses low bits unless num_split × 7 ≥ 53 + (e_rowmax − e_ik).
- So even perfectly scaled rows need 8 slices. The number of INT8 GEMMs for s slices is s(s+1)/2, which is **36 for s = 8**.
- Snippets of Uchino et al. say "7 or 8 slices (28–35 GEMMs)" are needed for DGEMM-level accuracy on well-conditioned inputs **[A]**.

**Badly scaled inputs.**
- "Weak tolerance for input with a wide exponent range": more splits are needed to keep the whole mantissa (Ootomo et al.) **[A]**.
- Abdelfattah, Dongarra, Fasi, Mikaitis, Tisseur, arXiv:2506.11277: the integer-slice method "may become inaccurate if rows of A or columns of B are badly scaled". They also give a cheap estimator of the minimum number of products for a target accuracy **[A]**.

## Q2. Mukunoki, Ozaki, Ogita, Imamura, ISC 2020

**Citation.** "DGEMM using Tensor Cores, and its accurate and reproducible versions", LNCS 12151, pp. 230–248, 2020. DOI [10.1007/978-3-030-50743-5_12](https://doi.org/10.1007/978-3-030-50743-5_12).

**Method.** FP16 Tensor Cores (FP16 inputs, FP32 accumulation), not INT8.

**Measured [A].** On a Titan RTX with inputs of dynamic range 1e9:
- The emulation reached about **980 GFlops** of FP64 work.
- cublasDgemm reached **539 GFlops**.
- Speed depends on the absolute-value range of the input entries.
- One snippet instead said "3.7 TFlops"; the 980/539 version is more specific, so I treat 3.7 TFlops as [U].

## Q3. Ootomo/Yokota and Uchino/Ozaki/Imamura

- **Ootomo & Yokota**, IJHPCA 36(4):475–491, 2022, arXiv:2203.03341. SGEMM emulation with error correction. Measured on A100 **[A]**:
  - 51 TFlop/s on FP16 Tensor Cores (limited exponent range).
  - 33 TFlop/s on TF32 Tensor Cores (full FP32 range).
  - Both exceed A100's FP32 peak.
- **Ootomo, Ozaki, Yokota**, "DGEMM on integer matrix multiplication unit", IJHPCA 38(4):297–313, 2024, DOI 10.1177/10943420241239588, arXiv:2306.11975. This is ozIMMU, Ozaki I on INT8 Tensor Cores **[A]**:
  - Faster than cuBLAS DGEMM and than FP16-TC Ozaki on consumer GPUs.
  - Speeds up a quantum-circuit simulation by up to 4.33× at FP64 accuracy.
- **Uchino, Ozaki, Imamura**, IJHPCA 39(3):462–476, 2025, DOI [10.1177/10943420241313064](https://doi.org/10.1177/10943420241313064), arXiv:2409.13313.
  - Methods: accumulation on INT8 TCs (ozIMMU_EF), round-to-nearest splitting (RN), and both combined (H) **[V, accelerator_for_ozIMMU README]**.
  - Measured on RTX 4090: EF-12 and H-12 were **1.6× and 1.4×** faster than ozIMMU at n=4096 **[A]**.
  - Measured on GH200: EF-11 and H-11 were **1.5× and 1.6×** faster than ozIMMU at n=2048 **[A]**.
  - A snippet says that for k=3 slices and n=16384 the variants ran 33.8 to 44.4× faster than FP64. That is an FP64-weak GPU at very low accuracy, so I treat it as [U].

## Q4. Ozaki scheme II (Ozaki, Uchino, Imamura)

**Citation.** arXiv:2504.08009 (2025). Published in IJHPCA 2026, DOI [10.1177/10943420261467787](https://doi.org/10.1177/10943420261467787).

**Algorithm.** Checked against the GEMMul8 code **[V]**:
1. **Scaling.** Compute per-row shifts s_i for A and per-column shifts t_j for B. Form integer matrices A' = trunc(2^{s_i} a_ik) and B' likewise.
   - *Fast mode:* s_i = floor(L − ½·log2‖a_i‖₂²), where L = log2P = round-down(log2(P−1)/2 − 0.5) and P is the product of the moduli. By Cauchy–Schwarz, |A'B'|_ij < P/2, so the CRT result is unique.
   - *Accurate mode:* one extra INT8 GEMM on coarse upper bounds of |A| and |B| bounds (|A'||B'|) more tightly. It costs one more GEMM and gives more bits.
2. **Residues.** Reduce A' and B' modulo each p_l into INT8. The INT8 moduli are 256, 255, 253, 251, 247, 241, 239, 233, 229, 227, 223, 217, 211, 199, 197, 193, 191, 181, 179, 173. They are pairwise coprime, not all prime **[V, table.hpp]**.
3. **Products.** One exact INT8×INT8→INT32 GEMM per modulus, followed by reduction mod p_l. GEMMul8 blocks the k dimension to avoid INT32 overflow. With |residue| ≤ 128, k ≤ 2^17 is safe **[D]**.
4. **Reconstruction.** CRT gives the centred integer A'B'. It is then converted to double and multiplied by 2^{−(s_i+t_j)} **[V]**.

**Number of GEMMs.** Ozaki II needs O(#moduli) GEMMs, against O(s²) for Ozaki I **[A]**. Effective bits per real INT8 configuration, from the GEMMul8 README and matching the log2P table **[V]**:

| Moduli | 12 | 13 | 14 | 15 | 16 | 18 | 20 |
|---|---|---|---|---|---|---|---|
| Effective bits | 47 | 51 | 55 | 59 | 63 | 70 | 78 |

**Measured [A].**
- FP64 emulation: **7.4 to 9.8 TFLOPS on RTX 4090** and **56.6 to 80.2 TFLOPS on GH200**, above native FP64 on both.
- CPU result in the same paper: using DGEMM as the base GEMM, Ozaki II was **up to 2.3× faster than Ozaki I for quadruple-precision emulation**.
- Another snippet: OS II-fast-14 at n=16384 reached 24.5 TFLOPS on A100 and 81.6 TFLOPS on GH200, about 1.4× DGEMM.

**Follow-ups [A] unless marked.**

| Work | Citation | Result or claim |
|---|---|---|
| INT8 matrix-engine DGEMM/SGEMM emulation (Uchino et al.) | SC'25 Workshops, DOI 10.1145/3731599.3767539, arXiv:2508.03984 | **Measured, GH200:** DGEMM emulation **1.4×** faster with **+43%** power efficiency; SGEMM emulation **3.0×** faster with **+154%** |
| Complex GEMM via CRT | arXiv:2512.08321, ISC 2026 | **Measured, B200:** **4.4 to 6.5×** over cuBLAS ZGEMM and 4.0 to 5.6× over CGEMM |
| FP8 variant | arXiv:2603.10634 | Claim: motivated by reduced INT8 throughput on Blackwell Ultra and Rubin |
| Fast-mode scaling fix (Kawakami & Takahashi) | arXiv:2606.29129 | Claim: the original fast-mode scaling is not scale-invariant and can cause CRT recovery failure; fixed at no extra cost |
| Integer-program choice of moduli (Mazumder, Calotoiu, Hoefler) | arXiv:2609.37693 | **Measured:** up to 83× native FP64 on B300 |
| Libraries | [GEMMul8](https://github.com/RIKEN-RCCS/GEMMul8) (CUDA/HIP), [ozIMMU](https://github.com/enp1s0/ozIMMU) | GPU only. **No CPU backend [V]** |

## Q5. NVIDIA adoption

**cuBLAS releases [A, cuBLAS docs and blog snippets].**
- cuBLAS 12.9: FP32 emulation with the **BF16x9** algorithm on compute capability 10.0 and 10.3.
- CUDA **13.0 Update 2**: FP64 "fixed-point emulation" based on Ozaki-I/II.
  - One power-of-two scale is shared per row of A and per column of B, scaling elements into [−1, 1].
  - ADP (automatic dynamic precision) is the default.
  - Controls: `cublasSetFixedPointEmulationMantissaBitOffset`, `cublasSetFixedPointEmulationMaxMantissaBitCount`, and the `CUBLAS_EMULATION_STRATEGY` environment variable (`performant` or `eager`).
- NVIDIA's blog cites 1.5 to 3× end-to-end speedups in ecTrans, BerkeleyGW and Quantum ESPRESSO.

**Schwarz, Anders, Brower, Bayraktar, Gunnels, Clark et al. (NVIDIA)**, "Guaranteed DGEMM Accuracy While Using Reduced Precision Tensor Cores Through Extensions of the Ozaki Scheme", arXiv:2511.13778, SCA/HPCAsia 2026 Workshops, DOI 10.1145/3773656.3773670 **[A]**:
- **ESC** (Exponent Span Capacity) is a conservative, input-dependent estimate of how many slices FP64 accuracy needs.
- **ADP** runs on the GPU with no host synchronisation and falls back to native FP64 when emulation would not pay off.
- An unsigned-integer slicing scheme reduces wasted bits.
- **Measured:** ADP stays at FP64 fidelity on BLAS-grading tests with **<10% overhead**. At a 55-bit setting it reaches **2.3× native DGEMM on GB200** and **13.2× on RTX PRO 6000 Blackwell Server Edition**.

## Q6. FP64 or FP32 emulation on Intel CPUs

| Work | Hardware | Result |
|---|---|---|
| Kouya, "Ozaki Scheme II Is Fast on CPUs Too", arXiv:2609.27831 (2026) **[A]** | 2-socket Xeon Gold 6526Y (Emerald Rapids, 32 cores) | Two backends: exact AMX INT8→INT32 tile GEMM, and DGEMM. **Measured:** within 1 ulp of MPFR for 53 to 2048 bits at N = 256 to 8192. Up to 167× faster than naive MPFR, up to 588× faster than BNCmatmul Strassen, 9 to 78× faster than Ozaki I. The DGEMM backend wins below N≈2048 (fewer moduli), the AMX backend above. AMX-INT8 rates as in TL;DR point 3. No 53-bit comparison against MKL DGEMM appears in the abstract **[U]**. |
| Kouya, same paper, Arm port **[A]** | NVIDIA GB10 (Arm SVE2-i8mm) | **Measured:** 6.5 Top/s INT8 |
| Cui & Liu, "BF16 Component-Product Emulation of FP32 and FP64 GEMM on Intel AMX", arXiv:2609.04663 (2026) **[A]** | 2× Xeon Platinum 8462Y+ (Sapphire Rapids) | **FP32** (3 BF16 components, 6 products): 1.14 to 2.56× over oneMKL SGEMM with normwise error < 1e-6. **FP64** (fixed 6-slice Ozaki, BF16 products accumulated in FP32 and then FP64, inputs limited to the BF16 exponent range): **up to 1.72× over oneMKL DGEMM with 6 products**. 10, 15 or 21 products give "diminishing returns or no improvement". The abstract does not claim full FP64 accuracy at 6 products **[U]**. |
| Henry, Tang, Heinecke, "Leveraging the bfloat16 AI datatype for higher-precision computations", ARITH 2019, pp. 69–76, DOI 10.1109/ARITH.2019.00019, arXiv:1904.06376 **[A]** | Intel (Cooper Lake era) | 3 BF16 components per FP32 operand and **6 products** recover FP32-level accuracy. BF16x9 is the full 9-product variant, later used by cuBLAS. I could not confirm their measured CPU numbers **[U]**. |
| AVX512-VNNI | — | No FP64-emulation work found. By the arithmetic above it cannot beat DGEMM **[D]**. |

## Q7. Error behaviour of Ozaki II

**Error bound [D], consistent with the code.** Write ΔA = 2^{s}A − A' with |ΔA| < 1, and ΔB likewise. Then:

|c_ij − ĉ_ij| ≤ 2^{−s_i}‖b_j‖₁ + 2^{−t_j}‖a_i‖₁ + k·2^{−s_i−t_j} (plus the final rounding to double).

In fast mode 2^{s_i} ≈ 2^L/‖a_i‖₂, so the error is about 2^{−L}·√k·‖a_i‖₂‖b_j‖₂.

- This is **normwise per row/column pair**. DGEMM's bound is componentwise: γ_k(|A||B|)_ij.
- Element a_ik keeps only about L − log2(‖a_i‖₂/|a_ik|) bits. Entries much smaller than the row norm are truncated or flushed to zero.
- So relative accuracy of c_ij falls when |a_i|ᵀ|b_j| ≪ ‖a_i‖‖b_j‖. That happens with cancellation, or with wide exponent spread inside a row of A or a column of B.

**What the papers say [A].**
- Uchino, Ozaki, Imamura, "Error Analysis of … Ozaki-II", arXiv:2602.02549 (2026): rigorous deterministic analysis. Accuracy "can degrade when the exponent distribution … is wide", and the analysis predicts the number of moduli for a target accuracy.
- The Ozaki II paper: with test-matrix spread parameter φ = 0.5, both modes need **14 or 15 moduli** to reach DGEMM-level accuracy. Accurate mode is more accurate because it overestimates |A'||B'| less.

**Recommendations.**
- GEMMul8 suggests INT8 real with **14 to 15 moduli** in fast mode, which matches cuBLAS fixed-point emulation at 55 mantissa bits. It calls these "practical starting points, not accuracy guarantees" **[V]**.
- NVIDIA estimates the required bits per call (ESC) and falls back to native FP64 when needed **[A]**.

## Q8. Strassen or fast matrix multiplication with exact modular arithmetic

**FFLAS-FFPACK [A].** Dumas, Giorgi, Pernet, *ACM TOMS* 35(3):19, 2008, arXiv:cs/0601133. Exact matrix multiplication over word-size prime fields. It runs floating-point BLAS on integers kept below 2^53, uses Strassen-Winograd, and delays modular reduction. Because the arithmetic is exact, Strassen's instability does not arise.

**Inside Ozaki.** I found **no paper that puts Strassen-Winograd inside Ozaki I or II [U, absence]**. Kouya reports Ozaki II is up to 588× faster than BNCmatmul's multiple-precision Strassen, and notes that Strassen loses "hundreds to thousands" of ulps **[A]**.

**Why it could work [D].** In Ozaki II each modulus GEMM only needs the product mod p_l.
- Winograd's pre-additions can be reduced mod p_l back into INT8, which is an O(n²) cost.
- Post-additions can be done in INT32 or INT64, then reduced.
- The result stays exact, so the final accuracy is unchanged.
- One Winograd level cuts INT8 GEMM work by 7/8 for all 14 to 16 moduli. This looks like an unexplored direction.

## Q9. Intel AMX and AVX-512 hardware facts

**Peak rates.**
- **AMX-INT8:** TDPBSSD computes a 16×16×64 product, 16,384 MACs, at a throughput of **one per 16 cycles**. That is **1024 MACs, or 2048 int8 ops, per cycle per core**. Intel quotes "2,048 INT8 operations per cycle" against 256 for AVX-512 VNNI **[A, Intel AMX brief and Chips and Cheese]**.
- Signed×signed (SS), SU, US and UU variants all exist **[A, ISA reference]**. So Ozaki II's signed residues need no offset trick on AMX.
- **AMX-BF16:** TDPBF16PS, also one per 16 cycles. That is **1024 BF16 flops (512 MACs) per cycle per core [A]**.
- **FP64:** two 512-bit FMA ports give **32 flop per cycle per core**. Real all-core AVX-512 and AMX clocks are lower than turbo; I do not have figures for the target SKU **[U]**.

**Measured int8 throughput [A].**
- Single-tile microbenchmark: 306 GOPS (Discoverer guide). This is not representative of full GEMMs.
- Full GEMM rates: see Kouya in TL;DR point 3.
- No authoritative published MKL `cblas_gemm_s8u8s32` throughput on Sapphire or Emerald Rapids was found **[U]**.

**Does MKL use AMX for int8 GEMM?** The oneMKL 2025.0 release notes say: "Improved `cblas_gemm_s8u8s32` and `cblas_gemm_bf16bf16f32` performance for large problem size on Intel AMX". So these routines use AMX **[A]**.
- `s8u8s32` takes one signed and one unsigned operand. Using it would need an offset or column-sum correction **[D]**.
- Calling oneDNN matmul or raw `_tile_dpbssd` avoids that.

**Target CPU.** family 6 model 207 (0xCF) should be Emerald Rapids, the same generation as Kouya's Xeon Gold 6526Y **[U, from memory]**.

---

## Key links
- Ozaki II: arXiv:2504.08009
- CPU AMX Ozaki II: arXiv:2609.27831
- BF16 on AMX: arXiv:2609.04663
- NVIDIA ADP: arXiv:2511.13778
- Error analysis: arXiv:2602.02549 and arXiv:2506.11277
- Scaling fix: arXiv:2606.29129
- SC'25 workshop paper: arXiv:2508.03984
- Code: https://github.com/RIKEN-RCCS/GEMMul8, https://github.com/enp1s0/ozIMMU, https://github.com/RIKEN-RCCS/accelerator_for_ozIMMU
- FFLAS-FFPACK: arXiv:cs/0601133

## Addendum (from docs/novelty_check.md, 2026-10-07)

* **LIBXS** (H. Pabst, https://github.com/hfp/libxs, `samples/ozaki`) is open-source prior art for
  Ozaki I and II on AMX-INT8 as a drop-in DGEMM replacement (16 moduli by default for FP64, per
  row/column power-of-two scaling, `_tile_dpbuud`). It is benchmarked against our implementation in
  `results/libxs_compare.md`.
* Fast *complex* bilinear tricks (Karatsuba/3M, 2M) are already used on the int8 residues inside
  Ozaki II (Uchino et al. arXiv:2512.08321; Caday arXiv:2609.05419), with the explicit remark that
  they cost no accuracy because the integer arithmetic is exact. Multimodular Strassen-Winograd per
  prime is standard in computer algebra (FLINT `fmpz_mat_mul_multi_mod`, FFLAS-FFPACK). Real block
  Strassen-Winograd inside Ozaki II was not found, but it is a direct transfer of that practice.
* Inside (k-dimension) power-of-two scaling is from Ballard, Benson, Druinsky, Lipshitz, Schwartz
  (SIMAX 2016) for fast matrix multiplication; no use inside Ozaki-type emulation was found.
