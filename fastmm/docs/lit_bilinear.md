# Literature survey: bilinear (Strassen-like) fast matrix multiplication for practical FP64 GEMM

Compiled 2026-10-07 for the goal: beat tuned DGEMM *and* a careful Strassen-Winograd in wall-clock time on a CPU, n = 500..20,000, with double-precision accuracy.

**Access caveat.** The sandbox proxy blocked arXiv, ACM DL, SIAM, Springer, Nature, HAL and university pages, so I could not read full texts. Evidence comes from search-indexed abstracts, from GitHub repositories I could read directly, and from search snippets.

Confidence tags:
- **[SRC]**: read in a primary artifact (a repo, README or data table).
- **[ABS]**: from the paper's abstract.
- **[SNIP]**: from a search snippet or secondary write-up. Plausible, but I did not read it in the paper.
- **[UNV]**: could not confirm.
- **[DER]**: my own arithmetic.

---

## 0. Bottom line for the project

1. **Measured CPU FP64 wins over vendor DGEMM.** The strongest verified ones come from 2x2 Strassen in an *alternative basis* (Karstadt-Schwartz lineage):
   - It beats DGEMM from n = 96, reaching close to 2x sequentially at large n (SISC 2023) [ABS].
   - An automatic generator reports more than 50% sequential and 25% parallel gains over vendor GEMM (SPAA 2026) [ABS].
   - The hardware, maximum n and thread counts are [UNV].
2. **Better exponents, no verified wall-clock win.** I found no verified measurement where an asymptotically better small base case (<3,3,6;40>, rational <4,4,4;48>, trilinear aggregation) beats a tuned Strassen-Winograd in wall-clock time.
3. **The arithmetic gap at n ≤ 20,000 is tiny.**
   - With full recursion, <4,4,4;48> (7.75·n^2.7925) only beats Strassen-Winograd (6·n^2.8074) for n > ~3·10^7. The alternative-basis versions (6.5 vs 5) need n > ~4.6·10^7 [DER].
   - With a BLAS cutoff, one <4,4,4;48> level against two Strassen-Winograd levels saves 1 of 49 block multiplications (~2%). It costs 216 vs 165 block-sized linear ops, so it wins on flops only for block size b > 26 [DER]. Those extra memory-bound additions probably cancel the 2% gain on multicore [DER, unmeasured].
4. **Omega results are galactic.** None of the 2024-2026 omega results (2.371339, 2.371177, OpenAI's claimed 9/4) is relevant at n ≤ 20,000.
5. **Accuracy rules out approximate schemes.** Every Strassen-like method gives only normwise error bounds, with error growth (n/n0)^e where e > 0. APA/border-rank schemes are not accurate to full FP64 precision.

---

## 1. Strassen 1969; optimality of 7

- **Strassen.** "Gaussian elimination is not optimal", *Numer. Math.* 13:354-356 (1969), DOI 10.1007/BF02165411. It gives 7 multiplications for 2x2 [ABS].
- **Optimality of 7.**
  - Winograd, "On multiplication of 2x2 matrices", *Linear Algebra Appl.* 4:381-388 (1971), proved rank 7 is optimal [SNIP].
  - Hopcroft & Kerr, *SIAM J. Appl. Math.* 20(1) (1971), proved independently that 6 multiplications are impossible [SNIP].
  - Landsberg showed the border rank of 2x2 is also 7 (arXiv math/0407224) [SNIP].
- **Verdict: claim confirmed.**

## 2. Additions and the alternative basis (Schwartz group)

**Additions in the standard basis.** Winograd's variant cuts additions from 18 to 15, so the leading coefficient drops from 7 to 6. Probert (1976) proved 15 is necessary in the standard basis [SNIP].

**Karstadt & Schwartz.** "Matrix multiplication, a little faster", SPAA'17 (DOI 10.1145/3087556.3087579) and *J. ACM* 67(1), Art. 1 (2020) (DOI 10.1145/3364504). Patent US 10,387,534.
- *Claimed:* 12 additions in an alternative basis, so the leading coefficient drops from 6 to 5. They also prove 5 is optimal for 2x2 base cases even allowing a change of basis [ABS].
- *Measured:* runtime against Strassen-Winograd as a function of the number of recursion steps before switching to MKL DGEMM [ABS].
- A snippet, probably from the Vaknin thesis, says the optimized variant beats Strassen-Winograd for n ≥ 32 and approaches the theoretical ratio 5/6 ≈ 0.83 as levels increase [SNIP].
- The same line of work reduced the linear operations of Smirnov's <6,3,3;40> from 1246 to 198 (about 83-84%) [SNIP].

**Beniamini & Schwartz.** "Faster matrix multiplication via sparse decomposition", SPAA 2019.
- *Claimed:* subcubic algorithms with leading coefficient 2, e.g. 2n^(log3 23)+o(·). Several are proved optimal [ABS].
- *Measured:* [UNV].

**Beniamini, Cheng, Holtz, Karstadt & Schwartz.** "Sparsifying the operators of fast matrix multiplication algorithms", arXiv 2008.03759 (2020).
- *Claimed:* heuristic and optimal sparsification methods for leading coefficients [ABS].
- *Measured:* none found [UNV].

**Hadas & Schwartz.** "Towards practical fast matrix multiplication based on trilinear aggregation", ISSAC 2023, DOI 10.1145/3597066.3597099.
- *Claimed:* Pan's exponents with small leading coefficients that "have the potential to outperform" [ABS].
- *Measured:* nothing in the abstract.

**Schwartz & Zwecher.** "Towards faster feasible matrix multiplication by trilinear aggregation", arXiv 2508.01748, ISSAC 2026.
- *Claimed:* O(n^2.773203), against Pan's 1982 bound of 2.773372. It is the fastest algorithm with base case below 1000 [ABS].
- *Measured:* [UNV].

**Schwartz & Vaknin.** "Pebbling game and alternative basis for high performance matrix multiplication", *SIAM J. Sci. Comput.* 45(6):C277-C303 (2023), DOI 10.1137/22M1502719.
- *Measured:* beats DGEMM from n = 96; speedup grows to "nearly ×2" sequentially at larger n, and is larger in parallel for some n [ABS].
- The baseline is Intel MKL [SNIP, thesis]. CPU model, maximum n and cores are [UNV].
- Note: about 2x needs about 5 Strassen levels, since (8/7)^5 = 1.95. That implies n far above 20,000, or very small cutoffs [DER].

**Schwartz, Toledo, Vaknin & Wiernik.** "Alternative basis matrix multiplication is fast and stable", IPDPS 2024, pp. 38-51. Journal version in *Numer. Math.* (2026), DOI 10.1007/s00211-026-01531-9.
- *Claimed:* the first error bounds for alternative-basis FMM, asymptotically the same as Strassen's. Also a 2x2 algorithm that is optimal in both leading coefficient and error exponent, beating the Bini-Lotti speed/stability trade-off [ABS].
- *Measured:* "on par with best in class" for speed and stability [ABS]. The numbers are [UNV].

**Brief announcement.** "An automatic framework for high performance alternative basis fast matrix multiplication", SPAA 2026, DOI 10.1145/3816782.3819195.
- *Claimed:* common-subexpression elimination plus a pebbling engine cut the arithmetic and communication leading coefficients by 20-90% and memory by about 50-70%.
- *Measured:* "typically faster than vendor-tuned GEMM, exceeding 50% sequential and 25% parallel", often faster than state-of-the-art Strassen code [ABS].
- Authors, hardware and sizes are [UNV].

## 3. 3x3 multiplication

- **Upper bound.** Laderman, *Bull. AMS* 82(1):126-128 (1976): rank 23 [ABS]. No scheme with fewer than 23 multiplications is known as of Oct 2026 [SRC, Perminov table].
- **Lower bound.** Bläser, *J. Complexity* 19:43-60 (2003): rank ≥ 19 over arbitrary fields [SNIP].
- **Threshold to beat Strassen.** log₃21 = 2.7712 < log₂7 = 2.8074 < log₃22 = 2.8136, so rank ≤ 21 is needed [DER]. **Confirmed.**

**News 2019-2026:**
- Heule, Kauers & Seidl, arXiv 1903.11391: more than 17,000 inequivalent rank-23 schemes, found by SAT/local search [ABS].
- Lower bounds over **F₂ only**:
  - ≥ 20, Chengu Wang, arXiv 2603.07280 [ABS].
  - ≥ 21, arXiv 2609.06725, with a structural proof in 2609.18722 [SNIP].
- Addition counts for rank 23 are falling fast:

| Additions | Source |
|---|---|
| 60 | arXiv 2508.03857 |
| 58 | Perminov, arXiv 2512.21980 |
| 59 | "arithmetic complexity" without basis change; Mårtensson, Stankovski Wagner & Stapleton, arXiv 2601.05272 |
| 56 | Sun, arXiv 2604.27645 |
| 55 | arXiv 2607.28676 |
| 52 | found over F₂ and lifted to arbitrary rings; arXiv 2609.06588 (author per snippet: H. Møller Nielsen) |

All [ABS] or [SNIP]. They are irrelevant to the exponent, since 2.854 is worse than Strassen.

## 4. 4x4 multiplication

**AlphaTensor.** Fawzi et al., *Nature* 610:47-53 (2022), DOI 10.1038/s41586-022-05172-4. Rank 47 over **Z₂ only** [ABS].

**AlphaEvolve.** DeepMind blog, 14 May 2025; Novikov et al., arXiv 2506.13131. Rank 48 over **ℂ** (complex coefficients). Exponent log₄48 = 2.7925 [ABS/SNIP].

**Dumas, Pernet & Sedoglavic (DPS), arXiv 2506.13242 (June 2025).**
- Rank 48 with **rational** coefficients, valid over any ring containing 1/2. It comes from the complex scheme via an isotropy.
- The alternative-basis variant costs 7·n^(2+log₄3) + o(·) [ABS].

**DPS, "A more accurate rational ... 48 multiplications", arXiv 2603.18699 (2026).**
- Error-bound exponent log₄γ ≈ 2.386.
- Leading constant (387/32)·n^(log₄48) [ABS].

**Dumas, Pernet, Sedoglavic & Tichavský, "Fast matrix multiplication via recursive <4x4x4:48> algorithms into practice", arXiv 2609.12027 (Sep 2026).**
- 48 multiplications plus **216** other operations (additions, subtractions, scalings).
- Leading term 7.75·n^(log₄48), or 6.5 in an alternative basis [ABS].
- Timings: [UNV], since the full text was not accessible.

**Moran, Schwartz & Yuan, "Complex to rational fast matrix multiplication", arXiv 2602.13171, ISSAC 2026.**
- A systematic complex-to-rational method.
- It shows that no real scheme is equivalent to Kaporin's complex <4,4,4;48>, and no rational one to Smirnov's <4,4,9> algorithm [ABS].

**Ternary or integer rank 48?** None known. Perminov's table (repo commit 2026-10-02) lists 4x4x4 as ZT 49, Z 49, Q 48 [SRC]. Every known rank-48 scheme needs 1/2 or complex numbers. Exact coefficient sizes: [UNV].

## 5. Flip graphs and best known ranks

**Papers:**
- Kauers & Moosbauer, "Flip graphs for matrix multiplication", ISSAC 2023, arXiv 2212.01175. New ranks for (4,4,5) and (5,5,5), including 5x5 going from 96 to 95 [ABS].
- Kauers & Moosbauer, "Some new non-commutative ... (n,m,6)", arXiv 2306.00882, ACM CCA (2025) [ABS].
- Arai, Ichikawa & Hukushima, "Adaptive flip graph algorithm", ISSAC 2024, pp. 292-298, arXiv 2312.16960. Ranks (4,5,5) 76→73 and (5,5,5) 95→94 [ABS]; I infer these are Z₂ results.
- Moosbauer & Poole, "Flip graphs with symmetry ...", ISSAC 2025, arXiv 2502.04514. 5x5 in **93** and 6x6 in **153** over arbitrary fields [ABS].
- Kauers & Wood, arXiv 2510.19787: about 30 formats improved [ABS].
- Perminov: arXiv 2511.20317, 2512.13365, 2603.02398 and 2606.02480 (ternary meta flip graphs, addition reduction) [SRC].

**Best known ranks, all dimensions ≤ 6.** Source: github.com/dronperminov/FastMatrixMultiplication README, commit of 2026-10-02 [SRC]. It aggregates the FMM catalogue, AlphaTensor, AlphaEvolve, flip-graph and LITA results. ω = 3·ln r / ln(mkn).

| fmt | r (Q) | r (Z) | ω | | fmt | r (Q) | r (Z) | ω |
|---|---|---|---|---|---|---|---|---|
| 222 | 7 | 7 | 2.8074 | | 335 | 36 | 36 | 2.8241 |
| 223 | 11 | 11 | 2.8950 | | **336** | **40** | 42 | **2.7743** |
| 224 | 14 | 14 | 2.8555 | | 344 | 38 | 38 | 2.8190 |
| 225 | 18 | 18 | 2.8945 | | 345 | 47 | 47 | 2.8211 |
| 226 | 21 | 21 | 2.8739 | | **346** | **54** | 54 | **2.7982** |
| 233 | 15 | 15 | 2.8108 | | 355 | 58 | 58 | 2.8214 |
| 234 | 20 | 20 | 2.8279 | | 356 | 68 | 68 | 2.8131 |
| 235 | 25 | 25 | 2.8392 | | 366 | 80 | 82 | 2.8077 |
| 236 | 30 | 30 | 2.8474 | | **444** | **48** | 49 | **2.7925** |
| 244 | 26 | 26 | 2.8203 | | 445 | 61 | 61 | 2.8144 |
| 245 | 32 | 33 | 2.8185 | | 446 | 73 | 73 | 2.8200 |
| 246 | 39 | 39 | 2.8391 | | 455 | 76 | 76 | 2.8212 |
| 255 | 40 | 40 | 2.8289 | | 456 | 90 | 90 | 2.8197 |
| 256 | 47 | 47 | 2.8211 | | 466 | 105 | 105 | 2.8093 |
| 266 | 56 | 56 | 2.8237 | | 555 | 93 | 93 | 2.8163 |
| 333 | 23 | 23 | 2.8541 | | 556 | 110 | 110 | 2.8143 |
| 334 | 29 | 29 | 2.8190 | | 566 | 130 | 130 | 2.8120 |
| | | | | | 666 | 153 | 153 | 2.8075 |

- **Below log₂7 with all dimensions ≤ 6:** only <3,3,6;40> (Q), <3,4,6;54> and <4,4,4;48> (Q). <6,6,6;153> at 2.80754 just misses.
- **Up to 16** (same table): the smallest exponents are <3,3,6;40> 2.7743, <16,16,16;2237> 2.7818, <6,6,12;280> 2.7856, <9,9,12;600> 2.7896 and <4,4,4;48> 2.7925 [SRC].

## 6. Smirnov

**Paper.** "The bilinear complexity and practical algorithms for matrix multiplication", *Comput. Math. Math. Phys.* 53(12):1781-1795 (2013), DOI 10.1134/S0965542513120129 [ABS].

**Claimed.** <3,3,6;40> gives ω = 3·ln40/ln54 = **2.77430** [SRC/DER]. **Confirmed**, but it needs fractional coefficients; the best integer rank is 42 [SRC].

**Measured.**
- Benson & Ballard implemented <6,3,3;40>. It beat classical on modest sizes (n up to about 13,000), but Strassen was faster in some cases [SNIP].
- An alternative basis cuts its linear operations by about 83% [SNIP].
- **No verified wall-clock win over a tuned Strassen-Winograd was found** [UNV].

## 7. Pan's trilinear aggregation and Kaporin

**Pan.** The 1982 algorithm reaches O(n^2.773372) [ABS, via Schwartz-Zwecher]. Details of Pan 1978 are [UNV].

**Kaporin 1999.** "A practical algorithm for faster matrix multiplication", *Numer. Linear Algebra Appl.* 6(8):687-700 [SNIP]. Measured speedups: [UNV].

**Kaporin 2004.** "The aggregation and cancellation techniques as a practical tool for faster matrix multiplication", *Theor. Comput. Sci.* 315(2-3):469-510.
- *Claimed:* O(N^2.7760). Flops are 4.894·N^2.7760 − 16.165·N² for N = 18·48^k, against Winograd's 3.732·N^2.8074 − 5N² [SNIP].
- *Measured:* tests up to order 7000 in double and single precision. Runtime is "comparable" to Winograd, with clear advantages in workspace and stability [SNIP].
- **Not a time win.**

## 8. Benson & Ballard, PPoPP 2015

"A framework for practical parallel fast matrix multiplication", DOI 10.1145/2688500.2688513, arXiv 1409.2908. Code: github.com/arbenson/fast-matmul [SRC].

**Claimed and measured.**
- Code generation for 20+ algorithms, sequential and shared-memory, with MKL as the base [ABS].
- In parallel: "5% over Strassen and >15% over MKL" [SNIP].
- About 25% over MKL near n ≈ 15,000; 6- and 24-core runs; the best algorithm depends on shape [SNIP].
- The exact best base cases and the CPU model are [UNV].

## 9. BLIS-style Strassen (van de Geijn group)

**Huang, Smith, Henry & van de Geijn, "Strassen's algorithm reloaded", SC16 (arXiv 1605.01078).**
- Additions are fused into the GEMM packing and micro-kernel, with no extra workspace.
- Practical for small matrices and rank-k updates; speedup even on Xeon Phi with 240 threads [ABS].
- "Up to 1.3×" [SNIP]. Crossover sizes: [UNV].

**Huang, Rice, Matthews & van de Geijn, "Generating families of practical FMM algorithms", IPDPS 2017 (arXiv 1611.01120).**
- A code generator for 20+ algorithms, with a performance model.
- A benefit over GEMM on single-core and multi-core systems [ABS]. Numbers: [UNV].

**Huang, Yu & van de Geijn, "Strassen's algorithm reloaded on GPUs", ACM TOMS 46(1) (2020), DOI 10.1145/3372419.**
- One level: up to **1.11×** over cublasSgemm on a V100, crossover **1,536**.
- Two levels: **1.19×**, crossover **7,680** (single precision) [ABS].

## 10. Numerical stability

**Classical bounds.**
- Higham, *ACM TOMS* 16(4):352-368 (1990): fast BLAS3 bounds; Strassen's error grows like (n/n0)^(log₂12) [SNIP].
- Growth factors 12 for Strassen and 18 for Winograd give bound exponents log₂12 ≈ 3.585 and log₂18 ≈ 4.170 [SNIP, quoted in DPS].
- Bini & Lotti, *Numer. Math.* 36:63-72 (1980). Demmel, Dumitriu, Holtz & Kleinberg, "Fast matrix multiplication is stable" (arXiv math/0603207) [titles].

**Ballard, Benson, Druinsky, Lipshitz & Schwartz, SIMAX 37(4):1382-1418 (2016), arXiv 1507.00687.**
- Improves stability by choosing and manipulating algorithms, and by diagonal scaling (outside and inside).
- Concludes the "numerical sacrifice ... is not prohibitive" [ABS].
- Its per-algorithm stability factors are [UNV].

**Correction: authorship.** "Strassen's algorithm is not optimally accurate" is by **Dumas, Pernet & Sedoglavic** (ISSAC 2024, arXiv 2402.05630, DOI 10.1145/3666000.3669697), not Schwartz-Toledo-Vaknin-Wiernik.
- It minimizes the growth factor over the orbit of Strassen's tensor decomposition, giving better measured accuracy and the best leading term [ABS].
- Their repo lists growth factors Winograd 17.85, Strassen 14.83 and DPS variants 12.07-12.20, in the norm used there [SRC: github.com/jgdumas/Fast-Matrix-Multiplication].
- Journal version: *J. Symbolic Comput.* (2026), arXiv 2506.19405 [ABS].

**Schwartz, Toledo, Vaknin & Wiernik.** See §2: optimal error and leading coefficient together for 2x2 base cases [ABS].

**Other 2024-2026 work.**
- Vermeylen & Van Barel, *Numer. Algorithms* (2024), DOI 10.1007/s11075-024-01806-y: augmented-Lagrangian search for decompositions with bounded coefficients [ABS].
- DPS 4x4:48 error exponent ≈ 2.386 (§4) [ABS].
- arXiv 2609.26077 covers fast MM in fp8 [title only].

## 11. Border rank and APA (approximate) schemes

**Bini, Capovani, Lotti & Romani.** "O(n^2.7799) complexity for n×n approximate matrix multiplication", *Inf. Process. Lett.* 8(5):234-235 (1979) [ABS]. The "border rank 10 for <3,2,2>" detail is [SNIP].

**Practical evaluations.**
- Benson & Ballard's framework includes Bini's APA [SRC].
- Ballard, Weissenberger & Zhang, ICPP-W 2021, DOI 10.1145/3458744.3474050:
  - Up to **28%** (1 core) and **21%** (12 cores) faster than classical.
  - MLP training error not significantly affected [ABS].
  - Precision: [UNV].

**Accuracy in FP64.** The standard theory is that an APA scheme of error degree σ reaches about u^(σ/(σ+1)) per level (about √u ≈ 1e-8 for σ = 1) [UNV]. That does **not** meet "accurate in double". Exact use mod p is common: Boyer et al., TOMS 2016, DOI 10.1145/2829947 [SNIP].

## 12. "OpenAI 9/4" and the current ω

**OpenAI.** github.com/openai/math, released 2026-10-06, is a set of 722 manuscripts from an internal model. Entry **107**, "Matrix multiplication with exponent at most 9/4" [SRC], includes:
- "An Upper Bound of 9/4 for the Matrix Multiplication Exponent" (dated Oct 2, 2026): ω ≤ 9/4 over ℂ.
- "Complex Matrix Multiplication Below 2.258 and Rectangular Bounds" (Sep 24): ω < 2.258 in characteristic 0, and ω(1,0.709,1) < 2.092.
- "Staggered extraction ... over every field" (Sep 24): ω < 2.371054886 over every field.

**Verification status.**
- Third-party Lean checks compile with standard axioms only and no `sorry`. They prove `Arithmetic.omega ℂ ≤ 9/4` and α > 93/200 [SRC: github.com/erenciracioglu-dotcom/openai-math-9-4-lean-check].
- Another repo extends ω ≤ 9/4 to all fields [SRC: github.com/selanavot/matrix-multiplication-all-fields].
- **Not peer-reviewed and one day old.** It is purely asymptotic, with no practical algorithm.

**Peer-track ω bounds.**
- ω < 2.371552: Vassilevska Williams, Xu, Xu & Zhou, SODA 2024 [ABS].
- ω < **2.371339**: Alman, Duan, Vassilevska Williams, Xu, Xu & Zhou, "More asymmetry yields faster matrix multiplication", SODA 2025, arXiv 2404.16349 [ABS].
- ω < **2.371177**: "Improving the matrix multiplication exponent with modern optimization and AlphaEvolve", arXiv 2608.16884 (Aug 17, 2026). Includes Alman and Vassilevska Williams; full author list [UNV] [ABS].

Also relevant: Alman & Yu, SODA 2025, arXiv 2410.20538, on leading constants [ABS].

## 13. Claimed practical wins, 2022-2026

**AlphaTensor (2022).**
- Hardware-tuned 4x4-block algorithms for 8192×8192 float32 on V100 and TPU v2. The repo benchmarks against Strassen-square and `jnp.dot` [SRC].
- Median **8.5%** (V100) and **10.3%** (TPU) over standard matmul [SNIP, secondary]. The margin over Strassen-square is [UNV].
- These are GPU/TPU float32 results, not CPU FP64.

**FalconGEMM.** arXiv 2605.06057 (2026).
- **7.59-17.85%** over cuBLAS/CUTLASS/MKL, and 12.41-55.61% over AlphaTensor.
- H20, A100, ARM and x86 hardware; FP32, BF16, FP16 and FP8. **FP64 not mentioned** [ABS/SNIP].

**CPU FP64.**
- Schwartz-Vaknin and SPAA 2026 (§2) are the only recent measured CPU FP64 claims found [ABS].
- An older Xeon Phi study reports 2-level Strassen 14-26% over MKL for N ≥ 1024 [SNIP].
- DPS+Tichavský 4x4:48 "into practice" timings: [UNV].

## 14. Rectangular base cases

All [SRC], Perminov table, valid over Z:
- <2,2,3;11>
- <2,3,3;15>
- <2,3,4;20>
- <3,3,4;29>
- <3,4,4;38>
- <4,4,5;**61**>

All are confirmed. Also <3,4,5;47>, <4,5,5;76> and <5,5,5;93>. Their exponents are 2.82-2.90, all **worse than Strassen**. Their only practical use is shape-matching for non-square or awkward n, as in Benson-Ballard and Huang 2017.

---

Readable primary sources: GitHub repos dronperminov/FastMatrixMultiplication, openai/math (CONTENTS.md #107), erenciracioglu-dotcom/openai-math-9-4-lean-check, selanavot/matrix-multiplication-all-fields, jgdumas/Fast-Matrix-Multiplication, google-deepmind/alphatensor, arbenson/fast-matmul and flame/fmm-gen. Everything else came from search-indexed abstracts (arXiv IDs and DOIs as cited).
