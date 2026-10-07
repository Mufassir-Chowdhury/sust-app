// Reference: oneDNN AMX int8 matmul throughput (s8 x s8 -> s32), weights pre-reordered.
#include <stdio.h>
#include <oneapi/dnnl/dnnl.h>
#include "common.h"
#define CHK(x) do { dnnl_status_t s_ = (x); if (s_ != dnnl_success) { fprintf(stderr, "%s failed %d\n", #x, s_); exit(1);} } while (0)
int main(int argc, char **argv) {
  dnnl_engine_t eng; dnnl_stream_t st;
  CHK(dnnl_engine_create(&eng, dnnl_cpu, 0));
  CHK(dnnl_stream_create(&st, eng, dnnl_stream_default_flags));
  for (int a = 1; a < argc; a++) {
    int64_t n = atol(argv[a]);
    dnnl_dims_t ds = {n, n}, ws = {n, n}, dd = {n, n};
    dnnl_memory_desc_t smd, wmd_any, wmd_plain, dmd;
    CHK(dnnl_memory_desc_create_with_tag(&smd, 2, ds, dnnl_s8, dnnl_ab));
    CHK(dnnl_memory_desc_create_with_tag(&wmd_any, 2, ws, dnnl_s8, dnnl_format_tag_any));
    CHK(dnnl_memory_desc_create_with_tag(&wmd_plain, 2, ws, dnnl_s8, dnnl_ab));
    CHK(dnnl_memory_desc_create_with_tag(&dmd, 2, dd, dnnl_s32, dnnl_ab));
    dnnl_primitive_desc_t pd; dnnl_primitive_t prim;
    CHK(dnnl_matmul_primitive_desc_create(&pd, eng, smd, wmd_any, NULL, dmd, NULL));
    CHK(dnnl_primitive_create(&prim, pd));
    const char *impl; dnnl_primitive_desc_query(pd, dnnl_query_impl_info_str, 0, &impl);
    const_dnnl_memory_desc_t wmd = dnnl_primitive_desc_query_md(pd, dnnl_query_weights_md, 0);
    int8_t *S = amalloc(n * n), *Wp = amalloc(n * n); int32_t *D = amalloc(n * n * 4);
    for (int64_t i = 0; i < n * n; i++) { S[i] = (int8_t)(i * 7); Wp[i] = (int8_t)(i * 13); }
    dnnl_memory_t ms, mwp, mw, md;
    CHK(dnnl_memory_create(&ms, smd, eng, S));
    CHK(dnnl_memory_create(&mwp, wmd_plain, eng, Wp));
    CHK(dnnl_memory_create(&mw, wmd, eng, DNNL_MEMORY_ALLOCATE));
    CHK(dnnl_memory_create(&md, dmd, eng, D));
    // reorder weights once
    dnnl_primitive_desc_t rpd; dnnl_primitive_t rp;
    CHK(dnnl_reorder_primitive_desc_create(&rpd, wmd_plain, eng, wmd, eng, NULL));
    CHK(dnnl_primitive_create(&rp, rpd));
    dnnl_exec_arg_t rargs[2] = {{DNNL_ARG_FROM, mwp}, {DNNL_ARG_TO, mw}};
    double t0 = now_sec();
    CHK(dnnl_primitive_execute(rp, st, 2, rargs)); dnnl_stream_wait(st);
    double trd = now_sec() - t0;
    dnnl_exec_arg_t args[3] = {{DNNL_ARG_SRC, ms}, {DNNL_ARG_WEIGHTS, mw}, {DNNL_ARG_DST, md}};
    double t[9];
    for (int r = -1; r < 9; r++) {
      t0 = now_sec();
      CHK(dnnl_primitive_execute(prim, st, 3, args)); dnnl_stream_wait(st);
      if (r >= 0) t[r] = now_sec() - t0;
    }
    double tm = median(t, 9);
    printf("dnnl n=%ld impl=%s  %.4f s  %.1f GOP/s  (weights reorder %.4f s)\n", (long)n, impl, tm, 2.0*n*n*n/tm*1e-9, trd);
    fflush(stdout);
  }
  return 0;
}
