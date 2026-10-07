# Cost model for recursive fast matrix multiplication on top of a BLAS

Code: `tools/costmodel.py` (model), `tools/calibrate.py` (inputs), `tools/validate_model.py`
(check against measurements), `tools/rank_plans.py` (search over the scheme library).

## What is modelled

A *plan* is a stack of recursion levels. Each level uses a bilinear base case <M,K,N;r>, given as a
straight-line program (SLP) produced from its (U,V,W) by `tools/slp.py` (exact verification +
greedy pairwise common-subexpression elimination). A level runs in one of three modes:

* `lean`: the memory-lean Strassen-Winograd schedule of `src/sw.c` (Boyer et al. 2009 order,
  2 temporaries, post-additions fused into one pass, two products accumulated through BLAS `beta`);
* `dfs`: generic engine (`src/gen.c`), one fused pass over the A blocks writes all non-trivial left
  factors, one over B the right factors, the r products run one after another with the
  multithreaded BLAS, one fused pass over the r products writes the C blocks;
* `bfs`: same passes, but the r products are OpenMP tasks with single-threaded BLAS leaves; deeper
  levels run inside the tasks.

Predicted time of one level for an m x k x n product with blocks mb x kb x nb:

    T = T_pass + T_products
    T_pass = 8 bytes * E / BW
    E (dfs/bfs) = (M*K + 2*nA)*mb*kb + (K*N + 2*nB)*kb*nb + (r + 2*M*N)*mb*nb
    E (lean)    = 33 * mb*nb            (block transfers of the lean schedule)
    T_products (dfs)  = r * T_child(threads = 4)
    T_products (bfs)  = max( ceil(tasks/4) * t_gemm_task , tasks * (t_gemm_task + t_pass_task) / 4 )

* nA, nB = number of left / right factors that are not a single input block (they need a buffer);
  writes are counted twice (write-allocate), reads once.
* Leaf times come from the measured MKL DGEMM table (`gemm["4"]`: 4 threads; `gemm["1c"]`: one
  single-threaded dgemm while three others run concurrently, i.e. the real situation of a BFS leaf),
  interpolated in log-log on the geometric mean size (mkn)^(1/3).
* BW is the measured streaming bandwidth of the fused passes (~30-65 GB/s; mean of the calibration
  points ~41 GB/s).
* `workspace_bytes()` gives the extra memory of a plan; plans that do not fit in 14 GB (with A, B,
  C) are marked infeasible.

Arithmetic in the passes (number of additions after CSE) is *not* in the model: at ~41 GB/s
and ~32 flop/cycle/core the passes are memory-bound by a wide margin, so the number of buffers and
of products matters, not the number of additions. This was checked: the rational <4,4,4;48>
scheme (308 additions after CSE) runs its passes as fast as Winograd squared (167 additions).

## Validation (results/model_validation.md)

30 plans x sizes 2000, 4000, 8000, measured interleaved in one session, 7 rounds each:

* mean |error| 9.4 %, max 25 %;
* depth-first and lean plans: within 8 % (most within 6 %);
* task-parallel (BFS) plans with 49 tasks are over-predicted by 6-25 % (the model is
  pessimistic for them). Plausible causes: the concurrency table was measured with four
  *synchronised* equal dgemms, while BFS tasks are desynchronised; passes of one task overlap
  GEMMs of others more than the `max(...)` term assumes.

Run-to-run noise of the machine itself is of the same order (MKL at n = 2000 varies between 150
and 240 GFLOP/s across runs; at n >= 4000 within ~3 %). The model is therefore good enough to
rank plans that differ by more than ~10 %, not to separate plans closer than that.

## What the model says (results/plan_ranking.md)

1. The multiplication count and the leaf BLAS efficiency dominate. On this machine MKL runs at
   ~200-240 GFLOP/s for 1000 <= n <= 3000 and ~280 GFLOP/s for n >= 4000 (4 cores). One
   Strassen level saves 12.5 % of the flops but moves the work to sizes where MKL is up to 25 %
   less efficient, so it cannot pay off until the leaves are >= 4000 (n >= 8000), or until the
   leaves run as concurrent single-threaded tasks (~250-265 GFLOP/s aggregate), which recovers
   most but not all of the efficiency.
2. Pass traffic is second order: a fused <4,4,4;49> level moves ~13 n^2 doubles, i.e.
   ~0.2 s at n = 8000 (6 % of the run); two lean levels ~0.3 s.
3. Better-exponent base cases help little at n <= 20000. Feasible plans with <4,4,4;48> or
   <3,3,6;40> below one lean Winograd level are predicted at 1.20-1.23x at n = 12000-16000,
   versus 1.10-1.24x for Winograd plans; the best-exponent single levels (<3,3,6;40>,
   <4,4,4;48>, <5,5,5;93>) are predicted 1.29-1.32x at n = 20000 but need 18-30 GB of buffers in
   the fused engine (infeasible here). A memory-lean schedule for them (as exists for Winograd)
   would be required.
4. Sensitivity: removing *all* operand buffers of a <4,4,4;48> level (nA = nB = 0, impossible)
   would gain < 4 % at n = 8000. A search for sparser or buffer-free variants of known ranks
   (flip graphs, change of basis) therefore cannot change the conclusion on this hardware; it was
   not pursued beyond the library comparison.
