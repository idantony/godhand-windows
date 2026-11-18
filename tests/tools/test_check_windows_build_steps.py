import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


PROJECT = Path(__file__).resolve().parents[2]
TOOL = PROJECT / "skill/ps2-source-port/scripts/check-windows-build-steps.py"
MACRO = "-DGH_NATIVE_STARTUP_QA=1"


class WindowsBuildStepsTest(unittest.TestCase):
    def run_check(self, report, *requirements):
        scratch = PROJECT / ".work/tool-tests"
        scratch.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as directory:
            path = Path(directory) / "build.json"
            path.write_text(json.dumps(report), encoding="utf-8")
            args = [item for value in requirements for item in ("--require", value)]
            return subprocess.run([sys.executable, "-B", str(TOOL), str(path), *args],
                                  capture_output=True, text=True)

    def test_accepts_repeated_exact_requirements_on_cached_and_fresh_steps(self):
        report = {"status": "built", "steps": [
            {"name": "compile-video_player", "command": ["zig", MACRO],
             "returncode": 0, "object_cache": "digest"},
            {"name": "compile-main", "command": ["zig", MACRO], "returncode": 0},
        ]}
        result = self.run_check(report, f"compile-video_player={MACRO}", f"compile-main={MACRO}")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Verified 2", result.stdout)

    def test_rejects_invalid_or_unproven_step(self):
        good = {"name": "compile-video_player", "command": ["zig", MACRO], "returncode": 0}
        for status, steps, diagnostic in (
            ("failed", [good], "expected a built"),
            ("built", {}, "malformed build step list"),
            ("built", [None], "malformed build step list"),
            ("built", [], "missing build step"),
            ("built", [good, good], "ambiguous build step"),
            ("built", [{**good, "returncode": False}], "did not succeed"),
            ("built", [{**good, "returncode": 1}], "did not succeed"),
            ("built", [{**good, "command": None}], "no valid command"),
            ("built", [{**good, "command": ["zig", 1]}], "no valid command"),
            ("built", [{**good, "command": ["zig", MACRO + "0"]}], "lacks exact argument"),
        ):
            with self.subTest(status=status, steps=steps):
                result = self.run_check({"status": status, "steps": steps}, f"compile-video_player={MACRO}")
                self.assertEqual(result.returncode, 1)
                self.assertIn(diagnostic, result.stderr)

    def test_requires_step_and_argument(self):
        for requirement in ("compile-video_player", "compile-video_player=", f"={MACRO}"):
            with self.subTest(requirement=requirement):
                result = self.run_check({}, requirement)
                self.assertEqual(result.returncode, 2)
                self.assertIn("expected STEP=ARG", result.stderr)


if __name__ == "__main__":
    unittest.main()
