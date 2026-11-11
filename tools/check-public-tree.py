"""Check staged repository contents before a public progress push."""
import json
from pathlib import PurePosixPath
import re
import subprocess

ROOT_FILES = {".gitattributes", ".gitignore", "AGENTS.md", "README.md", "LICENSE", "NOTICE.md"}
PREFIXES = {"src", "tests", "tools", "docs", "skill"}
BINARY_SUFFIXES = {".iso", ".elf", ".rel", ".afs", ".fst", ".exe", ".dll", ".obj", ".o", ".pdb", ".zip", ".tgz", ".pyc", ".class", ".img", ".ims", ".png", ".jpg", ".bmp", ".rgba", ".tm2", ".rom", ".bin", ".lib"}
SECRETS = re.compile(rb"(?:github_pat_[A-Za-z0-9_]{20,}|gh[pousr]_[A-Za-z0-9]{20,}|sk-[A-Za-z0-9]{32,})")


def staged_entries():
    output = subprocess.check_output(["git", "ls-files", "--stage", "-z"])
    for entry in output.split(b"\0"):
        if entry:
            metadata, name = entry.split(b"\t", 1)
            _, object_id, stage = metadata.split()
            yield name.decode("utf-8"), object_id, stage


def read_blob(batch, object_id):
    batch.stdin.write(object_id + b"\n")
    batch.stdin.flush()
    header = batch.stdout.readline().split()
    if len(header) != 3 or header[0] != object_id or header[1] != b"blob":
        raise ValueError("indexed object is missing or is not a blob")
    size = int(header[2])
    if size < 0:
        raise ValueError("invalid blob size")
    content = batch.stdout.read(size)
    if len(content) != size or batch.stdout.read(1) != b"\n":
        raise ValueError("incomplete indexed blob")
    return content


def main():
    entries = list(staged_entries())
    failures, checked, total = [], 0, 0
    with subprocess.Popen(["git", "cat-file", "--batch"], stdin=subprocess.PIPE,
                          stdout=subprocess.PIPE) as batch:
        for name, object_id, stage in entries:
            path = PurePosixPath(name)
            if not (name in ROOT_FILES or name == "reference/.gitkeep" or path.parts[0] in PREFIXES):
                failures.append({"file": name, "reason": "outside published source allowlist"})
                continue
            if path.suffix.lower() in BINARY_SUFFIXES or name.endswith("_evidence.json") or name.endswith("/validation.json"):
                failures.append({"file": name, "reason": "original/intermediate/private evidence file"})
                continue
            if stage != b"0":
                failures.append({"file": name, "reason": "unmerged index entry"})
                continue
            try:
                content = read_blob(batch, object_id)
            except (OSError, ValueError):
                failures.append({"file": name, "reason": "could not read indexed blob"})
                # A broken response can leave the stream out of sync.
                break
            checked += 1
            total += len(content)
            if len(content) > 1024 * 1024 or b"\0" in content:
                failures.append({"file": name, "reason": "binary or oversized content"})
            if SECRETS.search(content):
                failures.append({"file": name, "reason": "credential-like content; value deliberately withheld"})
        try:
            batch.stdin.close()
        except BrokenPipeError:
            if not failures:
                failures.append({"file": "", "reason": "could not read indexed blobs"})
        batch.stdout.close()
        if batch.wait() and not failures:
            failures.append({"file": "", "reason": "could not read indexed blobs"})
    print(json.dumps({"status": "failed" if failures else "passed", "files": checked, "bytes": total, "failures": failures}))
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
