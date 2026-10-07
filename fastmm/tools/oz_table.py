#!/usr/bin/env python3
"""Table of the final-code emulation variants (results/sweep_oz_final.txt): paired speedup vs MKL.
Later lines for the same (n, method) supersede earlier ones (re-runs after fixes)."""
import re, sys
res = {}
for line in open(sys.argv[1]):
    if not line.startswith("time "): continue
    kv = dict(re.findall(r"(\w+)=(\S+)", line))
    if kv["method"] == "dgemm": continue
    n = int(kv["n"])
    iqr = re.search(r"IQR ([\d.]+)-([\d.]+)", line)
    br = re.search(r"\[scale ([\d.]+) conv ([\d.]+) gemm ([\d.]+) crt ([\d.]+)\]", line)
    res[(n, kv["method"])] = (float(kv["paired"]), iqr.groups(), br.groups() if br else None, float(kv["median"]))
meths = ["oz14", "ozw14", "oz16", "ozw16"]
fast = {"sw1": "sw1", "g:slp/winograd-squared.slp:1:0:1": "W2 task-par.", "g:slp/4x4x4_r48_plinopt-204.slp:1:0:1": "P48 task-par."}
cols = meths + [m for m in fast if any(k[1] == m for k in res)]
print("| n | " + " | ".join(fast.get(m, m) for m in cols) + " |")
print("|---|" + "---|" * len(cols))
for n in sorted({k[0] for k in res}):
    cells = []
    for m in cols:
        if (n, m) in res:
            p, (lo, hi), br, t = res[(n, m)]
            cells.append(f"{p:.2f} [{lo}-{hi}]")
        else:
            cells.append("")
    print(f"| {n} | " + " | ".join(cells) + " |")
print("\nTime breakdown (s): conversion / int8 GEMMs / CRT\n")
print("| n | " + " | ".join(meths) + " |")
print("|---|" + "---|" * len(meths))
for n in sorted({k[0] for k in res}):
    cells = []
    for m in meths:
        v = res.get((n, m))
        cells.append(f"{float(v[2][1]):.2f} / {float(v[2][2]):.2f} / {float(v[2][3]):.2f}" if v and v[2] else "")
    print(f"| {n} | " + " | ".join(cells) + " |")
