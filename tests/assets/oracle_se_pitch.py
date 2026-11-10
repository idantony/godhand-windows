"""Read user's ISO; execute only bounded integer SDK leaf oracles, never game code.
Independent of native se_pitch.c. No original bytes/tables/PCM are emitted.
Usage: python oracle_se_pitch.py ISO > PRIVATE_REPORT.json
"""
from pathlib import Path
import hashlib
import json
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from inspect_disc import Iso9660

def signed(x, bits=32):
    x &= (1 << bits) - 1
    return x - (1 << bits) if x >> (bits - 1) else x

class Image:
    def __init__(self, data):
        self.data = data
        ph, stride, count = struct.unpack_from("<I", data, 28)[0], *struct.unpack_from("<HH", data, 42)
        self.segments = [struct.unpack_from("<8I", data, ph + i * stride) for i in range(count)]
    def read(self, address, count):
        for kind, off, va, _, size, _, _, _ in self.segments:
            if kind == 1 and va <= address and address + count <= va + size:
                return self.data[off + address - va:off + address - va + count]
        raise ValueError(f"unmapped oracle read {address:x}/{count}")

def leaf(image, entry, args, state=None):
    r = [0] * 32
    r[4:4+len(args)] = [a & 0xffffffff for a in args]
    r[31] = 0xfffffff0
    pc, npc, hi, lo = entry, entry + 4, 0, 0
    memory = bytearray(struct.pack("<H", state)) if state is not None else None
    for _ in range(256):
        if pc == 0xfffffff0:
            return r[2], struct.unpack("<H", memory)[0] if memory is not None else None
        ins = struct.unpack("<I", image.read(pc, 4))[0]
        op, rs, rt, rd, sh, fn = ins >> 26, (ins >> 21) & 31, (ins >> 16) & 31, (ins >> 11) & 31, (ins >> 6) & 31, ins & 63
        imm = signed(ins, 16)
        next_pc = npc + 4
        if op == 0:
            if fn == 0: r[rd] = r[rt] << sh
            elif fn == 3: r[rd] = signed(r[rt]) >> sh
            elif fn == 4: r[rd] = r[rt] << (r[rs] & 31)
            elif fn == 6: r[rd] = r[rt] >> (r[rs] & 31)
            elif fn == 8: next_pc = r[rs]
            elif fn == 0x10: r[rd] = hi
            elif fn == 0x12: r[rd] = lo
            elif fn == 0x18:
                product = signed(r[rs]) * signed(r[rt])
                hi, lo = (product >> 32) & 0xffffffff, product & 0xffffffff
            elif fn in (0x21, 0x2d): r[rd] = r[rs] + r[rt]
            elif fn == 0x23: r[rd] = r[rs] - r[rt]
            elif fn == 0x25: r[rd] = r[rs] | r[rt]
            else: raise ValueError(f"unsupported oracle SPECIAL {ins:08x}")
        elif op == 1:
            take = signed(r[rs]) < 0 if rt == 0 else signed(r[rs]) >= 0 if rt == 1 else None
            if take is None: raise ValueError("REGIMM")
            if take: next_pc = pc + 4 + imm * 4
        elif op == 5:
            if r[rs] != r[rt]: next_pc = pc + 4 + imm * 4
        elif op == 9: r[rt] = r[rs] + imm
        elif op == 12: r[rt] = r[rs] & (ins & 65535)
        elif op == 13: r[rt] = r[rs] | (ins & 65535)
        elif op == 15: r[rt] = (ins & 65535) << 16
        elif op in (0x21, 0x25, 0x29):
            address = (r[rs] + imm) & 0xffffffff
            if memory is not None and address == 0x76a7b0:
                if op == 0x29: struct.pack_into("<H", memory, 0, r[rt] & 65535)
                else:
                    r[rt] = struct.unpack("<H", memory)[0]
                    if op == 0x21: r[rt] = signed(r[rt], 16)
            elif op == 0x25: r[rt] = struct.unpack("<H", image.read(address, 2))[0]
            else: raise ValueError(f"unapproved mutable oracle address {address:x}")
        else: raise ValueError(f"unsupported oracle opcode {ins:08x}")
        r = [v & 0xffffffff for v in r]
        r[0] = 0
        pc, npc = npc, next_pc & 0xffffffff
    raise ValueError("oracle instruction budget exhausted")

def add(h, v):
    for b in struct.pack("<I", v & 0xffffffff): h = ((h ^ b) * 1099511628211) & 0xffffffffffffffff
    return h

def main():
    if len(sys.argv) != 2: raise SystemExit("usage: oracle_se_pitch.py ISO")
    iso = Iso9660(Path(sys.argv[1]))
    try:
        entries = {e["path"]:e for e in iso.inventory()}
        elf_data = iso.contents(entries["/SLUS_215.03"])
        ims = iso.contents(entries["/MODULES/IRX.IMS"])
    finally: iso.close()
    modules = {}
    for i in range(struct.unpack_from("<I", ims, 8)[0]):
        name, off, size = struct.unpack_from("<16sII", ims, 16 + 32*i)
        modules[name.rstrip(b"\0")] = ims[off:off+size]
    lib_data = modules[b"libsd.irx"]
    elf, lib = Image(elf_data), Image(lib_data)
    rng_hash = 14695981039346656037
    for state in range(65536):
        value, after = leaf(elf, 0x377fa0, [], state)
        rng_hash = add(add(rng_hash, after), -128 + (value % 17)*16)
    pitch_hash = 14695981039346656037
    errors = 0
    for offset in range(-512, 513):
        whole = abs(offset)//128 * (-1 if offset < 0 else 1)
        fine = offset - whole*128
        note, _ = leaf(lib, 0x3bd0, [12, 0, (whole+12)&65535, fine])
        for rate in (1000, 14000, 16000, 18000, 48000):
            pitch = ((signed(note,16)*rate)&0xffffffff)//48000 & 65535
            pitch = min(pitch,0x3fff)
            if not pitch: errors += 1
            pitch_hash = add(pitch_hash, pitch)
    print(json.dumps({"schema":1,"oracle":"bounded actual MIPS integer leaves, independent native source", "elf_sha256":hashlib.sha256(elf_data).hexdigest(), "libsd_sha256":hashlib.sha256(lib_data).hexdigest(), "rng_states":65536,"rng_fnv64":f"{rng_hash:016x}","pitch_cases":1025*5,"pitch_fnv64":f"{pitch_hash:016x}","zero_pitch_cases":errors},indent=2))
if __name__ == "__main__": main()
