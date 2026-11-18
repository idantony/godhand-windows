"""Record a fresh private proof from matching immutable source/build/actual QA.

Read-only validation; never builds, runs, publishes, or estimates port completion.
Example: --snapshot .work/snapshots/room-transition-64 --scratch .work/verification/room-transition-64
 --output E:/godhand_windows/analysis/milestone64-private.json --expected-frontier 00132DA0
Bare frontier addresses mean main ELF; use 'module:address' for an overlay.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys

from workspace_paths import native_build_report, windows_build_report, work_root

PROJECT = Path(__file__).resolve().parents[1]
SCRATCH_ROOT = work_root(PROJECT)


class ProofError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise ProofError(message)


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def under(path, parent):
    return path != parent and parent in path.parents


def address(value):
    require(isinstance(value, str) and re.fullmatch(r"(?:0x)?[0-9a-fA-F]{1,8}", value), "invalid frontier address")
    return f"{int(value, 16):08X}"


STARTUP_NATIVE_ERRORS = ("renderer_error", "world_gpu_error", "window_error", "effect_post_error")


def require_clean_startup_tick_limit(qa, code, command_limit, expected_limit):
    require(type(expected_limit) is int and expected_limit > 0 and
            command_limit == str(expected_limit) and qa.get("ticks") == expected_limit,
            "QA tick limit differs from command or observation")
    require(code == 3 and qa.get("host_phase") == qa.get("host_error") == qa.get("subsystem_error") == 0
            and address(qa.get("frontier")) == "00000000"
            and not any(qa.get(key, 0) for key in STARTUP_NATIVE_ERRORS),
            "bounded QA must exit cleanly at tick limit")
    require(type(qa.get("scene_frames_presented")) is int and qa["scene_frames_presented"] > 0,
            "bounded QA needs presented scene frames")


def expected_stop(value):
    if value is None:
        return None
    module, raw = value.rsplit(":", 1) if ":" in value else ("main ELF", value)
    require(bool(module.strip()), "empty frontier module")
    result = (module.strip().casefold(), address(raw))
    require(result[1] != "00000000", "expected unfinished frontier must be nonzero")
    return result


def controlled_r003_status(log, qa):
    marker = "QA_R003_STATUS "
    records = [line[len(marker):] for line in log.splitlines() if line.startswith(marker)]
    require(len(records) == 1, "missing or duplicate controlled R003 status")
    status = json.loads(records[0])
    fields = ("handoff_tick", "first_scene", "last_scene", "frames", "last_ev02_cut")
    require(isinstance(status, dict) and all(type(status.get(key)) is int for key in fields),
            "invalid controlled R003 status")
    handoff, first, last, count, cut = (status[key] for key in fields)
    ticks, scenes = qa.get("ticks"), qa.get("scene_frames_presented")
    require(type(ticks) is int and type(scenes) is int and 0 < handoff <= ticks and 0 < first <= last <= scenes
            and count == last - first + 1, "controlled R003 has no verified presented range")
    cuts = [(int(n), int(serial)) for n, serial in re.findall(r"^QA_R003_PRESENTED cut=(\d+) scene=(\d+)$", log, re.M)]
    previous_scene = first - 1
    for expected, (number, serial) in enumerate(cuts):
        require(number == expected and previous_scene < serial <= last,
                "controlled R003 cut markers disagree with presented range")
        previous_scene = serial
    require(cut == len(cuts) - 1, "controlled R003 cut markers disagree with presented range")
    return status


def controlled_r105_status(log, qa, r003):
    status_marker, first_marker = "QA_R105_STATUS ", "QA_R105_PRESENTED "
    lines = log.splitlines()
    records = [line[len(status_marker):] for line in lines if line.startswith(status_marker)]
    observed = [line[len(first_marker):] for line in lines if line.startswith(first_marker)]
    if not records and not observed:
        return None  # A controlled R003 cohort can predate R105 telemetry.
    require(len(records) == 1, "missing or duplicate controlled R105 status")
    status = json.loads(records[0])
    fields = ("first_scene", "last_scene", "frames")
    require(isinstance(status, dict) and all(type(status.get(key)) is int for key in fields),
            "invalid controlled R105 status")
    first, last, count = (status[key] for key in fields)
    if (first, last, count) == (0, 0, 0):
        require(not observed, "controlled R105 marker without presented frames")
        return None
    require(r003["last_scene"] < first <= last <= qa["scene_frames_presented"] and
            count == last - first + 1, "controlled R105 has no verified presented range")
    require(len(observed) == 1, "missing or duplicate controlled R105 first presentation")
    marker = re.fullmatch(r"scene=([0-9]+) tick=([0-9]+)", observed[0])
    require(marker and int(marker[1]) == first and r003["handoff_tick"] < int(marker[2]) <= qa["ticks"],
            "controlled R105 first presentation disagrees with its range")
    return {**status, "first_tick": int(marker[2])}


def controlled_room_ranges(log, qa):
    r003 = controlled_r003_status(log, qa)
    r105 = controlled_r105_status(log, qa, r003)
    observations = {"controlled_r003": r003}
    if r105 is not None:
        observations["controlled_r105"] = r105
    r002_records = re.findall(r"^QA_R002_STATUS (.+)$", log, re.M)
    r002 = None
    if r002_records:
        require(len(r002_records) == 1, "duplicate R002 run summary")
        r002 = json.loads(r002_records[0])
        fields = ("first_scene", "last_scene", "frames", "first_cut_scene")
        require(all(type(r002.get(key)) is int for key in fields), "invalid R002 run summary")
        first, last, count, first_cut = (r002[key] for key in fields)
        if first:
            require(r105 and r105["last_scene"] < first <= last <= qa["scene_frames_presented"] and
                    count == last - first + 1, "R002 presentation does not match its frame range")
            cut_trace = re.findall(r"^QA_R002_PRESENTED event=4 cut=0 scene=(\d+) tick=(\d+)$", log, re.M)
            require(len(cut_trace) == 1 and int(cut_trace[0][0]) == first_cut and
                    first <= first_cut <= last and int(cut_trace[0][1]) <= qa["ticks"],
                    "EV04 first cut does not match the R002 device presentation")
            observations["controlled_r002_ev04"] = r002
    return observations


def controlled_room_observations(log, qa):
    observations = controlled_room_ranges(log, qa)
    ranges = observations.values()
    r002 = observations.get("controlled_r002_ev04")
    captures = [int(serial) for serial in re.findall(r"^QA_CAPTURE_PRESENTED (\d+)$", log, re.M)]
    require(all(any(room["first_scene"] <= serial <= room["last_scene"] for room in ranges)
                for serial in captures), "capture presented outside controlled rooms")
    if r002 and r002["first_cut_scene"]:
        cut_captures = re.findall(r"^QA_EV04_CAPTURE scene=(\d+) cut=1 frame=([\d.]+)$", log, re.M)
        require(captures and [int(serial) for serial, _ in cut_captures] == captures and
                all(r002["first_cut_scene"] <= int(serial) <= r002["last_scene"] and float(frame) >= 8
                    for serial, frame in cut_captures), "capture series does not prove EV04 cut 1")
    return observations


class Inputs:
    def __init__(self):
        self.files = {}

    def checked(self, path, expected=None):
        path = path.resolve()
        require(path.is_file(), f"missing input: {path}")
        actual = digest(path)
        if expected is not None:
            require(isinstance(expected, str) and re.fullmatch(r"[0-9a-fA-F]{64}", expected), f"invalid SHA256 for {path}")
            require(actual == expected.lower(), f"SHA256 mismatch: {path}")
        require(path not in self.files or self.files[path] == actual, f"input changed during validation: {path}")
        self.files[path] = actual
        return actual

    def report(self, path):
        sha = self.checked(path)
        try:
            data = json.loads(path.read_text(encoding="utf-8-sig"))
        except (ValueError, UnicodeError) as error:
            raise ProofError(f"invalid JSON: {path}: {error}") from error
        require(isinstance(data, dict), f"report must be an object: {path}")
        return data, {"path": str(path.resolve()), "sha256": sha}

    def recheck(self):
        for path, sha in self.files.items():
            require(path.is_file() and digest(path) == sha, f"input changed before recording: {path}")


def source_path(snapshot, name):
    require(isinstance(name, str) and "\\" not in name, "source receipt requires relative POSIX paths")
    parts = PurePosixPath(name)
    require(not parts.is_absolute() and parts.parts and not any(p in (".", "..") for p in parts.parts)
            and ":" not in name and name == parts.as_posix(), f"unsafe source path: {name}")
    path = (snapshot / name).resolve()
    require(under(path, snapshot), f"source escapes snapshot: {name}")
    return path


def source_map(inputs, snapshot, receipt, report, label, required=True):
    mapping = report.get("source_sha256")
    if mapping is None and not required:
        return 0
    require(isinstance(mapping, dict) and mapping, f"{label} missing source_sha256")
    for name, sha in mapping.items():
        require(name in receipt and receipt[name] == sha, f"{label} mixed source cohort: {name}")
        inputs.checked(source_path(snapshot, name), sha)
    return len(mapping)


def steps(report, label):
    rows = report.get("steps")
    require(isinstance(rows, list) and rows, f"{label} missing steps")
    names = []
    for row in rows:
        require(isinstance(row, dict) and isinstance(row.get("name"), str), f"{label} malformed step")
        require(type(row.get("returncode")) is int and row["returncode"] == 0,
                f"{label} failed step: {row.get('name')}")
        names.append(row["name"])
    require(len(set(names)) == len(names), f"{label} duplicate step names")
    return names


def artifact_path(raw, scratch, label):
    require(isinstance(raw, str) and raw, f"{label} missing artifact path")
    path = Path(raw).resolve()
    require(under(path, scratch / "build"), f"{label} artifact is outside matching scratch/build: {path}")
    return path


def per_build_report(local, fallback):
    return local if local.exists() or local.is_symlink() else fallback


def startup_build_report(snapshot, scratch):
    return per_build_report(windows_build_report(scratch, "startup-qa"),
                            snapshot / "analysis/windows-startup-qa-build.json")


def startup_build_inputs(snapshot, scratch, project):
    """Shared launch/receipt preflight for an exact startup-qa build."""
    require(under(snapshot, SCRATCH_ROOT) and under(scratch, SCRATCH_ROOT),
            "snapshot and scratch must be below the active work root")
    inputs = Inputs()
    receipt, receipt_fact = inputs.report(snapshot / "analysis/source-snapshot.json")
    require(receipt.get("status") == "copied-exact-bytes; not-yet-built-or-tested",
            "invalid source snapshot receipt")
    require(Path(receipt.get("snapshot", "")).resolve() == snapshot, "source snapshot path mismatch")
    require(Path(receipt.get("source_project", "")).resolve() == project.resolve(), "source project mismatch")
    hashes = receipt.get("source_sha256")
    require(isinstance(hashes, dict) and hashes, "empty snapshot source map")
    for name, sha in hashes.items():
        inputs.checked(source_path(snapshot, name), sha)
    report, report_fact = inputs.report(startup_build_report(snapshot, scratch))
    require(report.get("status") == "built" and report.get("target") == "startup-qa",
            "matching startup-qa build must be built")
    require(report.get("is_playable_game") is False, "build must retain non-playable scope")
    steps(report, "windows")
    source_map(inputs, snapshot, hashes, report, "windows")
    executable = scratch / "build/windows-startup-qa/probe-native-startup.exe"
    require(Path(report.get("executable", "")).resolve() == executable,
            "executable belongs to another scratch")
    exe_sha = inputs.checked(executable, report.get("executable_sha256"))
    return inputs, executable, exe_sha, receipt_fact, report_fact


def validate(snapshot_arg, scratch_arg, frontier=None):
    """Pure read-only validator, returning proof and its exact input guards."""
    supplied = Path(snapshot_arg).resolve()
    receipt_path = supplied / "analysis/source-snapshot.json" if supplied.is_dir() else supplied
    snapshot = receipt_path.parent.parent.resolve()
    scratch = Path(scratch_arg).resolve()
    require(under(snapshot, SCRATCH_ROOT) and under(scratch, SCRATCH_ROOT), "snapshot and scratch must be below the active work root")
    require(receipt_path == snapshot / "analysis/source-snapshot.json", "snapshot must use analysis/source-snapshot.json receipt")
    inputs = Inputs()
    receipt, receipt_fact = inputs.report(receipt_path)
    require(Path(receipt.get("snapshot", "")).resolve() == snapshot, "source receipt snapshot path mismatch")
    require(Path(receipt.get("source_project", "")).resolve() == PROJECT.resolve(), "source receipt project mismatch")
    require(receipt.get("status") == "copied-exact-bytes; not-yet-built-or-tested", "invalid source snapshot receipt status")
    hashes = receipt.get("source_sha256")
    require(isinstance(hashes, dict) and hashes, "empty source snapshot receipt")
    for name, sha in hashes.items():
        inputs.checked(source_path(snapshot, name), sha)

    analysis = snapshot / "analysis"
    reports, facts = {}, {"source_snapshot": receipt_fact}
    for key, path in (("native", per_build_report(native_build_report(scratch), analysis / "native-verification.json")),
                      ("windows", startup_build_report(snapshot, scratch)),
                      ("rng", analysis / "rng-differential.json"),
                      ("frame_rng", analysis / "frame-rng-differential.json")):
        reports[key], facts[key] = inputs.report(path)
        # Report the primary failed build before looking for subsequent RNG
        # reports that a failed native build legitimately never generated.
        if key == "native":
            require(reports[key].get("status") == "passed", "native.status must be passed")
        elif key == "windows":
            require(reports[key].get("status") == "built", "windows.status must be built")
    native, windows = reports["native"], reports["windows"]
    require(native.get("status") == "passed", "native.status must be passed")
    require(windows.get("status") == "built", "windows.status must be built")
    native_steps, windows_steps = steps(native, "native"), steps(windows, "windows")
    native_count = source_map(inputs, snapshot, hashes, native, "native")
    windows_count = source_map(inputs, snapshot, hashes, windows, "windows")
    require(native.get("is_playable_game") is False and windows.get("is_playable_game") is False,
            "build reports must retain unfinished/non-playable scope")
    modules = native.get("module_names")
    require(isinstance(modules, list) and modules and all(isinstance(n, str) and n for n in modules)
            and len(set(modules)) == len(modules), "invalid native module_names")
    require(all("compile-" + name in native_steps for name in modules), "native module missing successful compile step")
    tests = [n for n in native_steps if n.startswith("test-")]
    require(tests, "native report has no test-* checks")
    artifacts = native.get("artifacts")
    require(isinstance(artifacts, list) and artifacts, "native missing artifacts")
    artifact_hashes = {}
    for item in artifacts:
        require(isinstance(item, dict), "malformed native artifact")
        path = artifact_path(item.get("path"), scratch, "native")
        require(path not in artifact_hashes, "duplicate native artifact")
        artifact_hashes[path] = inputs.checked(path, item.get("sha256"))
        require(type(item.get("size")) is int and path.stat().st_size == item["size"], f"native artifact size mismatch: {path}")
    dll = scratch / "build/native/godhand_core.dll"
    require(dll in artifact_hashes, "native DLL missing from artifact report")
    limitations = ["Reports attest this bounded verification run, not whole-game completion, retail matching, human play, or hardware-equivalent PS2 behavior.",
                   "Actual process record captures command path/exit; current executable SHA is checked against its build report, not an OS-attested launch-time image capture."]
    for key in ("rng", "frame_rng"):
        report = reports[key]
        require(report.get("status") == "passed", f"{key}.status must be passed")
        require(artifact_path(report.get("native_dll"), scratch, key) == dll, f"{key} DLL path mismatch")
        require(report.get("native_sha256") == artifact_hashes[dll], f"{key} DLL SHA256 mismatch")
        inputs.checked(snapshot / "reference/SLUS_215.03", report.get("elf_sha256"))
        count = source_map(inputs, snapshot, hashes, report, key, required=(key == "frame_rng"))
        if not count:
            # Older rng-differential schema has no own source map. Its exact
            # report, script and matching native artifact are still pinned.
            require("tools/verify_rng.py" in hashes, "legacy RNG verifier absent from source receipt")
            inputs.checked(source_path(snapshot, "tools/verify_rng.py"), hashes["tools/verify_rng.py"])
            limitations.append("Legacy rng-differential.json has no independent source_sha256 field; its report/script hashes and matching native DLL are recorded, not retroactively supplied as run metadata.")

    executable = artifact_path(windows.get("executable"), scratch, "windows")
    exe_sha = inputs.checked(executable, windows.get("executable_sha256"))
    process, facts["process"] = inputs.report(scratch / "startup-process.json")
    qa, facts["actual_qa"] = inputs.report(scratch / "startup-newgame.json")
    for label, report in (("process", process), ("actual_qa", qa)):
        if "source_sha256" in report:
            source_map(inputs, snapshot, hashes, report, label)
    command = process.get("command")
    require(isinstance(command, list) and command and all(isinstance(n, str) for n in command), "invalid actual process.command")
    require(Path(command[0]).resolve() == executable, "actual process executable does not match windows report")
    require(all(flag in command for flag in ("--hidden", "--muted", "--qa-new-game")), "actual process must use hidden, muted, qa-new-game")
    require(qa.get("hidden") is True and qa.get("muted") is True, "actual QA must be hidden and muted")
    require(qa.get("prefix_mode") is False and qa.get("playable_game_verified") is False, "actual QA must retain unfinished continuation scope")
    require(type(process.get("exit_code")) is int and type(qa.get("exit_code")) is int
            and process["exit_code"] == qa["exit_code"], "actual process/QA exit mismatch")
    for name in ("renderer_error", "world_gpu_error", "window_error"):
        require(type(qa.get(name)) is int and qa[name] == 0, f"actual QA {name} is not zero")
    stop = expected_stop(frontier)
    if stop is None:
        require(process["exit_code"] == 0 and qa.get("host_error") == 0 and qa.get("subsystem_error") == 0
                and address(qa.get("frontier")) == "00000000", "nonzero actual frontier/error requires explicit --expected-frontier")
    else:
        require(process["exit_code"] == 4, "expected unfinished frontier requires actual exit4")
        require(isinstance(qa.get("frontier_module"), str) and qa["frontier_module"].strip().casefold() == stop[0]
                and address(qa.get("frontier")) == stop[1], "actual frontier/module does not match expectation")
        limitations.append("Exit4 was accepted only as the explicitly expected unfinished source frontier; it is not successful game completion.")
    fields = ("scope", "playable_game_verified", "hidden", "muted", "synthetic_input", "prefix_mode", "ticks", "host_phase",
              "host_error", "subsystem_error", "frontier_module", "frontier", "game_state", "room", "next_room", "assets_loaded",
              "mc_frames_presented", "movie_frames_presented", "movie_complete", "scene_frames_presented", "world_packets_submitted",
              "renderer_error", "world_gpu_error", "window_error", "exit_code")
    tool_sha = inputs.checked(Path(__file__))
    proof = {"schema": 1, "tool": "record-native-milestone", "tool_source_sha256": tool_sha,
             "created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
             "status": "verified-matching-private-evidence", "snapshot": str(snapshot), "scratch": str(scratch),
             "counts": {"native_modules": len(modules), "test_checks": len(tests), "native_steps": len(native_steps),
                        "windows_steps": len(windows_steps), "snapshot_sources": len(hashes), "native_sources": native_count,
                        "windows_sources": windows_count},
             "reports": facts, "artifacts": {"native_dll": {"path": str(dll), "sha256": artifact_hashes[dll]},
                                               "actual_executable": {"path": str(executable), "sha256": exe_sha}},
             "rng": {"rng": {k: reports["rng"][k] for k in ("status", "distinct_seeds", "function_calls_compared") if k in reports["rng"]},
                     "frame_rng": {k: reports["frame_rng"][k] for k in ("status", "distinct_MT_words", "comparisons") if k in reports["frame_rng"]}},
             "actual_qa": {k: qa[k] for k in fields if k in qa}, "expected_frontier": frontier, "limitations": limitations}
    inputs.recheck()
    return proof, inputs


def output_path(raw):
    path = Path(raw).resolve()
    require(under(path, (PROJECT / "analysis").resolve()) and path.name.endswith("-private.json"),
            "output must be a fresh ignored project/analysis/*-private.json")
    require(path.parent.is_dir() and not path.exists() and not path.is_symlink(), "output parent must exist; output must not already exist")
    result = subprocess.run(["git", "check-ignore", "--quiet", "--no-index", "--", str(path.relative_to(PROJECT))], cwd=PROJECT)
    require(result.returncode == 0, "output is not ignored by project Git rules")
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--scratch", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--expected-frontier")
    args = parser.parse_args()
    try:
        output = output_path(args.output)
        proof, inputs = validate(args.snapshot, args.scratch, args.expected_frontier)
        inputs.recheck()
        # Exclusive create also prevents a concurrent writer from overwriting a
        # freshly produced proof between the earlier path check and this write.
        with output.open("x", encoding="utf-8") as stream:
            json.dump(proof, stream, indent=2, ensure_ascii=False)
            stream.write("\n")
        print(str(output))
    except (ProofError, OSError) as error:
        print("record-native-milestone: " + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
