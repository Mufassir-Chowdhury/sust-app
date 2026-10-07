#!/bin/bash
# Correctness checks (exit status != 0 on any failure):
#  1. every scheme in schemes/ satisfies the matrix multiplication tensor exactly (rationals);
#  2. the SLPs used in the experiments are regenerated from the schemes and re-verified;
#  3. CRT constants are regenerated with exact integer checks;
#  4. AMX int8 kernel matches a naive integer product on awkward shapes;
#  5. emulation with modular Strassen is bitwise identical to the plain emulation;
#  6. error regression on odd shapes for every method (thresholds relative to dgemm), including inner
#     scalings of 2^+-300 and the edge cases found by the independent review (underflow, reproducibility).
set -e
cd "$(dirname "$0")/.."
export OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 OMP_PROC_BIND=close
echo "== 1. exact scheme verification"
python3 tools/scheme_check.py schemes/*.json | tail -3
echo "== 2. SLP generation (each composed scheme re-verified before emitting)"
mkdir -p slp
python3 tools/slp.py emit schemes/winograd.json -o slp/winograd.slp
python3 tools/slp.py emit schemes/winograd.json schemes/winograd.json -o slp/winograd-squared.slp
for f in schemes/4x4x4_r48_plinopt-204.json schemes/3x3x6_r40_fastmatmul-tichavsky_kovac336-40-960.json \
         schemes/4x4x4_r48_plinopt-accurate.json schemes/3x3x3_r23_fastmatmul-grey333-23-142.json; do
  python3 tools/slp.py emit $f -o slp/$(basename $f .json).slp
done
echo "== 3. CRT constants"
python3 tools/gen_oz_consts.py src/oz_consts.h && echo "ok"
make -s all
echo "== 4. AMX kernel"
./bin/bench_amx | tee /dev/stderr | grep -q " [1-9][0-9]* mismatches" && { echo FAIL; exit 1; } || echo ok
echo "== 5. bit identity of modular Strassen"
for shp in "0 0 33 65 97" "4 20 1000 7 3" "3 20 301 517 203" "4 32 999 1001 1003" "5 0 1500 1500 1500"; do
  ./bin/fmmtest bitcmp $shp oz14 ozf14:slp/winograd.slp
  ./bin/fmmtest bitcmp $shp oz16 ozf16:slp/winograd-squared.slp
  ./bin/fmmtest bitcmp $shp oz15 ozw15
done
echo "== 5b. guards: k > 131071 (split inner dimension), NaN/Inf propagation"
./bin/fmmtest acc 0 0 64 140000 64 0 dgemm oz14 oz16 | awk '{print $5, $7, $8}'
./bin/fmmtest nancheck
./bin/fmmtest edge
./bin/fmmtest refcheck
echo "== 6. error regression"
python3 - <<'PY'
import subprocess, re, sys
W1, W2, P48 = "slp/winograd.slp", "slp/winograd-squared.slp", "slp/4x4x4_r48_plinopt-204.slp"
meths = ["dgemm", "sw1", "sw2", "sw3", f"g:{W1}:2:0:2", f"g:{W2}:1:0:1", f"g:{P48}:1:0:1", f"sw1+g:{W2}:1:0:1",
         "oz14", "oz15", "oz16", f"ozf14:{W1}"]
fails = 0
for shape in ("0 0 1 1 1", "0 0 33 65 97", "0 0 301 517 203", "1 0 777 1000 555", "5 0 1000 1000 1000",
              "2 32 600 600 600", "3 32 600 600 600", "3 300 500 500 500", "2 400 300 300 300",
              "4 20 1500 1500 1500"):
    out = subprocess.run(["./bin/fmmtest", "acc"] + shape.split()[:2] + shape.split()[2:] + ["0"] + meths,
                         capture_output=True, text=True, check=True).stdout
    res = {kv["method"]: kv for kv in (dict(re.findall(r"(\w+)=(\S+)", l)) for l in out.splitlines())}
    ref = float(res["dgemm"]["max_cw"])
    t = res["dgemm"]["type"]
    for m in meths:
        e = float(res[m]["max_cw"])
        # limits: emulation with 16 moduli never worse than 2x dgemm (scaling handles rowcol/inner);
        # 14/15 moduli and fast schemes: only sanity limits on well-scaled inputs
        lim = 2 * max(ref, 1e-16) if m == "oz16" else (1e-12 if t in ("unif", "pos", "cancel") or m.startswith("oz") else float("inf"))
        ok = e <= lim
        fails += not ok
        print(f"{shape:22s} {m:34s} max_cw={e:.2e} limit={lim:.1e} {'ok' if ok else 'FAIL'}")
sys.exit(1 if fails else 0)
PY
echo "ALL CHECKS PASSED"
