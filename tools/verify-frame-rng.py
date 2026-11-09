"""Check finite frame RNG conversion against actual restricted instruction words.

Development oracle only, not PS2 hardware or a deployed guest runtime. The MT
callee is an explicit supplied-word fixture; its state engine is tested apart.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
from inspect_disc import parse_elf
from verify_rng import chop_positive, f32

ROOT = Path(__file__).resolve().parents[1]
STOP = 0x800000


def signed_chop(value):
    return chop_positive(abs(value)) | (0x80000000 if value < 0 else 0)


class InstructionFixture:
    def __init__(self, blob):
        self.blob = blob
        self.segments = [p for p in parse_elf(blob)["program_headers"] if p["type"] == 1]
        self.covered = set()

    def fetch(self, pc):
        if not (0x30F518 <= pc < 0x30F54C or 0x31D020 <= pc < 0x31D04C) or pc % 4:
            raise ValueError("Unimplemented instruction address: " + hex(pc))
        for p in self.segments:
            delta = pc - p["vaddr"]
            if 0 <= delta <= p["filesz"] - 4:
                self.covered.add(pc)
                return struct.unpack_from("<I", self.blob, p["offset"] + delta)[0]
        raise ValueError("Unmapped instruction")

    def call(self, entry, mt_word):
        r, f, stack = [0] * 32, [0] * 32, {}
        r[29], r[31] = 0x910000, STOP
        mt_calls = 0

        def step(pc):
            nonlocal mt_calls
            word = self.fetch(pc)
            op, rs, rt, rd, sa, fn = word >> 26, (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31, (word >> 6) & 31, word & 63
            immediate = word & 0xFFFF
            signed = immediate - 65536 if immediate & 32768 else immediate
            branch = None
            if word == 0:
                pass
            elif op == 9:
                r[rt] = (r[rs] + signed) & 0xFFFFFFFFFFFFFFFF
            elif op in (55, 63):
                address = r[rs] + signed
                if not 0x90FFE0 <= address < 0x910000:
                    raise ValueError("Unexpected stack access")
                if op == 63:
                    stack[address] = r[rt]
                else:
                    r[rt] = stack[address]
            elif op == 0 and fn == 2:
                r[rd] = (r[rt] & 0xFFFFFFFF) >> sa
            elif op == 0 and fn == 8:
                branch = r[rs]
            elif op == 3:
                r[31] = pc + 8
                branch = ((pc + 4) & 0xF0000000) | ((word & 0x3FFFFFF) << 2)
                if branch == 0x30F348:
                    mt_calls += 1
                    r[2] = mt_word
                    branch = pc + 8
            elif op == 15:
                r[rt] = immediate << 16
            elif op == 17 and rs == 4:
                f[rd] = r[rt] & 0xFFFFFFFF
            elif op == 17 and rs == 20 and fn == 32:
                if f[rd] & 0x80000000:
                    raise ValueError("Fixture CVT only supports the actual positive domain")
                f[sa] = chop_positive(f[rd])
            elif op == 17 and rs == 16 and fn in (0, 1, 2):
                a, b = f32(f[rd]), f32(f[rt])
                f[sa] = signed_chop(a + b if fn == 0 else a - b if fn == 1 else a * b)
            else:
                raise ValueError("Unsupported instruction %08X at %08X" % (word, pc))
            r[0] = 0
            return branch

        pc = entry
        for _ in range(100):
            if pc == STOP:
                if mt_calls != 1:
                    raise ValueError("Unexpected MT fixture call count")
                return f[0]
            branch = step(pc)
            if branch is not None:
                if step(pc + 4) is not None:
                    raise ValueError("Branch in delay slot")
                pc = branch
            else:
                pc += 4
        raise ValueError("Instruction budget exhausted")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dll", type=Path, default=Path(r"G:\middle_thing\godhand_windows\build\native\godhand_core.dll"))
    args = parser.parse_args()
    blob = (ROOT / "reference/SLUS_215.03").read_bytes()
    fixture = InstructionFixture(blob)
    dll = ctypes.CDLL(str(args.dll.resolve()))
    functions = [(0x30F518, dll.gh_frame_unit_bits_0030f518_word), (0x31D020, dll.gh_frame_jitter_bits_0031d020_word)]
    values = {0, 1, 0xFFFFFFFF, 0x80000000, 0x7FFFFFFF}
    for bit in range(32):
        for delta in range(-4, 5):
            values.add(((1 << bit) + delta) & 0xFFFFFFFF)
    rng = random.Random(0x30F518)
    values.update(rng.getrandbits(32) for _ in range(10000))
    for address, function in functions:
        function.argtypes, function.restype = [ctypes.c_uint32], ctypes.c_uint32
        for value in sorted(values):
            expected, actual = fixture.call(address, value), function(value)
            if expected != actual:
                raise AssertionError("entry%08X word%08X expected%08X actual%08X" % (address, value, expected, actual))
    report = {"status": "passed", "distinct_MT_words": len(values), "comparisons": len(values) * 2,
              "instruction_addresses": len(fixture.covered), "MT_callee": "Supplied-word fixture; state engine verified separately",
              "numeric_policy": "Pinned default EE chop reference; no hardware capture", "hardware_capture": False,
              "elf_sha256": hashlib.sha256(blob).hexdigest(), "native_dll": str(args.dll.resolve()),
              "native_sha256": hashlib.sha256(args.dll.read_bytes()).hexdigest(),
              "source_sha256": {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in
                                ("src/core/frame_state.c", "src/core/frame_state.h", "tools/verify-frame-rng.py", "tools/verify_rng.py")}}
    report["original_ranges"] = []
    for address, size in ((0x30F518, 0x34), (0x31D020, 0x2C), (0x31CFE0, 0x1C), (0x31CEB8, 0x38)):
        segment = next(p for p in fixture.segments if p["vaddr"] <= address and address + size <= p["vaddr"] + p["filesz"])
        offset = segment["offset"] + address - segment["vaddr"]
        report["original_ranges"].append({"address": "%08X" % address, "size": size,
                                          "sha256": hashlib.sha256(blob[offset:offset + size]).hexdigest()})
    report["reference_sources"] = [
        "https://github.com/PCSX2/pcsx2/blob/0ac53afdc72dee599b422bb30a1d683f98e74625/pcsx2/FPU.cpp#L345",
        "https://github.com/PCSX2/pcsx2/blob/0ac53afdc72dee599b422bb30a1d683f98e74625/pcsx2/Pcsx2Config.cpp"]
    report["caller_scope"] = "Observed constant seed and per-frame leaves; earlier constructors/full boot and hardware frame cadence remain unverified"
    (ROOT / "analysis/frame-rng-differential.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: report[k] for k in ("status", "distinct_MT_words", "comparisons", "instruction_addresses", "hardware_capture")}))


if __name__ == "__main__":
    main()
