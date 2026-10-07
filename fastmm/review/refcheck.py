import struct, sys
from fractions import Fraction as F
data = open(sys.argv[1], "rb").read()
n = struct.unpack_from("<Q", data, 0)[0]; off = 8; worst = 0; worst_rel = 0
while off < len(data):
    a = struct.unpack_from(f"<{n}d", data, off); off += 8 * n
    b = struct.unpack_from(f"<{n}d", data, off); off += 8 * n
    hi, lo, ab = struct.unpack_from("<3d", data, off); off += 24
    exact = sum(F(x) * F(y) for x, y in zip(a, b))
    err = abs(F(hi) + F(lo) - exact)
    worst = max(worst, float(err / F(ab)) if ab else 0)
    if exact: worst_rel = max(worst_rel, float(err / abs(exact)))
print(f"reference error vs exact: max |ref-exact|/(|A||B|) = {worst:.3e}, max |ref-exact|/|exact| = {worst_rel:.3e}")
