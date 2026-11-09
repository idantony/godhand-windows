"""Compare native RNG with the actual disc's two restricted MIPS routines.

The small instruction model is a development-only oracle, NOT a PS2 emulator
or hardware capture. It fails closed on unsupported operations and models only
the positive finite FPU values these routines produce, using EE chop rounding.
No interpreter or Python dependency is linked into the native module.
"""
import argparse
import collections
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

sys.dont_write_bytecode = True
from inspect_disc import parse_elf

ROOT = Path(__file__).resolve().parents[1]
STATE = 0x747B14
STOP = 0x800000
MASK = (1 << 64) - 1


def s32(x):
    x &= 0xFFFFFFFF
    return x - (1 << 32) if x & 0x80000000 else x


def f32(bits):
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def chop_positive(value):
    # Independent reference: first use Python's IEEE nearest packing, then
    # move one float toward zero if the nearest value exceeded the input.
    if not (0 <= value < 1e30):
        raise ValueError("Oracle only supports positive finite RNG operands")
    nearest = struct.unpack("<I", struct.pack("<f", value))[0]
    if f32(nearest) > value:
        nearest -= 1
    return nearest


class RestrictedMips:
    def __init__(self, blob):
        self.blob = blob
        self.segments = [ph for ph in parse_elf(blob)["program_headers"] if ph["type"] == 1]
        self.coverage = collections.Counter()
        self.instructions = set()

    def instruction(self, address):
        if not 0x31CF30 <= address <= 0x31CFD8 or address % 4:
            raise ValueError("Unexpected instruction address: " + hex(address))
        for ph in self.segments:
            relative = address - ph["vaddr"]
            if 0 <= relative <= ph["filesz"] - 4:
                return struct.unpack_from("<I", self.blob, ph["offset"] + relative)[0]
        raise ValueError("Instruction not mapped")

    def call(self, address, seed):
        r, f, memory = [0] * 32, [0] * 32, {}
        r[29], r[31] = 0x90FFF0, STOP
        cond = False
        for i, byte in enumerate(struct.pack("<I", seed)):
            memory[STATE + i] = byte

        def read(address, size):
            if address != STATE and not 0x90FFE0 <= address < 0x910000:
                raise ValueError("Unexpected data address " + hex(address))
            return int.from_bytes(bytes(memory.get(address + i, 0) for i in range(size)), "little")

        def write(address, size, value):
            if address != STATE and not 0x90FFE0 <= address < 0x910000:
                raise ValueError("Unexpected data write " + hex(address))
            for i in range(size):
                memory[address + i] = (value >> (8 * i)) & 255

        def step(pc, delay=False):
            nonlocal cond
            word = self.instruction(pc)
            self.instructions.add(pc)
            op, rs, rt, rd, sa, fn = word >> 26, (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31, (word >> 6) & 31, word & 63
            imm = word & 0xFFFF
            signed_imm = imm - 65536 if imm & 32768 else imm
            branch = None
            label = ""
            if word == 0:
                label = "nop"
            elif op == 0 and fn == 24:
                label = "R5900 mult rd,rs,rt"
                r[rd] = s32(s32(r[rs]) * s32(r[rt])) & MASK
            elif op == 0 and fn == 2:
                label = "srl"
                r[rd] = s32((r[rt] & 0xFFFFFFFF) >> sa) & MASK
            elif op == 0 and fn == 37:
                label = "or"
                r[rd] = r[rs] | r[rt]
            elif op == 0 and fn == 8:
                label = "jr"
                branch = r[rs]
            elif op == 1 and rt == 0:
                label = "bltz"
                branch = pc + 4 + signed_imm * 4 if r[rs] & (1 << 63) else pc + 8
            elif op == 4:
                label = "beq"
                branch = pc + 4 + signed_imm * 4 if r[rs] == r[rt] else pc + 8
            elif op == 3:
                label = "jal"
                r[31] = pc + 8
                branch = ((pc + 4) & 0xF0000000) | ((word & 0x3FFFFFF) << 2)
            elif op == 9:
                label = "addiu"
                r[rt] = s32(r[rs] + signed_imm) & MASK
            elif op == 12:
                label = "andi"
                r[rt] = r[rs] & imm
            elif op == 13:
                label = "ori"
                r[rt] = r[rs] | imm
            elif op == 15:
                label = "lui"
                r[rt] = s32(imm << 16) & MASK
            elif op in (35, 43, 55, 63):
                effective = (r[rs] + signed_imm) & MASK
                if op == 35:
                    label = "lw"
                    r[rt] = s32(read(effective, 4)) & MASK
                elif op == 43:
                    label = "sw"
                    write(effective, 4, r[rt])
                elif op == 55:
                    label = "ld"
                    r[rt] = read(effective, 8)
                else:
                    label = "sd"
                    write(effective, 8, r[rt])
            elif op == 17 and rs == 4:
                label = "mtc1"
                f[rd] = r[rt] & 0xFFFFFFFF
            elif op == 17 and rs == 20 and fn == 32:
                label = "cvt.s.w chop"
                f[sa] = chop_positive(s32(f[rd]))
            elif op == 17 and rs == 16 and fn in (0, 2, 6, 54):
                label = {0: "add.s chop", 2: "mul.s chop", 6: "mov.s", 54: "c.le.s"}[fn]
                if fn == 0:
                    f[sa] = chop_positive(f32(f[rd]) + f32(f[rt]))
                elif fn == 2:
                    f[sa] = chop_positive(f32(f[rd]) * f32(f[rt]))
                elif fn == 6:
                    f[sa] = f[rd]
                else:
                    cond = f32(f[rd]) <= f32(f[rt])
            elif op == 17 and rs == 8 and rt in (0, 1):
                label = "bc1t" if rt else "bc1f"
                branch = pc + 4 + signed_imm * 4 if cond == bool(rt) else pc + 8
            else:
                raise ValueError("Unsupported oracle instruction %08x at %08x" % (word, pc))
            self.coverage[label] += 1
            r[0] = 0
            if delay and branch is not None:
                raise ValueError("Branch in delay slot")
            return branch

        pc, steps = address, 0
        while pc != STOP:
            steps += 1
            if steps > 100:
                raise ValueError("Oracle step budget exceeded")
            branch = step(pc)
            if branch is None:
                pc += 4
            else:
                step(pc + 4, delay=True)
                pc = branch
        return read(STATE, 4), f[0]


class NativeState(ctypes.Structure):
    _fields_ = [("word", ctypes.c_uint32)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dll", type=Path, default=Path(r"G:\middle_thing\godhand_windows\build\native\godhand_core.dll"))
    args = parser.parse_args()
    blob = (ROOT / "reference" / "SLUS_215.03").read_bytes()
    oracle = RestrictedMips(blob)
    dll = ctypes.CDLL(str(args.dll.resolve()))
    functions = [(0x31CF78, dll.gh_rng_0031cf78), (0x31CF30, dll.gh_rng_0031cf30)]
    for _, function in functions:
        function.argtypes, function.restype = [ctypes.POINTER(NativeState)], ctypes.c_float
    seeds = {0, 1, 0xFFFFFFFF, 0x80000000}
    inverse = pow(0x10DCD, -1, 1 << 32)
    for bit in range(32):
        for delta in range(-4, 5):
            seeds.add((((1 << bit) + delta) * inverse) & 0xFFFFFFFF)
    for value in (0x7FFFFFFF, 0x80000001, 0x80000080, 0x80000081, 0x80000180, 0xFFFFFFFF):
        seeds.add((value * inverse) & 0xFFFFFFFF)
    generator = random.Random(0x21503)
    seeds.update(generator.getrandbits(32) for _ in range(10000))
    comparisons = 0
    for address, function in functions:
        for seed in sorted(seeds):
            expected_word, expected_bits = oracle.call(address, seed)
            state = NativeState(seed)
            value = function(ctypes.byref(state))
            actual_bits = struct.unpack("<I", struct.pack("<f", value))[0]
            if (state.word, actual_bits) != (expected_word, expected_bits):
                raise AssertionError("Mismatch function=%x seed=%08x native=(%08x,%08x) reference=(%08x,%08x)" %
                                     (address, seed, state.word, actual_bits, expected_word, expected_bits))
            comparisons += 1
    report = {"status": "passed", "elf_sha256": hashlib.sha256(blob).hexdigest(),
              "native_dll": str(args.dll.resolve()), "native_sha256": hashlib.sha256(args.dll.read_bytes()).hexdigest(),
              "distinct_seeds": len(seeds), "function_calls_compared": comparisons,
              "functions": [hex(address) for address, _ in functions], "comparison": "state word and return float bits, exact",
              "rounding_model": "EE chop, based on analysis/fpu-rounding-evidence.json",
              "instructions_exercised": len(oracle.instructions), "opcode_coverage": dict(oracle.coverage),
              "ps2_hardware_capture": False, "whole_game_validated": False,
              "limitations": ["Restricted development-only MIPS model, independently structured from native C",
                              "Unicorn lacks the R5900 mult rd extension and was not used as an unmodified oracle",
                              "No general EE floating-point correctness, timing or exception claim"]}
    target = ROOT / "analysis" / "rng-differential.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": "passed", "seeds": len(seeds), "exact_comparisons": comparisons,
                      "instructions": len(oracle.instructions), "ps2_hardware_capture": False}))


if __name__ == "__main__":
    main()
