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
TITLE = "QA Title tick152 move0 mode1\n"
LOAD = "QA Title Load tick=510 slots00000001 done1\n"


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

    def test_plain_text_keeps_exact_order_and_chooses_longer_prefix(self):
        prefixes = ["QA Title ", "QA Title Load "]
        later = "QA Title Load tick511 another-row\n"
        result = PREFIX.compare_text([TITLE, LOAD, later], [TITLE, PLAY, LOAD, later], prefixes, before_tick=511)
        self.assertTrue(result["full_baseline_match"])
        self.assertEqual(result["tag_counts"], {"QA Title": 1, "QA Title Load": 1})

        changed = LOAD.replace(" done1", "  done1")
        mismatch = PREFIX.compare_text([TITLE, LOAD], [TITLE, changed], prefixes, before_tick=511)
        self.assertFalse(mismatch["full_baseline_match"])
        self.assertEqual(mismatch["first_difference"], {
            "expected": ("QA Title Load", LOAD.rstrip("\n")),
            "actual": ("QA Title Load", changed.rstrip("\n")),
        })
        reordered = PREFIX.compare_text([TITLE, LOAD], [LOAD, TITLE], prefixes, before_tick=511)
        self.assertEqual(reordered["records_matched"], 0)

    def test_plain_text_missing_record_preserves_coverage_failure(self):
        result = PREFIX.compare_text([TITLE, LOAD], [TITLE], ["QA Title", "QA Title Load"], before_tick=511)
        self.assertEqual(result["records_matched"], 1)
        self.assertIsNone(result["first_difference"]["actual"])
        self.assertFalse(result["full_baseline_match"])

    def test_plain_text_rejects_empty_or_missing_selection(self):
        for prefixes in ([], [" "], ["QA Title\n"]):
            with self.subTest(prefixes=prefixes), self.assertRaisesRegex(ValueError, "nonempty text prefixes"):
                PREFIX.compare_text([TITLE], [TITLE], prefixes, before_tick=511)
        with self.assertRaisesRegex(ValueError, "No baseline records in range for QA Title Load"):
            PREFIX.compare_text([TITLE, LOAD], [TITLE, LOAD], ["QA Title", "QA Title Load"], before_tick=510)

    def test_plain_text_rejects_malformed_selected_tick(self):
        with self.assertRaisesRegex(ValueError, "Line 2, QA Title Load: expected tickN or tick=N"):
            PREFIX.compare_text(
                [TITLE, LOAD], [TITLE, "QA Title Load tick510x done1\n"],
                ["QA Title", "QA Title Load"], before_tick=511,
            )

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
