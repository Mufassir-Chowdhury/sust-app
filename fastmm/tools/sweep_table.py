#!/usr/bin/env python3
"""Summarise results/sweep.txt: per size, MKL time and the paired speedup of every method."""
import re, sys, collections
rows = collections.defaultdict(dict)   # n -> method -> (median s, paired, IQR lo, hi)
libs = collections.defaultdict(dict)   # n -> lib -> GFLOP/s
for line in open(sys.argv[1]):
    if line.startswith(("openblas", "blis")):
        lib = line.split()[0]
        m = re.search(r"n=(\d+) .*median=([\d.]+)", line)
        n = int(m.group(1)); libs[n][lib] = 2 * n ** 3 / float(m.group(2)) / 1e9
        continue
    if not line.startswith("time "): continue
    kv = dict(re.findall(r"(\w+)=(\S+)", line))
    n = int(kv["n"]); meth = kv["method"]
    iqr = re.search(r"IQR ([\d.]+)-([\d.]+)", line)
    val = (float(kv["median"]), float(kv["paired"]), float(iqr.group(1)), float(iqr.group(2)))
    if meth == "dgemm":
        rows[n].setdefault("dgemm_all", []).append(float(kv["median"]))
    else:
        rows[n][meth] = val
short = lambda m: (m.replace("slp/", "").replace(".slp", "").replace("winograd-squared", "W2").replace("winograd", "W1")
                   .replace("4x4x4_r48_plinopt-204", "P48").replace(":1:0:1", "[bfs]"))
meths = []
for n in sorted(rows):
    for m in rows[n]:
        if m != "dgemm_all" and m not in meths: meths.append(m)
print("Paired speedup over MKL DGEMM (median of per-round ratios t_MKL / t_method; same process, interleaved).")
print("MKL column: median seconds. OpenBLAS/BLIS: their own dgemm, GFLOP/s (MKL GFLOP/s in brackets).\n")
print("| n | MKL s [GF/s] | OpenBLAS GF/s | BLIS GF/s | " + " | ".join(short(m) for m in meths) + " |")
print("|---|---|---|---|" + "---|" * len(meths))
for n in sorted(rows):
    d = rows[n]
    mk = sorted(d["dgemm_all"])[len(d["dgemm_all"]) // 2]
    cells = []
    for m in meths:
        if m in d:
            _, p, lo, hi = d[m]
            cells.append(f"**{p:.2f}**" if p == max(v[1] for k, v in d.items() if k != "dgemm_all") else f"{p:.2f}")
        else:
            cells.append("")
    print(f"| {n} | {mk:.4g} [{2 * n ** 3 / mk / 1e9:.0f}] | {libs[n].get('openblas', 0):.0f} | {libs[n].get('blis', 0):.0f} | "
          + " | ".join(cells) + " |")
