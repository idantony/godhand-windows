import hashlib
import json
from pathlib import Path
from runpy import run_path
from tempfile import TemporaryDirectory
import unittest


ROOT = Path(__file__).resolve().parents[2]
TOOL = run_path(str(ROOT / "tools/export-startup-capture.py"))
FRAME_PIXELS = TOOL["WIDTH"] * TOOL["HEIGHT"]


class StartupCaptureTests(unittest.TestCase):
    def setUp(self):
        self.scratch = TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.directory = Path(self.scratch.name)
        self.receipt = self.directory / "verification.json"
        self.output = self.directory / "preview.png"

    def raw_frame(self, name, color):
        path = self.directory / name
        pixels = bytes(color) * FRAME_PIXELS
        path.write_bytes(pixels)
        return path, hashlib.sha256(pixels).hexdigest()

    def load_receipt(self, proof):
        self.receipt.write_text(json.dumps(proof), encoding="utf-8")

    def test_late_replay_exports_the_latest_presented_capture(self):
        _, actual = self.raw_frame("native.12.rgba", (50, 20, 10, 255))
        _, queued = self.raw_frame("native.13.rgba", (100, 30, 10, 255))
        self.load_receipt({
            "actual_presented": [{"scene": "12"}],
            "files": {"native.12.rgba": actual, "native.13.rgba": queued},
        })
        serial, digest = TOOL["export"](self.receipt, self.output, "composite", None)
        self.assertEqual(serial, 12)
        self.assertEqual(hashlib.sha256(self.output.read_bytes()).hexdigest(), digest)
        self.assertTrue(self.output.read_bytes().startswith(b"\x89PNG\r\n\x1a\n"))
        with self.assertRaisesRegex(ValueError, "No matching presented capture"):
            TOOL["export"](self.receipt, self.output, "composite", 13)

    def test_late_replay_preserves_capture_hash_validation(self):
        self.raw_frame("native.12.rgba.world.rgba", (50, 20, 10, 255))
        self.load_receipt({
            "actual_presented": [{"scene": 12}],
            "files": {"native.12.rgba.world.rgba": "0" * 64},
        })
        with self.assertRaisesRegex(ValueError, "SHA-256 differs"):
            TOOL["export"](self.receipt, self.output, "world", 12)
        self.assertFalse(self.output.exists())

    def test_original_startup_receipt_preserves_explicit_selection(self):
        path, digest = self.raw_frame("candidate.rgba", (50, 20, 10, 255))
        self.load_receipt({
            "tool": "record-startup-frontier",
            "schema": 1,
            "captures": [{"scene_serial": 11, "presented": True,
                          "composite": {"path": str(path), "sha256": digest}}],
        })
        serial, _ = TOOL["export"](self.receipt, self.output, "composite", 11)
        self.assertEqual(serial, 11)


if __name__ == "__main__":
    unittest.main()
