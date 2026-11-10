"""Public world claims require matching private image/process evidence."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("export_progress", ROOT / "tools/export-progress.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)


class WorldEvidenceTests(unittest.TestCase):
    def setUp(self):
        base = Path("G:/middle_thing/godhand_windows/tests/export-progress")
        base.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=str(base)); self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve(); self.snapshot = self.root / "source"
        self.snapshot.mkdir(); self.raw = self.root / "frame.rgba.world.rgba"
        self.raw.write_bytes(bytes([1, 2, 3, 128]) + bytes(512*448*4-4))
        self.final = self.root / "frame.rgba"; self.final.write_bytes(bytes([0, 0, 0, 255])*(512*448))
        self.process = self.root / "startup-process.json"
        self.build = {"executable": str(self.root / "probe.exe"), "executable_sha256": "1"*64}
        self.process.write_text(json.dumps({"command": [self.build["executable"]], "executable_sha256_before": "1"*64}), encoding="utf-8")
        self.observation = {"snapshot": str(self.snapshot), "native_whole_verified": True, "packet_count": 10,
                            "capture_metadata": {"scene_serial": 370, "fade_visible": 1, "fade_rgba": "FF000000"}}
        self.path = self.root / "observation.json"
        self.execution = dict(world_packets_submitted=10, scene_frame_serial=370, renderer_error=0, world_gpu_error=0, window_error=0)
        self.save()

    def save(self):
        self.observation["files"] = {str(p): {"bytes": p.stat().st_size, "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}
                                     for p in (self.raw, self.final, self.process)}
        self.path.write_text(json.dumps(self.observation), encoding="utf-8")

    def summary(self):
        return tool.world_layer_summary(self.path, self.snapshot, self.execution, self.build)

    def test_matching_world_under_opaque_fade_exports_no_private_paths(self):
        d = self.summary(); self.assertEqual(d["nonblack_world_pixels"], 1)
        self.assertEqual(d["nonblack_composite_pixels"], 0)
        self.assertIn("opaque opening fade", d["final_composite"])
        self.assertNotIn(str(self.root), json.dumps(d))

    def test_other_or_unverified_snapshot_rejected(self):
        for key, value in (("native_whole_verified", False), ("snapshot", str(self.root / "other"))):
            with self.subTest(key=key):
                old = self.observation[key]; self.observation[key] = value; self.save()
                with self.assertRaises(ValueError): self.summary()
                self.observation[key] = old

    def test_modified_image_or_other_launch_rejected(self):
        self.raw.write_bytes(b"changed")
        with self.assertRaises(ValueError): self.summary()
        self.raw.write_bytes(bytes([1, 2, 3, 128]) + bytes(512*448*4-4)); self.save()
        self.build["executable_sha256"] = "2"*64
        with self.assertRaises(ValueError): self.summary()

    def test_black_world_or_gpu_error_cannot_be_claimed(self):
        self.raw.write_bytes(bytes(512*448*4)); self.save()
        with self.assertRaises(ValueError): self.summary()
        self.raw.write_bytes(bytes([1, 2, 3, 128]) + bytes(512*448*4-4)); self.save()
        self.execution["world_gpu_error"] = 2
        with self.assertRaises(ValueError): self.summary()


if __name__ == "__main__":
    unittest.main()
