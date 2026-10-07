// Straight-line programs for bilinear schemes (shared by gen.c and ozfmm.c).
#pragma once
#include "gen.h"
typedef struct { int nt; int *src; double *coef; } ginstr;
typedef struct { int nin, ninstr, nout; ginstr *ins; int *ovar; double *oscale; } gslp;
struct gen_scheme { int M, K, N, r; gslp s[3]; double *pscale; int nbufA, nbufB; };

