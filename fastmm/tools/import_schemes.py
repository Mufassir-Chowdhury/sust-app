#!/usr/bin/env python3
"""Import published fast matrix-multiplication schemes into schemes/*.json.

Usage:  python3 -I tools/import_schemes.py SRC_DIR [OUT_DIR]

SRC_DIR holds shallow git clones (directory name -> repository):
  fastmatmul_git                      github.com/arbenson/fast-matmul
  jgdumas_plinopt                     github.com/jgdumas/plinopt
  google-deepmind_alphatensor         github.com/google-deepmind/alphatensor
  google-deepmind_alphaevolve_results github.com/google-deepmind/alphaevolve_results
  mkauers_matrix-multiplication       github.com/mkauers/matrix-multiplication
  jakobmoosbauer_flips                github.com/jakobmoosbauer/flips
  perminov                            github.com/dronperminov/FastMatrixMultiplication

All downloaded files are read as data only (no eval / no unrestricted unpickling).
Each candidate is converted to the schemes/ convention (see scheme_check.py); the
source convention is unknown a priori, so the few plausible readings (W indexed by
(i,j) or (j,i), U/V row- or column-major) are tried in order and the first one that
passes the exact check is kept and recorded.  For each requested format the (up to)
3 verified schemes that are distinct up to relabelling (see fingerprint()) are
written, ranked by (ring Z<Q<C, largest denominator,
total nonzeros).  Candidates that fail every reading are written to
OUT_DIR/meta/import_log.json (with a per-format summary).
"""
import ast
import glob
import io
import json
import math
import os
import pickle
import re
import subprocess
import sys
import zipfile
from fractions import Fraction

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import scheme_check as sc  # noqa: E402

REPOS = {
    "fastmatmul_git": "https://github.com/arbenson/fast-matmul",
    "jgdumas_plinopt": "https://github.com/jgdumas/plinopt",
    "google-deepmind_alphatensor": "https://github.com/google-deepmind/alphatensor",
    "google-deepmind_alphaevolve_results": "https://github.com/google-deepmind/alphaevolve_results",
    "mkauers_matrix-multiplication": "https://github.com/mkauers/matrix-multiplication",
    "jakobmoosbauer_flips": "https://github.com/jakobmoosbauer/flips",
    "perminov": "https://github.com/dronperminov/FastMatrixMultiplication",
}

# (dims, rank, ring category); category "ZQ" = integer preferred, rationals allowed
REQUESTED = [
    ((2, 2, 3), 11, "ZQ"), ((2, 3, 2), 11, "ZQ"), ((3, 2, 2), 11, "ZQ"),
    ((2, 3, 3), 15, "ZQ"), ((3, 3, 2), 15, "ZQ"),
    ((2, 2, 4), 14, "ZQ"), ((2, 4, 2), 14, "ZQ"),
    ((2, 3, 4), 20, "ZQ"),
    ((3, 3, 3), 23, "ZQ"),
    ((2, 4, 4), 26, "ZQ"), ((4, 2, 4), 26, "ZQ"),
    ((3, 3, 4), 29, "ZQ"),
    ((3, 4, 4), 38, "ZQ"),
    ((4, 4, 4), 48, "Q"), ((4, 4, 4), 48, "C"),
    ((4, 4, 4), 49, "ZQ"),
    ((3, 3, 6), 40, "ZQ"),
    ((2, 2, 5), 18, "ZQ"),
    ((5, 5, 5), 93, "ZQ"),
    ((4, 4, 5), 61, "ZQ"),
]
KEEP = 3
WANTED = {tuple(sorted(d)) for d, _, _ in REQUESTED}


def wanted(dims):
    return tuple(sorted(dims)) in WANTED


# --------------------------------------------------------------------------
# helpers
# --------------------------------------------------------------------------
class Raw:
    """A parsed candidate in its native layout: rows are products."""

    def __init__(self, dims, U, V, W, repo, path, origin, tag, native, readings=None):
        self.dims, self.U, self.V, self.W = tuple(dims), U, V, W
        self.repo, self.path, self.origin, self.tag, self.native = repo, path, origin, tag, native
        # readings: list of (tU, tV, tW) flags to try; True = transposed index order
        self.readings = readings or [(False, False, False), (False, False, True)]


def commit(src, repo):
    try:
        return subprocess.run(["git", "-C", os.path.join(src, repo), "rev-parse", "HEAD"],
                              capture_output=True, text=True, check=True).stdout.strip()[:12]
    except Exception:  # noqa: BLE001
        return "?"


def cols_to_rows(rows):
    """Factor matrix stored as (entries x rank) -> list over products."""
    return [list(col) for col in zip(*rows)]


def num(x):
    if isinstance(x, (int, Fraction)):
        return Fraction(x)
    if isinstance(x, float):
        f = Fraction(x)
        if f.denominator > 64:
            raise ValueError(f"non-dyadic float {x}")
        return f
    if isinstance(x, complex):
        re_, im = num(x.real), num(x.imag)
        return sc.QI(re_, im) if im else re_
    return sc.parse_coef(str(x))


def tr_rows(rows, R, C):
    return [sc._tr(r, R, C) for r in rows]


def resolve(raw):
    """Try the readings; return (scheme, description) or (None, reason)."""
    M, K, N = raw.dims
    r = len(raw.U)
    if not (len(raw.V) == r and len(raw.W) == r):
        return None, f"factor lengths differ: {len(raw.U)}, {len(raw.V)}, {len(raw.W)}"
    for row, n, F in ((raw.U[0], M * K, "U"), (raw.V[0], K * N, "V"), (raw.W[0], M * N, "W")):
        if len(row) != n:
            return None, f"{F} rows have length {len(row)}, expected {n} for dims {raw.dims}"
    last = ""
    for tU, tV, tW in raw.readings:
        # native column-major (k,i) for U means U[l][k*M+i]; convert to row-major (i,k)
        U = tr_rows(raw.U, K, M) if tU else raw.U
        V = tr_rows(raw.V, N, K) if tV else raw.V
        W = tr_rows(raw.W, N, M) if tW else raw.W
        s = sc.from_matrices(raw.dims, U, V, W)
        ok, errs = sc.verify(s)
        desc = ", ".join([
            "U " + ("(k,i)" if tU else "(i,k)"), "V " + ("(j,k)" if tV else "(k,j)"),
            "W " + ("(j,i) i.e. C^T / trilinear tr(ABC)" if tW else "(i,j)")])
        if ok:
            return s, desc
        last = f"{desc}: {errs[0] if errs else ''}"
    return None, "no reading verifies; last tried " + last


# --------------------------------------------------------------------------
# parsers
# --------------------------------------------------------------------------
def p_fastmatmul(src):
    """arbenson/fast-matmul codegen/algorithms: U (MK x r), '#', V (KN x r), '#', W (MN x r)."""
    repo = "fastmatmul_git"
    who = {"grey": "Benson & Ballard 2015 (numerical search)", "hk": "Hopcroft & Kerr 1971",
           "madan": "M. Musuvathi (SAT solver), via Benson-Ballard", "smirnov": "Smirnov 2013",
           "tichavsky_kovac": "Tichavsky, Phan & Cichocki 2017", "strassen": "Strassen 1969"}
    out, fails = [], []
    for path in sorted(glob.glob(os.path.join(src, repo, "codegen/algorithms/*"))):
        base = os.path.basename(path)
        if os.path.isdir(path) or "approx" in base or base.startswith("classical"):
            continue
        m = re.match(r"^(grey|hk|madan|smirnov|tichavsky_kovac)(\d)(\d)(\d)-(\d+)-(\d+)$", base)
        if m:
            fam, dims = m.group(1), tuple(int(m.group(i)) for i in (2, 3, 4))
        elif base in ("strassen", "grey-strassen"):
            fam, dims = ("strassen" if base == "strassen" else "grey"), (2, 2, 2)
        else:
            continue
        if not wanted(dims):
            continue
        text = open(path).read()
        if "Substitution" in text or "Eliminated" in text:
            continue
        lines = [ln.split() for ln in text.splitlines() if ln.strip() and not ln.lstrip().startswith("#")]
        M, K, N = dims
        if len(lines) != M * K + K * N + M * N:
            fails.append({"source": f"{REPOS[repo]}/{path.split(repo + '/')[1]}", "reason": f"{len(lines)} rows, expected {M*K+K*N+M*N}"})
            continue
        rows = [[Fraction(x) for x in ln] for ln in lines]
        U = cols_to_rows(rows[:M * K])
        V = cols_to_rows(rows[M * K:M * K + K * N])
        W = cols_to_rows(rows[M * K + K * N:])
        out.append(Raw(dims, U, V, W, repo, path, who[fam], f"fastmatmul-{base}",
                       "columns of U (MK x r), V (KN x r), W (MN x r), row-major entries"))
    return out, fails


def read_sms(path):
    rows = cols = None
    ent = {}
    for ln in open(path):
        ln = ln.strip()
        if not ln or ln.startswith("#"):
            continue
        t = ln.split()
        if rows is None:
            rows, cols = int(t[0]), int(t[1])
            continue
        i, j = int(t[0]), int(t[1])
        if i == 0 and j == 0:
            break
        ent[(i - 1, j - 1)] = Fraction(t[2])
    return [[ent.get((i, j), Fraction(0)) for j in range(cols)] for i in range(rows)]


def p_plinopt(src):
    """jgdumas/plinopt data/MxKxN_R_tag_{L,R,P}.sms: L (r x MK), R (r x KN), P (MN x r)."""
    repo = "jgdumas_plinopt"
    out, fails = [], []
    for lpath in sorted(glob.glob(os.path.join(src, repo, "data", "*_L.sms"))):
        base = os.path.basename(lpath)[:-6]
        m = re.match(r"^(\d+)x(\d+)x(\d+)_(?:(\d+)_)?(.*)$", base)
        if not m or "ALT" in base or "CoB" in base or "mod_2" in base:
            continue
        dims = tuple(int(m.group(i)) for i in (1, 2, 3))
        if not wanted(dims):
            continue
        tag = m.group(5) or "x"
        L = read_sms(lpath)
        R = read_sms(lpath[:-6] + "_R.sms")
        P = cols_to_rows(read_sms(lpath[:-6] + "_P.sms"))
        origin = {"4x4x4": "Dumas, Pernet & Sedoglavic 2025 (arXiv 2506.13242, hal-05112145)",
                  "3x3x6": "Smirnov 2013 (plinopt variant)", "3x6x3": "Smirnov 2013 (plinopt variant)",
                  "6x3x3": "Smirnov 2013 (plinopt variant)"}.get(f"{dims[0]}x{dims[1]}x{dims[2]}", "plinopt data")
        readings = [(False, False, False), (False, False, True), (True, True, False), (True, True, True)]
        out.append(Raw(dims, L, R, P, repo, lpath, origin, f"plinopt-{tag}",
                       "HM representation L (r x MK), R (r x KN), P (MN x r)", readings))
    return out, fails


class _SafeUnpickler(pickle.Unpickler):
    """Only numpy array reconstruction is allowed."""

    def find_class(self, module, name):
        import numpy as np
        allowed = {("numpy.core.multiarray", "_reconstruct"), ("numpy._core.multiarray", "_reconstruct"),
                   ("numpy", "ndarray"), ("numpy", "dtype"),
                   ("numpy.core.multiarray", "scalar"), ("numpy._core.multiarray", "scalar")}
        if (module, name) not in allowed:
            raise pickle.UnpicklingError(f"blocked {module}.{name}")
        if name == "_reconstruct":
            return np._core.multiarray._reconstruct
        if name == "scalar":
            return np._core.multiarray.scalar
        return getattr(np, name)


def _read_npy(data):
    import numpy as np
    fp = io.BytesIO(data)
    version = np.lib.format.read_magic(fp)
    shape, fortran, dtype = (np.lib.format.read_array_header_1_0 if version == (1, 0)
                             else np.lib.format.read_array_header_2_0)(fp)
    if dtype.hasobject:
        return _SafeUnpickler(fp, encoding="latin1").load()
    arr = np.frombuffer(fp.read(), dtype=dtype).reshape(shape, order="F" if fortran else "C")
    return arr


def p_alphatensor(src):
    """google-deepmind/alphatensor algorithms/factorizations_r.npz: key 'M,K,N' -> (u, v, w)."""
    repo = "google-deepmind_alphatensor"
    path = os.path.join(src, repo, "algorithms", "factorizations_r.npz")
    out, fails = [], []
    z = zipfile.ZipFile(path)
    for nm in z.namelist():
        dims = tuple(int(x) for x in nm[:-4].split(","))
        if not wanted(dims):
            continue
        arr = _read_npy(z.read(nm))
        u, v, w = arr[0], arr[1], arr[2]
        U = [[num(x) for x in col] for col in u.T.tolist()]
        V = [[num(x) for x in col] for col in v.T.tolist()]
        W = [[num(x) for x in col] for col in w.T.tolist()]
        out.append(Raw(dims, U, V, W, repo, path + f"[{nm[:-4]}]",
                       "Fawzi et al., Nature 610 (2022), AlphaTensor", "alphatensor",
                       "factor matrices u (MK x r), v (KN x r), w (MN x r) [index order of w not documented]"))
    return out, fails


def _np_arrays(text):
    """Extract the bracketed literal of each np.array( ... ) call."""
    res = []
    for m in re.finditer(r"np\.array\(", text):
        i = m.end()
        assert text[i] == "["
        depth = 0
        for j in range(i, len(text)):
            if text[j] == "[":
                depth += 1
            elif text[j] == "]":
                depth -= 1
                if depth == 0:
                    res.append(ast.literal_eval(text[i:j + 1]))
                    break
    return res


def p_alphaevolve(src):
    """google-deepmind/alphaevolve_results mathematical_results.ipynb decomposition_XYZ cells.

    Their check builds T[i*m+j][j*p+k][k*n+i]: third factor indexed (col of C, row of C).
    """
    repo = "google-deepmind_alphaevolve_results"
    path = os.path.join(src, repo, "mathematical_results.ipynb")
    nb = json.load(open(path))
    out, fails = [], []
    for c in nb["cells"]:
        s = "".join(c["source"])
        m = re.search(r"^decomposition_(\d)(\d)(\d) = \(", s, re.M)
        if not m or c["cell_type"] != "code":
            continue
        dims = tuple(int(m.group(i)) for i in (1, 2, 3))
        if not wanted(dims):
            continue
        f1, f2, f3 = _np_arrays(s)
        U, V, W = ([[num(x) for x in col] for col in zip(*f)] for f in (f1, f2, f3))
        out.append(Raw(dims, U, V, W, repo, path + f"[decomposition_{''.join(map(str, dims))}]",
                       "Novikov et al. 2025, AlphaEvolve", "alphaevolve",
                       "factor matrices (MK x r), (KN x r), (NM x r) with third index k*n+i",
                       [(False, False, True), (False, False, False)]))
    return out, fails


def _mathematica_list(text):
    t = text.replace("{", "[").replace("}", "]")
    t = re.sub(r"(-?\d+)\s*/\s*(\d+)", r'"\1/\2"', t)
    return json.loads(t)


def parse_m(path):
    """Kauers-Moosbauer .m: list of {A (MxK), B (KxN), C (NxM)} per product."""
    data = _mathematica_list(open(path).read())
    U, V, W = [], [], []
    for a, b, cmat in data:
        U.append([num(x) for row in a for x in row])
        V.append([num(x) for row in b for x in row])
        # C given as N x M (trilinear convention): flatten as is, reading (j,i)
        W.append([num(x) for row in cmat for x in row])
    M, K, N = len(data[0][0]), len(data[0][1]), len(data[0][1][0])
    return (M, K, N), U, V, W


# ---- tiny exact polynomial parser for .exp lines ----
_TOK = re.compile(r"\s*(?:(\d+)|([abc])(\d)(\d)|(.))")


def _tokens(s):
    toks = []
    for m in _TOK.finditer(s):
        if m.group(1):
            toks.append(("n", int(m.group(1))))
        elif m.group(2):
            toks.append(("v", (m.group(2), int(m.group(3)) - 1, int(m.group(4)) - 1)))
        elif m.group(5) and not m.group(5).isspace():
            toks.append(("o", m.group(5)))
    return toks


def _pmul(p, q):
    r = {}
    for a, x in p.items():
        for b, y in q.items():
            k = tuple(sorted(a + b))
            r[k] = r.get(k, 0) + x * y
    return {k: v for k, v in r.items() if v}


def _padd(p, q, sgn=1):
    r = dict(p)
    for k, v in q.items():
        r[k] = r.get(k, 0) + sgn * v
    return {k: v for k, v in r.items() if v}


def parse_poly(s):
    toks = _tokens(s)
    pos = [0]

    def peek():
        return toks[pos[0]] if pos[0] < len(toks) else ("o", None)

    def eat():
        pos[0] += 1
        return toks[pos[0] - 1]

    def expr():
        p = term()
        while peek() in (("o", "+"), ("o", "-")):
            op = eat()[1]
            p = _padd(p, term(), 1 if op == "+" else -1)
        return p

    def term():
        p = factor()
        while True:
            t = peek()
            if t in (("o", "*"), ("o", "/")):
                op = eat()[1]
            elif t == ("o", "(") or t[0] in ("v", "n"):
                op = "*"  # juxtaposition = multiplication
            else:
                break
            q = factor()
            if op == "*":
                p = _pmul(p, q)
            else:
                if list(q.keys()) != [()]:
                    raise ValueError("division by non-constant")
                p = {k: v / q[()] for k, v in p.items()}
        return p

    def factor():
        t = eat()
        if t == ("o", "-"):
            return {k: -v for k, v in factor().items()}
        if t == ("o", "+"):
            return factor()
        if t[0] == "n":
            return {(): Fraction(t[1])}
        if t[0] == "v":
            return {(t[1],): Fraction(1)}
        if t == ("o", "("):
            p = expr()
            if eat() != ("o", ")"):
                raise ValueError("missing )")
            return p
        raise ValueError(f"unexpected token {t}")

    p = expr()
    if pos[0] != len(toks):
        raise ValueError("trailing tokens")
    return p


def parse_exp(path, dims=None):
    """Kauers-Moosbauer .exp: one product (a-form)*(b-form)*(c-form) per line, c indexed (j,i)."""
    lines = [ln for ln in open(path).read().splitlines() if ln.strip()]
    polys = []
    for no, ln in enumerate(lines, 1):
        try:
            polys.append(parse_poly(ln))
        except ValueError as e:
            raise ValueError(f"line {no}: {e}: {ln.strip()[:80]!r}") from None
    if dims is None:
        M = 1 + max(v[1] for p in polys for mono in p for v in mono if v[0] == "a")
        K = 1 + max(v[2] for p in polys for mono in p for v in mono if v[0] == "a")
        N = 1 + max(v[2] for p in polys for mono in p for v in mono if v[0] == "b")
        dims = (M, K, N)
    M, K, N = dims
    U, V, W = [], [], []
    for p in polys:
        if any(len(mono) != 3 or [v[0] for v in mono] != ["a", "b", "c"] for mono in p):
            raise ValueError("line is not trilinear in a, b, c")
        (a0, b0, c0), t = next(iter(p.items()))
        u = [Fraction(0)] * (M * K)
        v = [Fraction(0)] * (K * N)
        w = [Fraction(0)] * (M * N)
        for (a, b, c), x in p.items():
            if b == b0 and c == c0:
                u[a[1] * K + a[2]] = x
            if a == a0 and c == c0:
                v[b[1] * N + b[2]] = x / t
            if a == a0 and b == b0:
                w[c[1] * M + c[2]] = x / t  # c_{j,i} -> native index j*M+i
        # the expanded polynomial fixes only the product u*v*w; choose primitive integer u, v
        # (so an integral source scheme stays integral) and push the scalars into w
        u, cu = _primitive(u)
        v, cv = _primitive(v)
        U.append(u)
        V.append(v)
        W.append([x * cu * cv for x in w])
    return dims, U, V, W


def _primitive(vec):
    den = 1
    for x in vec:
        den = den * x.denominator // math.gcd(den, x.denominator)
    g = 0
    for x in vec:
        g = math.gcd(g, int(x * den))
    c = Fraction(g, den)
    return [x / c for x in vec], c


def p_flips(src):
    """jakobmoosbauer/flips solutions/*-mod0.m (and .exp when no .m), Kauers & Moosbauer."""
    repo = "jakobmoosbauer_flips"
    out, fails = [], []
    files = sorted(glob.glob(os.path.join(src, repo, "solutions", "*-mod0*.m")))
    have = {f[:-2] for f in files}
    files += [f for f in sorted(glob.glob(os.path.join(src, repo, "solutions", "*-mod0*.exp"))) if f[:-4] not in have]
    for path in files:
        base = os.path.basename(path)
        m = re.match(r"^(\d)(\d)(\d)-(\d+)-mod0", base)
        dims = tuple(int(m.group(i)) for i in (1, 2, 3))
        if not wanted(dims):
            continue
        if path.endswith(".m"):
            d, U, V, W = parse_m(path)
        else:
            d, U, V, W = parse_exp(path)
        out.append(Raw(d, U, V, W, repo, path, "Kauers & Moosbauer, flip graphs (2022/23)", "flips",
                       "Mathematica/expression list {A (MxK), B (KxN), C (NxM)}: trilinear, C indexed (j,i)",
                       [(False, False, True), (False, False, False)]))
    return out, fails


def p_kauers(src):
    """mkauers/matrix-multiplication structured/*.exp (Kauers, Moosbauer, Wood et al.)."""
    repo = "mkauers_matrix-multiplication"
    out, fails = [], []
    for path in sorted(glob.glob(os.path.join(src, repo, "structured", "*.exp"))):
        base = os.path.basename(path)
        m = re.search(r"(?:^|-)(\d)(\d)(\d)(?:-|\.exp|r)", base)
        if not m:
            continue
        dims = tuple(int(m.group(i)) for i in (1, 2, 3))
        if not wanted(dims) or "mod2" in base:
            continue
        try:
            d, U, V, W = parse_exp(path)  # file names give the sorted format; infer orientation
            if sorted(d) != sorted(dims):
                raise ValueError(f"content dims {d} do not match name {dims}")
        except Exception as e:  # noqa: BLE001
            fails.append({"source": f"{REPOS[repo]}/{path.split(repo + '/')[1]}", "reason": f"parse error: {e}"})
            continue
        out.append(Raw(d, U, V, W, repo, path, "Kauers et al., flip-graph schemes (mkauers/matrix-multiplication)",
                       "kauers-structured", "expression list (a-form)*(b-form)*(c-form), c indexed (j,i)",
                       [(False, False, True), (False, False, False)]))
    return out, fails


def _perminov_origin(rel):
    table = [
        ("known/tensor/", "FMM catalogue (fmm.univ-lille.fr, Sedoglavic) copy", "fmmcat"),
        ("known/classic/", "classic scheme", "classic"),
        ("known/alpha_tensor/", "AlphaTensor (Fawzi et al. 2022), Perminov copy", "alphatensor-pm"),
        ("known/alpha_evolve/", "AlphaEvolve (Novikov et al. 2025), Perminov copy", "alphaevolve-pm"),
        ("known/fmm_add_reduction/", "Moosbauer et al. schemes, addition-reduction collection", "moosbauer"),
        ("known/jakobmoosbauer_flips/", "Kauers & Moosbauer flips", "flips-pm"),
        ("known/jakobmoosbauer_symmetric_flips/", "Moosbauer & Poole, flip graphs with symmetry (2025)", "moosbauer-poole"),
        ("known/meta_flip_graph/", "Kauers & Wood, meta flip graph", "kauers-wood"),
        ("known/a_60_addition/", "Perminov, 60-addition 3x3 scheme", "perminov-a60"),
        ("results/addition_reduced_ZT/", "Perminov 2025-26 (ternary, addition-reduced search)", "perminov-addred"),
        ("results/naive_addition_reduced_ZT/", "Perminov 2025-26 (ternary, naive-addition reduced)", "perminov-naive"),
        ("results/serendipitous_base/", "Perminov 2025-26 (serendipitous base)", "perminov-seren"),
        ("results/ZT/", "Perminov 2025-26 (ternary meta flip graph)", "perminov-zt"),
        ("results/Z/", "Perminov 2025-26", "perminov-z"),
        ("results/Q/", "Perminov 2025-26", "perminov-q"),
    ]
    for pre, who, tag in table:
        if rel.startswith("schemes/" + pre):
            return who, tag
    return "Perminov repository", "perminov"


def parse_tensor_mpl(path):
    """Maple TriadSet([Triad([Matrix(M,K,[..]), Matrix(K,N,[..]), Matrix(N,M,[..])]), ...])."""
    text = open(path, encoding="utf-8").read()
    mats = re.findall(r"Matrix\((\d+), ?(\d+), ?(\[\[.*?\]\])\)", text.split("Tensor:=", 1)[1])
    U, V, W = [], [], []
    for t in range(0, len(mats), 3):
        trip = []
        for r_, c_, body in mats[t:t + 3]:
            body = re.sub(r"(-?\d+)/(\d+)", r'"\1/\2"', body)
            trip.append([num(x) for row in json.loads(body) for x in row])
        U.append(trip[0])
        V.append(trip[1])
        W.append(trip[2])
    M, K = int(mats[0][0]), int(mats[0][1])
    N = int(mats[1][1])
    return (M, K, N), U, V, W


def parse_perminov_json(path):
    d = json.load(open(path))
    dims = tuple(d["n"]) if isinstance(d["n"], list) else (d["n"],) * 3
    M, K, N = dims
    if "u_fresh" in d:  # addition-reduced straight-line form with fresh variables
        def expand(exprs, fresh, nreal):
            fv = {}
            for i, f in enumerate(fresh):
                fv[nreal + i] = f
                fv[-(nreal + i)] = [{"index": x["index"], "value": -x["value"]} for x in f]

            def rep(expr, mult):
                outl = []
                for x in expr:
                    idx, val = x["index"], num(x["value"]) * mult
                    if idx in fv:
                        outl += rep(fv[idx], val)
                    else:
                        outl.append((idx, val))
                return outl
            return [rep(e, Fraction(1)) for e in exprs]
        r = d["m"]
        U = [[Fraction(0)] * (M * K) for _ in range(r)]
        V = [[Fraction(0)] * (K * N) for _ in range(r)]
        W = [[Fraction(0)] * (M * N) for _ in range(r)]
        for l, terms in enumerate(expand(d["u"], d["u_fresh"], M * K)):
            for idx, val in terms:
                U[l][idx] += val
        for l, terms in enumerate(expand(d["v"], d["v_fresh"], K * N)):
            for idx, val in terms:
                V[l][idx] += val
        for p, terms in enumerate(expand(d["w"], d["w_fresh"], r)):
            for l, val in terms:
                W[l][p] += val
        return dims, U, V, W
    U = [[num(x) for x in row] for row in d["u"]]
    V = [[num(x) for x in row] for row in d["v"]]
    W = [[num(x) for x in row] for row in d["w"]]
    return dims, U, V, W


def p_perminov(src):
    """dronperminov/FastMatrixMultiplication: every scheme listed in schemes/status.json."""
    repo = "perminov"
    root = os.path.join(src, repo)
    status = json.load(open(os.path.join(root, "schemes", "status.json")))
    out, fails, seen = [], [], set()
    for key, e in status.items():
        dims0 = tuple(int(x) for x in key.split("x"))
        if not wanted(dims0):
            continue
        for ring, lst in e["schemes"].items():
            for item in lst:
                for rel in [item["source"]] + item.get("duplicates", []):
                    path = os.path.join(root, rel)
                    if rel in seen or not os.path.exists(path):
                        continue
                    seen.add(rel)
                    who, tag = _perminov_origin(rel)
                    try:
                        if rel.endswith("tensor.mpl"):
                            d, U, V, W = parse_tensor_mpl(path)
                            native = "Maple TriadSet: Triad([A (MxK), B (KxN), C (NxM)])"
                        elif rel.endswith(".json"):
                            d, U, V, W = parse_perminov_json(path)
                            native = "JSON u (MK), v (KN), w (NM, index j*M+i)" + (
                                "; straight-line form with fresh variables expanded" if "reduced" in rel else "")
                        elif rel.endswith(".m"):
                            d, U, V, W = parse_m(path)
                            native = "Mathematica {A (MxK), B (KxN), C (NxM)}"
                        elif rel.endswith(".exp"):
                            d, U, V, W = parse_exp(path)
                            native = "expression list, c indexed (j,i)"
                        else:
                            continue
                    except Exception as ex:  # noqa: BLE001
                        fails.append({"source": f"{REPOS[repo]}/{rel}", "reason": f"parse error: {ex}"})
                        continue
                    stem = os.path.basename(rel).rsplit(".", 1)[0]
                    out.append(Raw(d, U, V, W, repo, path, who, f"{tag}", native,
                                   [(False, False, True), (False, False, False)]))
                    out[-1].stem = stem
    return out, fails


# --------------------------------------------------------------------------
# selection
# --------------------------------------------------------------------------
def canon_key(s):
    """Product-order and per-term scaling invariant key (detects duplicates)."""
    terms = []
    for u, v, w in zip(s["U"], s["V"], s["W"]):
        fu = next(c for c in u if c)
        fv = next(c for c in v if c)
        terms.append((tuple(sc.coef_str(c / fu) for c in u),
                      tuple(sc.coef_str(c / fv) for c in v),
                      tuple(sc.coef_str(c * fu * fv) for c in w)))
    return tuple(sorted(terms))


def sym_key(s):
    """canon_key minimised over the transpose/cyclic symmetries that preserve the dims."""
    keys = []
    for tb in (False, True):
        c = sc.transpose(s) if tb else s
        for _ in range(3):
            if list(c["dims"]) == list(s["dims"]):
                keys.append(canon_key(c))
            c = sc.cyclic(c)
    return min(keys)


def _rank(vec, R, C):
    """Exact rank of a row-major R x C coefficient vector (Fraction or QI entries)."""
    A = [list(vec[i * C:(i + 1) * C]) for i in range(R)]
    rk, col = 0, 0
    while rk < R and col < C:
        piv = next((i for i in range(rk, R) if A[i][col]), None)
        if piv is None:
            col += 1
            continue
        A[rk], A[piv] = A[piv], A[rk]
        for i in range(rk + 1, R):
            if A[i][col]:
                f = A[i][col] / A[rk][col]
                A[i] = [x - f * y for x, y in zip(A[i], A[rk])]
        rk, col = rk + 1, col + 1
    return rk


def _fp(s):
    M, K, N = s["dims"]
    def mag(c):  # exact |c| for real coefficients, rounded modulus for complex ones
        return abs(c) if not sc.is_complex(c) else Fraction(round(abs(c) * 10**9), 10**9)

    terms = []
    for u, v, w in zip(s["U"], s["V"], s["W"]):
        # scale-normalised magnitudes: u/max|u|, v/max|v|, w*max|u|*max|v|
        mu = max(mag(c) for c in u)
        mv = max(mag(c) for c in v)
        terms.append((sum(1 for c in u if c), sum(1 for c in v if c), sum(1 for c in w if c),
                      _rank(u, M, K), _rank(v, K, N), _rank(w, M, N),
                      tuple(sorted(sc.coef_str(mag(c) / mu) for c in u if c)),
                      tuple(sorted(sc.coef_str(mag(c) / mv) for c in v if c)),
                      tuple(sorted(sc.coef_str(mag(c) * mu * mv) for c in w if c))))

    def prof(F, R, C, axis):
        cnt = [0] * (R if axis == 0 else C)
        for row in s[F]:
            for p, c in enumerate(row):
                if c:
                    cnt[p // C if axis == 0 else p % C] += 1
        return tuple(sorted(cnt))
    return (tuple(sorted(terms)), prof("U", M, K, 0), prof("U", M, K, 1), prof("V", K, N, 0),
            prof("V", K, N, 1), prof("W", M, N, 0), prof("W", M, N, 1))


def fingerprint(s):
    """Invariant under product order, per-term scaling, index permutations and sign
    changes of A, B, C, and the dims-preserving transpose/cyclic symmetries.  Schemes
    with equal fingerprints are treated as relabelled copies of one scheme."""
    fps = []
    for tb in (False, True):
        c = sc.transpose(s) if tb else s
        for _ in range(3):
            if list(c["dims"]) == list(s["dims"]):
                fps.append(_fp(c))
            c = sc.cyclic(c)
    return min(fps)


def maxden(s):
    d = 1
    for F in "UVW":
        for row in s[F]:
            for c in row:
                if isinstance(c, sc.QI):
                    d = max(d, c.re.denominator, c.im.denominator)
                else:
                    d = max(d, Fraction(c).denominator)
    return d


RING_ORDER = {"Z": 0, "Q": 1, "C": 2}


def main(src, outdir):
    commits = {r: commit(src, r) for r in REPOS}
    raws, fails = [], []
    for parser in (p_fastmatmul, p_plinopt, p_alphatensor, p_alphaevolve, p_flips, p_kauers, p_perminov):
        try:
            o, f = parser(src)
        except Exception as e:  # noqa: BLE001
            o, f = [], [{"source": parser.__doc__.split(":")[0], "reason": f"parser crashed: {e!r}"}]
        print(f"{parser.__name__}: {len(o)} candidates, {len(f)} parse failures", file=sys.stderr)
        raws += o
        fails += f

    verified = []
    for raw in raws:
        rel = raw.path.split(raw.repo + "/", 1)[1]
        srcstr = f"{REPOS[raw.repo]} @ {commits[raw.repo]} : {rel}"
        s, desc = resolve(raw)
        if s is None:
            fails.append({"source": srcstr, "dims": list(raw.dims), "rank": len(raw.U), "reason": desc})
            continue
        s["origin"] = raw.origin
        s["source"] = srcstr
        s["source_convention"] = f"{raw.native}; read as {desc}"
        s["_tag"] = raw.tag
        verified.append(s)
    print(f"verified {len(verified)} / {len(raws)} candidates", file=sys.stderr)

    # classical 49: tensor squares of the hand-written 2x2 schemes (listed first so that
    # relabelled copies found elsewhere are reported under this name)
    for nm in ("strassen", "winograd"):
        base = sc.load(os.path.join(outdir, nm + ".json"))
        s = sc.compose(base, base)
        s["origin"] = f"{base['source'].split(';')[0]}, applied twice"
        s["source"] = f"compose(schemes/{nm}.json, schemes/{nm}.json) via tools/scheme_check.py"
        s["source_convention"] = "native (tensor product, outer 2x2 blocks from first factor)"
        s["_tag"] = f"{nm}-squared"
        verified.insert(0, s)

    for f in os.listdir(outdir):  # drop files from earlier imports
        if re.match(r"^\d+x\d+x\d+_r\d+_.*\.json$", f) or f in ("import_failures.json", "import_log.json"):
            os.remove(os.path.join(outdir, f))
    written, summary = [], []
    for dims, rank, cat in REQUESTED:
        pool = []
        for s in verified:
            if sorted(s["dims"]) != sorted(dims) or s["rank"] != rank:
                continue
            if cat == "ZQ" and s["ring"] not in ("Z", "Q"):
                continue
            if cat in ("Q", "C") and s["ring"] != cat:
                continue
            t, how = sc.permute_to(s, dims)
            t = sc.Scheme(t)  # never mutate the shared verified candidate
            for k in ("origin", "source", "source_convention", "_tag"):
                t[k] = s[k]
            if how != "identity":
                t["source_convention"] += f"; then mapped {tuple(s['dims'])} -> {tuple(dims)} by {how} symmetry"
            ok, errs = sc.verify(t)
            assert ok, (t["source"], errs)
            st = sc.stats(t)
            pool.append((RING_ORDER[t["ring"]], maxden(t), st["nnz"], st["naive_adds"], len(pool), t))
        pool.sort(key=lambda x: x[:5])
        classes = {}  # fingerprint -> [best scheme, other sources]
        for item in pool:
            k = fingerprint(item[-1])
            if k in classes:
                if item[-1]["source"] not in classes[k][1] + [classes[k][0]["source"]]:
                    classes[k][1].append(item[-1]["source"])
            else:
                classes[k] = [item[-1], []]
        chosen = list(classes.values())[:KEEP]
        names = []
        for t, others in chosen:
            M, K, N = dims
            t = sc.Scheme(t)
            base = f"{M}x{K}x{N}_r{rank}_{t.pop('_tag')}"
            base = re.sub(r"[^A-Za-z0-9_.+-]", "-", base)
            name, i = base, 2
            while name in written:
                name, i = f"{base}-{i}", i + 1
            t["name"] = name
            order = {k: t[k] for k in ("name", "dims", "rank", "source", "ring", "origin", "source_convention")}
            if others:
                order["also_in"] = others
            out = sc.Scheme(order)
            out.update({"U": t["U"], "V": t["V"], "W": t["W"]})
            sc.dump(out, os.path.join(outdir, name + ".json"))
            written.append(name)
            names.append(name)
        summary.append({"dims": list(dims), "rank": rank, "category": cat, "verified_candidates": len(pool),
                        "distinct": len(classes),
                        "distinct_nnz": [sc.stats(c[0])["nnz"] for c in classes.values()],
                        "kept": names})

    os.makedirs(os.path.join(outdir, "meta"), exist_ok=True)
    with open(os.path.join(outdir, "meta", "import_log.json"), "w") as f:
        json.dump({"summary": summary, "failures": fails}, f, indent=1)
    for e in summary:
        d = e["dims"]
        print(f"<{d[0]},{d[1]},{d[2]};{e['rank']}> [{e['category']}]: {e['verified_candidates']} verified candidates, "
              f"{e['distinct']} distinct up to relabelling (nnz {e['distinct_nnz']}), kept {e['kept']}", file=sys.stderr)
    print(f"wrote {len(written)} schemes, {len(fails)} failures logged", file=sys.stderr)


if __name__ == "__main__":
    src = sys.argv[1]
    outdir = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "schemes")
    main(src, outdir)
