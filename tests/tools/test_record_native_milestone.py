"""Milestone proof tests; synthetic inputs stay in G:, no builds/QA execution."""
import importlib.util
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

PROJECT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("native_milestone", PROJECT / "tools/record-native-milestone.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
TEST_ROOT = Path("G:/middle_thing/godhand_windows/tool-tests")


class MilestoneTests(unittest.TestCase):
    def setUp(self):
        TEST_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="milestone-", dir=TEST_ROOT)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.g = self.root / "G"
        self.snapshot, self.scratch = self.g / "snapshots/one", self.g / "verification/one"
        self.project = self.root / "project"
        (self.project / "analysis").mkdir(parents=True)
        self.addCleanup(patch.stopall)
        patch.object(tool, "SCRATCH_ROOT", self.g).start()
        patch.object(tool, "PROJECT", self.project).start()
        source = {"src/core/a.c": b"int a;\n", "src/core/a.h": b"extern int a;\n",
                  "tools/verify_rng.py": b"# verifier\n", "tools/verify-frame-rng.py": b"# frame\n"}
        hashes = {}
        for name, data in source.items():
            path = self.snapshot / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            hashes[name] = tool.digest(path)
        elf = self.snapshot / "reference/SLUS_215.03"
        elf.parent.mkdir()
        elf.write_bytes(b"synthetic ELF hash input, never executed")
        self.analysis = self.snapshot / "analysis"
        self.dll = self.scratch / "build/native/godhand_core.dll"
        self.exe = self.scratch / "build/windows-startup-qa/probe-native-startup.exe"
        self.test = self.scratch / "build/native/test_a.exe"
        for path, data in ((self.dll, b"dll"), (self.exe, b"exe"), (self.test, b"test")):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        self.write(self.analysis / "source-snapshot.json", {"snapshot": str(self.snapshot), "source_project": str(self.project),
                  "source_sha256": hashes, "status": "copied-exact-bytes; not-yet-built-or-tested"})
        self.write(self.analysis / "native-verification.json", {"status": "passed", "is_playable_game": False,
                   "source_sha256": hashes, "module_names": ["a"],
                   "steps": [{"name": "compile-a", "returncode": 0}, {"name": "test-a", "returncode": 0}],
                   "artifacts": [{"path": str(p), "size": p.stat().st_size, "sha256": tool.digest(p)} for p in (self.dll, self.test)]})
        self.write(self.analysis / "windows-startup-qa-build.json", {"status": "built", "is_playable_game": False,
                   "source_sha256": hashes, "steps": [{"name": "link", "returncode": 0}],
                   "executable": str(self.exe), "executable_sha256": tool.digest(self.exe)})
        rng = {"status": "passed", "native_dll": str(self.dll), "native_sha256": tool.digest(self.dll), "elf_sha256": tool.digest(elf)}
        self.write(self.analysis / "rng-differential.json", rng)
        self.write(self.analysis / "frame-rng-differential.json", dict(rng, source_sha256={"src/core/a.c": hashes["src/core/a.c"]}))
        self.write(self.scratch / "startup-process.json", {"command": [str(self.exe), "--hidden", "--muted", "--qa-new-game"], "exit_code": 4})
        self.write(self.scratch / "startup-newgame.json", {"hidden": True, "muted": True, "prefix_mode": False,
                   "playable_game_verified": False, "synthetic_input": True, "frontier_module": "main ELF", "frontier": "00132DA0",
                   "exit_code": 4, "host_error": 7, "subsystem_error": 7, "renderer_error": 0, "world_gpu_error": 0,
                   "window_error": 0, "ticks": 489, "game_state": 4, "world_packets_submitted": 0})

    def write(self, path, value):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(value), encoding="utf8")

    def change(self, path, update):
        data = json.loads(path.read_text())
        update(data)
        self.write(path, data)

    def validate(self, frontier="00132DA0"):
        return tool.validate(self.snapshot, self.scratch, frontier)

    def test_matching_frontier_and_legacy_rng_scope(self):
        report, _ = self.validate()
        self.assertEqual(report["counts"]["native_modules"], 1)
        self.assertEqual(report["counts"]["test_checks"], 1)
        self.assertEqual(report["actual_qa"]["exit_code"], 4)
        self.assertFalse(report["actual_qa"]["playable_game_verified"])
        self.assertTrue(any("Legacy" in text for text in report["limitations"]))
        tool.validate(self.analysis / "source-snapshot.json", self.scratch, "main ELF:00132DA0")

    def test_failed_native_status_and_failed_step_rejected(self):
        path = self.analysis / "native-verification.json"
        self.change(path, lambda d: d.update(status="failed"))
        with self.assertRaisesRegex(tool.ProofError, "native.status"):
            self.validate()
        self.change(path, lambda d: d.update(status="passed"))
        self.change(path, lambda d: d["steps"][1].update(returncode=1))
        with self.assertRaisesRegex(tool.ProofError, "failed step"):
            self.validate()

    def test_zero_exit_accepts_only_zero_host_and_frontier(self):
        self.change(self.scratch / "startup-process.json", lambda d: d.update(exit_code=0))
        self.change(self.scratch / "startup-newgame.json", lambda d: d.update(exit_code=0, host_error=0, subsystem_error=0, frontier="00000000"))
        report, _ = self.validate(None)
        self.assertEqual(report["actual_qa"]["exit_code"], 0)
        self.assertIsNone(report["expected_frontier"])
        with self.assertRaisesRegex(tool.ProofError, "requires actual exit4"):
            self.validate()

    def test_failed_windows_and_unsafe_source_receipt_rejected(self):
        path = self.analysis / "windows-startup-qa-build.json"
        self.change(path, lambda d: d.update(status="failed"))
        with self.assertRaisesRegex(tool.ProofError, "windows.status"):
            self.validate()
        self.change(path, lambda d: d.update(status="built"))
        self.change(self.analysis / "source-snapshot.json", lambda d: d["source_sha256"].update({"../outside.c": "f"*64}))
        with self.assertRaisesRegex(tool.ProofError, "unsafe source path"):
            self.validate()

    def test_mixed_native_or_windows_source_map_rejected(self):
        for name in ("native-verification.json", "windows-startup-qa-build.json", "frame-rng-differential.json"):
            path = self.analysis / name
            original = path.read_bytes()
            self.change(path, lambda d: d["source_sha256"].update({"src/core/a.c": "f"*64}))
            with self.assertRaisesRegex(tool.ProofError, "mixed source cohort"):
                self.validate()
            path.write_bytes(original)

    def test_stale_actual_source_rejected(self):
        (self.snapshot / "src/core/a.h").write_text("changed", encoding="utf8")
        with self.assertRaisesRegex(tool.ProofError, "SHA256 mismatch"):
            self.validate()

    def test_stale_dll_and_rng_hash_rejected(self):
        self.dll.write_bytes(b"changed")
        with self.assertRaisesRegex(tool.ProofError, "SHA256 mismatch"):
            self.validate()
        self.dll.write_bytes(b"dll")
        self.change(self.analysis / "rng-differential.json", lambda d: d.update(native_sha256="f"*64))
        with self.assertRaisesRegex(tool.ProofError, "DLL SHA256"):
            self.validate()

    def test_mixed_actual_command_or_executable_rejected(self):
        self.change(self.scratch / "startup-process.json", lambda d: d["command"].__setitem__(0, str(self.test)))
        with self.assertRaisesRegex(tool.ProofError, "executable does not match"):
            self.validate()

    def test_exit_and_hidden_muted_render_errors_rejected(self):
        for update, expected in (({"exit_code": 0}, "exit mismatch"), ({"muted": False}, "hidden and muted"),
                                 ({"renderer_error": 1}, "renderer_error"), ({"world_gpu_error": 1}, "world_gpu_error"),
                                 ({"window_error": 1}, "window_error")):
            path = self.scratch / "startup-newgame.json"
            original = path.read_bytes()
            self.change(path, lambda d: d.update(update))
            with self.assertRaisesRegex(tool.ProofError, expected):
                self.validate()
            path.write_bytes(original)

    def test_frontier_requires_explicit_module_aware_expectation(self):
        with self.assertRaisesRegex(tool.ProofError, "requires explicit"):
            self.validate(None)
        with self.assertRaisesRegex(tool.ProofError, "does not match"):
            self.validate("0014FB48")
        self.change(self.scratch / "startup-newgame.json", lambda d: d.update(frontier_module="r020.rel"))
        with self.assertRaisesRegex(tool.ProofError, "does not match"):
            self.validate()
        self.validate("r020.rel:00132DA0")

    def test_missing_frame_rng_source_map_and_escaped_artifact(self):
        path = self.analysis / "frame-rng-differential.json"
        original = path.read_bytes()
        self.change(path, lambda d: d.pop("source_sha256"))
        with self.assertRaisesRegex(tool.ProofError, "missing source_sha256"):
            self.validate()
        path.write_bytes(original)
        self.change(self.analysis / "windows-startup-qa-build.json", lambda d: d.update(executable=str(self.root / "outside.exe")))
        with self.assertRaisesRegex(tool.ProofError, "outside matching"):
            self.validate()

    def test_mutation_after_validation_detected(self):
        _, inputs = self.validate()
        self.exe.write_bytes(b"later replacement")
        with self.assertRaisesRegex(tool.ProofError, "input changed"):
            inputs.recheck()

    def test_output_requires_fresh_ignored_private_analysis(self):
        path = self.project / "analysis/proof-private.json"
        with patch.object(tool.subprocess, "run", return_value=SimpleNamespace(returncode=0)):
            self.assertEqual(tool.output_path(path), path)
            path.write_text("old", encoding="utf8")
            with self.assertRaisesRegex(tool.ProofError, "must not already exist"):
                tool.output_path(path)
            with self.assertRaisesRegex(tool.ProofError, "analysis/\\*-"):
                tool.output_path(self.project / "src/proof-private.json")
            with self.assertRaises(tool.ProofError):
                tool.output_path(self.project / "analysis/public.json")
        with patch.object(tool.subprocess, "run", return_value=SimpleNamespace(returncode=1)):
            with self.assertRaisesRegex(tool.ProofError, "not ignored"):
                tool.output_path(self.project / "analysis/other-private.json")


if __name__ == "__main__":
    unittest.main()
