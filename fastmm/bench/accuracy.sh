#!/bin/bash
# Accuracy study: every method on every input class, errors against a double-double reference.
# usage: bench/accuracy.sh [out_file]
cd "$(dirname "$0")/.."
OUT=${1:-results/accuracy.txt}
export OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 OMP_PROC_BIND=close
M="dgemm sw1 sw2 sw3 g:slp/winograd-squared.slp:1:0:1 g:slp/4x4x4_r48_plinopt-204.slp:1:0:1 g:slp/4x4x4_r48_plinopt-accurate.slp:1:0:1 g:slp/3x3x3_r23_fastmatmul-grey333-23-142.slp:1:0:1 g:slp/3x3x6_r40_fastmatmul-tichavsky_kovac336-40-960.slp:1:0:1 oz12 oz13 oz14 oz15 oz16 ozf14:slp/winograd.slp"
: > "$OUT"
for n in 1000 4000; do
  ns=0; [ $n -gt 1000 ] && ns=20000
  for cfg in "0 0" "1 0" "5 0" "2 10" "2 32" "3 10" "3 20" "3 32" "4 10" "4 20" "4 32"; do
    set -- $cfg
    ./bin/fmmtest acc $1 $2 $n $n $n $ns $M >> "$OUT"
  done
done
