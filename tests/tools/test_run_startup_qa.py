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
frontier_spec = importlib.util.spec_from_file_location("startup_frontier", ROOT / "tools/record-startup-frontier.py")
frontier = importlib.util.module_from_spec(frontier_spec)
frontier_spec.loader.exec_module(frontier)



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

    def fake_run(self, data, trace=b"test-only synthetic launch\n"):
        def launch(command, stdout, stderr):
            stdout.write(json.dumps(data).encode("utf-8"))
            stderr.write(trace)
            return subprocess.CompletedProcess(command, data["exit_code"])
        return launch

    def test_frontier_record_uses_runner_output_fingerprints(self):
        prepared = tool.prepare(self.snapshot, self.scratch, self.iso, self.ffmpeg, control_probe="run")
        qa = self.qa(playable_game_verified=False, frontier_module="main ELF",
                     last_move_handle=248, player_updates_completed=2)
        placement = {"handle": 248, "stamp": 8, "appearance": 5}
        task = {"slot": 3, "stage": 3}
        render = {"handle": 254, "frontier": "00142060", "draw_phase": 10}
        trace = ("QA_ENEMY_PLACEMENT " + json.dumps(placement) + "\n"
                 "QA_SCENARIO_TASK " + json.dumps(task) + "\n"
                 "GH_ACTOR_FRONTIER " + json.dumps(render) + "\n"
                 "QA_CONTROL mode=run tick=488 updates=3 handle=157 axes=0,127\n"
                 "QA_CONTROL_PRESENTED mode=run handle=157 scene=370\n"
                 "QA_CONTROL_STATUS mode=run saw_event=1 pressed=1 motion_presented=1\n").encode("utf-8")
        with patch.object(tool.subprocess, "run", self.fake_run(qa, trace)):
            tool.run(prepared)
        runner = self.snapshot / "tools/run-startup-qa.py"
        runner.parent.mkdir()
        runner.write_bytes((ROOT / "tools/run-startup-qa.py").read_bytes())
        result, _ = frontier.validate(self.snapshot, self.scratch, expected_frontier="00142060")
        self.assertEqual(result["classification"], "original-frontier")
        self.assertFalse(result["verification_success"])
        self.assertFalse(result["playable_game_verified"])
        self.assertEqual(result["observation"]["enemy_placement"], placement)
        self.assertEqual(result["observation"]["scenario_task"], task)
        self.assertEqual(result["observation"]["actor_render"], render)
        self.assertEqual(result["observation"]["control_probe"]["presented_motion"],
                         {"handle": 157, "scene_serial": 370})
        self.assertEqual(result["observation"]["player_updates_completed"], 2)
        (self.scratch / "startup-newgame.stderr.log").write_bytes(b"post-launch modification")
        with self.assertRaisesRegex(frontier.proof.ProofError, "SHA256 mismatch"):
            frontier.validate(self.snapshot, self.scratch, expected_frontier="00142060")

    def test_square_presentation_requires_an_advancing_attack_sample(self):
        command = ["probe-native-startup.exe", "--qa-control", "square"]
        status = "QA_CONTROL_STATUS mode=square saw_event=1 pressed=1 motion_presented=1"
        presented = "QA_CONTROL_PRESENTED mode=square handle=157 scene=370 entry=0010D3A8"
        result = frontier.control_observation(command, [status, presented + " sample_before=0.5 sample=1"])
        self.assertEqual(result["presented_motion"], {"handle": 157, "scene_serial": 370,
                         "entry": "0010D3A8", "sample_before": 0.5, "sample": 1})
        with self.assertRaisesRegex(frontier.proof.ProofError, "sample did not advance"):
            frontier.control_observation(command, [status, presented + " sample_before=1 sample=1"])
        with self.assertRaisesRegex(frontier.proof.ProofError, "presentation differ"):
            frontier.control_observation(command, [status])

    def test_frontier_capture_range_hashes_both_layers_and_ignores_alpha(self):
        prepared = tool.prepare(self.snapshot, self.scratch, self.iso, self.ffmpeg, 500, 370, 3)
        qa = self.qa(playable_game_verified=False, frontier_module="main ELF")
        size = 512 * 448 * 4
        capture_lines = "".join("QA_WORLD_CAPTURE " + json.dumps({"scene_serial": scene,
            "world_bytes": size, "fade_visible": 1, "fade_rgba": fade, "presentation_tracking": True}) + "\n"
            for scene, fade in ((370, "FF000000"), (371, "77000000")))
        trace = (capture_lines + "QA_CAPTURE_PRESENTED 370\n").encode("utf-8")
        capture = self.scratch / "first-scene-candidate.rgba"
        later = self.scratch / "first-scene-candidate.scene-371.rgba"
        alpha_only = b"\x00\x00\x00\xff"
        visible = b"\x02\x00\x00\xff"

        def launch(command, stdout, stderr):
            for path, pixel in ((capture, alpha_only), (later, visible)):
                path.write_bytes(pixel + bytes(size - 4))
                path.with_name(path.name + ".world.rgba").write_bytes(visible + bytes(size - 4))
            return self.fake_run(qa, trace)(command, stdout, stderr)

        with patch.object(tool.subprocess, "run", launch):
            tool.run(prepared)
        runner = self.snapshot / "tools/run-startup-qa.py"
        runner.parent.mkdir()
        runner.write_bytes((ROOT / "tools/run-startup-qa.py").read_bytes())
        options = {"expected_frontier": "00142060", "capture_series": True}
        result, _ = frontier.validate(self.snapshot, self.scratch, **options)
        frames = result["captures"]
        self.assertEqual([frame["scene_serial"] for frame in frames], [370, 371])
        self.assertEqual([frame["presented"] for frame in frames], [True, False])
        self.assertEqual([frame["composite"]["nonblack_rgb_pixels"] for frame in frames], [0, 1])
        self.assertEqual([frame["world"]["nonblack_rgb_pixels"] for frame in frames], [1, 1])
        later.with_name(later.name + ".world.rgba").write_bytes(bytes(size))
        with self.assertRaisesRegex(frontier.proof.ProofError, "SHA256 mismatch"):
            frontier.validate(self.snapshot, self.scratch, **options)

    def test_renderer_error_has_distinct_classification_and_no_success(self):
        prepared = self.prepare()
        qa = self.qa(playable_game_verified=False, frontier_module="main ELF", world_gpu_error=2)
        with patch.object(tool.subprocess, "run", self.fake_run(qa)), self.assertRaises(tool.proof.ProofError):
            tool.run(prepared)
        runner = self.snapshot / "tools/run-startup-qa.py"
        runner.parent.mkdir()
        runner.write_bytes((ROOT / "tools/run-startup-qa.py").read_bytes())
        result, _ = frontier.validate(self.snapshot, self.scratch, expected_native_error="world_gpu_error:2")
        self.assertEqual(result["classification"], "native-renderer-error")
        self.assertFalse(result["verification_success"])
        with self.assertRaisesRegex(frontier.proof.ProofError, "no native renderer errors"):
            frontier.validate(self.snapshot, self.scratch, expected_frontier="00142060")
        with self.assertRaisesRegex(frontier.proof.ProofError, "unexpected native error"):
            frontier.validate(self.snapshot, self.scratch, expected_native_error="world_gpu_error:3")

    def test_exact_command_always_hides_mutes_and_uses_absent_profile(self):
        command = self.prepare()["command"]
        self.assertEqual(command[0], str(self.exe))
        self.assertTrue(all(flag in command for flag in ("--hidden", "--muted", "--qa-new-game")))
        self.assertEqual(command[command.index("--profile") + 1], str(self.scratch / "profile-absent"))
        self.assertEqual(command[-4:], ["--qa-capture-frame", "370", "--max-ticks", "1100"])

    def test_capture_range_preserves_first_path_and_process_command(self):
        prepared = tool.prepare(self.snapshot, self.scratch, self.iso, self.ffmpeg, 500, 370, 3)
        command = prepared["command"]
        self.assertEqual(command[-2:], ["--qa-capture-count", "3"])
        self.assertEqual(command[command.index("--qa-capture") + 1],
                         str(self.scratch / "first-scene-candidate.rgba"))
        with patch.object(tool.subprocess, "run", self.fake_run(self.qa())):
            tool.run(prepared)
        receipt = json.loads((self.scratch / "startup-process.json").read_text())
        self.assertEqual(receipt["command"], command)

    def test_capture_range_rejects_existing_scene_or_world_frame(self):
        for name in ("first-scene-candidate.scene-371.rgba",
                     "first-scene-candidate.scene-371.rgba.world.rgba"):
            path = self.scratch / name
            path.write_bytes(b"preserve")
            with self.subTest(name=name), self.assertRaises(tool.proof.ProofError):
                tool.prepare(self.snapshot, self.scratch, self.iso, self.ffmpeg, 500, 370, 3)
            self.assertEqual(path.read_bytes(), b"preserve")
            path.unlink()

    def test_capture_range_bounds_rejected(self):
        for capture, count in ((370, 0), (370, tool.MAX_CAPTURE_COUNT + 1), (500, 2)):
            with self.subTest(capture=capture, count=count), self.assertRaises(tool.proof.ProofError):
                tool.prepare(self.snapshot, self.scratch, self.iso, self.ffmpeg, 500, capture, count)
        self.assertFalse((self.scratch / "startup-run.lock").exists())

    def test_control_probe_requires_a_named_action_and_preserves_qa_flags(self):
        for action in ("walk", "run", "square", "approach-square"):
            with self.subTest(action=action):
                command = tool.prepare(self.snapshot, self.scratch, self.iso, self.ffmpeg,
                                       control_probe=action)["command"]
                self.assertEqual(command[-2:], ["--qa-control", action])
                self.assertTrue(all(flag in command for flag in ("--hidden", "--muted", "--qa-new-game")))
        with self.assertRaises(tool.proof.ProofError):
            tool.prepare(self.snapshot, self.scratch, self.iso, self.ffmpeg, control_probe="other")
        self.assertNotIn("--qa-control", self.prepare()["command"])

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
