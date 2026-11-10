"""Read-only bounded SSD/EST typed field report, no game byte extraction.
Repeated FX source-field audits share this tool rather than ad-hoc test printf.
Output must be a fresh G: scratch JSON. Metadata is not runtime completion proof.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.dont_write_bytecode=True
from inspect_disc import Iso9660, SECTOR
from inspect_assets import parse_fst

def word(raw, offset):
    if offset < 0 or offset + 4 > len(raw):
        raise ValueError("word outside source")
    return struct.unpack_from("<I", raw, offset)[0]

def span(raw, offset, size):
    if offset < 0 or size < 0 or offset + size > len(raw):
        raise ValueError("span outside source")
    return raw[offset:offset + size]

def bundle_entry(raw, tag):
    count = word(raw, 0)
    if not count or count > (len(raw) - 4) // 8:
        raise ValueError("invalid bundle directory")
    offsets = [word(raw, 4 + i * 4) for i in range(count)]
    if offsets != sorted(offsets) or offsets[0] < 4 + count * 8 or offsets[-1] > len(raw):
        raise ValueError("invalid bundle offsets")
    tags = [span(raw, 4 + count * 4 + i * 4, 4).split(b"\0")[0].decode("ascii") for i in range(count)]
    index = tags.index(tag)
    end = offsets[index + 1] if index + 1 < count else len(raw)
    return span(raw, offsets[index], end - offsets[index])

def est_row(ssd, group_id, row):
    groups = 16 + word(ssd, 16)
    table = 16 + word(ssd, 20)
    count = word(ssd, groups + 4)
    span(ssd, groups + 16, count * 4)
    span(ssd, table + 16, count * 4)
    for index in range(count):
        offset = groups + 16 + word(ssd, groups + 16 + index * 4)
        if struct.unpack("<H", span(ssd, offset, 2))[0] != group_id:
            continue
        est = table + 16 + word(ssd, table + 16 + index * 4)
        if word(ssd, est) != 0x545345:
            raise ValueError("matching source is not EST")
        rows = word(ssd, est + 4)
        if row < 0 or row >= rows:
            raise ValueError("EST row out of range")
        source = span(ssd, est + 16 + row * 0x2F0, 0x2F0)
        vector_offsets = dict(scale=0x18,scale_variation=0x20,euler=0x48,euler_velocity=0x54,euler_variation=0x64,euler_velocity_variation=0x70,position=0x7C,position_variation=0x88,direction=0x94,direction_parameters_a0=0xA0)
        vectors = {}
        for name, start in vector_offsets.items():
            width = 2 if name in ("scale", "scale_variation") else 3
            vectors[name] = [struct.unpack("<f", span(source, start + axis * 4, 4))[0] for axis in range(width)]
        return dict(ordinal=index,group_id=group_id,row=row,row_count=rows,est_offset=est,source_sha256=hashlib.sha256(source).hexdigest(),flags=f"{word(source,0):08X}",flags4=f"{word(source,4):08X}",emitter_type=source[0x128],esp_type=source[0x133],part=struct.unpack("b",source[0xBE:0xBF])[0],seed=source[0x28E],direction_mode=source[0x28F],scale_mode=source[0x290],position_mode=source[0x291],euler_mode=source[0x292],vectors=vectors,motion_float_fields={f"{offset:X}":struct.unpack("<f",span(source,offset,4))[0] for offset in range(0xA0,0xBC,4)},colour_mode=source[0x293],sprite_parameters=dict(blend129=source[0x129],class12a=struct.unpack("b",source[0x12A:0x12B])[0],columns12c=source[0x12C],rows12d=source[0x12D],texture_item12e=source[0x12E],texture_bank28c=struct.unpack("<h",source[0x28C:0x28E])[0],near134=source[0x134],fade135=source[0x135],far2ca=struct.unpack("<H",source[0x2CA:0x2CC])[0],fade_in2c0=word(source,0x2C0),fade_hold2c4=word(source,0x2C4),aspect_anchor=[struct.unpack("<f",span(source,o,4))[0] for o in (8,0x10,0x14)],colour_c4_d0=[struct.unpack("<f",span(source,o,4))[0] for o in range(0xC4,0xD4,4)]),detach_delay_bf=struct.unpack("b",source[0xBF:0xC0])[0],type6_parameters={"field16C":word(source,0x16C),"count170":word(source,0x170),"radius13C":struct.unpack("<f",span(source,0x13C,4))[0],"spacing140":struct.unpack("<f",span(source,0x140,4))[0]})
    raise ValueError("group not present")

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso",type=Path,required=True)
    parser.add_argument("--room",default="/ST1/R100.DAT")
    parser.add_argument("--index",type=int,default=0)
    parser.add_argument("--row",type=int,default=0)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    output=args.output.resolve()
    if output.drive.upper() != "G:" or output.suffix.lower() != ".json" or output.exists():
        raise ValueError("output must be fresh G: scratch JSON")
    iso=Iso9660(args.iso)
    try:
        files={item["path"].upper():item for item in iso.inventory() if not item["directory"]}
        fst=files["/GODHAND.FST"]
        _,named=parse_fst(iso.read(fst["lba"]*SECTOR,fst["size"]))
        entry=next(item for item in named if item["path"].upper()==args.room.strip("/").upper())
        afs=files["/_DATA.AFS"];base=afs["lba"]*SECTOR
        header=iso.read(base,8)
        if header[:4]!=b"AFS\0" or entry["afs_index"]>=word(header,4):
            raise ValueError("invalid AFS slot")
        slot=iso.read(base+8+entry["afs_index"]*8,8)
        offset,size=struct.unpack("<II",slot)
        if offset<8+word(header,4)*8 or offset+size>afs["size"]:
            raise ValueError("AFS slot outside archive")
        raw=iso.read(base+offset,size)
        report=est_row(bundle_entry(raw,"SSD"),args.index,args.row)
        report.update(schema=1,iso=str(args.iso.resolve()),room=entry["path"],room_sha256=hashlib.sha256(raw).hexdigest(),limitation="readonly typed source metadata; not execution or FX creation proof")
        with output.open("x",encoding="utf-8") as stream:json.dump(report,stream,indent=2,allow_nan=False);stream.write("\n")
        print(output)
    finally:iso.close()

if __name__=="__main__":main()
