#!/bin/bash
# Final-code measurements at n = 4000 and 8000 (emulation variants and the best Strassen-type plans in
# the same interleaved session), plus ozw16 at 16000/20000 after the small-block fallback.
cd "$(dirname "$0")/.."
OUT=${1:-results/sweep_oz_final.txt}
export OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 OMP_PROC_BIND=close
W2=slp/winograd-squared.slp; P48=slp/4x4x4_r48_plinopt-204.slp
echo "# final code, n = 4000/8000 with Strassen-type plans $(date -u)" >> "$OUT"
./bin/fmmtest time 9 4000 4000 4000 dgemm sw1 g:$W2:1:0:1 g:$P48:1:0:1 oz14 ozw14 oz16 ozw16 >> "$OUT"
./bin/fmmtest time 5 8000 8000 8000 dgemm sw1 g:$W2:1:0:1 g:$P48:1:0:1 oz14 ozw14 oz16 ozw16 >> "$OUT"
echo "# ozw16 after small-block fallback $(date -u)" >> "$OUT"
for n in 16000 20000; do ./bin/fmmtest time 3 $n $n $n dgemm ozw16 >> "$OUT"; done
echo "# done $(date -u)" >> "$OUT"
