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

from workspace_paths import native_build_report, windows_build_report

ROOT = Path(__file__).resolve().parents[1]


def git(root, arguments, data=None):
    return subprocess.check_output(["git", *arguments], cwd=root, input=data, stderr=subprocess.DEVNULL)


def filtered_blobs(root, snapshot, files):
    """Hash immutable files through the repository's clean attributes."""
    names = list(files)
    if not names:
        return []
    git_dir = git(root, ["rev-parse", "--absolute-git-dir"]).decode().strip()
    worktree = ["--git-dir=" + git_dir, "--work-tree=" + str(snapshot)]
    paths = ("\0".join(names) + "\0").encode("utf-8")
    attributes = ["check-attr", "--all", "-z", "--stdin"]
    live = git(root, attributes, paths)
    frozen = git(snapshot, worktree + attributes, paths)
    # External clean commands can read their current directory. Retain the
    # original invocation for them or when the worktrees disagree on attributes.
    if live == frozen and b"filter" not in live.split(b"\0")[1::3]:
        lines = ("\n".join(names) + "\n").encode("utf-8")
        blobs = git(snapshot, worktree + ["hash-object", "-w", "--stdin-paths"], lines).decode().splitlines()
        if len(blobs) != len(names):
            raise SystemExit("Git did not return one filtered blob per verified source")
        if any((snapshot / name).read_bytes() != files[name] for name in names):
            raise SystemExit("Snapshot changed during Git filtering")
        return blobs
    return [git(root, ["hash-object", "-w", "--path=" + name, "--stdin"], files[name]).decode().strip()
            for name in names]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--windows-target", choices=("startup-qa", "native", "mc-frame", "title-ui"))
    parser.add_argument("--native-build-scratch", type=Path,
                        help="Use the full native test report from this build scratch")
    parser.add_argument("--windows-build-scratch", type=Path,
                        help="Use the selected Windows target's report from this build scratch")
    parser.add_argument("--extra-authored", action="append", default=[])
    parser.add_argument("--check-only", action="store_true")
    args = parser.parse_args()
    if args.windows_build_scratch and not args.windows_target:
        parser.error("--windows-build-scratch requires --windows-target")
    snapshot = args.snapshot.resolve()
    analysis = snapshot / "analysis"
    receipt = json.loads((analysis / "source-snapshot.json").read_text(encoding="utf-8"))
    if Path(receipt["source_project"]).resolve() != ROOT or Path(receipt["snapshot"]).resolve() != snapshot:
        raise SystemExit("Receipt does not identify this project and source copy")
    native_path = (native_build_report(args.native_build_scratch) if args.native_build_scratch
                   else analysis / "native-verification.json")
    try:
        native = json.loads(native_path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        raise SystemExit(f"Native verification report missing at {native_path}") from None
    if native.get("status") != "passed" or not native.get("source_sha256"):
        raise SystemExit("Native source copy has not passed verification")
    inputs = dict(native["source_sha256"])
    if args.windows_target:
        windows_path = (windows_build_report(args.windows_build_scratch, args.windows_target)
                        if args.windows_build_scratch else analysis / f"windows-{args.windows_target}-build.json")
        try:
            window = json.loads(windows_path.read_text(encoding="utf-8"))
        except FileNotFoundError:
            raise SystemExit(f"Windows build report missing at {windows_path}") from None
        if (window.get("status") != "built" or window.get("target") != args.windows_target
                or not window.get("source_sha256")):
            raise SystemExit("Requested Windows target has not finished building")
        for name, sha in window["source_sha256"].items():
            if name in inputs and inputs[name] != sha:
                raise SystemExit("Reports disagree for source: " + name)
            inputs[name] = sha
    for name in args.extra_authored:
        if name not in receipt["source_sha256"]:
            raise SystemExit("Extra file is absent from original source copy: " + name)
        inputs.setdefault(name, receipt["source_sha256"][name])
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
    blobs = filtered_blobs(ROOT, snapshot, files)
    index = ["100644 " + blob + "\t" + name + "\n" for name, blob in zip(files, blobs)]
    subprocess.run(["git", "update-index", "--index-info"], input="".join(index).encode(), cwd=ROOT, check=True)
    print("Staged verified authored inputs with repository clean filters:", len(files))


if __name__ == "__main__":
    main()
