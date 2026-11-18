import importlib.util
import json
from pathlib import Path
import unittest


SCRIPT = Path(__file__).resolve().parents[2] / "skill/ps2-source-port/scripts/check-xaudio2-trace.py"
SPEC = importlib.util.spec_from_file_location("check_xaudio2", SCRIPT)
tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(tool)


def summarize(*rows):
    return tool.summarize(tool.PREFIX + json.dumps(row) for row in rows)


GAIN_ZERO = {"event": "gain", "value": 0}
START = {"event": "start", "voice": 1, "frames": 20, "channels": 1,
         "nonzero_samples": 19, "peak": 32768, "looped": False}
COMPLETE = {"event": "complete", "voice": 1, "frames_observed": 18}


class XAudio2Trace(unittest.TestCase):
    def test_positive_observation_survives_end_of_stream_reset(self):
        result = summarize(GAIN_ZERO, START, COMPLETE)
        self.assertTrue(result["muted_verified"])
        self.assertEqual(result["nonzero_finite_sources_completed"][0]["frames_observed"], 18)

    def test_loop_silence_and_unobserved_progress_do_not_qualify(self):
        for start, complete in (({**START, "looped": True}, COMPLETE),
                                ({**START, "nonzero_samples": 0, "peak": 0}, COMPLETE),
                                (START, {**COMPLETE, "frames_observed": 0})):
            with self.subTest(start=start, complete=complete):
                self.assertEqual(summarize(GAIN_ZERO, start, complete)["nonzero_finite_sources_completed"], [])

    def test_join_rejects_duplicate_and_impossible_completion(self):
        with self.assertRaisesRegex(ValueError, "completion has no unique start"):
            summarize(GAIN_ZERO, START, COMPLETE, COMPLETE)
        with self.assertRaisesRegex(ValueError, "past its submitted frames"):
            summarize(GAIN_ZERO, START, {**COMPLETE, "frames_observed": 21})
        self.assertFalse(summarize(GAIN_ZERO, START, {"event": "gain", "value": 1},
                                   GAIN_ZERO, COMPLETE)["muted_verified"])


if __name__ == "__main__":
    unittest.main()
