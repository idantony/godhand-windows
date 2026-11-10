"""Independent ordered-binary32 hierarchy oracle for a private model resource.
Usage: oracle_actor_bind_pose.py caller-owned-om00-or-pl00.dat
The native test reads the ISO itself and does not require an extractedfixture.
Only numeric hashes are emitted; no game bytes or decompiledcode are bundled.
"""
import argparse
import pathlib
import struct


def word(b, o):
    return struct.unpack_from("<I", b, o)[0]


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("resource", type=pathlib.Path)
    data = parser.parse_args().resource.read_bytes()
    count = word(data, 0)
    script = None
    for i in range(count):
        tag = data[4+count*4+i*4:8+count*4+i*4].split(b"\0")[0]
        if tag == b"MD":
            start = word(data, 4+i*4)
            end = word(data, 8+i*4) if i+1<count else len(data)
            script = data[start:end]
            break
    if script is None:
        raise ValueError("model resource has no MD")
    descriptor = word(script, 16)
    mdb = descriptor + struct.unpack_from("<i", script, descriptor)[0]
    first = mdb + word(script, mdb+4)
    parts = struct.unpack_from("<H", script, mdb+8)[0] & 255
    world = []
    result = 14695981039346656037
    for i in range(parts):
        x, y, z, mirror, parent = struct.unpack_from("<3fhh", script, first+i*16)
        if parent >= i:
            raise ValueError("parent is not an earlier part")
        base = world[parent] if parent >= 0 else [0, 0, 0]
        point = [f32(a+b) for a, b in zip((x, y, z), base)]
        world.append(point)
        for value in point+[-v for v in point]:
            for byte in struct.pack("<f", value):
                result = ((result ^ byte)*1099511628211) & ((1 << 64)-1)
    print(f"parts={parts} world+inversebindXYZ fnv64={result:016x}")


if __name__ == "__main__":
    main()
