#!/bin/sh
# build a review driver: ./build.sh name [extra sources]   (driver that #includes ozaki.c: no src/ozaki.c in the list)
cd /home/user/sust-app/fastmm
n=$1; shift
gcc -O3 -march=native -fopenmp -Wall -Wno-unused-function -Wno-unused-variable -mamx-tile -mamx-int8 -DAMX_KERNEL=14 -DAMX_KC=512 -DAMX_MC=128 -DAMX_NC=256 -DAMX_PF=1 -DUSE_MKL -Isrc \
  -o review2/$n review2/$n.c "$@" src/amx.c -I/usr/local/include -L/usr/local/lib -l:libmkl_intel_lp64.so.3 -l:libmkl_gnu_thread.so.3 -l:libmkl_core.so.3 -Wl,-rpath,/usr/local/lib -lm
