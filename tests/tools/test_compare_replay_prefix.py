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


if __name__ == "__main__":
    unittest.main()
