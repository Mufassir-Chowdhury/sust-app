CC      = gcc
CFLAGS  = -O3 -march=native -fopenmp -Wall -Wno-unused-function
OBLAS   = -I/usr/include/x86_64-linux-gnu/openblas-openmp -L/usr/lib/x86_64-linux-gnu/openblas-openmp -lopenblas
BLIS    = -I/usr/include/x86_64-linux-gnu/blis-openmp -DBLIS_CBLAS -L/usr/lib/x86_64-linux-gnu/blis-openmp -lblis
MKL     = -I/usr/local/include -L/usr/local/lib -l:libmkl_intel_lp64.so.3 -l:libmkl_gnu_thread.so.3 -l:libmkl_core.so.3 -Wl,-rpath,/usr/local/lib
BIN     = bin
LIBS    = -lm

$(BIN):
	mkdir -p $(BIN)

$(BIN)/bench_dgemm_openblas: src/bench_dgemm.c src/common.h | $(BIN)
	$(CC) $(CFLAGS) -o $@ $< $(OBLAS) $(LIBS)
$(BIN)/bench_dgemm_blis: src/bench_dgemm.c src/common.h | $(BIN)
	$(CC) $(CFLAGS) -o $@ $< $(BLIS) $(LIBS)
$(BIN)/bench_dgemm_mkl: src/bench_dgemm.c src/common.h | $(BIN)
	$(CC) $(CFLAGS) -DUSE_MKL -o $@ $< $(MKL) $(LIBS)

baselines: $(BIN)/bench_dgemm_openblas $(BIN)/bench_dgemm_blis $(BIN)/bench_dgemm_mkl

$(BIN)/bench_sw_mkl: src/bench_sw.c src/sw.c src/fmm.h src/blas.h src/common.h | $(BIN)
	$(CC) $(CFLAGS) -DUSE_MKL -o $@ src/bench_sw.c src/sw.c $(MKL) $(LIBS)
$(BIN)/bench_sw_openblas: src/bench_sw.c src/sw.c src/fmm.h src/blas.h src/common.h | $(BIN)
	$(CC) $(CFLAGS) -o $@ src/bench_sw.c src/sw.c $(OBLAS) $(LIBS)

AMXF = -mamx-tile -mamx-int8 -DAMX_KERNEL=14 -DAMX_KC=512 -DAMX_MC=128 -DAMX_NC=256 -DAMX_PF=1
FSRC = src/fmmtest.c src/sw.c src/ozaki.c src/ozfmm.c src/ozw.c src/amx.c src/gen.c
FHDR = src/slp.h src/oz_internal.h src/gen.h src/fmm.h src/blas.h src/common.h src/ozaki.h src/amx.h src/testmat.h src/oz_consts.h

src/oz_consts.h: tools/gen_oz_consts.py
	python3 tools/gen_oz_consts.py $@

$(BIN)/fmmtest: $(FSRC) $(FHDR) | $(BIN)
	$(CC) $(CFLAGS) $(AMXF) -DUSE_MKL -o $@ $(FSRC) $(MKL) $(LIBS)

$(BIN)/bench_amx: src/bench_amx.c src/amx.c src/amx.h src/common.h | $(BIN)
	$(CC) $(CFLAGS) $(AMXF) -o $@ src/bench_amx.c src/amx.c $(LIBS)
$(BIN)/bench_par: src/bench_par.c src/blas.h src/common.h | $(BIN)
	$(CC) $(CFLAGS) -DUSE_MKL -o $@ $< $(MKL) $(LIBS)
$(BIN)/stream: src/stream.c src/common.h | $(BIN)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

all: baselines $(BIN)/fmmtest $(BIN)/bench_amx $(BIN)/bench_par $(BIN)/stream
.PHONY: all baselines
