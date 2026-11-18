"""Verify nonzero PCM and source-queue completion in one native XAudio2 trace."""

import argparse
import json
from pathlib import Path


PREFIX = "NATIVE_XAUDIO2 "


def summarize(lines):
    starts = {}
    completed = set()
    qualified = []
    gain = None
    all_muted = True
    for number, line in enumerate(lines, 1):
        if not line.startswith(PREFIX):
            continue
        try:
            row = json.loads(line[len(PREFIX):])
            event = row["event"]
            if event == "gain":
                gain = row["value"]
                if type(gain) not in (int, float) or not 0 <= gain <= 1:
                    raise ValueError("invalid master gain")
                all_muted &= gain == 0
            elif event == "start":
                voice, frames, channels = row["voice"], row["frames"], row["channels"]
                count, peak, looped = row["nonzero_samples"], row["peak"], row["looped"]
                if any(type(item) is not int for item in (voice, frames, channels, count, peak)):
                    raise ValueError("invalid source counter type")
                if (voice <= 0 or voice in starts or frames <= 0 or channels not in (1, 2)
                        or not 0 <= count <= frames * channels or not 0 <= peak <= 32768
                        or bool(count) != bool(peak) or type(looped) is not bool):
                    raise ValueError("invalid or duplicate source start")
                starts[voice] = {**row, "muted_on_start": gain == 0}
            elif event == "complete":
                voice, observed = row["voice"], row["frames_observed"]
                if type(voice) is not int or type(observed) is not int or observed < 0:
                    raise ValueError("invalid source completion")
                if voice in completed or voice not in starts:
                    raise ValueError("completion has no unique start")
                completed.add(voice)
                start = starts[voice]
                if not start["looped"] and observed > start["frames"]:
                    raise ValueError("finite source advanced past its submitted frames")
                if not start["looped"] and start["nonzero_samples"] and observed:
                    qualified.append({**start, "frames_observed": observed})
            else:
                raise ValueError("unknown XAudio2 event")
        except (KeyError, TypeError, ValueError) as error:
            raise ValueError(f"Line {number}: {error}") from error
    return {"sources_started": len(starts), "queues_completed": len(completed),
            "muted_verified": bool(starts) and all_muted and all(row["muted_on_start"] for row in starts.values()),
            "nonzero_finite_sources_completed": qualified}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--require-muted", action="store_true")
    args = parser.parse_args()
    try:
        with args.trace.open(encoding="utf-8-sig") as lines:
            result = summarize(lines)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    print(json.dumps(result, indent=2))
    passed = bool(result["nonzero_finite_sources_completed"]) and (not args.require_muted or result["muted_verified"])
    return 0 if passed else 2


if __name__ == "__main__":
    raise SystemExit(main())
