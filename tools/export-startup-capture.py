"""Export RGB PNG pixels from a hash-linked, presented startup capture."""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib


WIDTH, HEIGHT = 512, 448


def png_chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def export(proof_path, output, layer, scene):
    proof = json.loads(proof_path.read_text(encoding="utf-8-sig"))
    if proof.get("tool") != "record-startup-frontier" or proof.get("schema") != 1:
        raise ValueError("Expected a startup capture receipt")
    candidates = [frame for frame in proof.get("captures", [])
                  if frame["presented"] is True and (scene is None or frame["scene_serial"] == scene)
                  and (scene is not None or frame[layer]["nonblack_rgb_pixels"])]
    if not candidates:
        raise ValueError("No matching presented capture")
    frame = max(candidates, key=lambda item: item["scene_serial"])
    source = frame[layer]
    raw = Path(source["path"]).read_bytes()
    if len(raw) != WIDTH * HEIGHT * 4 or hashlib.sha256(raw).hexdigest() != source["sha256"]:
        raise ValueError("Capture size or SHA-256 differs from the receipt")

    stride = WIDTH * 4
    pixels = bytearray(HEIGHT * (1 + WIDTH * 3))
    for y in range(HEIGHT):
        row = raw[y * stride:(y + 1) * stride]
        first = y * (1 + WIDTH * 3) + 1
        for channel in range(3):
            pixels[first + channel:first + WIDTH * 3:3] = row[channel::4]
    header = struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0)
    png = (b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", header)
           + png_chunk(b"IDAT", zlib.compress(pixels, 9)) + png_chunk(b"IEND", b""))
    with output.open("xb") as stream:
        stream.write(png)
    return frame["scene_serial"], hashlib.sha256(png).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--proof", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--layer", choices=("composite", "world"), default="composite")
    parser.add_argument("--scene", type=int, help="Default is the latest nonblack presented scene")
    args = parser.parse_args()
    try:
        scene, sha256 = export(args.proof, args.output, args.layer, args.scene)
    except (OSError, KeyError, TypeError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    print(f"Scene {scene}, {args.layer}, {sha256}")


if __name__ == "__main__":
    main()
