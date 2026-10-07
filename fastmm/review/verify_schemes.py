#!/usr/bin/env python3
"""Independent check of bilinear schemes and SLPs (review code, not part of the project).
Own coefficient parser (int, p/q, Gaussian a+bi), own tensor check, own random evaluation,
own SLP interpreter. Exact arithmetic only (Fractions / pairs of Fractions)."""
import json, re, sys, random
from fractions import Fraction as F

def parse(c):
    """-> (re, im) Fractions"""
    if isinstance(c, int): return (F(c), F(0))
    s = str(c).replace(" ", "")
    if not s.endswith("i"): return (F(s), F(0))
    body = s[:-1]
    # split at the last +/- that is not at position 0 and not after '/'
    m = None
    for k in range(len(body) - 1, 0, -1):
        if body[k] in "+-" and body[k-1] not in "/eE":
            m = k; break
    if m is None:  # pure imaginary
        im = body if body not in ("", "+", "-") else body + "1"
        return (F(0), F(im))
    re_, im = body[:m], body[m:]
    if im in ("+", "-"): im += "1"
    return (F(re_), F(im))

def mul(a, b): return (a[0]*b[0] - a[1]*b[1], a[0]*b[1] + a[1]*b[0])
def add(a, b): return (a[0]+b[0], a[1]+b[1])
Z = (F(0), F(0))

def tensor_check(d):
    M, K, N = d["dims"]; U = [[parse(x) for x in r] for r in d["U"]]
    V = [[parse(x) for x in r] for r in d["V"]]; W = [[parse(x) for x in r] for r in d["W"]]
    r = len(U); bad = 0
    for i in range(M):
     for k in range(K):
      for k2 in range(K):
       for j in range(N):
        for i2 in range(M):
         for j2 in range(N):
            s = Z
            for l in range(r):
                u, v, w = U[l][i*K+k], V[l][k2*N+j], W[l][i2*N+j2]
                if u == Z or v == Z or w == Z: continue
                s = add(s, mul(mul(u, v), w))
            want = (F(1) if (i == i2 and k == k2 and j == j2) else F(0), F(0))
            bad += s != want
    return r, bad

def eval_check(d, trials=3):
    M, K, N = d["dims"]; U = [[parse(x) for x in r] for r in d["U"]]
    V = [[parse(x) for x in r] for r in d["V"]]; W = [[parse(x) for x in r] for r in d["W"]]
    ok = True
    for t in range(trials):
        A = [[(F(random.randint(-10**6, 10**6)), F(0)) for _ in range(K)] for _ in range(M)]
        B = [[(F(random.randint(-10**6, 10**6)), F(0)) for _ in range(N)] for _ in range(K)]
        prods = []
        for l in range(len(U)):
            a = Z; b = Z
            for i in range(M):
                for k in range(K): a = add(a, mul(U[l][i*K+k], A[i][k]))
            for k in range(K):
                for j in range(N): b = add(b, mul(V[l][k*N+j], B[k][j]))
            prods.append(mul(a, b))
        for i in range(M):
            for j in range(N):
                c = Z
                for l in range(len(U)): c = add(c, mul(W[l][i*N+j], prods[l]))
                ref = (sum(A[i][k][0]*B[k][j][0] for k in range(K)), F(0))
                ok &= c == ref
    return ok

def slp_check(path, trials=3):
    toks = open(path).read().split("\n")
    lines = [l.split() for l in toks if l.strip()]
    pos = 0
    assert lines[pos][0] == "dims"; M, K, N, r = map(int, lines[pos][1:]); pos += 1
    assert lines[pos][0] == "prodscale"; ps = [F(x) for x in lines[pos][1:]]; pos += 1
    sec = {}
    for name in "ABC":
        assert lines[pos] == ["slp", name], lines[pos]; pos += 1
        nin, ni, no = map(int, lines[pos]); pos += 1
        ins = []
        for _ in range(ni):
            t = lines[pos]; pos += 1; nt = int(t[0])
            ins.append([(int(t[1+2*q]), F(t[2+2*q])) for q in range(nt)])
        outs = []
        for _ in range(no):
            t = lines[pos]; pos += 1; outs.append((int(t[0]), F(t[1])))
        sec[name] = (nin, ins, outs)
    def run(s, x):
        nin, ins, outs = s; v = list(x); assert len(v) == nin
        for I in ins: v.append(sum(c * v[src] for src, c in I))
        return [sc * v[o] for o, sc in outs]
    ok = True
    for t in range(trials):
        A = [F(random.randint(-10**9, 10**9), random.randint(1, 50)) for _ in range(M*K)]
        B = [F(random.randint(-10**9, 10**9), random.randint(1, 50)) for _ in range(K*N)]
        a = run(sec["A"], A); b = run(sec["B"], B)
        assert len(a) == r and len(b) == r
        p = [ps[l] * a[l] * b[l] for l in range(r)]
        c = run(sec["C"], p)
        ref = [sum(A[i*K+k] * B[k*N+j] for k in range(K)) for i in range(M) for j in range(N)]
        ok &= c == ref
    return (M, K, N, r), ok

if __name__ == "__main__":
    random.seed(12345)
    for f in sys.argv[1:]:
        if f.endswith(".json"):
            d = json.load(open(f))
            r, bad = tensor_check(d)
            print(f"{f}: dims={d['dims']} rank={r} ring={d.get('ring')} tensor-mismatches={bad} random-eval={'ok' if eval_check(d) else 'FAIL'}")
        else:
            dims, ok = slp_check(f)
            print(f"{f}: SLP dims/rank={dims} random-eval={'ok' if ok else 'FAIL'}")
