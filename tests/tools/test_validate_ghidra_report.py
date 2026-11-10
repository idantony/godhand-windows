"""Synthetic validator regressions; temporary files are confined to G scratch."""
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import time
import unittest

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("ghidra_report_validator", ROOT/"tools/validate-ghidra-report.py")
v = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v)
SCRATCH = Path("G:/middle_thing/godhand_windows/build/ghidra-validator-tests")
SCRATCH.mkdir(parents=True, exist_ok=True)


def image():
    data = bytearray(0x110)
    data[:7] = b"\x7fELF\x01\x01\x01"
    struct.pack_into("<HHIIIIIHHHHHH", data, 16, 2, 8, 1, 0x1000, 52, 0, 0, 52, 32, 1, 40, 0, 0)
    struct.pack_into("<8I", data, 52, 1, 0x100, 0x1000, 0x1000, 16, 32, 5, 16)
    struct.pack_into("<4I", data, 0x100, 0x03e00008, 0, 0, 0)
    return bytes(data)


class Tests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(dir=SCRATCH)
        self.root = Path(self.tmp.name)
        self.elf = self.root/"image.elf"
        self.elf.write_bytes(image())
        self.report = self.root/"report.json"
        self.log = self.root/"run.log"
        self.manifest = v.prepare(self.elf, ["0x00001000"], self.report, [self.log], 1)
        self.payload = {"elfSha256": hashlib.sha256(image()).hexdigest(), "elfSize": len(image()),
                        "imageBase": "00001000", "rawInstructionLimit": 1,
                        "functions": [{"requestedAddress": "00001000", "entry": "00001000",
                                       "mapped": True, "instructionCount": 1, "instructionLimit": 1,
                                       "disassemblyTruncated": True, "decompileCompleted": True,
                                       "decompiledC": "void actual_fixture(void) {}",
                                       "rawInstructions": [{"address": "00001000", "bytesLittleEndian": "0800e003"}]}]}
        self.write()

    def tearDown(self):
        self.tmp.cleanup()

    def write(self):
        self.report.write_text(json.dumps(self.payload))
        self.log.write_text("INFO GODHAND_PROBE_OUTPUT="+str(self.report.resolve())+"\n")

    def rejects(self, match=None):
        with self.assertRaisesRegex(v.ValidationError, match or "."):
            v.validate(self.manifest, 0)

    def test_normalization_dedup_and_types(self):
        self.assertEqual(v.addresses([" 0X0012E060 ", "0012e060", "1000"]), ["0012e060", "00001000"])
        for token in [1.2e61, 1000, True, None, "1.2E+61", "0012e060,00133888", "FFFFFFFFF", "1001"]:
            with self.subTest(token=token), self.assertRaises(v.ValidationError):
                v.address(token)

    def test_valid_bounded_truncated_report_preserved(self):
        raw, result = v.validate(self.manifest, 0)
        destination = self.root/"published.json"
        v.publish(destination, raw)
        self.assertEqual(destination.read_bytes(), self.report.read_bytes())
        self.assertEqual(result["addresses"], ["00001000"])
        self.assertTrue(json.loads(raw)["functions"][0]["disassemblyTruncated"])

    def test_prepare_rejects_old_candidate_or_log(self):
        with self.assertRaises(v.ValidationError):
            v.prepare(self.elf, ["1000"], self.report, [self.log], 1)

    def test_scientific_numeric_and_unmapped_request_rejected_before_probe(self):
        for value in [1.2e61, 1000, "2000"]:
            with self.subTest(value=value), self.assertRaises(v.ValidationError):
                v.prepare(self.elf, [value], self.root/"fresh.json", [self.root/"fresh.log"], 1)

    def test_headless_zero_with_java_error_rejected(self):
        for text in ["ERROR REPORT SCRIPT ERROR", "java.lang.NullPointerException: null", "FATAL execution"]:
            self.write()
            with self.log.open("a") as stream:
                stream.write(text)
            self.rejects()

    def test_nonzero_headless(self):
        with self.assertRaises(v.ValidationError):
            v.validate(self.manifest, 1)

    def test_missing_json_despite_zero_exit(self):
        self.report.unlink()
        self.rejects("missing")

    def test_missing_or_wrong_completion_marker(self):
        self.log.write_text("INFO completed unrelated report.json")
        self.rejects("marker")

    def test_stale_report_and_stale_log(self):
        for target in [self.report, self.log]:
            self.write()
            old = self.manifest["started_ns"]-10_000_000_000
            os.utime(target, ns=(old, old))
            self.rejects("stale")

    def test_missing_requested_entry_mismatch_mapping_and_error(self):
        base = copy.deepcopy(self.payload)
        for patch in [{"requestedAddress": "1004"}, {"entry": "1004"}, {"mapped": False}, {"error": "No function"}]:
            self.payload = copy.deepcopy(base)
            self.payload["functions"][0].update(patch)
            self.write()
            self.rejects()

    def test_empty_raw_or_incomplete_decompile(self):
        base = copy.deepcopy(self.payload)
        for patch in [{"rawInstructions": []}, {"instructionCount": 0}, {"decompileCompleted": False}, {"decompiledC": " "}]:
            self.payload = copy.deepcopy(base)
            self.payload["functions"][0].update(patch)
            self.write()
            self.rejects()

    def test_raw_hash_cap_and_image_identity(self):
        base = copy.deepcopy(self.payload)
        for patch in [{"elfSha256": "0"*64}, {"elfSize": 1}, {"imageBase": "2000"}, {"rawInstructionLimit": 2}]:
            self.payload = copy.deepcopy(base)
            self.payload.update(patch)
            self.write()
            self.rejects()
        self.payload = copy.deepcopy(base)
        self.payload["functions"][0]["rawInstructions"][0]["bytesLittleEndian"] = "00000000"
        self.write()
        self.rejects("raw instruction")

    def test_changed_elf_and_bounds(self):
        original = self.elf.read_bytes()
        changed = bytearray(original)
        changed[-1] ^= 1
        self.elf.write_bytes(changed)
        self.rejects("changed")
        broken = bytearray(original)
        struct.pack_into("<I", broken, 52+4, len(broken)+1)
        self.elf.write_bytes(broken)
        with self.assertRaisesRegex(v.ValidationError, "bounds"):
            v.elf_image(self.elf)

    def test_exclusive_publish_does_not_overwrite_old_evidence(self):
        destination = self.root/"old.json"
        destination.write_bytes(b"OLD REPORT")
        with self.assertRaises(FileExistsError):
            v.publish(destination, b"new report")
        self.assertEqual(destination.read_bytes(), b"OLD REPORT")


if __name__ == "__main__":
    unittest.main()
