#!/bin/bash
# Final-code measurements of the emulation variants (plain vs exact-Winograd) at larger sizes.
cd "$(dirname "$0")/.."
OUT=${1:-results/sweep_oz_final.txt}
export OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 OMP_PROC_BIND=close
echo "# $(date -u)" >> "$OUT"
./bin/fmmtest time 5 6000 6000 6000 dgemm oz14 ozw14 oz16 ozw16 >> "$OUT"
./bin/fmmtest time 3 12000 12000 12000 dgemm oz14 ozw14 oz16 ozw16 >> "$OUT"
for n in 16000 20000; do
  for meth in oz14 ozw14 oz16 ozw16; do ./bin/fmmtest time 3 $n $n $n dgemm $meth >> "$OUT"; done
done
echo "# done $(date -u)" >> "$OUT"
