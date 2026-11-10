"""Remove only reported terminal blank lines from staged text blobs.

The working tree and immutable verification copies are never changed. Other
whitespace failures are rejected before any index update. Default is a dry run.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import uuid


def git(root, *args, data=None, env=None):
    return subprocess.run(["git", "-c", "core.quotepath=false", *args], cwd=root,
                          input=data, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)


def checked(root, *args, data=None, env=None):
    result = git(root, *args, data=data, env=env)
    if result.returncode:
        raise RuntimeError(result.stderr.decode("utf-8", errors="replace"))
    return result.stdout


def replace_index(index_path, expected, replacement):
    """Honor Git's lock and reject concurrent staging before atomic replacement."""
    lock = index_path.with_name(index_path.name + ".lock")
    acquired = False
    try:
        with lock.open("xb") as output:
            acquired = True
            if index_path.read_bytes() != expected:
                raise RuntimeError("Index changed before commit; no normalization applied")
            output.write(replacement)
            output.flush()
            os.fsync(output.fileno())
        os.replace(lock, index_path)
    finally:
        if acquired and lock.exists():
            lock.unlink()


def normalize(root, apply=False):
    root = Path(checked(root, "rev-parse", "--show-toplevel").decode().strip()).resolve()
    index_name = checked(root, "rev-parse", "--git-path", "index").decode().strip()
    index_path = Path(index_name)
    if not index_path.is_absolute():
        index_path = root / index_path
    before_index = index_path.read_bytes()
    result = git(root, "diff", "--cached", "--check")
    if result.returncode not in (0, 1, 2):
        raise RuntimeError(result.stderr.decode("utf-8", errors="replace"))
    if result.returncode and not result.stdout:
        raise RuntimeError(result.stderr.decode("utf-8", errors="replace"))
    names = []
    for line in result.stdout.decode("utf-8").splitlines():
        match = re.fullmatch(r"(.+):\d+: new blank line at EOF\.", line)
        if not match:
            raise ValueError("Refusing non-EOF whitespace error: " + line)
        names.append(match.group(1))
    updates, rows = [], []
    for name in sorted(set(names)):
        entry = checked(root, "ls-files", "--stage", "-z", "--", name).split(b"\0")
        entries = [row.split(b"\t", 1) for row in entry if row]
        if len(entries) != 1:
            raise ValueError("Refusing unmerged/ambiguous index path: " + name)
        mode, old_blob, stage = entries[0][0].decode().split()
        if stage != "0" or mode not in ("100644", "100755"):
            raise ValueError("Refusing non-regular staged file: " + name)
        old = checked(root, "cat-file", "blob", old_blob)
        if not old.endswith(b"\n\n"):
            raise ValueError("Reported EOF no longer matches staged bytes: " + name)
        new = old.rstrip(b"\n") + b"\n"
        rows.append({"path": name, "before_sha256": hashlib.sha256(old).hexdigest(),
                     "after_sha256": hashlib.sha256(new).hexdigest(),
                     "terminal_newlines_removed": len(old) - len(new)})
        if apply:
            blob = checked(root, "hash-object", "-w", "--stdin", data=new).decode().strip()
            updates.append(mode + " " + blob + "\t" + name + "\n")
    if index_path.read_bytes() != before_index:
        raise RuntimeError("Index changed during inspection; no normalization applied")
    if apply and updates:
        scratch = Path(r"G:/middle_thing/godhand_windows/tool-tests")
        scratch.mkdir(parents=True, exist_ok=True)
        temporary_index = scratch / ("eof-" + uuid.uuid4().hex + ".index")
        temporary_index.write_bytes(before_index)
        env = dict(os.environ, GIT_INDEX_FILE=str(temporary_index.resolve()))
        checked(root, "update-index", "--index-info", data="".join(updates).encode("utf-8"), env=env)
        replace_index(index_path, before_index, temporary_index.read_bytes())
    return {"status": "normalized" if apply else "dry-run", "files": rows,
            "working_tree_changed": False, "verified_snapshots_changed": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--report", type=Path, help="New private JSON receipt; refuses overwrite")
    args = parser.parse_args()
    if args.report and args.report.exists():
        parser.error("Report exists; choose a new receipt path")
    try:
        result = normalize(args.repository, args.apply)
    except (OSError, ValueError, RuntimeError) as error:
        parser.exit(1, str(error) + "\n")
    result["created_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))


if __name__ == "__main__":
    main()
