"""Independent non-clipped program10 scalar geometry oracle for actual SCR.
Not an instruction interpreter or PS2 hardware capture. Reads a caller-supplied
private asset; emits hashes/indices only. Native test fixtures match this model.
"""
import argparse
import hashlib
import json
import pathlib
import struct


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def mul(a, b):
    return f32(a * b)


def add(a, b):
    return f32(a + b)


def sub(a, b):
    return f32(a - b)


def bits(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


def matrix_apply(m, p):
    return [add(add(add(mul(m[j], p[0]), mul(m[4+j], p[1])),
                        mul(m[8+j], p[2])), m[12+j]) for j in range(4)]


def hash_word(h, x):
    for byte in struct.pack("<I", x & 0xffffffff):
        h = ((h ^ byte) * 1099511628211) & ((1 << 64)-1)
    return h


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("scr", type=pathlib.Path)
    args = parser.parse_args()
    data = args.scr.read_bytes()
    if len(data) != 12640:
        raise ValueError("requires actual first R100 SCR fixture (12640 bytes)")
    screen = [100,0,0,0, 0,-100,0,0, 0,0,1,f32(.05), 2048,2048,1000,2]
    clip_matrix = [0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1]
    batches = []
    for start, count in [(0,38),(36,12)]:
        rows, history, indices = [], [], []
        value_hash = 14695981039346656037
        for i in range(count):
            k = start+i
            raw = struct.unpack_from("<hhhH", data, 160+k*8)
            point = [mul(x, f32(.01)) for x in raw[:3]]
            s, c = matrix_apply(screen, point), matrix_apply(clip_matrix, point)
            q = f32(1/s[3])
            xyz = [mul(x,q) for x in s[:3]]
            history.append(xyz)
            uv = struct.unpack_from("<hh", data, 544+k*4)
            stq = [mul(add(f32(x/4096),off),q)
                   for x,off in zip(uv,[.25,-.125])] + [q]
            color = [int(min(mul(x,scale),255)) for x,scale in
                     zip(data[736+k*4:740+k*4],[.75,1,1.25,.5])]
            flags = sum((int(x>abs(c[3])) | (int(x < -abs(c[3])) << 1)) << (2*j)
                        for j,x in enumerate(c[:3]))
            fog = int(mul(min(max(add(100,mul(-25,s[3])),0),255),16))
            adc = int(bool(raw[3] & 0x8000))
            if i >= 2:
                a = [sub(history[i-2][j],history[i-1][j]) for j in range(2)]
                b = [sub(history[i][j],history[i-1][j]) for j in range(2)]
                if raw[3] & 0x7fff:
                    area = sub(mul(b[0],a[1]),mul(b[1],a[0]))
                else:
                    area = sub(mul(a[0],b[1]),mul(a[1],b[0]))
                if bits(area) >> 31 or flags or rows[-1][-3] or rows[-2][-3]:
                    adc = 1
                if not adc:
                    indices.extend([i-1,i-2,i] if i & 1 else [i-2,i-1,i])
            row = [*map(bits,stq), *color, *[int(mul(x,16)) for x in xyz],
                   fog, raw[3], flags, adc, int(i>=2)]
            rows.append(row)
            for x in row:
                value_hash = hash_word(value_hash,x)
        for x in indices:
            value_hash = hash_word(value_hash,x)
        batches.append(dict(first=start, vertices=count, triangles=len(indices)//3,
                            fnv64=f"{value_hash:016x}", indices=indices))
    print(json.dumps(dict(source_sha256=hashlib.sha256(data).hexdigest(),
                          numerical_policy="ordered native binary32 reference; fixture matrices",
                          batches=batches), indent=2))


if __name__ == "__main__":
    main()
