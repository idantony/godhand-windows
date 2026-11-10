"""Synthetic public tests only; no game bytes or generated private evidence."""
import contextlib
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True
PROJECT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("function_context", PROJECT / "tools/function-context.py")
fc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fc)


def make_rel():
    b = bytearray(0x300)
    h = [0] * 15
    h[0], h[1], h[3], h[4], h[11] = 0x32524E53, 0x200, 0x100, 2, len(b)
    struct.pack_into("<15I", b, 0, *h)
    struct.pack_into("<IIHBB", b, 0x10C, 0x180, 0x40, 0, 2, 0)
    b[0x180:0x188] = b"FakeFoo\0"
    struct.pack_into("<II", b, 0x40, 0x03E00008, 0)
    return bytes(b)


def make_elf(overlay=None):
    payload = bytearray(0x30A600)
    struct.pack_into("<4I", payload, 0, 0x0C000000, 0, 0x03E00008, 0)
    h = [0] * 15
    h[0], h[1], h[2], h[3], h[4] = 0x32524E53, 0x40A440, 1, 0x40A460, 2
    struct.pack_into("<15I", payload, 0x30A400, *h)
    struct.pack_into("<IBHBI", payload, 0x30A440, 0x100000, 4, 1, 0, 0)
    struct.pack_into("<IIHBB", payload, 0x30A46C, 0x40A490, 0, 0, 1, 0)
    payload[0x30A490:0x30A498] = b"FakeFoo\0"
    b = bytearray(0x100) + payload
    b[:7] = b"\x7fELF\x01\x01\x01"
    struct.pack_into("<HHI", b, 16, 2, 8, 1)
    struct.pack_into("<III", b, 24, 0x100000, 52, 0)
    struct.pack_into("<IHHH", b, 36, 0x20924001, 52, 32, 2 if overlay else 1)
    struct.pack_into("<8I", b, 52, 1, 0x100, 0x100000, 0x100000, len(payload), len(payload), 7, 16)
    if overlay:
        off = len(b)
        b += overlay
        struct.pack_into("<8I", b, 84, 1, off, 0, 0, len(overlay), len(overlay), 7, 16)
    return bytes(b)


def make_report(binary, overlay_function=False):
    start, words = (0x40, [0x03E00008, 0]) if overlay_function else (0x100000, [0x0C000000, 0, 0x03E00008, 0])
    raw = [{"address": f"{start + i * 4:08x}", "instruction": "synthetic instruction",
            "bytesLittleEndian": struct.pack("<I", word).hex(), "wordHex": f"0x{word:08x}",
            "flowTargets": ["00000000"] if word == 0x0C000000 else []} for i, word in enumerate(words)]
    return {"program": "synthetic.elf", "executablePath": "synthetic.elf", "imageBase": "00000000" if len(binary) > 0x30A700 else "00100000",
            "elfSha256": hashlib.sha256(binary).hexdigest(), "elfSize": len(binary), "elfMachine": 8,
            "functions": [{"requestedAddress": f"{start:08x}", "entry": f"{start:08x}", "mapped": True,
                "name": "FUN_synthetic", "bodyAddresses": len(words) * 4, "rawInstructions": raw,
                "outgoingCalls": [raw[0]] if not overlay_function else [], "decompileCompleted": True,
                "decompiledC": "void synthetic(void) {}", "decompilerWarnings": ["synthetic warning"]}]}


class ContextTests(unittest.TestCase):
    def setUp(self):
        scratch = Path(os.environ.get("GH_TEST_SCRATCH", "G:/middle_thing/godhand_windows/build/function-context-tests"))
        scratch.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="owned-", dir=scratch)
        self.root = Path(self.temp.name).resolve()
        self.assertTrue(self.root.is_relative_to(scratch.resolve()))
        self.elf, self.report = self.root / "synthetic.elf", self.root / "report.json"
        self.binary = make_elf()
        self.document = make_report(self.binary)
        self.save()

    def tearDown(self):
        self.temp.cleanup()

    def save(self):
        self.elf.write_bytes(self.binary)
        self.document["elfSha256"] = hashlib.sha256(self.binary).hexdigest()
        self.document["elfSize"] = len(self.binary)
        self.report.write_text(json.dumps(self.document), encoding="utf-8")

    def build(self, **kwargs):
        return fc.build_context(self.report, self.elf, [0x100000], self.root, search=False, **kwargs)

    def test_real_hash_ranges_and_explicit_unknown_import(self):
        result = self.build()
        f = result["functions"][0]
        self.assertTrue(result["input"]["report_hash_verified"])
        self.assertEqual(f["ranges"][0]["sha256"], hashlib.sha256(self.binary[0x100:0x110]).hexdigest())
        self.assertEqual(f["calls"][0]["import"]["symbol"], "FakeFoo")
        self.assertEqual(f["calls"][0]["import"]["status"], "symbol_only")
        self.assertEqual(f["decompile"]["warnings"], ["synthetic warning"])

    def test_resolve_rel_definition_and_missing_symbol(self):
        rel = self.root / "synthetic.rel"
        rel.write_bytes(make_rel())
        result = self.build(rel_paths=[rel])
        imp = result["functions"][0]["calls"][0]["import"]
        self.assertEqual(imp["status"], "resolved_identity")
        self.assertEqual(imp["definitions"][0]["address"], "00000040")
        b = bytearray(self.binary)
        struct.pack_into("<H", b, 0x100 + 0x30A440 + 5, 9)
        self.binary = bytes(b)
        self.save()
        missing = self.build()["functions"][0]["calls"][0]["import"]
        self.assertEqual(missing["status"], "unknown")
        self.assertEqual(missing["reason"], "missing relocation symbol")

    def test_hash_size_image_and_raw_mismatches(self):
        for key, value, message in [("elfSha256", "0" * 64, "elfSha256"),
                                    ("elfSize", 1, "elfSize"), ("imageBase", "00000000", "imageBase")]:
            with self.subTest(key=key):
                doc = json.loads(json.dumps(self.document)); doc[key] = value
                self.report.write_text(json.dumps(doc))
                with self.assertRaisesRegex(fc.EvidenceError, message): self.build()
        self.save()
        self.document["functions"][0]["rawInstructions"][0]["bytesLittleEndian"] = "00000000"
        self.save()
        with self.assertRaisesRegex(fc.EvidenceError, "raw instruction/source mismatch"): self.build()

    def test_call_target_and_word_metadata_mismatch(self):
        item = self.document["functions"][0]["rawInstructions"][0]
        item["wordHex"] = "0x00000000"
        self.save()
        with self.assertRaisesRegex(fc.EvidenceError, "wordHex"): self.build()
        item["wordHex"] = "0x0c000000"
        self.document["functions"][0]["outgoingCalls"][0]["flowTargets"] = ["00123450"]
        self.save()
        with self.assertRaisesRegex(fc.EvidenceError, "target disagrees"): self.build()

    def test_requested_entry_difference_and_ambiguity(self):
        self.document["functions"][0]["requestedAddress"] = "00100004"
        self.save()
        result = fc.build_context(self.report, self.elf, [0x100004], self.root, search=False)
        self.assertTrue(result["functions"][0]["requested_entry_disagreement"])
        self.assertTrue(result["functions"][0]["requested_in_verified_body"])
        duplicate = dict(self.document["functions"][0]); duplicate["entry"] = "00100008"
        self.document["functions"].append(duplicate)
        self.save()
        with self.assertRaisesRegex(fc.EvidenceError, "2 matching"): fc.build_context(self.report, self.elf, [0x100004], self.root, search=False)
        with self.assertRaisesRegex(fc.EvidenceError, "0 matching"): fc.build_context(self.report, self.elf, [0x123456], self.root, search=False)

    def test_unmapped_probe_and_bounded_candidate_search(self):
        self.document["functions"].append({"requestedAddress": "00999999", "entry": None, "mapped": False})
        self.save()
        self.assertEqual(len(self.build()["functions"]), 1)
        with self.assertRaisesRegex(fc.EvidenceError, "not mapped"):
            fc.build_context(self.report, self.elf, [], self.root, search=False)
        if not fc.shutil.which("rg"):
            self.skipTest("rg unavailable; actual project QA also checks candidate search")
        (self.root / "src").mkdir()
        (self.root / "src/candidate.c").write_text("/* candidate at 00100000 */\n")
        match = fc.candidates(self.root, 0x100000, [], 16)
        self.assertEqual(match["status"], "ok")
        self.assertEqual(match["match_count"], 1)
        self.assertIn("candidate locations only", match["policy"])

    def test_wrapper_manifest_and_original_range_identity(self):
        rel = self.root / "original.rel"; rel.write_bytes(make_rel())
        self.binary = make_elf(rel.read_bytes()); self.document = make_report(self.binary, True); self.save()
        manifest = self.root / "wrapper.json"
        record = {"wrapper_sha256": hashlib.sha256(self.binary).hexdigest(), "source_rel_sha256": hashlib.sha256(rel.read_bytes()).hexdigest()}
        manifest.write_text(json.dumps(record))
        result = fc.build_context(self.report, self.elf, [0x40], self.root, [rel], search=False, wrapper_manifest=manifest)
        self.assertEqual(result["functions"][0]["original_rel_ranges"][0]["sha256"], hashlib.sha256(rel.read_bytes()[0x40:0x48]).hexdigest())
        self.assertTrue(result["input"]["wrapper_manifest"]["original_plus_synthetic_import_bytes_verified"])
        b = bytearray(self.binary)
        b[-len(rel.read_bytes()) + 0x70] ^= 1  # outside reported function and metadata
        self.binary = bytes(b)
        self.save()
        record["wrapper_sha256"] = hashlib.sha256(self.binary).hexdigest(); manifest.write_text(json.dumps(record))
        with self.assertRaisesRegex(fc.EvidenceError, "wrapper bytes disagree"):
            fc.build_context(self.report, self.elf, [0x40], self.root, [rel], search=False, wrapper_manifest=manifest)
        record["wrapper_sha256"] = "0" * 64; manifest.write_text(json.dumps(record))
        with self.assertRaisesRegex(fc.EvidenceError, "wrapper manifest hash"): fc.build_context(self.report, self.elf, [0x40], self.root, [rel], search=False, wrapper_manifest=manifest)

    def test_private_output_no_clobber_and_failure_no_output(self):
        output = self.root / "analysis/result-private.json"
        argv = ["--report", str(self.report), "--elf", str(self.elf), "--project", str(self.root), "--address", "00100000", "--output", str(output), "--no-search"]
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(fc.main(argv), 0)
            before = output.read_bytes()
            self.assertEqual(fc.main(argv), 2)
            self.assertEqual(output.read_bytes(), before)
            self.document["elfSha256"] = "0" * 64; self.report.write_text(json.dumps(self.document))
            self.assertEqual(fc.main(argv + ["--force"]), 2)
            self.assertEqual(output.read_bytes(), before)
            outside = list(argv); outside[outside.index(str(output))] = str(self.root / "public.json")
            self.assertEqual(fc.main(outside), 2)
            self.assertFalse((self.root / "public.json").exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
