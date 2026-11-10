"""Compact, verified PS2 function evidence. Read-only analysis, never execution.

Current scope: little-endian MIPS ELF32, God Hand Main SNR2 at VA40A400,
and an optional SNR2 overlay at VA0 / supplied original REL files. Symbol
identities are evidence, not native-callability or recovered prototypes.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys


class EvidenceError(ValueError):
    pass


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def address(value) -> int:
    if isinstance(value, int):
        return value
    try:
        return int(str(value), 16)
    except ValueError as error:
        raise EvidenceError(f"invalid hexadecimal address: {value!r}") from error


def hx(value: int) -> str:
    return f"{value:08x}"


class Elf:
    def __init__(self, data: bytes):
        self.data = data
        if len(data) < 52 or data[:7] != b"\x7fELF\x01\x01\x01":
            raise EvidenceError("requires little-endian ELF32")
        if struct.unpack_from("<H", data, 18)[0] != 8:
            raise EvidenceError("requires MIPS e_machine=8")
        ph = struct.unpack_from("<I", data, 28)[0]
        stride, count = struct.unpack_from("<HH", data, 42)
        if stride < 32 or count > 128 or ph + stride * count > len(data):
            raise EvidenceError("invalid ELF program headers")
        self.segments = []
        for i in range(count):
            kind, off, va, _, size, memory, _, _ = struct.unpack_from("<8I", data, ph + stride * i)
            if kind == 1:
                if size > memory or off + size > len(data) or va + memory > 0x100000000:
                    raise EvidenceError("invalid PT_LOAD bounds")
                self.segments.append((va, size, off, memory))
        if not self.segments:
            raise EvidenceError("ELF has no PT_LOAD")
        self.image_base = min(p[0] for p in self.segments)

    def offset(self, va: int, size: int) -> int:
        matches = [off + va - base for base, length, off, _ in self.segments
                   if size >= 0 and base <= va and va + size <= base + length]
        if len(matches) != 1:
            raise EvidenceError(f"VA {hx(va)} +{size} lacks unique file-backed PT_LOAD")
        return matches[0]

    def read(self, va: int, size: int) -> bytes:
        off = self.offset(va, size)
        return self.data[off:off + size]


class Snr2:
    def __init__(self, read, base: int, label: str, relative: bool):
        self.read, self.base, self.label = read, base, label
        self.header = struct.unpack("<15I", read(base, 60))
        if self.header[0] != 0x32524E53:
            raise EvidenceError(f"{label}: no SNR2 header")
        if self.header[2] > 100000 or self.header[4] > 100000:
            raise EvidenceError(f"{label}: excessive SNR2 table")
        self.symbols, self.relocs = [], {}
        def location(word):
            return base + word if relative else word
        for i in range(self.header[4]):
            raw = read(location(self.header[3]) + i * 12, 12)
            name, target, _, kind, _ = struct.unpack("<IIHBB", raw)
            chars = bytearray()
            if name:
                start = location(name)
                for j in range(512):
                    char = read(start + j, 1)
                    if char == b"\0":
                        break
                    chars += char
                else:
                    raise EvidenceError(f"{label}: unterminated symbol {i}")
            self.symbols.append({"index": i, "name": chars.decode("ascii", "replace"),
                                 "address": target, "type": kind})
        for i in range(self.header[2]):
            raw = read(location(self.header[1]) + i * 12, 12)
            site, kind, index, _, addend = struct.unpack("<IBHBI", raw)
            site = base + site if relative else site
            symbol = self.symbols[index] if index < len(self.symbols) else None
            self.relocs.setdefault(site, []).append({"table_index": i, "kind": kind,
                "symbol_index": index, "symbol": symbol, "addend": addend})


def read_rel(path: Path) -> tuple[Snr2, dict, bytes]:
    data = path.read_bytes()
    if len(data) < 60 or struct.unpack_from("<I", data, 44)[0] != len(data):
        raise EvidenceError(f"{path}: invalid REL declared size")
    def read(off, size):
        if off < 0 or off + size > len(data):
            raise EvidenceError(f"{path}: REL out of bounds")
        return data[off:off + size]
    return Snr2(read, 0, path.name, True), {"path": str(path.resolve()), "bytes": len(data), "sha256": digest(data)}, data


def tables_in_elf(elf: Elf):
    tables = []
    for va, relative, label in [(0x40A400, False, "Main SNR2"), (0, True, "overlay SNR2")]:
        try:
            magic = elf.read(va, 4)
        except EvidenceError:
            continue
        if magic == b"SNR2":
            tables.append(Snr2(elf.read, va, label, relative))
    return tables


def verify_wrapper_bytes(elf: Elf, table: Snr2, original: bytes):
    """Validate this project's explicit synthetic-import wrapper, not a loader."""
    expected = bytearray(original)
    for site, records in table.relocs.items():
        if len(records) != 1 or site < 0 or site + 4 > len(expected):
            raise EvidenceError("wrapper original relocation bounds/duplicates")
        record = records[0]
        target = 0x02000000 + record["symbol_index"] * 0x10000 + record["addend"]
        word = struct.unpack_from("<I", original, site)[0]
        if record["kind"] == 4 and word >> 26 == 3:
            patched = (word & 0xFC000000) | ((target >> 2) & 0x03FFFFFF)
        elif record["kind"] == 5 and word >> 26 == 15:
            patched = (word & 0xFFFF0000) | (((target + 0x8000) >> 16) & 0xFFFF)
        elif record["kind"] == 6:
            patched = (word & 0xFFFF0000) | (target & 0xFFFF)
        else:
            raise EvidenceError("wrapper uses an unsupported original relocation kind/opcode")
        struct.pack_into("<I", expected, site, patched)
    if elf.read(0, len(original)) != expected:
        raise EvidenceError("wrapper bytes disagree with original REL plus explicit synthetic imports")


def import_for(site: int, tables: list[Snr2], related: list[Snr2]):
    records = [(table, record) for table in tables for record in table.relocs.get(site, [])
               if record["kind"] == 4]
    if not records:
        return {"status": "unknown", "reason": "no supported JAL relocation at callsite"}
    if len(records) != 1:
        return {"status": "unknown", "reason": "ambiguous callsite relocation"}
    table, record = records[0]
    result = {"table": table.label, "relocation_kind": record["kind"],
              "symbol_index": record["symbol_index"], "addend": record["addend"]}
    symbol = record["symbol"]
    if symbol is None or not symbol["name"]:
        return dict(result, status="unknown", reason="missing relocation symbol")
    result["symbol"] = symbol["name"]
    definitions = []
    for source in tables + related:
        for candidate in source.symbols:
            if (candidate["name"] == symbol["name"] and candidate["type"] in (2, 3)
                    and candidate["address"]):
                definitions.append({"table": source.label, "address": hx(candidate["address"] + record["addend"]),
                                    "address_space": "overlay-relative" if source.base == 0 and source.label != "Main SNR2" else "Main VA"})
    # Exact duplicate metadata from a supplied REL and its wrapper is one identity;
    # retain the sources rather than guessing that different modules are aliases.
    definitions = list({(item["table"], item["address"], item["address_space"]): item for item in definitions}.values())
    result["definitions"] = definitions
    result["status"] = "resolved_identity" if len(definitions) == 1 else "symbol_only"
    if not definitions:
        result["reason"] = "symbol is known; defining module/address unavailable"
    elif len(definitions) > 1:
        result["reason"] = "multiple definitions; no target chosen"
    return result


def vendor_names(project: Path):
    path = project / "vendor/god-hand-decomp/config/symbol_addrs.txt"
    result = {}
    if path.is_file():
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            match = re.match(r"\s*(\S+)\s*=\s*0x([0-9a-fA-F]+)\s*;", line)
            if match:
                result.setdefault(int(match[2], 16), []).append(match[1])
    return result


def candidates(project: Path, va: int, names: list[str], limit: int):
    executable = shutil.which("rg")
    if not executable:
        return {"status": "unavailable", "reason": "rg not on PATH", "matches": []}
    roots = [path for path in [project / "src", project / "vendor/god-hand-decomp/src",
             project / "vendor/god-hand-decomp/config/symbol_addrs.txt"] if path.exists()]
    if not roots:
        return {"status": "no_search_roots", "matches": []}
    needles = list(dict.fromkeys([hx(va), f"{va:x}"] + names[:4]))
    command = [executable, "--json", "-i", "-F", "--max-count", "2", "--glob", "*.c",
               "--glob", "*.h", "--glob", "*.cpp", "--glob", "symbol_addrs.txt"]
    for needle in needles:
        if needle:
            command += ["-e", needle]
    proc = subprocess.run(command + [str(path) for path in roots], capture_output=True, text=True,
                          encoding="utf-8", errors="replace", timeout=30)
    if proc.returncode not in (0, 1):
        return {"status": "search_error", "reason": proc.stderr[:512], "matches": []}
    matches = []
    for line in proc.stdout.splitlines():
        record = json.loads(line)
        if record["type"] == "match":
            item = record["data"]
            matches.append({"path": item["path"].get("text"), "line": item["line_number"],
                            "text": item["lines"].get("text", "").rstrip()[:512]})
    return {"status": "ok", "query": needles, "match_count": len(matches),
            "truncated": len(matches) > limit, "matches": matches[:limit],
            "policy": "candidate locations only; not verified source/prototypes"}


# Independently decoded R5900 I-type facts, not inferred object fields.
_GPR = "zero at v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 s0 s1 s2 s3 s4 s5 s6 s7 t8 t9 k0 k1 gp sp s8 ra".split()
_MEMORY = dict((op, (name, width)) for op,name,width in [
    (32,"LB",1),(36,"LBU",1),(33,"LH",2),(37,"LHU",2),(35,"LW",4),(39,"LWU",4),(55,"LD",8),(30,"LQ",16),
    (40,"SB",1),(41,"SH",2),(43,"SW",4),(63,"SD",8),(31,"SQ",16),
    (26,"LDL",8),(27,"LDR",8),(34,"LWL",4),(38,"LWR",4),(44,"SDL",8),(45,"SDR",8),(42,"SWL",4),(46,"SWR",4),
    (49,"LWC1",4),(57,"SWC1",4),(54,"LQC2",16),(62,"SQC2",16)])


def decode_memory_word(word, pc):
    if not isinstance(word, int) or not 0 <= word <= 0xffffffff:
        raise EvidenceError("instruction word outside unsigned32")
    if not isinstance(pc, int) or not 0 <= pc <= 0xfffffffc or pc % 4:
        raise EvidenceError("instruction PC outside aligned unsigned32")
    op, rs, rt, imm = word >> 26, (word >> 21) & 31, (word >> 16) & 31, word & 65535
    if op in (47,51):
        return {"kind":"hint", "mnemonic":"CACHE" if op == 47 else "PREF", "pc":hx(pc)}
    if op not in _MEMORY:
        return {"kind":"unknown_memory_opcode", "opcode":op, "pc":hx(pc)} if op >= 32 else None
    name,width = _MEMORY[op]
    return {"kind":"access", "rs":rs, "rs_alias":_GPR[rs], "offset_signed16":imm if imm < 32768 else imm-65536,
        "offset_encoded16":imm, "operation":"load" if name.startswith("L") else "store", "nominal_width_bytes":width,
        "access_shape":"partial_merge" if name in ("LDL","LDR","LWL","LWR","SDL","SDR","SWL","SWR") else "ordinary",
        "data_register_bank":"fpr" if op in (49,57) else "vf" if op in (54,62) else "gpr",
        "data_register":rt, "mnemonic":name, "pc":hx(pc)}


def observed_memory(instructions, elf, site_limit=3):
    if not isinstance(site_limit,int) or not 1 <= site_limit <= 16:
        raise EvidenceError("memory site limit must be 1..16")
    groups,hints,unknown = {},[],[]
    for pc in sorted(instructions):
        item = decode_memory_word(struct.unpack("<I",elf.read(pc,4))[0],pc)
        if item is None:
            continue
        if item["kind"] != "access":
            (hints if item["kind"] == "hint" else unknown).append(item)
            continue
        key = tuple(item[k] for k in ("rs","offset_signed16","operation","nominal_width_bytes","access_shape","data_register_bank"))
        if key not in groups:
            groups[key] = {k:v for k,v in item.items() if k not in ("kind","pc","mnemonic","data_register")}
            groups[key].update(static_site_count=0,mnemonics={},pc_examples=[],data_registers=[])
        g=groups[key];g["static_site_count"]+=1
        g["mnemonics"][item["mnemonic"]]=g["mnemonics"].get(item["mnemonic"],0)+1
        if len(g["pc_examples"]) < site_limit:
            g["pc_examples"].append(item["pc"])
        if item["data_register"] not in g["data_registers"]:
            g["data_registers"].append(item["data_register"])
    result=list(groups.values())
    for g in result:
        g["pc_examples_truncated"]=len(g["pc_examples"]) < g["static_site_count"]
    return {"scope":"static verified EE I-type instructions only; register addressing, not class fields, effective addresses, aliases or execution counts",
        "partial_merge_policy":"nominal container width only; bytes touched depend on runtime address",
        "static_site_count":sum(g["static_site_count"] for g in result),"group_count":len(result),"groups":result,
        "hints":{"count":len(hints),"examples":hints[:site_limit]},
        "unknown_memory_opcodes":{"count":len(unknown),"examples":unknown[:site_limit]}}


def function_evidence(function: dict, elf: Elf, tables: list[Snr2], related: list[Snr2],
                      project: Path, known: dict, search: bool, decompile_limit: int, memory_site_limit=3):
    if not function.get("mapped") or not function.get("entry"):
        raise EvidenceError(f"requested {function.get('requestedAddress')}: report function not mapped")
    entry, requested = address(function["entry"]), address(function["requestedAddress"])
    instructions = {}
    for item in function.get("rawInstructions", []):
        va = address(item["address"])
        try:
            raw = bytes.fromhex(item["bytesLittleEndian"])
        except (KeyError, ValueError) as error:
            raise EvidenceError(f"{hx(va)}: invalid report raw bytes") from error
        if len(raw) != 4 or va in instructions or elf.read(va, 4) != raw:
            raise EvidenceError(f"{hx(va)}: duplicate or raw instruction/source mismatch")
        if item.get("wordHex") and address(item["wordHex"]) != struct.unpack("<I", raw)[0]:
            raise EvidenceError(f"{hx(va)}: wordHex disagrees with verified bytes")
        instructions[va] = item
    if not instructions or entry not in instructions:
        raise EvidenceError(f"{hx(entry)}: no verified entry bytes")
    ordered = sorted(instructions)
    groups = [[ordered[0]]]
    for va in ordered[1:]:
        if va == groups[-1][-1] + 4:
            groups[-1].append(va)
        else:
            groups.append([va])
    ranges = [{"start": hx(group[0]), "end_exclusive": hx(group[-1] + 4),
               "file_offset": elf.offset(group[0], len(group) * 4), "bytes": len(group) * 4,
               "sha256": digest(elf.read(group[0], len(group) * 4))} for group in groups]
    metadata = {address(item["address"]): item for item in function.get("outgoingCalls", [])}
    calls = []
    for va in ordered:
        item = instructions[va]
        word = struct.unpack("<I", elf.read(va, 4))[0]
        op, rt = word >> 26, (word >> 16) & 31
        target = None
        if op == 3:
            target = ((va + 4) & 0xF0000000) | ((word & 0x3FFFFFF) << 2)
            kind = "direct_jal"
        elif op == 1 and rt in (16, 17, 18, 19):
            offset = word & 0xFFFF
            offset = offset if offset < 0x8000 else offset - 0x10000
            target, kind = (va + 4 + offset * 4) & 0xFFFFFFFF, "conditional_link"
        elif op == 0 and word & 63 == 9:
            kind = "computed_jalr"
        else:
            continue
        row = {"site": hx(va), "kind": kind, "instruction": item.get("instruction"),
               "raw": elf.read(va, 4).hex(), "target": hx(target) if target is not None else None}
        if target is not None:
            reported = metadata.get(va, {}).get("flowTargets", [])
            if reported and target not in [address(value) for value in reported]:
                raise EvidenceError(f"{hx(va)}: reported call target disagrees with raw instruction")
            relocation = import_for(va, tables, related)
            if target == 0 or "symbol" in relocation or relocation.get("symbol_index") is not None:
                row["import"] = relocation
            row["vendor_candidate_names"] = known.get(target, [])[:6]
        else:
            row["resolution"] = "unknown computed target; no fabricated callee"
        calls.append(row)
    call_sites = {address(item["site"]) for item in calls}
    for va in metadata:
        if va not in instructions or va not in call_sites:
            raise EvidenceError(f"{hx(va)}: outgoing call is not a verified call instruction")
    direct = {}
    for call in calls:
        if call["target"] is not None:
            name = call.get("import", {}).get("symbol")
            key = (call["target"], name)
            direct.setdefault(key, {"target": call["target"], "import_symbol": name, "callsites": []})["callsites"].append(call["site"])
    text = function.get("decompiledC") or ""
    clipped = text if decompile_limit == 0 else text[:decompile_limit]
    warnings = list(function.get("decompilerWarnings") or [])
    if requested != entry:
        warnings.append("requestedAddress differs from actual report entry; both identities retained")
    names = known.get(entry, [])
    return {"requested": hx(requested), "entry": hx(entry), "requested_entry_disagreement": requested != entry,
            "requested_in_verified_body": requested in instructions, "report_name": function.get("name"),
            "body_bytes_reported": function.get("bodyAddresses"), "verified_instruction_bytes": len(instructions) * 4,
            "body_count_matches_verified_bytes": function.get("bodyAddresses") == len(instructions) * 4,
            "observed_memory": observed_memory(instructions, elf, memory_site_limit),
            "ranges": ranges, "range_policy": "only verified report instructions; gaps excluded, not assumed full function",
            "raw_ordered_sha256": digest(b"".join(elf.read(va, 4) for va in ordered)),
            "calls": calls, "direct_callees": list(direct.values()),
            "decompile": {"completed": function.get("decompileCompleted"), "error": function.get("decompileError"),
                          "warnings": warnings, "text": clipped, "chars": len(text), "sha256": digest(text.encode()),
                          "truncated": len(clipped) != len(text)},
            "vendor_candidate_names": names[:8],
            "source_candidates": candidates(project, entry, names, 16) if search else {"status": "disabled", "matches": []}}


def build_context(report_path: Path, elf_path: Path, addresses: list[int], project: Path,
                  rel_paths=(), search=True, decompile_limit=10000, wrapper_manifest: Path | None = None, memory_site_limit=3):
    report_bytes, binary = report_path.read_bytes(), elf_path.read_bytes()
    report = json.loads(report_bytes)
    actual = digest(binary)
    if not report.get("elfSha256") or report["elfSha256"].lower() != actual:
        raise EvidenceError("report elfSha256 does not match supplied input")
    if report.get("elfSize") != len(binary):
        raise EvidenceError("report elfSize does not match supplied input")
    elf = Elf(binary)
    if report.get("elfMachine", 8) != 8:
        raise EvidenceError("report elfMachine does not match MIPS input")
    if report.get("elfFlagsHex") and address(report["elfFlagsHex"]) != struct.unpack_from("<I", binary, 36)[0]:
        raise EvidenceError("report elfFlagsHex does not match input")
    if address(report.get("imageBase", "-1")) != elf.image_base:
        raise EvidenceError("report imageBase does not match PT_LOAD image base")
    tables = tables_in_elf(elf)
    related, related_inputs, related_data = [], [], {}
    for path in rel_paths:
        table, evidence, data = read_rel(Path(path))
        related.append(table)
        related_inputs.append(evidence)
        related_data[evidence["sha256"]] = (table, data)
    wrapper = None
    if wrapper_manifest:
        raw = wrapper_manifest.read_bytes()
        wrapper = json.loads(raw)
        if wrapper.get("wrapper_sha256") != actual:
            raise EvidenceError("wrapper manifest hash does not match input ELF")
        if wrapper.get("source_rel_sha256") not in [item["sha256"] for item in related_inputs]:
            raise EvidenceError("wrapper manifest original REL hash unavailable/mismatched")
        original_table, original_data = related_data[wrapper["source_rel_sha256"]]
        overlays = [table for table in tables if table.label == "overlay SNR2"]
        if len(overlays) != 1 or overlays[0].header != original_table.header or overlays[0].symbols != original_table.symbols:
            raise EvidenceError("wrapper overlay metadata does not match original REL")
        verify_wrapper_bytes(elf, original_table, original_data)
        original_table.label = overlays[0].label  # manifest-proven alias, not a guessed cross-module identity
        wrapper = {"path": str(wrapper_manifest.resolve()), "sha256": digest(raw),
                   "source_rel_sha256": wrapper["source_rel_sha256"], "wrapper_sha256": actual,
                   "original_plus_synthetic_import_bytes_verified": True}
    records = report.get("functions", [])
    selected = []
    if addresses:
        for va in addresses:
            found = [f for f in records if va in [address(value) for value in (f.get("requestedAddress"), f.get("entry")) if value is not None and str(value)]]
            if len(found) != 1:
                raise EvidenceError(f"address {hx(va)} has {len(found)} matching report functions")
            if found[0] not in selected:
                selected.append(found[0])
    else:
        selected = records
    if not selected:
        raise EvidenceError("report contains no selected functions")
    known = vendor_names(project)
    functions = [function_evidence(f, elf, tables, related, project, known, search, decompile_limit, memory_site_limit) for f in selected]
    if wrapper:
        for function in functions:
            ranges = function["ranges"]
            if all(address(item["end_exclusive"]) <= len(original_data) for item in ranges):
                function["original_rel_ranges"] = [{"start": item["start"], "end_exclusive": item["end_exclusive"],
                    "bytes": item["bytes"], "sha256": digest(original_data[address(item["start"]):address(item["end_exclusive"])])}
                    for item in ranges]
            else:
                function["original_rel_ranges"] = None
    return {"schema": 1, "tool": "godhand-function-context", "tool_version": 2, "input": {"elf": str(elf_path.resolve()),
            "sha256": actual, "bytes": len(binary), "image_base": hx(elf.image_base),
            "report": str(report_path.resolve()), "report_sha256": digest(report_bytes),
            "declared_program": report.get("program"), "declared_path": report.get("executablePath"),
            "report_hash_verified": True, "related_rel": related_inputs, "wrapper_manifest": wrapper},
            "scope": "read-only evidence; no native body generation, no guest execution or success stubs",
            "xref_limitation": report.get("xrefLimitation", "report xrefs may be incomplete"),
            "symbols": [{"table": t.label, "count": len(t.symbols), "relocation_sites": len(t.relocs)} for t in tables],
            "functions": functions}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--address", action="append", default=[], help="hex address; repeatable or comma-separated")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--rel", type=Path, action="append", default=[])
    parser.add_argument("--wrapper-manifest", type=Path)
    parser.add_argument("--decompile-limit", type=int, default=10000, help="0 includes complete decompile")
    parser.add_argument("--memory-sites", type=int, default=3, help="PC examples per memory group, 1..16")
    parser.add_argument("--no-search", action="store_true")
    parser.add_argument("--force", action="store_true", help="overwrite this explicitly selected generated private output")
    args = parser.parse_args(argv)
    try:
        output, analysis = args.output.resolve(), (args.project / "analysis").resolve()
        if not output.is_relative_to(analysis) or not output.name.endswith("-private.json"):
            raise EvidenceError("output must be project/analysis/*-private.json")
        if args.decompile_limit < 0:
            raise EvidenceError("decompile limit must be nonnegative")
        if output.exists() and not args.force:
            raise EvidenceError("output exists; use a new name or explicit --force")
        wanted = [address(item) for group in args.address for item in group.split(",")]
        context = build_context(args.report, args.elf, wanted, args.project, args.rel,
                                not args.no_search, args.decompile_limit, args.wrapper_manifest, args.memory_sites)
        output.parent.mkdir(parents=True, exist_ok=True)
        # Validate all input evidence before touching the output; no temp files on E:.
        with output.open("w" if args.force else "x", encoding="utf-8", newline="\n") as stream:
            stream.write(json.dumps(context, indent=2, ensure_ascii=False) + "\n")
        print(json.dumps({"status": "verified", "functions": len(context["functions"]),
                          "calls": sum(len(f["calls"]) for f in context["functions"]),
                          "output": str(output), "sha256": digest(output.read_bytes())}))
        return 0
    except (EvidenceError, OSError, ValueError, KeyError, struct.error, subprocess.TimeoutExpired) as error:
        print(f"function-context: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
