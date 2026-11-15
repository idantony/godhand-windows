"""Focused receipt and three-way merge checks using temporary UTF-8 fixtures."""

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

PROJECT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("rebase_overlay", PROJECT / "tools/rebase-source-overlay.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
RebaseError, digest, rebase = tool.RebaseError, tool.digest, tool.rebase


class RebaseOverlayTest(unittest.TestCase):
    path = "src/game.c"
    other = "src/other.h"
    base_content = b"top\nvalue = 1;\nseparator\nlast = 1;\n"
    overlay_content = b"top\nvalue = 2;\nseparator\nlast = 1;\n"

    def setUp(self):
        scratch_root = tool.SCRATCH_ROOT / "tests/tools"
        scratch_root.mkdir(parents=True, exist_ok=True)
        self.scratch = tempfile.TemporaryDirectory(prefix="rebase-overlay-", dir=scratch_root)
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name).resolve()
        self.base = self.snapshot("base", {self.path: self.base_content})
        self.output = self.root / "result"

    def write_json(self, file, data):
        file.parent.mkdir(parents=True, exist_ok=True)
        data = json.dumps(data).encode("utf-8")
        file.write_bytes(data)
        return file, digest(data)

    def write_file(self, root, name, data):
        file = root / name
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_bytes(data)

    def snapshot(self, label, files):
        root = self.root / label
        for name, data in files.items():
            self.write_file(root, name, data)
        return self.write_json(root / "analysis/source-snapshot.json", {
            "snapshot": str(root),
            "source_project": str(self.root / "project"),
            "source_sha256": {name: digest(data) for name, data in files.items()},
        })

    def overlay(self, **extra):
        root = self.root / "overlay"
        self.write_file(root, self.path, self.overlay_content)
        return self.write_json(root / "receipt.json", {
            "schema": 1,
            "base_receipt": str(self.base[0]),
            "base_receipt_sha256": self.base[1],
            "files": [{"path": self.path, "sha256": digest(self.overlay_content)}],
            "supersedes": {self.path: digest(self.base_content)},
            **extra,
        })

    def test_disjoint_edit_and_unrelated_newline_difference(self):
        target_content = b"top\r\nvalue = 1;\r\nseparator\nlast = 3;\r\n"
        target = self.snapshot("target", {self.path: target_content})
        result = rebase(*self.overlay(), *target, self.output)
        expected = b"top\r\nvalue = 2;\r\nseparator\r\nlast = 3;\r\n"
        self.assertEqual((self.output / self.path).read_bytes(), expected)
        self.assertEqual(result["supersedes"], {self.path: digest(target_content)})
        self.assertEqual(result["files"], [{"path": self.path, "sha256": digest(expected)}])
        self.assertEqual((target[0].parent.parent / self.path).read_bytes(), target_content)

    def test_same_line_conflict_leaves_no_output(self):
        target = self.snapshot("target", {self.path: self.base_content.replace(b"= 1", b"= 3", 1)})
        with self.assertRaisesRegex(RebaseError, "Overlapping edits"):
            rebase(*self.overlay(), *target, self.output)
        self.assertFalse(self.output.exists())

    def test_pinned_tokens_merge_same_line_only_with_unique_predecessor(self):
        overlay = self.overlay()
        recipe = overlay[0].parent / "evidence/integration-tokens.json"
        tokens_path, tokens_sha256 = self.write_json(recipe, {self.path: [["= 1;\nseparator", "= 2;\nseparator"]]})
        tokens = {"tokens_path": tokens_path, "tokens_sha256": tokens_sha256}
        target_content = self.base_content.replace(b"value =", b"renamed_value =")
        target = self.snapshot("target", {self.path: target_content})
        result = rebase(*overlay, *target, self.output, **tokens)
        self.assertEqual(result["rebase_results"][self.path]["status"], "merged-tokens")
        self.assertEqual((self.output / self.path).read_bytes(), target_content.replace(b"= 1;\nseparator", b"= 2;\nseparator"))

        ambiguous = self.snapshot("ambiguous", {self.path: target_content * 2})
        with self.assertRaisesRegex(RebaseError, "ambiguous in target"):
            rebase(*overlay, *ambiguous, self.root / "ambiguous-result", **tokens)
        self.assertFalse((self.root / "ambiguous-result").exists())

        _, wrong_pin = self.write_json(recipe, {self.path: [["= 1;\nseparator", "= 9;\nseparator"]]})
        with self.assertRaisesRegex(RebaseError, "do not reproduce"):
            rebase(*overlay, *target, self.root / "wrong-result", tokens_path=recipe, tokens_sha256=wrong_pin)

    def test_absolute_worker_receipt_keeps_relative_and_base_fingerprints(self):
        root = self.root / "absolute-overlay"
        self.write_file(root, self.path, self.overlay_content)
        row = {"path": str(root / self.path), "relative": self.path, "sha256": digest(self.overlay_content),
               "supersedes": {"path": str(self.base[0].parent.parent / self.path), "sha256": digest(self.base_content)}}
        record = {"schema": 1, "base": {"path": str(self.base[0]), "sha256": self.base[1]}, "files": [row],
                  "proof": {"scope": "worker evidence"}}
        target = self.snapshot("target", {self.path: self.base_content.replace(b"last = 1", b"last = 3")})
        result = rebase(*self.write_json(root / "receipt.json", record), *target, self.output)
        self.assertEqual(result["rebase_results"][self.path]["status"], "merged")
        self.assertEqual((self.output / self.path).read_bytes(), self.overlay_content.replace(b"last = 1", b"last = 3"))
        row["supersedes"]["path"] = str(target[0].parent.parent / self.path)
        with self.assertRaisesRegex(RebaseError, "Invalid worker source predecessor"):
            rebase(*self.write_json(root / "receipt.json", record), *target, self.root / "rejected")
        self.assertFalse((self.root / "rejected").exists())

    def test_absolute_worker_without_predecessor_can_only_add_new_files(self):
        root = self.root / "absolute-overlay"
        record = {"schema": 1, "base": {"path": str(self.base[0]), "sha256": self.base[1]}, "files": []}
        for name in (self.other, self.path):
            self.write_file(root, name, self.overlay_content)
            row = {"path": str(root / name), "relative": name, "sha256": digest(self.overlay_content)}
            record["files"] = [row]
            receipt = self.write_json(root / "receipt.json", record)
            if name == self.other:
                result = rebase(*receipt, *self.base, self.output)
                self.assertEqual(result["rebase_results"][name]["status"], "added")
            else:
                with self.assertRaisesRegex(RebaseError, "predecessor does not match"):
                    rebase(*receipt, *self.base, self.root / "rejected")

    def test_relative_worker_can_use_an_explicit_base_pin(self):
        root = self.root / "relative-overlay"
        self.write_file(root, self.path, self.overlay_content)
        overlay = self.write_json(root / "receipt.json", {
            "schema": 1,
            "files": [{"relative": self.path, "sha256": digest(self.overlay_content),
                       "supersedes": {"sha256": digest(self.base_content)}}],
        })
        target = self.snapshot("target", {self.path: self.base_content.replace(b"last = 1", b"last = 3")})
        result = rebase(*overlay, *target, self.output, source_base=self.base)
        self.assertEqual(result["rebase_results"][self.path]["status"], "merged")
        self.assertEqual((self.output / self.path).read_bytes(), self.overlay_content.replace(b"last = 1", b"last = 3"))
        self.assertIn({"path": str(self.base[0]), "sha256": self.base[1]}, result["dependencies"])
        with self.assertRaisesRegex(RebaseError, "SHA-256 mismatch"):
            rebase(*overlay, *target, self.root / "rejected", source_base=(self.base[0], "0" * 64))

    def test_tampered_snapshot_bytes_fail_even_when_only_newline_changed(self):
        target = self.snapshot("target", {self.path: self.base_content})
        self.write_file(target[0].parent.parent, self.path, self.base_content.replace(b"\n", b"\r\n"))
        with self.assertRaisesRegex(RebaseError, "SHA-256 mismatch"):
            rebase(*self.overlay(), *target, self.output)
        self.assertFalse(self.output.exists())

    def test_wrong_predecessor_pin_fails(self):
        target = self.snapshot("target", {self.path: self.base_content})
        with self.assertRaisesRegex(RebaseError, "predecessor does not match"):
            rebase(*self.overlay(supersedes={self.path: "a" * 64}), *target, self.output)
        self.assertFalse(self.output.exists())

    def test_pinned_evidence_survives_rebase_and_rejects_tampering(self):
        evidence = self.root / "evidence.json"
        evidence.write_bytes(b"fixture evidence")
        dependencies = [{"path": str(evidence), "sha256": digest(evidence.read_bytes())}]
        overlay = self.overlay(dependencies=dependencies, merge={"scope": "worker review"}, bounds="focused fixture")
        target = self.snapshot("target", {self.path: self.base_content})
        result = rebase(*overlay, *target, self.output)
        self.assertEqual(result["dependencies"], dependencies)
        evidence.write_bytes(b"modified evidence")
        output = self.root / "rejected"
        with self.assertRaisesRegex(RebaseError, "SHA-256 mismatch"):
            rebase(*overlay, *target, output)
        self.assertFalse(output.exists())

    def test_target_dependency_newline_change_fails(self):
        target = self.snapshot("target", {self.path: self.base_content, self.other: b"required\r\n"})
        overlay = self.overlay(dependency_files={self.other: digest(b"required\n")})
        with self.assertRaisesRegex(RebaseError, "dependency differs"):
            rebase(*overlay, *target, self.output)
        self.assertFalse(self.output.exists())

    def test_pinned_patch_record_survives_rebase(self):
        patch = self.root / "worker.patch"
        patch.write_bytes(b"original worker diff")
        patches = [{"path": str(patch), "sha256": digest(patch.read_bytes())}]
        overlay = self.overlay(patches=patches)
        result = rebase(*overlay, *self.base, self.output)
        self.assertEqual(result["patches"], patches)
        patch.write_bytes(b"modified worker diff")
        with self.assertRaisesRegex(RebaseError, "SHA-256 mismatch"):
            rebase(*overlay, *self.base, self.root / "rejected")

    def test_required_overlay_still_requires_raw_pinned_postimage(self):
        required_root = self.root / "required-overlay"
        required_bytes = b"typed callback ABI\n"
        self.write_file(required_root, self.other, required_bytes)
        required = self.write_json(required_root / "receipt.json", {
            "schema": 1,
            "base_receipt": str(self.base[0]),
            "base_receipt_sha256": self.base[1],
            "files": [{"path": self.other, "sha256": digest(required_bytes)}],
            "supersedes": {},
        })
        target = self.snapshot("target", {
            self.path: self.base_content,
            self.other: required_bytes.replace(b"\n", b"\r\n"),
        })
        overlay = self.overlay(requires_overlay_receipt=str(required[0]), requires_overlay_receipt_sha256=required[1])
        with self.assertRaisesRegex(RebaseError, "missing exact required overlay file"):
            rebase(*overlay, *target, self.output)
        self.assertFalse(self.output.exists())

    def test_already_applied_preserves_raw_target_and_writes_only_receipt(self):
        target_content = self.overlay_content.replace(b"\n", b"\r\n")
        target = self.snapshot("target", {self.path: target_content})
        result = rebase(*self.overlay(), *target, self.output)
        self.assertEqual(result["files"], [])
        self.assertEqual(result["rebase_results"][self.path], {
            "status": "already-applied", "sha256": digest(target_content),
        })
        self.assertFalse((self.output / self.path).exists())


if __name__ == "__main__":
    unittest.main()
