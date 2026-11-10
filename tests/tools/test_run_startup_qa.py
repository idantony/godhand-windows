"""Exercise QA cohort guards and receipts without opening any window/device."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("startup_runner", ROOT / "tools/run-startup-qa.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)


class StartupRunnerTests(unittest.TestCase):
    def setUp(self):
        temp_root = tool.proof.SCRATCH_ROOT / "tests/tools"
        temp_root.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="startup-runner-", dir=str(temp_root))
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.snapshot, self.scratch = self.root / "snapshot", self.root / "scratch"
        self.analysis = self.snapshot / "analysis"
        self.analysis.mkdir(parents=True)
        self.source = self.snapshot / "src/fixture.c"
        self.source.parent.mkdir()
        self.source.write_bytes(b"fixture")
        self.exe = self.scratch / "build/windows-startup-qa/probe-native-startup.exe"
        self.exe.parent.mkdir(parents=True)
        self.exe.write_bytes(b"not executed")
        self.iso, self.ffmpeg = self.root / "fixture.iso", self.root / "ffmpeg.exe"
        self.iso.write_bytes(b"fixture"); self.ffmpeg.write_bytes(b"fixture")
        hashes = {"src/fixture.c": tool.proof.digest(self.source)}
        self.write(self.analysis / "source-snapshot.json", {"snapshot": str(self.snapshot), "source_project": str(ROOT),
                   "source_sha256": hashes, "status": "copied-exact-bytes; not-yet-built-or-tested"})
        self.report_path = self.analysis / "windows-startup-qa-build.json"
        self.report = {"status": "built", "target": "startup-qa", "is_playable_game": False,
                       "source_sha256": hashes, "steps": [{"name": "link", "returncode": 0}],
                       "executable": str(self.exe), "executable_sha256": tool.proof.digest(self.exe)}
        self.write(self.report_path, self.report)

    def write(self, path, value):
        path.write_text(json.dumps(value), encoding="utf-8")

    def prepare(self):
        return tool.prepare(self.snapshot, self.scratch, self.iso, self.ffmpeg)

    def qa(self, **changes):
        data = dict(hidden=True, muted=True, synthetic_input=True, prefix_mode=False, exit_code=4,
                    frontier="00142060", ticks=489, world_packets_submitted=0,
                    renderer_error=0, world_gpu_error=0, window_error=0)
        data.update(changes)
        return data

    def fake_run(self, data):
        def launch(command, stdout, stderr):
            stdout.write(json.dumps(data).encode("utf-8"))
            stderr.write(b"test-only synthetic launch\n")
            return subprocess.CompletedProcess(command, data["exit_code"])
        return launch

    def test_exact_command_always_hides_mutes_and_uses_absent_profile(self):
        command = self.prepare()["command"]
        self.assertEqual(command[0], str(self.exe))
        self.assertTrue(all(flag in command for flag in ("--hidden", "--muted", "--qa-new-game")))
        self.assertEqual(command[command.index("--profile") + 1], str(self.scratch / "profile-absent"))
        self.assertEqual(command[-4:], ["--qa-capture-frame", "370", "--max-ticks", "1100"])

    def test_modified_source_rejected_before_launch(self):
        self.source.write_bytes(b"changed")
        with self.assertRaises(tool.proof.ProofError): self.prepare()
        self.assertFalse((self.scratch / "startup-run.lock").exists())

    def test_modified_executable_rejected(self):
        self.exe.write_bytes(b"changed")
        with self.assertRaises(tool.proof.ProofError): self.prepare()

    def test_foreign_build_cohort_rejected(self):
        self.report["source_sha256"] = {"src/fixture.c": "0" * 64}
        self.write(self.report_path, self.report)
        with self.assertRaises(tool.proof.ProofError): self.prepare()

    def test_foreign_executable_and_wrong_target_rejected(self):
        for field, value in (("executable", str(self.root / "elsewhere.exe")), ("target", "native")):
            with self.subTest(field=field):
                report = dict(self.report); report[field] = value; self.write(self.report_path, report)
                with self.assertRaises(tool.proof.ProofError): self.prepare()

    def test_existing_outputs_and_profile_are_never_overwritten(self):
        for name in ("startup-run.lock", "startup-newgame.json", "startup-process.json", "startup-newgame.stderr.log",
                     "first-scene-candidate.rgba", "first-scene-candidate.rgba.world.rgba", "profile-absent"):
            path = self.scratch / name; path.write_bytes(b"preserve")
            with self.subTest(name=name), self.assertRaises(tool.proof.ProofError): self.prepare()
            self.assertEqual(path.read_bytes(), b"preserve"); path.unlink()

    def test_strict_frontier_exit_keeps_process_and_exact_hashes(self):
        with patch.object(tool.subprocess, "run", self.fake_run(self.qa())):
            result = tool.run(self.prepare())
        self.assertEqual(result["frontier"], "00142060"); self.assertEqual(result["exit_code"], 4)
        receipt = json.loads((self.scratch / "startup-process.json").read_text())
        self.assertEqual(receipt["executable_sha256_before"], receipt["executable_sha256_after"])
        self.assertIn("--muted", receipt["command"])
        with self.assertRaises(tool.proof.ProofError): self.prepare()

    def test_actual_device_error_rejected_but_receipt_retained(self):
        with patch.object(tool.subprocess, "run", self.fake_run(self.qa(world_gpu_error=2))):
            with self.assertRaises(tool.proof.ProofError): tool.run(self.prepare())
        self.assertTrue((self.scratch / "startup-process.json").is_file())

    def test_launch_failure_keeps_failure_receipt(self):
        with patch.object(tool.subprocess, "run", side_effect=OSError("fixture launch failure")):
            with self.assertRaises(tool.proof.ProofError): tool.run(self.prepare())
        receipt = json.loads((self.scratch / "startup-process.json").read_text())
        self.assertIn("launch_error", receipt); self.assertNotIn("exit_code", receipt)

    def test_frame_bounds_rejected(self):
        for ticks, capture in ((0, 370), (100, 370), (1100, 0), (100001, 370)):
            with self.subTest(ticks=ticks, capture=capture), self.assertRaises(tool.proof.ProofError):
                tool.prepare(self.snapshot, self.scratch, self.iso, self.ffmpeg, ticks, capture)


if __name__ == "__main__":
    unittest.main()
