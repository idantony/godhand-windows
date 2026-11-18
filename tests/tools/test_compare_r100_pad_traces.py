"""Check trace comparison and the baseline gate used before release."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools/compare-r100-pad-traces.py"


def sample(move, *, scene_shift=0, hp=20):
    return {
        "move": move, "tick": move + 300, "scene": move + scene_shift,
        "pad": [0, 0], "hp": [hp], "state": [0], "position_bits": [0, 0, 0],
        "yaw_bits": 0, "scenario": 0, "dead": 0, "target": 0, "skill": 0,
    }


class BaselineTests(unittest.TestCase):
    def compare(self, left, right, *options):
        scratch = ROOT / ".work/tests"
        scratch.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as directory:
            paths = [Path(directory) / name for name in ("left.log", "right.log")]
            for path, rows in zip(paths, (left, right)):
                path.write_text("".join(f"QA_R100_PAD_STATE {json.dumps(row)}\n" for row in rows),
                                encoding="utf-8")
            run = subprocess.run([sys.executable, "-B", str(TOOL), *map(str, paths), *options],
                                 capture_output=True, text=True, check=False)
        self.assertTrue(run.stdout, run.stderr)
        return run.returncode, json.loads(run.stdout)

    def test_constant_scene_offset_and_extended_candidate_pass(self):
        left = [sample(1), sample(2)]
        right = [sample(move, scene_shift=5) for move in (1, 2, 3)]
        status, report = self.compare(left, right, "--require-baseline")
        self.assertEqual(status, 0)
        self.assertTrue(report["right_covers_left_prefix"])
        self.assertEqual(report["timeline_offsets"]["scene"], {"initial": 5, "first_change": None})

    def test_incomplete_candidate_is_only_valid_for_diagnostic_comparison(self):
        left = [sample(1), sample(2), sample(3)]
        for flag, expected in (((), 0), (("--require-baseline",), 2)):
            with self.subTest(options=flag):
                status, report = self.compare(left, [sample(1), sample(3)], *flag)
                self.assertEqual(status, expected)
                self.assertFalse(report["right_covers_left_prefix"])
                self.assertEqual(report["first_missing_left_update"], 2)
                self.assertEqual(report["first_difference_by_field"], {})

    def test_state_change_and_timeline_change_report_separately(self):
        status, report = self.compare([sample(1), sample(2)],
                                      [sample(1, scene_shift=5), sample(2, scene_shift=8, hp=19)],
                                      "--require-baseline")
        self.assertEqual(status, 2)
        self.assertTrue(report["right_covers_left_prefix"])
        self.assertEqual(report["first_difference_by_field"]["hp"]["right"]["value"], [19])
        self.assertEqual(report["timeline_offsets"]["scene"]["first_change"], {"move": 2, "offset": 8})


if __name__ == "__main__":
    unittest.main()
