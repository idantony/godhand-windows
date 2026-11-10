"""Copy authored source into an immutable G: verification workspace.

Workers can continue editing E: while a complete build/tests run on this copy.
The receipt identifies exact bytes; this is not a verification result itself.
Original game data/tool installations are borrowed through read-only usage.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

PROJECT = Path(__file__).resolve().parents[1]
SCRATCH = Path(r"G:\middle_thing\godhand_windows").resolve()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--destination", type=Path, required=True)
    args = parser.parse_args()
    destination = args.destination.resolve()
    if SCRATCH not in destination.parents or destination.exists():
        parser.error("Destination must be a new directory below the project's G: scratch")
    names = subprocess.check_output(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"], cwd=PROJECT
    ).decode("utf-8").split("\0")
    names = sorted({name for name in names if name and not name.startswith(("reference/", ".tools/"))
                    and (PROJECT / name).is_file()})
    destination.mkdir(parents=True)
    for attempt in range(3):
        before = {name: digest(PROJECT / name) for name in names}
        for name in names:
            target = destination / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(PROJECT / name, target)
        after = {name: digest(PROJECT / name) for name in names}
        copied = {name: digest(destination / name) for name in names}
        if before == after == copied:
            break
    else:
        raise SystemExit("Sources kept changing during copy; keep this failed scratch and use a new destination")
    analysis = destination / "analysis"
    analysis.mkdir(exist_ok=True)
    shutil.copyfile(PROJECT / "analysis/disc-baseline.json", analysis / "disc-baseline.json")
    # Constant script plus argument array: paths are not composed as shell code.
    for name in (".tools", "reference"):
        subprocess.run(["powershell", "-NoProfile", "-Command",
            "& { param($linkPath,$targetPath) New-Item -ItemType Junction -Path $linkPath -Target $targetPath -ErrorAction Stop | Out-Null }",
            str(destination / name), str(PROJECT / name)], check=True)
        if (destination / name).resolve() != (PROJECT / name).resolve():
            raise SystemExit("Borrowed junction did not resolve to its recorded source")
    receipt = {"created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "source_project": str(PROJECT), "snapshot": str(destination),
        "source_sha256": copied, "game_data_and_tools": "Borrowed from E:, never copied into authored source",
        "status": "copied-exact-bytes; not-yet-built-or-tested"}
    (analysis / "source-snapshot.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(str(destination))


if __name__ == "__main__":
    main()
