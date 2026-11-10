"""Bounded offline R5900 scalar static-initializer write audit.

This is a provenance/test tool, never a product runtime or a general emulator.
It accepts explicit function bytes, input registers, output ranges and one
optional memset semantic hook. Unsupported instructions, unknown values reaching
output/control, and writes outside declared output/stack ranges stop the audit.
No original bytes are emitted: reports contain hashes/counts only.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

REGISTERS = "zero at v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 s0 s1 s2 s3 s4 s5 s6 s7 s8 t9 k0 k1 gp sp fp ra".split()
# R5900 register24 is t8, not s8; permit Ghidra's s8 spelling for register30.
REGISTERS[24] = "t8"
MASK = (1 << 64) - 1


def signed(n, bits):
    return n - (1 << bits) if n & (1 << (bits - 1)) else n


class AuditError(RuntimeError):
    pass


class Elf:
    def __init__(self, data):
        self.data, self.segments = data, []
        if len(data) < 52 or data[:7] != b"\x7fELF\x01\x01\x01" or struct.unpack_from("<H", data, 18)[0] != 8:
            raise AuditError("Expected little-endian ELF32 MIPS")
        offset = struct.unpack_from("<I", data, 28)[0]
        stride, count = struct.unpack_from("<HH", data, 42)
        if stride < 32 or count > 128 or offset + stride * count > len(data):
            raise AuditError("Invalid ELF program table")
        for i in range(count):
            kind, source, base, _, files, memory = struct.unpack_from("<6I", data, offset + i * stride)
            if kind == 1:
                if files > memory or source + files > len(data):
                    raise AuditError("Invalid PT_LOAD extent")
                self.segments.append((base, source, files, memory))

    def read(self, address, count):
        matches = [s for s in self.segments if s[0] <= address and address + count <= s[0] + s[3]]
        if len(matches) != 1:
            raise AuditError(f"Unmapped/ambiguous ELF span {address:08X}+{count:X}")
        base, source, files, _ = matches[0]
        delta = address - base
        available = min(count, max(0, files - delta))
        return self.data[source + delta:source + delta + available] + bytes(count - available)


class Engine:
    def __init__(self, elf, start, end, spans, inputs, memset_address=None, maximum=10000):
        self.elf, self.start, self.end = elf, start, end
        self.spans, self.memset_address, self.maximum = spans, memset_address, maximum
        self.registers, self.floats, self.memory = [None] * 32, [None] * 32, {}
        self.registers[0], self.registers[29], self.registers[31] = 0, 0x0F000000, 0xFFFF0000
        for register, value in inputs.items():
            self.registers[register] = value & MASK
        self.calls, self.steps, self.writes = [], 0, 0
        for address, size in spans:
            if size <= 0 or address < 0 or address + size > 0x100000000:
                raise AuditError("Invalid output range")
            for i, byte in enumerate(elf.read(address, size)):
                if address + i in self.memory:
                    raise AuditError("Overlapping output ranges")
                self.memory[address + i] = byte

    def stack(self, address, count):
        return 0x0EFF0000 <= address and address + count <= 0x0F000100

    def output(self, address, count):
        return any(base <= address and address + count <= base + size for base, size in self.spans)

    def store(self, address, count, value):
        is_output = self.output(address, count)
        if not is_output and not self.stack(address, count):
            raise AuditError(f"Write outside declared output/stack {address:08X}+{count}")
        if is_output and (value is None or count > 8):
            raise AuditError("Unknown/wide register value reaches output")
        for i in range(count):
            self.memory[address + i] = None if value is None or i >= 8 else (value >> (8 * i)) & 255
        self.writes += int(is_output)

    def load(self, address, count):
        if self.output(address, count) or self.stack(address, count):
            values = [self.memory.get(address + i) for i in range(count)]
            if any(x is None for x in values):
                return None
            return int.from_bytes(bytes(values), "little")
        return int.from_bytes(self.elf.read(address, count), "little")

    def known(self, value, description):
        if value is None:
            raise AuditError("Unknown value in " + description)
        return value

    def word(self, pc):
        if pc & 3 or not self.start <= pc < self.end:
            raise AuditError(f"Control left declared function at {pc:08X}")
        self.steps += 1
        if self.steps > self.maximum:
            raise AuditError("Instruction budget exceeded")
        return int.from_bytes(self.elf.read(pc, 4), "little")

    def simple(self, pc, word):
        op, rs, rt, rd = word >> 26, word >> 21 & 31, word >> 16 & 31, word >> 11 & 31
        immediate = word & 65535
        displacement = signed(immediate, 16)
        a, b = self.registers[rs], self.registers[rt]
        if word == 0:
            return
        if op == 15:
            self.registers[rt] = signed(immediate << 16, 32) & MASK
        elif op in (9, 10, 11, 12, 13):
            if a is None:
                self.registers[rt] = None
            elif op == 9:
                self.registers[rt] = signed((a + displacement) & 0xFFFFFFFF, 32) & MASK
            elif op == 10:
                self.registers[rt] = int(signed(a, 64) < displacement)
            elif op == 11:
                self.registers[rt] = int(a < (displacement & MASK))
            else:
                self.registers[rt] = a & immediate if op == 12 else a | immediate
        elif op == 0 and (word & 63) in (33, 45, 36, 37, 42):
            function = word & 63
            if a is None or b is None:
                self.registers[rd] = None
            elif function in (33, 45):
                result = a + b
                self.registers[rd] = signed(result & 0xFFFFFFFF, 32) & MASK if function == 33 else result & MASK
            elif function == 36:
                self.registers[rd] = a & b
            elif function == 37:
                self.registers[rd] = a | b
            else:
                self.registers[rd] = int(signed(a, 64) < signed(b, 64))
        elif op == 17 and rs == 4:
            self.floats[rd] = None if b is None else b & 0xFFFFFFFF
        elif op in (30, 31, 35, 40, 43, 49, 55, 57, 63):
            address = (self.known(a, "memory address") + displacement) & 0xFFFFFFFF
            if op in (40, 43, 63, 31):
                self.store(address, {40: 1, 43: 4, 63: 8, 31: 16}[op], b)
            elif op == 57:
                self.store(address, 4, self.floats[rt])
            elif op == 49:
                self.floats[rt] = self.load(address, 4)
            else:
                count = {30: 16, 35: 4, 55: 8}[op]
                value = self.load(address, count)
                self.registers[rt] = None if value is None or count == 16 else (signed(value, 32) & MASK if count == 4 else value)
        else:
            raise AuditError(f"Unsupported opcode at {pc:08X}: {word:08X}")
        self.registers[0] = 0

    def delay(self, pc):
        word = self.word(pc)
        if word >> 26 in (2, 3, 4, 5, 20, 21) or (word >> 26 == 0 and word & 63 == 8):
            raise AuditError("Control transfer in delay slot")
        self.simple(pc, word)

    def run(self):
        pc = self.start
        while True:
            word = self.word(pc)
            op, rs, rt = word >> 26, word >> 21 & 31, word >> 16 & 31
            if op in (4, 5, 20, 21):
                equal = self.known(self.registers[rs], "branch") == self.known(self.registers[rt], "branch")
                taken = equal == (op in (4, 20))
                if taken or op in (4, 5):
                    self.delay(pc + 4)
                pc = pc + 4 + signed(word & 65535, 16) * 4 if taken else pc + 8
            elif op in (2, 3):
                target = ((pc + 4) & 0xF0000000) | ((word & 0x3FFFFFF) << 2)
                if op == 2:
                    self.delay(pc + 4)
                    pc = target
                    continue
                if target != self.memset_address:
                    raise AuditError(f"Unapproved external call {target:08X}")
                self.registers[31] = pc + 8
                self.delay(pc + 4)
                address, byte, count = [self.known(self.registers[i], "memset argument") for i in (4, 5, 6)]
                if count > 1048576:
                    raise AuditError("Unbounded memset hook")
                self.calls.append({"target": f"{target:08X}", "destination": f"{address:08X}", "bytes": count})
                for i in range(count):
                    self.store(address + i, 1, byte & 255)
                self.registers[2] = address
                # The semantic hook promises only standardmemset return/saved
                # registers, never arbitrary callee-saved values for extra calls.
                for i in (1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25):
                    self.registers[i] = None
                for i in range(20):
                    self.floats[i] = None
                pc += 8
            elif op == 0 and word & 63 == 8:
                target = self.known(self.registers[rs], "return")
                self.delay(pc + 4)
                if target != 0xFFFF0000:
                    raise AuditError(f"Unapproved indirect jump {target:08X}")
                break
            else:
                self.simple(pc, word)
                pc += 4
        result = []
        for base, size in self.spans:
            values = [self.memory[base + i] for i in range(size)]
            if any(v is None for v in values):
                raise AuditError("Unknown final output")
            data = bytes(values)
            fnv = 14695981039346656037
            for byte in data:
                fnv = ((fnv ^ byte) * 1099511628211) & MASK
            result.append({"address": f"{base:08X}", "bytes": size, "sha256": hashlib.sha256(data).hexdigest(), "fnv64": f"{fnv:016X}"})
        return {"steps": self.steps, "output_store_operations": self.writes, "memset_calls": self.calls, "outputs": result}


def pair(value):
    first, second = value.split(":", 1)
    return int(first, 0), int(second, 0)


def self_test():
    class Synthetic:
        def __init__(self, words):
            self.code = struct.pack("<" + "I" * len(words), *words)
        def read(self, address, count):
            if 0x1000 <= address and address + count <= 0x1000 + len(self.code):
                return self.code[address - 0x1000:address - 0x1000 + count]
            if 0x2000 <= address and address + count <= 0x2010:
                return bytes(count)
            raise AuditError("Synthetic unmapped read")
    # Takenbranch still executes the delaystore; known arbitrary field is7.
    words = [0x24082000, 0x24090007, 0x10000002, 0xAD090000, 0, 0x03E00008, 0]
    report = Engine(Synthetic(words), 0x1000, 0x101C, [(0x2000, 4)], {}).run()
    assert report["outputs"][0]["sha256"] == hashlib.sha256(struct.pack("<I", 7)).hexdigest()
    likely = [0x24082000, 0x24090007, 0x50430001, 0xAD090000, 0x03E00008, 0]
    report = Engine(Synthetic(likely), 0x1000, 0x1018, [(0x2000, 4)], {2: 1, 3: 0}).run()
    assert report["outputs"][0]["sha256"] == hashlib.sha256(bytes(4)).hexdigest()
    failures = [([0x24082000, 0xAD090000, 0x03E00008, 0], "Unknown"),
                ([0x24083000, 0xAD000000, 0x03E00008, 0], "outside"),
                ([0x46000000, 0x03E00008, 0], "Unsupported opcode")]
    for words, expected in failures:
        try:
            Engine(Synthetic(words), 0x1000, 0x1000 + len(words) * 4, [(0x2000, 4)], {}).run()
        except AuditError as error:
            assert expected in str(error), str(error)
        else:
            raise AssertionError("Unknown/outside/unsupported instruction did not fail")
    print("PASS synthetic delay-slot/hash, unknown-value, write-boundary and unknown-opcode audits")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--elf", type=Path)
    parser.add_argument("--function", type=pair, help="start:end virtual addresses; end exclusive")
    parser.add_argument("--span", type=pair, action="append", help="allowed output address:bytes, repeatable")
    parser.add_argument("--register", action="append", default=[], help="register=value, explicit initial input")
    parser.add_argument("--memset", type=lambda x: int(x, 0), help="single verified memset target")
    parser.add_argument("--max-instructions", type=int, default=10000)
    parser.add_argument("--output", type=Path, help="new JSON path; existing files are never overwritten")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    if not args.elf or not args.function or not args.span:
        parser.error("--elf, --function and at least one --span are required")
    start, end = args.function
    if start & 3 or end & 3 or not 0 < end - start <= 1048576 or not 0 < args.max_instructions <= 100000:
        parser.error("Invalid bounded function/instruction extent")
    inputs = {}
    for item in args.register:
        name, value = item.split("=", 1)
        index = 30 if name == "s8" else REGISTERS.index(name)
        if index == 0 or index in inputs:
            parser.error("Duplicate/zero register input")
        inputs[index] = int(value, 0)
    data = args.elf.read_bytes()
    elf = Elf(data)
    report = Engine(elf, start, end, args.span, inputs, args.memset, args.max_instructions).run()
    report.update(schema=1, scope="offline bounded static-write audit, not runtime/game execution proof", elf_sha256=hashlib.sha256(data).hexdigest(), function_start=f"{start:08X}", function_end_exclusive=f"{end:08X}", function_sha256=hashlib.sha256(elf.read(start, end-start)).hexdigest(), initial_registers=args.register)
    text = json.dumps(report, indent=2) + "\n"
    if args.output:
        with args.output.open("x", encoding="utf-8") as stream:
            stream.write(text)
        print(args.output)
    else:
        print(text, end="")


if __name__ == "__main__":
    main()
