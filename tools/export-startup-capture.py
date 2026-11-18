"""Export RGB PNG pixels from a hash-linked, presented startup capture."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import zlib


WIDTH, HEIGHT = 512, 448


def png_chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def checked_rgba(path, sha256):
    raw = path.read_bytes()
    if len(raw) != WIDTH * HEIGHT * 4 or hashlib.sha256(raw).hexdigest() != sha256:
        raise ValueError("Capture size or SHA-256 differs from the receipt")
    return raw


def startup_capture(proof, layer, scene):
    candidates = [frame for frame in proof.get("captures", [])
                  if frame["presented"] is True and (scene is None or frame["scene_serial"] == scene)
                  and (scene is not None or frame[layer]["nonblack_rgb_pixels"])]
    if not candidates:
        raise ValueError("No matching presented capture")
    frame = max(candidates, key=lambda item: item["scene_serial"])
    source = frame[layer]
    return frame["scene_serial"], checked_rgba(Path(source["path"]), source["sha256"])


def late_replay_capture(proof, directory, layer, scene):
    files = proof.get("files")
    presented = proof.get("actual_presented")
    if not isinstance(files, dict) or not isinstance(presented, list):
        raise ValueError("Expected a startup capture or late replay receipt")
    presented_scenes = {int(frame["scene"]) for frame in presented}
    candidates = {}
    for name, sha256 in files.items():
        match = re.fullmatch(r"[A-Za-z0-9_.-]+\.([0-9]+)\.rgba(\.world\.rgba)?", name)
        if not match or bool(match[2]) != (layer == "world"):
            continue
        serial = int(match[1])
        if serial not in presented_scenes or (scene is not None and serial != scene):
            continue
        if serial in candidates:
            raise ValueError("Multiple captures for the same presented scene")
        candidates[serial] = name, sha256

    for serial in sorted(candidates, reverse=True):
        name, sha256 = candidates[serial]
        path = (directory / name).resolve(strict=True)
        if path.parent != directory:
            raise ValueError("Capture points outside the receipt directory")
        raw = checked_rgba(path, sha256)
        if scene is not None or any(raw[0::4]) or any(raw[1::4]) or any(raw[2::4]):
            return serial, raw
    raise ValueError("No matching presented capture")


def export(proof_path, output, layer, scene):
    proof = json.loads(proof_path.read_text(encoding="utf-8-sig"))
    if proof.get("tool") == "record-startup-frontier" and proof.get("schema") == 1:
        serial, raw = startup_capture(proof, layer, scene)
    else:
        serial, raw = late_replay_capture(proof, proof_path.resolve().parent, layer, scene)

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
    return serial, hashlib.sha256(png).hexdigest()


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
