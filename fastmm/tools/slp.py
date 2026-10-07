#!/usr/bin/env python3
"""Turn a bilinear scheme (U, V, W) into straight-line programs (SLPs) for the C engine.

Scheme JSON (see schemes/INDEX.md): dims [M,K,N], rank r, U (r x MK), V (r x KN), W (r x MN),
exact rationals ("1/2") or ints.  Product l: M_l = (sum U[l][i*K+k] A_ik) (sum V[l][k*N+j] B_kj),
C_ij = sum_l W[l][i*N+j] M_l.

Usage:
  slp.py emit  scheme.json [scheme2.json ...] -o out.slp   # tensor product of the schemes, then CSE
  slp.py stats scheme.json [...]

The emitted text format (read by src/gen.c):
  dims M K N r
  prodscale r numbers            (scalar folded into each product)
  then three SLP sections "slp A", "slp B", "slp C":
    nin ninstr nout
    ninstr lines:  nterms  src coef  src coef ...      (defines var nin+idx)
    nout lines:    var scale                           (output o = scale * var)
For A: inputs are the M*K blocks (row-major block index), outputs the r left factors.
For B: inputs K*N blocks, outputs r right factors.  For C: inputs the r products, outputs M*N blocks.
A left/right factor that is +-c times a single input block needs no buffer (var < nin).
"""
import json, sys, itertools
from fractions import Fraction as F


def load(path):
    d = json.load(open(path))
    cv = lambda x: F(x) if not isinstance(x, list) else None
    U = [[F(str(x)) for x in row] for row in d["U"]]
    V = [[F(str(x)) for x in row] for row in d["V"]]
    W = [[F(str(x)) for x in row] for row in d["W"]]
    return {"name": d.get("name", path), "dims": tuple(d["dims"]), "U": U, "V": V, "W": W}


def verify(s):
    M, K, N = s["dims"]
    U, V, W = s["U"], s["V"], s["W"]
    r = len(U)
    # T[(i,k),(k',j),(i',j')] = sum_l U V W must equal delta
    for i in range(M):
        for k in range(K):
            ul = [(l, U[l][i * K + k]) for l in range(r) if U[l][i * K + k] != 0]
            for k2 in range(K):
                for j in range(N):
                    uv = [(l, a * V[l][k2 * N + j]) for l, a in ul if V[l][k2 * N + j] != 0]
                    for i2 in range(M):
                        for j2 in range(N):
                            t = sum((a * W[l][i2 * N + j2] for l, a in uv), F(0))
                            want = 1 if (i == i2 and k == k2 and j == j2) else 0
                            if t != want:
                                return False
    return True


def compose(s1, s2):
    (M1, K1, N1), (M2, K2, N2) = s1["dims"], s2["dims"]
    M, K, N = M1 * M2, K1 * K2, N1 * N2
    U, V, W = [], [], []
    for l1 in range(len(s1["U"])):
        for l2 in range(len(s2["U"])):
            u = [F(0)] * (M * K)
            v = [F(0)] * (K * N)
            w = [F(0)] * (M * N)
            for i1, k1, i2, k2 in itertools.product(range(M1), range(K1), range(M2), range(K2)):
                c = s1["U"][l1][i1 * K1 + k1] * s2["U"][l2][i2 * K2 + k2]
                if c: u[(i1 * M2 + i2) * K + (k1 * K2 + k2)] = c
            for k1, j1, k2, j2 in itertools.product(range(K1), range(N1), range(K2), range(N2)):
                c = s1["V"][l1][k1 * N1 + j1] * s2["V"][l2][k2 * N2 + j2]
                if c: v[(k1 * K2 + k2) * N + (j1 * N2 + j2)] = c
            for i1, j1, i2, j2 in itertools.product(range(M1), range(N1), range(M2), range(N2)):
                c = s1["W"][l1][i1 * N1 + j1] * s2["W"][l2][i2 * N2 + j2]
                if c: w[(i1 * M2 + i2) * N + (j1 * N2 + j2)] = c
            U.append(u); V.append(v); W.append(w)
    return {"name": s1["name"] + "x" + s2["name"], "dims": (M, K, N), "U": U, "V": V, "W": W}


def cse(forms, nin):
    """Greedy pairwise common-subexpression elimination over the rationals.
    forms: list of dict var->coef.  Returns (instrs, outs) with instrs = list of dict (var->coef)
    defining new vars nin, nin+1, ...; outs = list of (var, scale)."""
    forms = [dict(f) for f in forms]
    instrs = []
    nvar = nin
    while True:
        # count pairs (a, b, ratio cb/ca) with a < b
        cnt = {}
        for f in forms:
            if len(f) < 2:
                continue
            items = sorted(f.items())
            for (a, ca), (b, cb) in itertools.combinations(items, 2):
                key = (a, b, cb / ca)
                cnt[key] = cnt.get(key, 0) + 1
        if not cnt:
            break
        key, c = max(cnt.items(), key=lambda kv: (kv[1], -kv[0][0], -kv[0][1]))
        if c < 2:
            break
        a, b, ratio = key
        t = nvar
        nvar += 1
        instrs.append({a: F(1), b: ratio})
        for f in forms:
            if a in f and b in f and f[b] / f[a] == ratio:
                ca = f.pop(a)
                f.pop(b)
                f[t] = f.get(t, F(0)) + ca
                if f[t] == 0:
                    f.pop(t)
    outs = []
    for f in forms:
        if len(f) == 0:
            outs.append((-1, F(0)))
        elif len(f) == 1:
            (v, c), = f.items()
            outs.append((v, c))
        else:
            items = sorted(f.items())
            c0 = items[0][1]
            instrs.append({v: c / c0 for v, c in items})
            outs.append((nvar, c0))
            nvar += 1
    return instrs, outs


def count_adds(instrs):
    return sum(len(d) - 1 for d in instrs)


def build(s):
    M, K, N = s["dims"]
    r = len(s["U"])
    fa = [{j: c for j, c in enumerate(row) if c} for row in s["U"]]
    fb = [{j: c for j, c in enumerate(row) if c} for row in s["V"]]
    fc = [{l: s["W"][l][ij] for l in range(r) if s["W"][l][ij]} for ij in range(M * N)]
    ia, oa = cse(fa, M * K)
    ib, ob = cse(fb, K * N)
    # fold the A/B output scales into the products
    pscale = [oa[l][1] * ob[l][1] for l in range(r)]
    oa = [(v, F(1)) for v, _ in oa]
    ob = [(v, F(1)) for v, _ in ob]
    fc = [{l: c * pscale[l] for l, c in f.items()} for f in fc]
    ic, oc = cse(fc, r)
    return {"dims": (M, K, N), "r": r, "A": (M * K, ia, oa), "B": (K * N, ib, ob), "C": (r, ic, oc)}


def emit(prog, out):
    M, K, N = prog["dims"]
    r = prog["r"]
    L = [f"dims {M} {K} {N} {r}", "prodscale " + " ".join("1" for _ in range(r))]
    for sec in "ABC":
        nin, ins, outs = prog[sec]
        L.append(f"slp {sec}")
        L.append(f"{nin} {len(ins)} {len(outs)}")
        for d in ins:
            L.append(str(len(d)) + " " + " ".join(f"{v} {float(c)!r}" for v, c in sorted(d.items())))
        for v, c in outs:
            L.append(f"{v} {float(c)!r}")
    open(out, "w").write("\n".join(L) + "\n")


def stats(prog):
    M, K, N = prog["dims"]
    r = prog["r"]
    res = {"dims": (M, K, N), "r": r}
    for sec in "ABC":
        nin, ins, outs = prog[sec]
        res["adds_" + sec] = count_adds(ins)
        res["bufs_" + sec] = sum(1 for v, c in outs if v >= nin) if sec != "C" else len(outs)
    return res


def main():
    args = sys.argv[1:]
    cmd = args[0]
    out = None
    if "-o" in args:
        i = args.index("-o"); out = args[i + 1]; args = args[:i] + args[i + 2:]
    schemes = [load(p) for p in args[1:]]
    s = schemes[0]
    for t in schemes[1:]:
        s = compose(s, t)
    ok = verify(s)
    prog = build(s)
    st = stats(prog)
    print(f"{'+'.join(x['name'] for x in schemes)} dims={st['dims']} r={st['r']} exact={ok} "
          f"adds A/B/C={st['adds_A']}/{st['adds_B']}/{st['adds_C']} total={st['adds_A']+st['adds_B']+st['adds_C']} "
          f"buffers A/B={st['bufs_A']}/{st['bufs_B']}")
    if not ok:
        sys.exit(1)
    if cmd == "emit":
        emit(prog, out)


if __name__ == "__main__":
    main()
