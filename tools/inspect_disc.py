"""Read-only ISO9660 and ELF baseline for the God Hand source-port project."""
import argparse
import collections
import datetime
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import struct

SECTOR = 2048


def hashes_file(path):
    digests = {name: hashlib.new(name) for name in ("sha256", "sha1")}
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(8 * 1024 * 1024), b""):
            for digest in digests.values():
                digest.update(block)
    return {name: digest.hexdigest() for name, digest in digests.items()}


class Iso9660:
    def __init__(self, path):
        self.path = path
        self.stream = path.open("rb")
        self.size = path.stat().st_size
        self.pvd = None
        for sector in range(16, 80):
            descriptor = self.read(sector * SECTOR, SECTOR)
            if descriptor[1:6] != b"CD001":
                continue
            if descriptor[0] == 1:
                self.pvd = descriptor
                break
        if self.pvd is None:
            raise ValueError("No ISO9660 primary volume descriptor found")
        self.block_size = struct.unpack_from("<H", self.pvd, 128)[0]
        if self.block_size != SECTOR:
            raise ValueError("Only 2048-byte logical blocks are supported")
        self.volume_id = self.pvd[40:72].decode("ascii", "replace").strip()
        self.root = self.record(self.pvd[156:190])

    def close(self):
        self.stream.close()

    def read(self, offset, size):
        if offset < 0 or size < 0 or offset + size > self.size:
            raise ValueError("ISO record extends outside the image")
        self.stream.seek(offset)
        result = self.stream.read(size)
        if len(result) != size:
            raise ValueError("Truncated ISO read")
        return result

    @staticmethod
    def record(data):
        if len(data) < 34:
            raise ValueError("Short ISO directory record")
        lba, size = struct.unpack_from("<I", data, 2)[0], struct.unpack_from("<I", data, 10)[0]
        if lba != struct.unpack_from(">I", data, 6)[0] or size != struct.unpack_from(">I", data, 14)[0]:
            raise ValueError("ISO dual-endian record fields disagree")
        length = data[32]
        name = data[33:33 + length]
        return {"lba": lba, "size": size, "flags": data[25], "name_raw": name}

    def inventory(self):
        pending = [("", self.root)]
        visited = set()
        entries = []
        while pending:
            parent, directory = pending.pop()
            key = (directory["lba"], directory["size"])
            if key in visited:
                continue
            visited.add(key)
            if len(visited) > 10000:
                raise ValueError("Unreasonable directory count")
            if directory["size"] > 32 * 1024 * 1024:
                raise ValueError("Unreasonable directory size")
            raw = self.read(directory["lba"] * SECTOR, directory["size"])
            position = 0
            while position < len(raw):
                length = raw[position]
                if length == 0:
                    position = (position // SECTOR + 1) * SECTOR
                    continue
                if position + length > len(raw):
                    raise ValueError("Truncated directory record")
                record = self.record(raw[position:position + length])
                position += length
                if record["name_raw"] in (b"\x00", b"\x01"):
                    continue
                name = record.pop("name_raw").decode("ascii", "strict").split(";", 1)[0]
                if name in (".", "..") or "/" in name or "\\" in name:
                    raise ValueError("Unsafe ISO filename")
                record["path"] = parent + "/" + name
                record["directory"] = bool(record["flags"] & 2)
                if record["lba"] * SECTOR + record["size"] > self.size:
                    raise ValueError("Entry outside disc: " + record["path"])
                entries.append(record)
                if len(entries) > 100000:
                    raise ValueError("Unreasonable file count")
                if record["directory"]:
                    pending.append((record["path"], record))
        return sorted(entries, key=lambda entry: entry["path"])

    def contents(self, entry, limit=64 * 1024 * 1024):
        if entry["directory"] or entry["size"] > limit or entry["flags"] & 0x80:
            raise ValueError("Entry cannot be extracted by this bounded reader")
        return self.read(entry["lba"] * SECTOR, entry["size"])


def parse_elf(blob):
    if blob[:4] != b"\x7fELF" or blob[4:6] != b"\x01\x01":
        raise ValueError("Expected ELF32 little-endian PS2 executable")
    header = struct.unpack_from("<HHIIIIIHHHHHH", blob, 16)
    kind, machine, version, entry, phoff, shoff, flags, ehsize, phsize, phnum, shsize, shnum, shstr = header
    programs = []
    for index in range(phnum):
        fields = struct.unpack_from("<IIIIIIII", blob, phoff + index * phsize)
        programs.append(dict(zip(("type", "offset", "vaddr", "paddr", "filesz", "memsz", "flags", "align"), fields)))
    raw_sections = [struct.unpack_from("<IIIIIIIIII", blob, shoff + index * shsize) for index in range(shnum)]
    strings = b""
    if raw_sections and shstr < len(raw_sections):
        section = raw_sections[shstr]
        strings = blob[section[4]:section[4] + section[5]]
    sections = []
    for fields in raw_sections:
        offset = fields[0]
        end = strings.find(b"\x00", offset)
        name = strings[offset:end].decode("ascii", "replace") if 0 <= offset < len(strings) and end >= 0 else ""
        section = dict(zip(("name_offset", "type", "flags", "vaddr", "offset", "size", "link", "info", "align", "entry_size"), fields))
        section["name"] = name
        sections.append(section)
    return {"class": 32, "endian": "little", "type": kind, "machine": machine, "version": version,
            "entry": entry, "entry_hex": hex(entry), "flags": flags, "flags_hex": hex(flags),
            "program_headers": programs, "sections": sections,
            "has_symbol_table": any(section["type"] == 2 for section in sections),
            "has_mdebug": any(section["name"] == ".mdebug" for section in sections)}


def write_json(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("iso", type=Path)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    project = args.project.resolve()
    iso_path = args.iso.resolve()
    before = iso_path.stat()
    iso = Iso9660(iso_path)
    try:
        entries = iso.inventory()
        files = {entry["path"].upper(): entry for entry in entries if not entry["directory"]}
        system = iso.contents(files["/SYSTEM.CNF"], limit=1024 * 1024)
        match = re.search(rb"BOOT2\s*=\s*cdrom0:[\\/]+([^;\r\n]+)", system, re.IGNORECASE)
        if not match:
            raise ValueError("No BOOT2 entry in SYSTEM.CNF")
        boot_path = "/" + match.group(1).decode("ascii").replace("\\", "/").strip()
        boot_entry = files[boot_path.upper()]
        boot = iso.contents(boot_entry)
        elf = parse_elf(boot)
        reference = project / "reference"
        reference.mkdir(parents=True, exist_ok=True)
        (reference / "SYSTEM.CNF").write_bytes(system)
        boot_local = reference / PurePosixPath(boot_path).name
        boot_local.write_bytes(boot)
        extensions = collections.Counter(PurePosixPath(entry["path"]).suffix.upper() or "<none>" for entry in files.values())
        baseline = {"schema_version": 1, "created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                    "source_iso": {"path": str(iso_path), "size": before.st_size, **hashes_file(iso_path)},
                    "volume_id": iso.volume_id, "system_cnf": system.decode("ascii", "replace"),
                    "file_count": len(files), "directory_count": len(entries) - len(files),
                    "extensions": dict(extensions.most_common()),
                    "boot": {"disc_path": boot_path, "local_path": str(boot_local), "size": len(boot),
                             "sha256": hashlib.sha256(boot).hexdigest(), "sha1": hashlib.sha1(boot).hexdigest(), "elf": elf},
                    "input_filename": iso_path.name,
                    "input_note": "Retail identity and patch equivalence require independent hashes; neither is inferred from the input filename."}
        after = iso_path.stat()
        if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
            raise ValueError("Input ISO changed during inspection")
        write_json(project / "analysis" / "disc-inventory.json", entries)
        write_json(project / "analysis" / "disc-baseline.json", baseline)
        print(json.dumps({"volume": iso.volume_id, "files": len(files), "directories": len(entries) - len(files),
                          "boot": boot_path, "boot_size": len(boot), "boot_sha1": baseline["boot"]["sha1"],
                          "entry": elf["entry_hex"], "flags": elf["flags_hex"], "symbol_table": elf["has_symbol_table"],
                          "iso_sha256": baseline["source_iso"]["sha256"]}, ensure_ascii=False))
    finally:
        iso.close()


if __name__ == "__main__":
    main()
