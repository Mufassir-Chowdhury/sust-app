#!/usr/bin/env python3
"""README accuracy table from results/accuracy.txt: max componentwise error |C-C*|/(|A||B|) per input class
at one size, for DGEMM, the Strassen-type methods and the emulation variants, plus what the certified
mode did (all certified / partly recomputed / DGEMM fallback)."""
import re, sys, collections
n_sel = int(sys.argv[2]) if len(sys.argv) > 2 else 4000
rows = collections.OrderedDict()
for line in open(sys.argv[1]):
    if not line.startswith("acc "): continue
    kv = dict(re.findall(r"(\w+)=(\S+)", line))
    if int(kv["n"]) != n_sel: continue
    rows.setdefault((kv["type"], int(kv["r"])), {})[kv["method"]] = kv
P48 = "g:slp/4x4x4_r48_plinopt-204.slp:1:0:1"
cols = [("DGEMM", "dgemm"), ("sw1", "sw1"), ("sw2", "sw2"), ("sc:sw1", "sc:sw1"), ("P48", P48),
        ("oz14", "oz14"), ("oz16", "oz16"), ("ozc16", "ozc16")]
names = {"unif": "uniform [-1,1]", "pos": "positive [0,1]", "cancel": "cancellation", "rowcol": "rows/cols scaled 2^±r",
         "inner": "inner dim. scaled 2^±r", "randexp": "random exponents 2^±r", "checker": "checker, spread 2^r",
         "decay": "decay to 2^-r"}
print(f"| input (n = {n_sel}) | r | " + " | ".join(c for c, _ in cols) + " | ozc16 did |")
print("|---|---|" + "---|" * len(cols) + "---|")
for (t, r), d in rows.items():
    ref = float(d["dgemm"]["max_cw"]) if "dgemm" in d else None
    cells = []
    for c, m in cols:
        if m not in d: cells.append("-"); continue
        v = float(d[m]["max_cw"])
        s = f"{v:.1e}"
        if ref and m != "dgemm" and v > 10 * ref: s = f"**{s}**"
        cells.append(s)
    c = d.get("ozc16")
    if c is None: did = "-"
    elif int(c.get("fallback_blocks", "0/1").split("/")[0]) > 0: did = "DGEMM fallback"
    elif int(c.get("uncertified", 0)) == 0: did = "all certified"
    else:
        did = f"{int(c['uncertified']) / (n_sel * n_sel):.1%} recomputed"
        if int(c.get("dgemm_tiles", 0)): did += f" ({c['dgemm_tiles']} DGEMM tiles)"
    print(f"| {names.get(t, t)} | {r} | " + " | ".join(cells) + f" | {did} |")
