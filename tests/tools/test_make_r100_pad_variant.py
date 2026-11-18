import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


PROJECT = Path(__file__).resolve().parents[2]
SCRIPT = PROJECT / "tools/make-r100-pad-variant.py"
SPEC = importlib.util.spec_from_file_location("r100_pad_variant", SCRIPT)
PAD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PAD)

HEADER = b"tick,move,buttons,left_x,left_y,right_x,right_y,connected"
TRIGGERS = b",left_trigger,right_trigger"


class PadVariantTests(unittest.TestCase):
    def test_changes_only_selected_axes_in_all_rows_for_each_move(self):
        start = HEADER + TRIGGERS + b"\r\n0001,9,16,0,0,0,0,1,255,0\n"
        tail = b"6,12,0,55,0,0,0,1,0,0"
        data = (start + b"2,10,64,-9,1,-3,5,1,7,201\r\n"
                b"4,10,0,1,-9,5,-3,0,0,199\n"
                b"5,11,1,0,0,0,0,1,3,4\n" + tail)
        expected = (start + b"2,10,64,-32768,32767,-3,5,1,7,201\r\n"
                    b"4,10,0,-32768,32767,5,-3,0,0,199\n"
                    b"5,11,1,-32768,32767,0,0,1,3,4\n" + tail)
        self.assertEqual(PAD.variant(data, 10, 12, -32768, 32767), expected)

    def test_rejects_missing_moves_and_bad_range(self):
        data = HEADER + b"\n1,10,0,0,0,0,0,1\n2,12,0,0,0,0,0,1\n"
        for begin, end, missing in ((9, 10, 9), (10, 13, 11), (12, 14, 13)):
            with self.subTest(begin=begin, end=end), self.assertRaisesRegex(ValueError, f"Move {missing}"):
                PAD.variant(data, begin, end, 0, 1)
        with self.assertRaisesRegex(ValueError, "from-move"):
            PAD.variant(data, 10, 10, 0, 1)
        with self.assertRaisesRegex(ValueError, "signed 16-bit"):
            PAD.variant(data, 10, 11, 32768, 1)

    def test_right_trigger_only_or_combined_with_stick(self):
        prefix = HEADER + TRIGGERS + b"\r\n"
        data = prefix + b"10,9994,0,17,-21,0,0,1,3,255\n11,9995,0,0,0,0,0,1,0,42\r\n"
        self.assertEqual(PAD.variant(data, 9994, 9995, right_trigger=0),
                         prefix + b"10,9994,0,17,-21,0,0,1,3,0\n11,9995,0,0,0,0,0,1,0,42\r\n")
        self.assertIn(b"10,9994,0,4,5,0,0,1,3,7\n",
                      PAD.variant(data, 9994, 9995, 4, 5, right_trigger=7))

    def test_rejects_missing_fields_incomplete_stick_and_bad_trigger(self):
        data = HEADER + b"\n1,10,0,0,0,0,0,1\n"
        with self.assertRaisesRegex(ValueError, "Supply left-stick axes or a right-trigger"):
            PAD.variant(data, 10, 11)
        with self.assertRaisesRegex(ValueError, "both left-stick axes"):
            PAD.variant(data, 10, 11, left_x=0)
        with self.assertRaisesRegex(ValueError, "10-column tape"):
            PAD.variant(data, 10, 11, right_trigger=0)
        with self.assertRaisesRegex(ValueError, "0..255"):
            PAD.variant(data, 10, 11, right_trigger=256)

    def test_rejects_native_parser_invalid_data_even_outside_branch(self):
        row = b"1,10,0,0,0,0,0,1\n"
        for invalid in (b"1,11,0,0,0,0,0,1\n", b"2,9,0,0,0,0,0,1\n",
                        b"2,12,0,0,0,0,0,2\n", b"2,12,0,0,0,0,0,1,0\n",
                        b"2,12,0,+1,0,0,0,1\n", b"9223372036854775808,12,0,0,0,0,0,1"):
            with self.subTest(invalid=invalid), self.assertRaisesRegex(ValueError, "Line 3"):
                PAD.variant(HEADER + b"\n" + row + invalid, 10, 11, 0, 1)
        with self.assertRaisesRegex(ValueError, "header"):
            PAD.variant(HEADER + b",extra\n" + row, 10, 11, 0, 1)

    def test_cli_does_not_overwrite(self):
        scratch = PROJECT / ".work/tool-tests"
        scratch.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as directory:
            source = Path(directory) / "source.csv"
            output = Path(directory) / "variant.csv"
            source.write_bytes(HEADER + b"\n1,10,0,0,0,0,0,1\n")
            command = [sys.executable, "-B", str(SCRIPT), str(source), str(output),
                       "--from-move", "10", "--before-move", "11", "--left-x", "2", "--left-y", "-3"]
            first = subprocess.run(command, capture_output=True)
            self.assertEqual(first.returncode, 0, first.stderr)
            expected = HEADER + b"\n1,10,0,2,-3,0,0,1\n"
            self.assertEqual(output.read_bytes(), expected)
            second = subprocess.run(command, capture_output=True)
            self.assertNotEqual(second.returncode, 0)
            self.assertEqual(output.read_bytes(), expected)


if __name__ == "__main__":
    unittest.main()
