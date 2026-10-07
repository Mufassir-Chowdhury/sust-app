#!/bin/bash
# Final speed sweep: every contender against MKL dgemm, interleaved, same machine and library.
# usage: bench/sweep.sh [out_file] [sizes...]
cd "$(dirname "$0")/.."
OUT=${1:-results/sweep.txt}; shift
SIZES=${@:-500 750 1000 1500 2000 3000 4000 6000 8000 10000 12000 16000 20000}
export OMP_NUM_THREADS=4 MKL_NUM_THREADS=4 OPENBLAS_NUM_THREADS=4 BLIS_NUM_THREADS=4 OMP_PROC_BIND=close
W1=slp/winograd.slp; W2=slp/winograd-squared.slp; P48=slp/4x4x4_r48_plinopt-204.slp
echo "# $(date -u) $(lscpu | grep 'Model name' | sed 's/ \+/ /g')" >> "$OUT"
for n in $SIZES; do
  if   [ $n -le 2000 ]; then reps=15
  elif [ $n -le 4000 ]; then reps=9
  elif [ $n -le 8000 ]; then reps=5
  else reps=3; fi
  # other tuned BLAS libraries (their own dgemm, same thread count)
  ./bin/bench_dgemm_openblas $reps $n | sed "s/^/openblas /" >> "$OUT"
  ./bin/bench_dgemm_blis $reps $n | sed "s/^/blis /" >> "$OUT"
  if [ $n -le 4000 ]; then
    ./bin/fmmtest time $reps $n $n $n dgemm sw1 sw2 g:$W1:1:0:1 g:$W2:1:0:1 g:$P48:1:0:1 oz14 oz15 oz16 >> "$OUT"
  elif [ $n -le 12000 ]; then
    M="dgemm sw1 sw2 sw1+g:$W2:1:0:1 sw1+g:$P48:1:0:1 oz14 oz15 oz16"
    [ $n -le 8000 ] && M="$M g:$W2:1:0:1 g:$P48:1:0:1"
    ./bin/fmmtest time $reps $n $n $n $M >> "$OUT"
  else
    for meth in sw1 sw2 sw3 oz14 oz15 oz16; do
      ./bin/fmmtest time $reps $n $n $n dgemm $meth >> "$OUT"
    done
    [ $n -le 16000 ] && ./bin/fmmtest time $reps $n $n $n dgemm sw1+g:$P48:1:0:1 >> "$OUT"
  fi
done
echo "# done $(date -u)" >> "$OUT"
