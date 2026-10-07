#!/usr/bin/env python3
"""Measure the inputs of the cost model (tools/costmodel.py) and write results/calib.json:
  gemm[T]: median dgemm seconds for square n, with T threads (4 = all cores, 1 = one core);
  bw[T]:   effective streaming bandwidth of the fused passes as a function of footprint.
Run from the fastmm directory with the machine otherwise idle."""
import json, os, re, subprocess, sys

env = dict(os.environ, OMP_PROC_BIND="close")
def run(cmd, threads):
    e = dict(env, OMP_NUM_THREADS=str(threads), MKL_NUM_THREADS=str(threads))
    return subprocess.run(cmd, env=e, capture_output=True, text=True, check=True).stdout

calib = {"gemm": {"4": [], "1": []}, "bw": {"4": []}}
sizes4 = [256, 384, 500, 750, 1000, 1500, 2000, 3000, 4000, 6000, 8000]
sizes1 = [256, 384, 500, 750, 1000, 1500, 2000, 3000, 4000]
for T, sizes in (("4", sizes4), ("1", sizes1)):
    out = run(["./bin/bench_dgemm_mkl", "15"] + [str(n) for n in sizes], int(T))
    for line in out.splitlines():
        m = re.search(r"n=(\d+) .*median=([\d.]+)", line)
        if m: calib["gemm"][T].append([int(m.group(1)), float(m.group(2))])
# pass bandwidth from the fused passes of one Winograd level and one 4x4 (49) level
for slp in ("slp/winograd.slp", "slp/winograd-squared.slp"):
    hdr = open(slp).read().split()
    out = run(["./bin/fmmtest", "passes", slp, "7", "1000", "2000", "4000", "8000"], 4)
    sys.path.insert(0, "tools")
    from costmodel import read_slp_header
    M, K, N, r, nA, nB = read_slp_header(slp)
    for line in out.splitlines():
        m = re.search(r"n=(\d+) tA=([\d.]+) tB=([\d.]+) tC=([\d.]+)", line)
        if not m: continue
        n = int(m.group(1)); tA, tB, tC = (float(m.group(i)) for i in (2, 3, 4))
        b = n // M
        bytesA = 8 * (M * K + 2 * nA) * b * b
        bytesC = 8 * (r + 2 * M * N) * b * b
        foot = 8 * (3 * n * n + (nA + nB + r) * b * b)
        calib["bw"]["4"].append([foot, (2 * bytesA + bytesC) / (tA + tB + tC) / 1e9])
calib["bw"]["4"].sort()
json.dump(calib, open("results/calib.json", "w"), indent=1)
print(json.dumps(calib, indent=1))
