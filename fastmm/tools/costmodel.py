#!/usr/bin/env python3
"""Cost model for recursive fast matrix multiplication on top of a BLAS.

A *plan* is a list of levels, top first; each level is (slp_file, mode) with mode in
  'dfs'  products one after another, leaves use all T threads, passes parallel;
  'bfs'  products spawned as tasks (leaves single-threaded), passes parallel at this level;
  'ser'  inside a task: everything serial.
Predicted time = leaf GEMM time (from a measured DGEMM table) + fused-pass time
(bytes moved / measured streaming bandwidth) + task-scheduling imbalance.

Calibration file (JSON, produced by bench/calibrate.sh):
  {"gemm": {"4": [[n, seconds], ...], "1": [[n, seconds], ...]},
   "bw":   {"4": [[bytes_footprint, GB/s], ...], "1": [...]}}
"""
import json, math, sys, bisect


def read_slp_header(path):
    """Return (M, K, N, r, nbufA, nbufB) from an .slp file."""
    with open(path) as f:
        toks = f.read().split()
    it = iter(toks)
    assert next(it) == "dims"
    M, K, N, r = (int(next(it)) for _ in range(4))
    assert next(it) == "prodscale"
    for _ in range(r): next(it)
    nb = []
    for sec in range(3):
        next(it); next(it)  # "slp X"
        nin, nins, nout = (int(next(it)) for _ in range(3))
        for _ in range(nins):
            nt = int(next(it))
            for _ in range(2 * nt): next(it)
        outs = [(int(next(it)), float(next(it))) for _ in range(nout)]
        nb.append(sum(1 for v, c in outs if v >= nin or c != 1.0))
    return M, K, N, r, nb[0], nb[1]


class Model:
    def __init__(self, calib):
        c = json.load(open(calib))
        self.gemm = {int(t): sorted(v) for t, v in c["gemm"].items()}
        self.bw = {int(t): sorted(v) for t, v in c["bw"].items()}

    @staticmethod
    def _interp(table, x, logy=False):
        xs = [p[0] for p in table]
        i = bisect.bisect_left(xs, x)
        if i <= 0: return table[0][1], table[0][0]
        if i >= len(xs): return table[-1][1], table[-1][0]
        (x0, y0), (x1, y1) = table[i - 1], table[i]
        w = (math.log(x) - math.log(x0)) / (math.log(x1) - math.log(x0))
        return y0 + w * (y1 - y0), None

    def gemm_time(self, m, k, n, threads):
        """Seconds for an m x k x n dgemm: interpolate the measured GFLOP/s at size (mkn)^(1/3)."""
        tab = [(sz, 2.0 * sz ** 3 / t / 1e9) for sz, t in self.gemm[threads]]  # (n, GFLOP/s)
        g, _ = self._interp(tab, (m * k * n) ** (1.0 / 3))
        return 2.0 * m * k * n / (g * 1e9)

    def bandwidth(self, footprint_bytes, threads):
        g, _ = self._interp(self.bw[threads], footprint_bytes)
        return g * 1e9

    def predict(self, plan, m, k, n, threads=4, in_task=False):
        """Return (seconds, breakdown dict) for computing an m x k x n product with `plan`."""
        if not plan:
            return self.gemm_time(m, k, n, 1 if in_task else threads), {"gemm": None}
        path, mode = plan[0]
        M, K, N, r, nA, nB = read_slp_header(path)
        mb, kb, nb = m // M, k // K, n // N
        # bytes of the three fused passes (writes counted twice: write-allocate)
        elems = (M * K + 2 * nA) * mb * kb + (K * N + 2 * nB) * kb * nb + (r + 2 * M * N) * mb * nb
        footprint = 8 * (m * k + k * n + m * n + nA * mb * kb + nB * kb * nb + r * mb * nb)
        pt = threads if not in_task else 1
        t_pass = 8 * elems / self.bandwidth(footprint, pt)
        if mode == "bfs" and not in_task:
            # products as tasks on `threads` workers; children serial inside the task
            t_child, _ = self.predict(plan[1:], mb, kb, nb, threads, in_task=True)
            # leaf tasks: r^(number of further bfs levels) - here children are serial, so r tasks
            rounds = math.ceil(r / threads)
            t_prod = rounds * t_child
            # passes inside tasks run concurrently: they are part of t_child already
        else:
            t_child, _ = self.predict(plan[1:], mb, kb, nb, threads, in_task)
            t_prod = r * t_child
        return t_pass + t_prod, {"pass": t_pass, "prod": t_prod}


def main():
    calib, n = sys.argv[1], int(sys.argv[2])
    plans = sys.argv[3:]
    mdl = Model(calib)
    base = mdl.gemm_time(n, n, n, 4)
    print(f"n={n} dgemm predicted {base:.4f}s")
    for p in plans:
        plan = [tuple(x.split("@")) for x in p.split(",")] if p != "dgemm" else []
        t, b = mdl.predict(plan, n, n, n)
        print(f"  {p}: {t:.4f}s  speedup {base / t:.3f}  {b}")


if __name__ == "__main__":
    main()
