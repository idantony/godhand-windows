import importlib.util
import json
from pathlib import Path
import unittest


SCRIPT = Path(__file__).resolve().parents[2] / "skill/ps2-source-port/scripts/check-winmm-trace.py"
SPEC = importlib.util.spec_from_file_location("check_winmm", SCRIPT)
tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(tool)


def summarize(*rows):
    return tool.summarize(tool.PREFIX + json.dumps(row) for row in rows)


SUBMIT = {"event": "submit", "source": "adx", "stream": 1, "buffer": 1, "frames": 10,
          "rate": 48000, "muted": True, "decoded_nonzero": 19, "decoded_peak": 200,
          "submitted_nonzero": 0, "submitted_peak": 0}
COMPLETE = {"event": "complete", "source": "adx", "stream": 1, "buffer": 1, "frames": 10}


class WinMMTrace(unittest.TestCase):
    def test_separates_decoded_and_submitted_pcm(self):
        stats = summarize(SUBMIT, COMPLETE)["adx"]
        self.assertEqual(stats["frames_completed"], 10)
        self.assertEqual(stats["nonzero_decoded_buffers_completed"], 1)
        self.assertEqual(stats["nonzero_submitted_buffers_completed"], 0)
        live = {**SUBMIT, "muted": False, "submitted_nonzero": 5, "submitted_peak": 1}
        self.assertEqual(summarize(live, COMPLETE)["adx"]["nonzero_samples_submitted_in_completed_buffers"], 5)

    def test_matches_buffer_identity_per_stream_and_ignores_pending(self):
        next_stream = {**SUBMIT, "stream": 2}
        stats = summarize(SUBMIT, next_stream, {**COMPLETE, "stream": 2})["adx"]
        self.assertEqual(stats["buffers_submitted"], 2)
        self.assertEqual(stats["buffers_completed"], 1)

    def test_rejects_inconsistent_completion_or_mute(self):
        for rows in ((COMPLETE,), (SUBMIT, {**COMPLETE, "frames": 9}), (SUBMIT, COMPLETE, COMPLETE)):
            with self.subTest(rows=rows), self.assertRaisesRegex(ValueError, "unique matching submission"):
                summarize(*rows)
        with self.assertRaisesRegex(ValueError, "PCM mute"):
            summarize({**SUBMIT, "submitted_nonzero": 1, "submitted_peak": 1})


if __name__ == "__main__":
    unittest.main()
