"""Create a synthetic first-room save for the isolated native title QA run."""

import argparse
import hashlib
import struct
import tempfile
from pathlib import Path


SCRATCH_ROOT = Path(r"G:\middle_thing\godhand_windows").resolve()
SLOT = "BASLUS-21503GODHD003"


def build_record():
    record = bytearray(0xB400)
    struct.pack_into("<IIIHH", record, 0, 0x3DCCCCCD, 0, 1, 0x100, 1)
    # sceCdCLOCK, BCD second/minute/hour/day/month/year. A real timestamp
    # matters because the original chooser starts at slot 0 for all-zero dates.
    record[0x12:0x1A] = bytes.fromhex("00 20 15 11 00 11 11 26")
    struct.pack_into("<HHH", record, 0x2C, 0x100, 0x100, 1)
    struct.pack_into("<II", record, 0xA0, 100, 73)
    struct.pack_into("<III", record, 0x190, 1, 1, 1)
    struct.pack_into("<8f", record, 0xB0F0, 0, 0, 24, 0, 0, 0, 18, 120)
    checksum = 0x9BE6FA3B72AFDA1D
    for (word,) in struct.iter_unpack("<Q", record[:0xB108]):
        checksum ^= word
    struct.pack_into("<Q", record, 0xB110, checksum)
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scratch", type=Path, required=True)
    args = parser.parse_args()
    scratch = args.scratch.resolve()
    if SCRATCH_ROOT not in scratch.parents:
        parser.error("Scratch must be below the project G: scratch root")
    scratch.mkdir(parents=True, exist_ok=True)
    profile = scratch / "profile-load"
    if profile.exists() or profile.is_symlink():
        parser.error("The QA profile already exists")
    record = build_record()
    with tempfile.TemporaryDirectory(prefix="title-load-", dir=scratch) as staging:
        temporary_profile = Path(staging) / "profile-load"
        slot = temporary_profile / SLOT
        slot.mkdir(parents=True)
        (slot / SLOT).write_bytes(record)
        (slot / "icon.sys").write_bytes(bytes(0x3C4))
        temporary_profile.rename(profile)
    print(f"{profile / SLOT / SLOT} SHA256 {hashlib.sha256(record).hexdigest()}")


if __name__ == "__main__":
    main()
