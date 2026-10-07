#!/bin/bash
# Accuracy study: every method on every input class, errors against a double-double reference.
# usage: bench/accuracy.sh [out_file]
cd "$(dirname "$0")/.."
OUT=${1:-results/accuracy.txt}
export OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 OMP_PROC_BIND=close
W1=slp/winograd.slp; W2=slp/winograd-squared.slp; P48=slp/4x4x4_r48_plinopt-204.slp
S336=slp/3x3x6_r40_fastmatmul-tichavsky_kovac336-40-960.slp
M="dgemm sw1 sw2 sw3 sc:sw1 sc:sw2 g:$W2:1:0:1 g:$P48:1:0:1 g:$S336:1:0:1 oz13 oz14 oz15 oz16 ozf14:$W1"
: > "$OUT"
# every entry of C is checked (last-but-methods argument 0); the reference is computed once per input
for n in 1000 2000 4000; do
  for cfg in "0 0" "1 0" "5 0" "2 10" "2 32" "3 10" "3 20" "3 32" "4 10" "4 20" "4 32" "4 48" "4 64"; do
    set -- $cfg
    ./bin/fmmtest acc $1 $2 $n $n $n 0 $M >> "$OUT"
  done
done
# n = 8000: the input classes where the methods differ most, fewer methods
for cfg in "0 0" "4 32" "4 48" "4 64"; do
  set -- $cfg
  ./bin/fmmtest acc $1 $2 8000 8000 8000 0 dgemm sw1 sw2 g:$P48:1:0:1 oz14 oz15 oz16 ozw14 ozw16 >> "$OUT"
done
