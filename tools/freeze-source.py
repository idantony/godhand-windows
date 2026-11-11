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


def dependency_records(value):
    """Accept the original digest map or worker file/size records."""
    if isinstance(value, dict):
        rows = [{"path": path, "sha256": sha} for path, sha in value.items()]
    elif isinstance(value, list):
        rows = value
    else:
        raise ValueError("dependency_files must be a digest map or file records")
    result = []
    seen = set()
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError("Invalid dependency file record")
        name, sha, size = row.get("path"), row.get("sha256"), row.get("bytes")
        if (not isinstance(name, str) or not name or name in seen
                or not isinstance(sha, str) or len(sha) != 64
                or any(c not in "0123456789abcdef" for c in sha)
                or ("bytes" in row and (type(size) is not int or size < 0))):
            raise ValueError("Invalid or duplicate dependency file record")
        seen.add(name)
        result.append((name, sha, size))
    return result


def overlay_receipt(value):
    """Read worker digest pairs without rewriting their frozen receipt."""
    if isinstance(value, dict) and value.get("schema") == 1 and isinstance(value.get("files"), list):
        entries = value["files"]
        if not any(isinstance(entry, dict) and "relative" in entry for entry in entries):
            return value, {}
        rows = []
        for entry in entries:
            if not isinstance(entry, dict) or "supersedes" not in entry:
                raise ValueError("Missing worker overlay predecessor")
            previous = entry["supersedes"]
            if previous is not None and (not isinstance(previous, dict) or not isinstance(previous.get("sha256"), str)):
                raise ValueError("Invalid worker overlay predecessor")
            rows.append({"path": entry.get("relative"), "overlay": entry.get("sha256"),
                         "base": None if previous is None else previous["sha256"]})
            if "bytes" in entry:
                rows[-1]["bytes"] = entry["bytes"]
    elif isinstance(value, list):
        rows = value
    elif isinstance(value, dict) and isinstance(value.get("files"), dict):
        rows = []
        for name, pair in value["files"].items():
            if not isinstance(pair, dict) or "base_sha256" not in pair:
                raise ValueError("Invalid overlay hash pair")
            rows.append({"path": name, "base": pair.get("base_sha256"), "overlay": pair.get("overlay_sha256")})
    else:
        raise ValueError("Unsupported overlay bundle receipt")
    files, predecessors = [], {}
    for row in rows:
        if not isinstance(row, dict) or "base" not in row:
            raise ValueError("Invalid overlay hash pair")
        name, old, new = row.get("path"), row.get("base"), row.get("overlay")
        if not isinstance(name, str) or not name or name in predecessors:
            raise ValueError("Invalid or duplicate overlay file path")
        for digest_value in ((new,) if old is None else (old, new)):
            if (not isinstance(digest_value, str) or len(digest_value) != 64
                    or any(char not in "0123456789abcdefABCDEF" for char in digest_value)):
                raise ValueError("Invalid predecessor or replacement SHA-256 for " + name)
        predecessors[name] = None if old is None else old.lower()
        files.append({"path": name, "sha256": new.lower()})
        if "bytes" in row:
            if type(row["bytes"]) is not int or row["bytes"] < 0:
                raise ValueError("Invalid overlay byte count for " + name)
            files[-1]["bytes"] = row["bytes"]
    dependencies = value.get("dependency_files", {}) if isinstance(value, dict) else {}
    return {"schema": 1, "files": files, "dependency_files": dependencies}, predecessors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--destination", type=Path, required=True)
    parser.add_argument("--base-snapshot", type=Path, help="Reuse exact authored inputs from an immutable snapshot")
    parser.add_argument("--overlay", action="append", default=[], help="Explicit authored E: path replacing/adding a base input; repeatable")
    parser.add_argument("--overlay-bundle", type=Path, action="append", default=[],
                        help="Immutable G: delta directory with receipt.json or sha256.json; repeatable")
    parser.add_argument("--rebase-dependency", nargs=3, metavar=("BUNDLE", "SOURCE", "FINAL_SHA256"),
                        action="append", default=[],
                        help="Pin a reviewed replacement for one bundle dependency in a new, untested composition")
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
    rebases = {}
    for bundle_name, source_name, sha in args.rebase_dependency:
        key = (bundle_name, source_name)
        if key in rebases or len(sha) != 64 or any(c not in "0123456789abcdef" for c in sha):
            parser.error("Duplicate or invalid dependency rebase: " + str(key))
        rebases[key] = sha
    bundles, dependencies, rebased = [], [], []
    for value in args.overlay_bundle:
        bundle = value.resolve()
        if SCRATCH not in bundle.parents:
            parser.error("Overlay bundle must be below the project's G: scratch")
        manifest_path = bundle / "receipt.json"
        if not manifest_path.is_file():
            manifest_path = bundle / "sha256.json"
        manifest_path = manifest_path.resolve()
        if bundle not in manifest_path.parents or not manifest_path.is_file():
            parser.error("Missing overlay receipt within " + str(bundle))
        manifest_bytes = manifest_path.read_bytes()
        manifest_sha256 = hashlib.sha256(manifest_bytes).hexdigest()
        frozen_files[manifest_path] = manifest_sha256
        try:
            manifest, predecessors = overlay_receipt(json.loads(manifest_bytes.decode("utf-8-sig")))
            dependencies.extend((bundle.name, *row) for row in dependency_records(manifest.get("dependency_files", {})))
        except ValueError as error:
            parser.error(str(error) + ": " + str(manifest_path))
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
            if name in predecessors:
                previous = paths.get(name)
                expected = predecessors[name]
                actual = digest(previous) if previous is not None and previous.is_file() else None
                if actual != expected:
                    raise SystemExit("Frozen overlay predecessor does not match current source: " + bundle.name + ": " + name)
                if expected is not None:
                    frozen_files[previous] = expected
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
        bundles.append({"directory": str(bundle), "receipt_file": manifest_path.name, "receipt_sha256": manifest_sha256,
                        "files": sorted(authored), "private_evidence_retained_in_bundle": private})
    names = sorted(paths)
    for bundle_name, name, expected, size in dependencies:
        if name not in paths:
            raise SystemExit("Frozen overlay dependency is missing: " + name)
        source = paths[name]
        actual = digest(source)
        if actual != expected or (size is not None and source.stat().st_size != size):
            if rebases.pop((bundle_name, name), None) != actual:
                raise SystemExit("Frozen overlay dependency does not match final source: " + bundle_name + ": " + name)
            rebased.append({"bundle": bundle_name, "path": name, "old_sha256": expected, "new_sha256": actual})
        frozen_files[source] = actual
    if rebases:
        parser.error("Unused dependency rebases: " + str(sorted(rebases)))
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
    if rebased:
        receipt["rebased_dependencies"] = rebased
    (analysis / "source-snapshot.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(str(destination))


if __name__ == "__main__":
    main()
