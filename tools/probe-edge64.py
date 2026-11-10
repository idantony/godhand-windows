"""Offline exact edge arithmetic check; never executes game or GPU code.

Compares two-word uint32 arithmetic with an independent unbounded integer
determinant. Optional private packet input is read-only; report must be new.
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path

MASK = (1 << 32) - 1


def product_u32(a, b):
    a0, a1, b0, b1 = a & 65535, a >> 16, b & 65535, b >> 16
    low = a0 * b0
    c1, c2 = a0 * b1, a1 * b0
    high = a1 * b1 + (c1 >> 16) + (c2 >> 16)
    old = low
    low = (low + ((c1 << 16) & MASK)) & MASK
    high += low < old
    old = low
    low = (low + ((c2 << 16) & MASK)) & MASK
    high += low < old
    return low, high & MASK


def negate(a):
    low = (-a[0]) & MASK
    return low, ((~a[1]) + (low == 0)) & MASK


def product_s32(a, b):
    # Unsigned magnitude also avoids abs(INT_MIN) in a potential HLSL port.
    ma = ((-a) if a < 0 else a) & MASK
    mb = ((-b) if b < 0 else b) & MASK
    value = product_u32(ma, mb)
    return negate(value) if (a < 0) != (b < 0) else value


def subtract(a, b):
    return (a[0] - b[0]) & MASK, (a[1] - b[1] - (a[0] < b[0])) & MASK


def signed(a):
    value = (a[1] << 32) | a[0]
    return value - (1 << 64) if a[1] & (1 << 31) else value


def edge_pair(a, b, p):
    return subtract(product_s32(b[0] - a[0], p[1] - a[1]),
                    product_s32(b[1] - a[1], p[0] - a[0]))


def edge_oracle(a, b, p):
    return ((b[0] - a[0]) * (p[1] - a[1])
            - (b[1] - a[1]) * (p[0] - a[0]))


def inclusive(a, b):
    x, y = b[0] - a[0], b[1] - a[1]
    return y < 0 or (y == 0 and x > 0)


def inside(triangle, p):
    a, b, c = triangle
    area = edge_oracle(a, b, c)
    if area == 0:
        return False
    if area < 0:
        b, c, area = c, b, -area
    edges = [(b, c), (c, a), (a, b)]
    values = [edge_oracle(x, y, p) for x, y in edges]
    pairs = [edge_pair(x, y, p) for x, y in edges]
    assert all(signed(x) == y for x, y in zip(pairs, values))
    covered = all(v > 0 or (v == 0 and inclusive(*e))
                  for v, e in zip(values, edges))
    if covered:
        # All vertices share one offset and the same packed16 square.
        assert area <= 65535**2 < (1 << 32)
        assert all(0 <= v <= area for v in values)
        assert all(pair[1] == 0 for pair in pairs)
    return covered


def packet_check(path):
    raw = path.read_bytes()
    d = json.loads(raw)
    vertices, indices = d["vertices"], d["indices"]
    assert len(indices) % 3 == 0 and all(0 <= i < len(vertices) for i in indices)
    xy = [((v["xyz_fixed4"][0] & 65535) - d["offset_x"],
           (v["xyz_fixed4"][1] & 65535) - d["offset_y"]) for v in vertices]
    covered = 0
    for i in range(0, len(indices), 3):
        tri = [xy[j] for j in indices[i:i+3]]
        # Exact conservative viewport bounds, not vertex clamping.
        x0, x1 = max(0, min(p[0] for p in tri) // 16), min(511, max(p[0] for p in tri) // 16)
        y0, y1 = max(0, min(p[1] for p in tri) // 16), min(447, max(p[1] for p in tri) // 16)
        for y in range(y0, y1+1):
            for x in range(x0, x1+1):
                covered += inside(tri, (x*16, y*16))
    used = set(indices)
    return {"sha256": hashlib.sha256(raw).hexdigest(), "triangles": len(indices)//3,
            "used_vertices": len(used), "xy_min": [min(xy[i][k] for i in used) for k in range(2)],
            "xy_max": [max(xy[i][k] for i in used) for k in range(2)],
            "covered_fragments_before_tests": covered,
            "old_xy_rejections": [i for i in sorted(used) if any(v < -4096 or v > 12288 for v in xy[i])]}


def main():
    if not __debug__:
        raise SystemExit("verification requires Python without -O")
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--packet", type=Path)
    ap.add_argument("--output", type=Path, required=True)
    ap.add_argument("--samples", type=int, default=100000)
    args = ap.parse_args()
    if args.output.exists() or not 1 <= args.samples <= 10000000:
        ap.error("output must be absent and samples must be 1..10000000")
    rng = random.Random(0x160064)
    largest = 0
    for _ in range(args.samples):
        a, b = rng.randrange(1 << 32), rng.randrange(1 << 32)
        pair = product_u32(a, b)
        assert (pair[1] << 32) | pair[0] == a*b
        offset = (rng.randrange(65536), rng.randrange(65536))
        triangle = [(rng.randrange(65536)-offset[0], rng.randrange(65536)-offset[1]) for _ in range(3)]
        pixel = (rng.randrange(512)*16, rng.randrange(448)*16)
        for x, y in [(triangle[0], triangle[1]), (triangle[1], triangle[2]), (triangle[2], triangle[0])]:
            actual, expected = signed(edge_pair(x, y, pixel)), edge_oracle(x, y, pixel)
            assert actual == expected
            largest = max(largest, abs(actual))
        inside(triangle, pixel)
    a, b, c = (0, 0), (65535, 0), (0, 65535)
    exact = edge_oracle(a, b, c)
    old = ((exact + (1 << 31)) & MASK) - (1 << 31)
    assert exact == 4294836225 and old == -131071 and signed(edge_pair(a, b, c)) == exact
    report = {"schema": 1, "scope": "Offline exact integer edge verification; no GPU/frame claim",
              "samples": args.samples, "product_and_edges_pass": True,
              "largest_random_abs_edge": largest,
              "signed32_counterexample": {"a": a, "b": b, "c": c, "exact": exact, "signed32": old},
              "coverage_bound": "same packed16 square => positive area<=65535^2<2^32; covered edges0..area => highword0",
              "float_weights": "After exact sign/zero/top-left coverage, cast uint low words only. Covered values fit uint32; same rounding as old positive int32 domain.",
              "packet": packet_check(args.packet) if args.packet else None}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    print(json.dumps({"status": "PASS", "samples": args.samples, "packet": report["packet"]}))


if __name__ == "__main__":
    main()
