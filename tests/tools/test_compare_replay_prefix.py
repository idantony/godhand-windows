import importlib.util
from pathlib import Path
import unittest


SCRIPT = Path(__file__).resolve().parents[2] / "skill/ps2-source-port/scripts/compare-replay-prefix.py"
SPEC = importlib.util.spec_from_file_location("compare_replay_prefix", SCRIPT)
PREFIX = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PREFIX)

START = 'START {"tick": 5, "room": 0}\n'
PLAY = 'PLAY {"tick": 6, "hp": 10}\n'
AFTER = 'PLAY {"tick": 7, "hp": 8}\n'


class ReplayPrefix(unittest.TestCase):
    def compare(self, baseline, replay):
        return PREFIX.compare(baseline, replay, {"START", "PLAY"}, before_tick=7)

    def test_same_tick_window_ignores_unselected_and_later_records(self):
        result = self.compare([START, PLAY], ["other log\n", START, PLAY, AFTER])
        self.assertTrue(result["full_baseline_match"])
        self.assertEqual(result["tag_counts"], {"START": 1, "PLAY": 1})

    def test_missing_record_fails_coverage(self):
        result = self.compare([START, PLAY], [START, AFTER])
        self.assertFalse(result["full_baseline_match"])
        self.assertEqual(result["records_matched"], 1)
        self.assertEqual(result["first_difference"], {
            "expected": ("PLAY", {"tick": 6, "hp": 10}), "actual": None,
        })

    def test_live_stream_stops_consuming_at_first_difference(self):
        def replay():
            yield START
            yield PLAY.replace("10", "9")
            self.fail("Consumed replay after divergence")

        result = self.compare([START, PLAY], replay())
        self.assertFalse(result["full_baseline_match"])
        self.assertEqual(result["first_difference"]["actual"], ("PLAY", {"tick": 6, "hp": 9}))

    def test_missing_baseline_tag_fails_closed(self):
        with self.assertRaisesRegex(ValueError, "No baseline records in range for PLAY"):
            self.compare([START, AFTER], [START, PLAY])


    def test_raw_mode_excludes_boundary_and_does_not_read_replay_tail(self):
        def replay():
            yield b"native diagnostic\r\n"
            yield b"INPUT tick=6 ready=1\r\n"
            self.fail("Consumed replay at the excluded boundary")

        result = PREFIX.compare_bytes_before(
            [b"native diagnostic\r\n", b"INPUT tick=6 ready=1\r\n", b"STATE tick=7 old_stop\r\n"],
            replay(), b"STATE tick=7 ",
        )
        self.assertTrue(result["full_baseline_match"])
        self.assertEqual(result["baseline_boundary_line"], 3)
        self.assertEqual(result["lines_matched"], 2)

    def test_raw_mode_keeps_line_endings_and_rejects_a_truncated_replay(self):
        baseline = [b"native diagnostic\r\n", b"STATE tick=7 old_stop\r\n"]
        for replay in ([], [b"native diagnostic\n"]):
            result = PREFIX.compare_bytes_before(baseline, replay, b"STATE tick=7 ")
            self.assertFalse(result["full_baseline_match"])
            self.assertEqual(result["first_difference"]["line"], 1)

    def test_raw_mode_rejects_missing_or_empty_baseline_range(self):
        with self.assertRaisesRegex(ValueError, "does not contain"):
            PREFIX.compare_bytes_before([b"native diagnostic\n"], [b"native diagnostic\n"], b"STATE")
        with self.assertRaisesRegex(ValueError, "no preceding lines"):
            PREFIX.compare_bytes_before([b"STATE tick=7\n"], [], b"STATE")


if __name__ == "__main__":
    unittest.main()
