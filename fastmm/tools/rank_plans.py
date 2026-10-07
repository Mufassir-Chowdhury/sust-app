#!/usr/bin/env python3
"""Rank plans built from every scheme in slp/ with the cost model (square n x n x n)."""
import glob, os, sys
sys.path.insert(0, os.path.dirname(__file__))
from costmodel import Model, parse_plan, read_slp_header, workspace_bytes
RAM = 14.0e9  # usable bytes on this machine (15 GB total)
mdl = Model(sys.argv[1])
sizes = [int(x) for x in sys.argv[2:]] or [2000, 4000, 8000, 12000, 16000, 20000]
W1 = "slp/winograd.slp"
slps = sorted(p for p in glob.glob("slp/*.slp") if "squared" not in p or "winograd-squared" in p)
plans = ["dgemm", f"{W1}@lean", f"{W1}@lean,{W1}@lean", f"{W1}@lean,{W1}@lean,{W1}@lean"]
for p in slps:
    plans += [f"{p}@dfs", f"{p}@bfs", f"{W1}@lean,{p}@bfs", f"{W1}@lean,{p}@dfs"]
res = {}
for n in sizes:
    base = mdl.gemm(n, n, n, 4)
    for p in plans:
        try:
            t = base if p == "dgemm" else mdl.predict(parse_plan(p), n, n, n)[0]
            mem = 24.0 * n * n + (0 if p == "dgemm" else workspace_bytes(parse_plan(p), n, n, n))
        except Exception as e:
            continue
        res.setdefault(p, {})[n] = base / t if mem <= RAM else -base / t  # negative: does not fit
def info(p):
    if p == "dgemm": return ""
    last = parse_plan(p)[-1][0]
    M, K, N, r, nA, nB = read_slp_header(last)
    return f"<{M},{K},{N};{r}> bufs {nA}/{nB}"
rows = sorted(res.items(), key=lambda kv: -max(kv[1].values()))
fmt = lambda v: f"{v:.3f}" if v > 0 else f"({-v:.3f})"
print("Predicted speedup over MKL dgemm (4 threads). Values in parentheses: plan does not fit in 14 GB.\n")
print("| plan | base case | " + " | ".join(f"n={n}" for n in sizes) + " |")
print("|---|---|" + "---|" * len(sizes))
for p, d in rows[:40]:
    print(f"| `{p.replace('slp/', '').replace('.slp', '')}` | {info(p)} | " + " | ".join(fmt(d.get(n, 0)) for n in sizes) + " |")
