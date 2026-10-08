#!/bin/bash
# First call in a fresh process versus the next three calls (workspace allocation and page faults).
cd "$(dirname "$0")/.."
OUT=${1:-results/cold.txt}
export OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 OMP_PROC_BIND=close
: > "$OUT"
for n in 2000 8000; do
  for m in dgemm oz14 oz16 ozw14 ozc16 sw1; do ./bin/fmmtest cold $n $m >> "$OUT"; done
done
