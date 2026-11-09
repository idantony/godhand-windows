"""Check whether Unicorn can serve as a restricted offline MIPS test oracle.

This development tool is never linked into the native game. Unicorn has no
R5900 CPU model; this probe refuses to label it a PS2 hardware oracle.
"""
from pathlib import Path
import struct
import sys
from unicorn import Uc, UC_ARCH_MIPS, UC_MODE_LITTLE_ENDIAN, UC_MODE_MIPS64, UC_HOOK_CODE, UcError
import unicorn.mips_const as m

sys.path.insert(0, str(Path(__file__).parent))
from inspect_disc import parse_elf


def main():
    root = Path(__file__).resolve().parents[1]
    blob = (root / "reference" / "SLUS_215.03").read_bytes()
    elf = parse_elf(blob)
    vm = Uc(UC_ARCH_MIPS, UC_MODE_MIPS64 | UC_MODE_LITTLE_ENDIAN)
    vm.mem_map(0x100000, 0x700000)
    for ph in elf["program_headers"]:
        if ph["type"] == 1:
            vm.mem_write(ph["vaddr"], blob[ph["offset"]:ph["offset"] + ph["filesz"]])
    vm.mem_map(0x900000, 0x10000)
    vm.reg_write(m.UC_MIPS_REG_CP0_STATUS, vm.reg_read(m.UC_MIPS_REG_CP0_STATUS) | (1 << 29))
    vm.reg_write(m.UC_MIPS_REG_RA, 0x900000)
    vm.reg_write(m.UC_MIPS_REG_SP, 0x90FFF0)
    vm.mem_write(0x747B14, struct.pack("<I", 1))
    trace = []
    def instruction(uc, address, size, data):
        trace.append(hex(address))
    vm.hook_add(UC_HOOK_CODE, instruction)
    try:
        vm.emu_start(0x31CF78, 0x900000, count=100)
        print({"backend_completed": True, "state": vm.mem_read(0x747B14, 4).hex(),
               "f0": hex(vm.reg_read(m.UC_MIPS_REG_F0)), "trace": trace,
               "ps2_hardware_validation": False})
    except UcError as error:
        print({"backend_completed": False, "error": str(error), "trace": trace,
               "ps2_hardware_validation": False})
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
