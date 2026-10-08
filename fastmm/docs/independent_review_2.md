# Independent review 2: the certified mode (`ozc16`), 2026-10-08

**Claim attacked** (README §5): every entry is certified to (theta+1)u·sum|a||b|, recomputed with Dot2, or computed by DGEMM. "No input can make it less accurate than DGEMM."

**Setup**
* Test code is in `review2/`, built with the fmmtest flags (`review2/build.sh`).
* The drivers `#include src/ozaki.c`. This intercepts its `dgemm_nn` calls, so DGEMM-written entries (tile, block, NaN fallback) are marked and every other entry is held to the bound. It also reaches static functions.
* Runs used `OMP_NUM_THREADS=2` and the portable int8 kernel (no AMX).
* Every entry is checked against a double-double reference; the key counterexample also with exact rationals.

## Confirmed

* **E is derived correctly.** Each rounding error is at most 1/2: `scalef` is exact, values >= 2^53 are integers, and values that land subnormal are below 1/2. So |sum xy - sum AiBi| <= n1A/2 + n1B/2 + k/4. n1 is summed from the same doubles that feed `split32` (`ozaki.c:208`, `:114`); the q factor and the padding do not matter. The range bound (2^L + sqrt(k)/2)^2 <= P/4 holds.
* **LB <= sum|a||b|.**
  * q <= 127, because mx comes from the same exactly scaled values.
  * Underflow only touches values below 1 (floor 0).
  * The integer range is safe: 127²·131071 < 2^31.
* **The safety factor 1+2^-30 covers** the n1 summation ((k-1)u <= 2^-36) and the E and right-hand-side roundings.
* **Indexing** (transposed GEMM, flags layout, tile counters, recompute loops) is consistent.
  * Tested with memory blocking (`OZ_MEM_GB` 0.0002-0.01, up to 64 blocks), NaN-padded lda/ldb, untouched ldc padding, m or n = 1, k = 0, 1, 2, and odd shapes.
  * Dot2, tile DGEMM and block fallback all occur in these runs.
* **No violation** in:
  * all 8 classes with r up to 100 (about 400³);
  * checker, decay and random exponents near the threshold at 1500³ (2^±10/12 and 2^±300 scalings at 700³), with and without blocking;
  * integers, zero rows and columns, quantisation boundaries, theta = 3, 0.3 and 1e-3, subnormal inputs;
  * overflowing products (never Inf where the true value is finite);
  * NaN and Inf (whole result from DGEMM, same pattern).
* **The bound is sound and nearly attainable** (`t_adv near`, k = 16384).
  * Construction: a checker design whose scaled small entries are even + 1/2, so every rounding error is +1/2 and lines up with a large entry of the other operand. LB is exact.
  * Result: everything certifies, with a maximum error of **4.45u**.
* **The portable kernel is exact** (`t_kernel`).
  * It equals a naive mod-2^32 product for s8s8 and u8u8, on 7 odd padded shapes, with random and extreme bytes, and with 800 entries forced to wrap.
  * That is what TDPBSSD/TDPBUUD compute (dword sums of byte products, no saturation). The kernel's `vpmaddwd` cannot saturate for |x| <= 255.

## Problems found

**1. Medium: the guarantee is false for k > 131071.**
* `ozaki.c:399-406` certifies each chunk, then adds the chunk results in double. Those roundings are never certified.
* `review2/t_adv split 2 32` (k = 262142):
  * All 1024 entries are certified (no Dot2, no DGEMM).
  * **8 entries exceed 5u, maximum 5.274u.** Exact rationals give 5.2742u for entry (24,0); DGEMM there has 1.69u.
* With 4 chunks: 12 entries exceed 5u, maximum 5.59u.
* Fix: certify against (theta - number of chunks)·u, or add the chunks in double-double.

**2. Medium: "never less accurate than DGEMM on any input" is false** (README:35-36, :218, :335, :427).
* `t_adv exact` (16×16384×16): the large entries are powers of two and the small entries have half-integer tails that cancel in DGEMM's sum.
  * **DGEMM is exact.**
  * ozc16 certifies everything and has an error of **3.2u|A||B|**. Example entry (0,0): exact 1280.0000009536743, ozc16 1280.0000009536739.
* In the split test the matrix maximum is 5.27u for ozc16 against 4.19u for DGEMM.
* On ordinary data ozc16 is worse than DGEMM entry by entry on many entries: 31338 for decay 2^-8 at 1500³.
* Only the bound comparison holds: 5u <= gamma_k for 5 <= k <= 131071.

**3. Low: E's CRT term is too small by a factor of 2.**
* E uses 2^(2L-80) = 2^42 (`ozaki.c:478`, README:322). The rounding of fl(M+L) in `reconstruct` (|M| < 2^96.84) reaches **2^43**.
* `review2/crt` uses a bit-exact scalar replica: 97381 of 2.4M integers exceed u|c| + 2^42, by up to 2x. README:288's own figure, 1.7e-25·P = 2^43.1, agrees.
* The safety factor covers only about 2^29. The gap (at most 2^-17·E) matters only when the true error is that close to the worst case. My best designs reach about 0.93·E at theta = 4, so I observed no violation from this.
* Fix: use 2^(2L-78).

**4. Low: there is no underflow term.**
* When the output is subnormal, the final `scalef` adds an absolute error of 2^-1075, yet the entries are certified.
* Inputs around 2^-520 (`t_misc`): **86911 of 87000 certified entries exceed 5u, maximum 4872u.**
* DGEMM is worse here (1.6e5u).

**5. Low: wording.**
* (theta+1)u holds only to first order. The code says so (`ozaki.c:283`); the README does not.
* "Below DGEMM's worst-case bound" fails for k <= 4.
* Item 3 means "rigorous" is not literally true.

**6. Low: cost only.**
* A few outlier rows make the inner scaling turn a certifiable matrix into checker structure (`t_inner`: 4 large-at-even-p rows of A and 4 large-at-odd-p columns of B, 600×300×600).
  * With the default inner scaling: 355216 of 360000 entries fail and the whole block goes to DGEMM.
  * With `OZ_INNER=0`: only the 16 intended entries fail, and Dot2 repairs them.
* NaN/Inf inputs still run the certificate and Dot2 before the final DGEMM.

## Could not check

* **The AMX hardware path.** Bit identity with the portable kernel is argued from the instruction semantics only.
* **Timing claims.**
* **Larger sizes:** full-entry checks beyond 1500³, and k > 524284.
* **Thread counts:** whether the certification decisions are independent of the thread count.
* **Item 3 at theta = 4:** whether any input with k <= 131071 can exceed 5u through it.
