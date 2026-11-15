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
from inspect_assets import bundle_bounds, bundle_entry, parse_fst

def word(raw, offset):
    if offset < 0 or offset + 4 > len(raw):
        raise ValueError("word outside source")
    return struct.unpack_from("<I", raw, offset)[0]

def span(raw, offset, size):
    if offset < 0 or size < 0 or offset + size > len(raw):
        raise ValueError("span outside source")
    return raw[offset:offset + size]

def est_row_count(raw, est, end):
    span(raw, est, 16)
    if est + 16 > end or word(raw, est) != 0x545345:
        raise ValueError("matching source is not EST")
    rows = word(raw, est + 4)
    if rows > (end - est - 16) // 0x2F0:
        raise ValueError("EST rows exceed source entry")
    return rows


def describe_est(raw, est, end, row):
    rows = est_row_count(raw, est, end)
    if not 0 <= row < rows:
        raise ValueError("EST row out of range")
    offset = est + 16 + row * 0x2F0
    source = span(raw, offset, 0x2F0)

    def floats(start, width):
        return list(struct.unpack("<" + "f" * width, span(source, start, width * 4)))

    def signed_byte(start):
        return struct.unpack_from("b", source, start)[0]

    vector_fields = {
        "scale": (0x18, 2), "scale_variation": (0x20, 2), "euler": (0x48, 3),
        "euler_velocity": (0x54, 3), "euler_variation": (0x64, 3),
        "euler_velocity_variation": (0x70, 3), "position": (0x7C, 3),
        "position_variation": (0x88, 3), "direction": (0x94, 3),
        "direction_parameters_a0": (0xA0, 3),
    }
    actor = word(source, 0x2A0)
    return dict(
        row=row, row_count=rows, est_offset=est, row_offset=offset,
        source_sha256=hashlib.sha256(source).hexdigest(), flags=f"{word(source, 0):08X}",
        flags4=f"{word(source, 4):08X}", emitter_type=source[0x128], esp_type=source[0x133],
        actor_kind=actor >> 16, actor_code=actor & 255, part=signed_byte(0xBE), seed=source[0x28E],
        direction_mode=source[0x28F], scale_mode=source[0x290], position_mode=source[0x291],
        euler_mode=source[0x292], vectors={name: floats(*args) for name, args in vector_fields.items()},
        motion_float_fields={f"{field:X}": floats(field, 1)[0] for field in range(0xA0, 0xBC, 4)},
        colour_mode=source[0x293], sprite_parameters=dict(
            blend129=source[0x129], class12a=signed_byte(0x12A), columns12c=source[0x12C],
            rows12d=source[0x12D], texture_item12e=source[0x12E],
            texture_bank28c=struct.unpack_from("<h", source, 0x28C)[0], near134=source[0x134],
            fade135=source[0x135], far2ca=struct.unpack_from("<H", source, 0x2CA)[0],
            fade_in2c0=word(source, 0x2C0), fade_hold2c4=word(source, 0x2C4),
            depth_bias138=floats(0x138, 1)[0],
            aspect_anchor=[floats(field, 1)[0] for field in (8, 0x10, 0x14)],
            colour_c4_d0=floats(0xC4, 4)),
        detach_delay_bf=signed_byte(0xBF), type6_parameters={
            "field16C": word(source, 0x16C), "count170": word(source, 0x170),
            "radius13C": floats(0x13C, 1)[0], "spacing140": floats(0x140, 1)[0]})


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
        offsets = [table + 16 + word(ssd, table + 16 + item * 4) for item in range(count)]
        est = offsets[index]
        end = min([len(ssd)] + [offset for offset in offsets if offset > est])
        return dict(ordinal=index, group_id=group_id, **describe_est(ssd, est, end, row))
    raise ValueError("group not present")


def event_est_reports(raw, indices, row, missing_indices=None):
    # Original 297880's event effect bundle, then 305758's second TBL.
    span(raw, 0, 0x5C)
    start = word(raw, 0x1C)
    if start < 0x5C or start >= len(raw):
        raise ValueError("event effect bundle outside source")
    sections = [word(raw, field) for field in range(4, 0x5C, 4)]
    end = min([len(raw)] + [offset for offset in sections if offset > start])
    table_start, table_end = bundle_bounds(raw[start:end], "TBL", 1)
    table_start += start
    table_end += start
    if table_end - table_start < 4:
        raise ValueError("event effect TBL is truncated")
    count = word(raw, table_start)
    if count > (table_end - table_start - 4) // 4:
        raise ValueError("event effect directory outside second TBL")
    offsets = [word(raw, table_start + 4 * item) for item in range(1, count + 1)]
    reports = []
    for index in indices:
        if not 1 <= index <= count:
            raise ValueError("event effect index outside second TBL")
        offset = offsets[index - 1]
        if offset == 0 and missing_indices is not None:
            missing_indices.append(index)
            continue
        if offset < 4 + count * 4 or offset >= table_end - table_start:
            raise ValueError("event EST offset outside second TBL")
        est = table_start + offset
        end = min([table_end] + [table_start + next_offset for next_offset in offsets if next_offset > offset])
        rows = range(est_row_count(raw, est, end)) if row is None else (row,)
        reports.extend(dict(bank=6, index=index, **describe_est(raw, est, end, number)) for number in rows)
    return reports


def event_est_row(raw, index, row):
    return event_est_reports(raw, [index], row)[0]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso",type=Path,required=True)
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--room", default="/ST1/R100.DAT")
    source.add_argument("--event", type=int, help="Event number for the original bank 6 table")
    parser.add_argument("--index", type=int, action="append", help="Repeat to inspect several event groups in one ISO read")
    rows = parser.add_mutually_exclusive_group()
    rows.add_argument("--row", type=int, default=0)
    rows.add_argument("--all-rows", action="store_true", help="Report every row for the selected event groups")
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    indices = list(dict.fromkeys(args.index or [0]))
    if args.event is None and (len(indices) != 1 or args.all_rows):
        parser.error("Multiple indices and --all-rows require --event")
    if args.event is not None and not args.index:
        parser.error("--event requires at least one --index")
    output=args.output.resolve()
    if output.drive.upper() != "G:" or output.suffix.lower() != ".json" or output.exists():
        raise ValueError("output must be fresh G: scratch JSON")
    iso=Iso9660(args.iso)
    try:
        files={item["path"].upper():item for item in iso.inventory() if not item["directory"]}
        fst=files["/GODHAND.FST"]
        _,named=parse_fst(iso.read(fst["lba"]*SECTOR,fst["size"]))
        path = args.room if args.event is None else f"event/ev{args.event:02x}.dat"
        entry=next(item for item in named if item["path"].upper()==path.strip("/").upper())
        afs=files["/_DATA.AFS"];base=afs["lba"]*SECTOR
        header=iso.read(base,8)
        if header[:4]!=b"AFS\0" or entry["afs_index"]>=word(header,4):
            raise ValueError("invalid AFS slot")
        slot=iso.read(base+8+entry["afs_index"]*8,8)
        offset,size=struct.unpack("<II",slot)
        if offset<8+word(header,4)*8 or offset+size>afs["size"]:
            raise ValueError("AFS slot outside archive")
        raw=iso.read(base+offset,size)
        if args.event is None:
            report=est_row(bundle_entry(raw,"SSD"),indices[0],args.row)
            report.update(room=entry["path"],room_sha256=hashlib.sha256(raw).hexdigest())
        else:
            if args.all_rows or len(indices) > 1:
                missing = []
                report = {"rows": event_est_reports(raw, indices, None if args.all_rows else args.row, missing),
                          "missing_indices": missing}
            else:
                report = event_est_row(raw, indices[0], args.row)
            report.update(event=args.event,event_path=entry["path"],event_sha256=hashlib.sha256(raw).hexdigest())
        report.update(schema=1,iso=str(args.iso.resolve()),limitation="read-only typed source metadata")
        with output.open("x",encoding="utf-8") as stream:json.dump(report,stream,indent=2,allow_nan=False);stream.write("\n")
        print(output)
    finally:iso.close()

if __name__=="__main__":main()
