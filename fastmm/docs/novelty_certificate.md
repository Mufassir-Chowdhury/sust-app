# Novelty check: per-entry certificate for Ozaki II emulation (2026-10-08)

Tags: **[V]** I read the source code. **[A]** Abstract or search snippet only, taken from earlier agents' notes (`lit_emulation.md`, `novelty_check.md`). **[U]** From memory, not read.

**Limits.** arxiv.org, dl.acm.org, link.springer.com, docs.nvidia.com, developer.nvidia.com and api.crossref.org were all blocked. The shared web-search budget was already used up when this check started, so I ran **no new searches**. GitHub was reachable, so I cloned and read GEMMul8 (ce63fe9, 2026-10-08), ozIMMU (08eea92), LIBXS `samples/ozaki` (797f0b9, 2026-10-07) and NVIDIA CUDALibrarySamples (07c4f09). Every claim about a paper's content is therefore [A] or [U].

---

## Q1. Is the certified mode already published or implemented?

**Verdict: closely related work exists. The full combination was not found.** I did not find this combination anywhere: a rigorous per-entry a posteriori bound for an Ozaki II result, a componentwise test against a *lower* bound of |A||B| computed by one extra int8 GEMM, and repair of only the flagged entries (Dot2 per entry, DGEMM per block).

Each piece has a close relative:

- **Extra int8 GEMM on quantized |A|, |B|.**
  - GEMMul8's accurate mode does the same thing with the opposite rounding and for a different purpose. `upperBound_lo` = `ceil_scalbn_8i` rounds |a|·2^sft *up* into int8 (at most 5 significant bits). One INT8 GEMM then gives an upper bound of |A'||B'|, and `calc_sft` derives the per-row and per-column shifts from it. This maximises the bits kept; it certifies nothing. [V] [src/oz2/scaling/accu/store.hpp, calc_sft.hpp](https://github.com/RIKEN-RCCS/GEMMul8). The method is from Uchino et al., SC'25 workshops, [arXiv:2508.03984](https://arxiv.org/abs/2508.03984) [A].
  - The project uses floor rounding to get a *lower* bound, which turns the same trick into a certificate. It is the same trick put to a new use.
- **Rigorous a posteriori bounds for products.** Classical verified numerics bounds |fl(AB) − AB| by γ_k·|A||B|, with |A||B| evaluated in upward rounding:
  - Rump, "Fast and parallel interval arithmetic", BIT 1999 [U].
  - Rump, "Fast interval matrix multiplication", Numer. Algorithms 61, 2012 [U].
  - Ozaki, Ogita, Rump, Oishi, "Fast algorithms for floating-point interval matrix multiplication", JCAM 236, 2012 [U].

  These give *upper* bounds on the error. None tests a relative componentwise criterion, so none needs a lower bound of |A||B|.
- **A posteriori validation inside Ozaki I.** Ozaki, Ogita, Oishi, "Error-free transformation of matrix multiplication with a posteriori validation", Numer. Linear Algebra Appl. 2016 [U]. **This is the closest paper; read it before claiming novelty.** As I remember it, the bound comes from extra floating-point GEMMs on the unsplit remainders, it is for Ozaki I, and nothing is selectively repaired.
- **Per-entry repair.** Dot2 is Ogita, Rump, Oishi, "Accurate sum and dot product", SISC 26, 2005 [U]. Using it to repair individual entries of an emulated GEMM was not found.
- **The error term.** E_ij comes from the rounding of the scaled inputs to integers. This is the standard Ozaki II error analysis: Uchino, Ozaki, Imamura, [arXiv:2602.02549](https://arxiv.org/abs/2602.02549) [A], which is a priori and used to pick the number of moduli.

## Q2. Do cuBLAS or other libraries (a) estimate error, (b) pick slices or moduli adaptively, (c) fall back to native FP64?

**Verdict: (b) and (c) are already implemented, but only per call. (a) as a cheap per-entry estimate was not found.**

| Library | (a) Error estimate | (b) Adaptive count | (c) Fallback |
|---|---|---|---|
| **cuBLAS** (CUDA 13.0u2+, ADP) | A priori, per call: ESC conservatively estimates the bits needed from the inputs [A]. I could not read a per-entry estimate in the docs. | Yes. Dynamic mantissa control "determine[s] how many emulated mantissa bits should be retained to have the same or better accuracy than native FP64" [V, sample]. | Yes, the whole call. `cublasSetFixedPointEmulationMaxMantissaBitCount` sets "a maximum value before falling back to native FP64", and the `performant` strategy chooses between emulation and native by heuristics [V, [dgemm_dynamic sample](https://github.com/NVIDIA/CUDALibrarySamples/tree/main/cuBLAS/Emulation/dgemm_dynamic)]. |
| **ozIMMU** `fp64_int8_auto` | Per-call heuristic: the *average* mantissa bits lost per element, measured against the row or column maximum exponent. It is not a rigorous bound. | Yes. It picks the smallest split count from 3 to 18 whose average loss is at most the threshold (`OZIMMU_AUTO_AVG_MANTISSA_LOSS_THRESHOLD`). | Yes, the whole call goes to cuBLAS DGEMM if no split count qualifies. [V, `src/split.cu` `auto_mode_select_core`, [ozIMMU](https://github.com/enp1s0/ozIMMU)] |
| **GEMMul8** | No. `num_moduli` values are "practical starting points, not accuracy guarantees". | No, the user sets it. | Native BLAS is used only when `num_moduli` is out of range. [V, [README](https://github.com/RIKEN-RCCS/GEMMul8)] |
| **LIBXS** | Diagnostics only: `OZAKI_VERBOSE` / `OZAKI_EPS` / `GRADE` run a *reference BLAS GEMM* and compare against it. | Ozaki I only: an "adaptive cutoff" skips empty high slices. | No accuracy-driven fallback found. [V, [samples/ozaki](https://github.com/hfp/libxs/tree/main/samples/ozaki), [docs](https://github.com/hfp/libxs/blob/main/documentation/ozaki/index.md)] |

Further sources:
- Schwarz et al. (NVIDIA), "Guaranteed DGEMM accuracy … extensions of the Ozaki scheme", [arXiv:2511.13778](https://arxiv.org/abs/2511.13778) [A]. ESC plus ADP gives FP64 fidelity on BLAS-grading tests with less than 10% overhead. I could not check whether ESC is computed per matrix or per row and column.
- Mukunoki, Ozaki, Ogita, Imamura, ISC 2020, DOI 10.1007/978-3-030-50743-5_12 [A]. Ozaki I with an input-dependent number of splits; the reported speed depends on the input's range.

**The difference:** every adaptive scheme I found decides once per GEMM call, a priori, and falls back for the whole call. The project decides per entry, a posteriori, and repairs only the flagged entries or blocks.

## Q3. Is the weakness already documented?

**Verdict: already published.**

- Abdelfattah, Dongarra, Fasi, Mikaitis, Tisseur, [arXiv:2506.11277](https://arxiv.org/abs/2506.11277) [A]: the integer-slice method "may become inaccurate if rows of A or columns of B are badly scaled".
- Uchino, Ozaki, Imamura, [arXiv:2602.02549](https://arxiv.org/abs/2602.02549) [A]: accuracy degrades with a wide exponent distribution.
- Ootomo, Ozaki, Yokota, [arXiv:2306.11975](https://arxiv.org/abs/2306.11975) [A]: "weak tolerance for input with a wide exponent range".
- Demmel et al., "How to grade the accuracy of an implementation of the BLAS" (BLIS Retreat 2024, [slides](https://www.cs.utexas.edu/~flame/BLISRetreat2024/slides/Grading_BLAS.pdf)), Test 2b. It detects *fixed-point* GEMM through componentwise failure on y = xD, z = D⁻¹xᵀ, which is an exponent-graded inner product. NVIDIA ships it as `test4` in [gemm_grading](https://github.com/NVIDIA/CUDALibrarySamples/tree/main/cuBLAS/Emulation/gemm_grading) [V code]. That pattern is rank-1 along k, so the project's inner scaling removes it.
- LIBXS `gemm.c` [V] documents the per-element case directly. `EVIL=-N` gives each entry a pseudorandom exponent in [0, N], with the opposite sign for B, so large entries meet small ones. Its comment says: "Per-element exponent spread degrades componentwise accuracy steeply": grade 233 at EVIL=-8, 8960 at -16, and 4.7e9 at -52 (n = 512, bound f(n) = n), "while eps reads 6.9e-16".

The mechanism (error ∝ ‖a_i‖‖b_j‖, not ∝ (|A||B|)_ij) and adversarial per-element patterns are therefore both known. The checkerboard construction is one more instance of them.

## What the project can claim

- A cheap, rigorous, per-entry componentwise certificate for Ozaki II: O(n²) for the norms plus one int8 GEMM for the lower bound.
- Selective repair of the flagged entries, giving (θ+1)u(|A||B|)_ij or a DGEMM result.

This holds **only after** these are checked:
1. Ozaki, Ogita, Oishi (NLAA 2016).
2. The full text of Schwarz et al. (whether ESC or ADP has a per-entry component).
3. 2026 arXiv work that the exhausted search budget may have missed.
