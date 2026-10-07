#!/usr/bin/env python3
"""Cost model for recursive fast matrix multiplication on top of a BLAS (see docs/costmodel.md).

A plan is a comma-separated list of levels, top first, each "<slp-file>@<mode>":
  lean  memory-lean Strassen-Winograd level (src/sw.c, Boyer et al. schedule; slp must be 2x2x2 r=7)
  dfs   generic engine, products one after another, leaves use all threads, passes in parallel
  bfs   generic engine, products as tasks (this and all deeper levels inside the tasks)
Time = leaf GEMMs + fused-pass memory traffic (+ task imbalance), using measured inputs
(results/calib.json from tools/calibrate.py):
  * leaf GEMM time from the measured DGEMM table (4 threads, or 1 thread inside tasks with a
    concurrency derating CONC, because 4 busy cores run slower than one),
  * pass time = bytes moved / measured fused-pass bandwidth (writes counted twice for
    write-allocate),
  * BFS levels: ceil(#leaf tasks / threads) rounds of the per-task time.
"""
import json, math, sys, bisect

CONC = 0.88          # throughput of one core when all four are busy, relative to running alone
LEAN_TRANSFERS = 33  # block transfers (reads+writes, in units of one quarter block) per lean level


def read_slp_header(path):
    """Return (M, K, N, r, nbufA, nbufB) from an .slp file."""
    toks = open(path).read().split()
    it = iter(toks)
    assert next(it) == "dims"
    M, K, N, r = (int(next(it)) for _ in range(4))
    assert next(it) == "prodscale"
    for _ in range(r): next(it)
    nb = []
    for sec in range(3):
        next(it); next(it)
        nin, nins, nout = (int(next(it)) for _ in range(3))
        for _ in range(nins):
            nt = int(next(it))
            for _ in range(2 * nt): next(it)
        outs = [(int(next(it)), float(next(it))) for _ in range(nout)]
        nb.append(sum(1 for v, c in outs if v >= nin or c != 1.0))
    return M, K, N, r, nb[0], nb[1]


def interp_loglog(table, x):
    xs = [p[0] for p in table]
    if x <= xs[0]: return table[0][1]
    if x >= xs[-1]: return table[-1][1]
    i = bisect.bisect_left(xs, x)
    (x0, y0), (x1, y1) = table[i - 1], table[i]
    w = (math.log(x) - math.log(x0)) / (math.log(x1) - math.log(x0))
    return y0 + w * (y1 - y0)


class Model:
    def __init__(self, calib, threads=4):
        c = json.load(open(calib))
        self.T = threads
        # GFLOP/s tables
        self.gf = {t: sorted((n, 2.0 * n ** 3 / s / 1e9) for n, s in v) for t, v in c["gemm"].items()}
        bw = sorted(c["bw"]["4"])
        self.bw = sum(b for _, b in bw) / len(bw) * 1e9  # bytes/s (footprints all exceed caches)

    def gemm(self, m, k, n, threads):
        """threads = 4: multithreaded BLAS; 1: one task while all cores run tasks (table '1c',
        measured with 4 concurrent single-threaded dgemms; falls back to '1' derated by CONC)."""
        if threads == 1 and "1c" in self.gf:
            g = interp_loglog(self.gf["1c"], (m * k * n) ** (1 / 3))
            return 2.0 * m * k * n / (g * 1e9)
        g = interp_loglog(self.gf[str(threads)], (m * k * n) ** (1 / 3))
        t = 2.0 * m * k * n / (g * 1e9)
        return t / CONC if threads == 1 else t

    def predict(self, plan, m, k, n, in_task=False):
        """seconds for one m x k x n product; returns (total, gemm_part, pass_part, ntasks)."""
        if not plan:
            t = self.gemm(m, k, n, 1 if in_task else self.T)
            return t, t, 0.0, 1
        path, mode = plan[0]
        M, K, N, r, nA, nB = read_slp_header(path)
        mb, kb, nb = m // M, k // K, n // N
        if mode == "lean":
            elems = LEAN_TRANSFERS * mb * nb
        else:
            elems = (M * K + 2 * nA) * mb * kb + (K * N + 2 * nB) * kb * nb + (r + 2 * M * N) * mb * nb
        t_pass = 8.0 * elems / self.bw
        if mode == "bfs" and not in_task:
            # every level below runs inside tasks; count leaf tasks of consecutive bfs levels
            ct, cg, cp, ntask = self.predict(plan[1:], mb, kb, nb, in_task=True)
            tasks = r * (ntask if plan[1:] and plan[1][1] == "bfs" else 1)
            per = ct / (ntask if plan[1:] and plan[1][1] == "bfs" else 1)
            # tasks run on T workers; a task's passes (memory-bound) overlap with other tasks'
            # GEMMs, so the makespan is bounded below both by the critical round structure of
            # the GEMM parts and by the total work spread over the workers.
            g_per, p_per = per * (cg / ct), per * (cp / ct)
            rounds = math.ceil(tasks / self.T)
            t_prod = max(rounds * g_per, tasks * (g_per + p_per) / self.T)
            return t_pass + t_prod, rounds * g_per, t_pass + max(0.0, t_prod - rounds * g_per), tasks
        ct, cg, cp, nt = self.predict(plan[1:], mb, kb, nb, in_task)
        if in_task and mode == "bfs":
            return t_pass + r * ct, r * cg, t_pass + r * cp, r * nt
        return t_pass + r * ct, r * cg, t_pass + r * cp, 1


def workspace_bytes(plan, m, k, n):
    """Extra memory (bytes) used by a plan, mirroring sw_workspace / gen_workspace."""
    if not plan: return 0
    path, mode = plan[0]
    M, K, N, r, nA, nB = read_slp_header(path)
    mb, kb, nb = m // M, k // K, n // N
    child = workspace_bytes(plan[1:], mb, kb, nb)
    if mode == "lean":
        return 8 * (mb * max(kb, nb) + kb * nb + mb * nb) + child
    own = 8 * (nA * mb * kb + nB * kb * nb + r * mb * nb)
    tasks_here = mode == "bfs" and not any(md == "bfs" for _, md in [])
    return own + (r if mode == "bfs" else 1) * child


def parse_plan(s):
    if s in ("dgemm", ""): return []
    return [tuple(x.split("@")) for x in s.split(",")]


def main():
    calib, n = sys.argv[1], int(sys.argv[2])
    mdl = Model(calib)
    base = mdl.gemm(n, n, n, 4)
    print(f"n={n}  dgemm predicted {base:.4f}s")
    for p in sys.argv[3:]:
        t, g, ps, _ = mdl.predict(parse_plan(p), n, n, n)
        print(f"  {p:60s} {t:8.4f}s  speedup {base / t:6.3f}   (gemm {g:.3f}  passes {ps:.3f})")


if __name__ == "__main__":
    main()
