"""Run one hidden/muted title probe from an exact frozen Windows build.

Writes fresh process/stdout/stderr receipts in the matching G: scratch. This
does not build, publish, certify gameplay, or replace record-native-milestone.
"""
import argparse
import datetime
import importlib.util
import json
from pathlib import Path
import subprocess

PROJECT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("milestone_proof", PROJECT / "tools/record-native-milestone.py")
proof = importlib.util.module_from_spec(spec)
spec.loader.exec_module(proof)


MAX_CAPTURE_COUNT = 120


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def require_frozen_runner(snapshot):
    frozen = proof.source_path(Path(snapshot).resolve(), "tools/run-startup-qa.py")
    proof.require(proof.digest(frozen) == proof.digest(Path(__file__)),
                  "QA launcher differs from the frozen snapshot; freeze the current launcher before running")


def prepare(snapshot_arg, scratch_arg, iso_arg, ffmpeg_arg, max_ticks=1100, capture_frame=370,
            capture_count=1, control_probe=None, title_load=False, build_scratch=None):
    """Read-only preflight. No launch or output creation occurs on rejection."""
    snapshot, scratch = Path(snapshot_arg).resolve(), Path(scratch_arg).resolve()
    build_scratch = scratch if build_scratch is None else Path(build_scratch).resolve()
    iso, ffmpeg = Path(iso_arg).resolve(), Path(ffmpeg_arg).resolve()
    proof.require(proof.under(snapshot, proof.SCRATCH_ROOT) and proof.under(scratch, proof.SCRATCH_ROOT),
                  "snapshot and scratch must be below project G scratch")
    proof.require(type(max_ticks) is int and 1 <= capture_frame <= max_ticks <= 100000,
                  "require 1 <= capture-frame <= max-ticks <= 100000")
    proof.require(type(capture_count) is int and 1 <= capture_count <= MAX_CAPTURE_COUNT
                  and capture_frame + capture_count - 1 <= max_ticks,
                  f"require 1 <= capture-count <= {MAX_CAPTURE_COUNT} and range within max-ticks")
    proof.require(control_probe in (None, "walk", "run", "square", "approach-square"), "unknown control probe")
    proof.require(not (title_load and control_probe), "gameplay input requires NewGame")
    _, executable, exe_sha, receipt_fact, report_fact = proof.startup_build_inputs(snapshot, build_scratch, PROJECT)
    proof.require(iso.is_file() and ffmpeg.is_file(), "ISO and ffmpeg must be existing files")
    profile = scratch / ("profile-load" if title_load else "profile-absent")
    capture = scratch / "first-scene-candidate.rgba"
    outputs = [scratch / name for name in ("startup-run.lock", "startup-newgame.json", "startup-newgame.stderr.log", "startup-process.json")]
    if title_load:
        proof.require(profile.is_dir() and profile.resolve() == profile, "QA Load requires the local scratch profile-load directory")
    else:
        proof.require(not profile.exists(), "QA profile must be absent; refusing to touch existing profile")
    proof.require(not capture.exists() and not Path(str(capture) + ".world.rgba").exists() and not any(p.exists() for p in outputs), "QA outputs already exist; use a fresh scratch cohort")
    command = [str(executable), "--iso", str(iso), "--profile", str(profile), "--ffmpeg", str(ffmpeg),
               "--hidden", "--muted", "--qa-load" if title_load else "--qa-new-game", "--qa-capture", str(capture),
               "--qa-capture-frame", str(capture_frame), "--max-ticks", str(max_ticks)]
    if capture_count > 1:
        proof.require(not any(scratch.glob(capture.stem + ".scene-*.rgba")),
                      "QA range outputs already exist; use a fresh scratch cohort")
        command.extend(("--qa-capture-count", str(capture_count)))
    if control_probe:
        command.extend(("--qa-control", control_probe))
    return {"command": command, "scratch": scratch, "build_scratch": build_scratch, "executable": executable,
            "executable_sha256": exe_sha, "source_snapshot": receipt_fact, "windows_build": report_fact}


def run(prepared):
    scratch = prepared["scratch"]
    scratch.mkdir(parents=True, exist_ok=True)
    # Exclusive reservation prevents two agents from replacing one QA receipt.
    with (scratch / "startup-run.lock").open("x", encoding="utf-8") as lock:
        lock.write(json.dumps({"created_utc": now(), "command": prepared["command"]}) + "\n")
    receipt = {"created_utc": now(), "command": prepared["command"],
               "executable_sha256_before": prepared["executable_sha256"],
               "source_snapshot": prepared["source_snapshot"], "windows_build": prepared["windows_build"],
               "runner_sha256": proof.digest(Path(__file__)), "scope": "actual QA execution; unfinished source port"}
    if prepared["build_scratch"] != scratch:
        receipt["build_scratch"] = str(prepared["build_scratch"])
    with (scratch / "startup-newgame.json").open("xb") as out, (scratch / "startup-newgame.stderr.log").open("xb") as err:
        try:
            child = subprocess.run(prepared["command"], stdout=out, stderr=err)
            receipt["exit_code"] = child.returncode
        except OSError as error:
            receipt["launch_error"] = str(error)
    # Fingerprint even if the child reports a renderer failure below.
    capture = Path(prepared["command"][prepared["command"].index("--qa-capture") + 1])
    captures = [capture, Path(str(capture) + ".world.rgba")]
    captures.extend(sorted(scratch.glob(capture.stem + ".scene-*.rgba")))
    outputs = [scratch / "startup-newgame.json", scratch / "startup-newgame.stderr.log"]
    outputs.extend(path for path in captures if path.is_file() and path.resolve().parent == scratch)
    receipt["outputs_sha256"] = {path.name: proof.digest(path) for path in outputs}
    receipt["finished_utc"] = now()
    receipt["executable_sha256_after"] = proof.digest(prepared["executable"])
    with (scratch / "startup-process.json").open("x", encoding="utf-8") as stream:
        json.dump(receipt, stream, indent=2)
        stream.write("\n")
    proof.require("launch_error" not in receipt, "QA launch failed; inspect process receipt")
    proof.require(receipt["executable_sha256_before"] == receipt["executable_sha256_after"], "QA executable changed during execution")
    qa = json.loads((scratch / "startup-newgame.json").read_text(encoding="utf-8-sig"))
    proof.require(qa.get("hidden") is True and qa.get("muted") is True and qa.get("synthetic_input") is True
                  and qa.get("prefix_mode") is False, "QA did not confirm expected execution flags")
    proof.require(qa.get("exit_code") == receipt["exit_code"] and receipt["exit_code"] in (0, 4), "unexpected child exit; inspect QA output")
    for field in ("renderer_error", "world_gpu_error", "window_error"):
        proof.require(qa.get(field) == 0, "QA reported " + field)
    return {"exit_code": receipt["exit_code"], "frontier": qa.get("frontier"), "ticks": qa.get("ticks"),
            "world_packets_submitted": qa.get("world_packets_submitted"), "receipt": str(scratch / "startup-process.json")}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--scratch", type=Path, required=True)
    parser.add_argument("--build-scratch", type=Path, help="reuse the exact build in this G: scratch cohort")
    parser.add_argument("--iso", type=Path, default=PROJECT / "God Hand (USA)(Uncensor Patch v1.0).iso")
    parser.add_argument("--ffmpeg", type=Path, default=PROJECT / ".tools/ffmpeg-9.0.2-essentials_build/bin/ffmpeg.exe")
    parser.add_argument("--max-ticks", type=int, default=1100)
    parser.add_argument("--capture-frame", type=int, default=370)
    parser.add_argument("--capture-count", type=int, default=1,
                        help=f"capture up to {MAX_CAPTURE_COUNT} ready scene frames beginning at capture-frame")
    parser.add_argument("--control-probe", choices=("walk", "run", "square", "approach-square"),
                        help="send original pad input after EV01 and two neutral player updates")
    parser.add_argument("--title-load", action="store_true", help="select Load Game using the scratch profile-load directory")
    args = parser.parse_args()
    try:
        prepared = prepare(args.snapshot, args.scratch, args.iso, args.ffmpeg, args.max_ticks, args.capture_frame,
                           args.capture_count, args.control_probe, args.title_load, args.build_scratch)
        require_frozen_runner(args.snapshot)
        print(json.dumps(run(prepared)), flush=True)
    except (proof.ProofError, OSError, ValueError) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
