# Independent review of fastmm (2026-10-07)

Test code is in `review/`: `oztest.c` (direct oz_dgemm/ozw/ozf driver), `speed.c` (timing plus a textbook Strassen-Winograd), `refcheck.c/.py`, and `verify_schemes.py`. The C files build with the Makefile's `fmmtest` flags plus `-Isrc`, linked against `src/{ozaki,ozfmm,ozw,amx,gen}.c` (and `sw.c` for `speed.c`). Every run used `OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 OMP_PROC_BIND=close`, one at a time.

## Confirmed

* **Scheme exactness.** I wrote my own coefficient parser (including Gaussian rationals), tensor loop, exact random evaluation and SLP interpreter. `winograd`, `4x4x4_r48_plinopt-204`, `3x3x6_r40_tichavsky_kovac`, `4x4x4_r48_alphaevolve` (complex) and the SLPs `winograd`, `winograd-squared`, `plinopt-204` and `3x3x6_r40` are all exact. The checker rejects deliberately corrupted inputs.
* **Shapes and leading dimensions** (`oztest shapes`). 747 problems × 7 methods (dgemm, oz/ozw/ozf with 14 and 16 moduli).
  * m and n range over 1-257, including 15, 17, 33, 63-65 and 127-129. k takes the values 1, 2, 63-65, 127-129 and 513.
  * lda > m, ldb > k and ldc > m, with NaN in the padding of A and B and a sentinel in the padding of C.
  * Thin shapes up to k = 140001 (the split path), and the ozw thresholds 64×128×64, 63/127 variants and k = 65999/66000/66001.
  * Result: no errors, no reads or writes of the padding, and ozw/ozf bit-identical to oz.
* **Value classes that pass:** zero rows and columns; 1e±300; subnormals; rows and columns scaled by 2^±500; integers; exact cancellation; negative zeros; NaN and Inf, including Inf·0 (the fallback matches DGEMM).
* **The range argument.**
  * Every s = 2..16 stays under my predicted error bound (worst: 0.42 of the bound). This includes B = ±Aᵀ with row norms 2^e(1−2^-28), the Cauchy-Schwarz equality case.
  * `resid_split`: 1.3M bit-exact simulations with `math.fma`, about 10k of them at the minimal 1/(2p) distance from a rounding boundary. All are exact and in int8 range.
  * `oz_consts.h` regenerates identically, and an exact simulation of `reconstruct()` on 100k values is correct.
  * int32 range: 127²·131071 < 2^31; the p = 256 wraparound is harmless; for ozw, 254²·33000 < 2^31.
* **Bit identity holds under memory blocking** (`OZ_MEM_GB=1`, about 8200×200×8200, row and column blocks) and for ozf with the W2 and 3x3x3 schemes on odd shapes, with the exception in problem 5.
* **The reference is accurate.** `ref_entry` agrees with exact Fractions to within 9e-31·|A||B| on 160 entries from 4 classes. The error metric does not flatter any method.
* **Speed reproduces** (paired median vs MKL):

  | n | oz14 | oz16 | ozw14 | best Strassen-type |
  |---|---|---|---|---|
  | 4000 | 1.35 (another session: 1.20) | 1.12 | – | W2 1.08 |
  | 8000 | 1.45 | 1.26 | 1.50 | W2 1.15, sw1 1.08 |
  | 12000 | 1.44 | 1.30 | – | – |

* **The baselines are fair.**
  * The Strassen baselines are not weak. My textbook one-level Strassen-Winograd on MKL is no better than the project's `sw1`: 0.84 vs 0.80 at n = 4000, and 1.08 vs 1.08 at n = 8000.
  * MKL is not handicapped. It runs at 286-296 GF/s at n = 8000 with OMP_PROC_BIND close, false or spread, with MKL_DYNAMIC=FALSE, and with the Intel OpenMP layer.
  * Interleaving does not slow MKL (alone 0.470 s, with oz14 0.453 s at n = 4000), and a duplicated DGEMM gives 0.985-1.016, so there is no position bias.
* **oz14 beats the two-level Strassen-type plans even with all entries checked.** At n = 2000 with random exponents 2^±10/48/64: oz14 gives 2.0e-15/9.0e-13/9.6e-12, against 1.0e-14/3.1-3.8e-12/3.9-5.3e-11 for sw2, W2 and P48.
* **The cold-call section is honest.** My repeats of the oz14 first call took 5.19 s and 2.76 s (MKL: 3.73 s).

## Problems found

**1. High: the accuracy "exception at small n" is an artifact of sampling.** README §6 and verdict item 3 say that with 16 moduli the error is "about DGEMM's (0.6-0.7x)" for random exponents 2^±48 at n = 2000 and 4000, and 0.7x for 2^±64 at n = 4000. Those numbers come from 20000 sampled entries, which is 0.5% of C at n = 2000 and 0.125% at n = 4000. I re-ran with all entries (`fmmtest acc 4 r n n n 0 …`):

| n | r | DGEMM | oz16 | oz16/DGEMM (README) | oz14 |
|---|---|---|---|---|---|
| 2000 | 48 | 1.85e-15 | 7.7e-15 | **4.2x** (0.7x) | 9.0e-13 |
| 2000 | 64 | 1.48e-15 | 9.0e-14 | **61x** (10x) | 9.6e-12 |
| 4000 | 48 | 1.67e-15 | 2.3e-15 | **1.4x** (0.6x) | 2.1e-13 |
| 4000 | 64 | 1.62e-15 | 9.1e-15 | **5.6x** (0.7x) | 1.2e-12 |

* Sampling underestimates the emulation's heavy-tailed maximum by 4-13x, but DGEMM's by only 1.4-1.8x, so it systematically favours the emulation.
* For random exponents of 2^±48 or wider, 16 moduli are worse than DGEMM componentwise at every tested n.
* 2^±32 at n = 2000 still holds: 7.7e-16 against DGEMM's 2.2e-15.

**2. Medium: inner scaling overflows and silently drops columns.**
* `oz_internal.h:131` computes `lround(0.25*log2(rb/ca))`. When ‖B_p,:‖/‖A_:,p‖ exceeds 2^512, the ratio overflows to inf or 0, `lround` returns LONG_MIN, and column p is lost.
* Reproduced with the project's own driver, `fmmtest acc 3 R 500 500 500 0 dgemm oz14 oz16 ozw16`. All emulation variants give:
  * R = 250: 1.6e-17 (fine)
  * R = 256: 8.5e-3
  * R = 300: 9.2e-2
  * R = 500: 0.20
* DGEMM stays at 2.7e-16. This is the one robustness feature the README sells against LIBXS.
* Fix: use `0.25*(log2 rb − log2 ca)`.

**3. Medium: rows or columns that underflow are zeroed.**
* Row norms are computed on A·2^ek (`ozaki.c:133`, `oz_internal.h:93`). If every entry of a row underflows there, `safe_norm_exp_sc` returns `L_ZERO_ROW` (`oz_internal.h:95`) and the row is treated as zero.
* Minimal case (`oztest underflow`): A = [1e150; 1e-300], B = [1e100]. DGEMM gives [1e250, 1e-200]; oz16 gives [1e250, **0**]. The result is correct with `OZ_INNER=0`.
* A 150×301×130 version gives a componentwise error of 0.18 for every oz variant.
* Problems 2 and 3 contradict §5: "the only rounding errors are the rounding of the scaled inputs…".

**4. Medium: speed depends on free memory, and the tables do not show this.**
* At n = 8000, oz14 runs at 1.22x MKL with `OZ_MEM_GB=1.5`, 0.68x with 0.6, and 0.61x with 0.25 (the blocks collapse to H = 256).
* In `results/sweep_oz_final.txt`, the first n = 20000 runs (oz14 1.00, oz16 0.88, ozw16 0.78) were replaced by reruns after code changes, and the "final code" table mixes three code versions (its 16000 row predates the blocking and cgroup fixes).
* "Beats MKL up to 20000" therefore assumes roughly 8 GB of free workspace.

**5. Low: results are nondeterministic, and bit identity can break.**
* `rb` is summed in an `omp critical` section in thread-arrival order (`oz_internal.h:126-127`). When rb/ca sits exactly on a rounding boundary of `lround`, ek flips.
* Constructed case (`oztest nondet`): oz14 differed from its own first run in 135 of 400 runs, and from ozw14 in 80 of 200 paired runs (1 ulp, 1 entry).
* The effect disappears with `OZ_INNER=0` or with one thread.
* "Bit-identical" therefore holds only for identical scaling, and neither variant is reproducible from run to run.

**6. Low: the n = 1500 claim is within noise.**
* Verdict: "beats both MKL and the best Strassen-type plan from n = 1500". The project's own IQR vs MKL at 1500 is 0.907-1.212.
* In my two sessions oz14 was 1.05-1.08, while the duplicated DGEMM spanned 0.93-1.15. By ratio of medians, W1 beats oz14 at 1500 (1.127 vs 1.103).
* The choice of statistic moves small-n results by up to about 10% (n = 4000: paired median 1.35, ratio of minimum times 1.22).

**7. Low: wording and design details.**
* The CRT low part adds up to 1.7e-25·P (negligible), and k > 131071 adds a rounding per chunk, so "only the final rounding" is not literally true.
* The range uses P/4 where P/2 would suffice, wasting one bit of L at s = 15 and 16.
* The code is not reentrant (`ozaki.c:31`, `ozaki.c:45`), and the cold timings are single samples.

## Could not check

* n > 12000; the OpenBLAS, BLIS and LIBXS numbers; the literature and novelty claims; CPUs without AMX.
* Full-entry errors for classes other than random exponents, and at n = 4000 for the Strassen-type plans.
