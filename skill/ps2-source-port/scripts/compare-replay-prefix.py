"""Compare replay logs before an archived tick or line boundary."""

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


def compare_bytes_before(baseline, replay, marker):
    """Compare every byte before the first baseline line starting with marker."""
    if not marker or b"\r" in marker or b"\n" in marker:
        raise ValueError("Expected a nonempty line prefix without a line break")
    actual_lines = iter(replay)
    matched_bytes = 0
    for number, expected in enumerate(baseline, 1):
        if expected.startswith(marker):
            if number == 1:
                raise ValueError("The baseline marker has no preceding lines")
            return {
                "baseline_boundary_line": number,
                "lines_matched": number - 1,
                "bytes_matched": matched_bytes,
                "first_difference": None,
                "full_baseline_match": True,
            }
        actual = next(actual_lines, None)
        if actual != expected:
            return {
                "lines_matched": number - 1,
                "bytes_matched": matched_bytes,
                "first_difference": {
                    "line": number,
                    "expected": expected.decode("utf-8", errors="backslashreplace"),
                    "actual": actual.decode("utf-8", errors="backslashreplace") if actual is not None else None,
                },
                "full_baseline_match": False,
            }
        matched_bytes += len(expected)
    raise ValueError("The baseline does not contain the requested line prefix")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("replay", type=Path)
    parser.add_argument("--tag", action="append", help="JSON record prefix to include. Repeat as needed.")
    boundary = parser.add_mutually_exclusive_group(required=True)
    boundary.add_argument("--before-tick", type=int, help="Exclusive tick limit for tagged JSON records.")
    boundary.add_argument("--before-line-prefix", help="Compare raw bytes up to the first baseline line with this prefix.")
    args = parser.parse_args()
    if args.before_tick is not None and not args.tag:
        parser.error("--before-tick requires at least one --tag")
    if args.before_line_prefix is not None and args.tag:
        parser.error("--before-line-prefix includes every preceding line")
    try:
        if args.before_tick is not None:
            with args.baseline.open(encoding="utf-8-sig", errors="replace") as baseline:
                with args.replay.open(encoding="utf-8-sig", errors="replace") as replay:
                    result = compare(baseline, replay, args.tag, args.before_tick)
        else:
            with args.baseline.open("rb") as baseline, args.replay.open("rb") as replay:
                result = compare_bytes_before(baseline, replay, args.before_line_prefix.encode("utf-8"))
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    print(json.dumps(result, indent=2))
    return 0 if result["full_baseline_match"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
