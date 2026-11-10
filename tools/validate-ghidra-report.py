"""Validate this run's bounded PS2 Ghidra evidence before exclusive publication.
No game execution, source translation, or report mutation. Standard library only.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys
import time


class ValidationError(ValueError):
    pass


def address(value):
    # Numeric PowerShell arguments have already lost their lexical identity.
    # Reject them rather than reinterpret decimal/scientific values as hex.
    if not isinstance(value, str):
        raise ValidationError("address must be a quoted hexadecimal string")
    token = value.strip()
    if not re.fullmatch(r"(?:0[xX])?[0-9a-fA-F]{1,8}", token):
        raise ValidationError("invalid hexadecimal address: " + repr(value))
    number = int(token, 16)
    if number & 3:
        raise ValidationError("instruction address is not four-byte aligned")
    return f"{number:08x}"


def addresses(values):
    if not isinstance(values, list) or not values:
        raise ValidationError("at least one explicit address is required")
    result = list(dict.fromkeys(address(v) for v in values))
    if len(result) > 30:
        raise ValidationError("GodHandProbe supports at most 30 explicit addresses")
    return result


def elf_image(path):
    data = Path(path).read_bytes()
    if len(data) < 52 or data[:7] != b"\x7fELF\x01\x01\x01" or struct.unpack_from("<H", data, 18)[0] != 8:
        raise ValidationError("input must be a little-endian ELF32 MIPS image")
    start = struct.unpack_from("<I", data, 28)[0]
    stride, count = struct.unpack_from("<HH", data, 42)
    if stride < 32 or start > len(data) or count > (len(data)-start)//stride:
        raise ValidationError("ELF program header bounds")
    segments = []
    for i in range(count):
        kind, offset, va, _, size, memory, _, _ = struct.unpack_from("<8I", data, start+i*stride)
        if kind != 1:
            continue
        if size > memory or offset > len(data) or size > len(data)-offset or va+memory > 1 << 32:
            raise ValidationError("ELF PT_LOAD bounds")
        segments.append((va, offset, size, memory))
    if not segments:
        raise ValidationError("ELF has no PT_LOAD")
    return data, segments


def mapped_bytes(data, segments, pc):
    for va, offset, size, _ in segments:
        if va <= pc and pc+4 <= va+size:
            return data[offset+pc-va:offset+pc-va+4]
    raise ValidationError(f"instruction {pc:08x} is not file-backed PT_LOAD data")


def prepare(elf, requested, report, logs, limit):
    normalized = addresses(requested)
    if isinstance(limit, bool) or not isinstance(limit, int) or not 1 <= limit <= 65536:
        raise ValidationError("instruction limit must be 1..65536")
    data, segments = elf_image(elf)
    for pc in normalized:
        mapped_bytes(data, segments, int(pc, 16))
    if Path(report).exists() or any(Path(log).exists() for log in logs):
        raise ValidationError("run report/log paths must be fresh and absent")
    if not logs:
        raise ValidationError("at least one run log is required")
    return {"schema": 1, "started_ns": time.time_ns(),
            "elf": str(Path(elf).resolve()), "elf_sha256": hashlib.sha256(data).hexdigest(),
            "elf_size": len(data), "addresses": normalized, "instruction_limit": limit,
            "report": str(Path(report).resolve()), "logs": [str(Path(p).resolve()) for p in logs]}


def fresh_file(path, started):
    p = Path(path)
    if not p.is_file() or not p.stat().st_size:
        raise ValidationError("missing or empty run output: " + str(p))
    if p.stat().st_mtime_ns < started:
        raise ValidationError("stale output predates this run: " + str(p))
    if p.stat().st_mtime_ns > time.time_ns()+5_000_000_000:
        raise ValidationError("run output timestamp is in the future")
    return p.read_bytes()


def validate(manifest, exit_code):
    if manifest.get("schema") != 1 or exit_code != 0:
        raise ValidationError("headless failed or unsupported run manifest")
    requested = addresses(manifest.get("addresses"))
    started = manifest.get("started_ns")
    if not isinstance(started, int) or started <= 0:
        raise ValidationError("missing run start time")
    marker = False
    report_path = str(Path(manifest["report"]).resolve())
    for name in manifest["logs"]:
        log = fresh_file(name, started).decode("utf-8-sig", errors="replace")
        if re.search(r"(?im)\b(?:ERROR|FATAL)\b|java\.[a-zA-Z0-9_.]*(?:Exception|Error)", log):
            raise ValidationError("headless/Java error in run log: " + name)
        for line in log.splitlines():
            if "GODHAND_PROBE_OUTPUT=" in line:
                emitted = line.split("GODHAND_PROBE_OUTPUT=", 1)[1].strip()
                if os.path.normcase(os.path.abspath(emitted)) == os.path.normcase(report_path):
                    marker = True
    if not marker:
        raise ValidationError("missing current report completion marker")
    raw_report = fresh_file(report_path, started)
    try:
        report = json.loads(raw_report)
    except (ValueError, UnicodeError) as exc:
        raise ValidationError("malformed report JSON") from exc
    data, segments = elf_image(manifest["elf"])
    digest = hashlib.sha256(data).hexdigest()
    if digest != manifest["elf_sha256"] or len(data) != manifest["elf_size"]:
        raise ValidationError("input ELF changed during probe")
    if report.get("elfSha256") != digest or report.get("elfSize") != len(data):
        raise ValidationError("report/input ELF identity mismatch")
    if address(report.get("imageBase")) != f"{min(s[0] for s in segments):08x}":
        raise ValidationError("report/input image base mismatch")
    limit = manifest["instruction_limit"]
    if report.get("rawInstructionLimit") != limit:
        raise ValidationError("raw instruction cap mismatch")
    functions = report.get("functions")
    if not isinstance(functions, list):
        raise ValidationError("missing functions array")
    found = {}
    for function in functions:
        try:
            pc = address(function.get("requestedAddress"))
        except (ValidationError, AttributeError):
            raise ValidationError("function has invalid requestedAddress") from None
        if pc in found:
            raise ValidationError("duplicate requested function in report")
        found[pc] = function
    for pc in requested:
        if pc not in found:
            raise ValidationError("missing requested function: " + pc)
        f = found[pc]
        if f.get("error") or f.get("mapped") is not True or address(f.get("entry")) != pc:
            raise ValidationError("requested/entry/mapping/error disagreement: " + pc)
        raw = f.get("rawInstructions")
        if not isinstance(raw, list) or not raw or len(raw) > limit:
            raise ValidationError("empty or over-cap raw function: " + pc)
        if f.get("instructionCount") != len(raw) or f.get("instructionLimit") != limit or not isinstance(f.get("disassemblyTruncated"), bool):
            raise ValidationError("raw instruction count/cap/truncation metadata mismatch")
        if f.get("decompileCompleted") is not True or not isinstance(f.get("decompiledC"), str) or not f["decompiledC"].strip():
            raise ValidationError("requested decompilation incomplete or empty: " + pc)
        seen = set()
        for row in raw:
            site = int(address(row.get("address")), 16)
            if site in seen:
                raise ValidationError("duplicate raw instruction address")
            seen.add(site)
            try:
                encoded = bytes.fromhex(row.get("bytesLittleEndian", ""))
            except (ValueError, TypeError):
                raise ValidationError("invalid raw instruction bytes") from None
            if len(encoded) != 4 or encoded != mapped_bytes(data, segments, site):
                raise ValidationError("raw instruction/input mismatch")
    return raw_report, {"status": "verified", "addresses": requested,
                        "elf_sha256": digest, "report_sha256": hashlib.sha256(raw_report).hexdigest(),
                        "report_bytes": len(raw_report)}


def publish(path, payload):
    destination = Path(path)
    destination.parent.mkdir(parents=True, exist_ok=True)
    created = False
    try:
        with destination.open("xb") as stream:
            created = True
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
        if destination.read_bytes() != payload:
            raise ValidationError("published report byte verification failed")
    except BaseException:
        if created:
            destination.unlink(missing_ok=True)
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("prepare")
    for name in ("elf", "addresses", "report", "manifest"):
        p.add_argument("--"+name, required=True)
    p.add_argument("--log", action="append", required=True)
    p.add_argument("--limit", type=int, default=2048)
    p = sub.add_parser("validate")
    p.add_argument("--manifest", required=True)
    p.add_argument("--exit-code", type=int, required=True)
    p.add_argument("--publish", required=True)
    args = parser.parse_args()
    try:
        if args.command == "prepare":
            run = prepare(args.elf, json.loads(Path(args.addresses).read_text(encoding="utf-8-sig")), args.report, args.log, args.limit)
            publish(args.manifest, (json.dumps(run, indent=2)+"\n").encode())
            print(json.dumps({"status": "prepared", "addresses": run["addresses"]}))
        else:
            run = json.loads(Path(args.manifest).read_text(encoding="utf-8-sig"))
            raw, result = validate(run, args.exit_code)
            publish(args.publish, raw)
            result["output"] = str(Path(args.publish).resolve())
            print(json.dumps(result))
    except (ValidationError, OSError, ValueError, KeyError, TypeError) as exc:
        print("Ghidra report rejected: " + str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
