"""Read-only MIPS ELF32 method-table evidence, never native dispatch/execution.

Decode PMF records separately from virtual-method delta/reserved/target pairs.
Use --table NAME:HEX_VA:COUNT repeatedly. Output contains hashes, not gamebytes.
The file-backed ELF reader is shared with function-context.py.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import struct
import sys


def decode(raw: bytes, kind: str, read):
    if len(raw) != 8:
        raise ValueError("method record requires eight bytes")
    delta, secondary, target = struct.unpack("<hhI", raw)
    direct = kind == "vtable" or secondary < 0
    result = {"this_adjustment": delta, "record_sha256": hashlib.sha256(raw).hexdigest()}
    result["reserved_u16" if kind == "vtable" else "virtual_selector"] = secondary & 0xffff if kind == "vtable" else secondary
    if direct:
        result["target"] = f"{target:08x}"
        if not target:
            result["classification"] = "zero_or_unresolved_not_a_NULL_behavior_proof"
        else:
            if target & 3:
                raise ValueError("unaligned direct method address")
            result["classification"] = "direct_address_not_a_native_callable"
            result["first16_bytes_sha256"] = hashlib.sha256(read(target, 16)).hexdigest()
    else:
        result["classification"] = "virtual_PMF_requires_typed_vtable_resolution"
        result["vtable_field_offset"] = target
    return result


def self_test():
    read = lambda address, count: bytes(count) if address == 0x1000 else (_ for _ in ()).throw(ValueError("unmapped"))
    a = decode(struct.pack("<hhI", -4, -1, 0x1000), "pmf", read)
    assert a["this_adjustment"] == -4 and a["target"] == "00001000"
    a = decode(struct.pack("<hhI", 8, 3, 0x214), "pmf", read)
    assert a["virtual_selector"] == 3 and "target" not in a
    a = decode(struct.pack("<hhI", 0, 0, 0x1000), "vtable", read)
    assert a["classification"].startswith("direct")
    assert decode(bytes(8), "vtable", read)["classification"].startswith("zero_or_unresolved")
    for raw in [bytes(7), struct.pack("<hhI", 0, -1, 0x1001), struct.pack("<hhI", 0, -1, 0x2000)]:
        try:
            decode(raw, "pmf", read)
        except ValueError:
            continue
        raise AssertionError("bad method record accepted")
    print("PASS PMF/direct-vtable distinctions, adjustments, unresolved0, bounds/alignment strict")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path)
    parser.add_argument("--kind", choices=["pmf", "vtable"])
    parser.add_argument("--table", action="append", default=[])
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    if not args.elf or not args.kind or not args.table or not args.output:
        parser.error("--elf, --kind, --table and --output are required")
    dependency = Path(__file__).with_name("function-context.py")
    shared = runpy.run_path(str(dependency), run_name="method_table_shared_reader")
    raw = args.elf.read_bytes()
    elf = shared["Elf"](raw)
    result = {"scope": "read-only address evidence; no function prototype, case-label or completed-native-body claim", "elf_sha256": hashlib.sha256(raw).hexdigest(), "reader_sha256": hashlib.sha256(dependency.read_bytes()).hexdigest(), "kind": args.kind, "tables": []}
    for spec in args.table:
        name, address, count = spec.split(":")
        address, count = int(address, 16), int(count)
        if not name or not 1 <= count <= 2048 or address & 3:
            raise ValueError("invalid table name/count/alignment")
        data = elf.read(address, count * 8)
        rows = [dict(index=i, **decode(data[i * 8:i * 8 + 8], args.kind, elf.read)) for i in range(count)]
        result["tables"].append({"name": name, "address": f"{address:08x}", "count": count, "sha256": hashlib.sha256(data).hexdigest(), "records": rows})
    with args.output.open("x", encoding="utf-8") as file:
        json.dump(result, file, indent=2)
        file.write("\n")
    print(json.dumps({"status": "verified", "tables": len(result["tables"]), "output": str(args.output)}))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError, struct.error) as error:
        print(f"audit-method-table: {error}", file=sys.stderr)
        raise SystemExit(2)
