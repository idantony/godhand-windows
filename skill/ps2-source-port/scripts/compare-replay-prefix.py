"""Compare ordered tagged JSON replay records within an archived tick range."""

import argparse
from collections import Counter
from itertools import zip_longest
import json
from pathlib import Path


def records(lines, tags, before_tick):
    """Read selected records lazily, including from a running replay."""
    for number, line in enumerate(lines, 1):
        tag, _, payload = line.rstrip("\r\n").partition(" ")
        if tag not in tags:
            continue
        try:
            row = json.loads(payload)
            tick = row["tick"]
            if type(tick) is not int:
                raise ValueError("expected an integer tick")
        except (KeyError, TypeError, ValueError) as error:
            raise ValueError(f"Line {number}, {tag}: {error}") from error
        if tick < before_tick:
            yield tag, row


def compare(baseline, replay, tags, before_tick):
    tags = set(tags)
    expected = list(records(baseline, tags, before_tick))
    missing = tags.difference(tag for tag, _ in expected)
    if missing:
        raise ValueError("No baseline records in range for " + ", ".join(sorted(missing)))
    counts = Counter()
    difference = None
    for wanted, actual in zip_longest(expected, records(replay, tags, before_tick)):
        if wanted != actual:
            difference = {"expected": wanted, "actual": actual}
            break
        counts[actual[0]] += 1
    return {
        "before_tick": before_tick,
        "expected_records": len(expected),
        "records_matched": sum(counts.values()),
        "tag_counts": dict(counts),
        "first_difference": difference,
        "full_baseline_match": difference is None,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("replay", type=Path)
    parser.add_argument("--tag", action="append", required=True, help="Record prefix to include. Repeat as needed.")
    parser.add_argument("--before-tick", type=int, required=True, help="Exclusive upper limit shared by both logs.")
    args = parser.parse_args()
    try:
        with args.baseline.open(encoding="utf-8-sig", errors="replace") as baseline:
            with args.replay.open(encoding="utf-8-sig", errors="replace") as replay:
                result = compare(baseline, replay, set(args.tag), args.before_tick)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    print(json.dumps(result, indent=2))
    return 0 if result["full_baseline_match"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
