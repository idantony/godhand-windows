"""Inspect the original IDD texture table without extracting image data."""
import argparse
import json
from pathlib import Path
import struct

from inspect_assets import bounds, bundle_entry, sha
from inspect_disc import Iso9660


def table_item(table, item):
    bounds(0, 4, len(table))
    count = struct.unpack_from("<I", table)[0]
    bounds(4, count * 4, len(table))
    if not 1 <= item <= count:
        raise ValueError(f"ID item {item} is outside 1..{count}")
    offsets = struct.unpack_from(f"<{count}I", table, 4)
    start = offsets[item - 1]
    if not start:
        raise ValueError(f"ID item {item} is empty")
    header_size = 4 + count * 4
    if any(offset and not header_size <= offset < len(table) for offset in offsets):
        raise ValueError("ID texture offset outside payload")
    end = min((offset for offset in offsets if offset > start), default=len(table))
    return start, table[start:end]


def texture_metadata(texture):
    if texture.startswith(b"TIM2"):
        bounds(0, 16, len(texture))
        revision, layout, pictures = struct.unpack_from("<BBH", texture, 4)
        if revision != 4 or layout not in (0, 1) or pictures != 1:
            raise ValueError("Unsupported TIM2 file header")
        offset = 128 if layout else 16
        bounds(offset, 48, len(texture))
        total, clut_bytes, image_bytes, header, colors = struct.unpack_from("<IIIHH", texture, offset)
        bounds(offset, total, len(texture))
        if header < 48 or header + image_bytes + clut_bytes > total:
            raise ValueError("TIM2 image or palette outside picture")
        clut_type, image_type, width, height, tex0 = struct.unpack_from("<BBHHQ", texture, offset + 18)
        if not width or not height:
            raise ValueError("Invalid TIM2 dimensions")
        details = dict(format="TIM2", image_type=image_type, clut_type=clut_type,
                       width=width, height=height, image_bytes=image_bytes,
                       clut_bytes=clut_bytes, colors=colors)
    else:
        bounds(0, 48, len(texture))
        flags, = struct.unpack_from("<I", texture)
        if flags & 0x80000000:
            raise ValueError("Unsupported ID texture header")
        tex0, = struct.unpack_from("<Q", texture, 32)
        details = {"format": "T32"}
    details.update(tex0=f"{tex0:016X}", psm=(tex0 >> 20) & 63,
                   gs_width=1 << ((tex0 >> 26) & 15), gs_height=1 << ((tex0 >> 30) & 15),
                   tcc=(tex0 >> 34) & 1, tfx=(tex0 >> 35) & 3)
    return details


def inspect(iso_path, index_path, asset, items):
    index = json.loads(index_path.read_text(encoding="utf-8"))
    entry = next((entry for entry in index["entries"] if entry["path"].casefold() == asset.casefold()), None)
    if entry is None:
        raise ValueError(f"Asset missing from disc index: {asset}")
    if not 0 < entry["size"] <= 64 * 1024 * 1024:
        raise ValueError(f"ID source exceeds bounded reader: {asset}")
    iso = Iso9660(iso_path)
    try:
        raw = iso.read(entry["iso_offset"], entry["size"])
    finally:
        iso.close()
    source_digest = sha(raw)
    if source_digest != entry["sha256"]:
        raise ValueError("Disc entry differs from the pinned asset index")
    table = bundle_entry(bundle_entry(raw, "IDD"), "TBL", 1)
    records = []
    for item in dict.fromkeys(items):
        offset, texture = table_item(table, item)
        records.append(dict(item=item, table_offset=offset, bytes=len(texture),
                            sha256=sha(texture), **texture_metadata(texture)))
    return dict(asset=entry["path"], asset_sha256=source_digest, textures=records)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--index", type=Path, required=True, help="Verified analysis/asset-index.json")
    parser.add_argument("--asset", default="etc/core_usa.bin", help="Disc path containing the original IDD")
    parser.add_argument("--item", type=int, action="append", required=True, help="One-based texture number")
    args = parser.parse_args()
    try:
        report = inspect(args.iso, args.index, args.asset, args.item)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
