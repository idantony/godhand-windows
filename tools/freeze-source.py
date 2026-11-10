"""Copy authored source into an immutable G: verification workspace.

Workers can continue editing E: while a complete build/tests run on this copy.
The receipt identifies exact bytes; this is not a verification result itself.
Original game data/tool installations are borrowed through read-only usage.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import subprocess

PROJECT = Path(__file__).resolve().parents[1]
SCRATCH = Path(r"G:\middle_thing\godhand_windows").resolve()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--destination", type=Path, required=True)
    parser.add_argument("--base-snapshot", type=Path, help="Reuse exact authored inputs from an immutable snapshot")
    parser.add_argument("--overlay", action="append", default=[], help="Explicit authored E: path replacing/adding a base input; repeatable")
    parser.add_argument("--overlay-bundle", type=Path, action="append", default=[],
                        help="Immutable G: delta directory with receipt.json path/sha256 entries (optional bytes); repeatable")
    args = parser.parse_args()
    destination = args.destination.resolve()
    if SCRATCH not in destination.parents or destination.exists():
        parser.error("Destination must be a new directory below the project's G: scratch")
    frozen_files = {}

    def require_frozen():
        for source, expected in frozen_files.items():
            if not source.is_file() or digest(source) != expected:
                raise SystemExit("Frozen input changed while composing snapshot: " + str(source))

    if args.base_snapshot:
        base = args.base_snapshot.resolve()
        if SCRATCH not in base.parents:
            parser.error("Base snapshot must be below the project's G: scratch")
        base_receipt = base / "analysis/source-snapshot.json"
        base_receipt_bytes = base_receipt.read_bytes()
        base_receipt_sha256 = hashlib.sha256(base_receipt_bytes).hexdigest()
        frozen_files[base_receipt] = base_receipt_sha256
        receipt = json.loads(base_receipt_bytes.decode("utf-8-sig"))
        paths = {}
        for name, expected in receipt["source_sha256"].items():
            relative = PurePosixPath(name)
            source = (base / name).resolve()
            if relative.is_absolute() or ".." in relative.parts or "\\" in name or base not in source.parents:
                parser.error("Base input escaped snapshot: " + name)
            paths[name] = source
            frozen_files[source] = expected
        if any(digest(paths[name]) != expected for name, expected in receipt["source_sha256"].items()):
            raise SystemExit("Base snapshot source was modified; refusing to reuse it")
        for value in args.overlay:
            candidate = (PROJECT / value).resolve()
            try:
                name = candidate.relative_to(PROJECT).as_posix()
            except ValueError:
                parser.error("Overlay escaped project source")
            if name.startswith(("reference/", ".tools/", "analysis/")) or not candidate.is_file():
                parser.error("Overlay must be an existing authored input")
            paths[name] = candidate
    else:
        if args.overlay or args.overlay_bundle:
            parser.error("Overlays require --base-snapshot")
        names = subprocess.check_output(
            ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"], cwd=PROJECT
        ).decode("utf-8").split("\0")
        paths = {name: PROJECT / name for name in names if name and not name.startswith(("reference/", ".tools/"))
                 and (PROJECT / name).is_file()}
    bundles, dependencies = [], []
    for value in args.overlay_bundle:
        bundle = value.resolve()
        if SCRATCH not in bundle.parents:
            parser.error("Overlay bundle must be below the project's G: scratch")
        manifest_path = bundle / "receipt.json"
        manifest_bytes = manifest_path.read_bytes()
        manifest_sha256 = hashlib.sha256(manifest_bytes).hexdigest()
        frozen_files[manifest_path] = manifest_sha256
        manifest = json.loads(manifest_bytes.decode("utf-8-sig"))
        if manifest.get("schema") != 1 or not isinstance(manifest.get("files"), list):
            parser.error("Unsupported overlay bundle receipt")
        dependencies.extend(manifest.get("dependency_files", {}).items())
        seen, authored, private = set(), set(), []
        for entry in manifest["files"]:
            name = entry["path"]
            relative = PurePosixPath(name)
            is_private = bool(relative.parts and relative.parts[0] == "analysis" and relative.suffix == ".json")
            if (relative.is_absolute() or ".." in relative.parts or "\\" in name
                    or not relative.parts or (relative.parts[0] not in ("src", "tests", "tools", "docs", "skill") and not is_private)
                    or relative.suffix not in (".c", ".h", ".cpp", ".py", ".ps1", ".java", ".md", ".json")
                    or name in seen):
                parser.error("Refusing duplicate or non-authored bundle input: " + name)
            source = (bundle / name).resolve()
            if bundle not in source.parents or not source.is_file():
                parser.error("Overlay bundle input escaped its source directory: " + name)
            if ("bytes" in entry and source.stat().st_size != entry["bytes"]) or digest(source) != entry["sha256"]:
                raise SystemExit("Frozen overlay bundle changed: " + name)
            frozen_files[source] = entry["sha256"]
            seen.add(name)
            if is_private:
                # Validate private evidence in its original immutable bundle;
                # never add it to authored source or public staging candidates.
                private.append({"path": name, "sha256": entry["sha256"]})
            else:
                paths[name] = source
                authored.add(name)
        bundles.append({"directory": str(bundle), "receipt_sha256": manifest_sha256,
                        "files": sorted(authored), "private_evidence_retained_in_bundle": private})
    names = sorted(paths)
    for name, expected in dependencies:
        if name not in paths or digest(paths[name]) != expected:
            raise SystemExit("Frozen overlay dependency does not match final source: " + name)
        frozen_files[paths[name]] = expected
    require_frozen()
    destination.mkdir(parents=True)
    for attempt in range(3):
        require_frozen()
        before = {name: digest(paths[name]) for name in names}
        for name in names:
            target = destination / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(paths[name], target)
        after = {name: digest(paths[name]) for name in names}
        copied = {name: digest(destination / name) for name in names}
        require_frozen()
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
    if args.base_snapshot:
        receipt.update(base_snapshot=str(base), base_snapshot_receipt_sha256=base_receipt_sha256,
                       explicit_overlays=args.overlay)
    if bundles:
        receipt["frozen_overlay_bundles"] = bundles
    (analysis / "source-snapshot.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(str(destination))


if __name__ == "__main__":
    main()
