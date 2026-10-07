#!/usr/bin/env python3
"""Compare cost-model predictions with interleaved measurements (fmmtest time) for a set of plans.
usage: validate_model.py calib.json reps n [n ...]   -> markdown table on stdout"""
import re, subprocess, sys, os
sys.path.insert(0, os.path.dirname(__file__))
from costmodel import Model, parse_plan

W1, W2, P48 = "slp/winograd.slp", "slp/winograd-squared.slp", "slp/4x4x4_r48_plinopt-204.slp"
PLANS = [  # (cost-model plan, fmmtest method)
    ("dgemm", "dgemm"),
    (f"{W1}@lean", "sw1"),
    (f"{W1}@lean,{W1}@lean", "sw2"),
    (f"{W1}@dfs", f"g:{W1}:1:1:0"),
    (f"{W1}@bfs", f"g:{W1}:1:0:1"),
    (f"{W2}@dfs", f"g:{W2}:1:1:0"),
    (f"{W2}@bfs", f"g:{W2}:1:0:1"),
    (f"{P48}@bfs", f"g:{P48}:1:0:1"),
    (f"{W1}@lean,{W1}@bfs", f"sw1+g:{W1}:1:0:1"),
    (f"{W1}@lean,{W2}@bfs", f"sw1+g:{W2}:1:0:1"),
]
calib, reps, sizes = sys.argv[1], sys.argv[2], [int(x) for x in sys.argv[3:]]
mdl = Model(calib)
env = dict(os.environ, OMP_NUM_THREADS="4", MKL_NUM_THREADS="4", OMP_PROC_BIND="close")
print("| n | plan | predicted s | predicted speedup | measured s (median) | measured speedup (paired) | error |")
print("|---|---|---|---|---|---|---|")
for n in sizes:
    plans = PLANS if n <= 8000 else [p for p in PLANS if "bfs" not in p[0] or "lean" in p[0]]
    out = subprocess.run(["./bin/fmmtest", "time", reps, str(n), str(n), str(n)] + [m for _, m in plans],
                         env=env, capture_output=True, text=True).stdout
    meas = {}
    for line in out.splitlines():
        kv = dict(re.findall(r"(\w+)=(\S+)", line))
        if "method" in kv: meas[kv["method"]] = (float(kv["median"]), float(kv["paired"]))
    base = mdl.gemm(n, n, n, 4)
    for p, m in plans:
        t = mdl.predict(parse_plan(p), n, n, n)[0] if p != "dgemm" else base
        mt, ms = meas.get(m, (float("nan"), float("nan")))
        print(f"| {n} | `{m}` | {t:.3f} | {base / t:.3f} | {mt:.3f} | {ms:.3f} | {100 * (t - mt) / mt:+.0f}% |")
    sys.stdout.flush()
