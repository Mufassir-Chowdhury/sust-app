#!/bin/bash
# Final-code speed sweep: the emulation variants and the best Strassen-type plans of the main sweep,
# interleaved against MKL in one session per size (16000 and 20000: one MKL/method pair per process
# because of memory). Paired speedups: median over rounds of t_MKL / t_method in the same round.
cd "$(dirname "$0")/.."
OUT=${1:-results/sweep_final.txt}
export OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 OMP_PROC_BIND=close
W1=slp/winograd.slp; W2=slp/winograd-squared.slp; P48=slp/4x4x4_r48_plinopt-204.slp
OZ="oz14 ozw14 oz16 ozw16"
echo "# final sweep $(date -u) $(git rev-parse --short HEAD 2>/dev/null)" >> "$OUT"
./bin/fmmtest time 15 1000 1000 1000 dgemm sw1 g:$W1:1:0:1 g:$W2:1:0:1 oz14 oz16 >> "$OUT"
for n in 1500 2000; do ./bin/fmmtest time 15 $n $n $n dgemm g:$W1:1:0:1 g:$W2:1:0:1 g:$P48:1:0:1 $OZ >> "$OUT"; done
for n in 3000 4000; do ./bin/fmmtest time 9 $n $n $n dgemm sw1 g:$W2:1:0:1 g:$P48:1:0:1 $OZ >> "$OUT"; done
for n in 6000 8000; do ./bin/fmmtest time 5 $n $n $n dgemm sw1 g:$W2:1:0:1 g:$P48:1:0:1 $OZ >> "$OUT"; done
for n in 10000 12000; do ./bin/fmmtest time 3 $n $n $n dgemm sw1 sw1+g:$P48:1:0:1 $OZ >> "$OUT"; done
for meth in $OZ sw1+g:$P48:1:0:1; do ./bin/fmmtest time 3 16000 16000 16000 dgemm $meth >> "$OUT"; done
for meth in oz14 ozw14 oz16 sw2; do ./bin/fmmtest time 3 20000 20000 20000 dgemm $meth >> "$OUT"; done
echo "# done $(date -u)" >> "$OUT"
