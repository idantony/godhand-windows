"""Prepare private God Hand references from the user's ISO; never run SDK setup."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

PIN = "eaae18d0dfab1f9a0df276ae5ca1581ad326faf3"
REMOTE = "https://github.com/LucasPicoli/god-hand-decomp.git"


def inside(path, parent):
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def prepare(args):
    project, iso, scratch = args.project.resolve(), args.iso.resolve(), args.scratch.resolve()
    if not project.is_dir() or project.parent == project:
        raise ValueError("Project must be an existing, non-filesystem-root directory")
    for name in ("inspect_disc.py", "inspect_assets.py"):
        if not (project / "tools" / name).is_file():
            raise ValueError("Project is missing tools/" + name)
    if not iso.is_file():
        raise ValueError("--iso must identify the user's existing ISO file")
    prior = project / "analysis" / "disc-baseline.json"
    if prior.is_file():
        expected = json.loads(prior.read_text(encoding="utf-8"))["source_iso"]["sha256"]
        digest = hashlib.sha256()
        with iso.open("rb") as stream:
            for chunk in iter(lambda: stream.read(8 * 1024 * 1024), b""):
                digest.update(chunk)
        if digest.hexdigest() != expected:
            raise ValueError("ISO differs from this project's baseline; use the correct input or a separate project")
    for output in (project / "reference", project / "analysis", project / "vendor", scratch):
        if inside(iso, output.resolve()):
            raise ValueError("ISO must be outside preparation output directories")
    if inside(scratch, project):
        raise ValueError("Scratch must be outside the permanent project tree")
    for name in ("reference", "analysis", "vendor"):
        if not inside((project / name).resolve(), project):
            raise ValueError("Project output directory resolves outside project: " + name)
    scratch.mkdir(parents=True, exist_ok=True)
    logs = scratch / "prepare" / datetime.datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    logs.mkdir(parents=True)
    env = os.environ.copy()
    env.update(TMP=str(logs), TEMP=str(logs), TMPDIR=str(logs),
               PYTHONDONTWRITEBYTECODE="1", GIT_TERMINAL_PROMPT="0")
    sequence = []

    def run(argv, cwd=project, private_output=False):
        result = subprocess.run(argv, cwd=str(cwd), env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, text=True, encoding="utf-8", errors="replace")
        number = len(sequence)
        (logs / (str(number) + ".stdout.log")).write_text("[not recorded]" if private_output else result.stdout, encoding="utf-8")
        (logs / (str(number) + ".stderr.log")).write_text("[not recorded]" if private_output else result.stderr, encoding="utf-8")
        sequence.append({"operation": Path(argv[0]).name, "returncode": result.returncode})
        if result.returncode:
            raise ValueError("Preparation command failed; inspect scratch logs: " + str(logs))
        return result.stdout.strip()

    vendor = project / "vendor" / "god-hand-decomp"
    if not inside(vendor.resolve(), project):
        raise ValueError("Vendor checkout resolves outside project")
    cloned = False
    if vendor.exists():
        if not (vendor / ".git").exists():
            raise ValueError("Existing vendor directory is not a Git checkout; refusing to overwrite")
        origin = run(["git", "remote", "get-url", "origin"], vendor, private_output=True)
        if origin not in (REMOTE, REMOTE[:-4]):
            raise ValueError("Vendor origin is not the pinned official repository")
        if run(["git", "status", "--porcelain", "--untracked-files=all"], vendor):
            raise ValueError("Vendor checkout is dirty; refusing to change it")
        if run(["git", "rev-parse", "HEAD"], vendor) != PIN:
            raise ValueError("Existing vendor HEAD differs from required pin; refusing to replace it")
    else:
        vendor.parent.mkdir(parents=True, exist_ok=True)
        run(["git", "clone", "--no-checkout", REMOTE, str(vendor)])
        run(["git", "checkout", "--detach", PIN], vendor)
        cloned = True
    before = iso.stat()
    for name in ("inspect_disc.py", "inspect_assets.py"):
        run([sys.executable, "-B", str(project / "tools" / name), str(iso), "--project", str(project)])
    for name in ("inspect_render_assets.py", "inspect_model_asset.py", "inspect_core_bundle.py"):
        if (project / "tools" / name).is_file():
            run([sys.executable, "-B", str(project / "tools" / name), "--project", str(project)])
    after = iso.stat()
    if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
        raise ValueError("ISO changed during preparation")
    baseline = json.loads((project / "analysis" / "disc-baseline.json").read_text(encoding="utf-8"))
    return {"status": "prepared", "project": str(project), "source_iso": str(iso),
            "iso_sha256": baseline["source_iso"]["sha256"], "vendor_pin": PIN,
            "vendor_cloned": cloned, "scratch_logs": str(logs), "operations": sequence,
            "scope": "God Hand ISO9660/AFS/FST/SNR2 only; private references, not a playable game",
            "iso_copied_or_modified": False, "upstream_setup_executed": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--scratch", type=Path, default=Path(r"G:\middle_thing\godhand_windows"))
    args = parser.parse_args()
    try:
        result = prepare(args)
    except (ValueError, OSError) as error:
        print(json.dumps({"status": "failed", "error": str(error)}, ensure_ascii=False))
        return 1
    print(json.dumps(result, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
