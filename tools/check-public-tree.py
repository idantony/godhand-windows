"""Check staged repository contents before a public progress push."""
import json
from pathlib import PurePosixPath
import re
import subprocess

ROOT_FILES = {".gitattributes", ".gitignore", "AGENTS.md", "README.md", "LICENSE", "NOTICE.md"}
PREFIXES = {"src", "tests", "tools", "docs", "skill"}
BINARY_SUFFIXES = {".iso", ".elf", ".rel", ".afs", ".fst", ".exe", ".dll", ".obj", ".o", ".pdb", ".zip", ".tgz", ".pyc", ".class", ".img", ".ims", ".png", ".jpg", ".bmp", ".rgba", ".tm2", ".rom", ".bin", ".lib"}
SECRETS = re.compile(rb"(?:github_pat_[A-Za-z0-9_]{20,}|gh[pousr]_[A-Za-z0-9]{20,}|sk-[A-Za-z0-9]{32,})")


def main():
    entries = subprocess.check_output(["git", "ls-files", "-z"]).decode("utf-8").split("\0")
    failures, checked, total = [], 0, 0
    for name in filter(None, entries):
        path = PurePosixPath(name)
        if not (name in ROOT_FILES or name == "reference/.gitkeep" or path.parts[0] in PREFIXES):
            failures.append({"file": name, "reason": "outside published source allowlist"})
            continue
        if path.suffix.lower() in BINARY_SUFFIXES or name.endswith("_evidence.json") or name.endswith("/validation.json"):
            failures.append({"file": name, "reason": "original/intermediate/private evidence file"})
            continue
        content = subprocess.check_output(["git", "show", ":" + name])
        checked += 1
        total += len(content)
        if len(content) > 1024 * 1024 or b"\0" in content:
            failures.append({"file": name, "reason": "binary or oversized content"})
        if SECRETS.search(content):
            failures.append({"file": name, "reason": "credential-like content; value deliberately withheld"})
    print(json.dumps({"status": "failed" if failures else "passed", "files": checked, "bytes": total, "failures": failures}))
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
