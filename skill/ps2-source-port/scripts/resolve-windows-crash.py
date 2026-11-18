"""Resolve a Windows fault offset against the executable's local PDB."""

import argparse
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import sys


class SourceLine(ctypes.Structure):
    _fields_ = [
        ("structure_size", wintypes.DWORD),
        ("key", ctypes.c_void_p),
        ("line", wintypes.DWORD),
        ("filename", wintypes.LPWSTR),
        ("address", ctypes.c_uint64),
    ]


def source_location(executable, offset):
    dbghelp = ctypes.WinDLL("dbghelp.dll", use_last_error=True)

    def api(name, result_type, *arguments):
        function = getattr(dbghelp, name)
        function.restype = result_type
        function.argtypes = arguments
        return function

    handle, boolean, dword = wintypes.HANDLE, wintypes.BOOL, wintypes.DWORD
    address, wide_string = ctypes.c_uint64, wintypes.LPCWSTR
    settings = api("SymSetOptions", dword, dword)
    load_lines, exact_symbols, ignore_environment, no_prompts = 0x10, 0x400, 0x1000, 0x80000
    settings(load_lines | exact_symbols | ignore_environment | no_prompts)
    initialize = api("SymInitializeW", boolean, handle, wide_string, boolean)
    load = api("SymLoadModuleExW", address, handle, handle, wide_string, wide_string,
               address, dword, ctypes.c_void_p, dword)
    line_from_address = api("SymGetLineFromAddrW64", boolean, handle, address,
                            ctypes.POINTER(dword), ctypes.POINTER(SourceLine))
    cleanup = api("SymCleanup", boolean, handle)
    session = handle(1)
    if not initialize(session, str(executable.parent), False):
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        base = load(session, None, str(executable), None, 0x10000000, 0, None, 0)
        if not base:
            raise ctypes.WinError(ctypes.get_last_error())
        location = SourceLine(structure_size=ctypes.sizeof(SourceLine))
        displacement = dword()
        if not line_from_address(session, base + offset, ctypes.byref(displacement), ctypes.byref(location)):
            raise ctypes.WinError(ctypes.get_last_error())
        return {"module": str(executable), "offset": f"0x{offset:x}",
                "source": location.filename, "line": location.line}
    finally:
        cleanup(session)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("offset", help="The hexadecimal faulting module offset, such as 0x12ab")
    args = parser.parse_args()
    if sys.platform != "win32":
        parser.error("Run this tool on Windows")
    try:
        offset = int(args.offset, 16)
        if not 0 <= offset <= 0xFFFFFFFF:
            raise ValueError("Use a 32-bit module offset")
        executable = args.executable.resolve(strict=True)
        result = source_location(executable, offset)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Could not resolve the offset with local symbols: {error}\n")
    print(json.dumps(result, ensure_ascii=True))


if __name__ == "__main__":
    main()
