"""Recheck a recorded native window capture without launching the replay."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
from runpy import run_path


CHECKPOINTS = run_path(str(Path(__file__).with_name("check-replay-checkpoints.py")))


def digest(path):
    hasher = sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def read_pinned(path, expected):
    actual = digest(path)
    if actual != expected.lower():
        raise ValueError(f"hash changed for {path.name}: {actual}")
    return path


def contained_artifact(directory, name):
    if Path(name).name != name:
        raise ValueError(f"expected an artifact basename: {name}")
    path = (directory / name).resolve(strict=True)
    if path.parent != directory:
        raise ValueError(f"artifact points outside the receipt directory: {name}")
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path)
    parser.add_argument("--receipt-sha256", required=True)
    parser.add_argument("--capture", required=True, help="name of the raw RGBA window readback")
    parser.add_argument("--width", type=int, required=True)
    parser.add_argument("--height", type=int, required=True)
    parser.add_argument("--expect", type=CHECKPOINTS["checkpoint"], action="append", required=True)
    parser.add_argument("--check-inputs", action="store_true",
                        help="also recheck files recorded in inputs.json")
    args = parser.parse_args()
    if args.width <= 0 or args.height <= 0:
        parser.error("capture width and height must be positive")

    try:
        run = args.run.resolve(strict=True)
        receipt = read_pinned(contained_artifact(run, "process.json"), args.receipt_sha256)
        report = json.loads(receipt.read_bytes())
        artifacts = {name: read_pinned(contained_artifact(run, name), expected)
                     for name, expected in report["outputs"].items()}
        if report.get("mismatch") is not None:
            raise ValueError("recorded input prefix mismatch")
        capture = artifacts[args.capture]
        if capture.stat().st_size != args.width * args.height * 4:
            raise ValueError("capture size differs from the requested RGBA dimensions")
        with artifacts["replay.stderr.log"].open("rb") as trace:
            CHECKPOINTS["check"](trace, args.expect)
        input_count = 0
        if args.check_inputs:
            manifest = json.loads(artifacts["inputs.json"].read_bytes())["sha256"]
            for name, expected in manifest.items():
                read_pinned(Path(name), expected)
            input_count = len(manifest)
    except (OSError, ValueError, KeyError, AttributeError, TypeError) as error:
        parser.error(str(error))

    print(json.dumps({
        "capture": capture.name,
        "capture_sha256": report["outputs"][capture.name],
        "dimensions": [args.width, args.height],
        "archived_prefix_matches": report.get("prefix_matches", {}),
        "recipe_input_files_rechecked": input_count,
        "verified_artifacts": len(artifacts),
    }, indent=2))


if __name__ == "__main__":
    main()
