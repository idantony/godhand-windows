"""Check native WinMM submissions against normal buffer completion records."""

import argparse
import json
from pathlib import Path


PREFIX = "NATIVE_WINMM "
SOURCES = ("adx", "movie")


def summarize(lines):
    submitted = {}
    complete = set()
    totals = {}
    for number, line in enumerate(lines, 1):
        if not line.startswith(PREFIX):
            continue
        try:
            row = json.loads(line[len(PREFIX):])
            source, stream, buffer = row["source"], row["stream"], row["buffer"]
            frames = row["frames"]
            if source not in SOURCES or any(type(value) is not int or value <= 0 for value in (stream, buffer, frames)):
                raise ValueError("invalid buffer identity or size")
            key = (source, stream, buffer)
            if row["event"] == "submit":
                if key in submitted or type(row["muted"]) is not bool or type(row["rate"]) is not int or row["rate"] <= 0:
                    raise ValueError("invalid or duplicate submission")
                for phase in ("decoded", "submitted"):
                    count, peak = row[f"{phase}_nonzero"], row[f"{phase}_peak"]
                    if (type(count) is not int or type(peak) is not int or not 0 <= count <= 2 * frames
                            or not 0 <= peak <= 32768 or bool(count) != bool(peak)):
                        raise ValueError("invalid PCM counters")
                if row["muted"] and row["submitted_nonzero"]:
                    raise ValueError("PCM mute was marked on a nonzero submission")
                submitted[key] = row
                stats = totals.setdefault(source, {"buffers_submitted": 0, "buffers_completed": 0,
                    "frames_completed": 0, "nonzero_decoded_buffers_completed": 0,
                    "nonzero_submitted_buffers_completed": 0, "nonzero_samples_submitted_in_completed_buffers": 0})
                stats["buffers_submitted"] += 1
            elif row["event"] == "complete":
                start = submitted.get(key)
                if start is None or key in complete or frames != start["frames"]:
                    raise ValueError("completion has no unique matching submission")
                complete.add(key)
                stats = totals[source]
                stats["buffers_completed"] += 1
                stats["frames_completed"] += frames
                stats["nonzero_decoded_buffers_completed"] += bool(start["decoded_nonzero"])
                stats["nonzero_submitted_buffers_completed"] += bool(start["submitted_nonzero"])
                stats["nonzero_samples_submitted_in_completed_buffers"] += start["submitted_nonzero"]
            else:
                raise ValueError("unknown WinMM event")
        except (KeyError, TypeError, ValueError) as error:
            raise ValueError(f"Line {number}: {error}") from error
    return totals


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--source", action="append", choices=SOURCES, default=[])
    parser.add_argument("--require-submitted-nonzero", action="store_true")
    args = parser.parse_args()
    try:
        with args.trace.open(encoding="utf-8-sig") as lines:
            result = summarize(lines)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    print(json.dumps(result, indent=2))
    counter = ("nonzero_submitted_buffers_completed" if args.require_submitted_nonzero
               else "nonzero_decoded_buffers_completed")
    required = args.source or result.keys()
    return 0 if required and all(result.get(source, {}).get(counter, 0) for source in required) else 2


if __name__ == "__main__":
    raise SystemExit(main())
