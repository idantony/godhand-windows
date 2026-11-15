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
    for move in shared:
        a, b = left[move], right[move]
        for field in FIELDS:
            if field not in first and a[field] != b[field]:
                first[field] = {
                    "move": move,
                    "left": {"tick": a["tick"], "scene": a["scene"], "value": a[field]},
                    "right": {"tick": b["tick"], "scene": b["scene"], "value": b[field]},
                }
    return {"shared_updates": len(shared), "first_shared_update": shared[0], "last_shared_update": shared[-1],
            "left_samples": len(left), "right_samples": len(right), "first_difference_by_field": first}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("left", type=Path)
    parser.add_argument("right", type=Path)
    args = parser.parse_args()
    try:
        result = compare(samples(args.left), samples(args.right))
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
