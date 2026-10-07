#!/usr/bin/env python3
"""Exact verification and statistics for bilinear matrix-multiplication schemes.

Scheme JSON format (schemes/<name>.json):
  {"name", "dims": [M, K, N], "rank": r, "source", "ring",
   "U": r rows of M*K, "V": r rows of K*N, "W": r rows of M*N}
  product l:  M_l = (sum_{i,k} U[l][i*K+k] A[i][k]) * (sum_{k,j} V[l][k*N+j] B[k][j])
  output:     C[i][j] = sum_l W[l][i*N+j] M_l          (C = A B, no transposes)
Exactness:  sum_l U[l][i*K+k] V[l][k'*N+j] W[l][i'*N+j'] = [i=i'][k=k'][j=j'].

Coefficients: JSON ints, rational strings "p/q", or Gaussian rationals written
"a+bi" / "a-bi" / "bi" with rational a, b (e.g. "1/2-1/2i", "-i").

Usage:
  scheme_check.py FILE.json [...]     verify + print stats (exit 1 if any fails)
  scheme_check.py --json FILE ...     print stats as JSON lines
  scheme_check.py --selftest          check compose(winograd, winograd) etc.
  scheme_check.py --index [DIR]       check every DIR/*.json, write DIR/INDEX.md
Library: load, dump, verify, stats, compose, cyclic, transpose, permute_to.
"""
import json
import math
import os
import re
import sys
from fractions import Fraction
from functools import reduce


# --------------------------------------------------------------------------
# Exact Gaussian rationals (only used for complex schemes)
# --------------------------------------------------------------------------
class QI:
    """a + b*i with a, b Fractions. Immutable."""
    __slots__ = ("re", "im")

    def __init__(self, re_, im_=0):
        object.__setattr__(self, "re", Fraction(re_))
        object.__setattr__(self, "im", Fraction(im_))

    def __setattr__(self, *a):
        raise AttributeError("immutable")

    @staticmethod
    def lift(x):
        return x if isinstance(x, QI) else QI(x, 0)

    def __add__(self, o):
        o = QI.lift(o)
        return QI(self.re + o.re, self.im + o.im)
    __radd__ = __add__

    def __neg__(self):
        return QI(-self.re, -self.im)

    def __sub__(self, o):
        return self + (-QI.lift(o))

    def __rsub__(self, o):
        return QI.lift(o) - self

    def __mul__(self, o):
        o = QI.lift(o)
        return QI(self.re * o.re - self.im * o.im, self.re * o.im + self.im * o.re)
    __rmul__ = __mul__

    def inv(self):
        n = self.re * self.re + self.im * self.im
        return QI(self.re / n, -self.im / n)

    def __truediv__(self, o):
        return self * QI.lift(o).inv()

    def __rtruediv__(self, o):
        return QI.lift(o) * self.inv()

    def __eq__(self, o):
        if isinstance(o, (int, Fraction)):
            return self.im == 0 and self.re == o
        if isinstance(o, QI):
            return self.re == o.re and self.im == o.im
        return NotImplemented

    def __hash__(self):
        return hash(self.re) if self.im == 0 else hash((self.re, self.im))

    def __bool__(self):
        return bool(self.re) or bool(self.im)

    def __abs__(self):
        return math.hypot(self.re, self.im)

    def __repr__(self):
        return coef_str(self)


def _fstr(f):
    return str(f.numerator) if f.denominator == 1 else f"{f.numerator}/{f.denominator}"


def coef_str(c):
    """Canonical string form of a coefficient."""
    if isinstance(c, QI):
        if c.im == 0:
            return _fstr(c.re)
        im = c.im
        ims = "i" if im == 1 else "-i" if im == -1 else _fstr(im) + "i"
        if c.re == 0:
            return ims
        return _fstr(c.re) + ("" if ims.startswith("-") else "+") + ims
    return _fstr(Fraction(c))


_CPLX = re.compile(r"^\s*(?:(?P<re>[+-]?\d+(?:/\d+)?)(?=[+-]|\s*$))?\s*"
                   r"(?:(?P<im>[+-]?(?:\d+(?:/\d+)?)?)\*?i)?\s*$")


def parse_coef(x):
    """JSON value -> Fraction (real) or QI (complex)."""
    if isinstance(x, bool):
        raise ValueError("bool coefficient")
    if isinstance(x, int):
        return Fraction(x)
    if isinstance(x, float):
        if x != int(x):
            raise ValueError(f"float coefficient {x!r}: use exact strings")
        return Fraction(int(x))
    if isinstance(x, str):
        s = x.replace(" ", "")
        if "i" not in s:
            return Fraction(s)
        m = _CPLX.match(s)
        if not m:
            raise ValueError(f"cannot parse coefficient {x!r}")
        re_ = Fraction(m.group("re")) if m.group("re") else Fraction(0)
        ims = m.group("im")
        im = Fraction(1) if ims in ("", "+") else Fraction(-1) if ims == "-" else Fraction(ims)
        return QI(re_, im) if im else re_
    raise ValueError(f"bad coefficient {x!r}")


def to_json_coef(c):
    if isinstance(c, QI):
        if c.im == 0:
            c = c.re
        else:
            return coef_str(c)
    c = Fraction(c)
    return int(c) if c.denominator == 1 else _fstr(c)


def is_complex(c):
    return isinstance(c, QI) and c.im != 0


def ring_of(coefs):
    coefs = list(coefs)
    if any(is_complex(c) for c in coefs):
        return "C"
    def real(c):
        return c.re if isinstance(c, QI) else Fraction(c)
    if all(real(c).denominator == 1 for c in coefs):
        return "Z"
    return "Q"


# --------------------------------------------------------------------------
# Load / dump
# --------------------------------------------------------------------------
class Scheme(dict):
    """Plain dict with parsed coefficient matrices under U, V, W."""

    @property
    def dims(self):
        return tuple(self["dims"])


def from_matrices(dims, U, V, W, **meta):
    M, K, N = dims
    s = Scheme(meta)
    s["dims"] = [M, K, N]
    s["rank"] = len(U)
    s["U"] = [[c if isinstance(c, (Fraction, QI)) else parse_coef(c) for c in row] for row in U]
    s["V"] = [[c if isinstance(c, (Fraction, QI)) else parse_coef(c) for c in row] for row in V]
    s["W"] = [[c if isinstance(c, (Fraction, QI)) else parse_coef(c) for c in row] for row in W]
    check_shape(s)
    s["ring"] = ring_of(c for F in "UVW" for row in s[F] for c in row)
    return s


def check_shape(s):
    M, K, N = s["dims"]
    r = s["rank"]
    for F, n in (("U", M * K), ("V", K * N), ("W", M * N)):
        if len(s[F]) != r:
            raise ValueError(f"{F} has {len(s[F])} rows, rank is {r}")
        for row in s[F]:
            if len(row) != n:
                raise ValueError(f"{F} row has length {len(row)}, expected {n}")


def load(path):
    with open(path) as f:
        d = json.load(f)
    meta = {k: v for k, v in d.items() if k not in ("U", "V", "W", "dims", "rank")}
    s = from_matrices(d["dims"], d["U"], d["V"], d["W"], **meta)
    if d.get("rank") is not None and d["rank"] != s["rank"]:
        raise ValueError(f"declared rank {d['rank']} != {s['rank']} rows")
    if d.get("ring") and d["ring"] != s["ring"]:
        s["ring_declared"] = d["ring"]
    return s


def dump(s, path):
    """Write scheme JSON: header fields one per line, one factor row per line."""
    head = {k: s[k] for k in ("name", "dims", "rank", "source", "ring") if k in s}
    extra = {k: v for k, v in s.items() if k not in head and k not in ("U", "V", "W", "ring_declared")}
    lines = ["{"]
    for k, v in list(head.items()) + list(extra.items()):
        lines.append(f"  {json.dumps(k)}: {json.dumps(v)},")
    for F in "UVW":
        rows = [json.dumps([to_json_coef(c) for c in row], separators=(",", ":")) for row in s[F]]
        lines.append(f'  "{F}": [\n    ' + ",\n    ".join(rows) + "\n  ]" + ("," if F != "W" else ""))
    lines.append("}")
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")


# --------------------------------------------------------------------------
# Exact verification
# --------------------------------------------------------------------------
def _lcm(a, b):
    return a * b // math.gcd(a, b)


def _scaled(rows):
    """Return (scale d, rows of sparse [(idx, int or (re, im))], complex?)."""
    cplx = any(is_complex(c) for row in rows for c in row)
    dens = [1]
    for row in rows:
        for c in row:
            if isinstance(c, QI):
                dens += [c.re.denominator, c.im.denominator]
            elif c:
                dens.append(Fraction(c).denominator)
    d = reduce(_lcm, dens, 1)
    out = []
    for row in rows:
        sp = []
        for idx, c in enumerate(row):
            if not c:
                continue
            if cplx:
                c = QI.lift(c)
                sp.append((idx, (int(c.re * d), int(c.im * d))))
            else:
                c = c.re if isinstance(c, QI) else Fraction(c)
                sp.append((idx, int(c * d)))
        out.append(sp)
    return d, out, cplx


def verify(s, max_errors=10):
    """Exact check. Returns (ok, list of error strings)."""
    check_shape(s)
    M, K, N = s["dims"]
    r = s["rank"]
    dU, Us, cu = _scaled(s["U"])
    dV, Vs, cv = _scaled(s["V"])
    dW, Ws, cw = _scaled(s["W"])
    cplx = cu or cv or cw
    target = dU * dV * dW
    T = {}
    if not cplx:
        for l in range(r):
            ws = Ws[l]
            if not ws:
                continue
            for a, x in Us[l]:
                for b, y in Vs[l]:
                    xy = x * y
                    key = a * (K * N) + b
                    for c, z in ws:
                        kk = key * (M * N) + c
                        T[kk] = T.get(kk, 0) + xy * z
        def nz(v):
            return v != 0
        def is_target(v):
            return v == target
    else:
        def cm(p, q):
            return (p[0] * q[0] - p[1] * q[1], p[0] * q[1] + p[1] * q[0])
        def lift(sp):
            return [(i, v if isinstance(v, tuple) else (v, 0)) for i, v in sp]
        for l in range(r):
            us, vs, ws = lift(Us[l]), lift(Vs[l]), lift(Ws[l])
            for a, x in us:
                for b, y in vs:
                    xy = cm(x, y)
                    key = a * (K * N) + b
                    for c, z in ws:
                        kk = key * (M * N) + c
                        p = cm(xy, z)
                        o = T.get(kk, (0, 0))
                        T[kk] = (o[0] + p[0], o[1] + p[1])
        def nz(v):
            return v != (0, 0)
        def is_target(v):
            return v == (target, 0)

    def name(kk):
        ab, c = divmod(kk, M * N)
        a, b = divmod(ab, K * N)
        i, k = divmod(a, K)
        k2, j = divmod(b, N)
        i2, j2 = divmod(c, N)
        return f"(i,k)=({i},{k}) (k',j)=({k2},{j}) (i',j')=({i2},{j2})"

    def val(v):
        if isinstance(v, tuple):
            return coef_str(QI(Fraction(v[0], target), Fraction(v[1], target)))
        return _fstr(Fraction(v, target))

    errors = []
    expected = set()
    for i in range(M):
        for k in range(K):
            for j in range(N):
                kk = ((i * K + k) * (K * N) + (k * N + j)) * (M * N) + (i * N + j)
                expected.add(kk)
                v = T.get(kk, 0 if not cplx else (0, 0))
                if not is_target(v):
                    errors.append(f"{name(kk)}: got {val(v)}, want 1")
    for kk, v in T.items():
        if kk not in expected and nz(v):
            errors.append(f"{name(kk)}: got {val(v)}, want 0")
    return (not errors), errors[:max_errors] + ([f"... {len(errors) - max_errors} more"] if len(errors) > max_errors else [])


# --------------------------------------------------------------------------
# Statistics
# --------------------------------------------------------------------------
def stats(s):
    M, K, N = s["dims"]
    r = s["rank"]
    coefs = [c for F in "UVW" for row in s[F] for c in row if c]
    nnz = {F: sum(1 for row in s[F] for c in row if c) for F in "UVW"}

    def single(row):
        nzs = [c for c in row if c]
        return len(nzs) == 1 and (nzs[0] == 1 or nzs[0] == -1)

    singles = {F: sum(1 for row in s[F] if single(row)) for F in "UV"}
    # naive additions: forming each left/right factor, plus summing each C entry
    adds_u = sum(max(0, sum(1 for c in row if c) - 1) for row in s["U"])
    adds_v = sum(max(0, sum(1 for c in row if c) - 1) for row in s["V"])
    adds_w = sum(max(0, sum(1 for l in range(r) if s["W"][l][p]) - 1) for p in range(M * N))
    distinct = sorted(set(coefs), key=lambda c: (abs(c), coef_str(c)))
    maxabs = max(coefs, key=abs) if coefs else 0
    ma = abs(maxabs)
    if not any(is_complex(c) for c in coefs):
        ma = coef_str(abs(Fraction(maxabs.re if isinstance(maxabs, QI) else maxabs)))
    else:
        ma = round(ma, 6)
    return {
        "name": s.get("name"),
        "dims": [M, K, N],
        "rank": r,
        "ring": ring_of(coefs),
        "nnz_U": nnz["U"], "nnz_V": nnz["V"], "nnz_W": nnz["W"],
        "nnz": nnz["U"] + nnz["V"] + nnz["W"],
        "single_U": singles["U"], "single_V": singles["V"],
        "naive_adds": adds_u + adds_v + adds_w,
        "max_abs": ma,
        "coefs": [coef_str(c) for c in distinct],
        "omega": 3 * math.log(r) / math.log(M * K * N),
    }


def fmt_stats(st):
    return (f"<{st['dims'][0]},{st['dims'][1]},{st['dims'][2]};{st['rank']}> ring={st['ring']} "
            f"nnz(U,V,W)=({st['nnz_U']},{st['nnz_V']},{st['nnz_W']}) total={st['nnz']} "
            f"single(U,V)=({st['single_U']},{st['single_V']}) naive_adds={st['naive_adds']} "
            f"max|c|={st['max_abs']} coefs={{{', '.join(st['coefs'])}}} omega={st['omega']:.4f}")


# --------------------------------------------------------------------------
# Constructions
# --------------------------------------------------------------------------
def compose(s1, s2, name=None):
    """Kronecker/tensor product scheme of dims (M1*M2, K1*K2, N1*N2).

    Block indices: outer from s1, inner from s2 (i = i1*M2 + i2, k = k1*K2 + k2,
    j = j1*N2 + j2); product index l = l1*r2 + l2.
    """
    M1, K1, N1 = s1["dims"]
    M2, K2, N2 = s2["dims"]
    M, K, N = M1 * M2, K1 * K2, N1 * N2

    def kron(A, B, R1, C1, R2, C2):
        # A: rows over (R1 x C1) row-major, B over (R2 x C2); result over (R1R2 x C1C2)
        out = []
        for ra in A:
            for rb in B:
                row = [0] * (R1 * R2 * C1 * C2)
                for p, x in enumerate(ra):
                    if not x:
                        continue
                    a1, b1 = divmod(p, C1)
                    for q, y in enumerate(rb):
                        if not y:
                            continue
                        a2, b2 = divmod(q, C2)
                        row[(a1 * R2 + a2) * (C1 * C2) + (b1 * C2 + b2)] = x * y
                out.append([c if c else Fraction(0) for c in row])
        return out

    U = kron(s1["U"], s2["U"], M1, K1, M2, K2)
    V = kron(s1["V"], s2["V"], K1, N1, K2, N2)
    W = kron(s1["W"], s2["W"], M1, N1, M2, N2)
    return from_matrices((M, K, N), U, V, W,
                         name=name or f"{s1.get('name', 's1')}_x_{s2.get('name', 's2')}",
                         source=f"compose({s1.get('name')}, {s2.get('name')}) (tensor product, outer blocks from first)")


def _tr(row, R, C):
    """Transpose a row-major R x C coefficient vector -> row-major C x R."""
    return [row[i * C + j] for j in range(C) for i in range(R)]


def cyclic(s):
    """<M,K,N> -> <K,N,M>:  A' = B (KxN), B' = C^T (NxM), C' = A^T (KxM)."""
    M, K, N = s["dims"]
    U = [row[:] for row in s["V"]]
    V = [_tr(row, M, N) for row in s["W"]]
    W = [_tr(row, M, K) for row in s["U"]]
    return from_matrices((K, N, M), U, V, W, **{k: s[k] for k in ("name", "source") if k in s})


def transpose(s):
    """<M,K,N> -> <N,K,M> via C^T = B^T A^T."""
    M, K, N = s["dims"]
    U = [_tr(row, K, N) for row in s["V"]]
    V = [_tr(row, M, K) for row in s["U"]]
    W = [_tr(row, M, N) for row in s["W"]]
    return from_matrices((N, K, M), U, V, W, **{k: s[k] for k in ("name", "source") if k in s})


def permute_to(s, dims):
    """Return (scheme with the requested dims, description of the symmetry used)."""
    dims = list(dims)
    cur, desc = s, []
    for t in range(2):
        c = cur
        for rot in range(3):
            if list(c["dims"]) == dims:
                return c, "+".join(desc + ["cyclic"] * rot) or "identity"
            c = cyclic(c)
        cur = transpose(cur)
        desc = ["transpose"]
    raise ValueError(f"{s['dims']} is not a permutation of {dims}")


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------
def check_file(path):
    s = load(path)
    ok, errs = verify(s)
    st = stats(s)
    st["file"] = os.path.basename(path)
    st["verified"] = ok
    st["errors"] = errs
    st["source"] = s.get("source", "")
    if "ring_declared" in s:
        st["ring_declared"] = s["ring_declared"]
    return st


def selftest():
    here = os.path.dirname(os.path.abspath(__file__))
    sdir = os.path.join(here, "..", "schemes")
    w = load(os.path.join(sdir, "winograd.json"))
    st = load(os.path.join(sdir, "strassen.json"))
    allok = True
    for a, b in ((w, w), (st, st), (st, w)):
        c = compose(a, b)
        ok, errs = verify(c)
        print(f"compose({a['name']},{b['name']}): {'PASS' if ok else 'FAIL'} {fmt_stats(stats(c))}")
        allok &= ok
    # a broken scheme must fail
    bad = compose(w, w)
    bad["W"][0] = [x + 1 if p == 0 else x for p, x in enumerate(bad["W"][0])]
    ok, errs = verify(bad)
    print(f"corrupted compose detected: {'PASS' if not ok else 'FAIL'} ({errs[0] if errs else ''})")
    allok &= not ok
    # symmetries preserve exactness
    for d in ((2, 2, 2),):
        for f in (cyclic, transpose):
            ok, _ = verify(f(w))
            allok &= ok
    return allok


def _source_md(src):
    """'URL @ commit : path[key]' -> markdown link to the file at that commit."""
    m = re.match(r"^(https://github\.com/\S+) @ (\w+) : ([^\[]+)(\[.*\])?$", src)
    if not m:
        return src.replace("|", "\\|")
    url, rev, path, key = m.groups()
    repo = url.split("github.com/")[1]
    return f"[{repo}/{path}{key or ''}]({url}/blob/{rev}/{path})"


def write_index(sdir):
    files = sorted(f for f in os.listdir(sdir) if f.endswith(".json") and not f.startswith("import_"))

    def order(st):
        return (sorted(st.get("dims", [99])), st.get("dims", []), st.get("rank", 0), st.get("ring", ""), st.get("nnz", 0))

    results = []
    for f in files:
        p = os.path.join(sdir, f)
        try:
            st = check_file(p)
            st["origin"] = load(p).get("origin", "")
        except Exception as e:  # noqa: BLE001
            st = {"file": f, "verified": False, "errors": [f"load error: {e}"]}
        results.append(st)
    good = sorted((r for r in results if r["verified"]), key=order)
    bad = [r for r in results if not r["verified"]]
    L = ["# Fast matrix multiplication schemes: index", "",
         "Generated by `python3 tools/scheme_check.py --index schemes`. Every row below passed the exact check "
         "in `tools/scheme_check.py` (Python integers/Fractions; Gaussian rationals for ring C): "
         "sum_l U[l][i*K+k] V[l][k'*N+j] W[l][i'*N+j'] = [i=i'][k=k'][j=j'].", "",
         "Columns: nnz = nonzeros of U, V, W; adds = naive additions (row nnz-1 for each U and V row, "
         "column nnz-1 for each C entry, no common-subexpression elimination); single = rows of U / V that are "
         "a single +-1 entry; omega = 3 ln r / ln(MKN). Conventions of the sources and the conversion applied are "
         "stored per file in `source_convention`.", "",
         "| name | dims | rank | ring | nnz(U,V,W) | total | adds | single(U,V) | max abs coef | omega | verified | origin | source |",
         "|---|---|---|---|---|---|---|---|---|---|---|---|---|"]
    for r in good:
        L.append(f"| {r['file'][:-5]} | {'x'.join(map(str, r['dims']))} | {r['rank']} | {r['ring']} | "
                 f"{r['nnz_U']}, {r['nnz_V']}, {r['nnz_W']} | {r['nnz']} | {r['naive_adds']} | "
                 f"{r['single_U']}, {r['single_V']} | {r['max_abs']} | {r['omega']:.4f} | yes | "
                 f"{r['origin'].replace('|', '/')} | {_source_md(r['source'])} |")
    L += ["", "## Source conventions", "",
          "The exact check decided every conversion: each source was read with W indexed (i,j) and with W indexed (j,i) (plinopt also with column-major U/V), and the reading that passes is recorded in the file's `source_convention`:", "",
          "- arbenson/fast-matmul (`codegen/algorithms/*`: columns of U, V, W) and jgdumas/plinopt "
          "(`*_L/R/P.sms`, HM representation, P is MN x r): same convention as ours, C indexed (i,j).",
          "- google-deepmind/alphatensor (`factorizations_r.npz`), google-deepmind/alphaevolve_results (notebook), "
          "Kauers-Moosbauer `.m`/`.exp` (flips, mkauers/matrix-multiplication), the FMM catalogue Maple "
          "`TriadSet` files and Perminov's JSON: trilinear form tr(ABC), i.e. the third factor is C^T, indexed (j,i). "
          "Reading it as (i,j) fails the check (also for square formats).",
          "- `.exp` files only give the expanded product u*v*w; the importer chooses primitive integer u and v and "
          "puts the scalar into w, so integral schemes stay integral.",
          "- Perminov `*_reduced.json` files are straight-line programs with fresh variables; they are expanded "
          "back to plain U, V, W (the nnz/adds columns are for the expanded form).",
          "- Other orientations (e.g. <3,2,2> from <2,3,2>) are produced with the cyclic map <M,K,N> -> <K,N,M> "
          "(A'=B, B'=C^T, C'=A^T) and the transpose map <M,K,N> -> <N,K,M> (C^T = B^T A^T), which preserve total "
          "nnz, and are re-verified.",
          "- AlphaTensor's npz stores non-square formats as pickled object arrays; they are read with a "
          "restricted unpickler that only allows numpy array reconstruction."]
    L += ["", "## Coefficient sets", ""]
    for r in good:
        L.append(f"- `{r['file'][:-5]}`: {{{', '.join(r['coefs'])}}}")
    L += ["", "## Failures", ""]
    if bad:
        L.append("Files in `schemes/` that did NOT pass the exact check (not listed above):")
        for r in bad:
            L.append(f"- `{r['file']}`: {'; '.join(r['errors'][:3])}")
    else:
        L.append("All scheme files in `schemes/` pass the exact check.")
    fp = os.path.join(sdir, "meta", "import_log.json")
    if os.path.exists(fp):
        log = json.load(open(fp))
        L += ["", "Candidates rejected by `tools/import_schemes.py` (not written to `schemes/`):", ""]
        for f in log["failures"]:
            L.append(f"- {f['source']}: {f['reason']}")
        if not log["failures"]:
            L.append("- none")
        L += ["", "## Import summary", "",
              "Per requested format: verified candidates found across all sources (after mapping to the requested "
              "orientation), number of distinct schemes up to relabelling (index permutations, sign changes, "
              "per-product scaling, transpose/cyclic symmetry; detected by an invariant fingerprint), their total "
              "nonzeros in rank order, and the files kept (fewest nonzeros, integer before rational). "
              "Relabelled copies found in other sources are listed in each file's `also_in` field.", "",
              "| format | ring | verified candidates | distinct | distinct nnz | kept |", "|---|---|---|---|---|---|"]
        for e in log["summary"]:
            d = e["dims"]
            ring = {"ZQ": "Z (Q allowed)", "Q": "Q", "C": "C"}[e["category"]]
            L.append(f"| <{d[0]},{d[1]},{d[2]};{e['rank']}> | {ring} | {e['verified_candidates']} | {e['distinct']} | "
                     f"{', '.join(map(str, e['distinct_nnz']))} | {', '.join(e['kept']) or '-'} |")
    with open(os.path.join(sdir, "INDEX.md"), "w") as f:
        f.write("\n".join(L) + "\n")
    print(f"INDEX.md: {len(good)} verified, {len(bad)} failed")
    return not bad


def main(argv):
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__)
        return 0
    if argv[0] == "--selftest":
        return 0 if selftest() else 1
    if argv[0] == "--index":
        return 0 if write_index(argv[1] if len(argv) > 1 else "schemes") else 1
    as_json = argv[0] == "--json"
    files = argv[1:] if as_json else argv
    bad = 0
    for p in files:
        try:
            st = check_file(p)
        except Exception as e:  # noqa: BLE001
            st = {"file": os.path.basename(p), "verified": False, "errors": [f"load error: {e}"]}
        bad += not st["verified"]
        if as_json:
            print(json.dumps(st))
        else:
            tag = "PASS" if st["verified"] else "FAIL"
            print(f"{tag} {st['file']}: " + (fmt_stats(st) if "rank" in st else ""))
            for e in st["errors"]:
                print("    " + e)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
