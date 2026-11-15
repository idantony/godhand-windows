"""Rebase a pinned God Hand text overlay onto another frozen source snapshot."""

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
import tempfile

SCRATCH_ROOT = Path(r"G:\middle_thing\godhand_windows").resolve()


class RebaseError(Exception):
    pass


def digest(data):
    return hashlib.sha256(data).hexdigest()


def checked(path, expected):
    if not isinstance(expected, str) or not re.fullmatch(r"[a-f0-9]{64}", expected):
        raise RebaseError(f"Missing or invalid SHA-256 for {path}")
    data = path.read_bytes()
    actual = digest(data)
    if actual != expected:
        raise RebaseError(f"SHA-256 mismatch for {path}: expected {expected}, read {actual}")
    return data


def pinned_json(path, sha256):
    data = json.loads(checked(path, sha256))
    if not isinstance(data, dict):
        raise RebaseError(f"Expected JSON object at {path}")
    return data


def source_path(root, name):
    if not isinstance(name, str) or any(c in name for c in "\\:\x00\n\r"):
        raise RebaseError(f"Invalid relative source path {name!r}")
    path = PurePosixPath(name)
    if (path.is_absolute() or path.as_posix() != name or ".." in path.parts or
        len(path.parts) < 2 or path.parts[0] not in {"analysis", "docs", "skill", "src", "tests", "tools"} or
        path.suffix not in {".c", ".cpp", ".h", ".java", ".json", ".md", ".ps1", ".py"} or
        name == "analysis/source-snapshot.json"):
        raise RebaseError(f"Unsupported text overlay path {name!r}")
    resolved = (root / path).resolve()
    if root not in resolved.parents:
        raise RebaseError(f"Source path escapes its root: {name}")
    return resolved


def load_snapshot(receipt, sha256):
    manifest = pinned_json(receipt, sha256)
    root = receipt.parent.parent
    if receipt != root / "analysis/source-snapshot.json" or Path(manifest["snapshot"]).resolve() != root:
        raise RebaseError(f"Snapshot receipt does not identify its containing source tree: {receipt}")
    if not isinstance(manifest["source_sha256"], dict):
        raise RebaseError(f"Missing source fingerprints in {receipt}")
    return root, manifest


def normalize(data):
    text = data.replace(b"\r\n", b"\n")
    if b"\r" in text or b"\0" in text:
        raise RebaseError("Expected UTF-8 text with LF or CRLF")
    text.decode("utf-8")
    return text


def merge_text(base, target, overlay, temporary, name):
    old, current, new = (normalize(data) for data in (base, target, overlay))
    if target == base:
        return overlay, "carried"
    if current == new:
        return target, "already-applied"
    if current == old:
        merged = new
    else:
        paths = [temporary / file for file in ("target", "base", "overlay")]
        for path, data in zip(paths, (current, old, new)):
            path.write_bytes(data)
        result = subprocess.run(["git", "merge-file", "-q", "-p", *paths], capture_output=True)
        if result.returncode:
            if 0 < result.returncode < 128:
                raise RebaseError(f"Overlapping edits in {name}")
            raise RebaseError(f"git merge-file failed for {name}: exit {result.returncode}")
        merged = result.stdout
    if merged == current:
        return target, "already-applied"
    # For mixed files use the prevailing target convention, as the manual
    # integration does. Binary fingerprints above always use original bytes.
    crlf = target.count(b"\r\n")
    if crlf > target.count(b"\n") - crlf:
        merged = merged.replace(b"\n", b"\r\n")
    return merged, "merged"


def merge_tokens(base, target, overlay, substitutions, name):
    """Replay unique replacements which reproduce the entire pinned overlay."""
    original, current, desired = map(normalize, (base, target, overlay))
    if not isinstance(substitutions, list) or not substitutions:
        raise RebaseError(f"Missing token replacements: {name}")
    replacements = []
    for pair in substitutions:
        if (not isinstance(pair, list) or len(pair) != 2 or
            not all(isinstance(value, str) for value in pair)):
            raise RebaseError(f"Invalid token replacement: {name}")
        before, after = (normalize(value.encode("utf-8")) for value in pair)
        if not before or before == after or original.count(before) != 1:
            raise RebaseError(f"Token predecessor is absent or ambiguous in base: {name}")
        original = original.replace(before, after, 1)
        replacements.append((before, after))
    if original != desired:
        raise RebaseError(f"Token replacements do not reproduce the pinned overlay: {name}")
    if current == desired:
        return target, "already-applied"
    for before, after in replacements:
        if current.count(before) != 1:
            raise RebaseError(f"Token predecessor is absent or ambiguous in target: {name}")
        current = current.replace(before, after, 1)
    crlf = target.count(b"\r\n")
    if crlf > target.count(b"\n") - crlf:
        current = current.replace(b"\n", b"\r\n")
    return current, "merged-tokens"


def worker_receipt(record, receipt, source_base=None):
    """Normalize worker file records against a pinned source snapshot."""
    if set(record) - {"schema", "base", "files", "dependencies", "evidence", "proof", "verification", "scope", "history", "integration"}:
        raise RebaseError(f"Unsupported worker overlay fields: {receipt}")
    base = record.get("base")
    if source_base is not None:
        if base is not None:
            raise RebaseError(f"Worker overlay already identifies its base: {receipt}")
        base = {"path": str(source_base[0]), "sha256": source_base[1]}
    if not isinstance(base, dict) or not isinstance(base.get("path"), str) or not isinstance(record.get("files"), list):
        raise RebaseError(f"Missing worker base or file records: {receipt}")
    base_receipt = Path(base["path"]).resolve()
    if SCRATCH_ROOT not in base_receipt.parents:
        raise RebaseError(f"Worker base receipt escaped scratch: {base_receipt}")
    root = base_receipt.parent.parent
    output = {"schema": record.get("schema"), "base_receipt": str(base_receipt),
              "base_receipt_sha256": base.get("sha256"), "files": [], "supersedes": {}}
    if "dependencies" in record:
        output["dependencies"] = record["dependencies"]
    for item in record["files"]:
        if not isinstance(item, dict):
            raise RebaseError(f"Invalid worker file record: {receipt}")
        name = item.get("relative")
        expected = source_path(receipt.parent, name)
        path = item.get("path")
        if (path is not None or source_base is None) and (not isinstance(path, str) or Path(path).resolve() != expected):
            raise RebaseError(f"Worker source path differs from its relative name: {name}")
        # Missing predecessor is only valid for a file absent in the pinned base.
        predecessor = item.get("supersedes")
        if predecessor is not None:
            if not isinstance(predecessor, dict) or not isinstance(predecessor.get("sha256"), str):
                raise RebaseError(f"Invalid worker source predecessor: {name}")
            path = predecessor.get("path")
            if (path is not None or source_base is None) and (not isinstance(path, str) or Path(path).resolve() != source_path(root, name)):
                raise RebaseError(f"Invalid worker source predecessor: {name}")
            output["supersedes"][name] = predecessor["sha256"]
        output["files"].append({"path": name, "sha256": item.get("sha256")})
    return output


def checked_dependencies(record, receipt, field="dependencies"):
    rows = record.get(field, [])
    if not isinstance(rows, list):
        raise RebaseError(f"Invalid pinned dependencies in {receipt}")
    seen = set()
    for row in rows:
        if not isinstance(row, dict) or not isinstance(row.get("path"), str):
            raise RebaseError(f"Invalid pinned dependency in {receipt}")
        source = Path(row["path"])
        path = source.resolve()
        if not source.is_absolute() or SCRATCH_ROOT not in path.parents or path in seen:
            raise RebaseError(f"Invalid or duplicate scratch dependency: {source}")
        checked(path, row.get("sha256"))
        seen.add(path)


def load_overlay(receipt, sha256, source_base=None):
    record = pinned_json(receipt, sha256)
    if "base" in record or source_base is not None:
        record = worker_receipt(record, receipt, source_base)
    supported = {
        "schema", "files", "base_receipt", "base_receipt_sha256", "supersedes",
        "dependency_files", "requires_overlay_receipt", "requires_overlay_receipt_sha256",
        "input_overlay_receipt", "input_overlay_receipt_sha256", "rebase_results", "verification", "scope",
        "dependencies", "merge", "bounds", "patches", "rebased_overlay_receipts", "replay_dependencies",
    }
    if record.get("schema") != 1 or set(record) - supported:
        raise RebaseError(f"Unsupported overlay receipt format: {receipt}")
    predecessors = record.get("supersedes", {})
    if (not isinstance(record.get("files"), list) or not isinstance(predecessors, dict) or
        not isinstance(record.get("dependency_files", {}), dict)):
        raise RebaseError(f"Invalid file manifest in {receipt}")
    checked_dependencies(record, receipt)
    checked_dependencies(record, receipt, "patches")
    checked_dependencies(record, receipt, "rebased_overlay_receipts")
    replay_dependencies = record.get("replay_dependencies", {})
    if not isinstance(replay_dependencies, dict):
        raise RebaseError(f"Invalid pinned replay dependencies in {receipt}")
    checked_dependencies({"dependencies": [{"path": path, "sha256": pin}
                                            for path, pin in replay_dependencies.items()]}, receipt)
    old_receipt = Path(record["base_receipt"]).resolve()
    base, manifest = load_snapshot(old_receipt, record["base_receipt_sha256"])
    files = {}
    for item in record["files"]:
        name, pin = item["path"], item["sha256"]
        if name in files:
            raise RebaseError(f"Duplicate overlay entry: {name}")
        original = snapshot_file(base, manifest, name)
        predecessor = item.get("base_sha256", predecessors.get(name))
        if name in predecessors and predecessor != predecessors[name]:
            raise RebaseError(f"Conflicting overlay predecessors: {name}")
        if (digest(original) if original is not None else None) != predecessor:
            raise RebaseError(f"Overlay predecessor does not match its pinned base: {name}")
        files[name] = (pin, checked(source_path(receipt.parent, name), pin), original)
    if set(predecessors) - set(files):
        raise RebaseError(f"Unmatched predecessor fingerprint in {receipt}")
    if ("requires_overlay_receipt" in record) != ("requires_overlay_receipt_sha256" in record):
        raise RebaseError(f"Incomplete required overlay pin in {receipt}")
    return record, files, base, manifest


def snapshot_file(root, manifest, name):
    path = source_path(root, name)
    pin = manifest["source_sha256"].get(name)
    if pin is None:
        if path.exists():
            raise RebaseError(f"File present without snapshot fingerprint: {path}")
        return None
    return checked(path, pin)


def require_dependencies(record, target, manifest):
    for name, expected in record.get("dependency_files", {}).items():
        actual = snapshot_file(target, manifest, name)
        if actual is None or digest(actual) != expected:
            raise RebaseError(f"Target dependency differs from required SHA-256: {name}")


def check_required_overlay(record, target, manifest, seen, protected):
    require_dependencies(record, target, manifest)
    if "requires_overlay_receipt" not in record:
        return
    required = Path(record["requires_overlay_receipt"]).resolve()
    if required in seen:
        raise RebaseError(f"Cycle among overlay receipts: {required}")
    seen.add(required)
    dependency, files, base, _ = load_overlay(required, record["requires_overlay_receipt_sha256"])
    protected.update((required.parent, base))
    for name, (expected, _, _) in files.items():
        # Analysis attachments are pinned in the required overlay itself.
        # Frozen source manifests omit these private attachments.
        if name.startswith("analysis/") and name not in manifest["source_sha256"]:
            path = source_path(target, name)
            if path.exists():
                checked(path, expected)
            continue
        actual = snapshot_file(target, manifest, name)
        if actual is None or digest(actual) != expected:
            raise RebaseError(f"Target is missing exact required overlay file: {name}")
    check_required_overlay(dependency, target, manifest, seen, protected)


def rebase(overlay_receipt, overlay_sha256, target_receipt, target_sha256, output, *,
           tokens_path=None, tokens_sha256=None, source_base=None):
    overlay_receipt = overlay_receipt.resolve()
    target_receipt = target_receipt.resolve()
    output = output.resolve()
    if SCRATCH_ROOT not in output.parents:
        raise RebaseError(f"Output must be below the project scratch: {output}")
    record, files, base, old_manifest = load_overlay(overlay_receipt, overlay_sha256, source_base)
    if (tokens_path is None) != (tokens_sha256 is None):
        raise RebaseError("Token recipe path and SHA-256 must be supplied together")
    tokens = {}
    if tokens_path is not None:
        tokens_path = tokens_path.resolve()
        if overlay_receipt.parent not in tokens_path.parents or SCRATCH_ROOT not in tokens_path.parents:
            raise RebaseError("Token recipe must be inside the scratch overlay")
        tokens = pinned_json(tokens_path, tokens_sha256)
        if not tokens or set(tokens) - set(files):
            raise RebaseError("Token recipe must identify files in the pinned overlay")
    target, target_manifest = load_snapshot(target_receipt, target_sha256)
    project = Path(target_manifest["source_project"]).resolve()
    if Path(old_manifest["source_project"]).resolve() != project:
        raise RebaseError("The base and target are snapshots of different source projects")
    protected = {base, target, project, overlay_receipt.parent}
    check_required_overlay(record, target, target_manifest, {overlay_receipt}, protected)
    if output.exists() or any(root == output or root in output.parents for root in protected):
        raise RebaseError(f"Output must be a new directory outside protected trees: {output}")

    result = {
        "schema": 1,
        "base_receipt": str(target_receipt),
        "base_receipt_sha256": target_sha256,
        "input_overlay_receipt": str(overlay_receipt),
        "input_overlay_receipt_sha256": overlay_sha256,
        "supersedes": {},
        "files": [],
        "rebase_results": {},
    }
    for key in ("dependencies", "patches", "dependency_files", "requires_overlay_receipt",
                "requires_overlay_receipt_sha256", "rebased_overlay_receipts", "replay_dependencies"):
        if key in record:
            result[key] = record[key]
    if source_base is not None:
        dependencies = result.setdefault("dependencies", [])
        if all(Path(row["path"]).resolve() != base / "analysis/source-snapshot.json" for row in dependencies):
            dependencies.append({"path": record["base_receipt"], "sha256": record["base_receipt_sha256"]})

    with tempfile.TemporaryDirectory(prefix="rebase-", dir=output.parent) as directory:
        temporary = Path(directory).resolve()
        stage = temporary / "result"
        stage.mkdir()
        for name, (overlay_pin, overlay, base_bytes) in files.items():
            target_bytes = snapshot_file(target, target_manifest, name)
            if base_bytes is None:
                if name in tokens:
                    raise RebaseError(f"Token recipe cannot replace a new file: {name}")
                normalize(overlay)
                if target_bytes is not None and digest(target_bytes) != overlay_pin:
                    raise RebaseError(f"Both sides added {name}")
                merged, status = (overlay, "added") if target_bytes is None else (target_bytes, "already-applied")
            else:
                if target_bytes is None:
                    raise RebaseError(f"Target removed the overlay predecessor: {name}")
                if name in tokens:
                    merged, status = merge_tokens(base_bytes, target_bytes, overlay, tokens[name], name)
                else:
                    merged, status = merge_text(base_bytes, target_bytes, overlay, temporary, name)
            result["rebase_results"][name] = {"status": status, "sha256": digest(merged)}
            if merged == target_bytes:
                continue
            destination = source_path(stage, name)
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(merged)
            result["files"].append({"path": name, "sha256": digest(merged)})
            if target_bytes is not None:
                result["supersedes"][name] = digest(target_bytes)

        (stage / "receipt.json").write_bytes((json.dumps(result, indent=2) + "\n").encode("utf-8"))
        stage.rename(output)
    return result


def collect(base_receipt, base_sha256, source_receipt, source_sha256, output):
    """Collect the authored differences between two pinned local snapshots."""
    base_receipt, source_receipt = base_receipt.resolve(), source_receipt.resolve()
    output = output.resolve()
    if any(SCRATCH_ROOT not in path.parents for path in (base_receipt, source_receipt, output)):
        raise RebaseError("Collection requires snapshot receipts and output in project scratch")
    base, before = load_snapshot(base_receipt, base_sha256)
    source, after = load_snapshot(source_receipt, source_sha256)
    project = Path(before["source_project"]).resolve()
    if Path(after["source_project"]).resolve() != project:
        raise RebaseError("The snapshots belong to different source projects")
    if output.exists() or any(root == output or root in output.parents for root in (base, source, project)):
        raise RebaseError(f"Output must be a new directory outside protected trees: {output}")
    previous, current = before["source_sha256"], after["source_sha256"]
    removed = previous.keys() - current.keys()
    if removed:
        raise RebaseError(f"Cannot collect source deletions: {', '.join(sorted(removed))}")

    result = {
        "schema": 1,
        "base_receipt": str(base_receipt),
        "base_receipt_sha256": base_sha256,
        "dependencies": [{"path": str(source_receipt), "sha256": source_sha256}],
        "files": [],
    }
    with tempfile.TemporaryDirectory(prefix="collect-", dir=output.parent) as directory:
        stage = Path(directory) / "result"
        stage.mkdir()
        for name, expected in sorted(current.items()):
            if previous.get(name) == expected:
                continue
            previous_bytes = snapshot_file(base, before, name)
            content = checked(source_path(source, name), expected)
            normalize(content)
            destination = source_path(stage, name)
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(content)
            result["files"].append({"path": name, "sha256": expected, "bytes": len(content),
                                    "base_sha256": digest(previous_bytes) if previous_bytes is not None else None})
        (stage / "receipt.json").write_bytes((json.dumps(result, indent=2) + "\n").encode("utf-8"))
        checked(base_receipt, base_sha256)
        checked(source_receipt, source_sha256)
        stage.rename(output)
    return result


def collect_main(argv):
    parser = argparse.ArgumentParser(description=collect.__doc__)
    parser.add_argument("--base", required=True, type=Path)
    parser.add_argument("--base-sha256", required=True)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--source-sha256", required=True)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        result = collect(args.base, args.base_sha256, args.source, args.source_sha256, args.output)
    except (KeyError, OSError, RebaseError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    print(f"Collected {len(result['files'])} file(s) in {args.output / 'receipt.json'}")


def main():
    if sys.argv[1:2] == ["collect"]:
        return collect_main(sys.argv[2:])
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--overlay", required=True, type=Path)
    parser.add_argument("--overlay-sha256", required=True)
    parser.add_argument("--target", required=True, type=Path)
    parser.add_argument("--target-sha256", required=True)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--source-base", nargs=2, metavar=("RECEIPT", "SHA256"),
                        help="pin the original base for a relative worker receipt without a base")
    parser.add_argument("--tokens", type=Path)
    parser.add_argument("--tokens-sha256")
    args = parser.parse_args()
    try:
        result = rebase(args.overlay, args.overlay_sha256, args.target, args.target_sha256, args.output,
                        tokens_path=args.tokens, tokens_sha256=args.tokens_sha256, source_base=args.source_base)
    except (KeyError, OSError, RebaseError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    print(f"Wrote {len(result['files'])} file(s) to {args.output / 'receipt.json'}")


if __name__ == "__main__":
    main()
