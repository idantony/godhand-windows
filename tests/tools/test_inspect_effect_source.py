import importlib.util
from pathlib import Path
import struct
import sys
import unittest
sys.dont_write_bytecode=True
TOOLS=Path(__file__).resolve().parents[2]/"tools"
sys.path.insert(0,str(TOOLS))
spec=importlib.util.spec_from_file_location("effect_source",TOOLS/"inspect-effect-source.py")
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

def source():
    raw=bytearray(0x380)
    def put(offset,value):struct.pack_into("<I",raw,offset,value)
    put(16,0x10);put(20,0x40);put(0x24,1);put(0x30,0x10);put(0x60,0x20)
    struct.pack_into("<H",raw,0x40,7)
    put(0x80,0x545345);put(0x84,1)
    raw[0x90+0xBE]=255;raw[0x90+0x133]=6;raw[0x90+0x290]=2
    return raw

def event_source():
    table = 0x60 + 0x18
    first = table + 12
    second = first + 16 + 0x2F0
    raw = bytearray(second + 16 + 0x2F0)
    struct.pack_into("<I", raw, 0x1C, 0x60)
    struct.pack_into("<III", raw, 0x60, 2, 0x14, 0x18)
    raw[0x6C:0x74] = b"TBL\0TBL\0"
    struct.pack_into("<III", raw, table, 2, first - table, second - table)
    for est in (first, second):
        struct.pack_into("<II", raw, est, 0x545345, 1)
    row = second + 16
    struct.pack_into("<I", raw, row + 0x2A0, 0x06000003)
    raw[row + 0xBE] = 3
    raw[row + 0x133] = 0x1A
    return raw, first, row


class SourceBounds(unittest.TestCase):
    def test_known_typed_row(self):
        report=module.est_row(source(),7,0)
        self.assertEqual((report["esp_type"],report["part"],report["scale_mode"]),(6,-1,2))
        self.assertEqual(report["vectors"]["direction"],[0,0,0])
    def test_sprite_byte_halfword_fields(self):
        raw=source();base=0x90
        raw[base+0x129]=5;raw[base+0x12A]=254;raw[base+0x12C]=4;raw[base+0x12D]=2
        struct.pack_into("<h",raw,base+0x28C,-1)
        sprite=module.est_row(raw,7,0)["sprite_parameters"]
        self.assertEqual((sprite["blend129"],sprite["class12a"],sprite["texture_bank28c"]),(5,-2,-1))
        self.assertEqual((sprite["columns12c"],sprite["rows12d"]),(4,2))
    def test_group_missing(self):
        with self.assertRaises(ValueError):module.est_row(source(),8,0)
    def test_row_bounds(self):
        for row in (-1,1):
            with self.assertRaises(ValueError):module.est_row(source(),7,row)
    def test_est_outside(self):
        raw=source();struct.pack_into("<I",raw,0x60,0xFFFFFF00)
        with self.assertRaises(ValueError):module.est_row(raw,7,0)
    def test_huge_count(self):
        raw=source();struct.pack_into("<I",raw,0x24,0xFFFFFFFF)
        with self.assertRaises(ValueError):module.est_row(raw,7,0)
    def test_bad_bundle_directory(self):
        with self.assertRaises(ValueError):module.bundle_entry(bytes(16),"SSD")

    def test_event_uses_second_table_and_one_based_index(self):
        raw, _, row = event_source()
        report = module.event_est_row(raw, 2, 0)
        self.assertEqual((report["bank"], report["index"], report["row_offset"]), (6, 2, row))
        self.assertEqual((report["actor_kind"], report["actor_code"], report["part"], report["esp_type"]),
                         (0x600, 3, 3, 0x1A))
        with self.assertRaises(ValueError):
            module.event_est_row(raw, 0, 0)

    def test_event_est_cannot_read_into_next_source(self):
        raw, first, _ = event_source()
        struct.pack_into("<I", raw, first + 4, 2)
        with self.assertRaisesRegex(ValueError, "rows exceed"):
            module.event_est_row(raw, 1, 1)

    def test_event_group_batch_keeps_requested_order(self):
        raw, _, _ = event_source()
        reports = module.event_est_reports(raw, [2, 1], None)
        self.assertEqual([(row["index"], row["row"], row["esp_type"]) for row in reports],
                         [(2, 0, 0x1A), (1, 0, 0)])

    def test_event_group_batch_reports_empty_directory_slots(self):
        raw, _, _ = event_source()
        struct.pack_into("<I", raw, 0x60 + 0x18 + 4, 0)
        missing = []
        reports = module.event_est_reports(raw, [1, 2], None, missing)
        self.assertEqual((missing, [row["index"] for row in reports]), ([1], [2]))
        with self.assertRaises(ValueError):
            module.event_est_row(raw, 1, 0)

if __name__=="__main__":unittest.main()
