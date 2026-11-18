"""Replace selected R100 replay controls over a half-open Player Move range."""

import argparse
from io import BytesIO
from pathlib import Path
import re


HEADER = b"tick,move,buttons,left_x,left_y,right_x,right_y,connected"
HEADERS = {HEADER: 8, HEADER + b",left_trigger,right_trigger": 10}
AXIS = (-32768, 32767)
INT64_MAX = (1 << 63) - 1
LIMITS = ((1, INT64_MAX), (0, INT64_MAX), (0, 65535),
          AXIS, AXIS, AXIS, AXIS, (0, 1), (0, 255), (0, 255))


def record(line):
    if line.endswith(b"\r\n"):
        return line[:-2], b"\r\n"
    if line.endswith(b"\n"):
        return line[:-1], b"\n"
    return line, b""


def variant(data, from_move, before_move, left_x=None, left_y=None, *, right_trigger=None):
    if not 0 <= from_move < before_move <= INT64_MAX + 1:
        raise ValueError("Expected 0 <= from-move < before-move <= 2^63")
    if (left_x is None) != (left_y is None):
        raise ValueError("Supply both left-stick axes together")
    if left_x is None and right_trigger is None:
        raise ValueError("Supply left-stick axes or a right-trigger value")
    values_to_set = {3: left_x, 4: left_y, 9: right_trigger}
    replacements = {}
    for index, value in values_to_set.items():
        if value is not None:
            low, high = LIMITS[index]
            if not low <= value <= high:
                raise ValueError("Left-stick axes must be signed 16-bit integers" if index < 9 else
                                 "Right trigger must be in 0..255")
            replacements[index] = str(value).encode("ascii")
    lines = BytesIO(data)
    header = lines.readline()
    width = HEADERS.get(record(header)[0])
    if width is None:
        raise ValueError("Invalid R100 pad tape header")
    if right_trigger is not None and width != 10:
        raise ValueError("Right trigger requires a 10-column tape")
    output = bytearray(header)
    last_tick, last_move, next_move = 0, -1, from_move
    for number, line in enumerate(lines, 2):
        row, ending = record(line)
        fields = row.split(b",")
        if len(fields) != width or any(not re.fullmatch(rb"-?[0-9]+", field) for field in fields):
            raise ValueError(f"Line {number}: expected {width} decimal fields")
        values = list(map(int, fields))
        if any(not low <= value <= high for value, (low, high) in zip(values, LIMITS)):
            raise ValueError(f"Line {number}: field outside native parser range")
        tick, move = values[:2]
        if tick <= last_tick or move < last_move:
            raise ValueError(f"Line {number}: expected ascending ticks and nondecreasing Moves")
        if from_move <= move < before_move:
            if move != last_move:
                if move != next_move:
                    raise ValueError(f"Tape does not cover Move {next_move}")
                next_move += 1
            for index, replacement in replacements.items():
                fields[index] = replacement
            line = b",".join(fields) + ending
        output.extend(line)
        last_tick, last_move = tick, move
    if next_move != before_move:
        raise ValueError(f"Tape does not cover Move {next_move}")
    return bytes(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--from-move", type=int, required=True, help="First Move to replace")
    parser.add_argument("--before-move", type=int, required=True, help="Exclusive end Move")
    parser.add_argument("--left-x", type=int)
    parser.add_argument("--left-y", type=int)
    parser.add_argument("--right-trigger", type=int, help="Replace right trigger on a 10-column tape")
    args = parser.parse_args()
    try:
        data = variant(args.source.read_bytes(), args.from_move, args.before_move, args.left_x, args.left_y,
                       right_trigger=args.right_trigger)
        with args.output.open("xb") as output:
            output.write(data)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
