#!/usr/bin/env python3
"""Tabulate results/accuracy.txt (lines from `fmmtest acc`) as markdown tables."""
import sys, re, collections
rows = collections.OrderedDict()
meths = []
for line in open(sys.argv[1]):
    if not line.startswith("acc "): continue
    kv = dict(re.findall(r"(\w+)=(\S+)", line))
    key = (int(kv["n"]), kv["type"], int(kv["r"]))
    m = kv["method"]
    short = (m.replace("g:slp/", "").replace(".slp", "").replace(":1:0:1", "[bfs]").replace("ozf14:slp/winograd", "oz14+modW")
             .replace("winograd-squared", "W2").replace("4x4x4_r48_plinopt-204", "P48")
             .replace("3x3x6_r40_fastmatmul-tichavsky_kovac336-40-960", "S336").replace("3x3x3_r23_fastmatmul-grey333-23-142", "L23"))
    if short not in meths: meths.append(short)
    rows.setdefault(key, {})[short] = kv
for metric, title in (("max_cw", "max |C-C*| / (|A||B|)  (componentwise; classical bound ~ k*u)"),
                      ("med_rel", "median |C-C*| / |C*|"), ("nrm", "max |C-C*| / (max||a_i|| max||b_j||)  (normwise)")):
    print(f"\n### {title}\n")
    print("| n | input | r | " + " | ".join(meths) + " |")
    print("|---|---|---|" + "---|" * len(meths))
    for (n, t, r), d in rows.items():
        cells = []
        for m in meths:
            v = d.get(m, {}).get(metric)
            cells.append(f"{float(v):.1e}" if v else "")
        print(f"| {n} | {t} | {r} | " + " | ".join(cells) + " |")
