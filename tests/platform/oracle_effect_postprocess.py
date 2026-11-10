"""Independent FX13 packet pixel oracle for the explicit native sampling policy.

Reads the optional numeric dump from test_effect_postprocess_reference and
compares all four raw surfaces. Fractions prove the chosen rational/separable
reference, not undocumented GS hardware filtering precision.
"""
import argparse
from fractions import Fraction
import hashlib
import json
from pathlib import Path


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("prefix", type=Path)
    ap.add_argument("--output", type=Path, required=True)
    a = ap.parse_args()
    if not __debug__ or a.output.exists():
        ap.error("requires normal Python and a new output path")
    text = Path(str(a.prefix) + "-packets.txt").read_text()
    packets = []
    for line in text.splitlines():
        v = line.split()
        if v[0] == "packet":
            i, kind, source, destination, primitive = map(int, v[1:6])
            assert i == len(packets)
            packets.append(dict(kind=kind, source=source, destination=destination,
                                primitive=primitive, tex0=int(v[6], 16), test=int(v[7], 16),
                                alpha=int(v[8], 16), mask=int(v[9], 16), count=int(v[10]), quads=[]))
        else:
            assert v[0] == "quad"
            i, j = map(int, v[1:3])
            assert j == len(packets[i]["quads"])
            packets[i]["quads"].append(tuple(map(int, v[3:11])) + (int(v[11], 16),))
    assert [p["kind"] for p in packets] == [0, 0, 0, 1, 2, 2, 3]
    assert [p["destination"] for p in packets] == [1, 2, 3, 1, 2, 3, 0]
    dims = [(512, 448)] + [(128, 128)] * 3
    surfaces = [[bytearray([0xa5] * (w*4)) for _ in range(h)] for w, h in dims]
    for y in range(448):
        for x in range(512):
            surfaces[0][y][x*4:x*4+4] = bytes(((3*x+y) & 255, (7*y) & 255, (x ^ y) & 255, [100, 128, 129, 255][x & 3]))

    def sample(source, tw, th, u, v):
        # Exact rational coordinate mapping then explicit1/16 FST quantization.
        u, v = u.__floor__() - 8, v.__floor__() - 8
        x, y = u // 16, v // 16
        fx, fy = Fraction(u % 16, 16), Fraction(v % 16, 16)

        def pixel(px, py):
            px, py = px % tw, py % th
            width, height = dims[source]
            if not (0 <= px < width and 0 <= py < height):
                raise ValueError("positive-weight sampling outside known rows")
            return surfaces[source][py][px*4:px*4+4]

        rows = []
        for row in range(2 if fy else 1):
            left = pixel(x, y+row)
            right = pixel(x+1, y+row) if fx else left
            rows.append([((1-fx)*left[k] + fx*right[k]).__floor__() for k in range(4)])
        return [((1-fy)*rows[0][k] + fy*rows[1][k]).__floor__() if fy else rows[0][k] for k in range(4)]

    fragment_counts = []
    for p in packets:
        assert len(p["quads"]) == p["count"]
        dst = p["destination"]
        dw, dh = dims[dst]
        tw, th = 1 << ((p["tex0"] >> 26) & 15), 1 << ((p["tex0"] >> 30) & 15)
        writes = 0
        for x0, y0, x1, y1, u0, v0, u1, v1, rgba in p["quads"]:
            x0, x1, y0, y1 = x0-28672, x1-28672, y0-29184, y1-29184
            assert x1 > x0 and y1 > y0
            xs = range(max(0, (x0+15)//16), min(dw, (x1+15)//16))
            ys = range(max(0, (y0+15)//16), min(dh, (y1+15)//16))
            us = {x: Fraction(u0*(x1-x0)+(u1-u0)*(x*16-x0), x1-x0) for x in xs}
            vs = {y: Fraction(v0*(y1-y0)+(v1-v0)*(y*16-y0), y1-y0) for y in ys}
            for y in ys:
                row = surfaces[dst][y]
                for x in xs:
                    old = row[x*4:x*4+4]
                    if p["kind"] == 0:
                        new = bytes(4)
                    else:
                        tex = sample(p["source"], tw, th, us[x], vs[y])
                        if p["kind"] == 1:
                            # PacketALPHA48 simplifies to Cs*As/128+Cd;
                            # TEST3080D is GREATER128 with KEEP on fail.
                            assert rgba == 0x80808080 and p["alpha"] == 0x8000000048 and p["test"] == 0x3080d
                            if tex[3] <= 128:
                                continue
                            rgb = [min(255, (tex[k]*tex[3])//128+old[k]) for k in range(3)]
                        else:
                            # ALPHA68 FIX128 is additive; MODULATE weight
                            # happens before this blend. No genericselectors.
                            assert p["alpha"] == 0x8000000068
                            weight = rgba & 255
                            rgb = [min(255, tex[k]*weight//128+old[k]) for k in range(3)]
                        new = bytes(rgb + [old[3] if p["kind"] == 3 else tex[3]])
                    row[x*4:x*4+4] = new
                    writes += 1
        fragment_counts.append(writes)
    checks = []
    for i in range(4):
        expected = b"".join(surfaces[i])
        actual = Path(str(a.prefix) + f"-{i}.rgba").read_bytes()
        if actual != expected:
            at = next(j for j, (x, y) in enumerate(zip(actual, expected)) if x != y)
            raise AssertionError((i, at//4 % dims[i][0], at//4//dims[i][0], at % 4, actual[at], expected[at]))
        checks.append(dict(surface=i, bytes=len(expected), sha256=hashlib.sha256(actual).hexdigest()))
    report = dict(schema=1, scope="Fractions/integer oracle of native integer-sample/separable4bit reference, not hardwaretrace",
                  metadata_sha256=hashlib.sha256(text.encode()).hexdigest(), surfaces=checks, writes=fragment_counts,
                  matched_pixels=sum(w*h for w, h in dims))
    a.output.parent.mkdir(parents=True, exist_ok=True)
    with a.output.open("x", encoding="utf-8") as f:
        json.dump(report, f, indent=2)
        f.write("\n")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
