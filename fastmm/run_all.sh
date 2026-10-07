#!/bin/bash
# One command: build, run every correctness check, the accuracy study and the speed sweep.
# Usage: ./run_all.sh [quick]     ("quick" limits the sweep to n <= 4000)
# Requirements: gcc >= 13, Python 3, an Intel CPU with AMX-INT8 (Sapphire Rapids or later),
# MKL (pip install mkl mkl-devel), OpenBLAS and BLIS (apt: libopenblas-openmp-dev libblis-openmp-dev).
set -e
cd "$(dirname "$0")"
lscpu | grep -E "Model name|^CPU\(s\)|L2|L3" ; python3 -c "import sys; print(sys.version)"
bench/checks.sh 2>&1 | tee results/checks.txt
bench/accuracy.sh results/accuracy.txt && python3 tools/acc_table.py results/accuracy.txt > results/accuracy.md
if [ "$1" = quick ]; then bench/sweep.sh results/sweep.txt 500 1000 2000 4000
else bench/sweep.sh results/sweep.txt; fi
python3 tools/sweep_table.py results/sweep.txt > results/sweep.md
bench/cold.sh results/cold.txt
if [ "$1" != quick ]; then
  bench/sweep_final_small.sh results/sweep_oz_final.txt   # n = 4000, 8000 (+ ozw16 at 16000, 20000)
  bench/sweep_final_oz.sh results/sweep_oz_final.txt      # n = 6000 ... 20000
  python3 tools/oz_table.py results/sweep_oz_final.txt > results/sweep_oz_final.md
fi
echo "results in results/: checks.txt accuracy.md sweep.md cold.txt sweep_oz_final.md"
