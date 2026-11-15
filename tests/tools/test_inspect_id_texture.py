"""Regression fixtures for the layout and bounds of original ID textures."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[2] / "tools"
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location("inspect_id_texture", TOOLS / "inspect-id-texture.py")
texture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(texture)


class TestIdTexture(unittest.TestCase):
    def test_game_over_tim2_picture(self):
        raw = bytearray(16 + 48 + 1024 + 1024)
        struct.pack_into("<4sBBH", raw, 0, b"TIM2", 4, 0, 1)
        struct.pack_into("<IIIHH", raw, 16, len(raw) - 16, 1024, 1024, 48, 256)
        struct.pack_into("<BBHHQ", raw, 34, 3, 5, 32, 32, 0x155308000)
        table = struct.pack("<II", 1, 8) + raw
        offset, item = texture.table_item(table, 1)
        self.assertEqual(offset, 8)
        report = texture.texture_metadata(item)
        self.assertEqual((report["width"], report["height"], report["psm"], report["colors"]),
                         (32, 32, 19, 256))
        self.assertEqual(report["tex0"], "0000000155308000")
        with self.assertRaises(ValueError):
            texture.texture_metadata(item[:-1])

    def test_open_second_tbl_of_idd(self):
        idd = struct.pack("<III4s4s", 2, 20, 24, b"TBL", b"TBL") + b"data" + b"tex"
        self.assertEqual(texture.bundle_entry(idd, "TBL", 1), b"tex")

    def test_texture_table_rejects_offsets_into_its_own_header(self):
        with self.assertRaisesRegex(ValueError, "outside payload"):
            texture.table_item(struct.pack("<III", 2, 12, 6) + b"texture", 1)


if __name__ == "__main__":
    unittest.main()
