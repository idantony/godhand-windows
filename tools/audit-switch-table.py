"""Read-only ELF32 switch target evidence, never decompiler-label validation.

Use --table NAME:HEX_VA:COUNT repeatedly. Each index is read from actual ELF
words and its target prefix hashed; direct J/JAL and conditional branch targets
are reported to make case-label transcription reviewable. This does not infer
the index expression, delay-slot effects, prototype, or whole function body.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import struct
import sys


def target_prefix(address, read):
    if not address or address & 3:
        raise ValueError("zero/unaligned switch target")
    raw = read(address, 32)
    edges = []
    for i, word in enumerate(struct.unpack("<8I", raw)):
        pc, opcode = address + 4*i, word >> 26
        if opcode in (2, 3):
            target = ((pc+4) & 0xf0000000) | ((word & 0x3ffffff) << 2)
            edges.append({"pc": f"{pc:08x}", "kind": "JAL" if opcode == 3 else "J", "target": f"{target:08x}"})
        elif opcode in (1, 4, 5, 6, 7, 20, 21, 22, 23):
            imm = word & 0xffff
            if imm & 0x8000:
                imm -= 0x10000
            edges.append({"pc": f"{pc:08x}", "kind": "conditional_branch_encoding", "target": f"{(pc+4+4*imm)&0xffffffff:08x}"})
    return {"target": f"{address:08x}", "prefix32_sha256": hashlib.sha256(raw).hexdigest(), "prefix_direct_edges": edges}


def table(address, count, read):
    if address & 3 or not 1 <= count <= 2048:
        raise ValueError("invalid switch table alignment/count")
    raw = read(address, 4*count)
    return {"address": f"{address:08x}", "count": count, "sha256": hashlib.sha256(raw).hexdigest(),
            "entries": [dict(index=i, **target_prefix(target, read)) for i, target in enumerate(struct.unpack("<"+"I"*count, raw))]}


def self_test():
    sample = struct.pack("<8I", 0x0c000800, 0x1000fffe, 0, 0, 0, 0, 0, 0)
    def read(address, n):
        data = struct.pack("<I", 0x1000) if address == 0x800 else sample if address == 0x1000 else b""
        if len(data) != n:
            raise ValueError("unmapped read")
        return data
    result = table(0x800, 1, read)["entries"][0]
    assert result["index"] == 0 and result["target"] == "00001000"
    assert result["prefix_direct_edges"][0]["target"] == "00002000"
    assert result["prefix_direct_edges"][1]["target"] == "00001000"
    for address, count in [(0x801, 1), (0x800, 0), (0x800, 2049)]:
        try:
            table(address, count, read)
        except ValueError:
            continue
        raise AssertionError("invalid table accepted")
    print("PASS switch index/target provenance, JAL target, signed branch displacement, bounds/alignment")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--elf", type=Path)
    p.add_argument("--table", action="append", default=[])
    p.add_argument("--output", type=Path)
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args()
    if args.self_test:
        self_test()
        return 0
    if not args.elf or not args.table or not args.output:
        p.error("--elf, --table and --output are required")
    dependency = Path(__file__).with_name("function-context.py")
    shared = runpy.run_path(str(dependency), run_name="switch_table_shared_reader")
    raw = args.elf.read_bytes()
    elf = shared["Elf"](raw)
    rows = []
    for spec in args.table:
        name, address, count = spec.split(":")
        if not name:
            raise ValueError("empty table name")
        rows.append(dict(name=name, **table(int(address, 16), int(count), elf.read)))
    result = {"scope": "actual table indices and target prefixes only; caller dispatch/case labels require independent instruction review",
              "elf_sha256": hashlib.sha256(raw).hexdigest(), "reader_sha256": hashlib.sha256(dependency.read_bytes()).hexdigest(), "tables": rows}
    with args.output.open("x", encoding="utf-8") as file:
        json.dump(result, file, indent=2)
        file.write("\n")
    print(json.dumps({"status": "verified", "tables": len(rows), "output": str(args.output)}))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError, struct.error) as error:
        print(f"audit-switch-table: {error}", file=sys.stderr)
        raise SystemExit(2)
