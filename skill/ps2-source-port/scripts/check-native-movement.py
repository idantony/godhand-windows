"""Verify an observed left-stick movement interval in a NATIVE_INPUT trace."""

import argparse
import json
import math
from pathlib import Path
import struct


PREFIX = "NATIVE_INPUT "


def integer(value, minimum, maximum=None):
    return type(value) is int and minimum <= value and (maximum is None or value <= maximum)


def verify(lines, before_tick, through_tick, room, min_distance, *, lx=None, ly=None):
    if not 0 <= before_tick < through_tick or not integer(room, 1, 0xffff):
        raise ValueError("Expected a nonempty tick interval and a valid room")
    if not math.isfinite(min_distance) or min_distance <= 0:
        raise ValueError("Minimum distance must be positive and finite")
    if any(axis is not None and not integer(axis, -128, 127) for axis in (lx, ly)):
        raise ValueError("Expected axes must be signed 8-bit values")

    first = previous = None
    accepted = 0
    expected_tick = before_tick
    for number, line in enumerate(lines, 1):
        if not line.startswith(PREFIX):
            continue
        try:
            row = json.loads(line[len(PREFIX):])
            tick = row["tick"]
            if not integer(tick, 0):
                raise ValueError("invalid tick")
            if tick < before_tick:
                continue
            if tick > through_tick:
                break
            if tick != expected_tick:
                raise ValueError(f"expected tick {expected_tick}, got {tick}")
            player, control = row["player"], row["input"]
            if (not integer(row["room"], room, room) or not integer(row["next_room"], room, room)
                    or not integer(row["game_state"], 4, 4) or row["frontier"] != "00000000"
                    or not integer(player["ready"], 1, 1) or not integer(player["hp"], 1)):
                raise ValueError("expected active gameplay and a living player in the same room")
            updates, presents = player["updates"], row["scene_presents"]
            if not integer(updates, 0) or not integer(presents, 0):
                raise ValueError("invalid update or presentation counter")
            bits = player["position_bits"]
            if not isinstance(bits, list) or len(bits) != 3 or any(not integer(bit, 0, 0xffffffff) for bit in bits):
                raise ValueError("expected three binary32 position words")
            position = struct.unpack("<3f", struct.pack("<3I", *bits))
            if not all(map(math.isfinite, position)):
                raise ValueError("nonfinite position")
            observed = {"tick": tick, "hp": player["hp"], "updates": updates,
                        "presents": presents, "position": position}
            if first is None:
                first = observed
            else:
                if player["hp"] != first["hp"] or updates < previous["updates"] or presents < previous["presents"]:
                    raise ValueError("HP changed or a counter regressed")
                axes = control["axes"]
                if (not integer(control["analog_valid"], 1, 1) or not integer(control["focused"], 1, 1)
                        or not isinstance(axes, list) or len(axes) != 4
                        or not all(integer(axis, -128, 127) for axis in axes)
                        or not any(axes[:2]) or (lx is not None and axes[0] != lx)
                        or (ly is not None and axes[1] != ly)):
                    raise ValueError("expected an accepted left-stick sample")
                accepted += 1
            previous = observed
            expected_tick += 1
            if tick == through_tick:
                break
        except (ValueError, KeyError, TypeError) as error:
            raise ValueError(f"Line {number}: {error}") from error
    if expected_tick <= through_tick:
        raise ValueError(f"Trace does not cover tick {expected_tick} through {through_tick}")
    if previous["updates"] == first["updates"] or previous["presents"] == first["presents"]:
        raise ValueError("No new player updates or real presents in the interval")
    start, end = first["position"], previous["position"]
    distance = math.hypot(end[0] - start[0], end[2] - start[2])
    if distance < min_distance:
        raise ValueError(f"Horizontal movement {distance:.6f} is below {min_distance:.6f}")
    return {"room": room, "first": first, "last": previous, "accepted_ticks": accepted,
            "expected_axes": [lx, ly], "horizontal_distance": distance}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--before-tick", type=int, required=True, help="Recorded step before the accepted inputs")
    parser.add_argument("--through-tick", type=int, required=True, help="Last accepted input step, inclusive")
    parser.add_argument("--room", type=lambda value: int(value, 0), required=True)
    parser.add_argument("--min-distance", type=float, required=True, help="Minimum net horizontal distance")
    parser.add_argument("--lx", type=int)
    parser.add_argument("--ly", type=int)
    args = parser.parse_args()
    try:
        with args.trace.open(encoding="utf-8-sig") as lines:
            result = verify(lines, args.before_tick, args.through_tick, args.room, args.min_distance,
                            lx=args.lx, ly=args.ly)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
