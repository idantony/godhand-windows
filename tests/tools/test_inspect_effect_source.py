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

if __name__=="__main__":unittest.main()
