#!/usr/bin/env python3
"""Summary of results/sweep_final.txt: paired speedup over MKL (median of per-round ratios, IQR)
for the best Strassen-type plan and each emulation variant, one row per size."""
import re, sys, collections
res = collections.defaultdict(dict)   # n -> method -> (paired, lo, hi)
mkl = collections.defaultdict(list)   # n -> MKL GFLOP/s of each session
for line in open(sys.argv[1]):
    if not line.startswith("time "): continue
    kv = dict(re.findall(r"(\w+)=(\S+)", line))
    n, m = int(kv["n"]), kv["method"]
    if m == "dgemm": mkl[n].append(float(kv["eff_gflops"])); continue
    lo, hi = re.search(r"IQR ([\d.]+)-([\d.]+)", line).groups()
    res[n][m] = (float(kv["paired"]), float(lo), float(hi))
short = lambda m: (m.replace("slp/", "").replace(".slp", "").replace("winograd-squared", "W2").replace("winograd", "W1")
                   .replace("4x4x4_r48_plinopt-204", "P48").replace(":1:0:1", " task-par.").replace("sw1+g:", "lean+"))
oz = ["oz14", "ozw14", "oz16", "ozw16", "ozc16"]
print("| n | MKL GFLOP/s | best Strassen-type plan | its speedup | " + " | ".join(oz) + " |")
print("|---|---|---|---|" + "---|" * len(oz))
for n in sorted(res):
    fast = {m: v for m, v in res[n].items() if m not in oz}
    bm = max(fast, key=lambda m: fast[m][0]) if fast else None
    cells = [f"{res[n][m][0]:.2f} [{res[n][m][1]:.2f}-{res[n][m][2]:.2f}]" if m in res[n] else "" for m in oz]
    bs = f"{fast[bm][0]:.2f} [{fast[bm][1]:.2f}-{fast[bm][2]:.2f}]" if bm else ""
    print(f"| {n} | {sum(mkl[n]) / len(mkl[n]):.0f} | {short(bm) if bm else ''} | {bs} | " + " | ".join(cells) + " |")
print("\nAll Strassen-type plans measured:\n")
for n in sorted(res):
    fast = {m: v for m, v in res[n].items() if m not in oz}
    print(f"* n = {n}: " + ", ".join(f"{short(m)} {v[0]:.2f}" for m, v in fast.items()))
