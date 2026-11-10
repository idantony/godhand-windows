"""Stage a passing immutable source copy without overwriting newer worker edits.

Only fingerprinted authored inputs and explicitly named extra authored files are
staged. Git's normal clean filters preserve the repository's line-ending policy.
Private game data, analysis and binaries are never eligible inputs.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--windows-target", choices=("startup-qa", "native", "mc-frame", "title-ui"))
    parser.add_argument("--extra-authored", action="append", default=[])
    parser.add_argument("--check-only", action="store_true")
    args = parser.parse_args()
    snapshot = args.snapshot.resolve()
    receipt = json.loads((snapshot / "analysis/source-snapshot.json").read_text(encoding="utf-8"))
    if Path(receipt["source_project"]).resolve() != ROOT or Path(receipt["snapshot"]).resolve() != snapshot:
        raise SystemExit("Receipt does not identify this project and source copy")
    native = json.loads((snapshot / "analysis/native-verification.json").read_text(encoding="utf-8"))
    if native.get("status") != "passed" or not native.get("source_sha256"):
        raise SystemExit("Native source copy has not passed verification")
    inputs = dict(native["source_sha256"])
    if args.windows_target:
        window = json.loads((snapshot / ("analysis/windows-" + args.windows_target + "-build.json")).read_text(encoding="utf-8"))
        if window.get("status") != "built":
            raise SystemExit("Requested Windows target has not finished building")
        for name, sha in window["source_sha256"].items():
            if name in inputs and inputs[name] != sha:
                raise SystemExit("Reports disagree for source: " + name)
            inputs[name] = sha
    for name in args.extra_authored:
        if name not in receipt["source_sha256"]:
            raise SystemExit("Extra file is absent from original source copy: " + name)
        inputs[name] = receipt["source_sha256"][name]
    files = {}
    for name, expected in sorted(inputs.items()):
        path = PurePosixPath(name)
        if (path.is_absolute() or ".." in path.parts or "\n" in name or "\t" in name
                or path.parts[0] not in ("src", "tests", "tools", "docs", "skill")
                or path.suffix not in (".c", ".h", ".cpp", ".py", ".ps1", ".java", ".md", ".json")):
            raise SystemExit("Refusing non-authored/private input: " + name)
        target = (snapshot / name).resolve()
        if snapshot not in target.parents:
            raise SystemExit("Source escaped snapshot: " + name)
        data = target.read_bytes()
        if (hashlib.sha256(data).hexdigest() != expected
                or receipt["source_sha256"].get(name) != expected):
            raise SystemExit("Source changed since original copy: " + name)
        files[name] = data
    if args.check_only:
        print("Verified eligible authored inputs:", len(files))
        return
    index = []
    for name, data in files.items():
        # --path applies .gitattributes/clean filters. Raw --stdin alone would
        # turn CRLF in a Windows worktree into spurious whole-file Git changes.
        blob = subprocess.check_output(["git", "hash-object", "-w", "--path=" + name, "--stdin"],
                                       input=data, cwd=ROOT, stderr=subprocess.DEVNULL).decode().strip()
        index.append("100644 " + blob + "\t" + name + "\n")
    subprocess.run(["git", "update-index", "--index-info"], input="".join(index).encode(), cwd=ROOT, check=True)
    print("Staged verified authored inputs with repository clean filters:", len(files))


if __name__ == "__main__":
    main()
