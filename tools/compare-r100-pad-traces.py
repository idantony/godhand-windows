"""Compare native R100 replay state at the same original player update."""

import argparse
import json
from pathlib import Path


PREFIX = "QA_R100_PAD_STATE "
FIELDS = ("pad", "hp", "state", "position_bits", "yaw_bits", "scenario", "dead", "target", "skill")


def samples(path):
    result = {}
    previous = -1
    with path.open(encoding="utf-8-sig") as trace:
        for number, line in enumerate(trace, 1):
            if not line.startswith(PREFIX):
                continue
            try:
                row = json.loads(line[len(PREFIX):])
                move = row["move"]
                if type(move) is not int or move <= previous:
                    raise ValueError("expected increasing player updates")
                for field in (*FIELDS, "tick", "scene"):
                    if field not in row:
                        raise ValueError(f"missing {field}")
                for field in ("tick", "scene"):
                    if type(row[field]) is not int:
                        raise ValueError(f"expected integer {field}")
            except (KeyError, TypeError, ValueError) as error:
                raise ValueError(f"{path}:{number}: {error}") from error
            result[move] = row
            previous = move
    if not result:
        raise ValueError(f"No R100 pad state rows in {path}")
    return result


def compare(left, right):
    shared = sorted(left.keys() & right.keys())
    if not shared:
        raise ValueError("The runs have no player updates in common")
    first = {}
    offsets = {}
    for move in shared:
        a, b = left[move], right[move]
        for field in ("tick", "scene"):
            delta = b[field] - a[field]
            offset = offsets.setdefault(field, {"initial": delta, "first_change": None})
            if offset["first_change"] is None and delta != offset["initial"]:
                offset["first_change"] = {"move": move, "offset": delta}
        for field in FIELDS:
            if field not in first and a[field] != b[field]:
                first[field] = {
                    "move": move,
                    "left": {"tick": a["tick"], "scene": a["scene"], "value": a[field]},
                    "right": {"tick": b["tick"], "scene": b["scene"], "value": b[field]},
                }
    return {
        "shared_updates": len(shared),
        "first_shared_update": shared[0],
        "last_shared_update": shared[-1],
        "left_samples": len(left),
        "right_samples": len(right),
        "right_covers_left_prefix": list(right)[:len(left)] == list(left),
        "first_missing_left_update": next((move for move in left if move not in right), None),
        "timeline_offsets": offsets,
        "first_difference_by_field": first,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("left", type=Path)
    parser.add_argument("right", type=Path)
    parser.add_argument("--require-baseline", action="store_true",
                        help="Exit 2 if right does not reproduce all left gameplay samples as its prefix.")
    args = parser.parse_args()
    try:
        result = compare(samples(args.left), samples(args.right))
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    print(json.dumps(result, indent=2))
    if args.require_baseline and (
        not result["right_covers_left_prefix"] or result["first_difference_by_field"]
    ):
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
