"""Record a fresh private proof from matching immutable source/build/actual QA.

Read-only validation; never builds, runs, publishes, or estimates port completion.
Example: --snapshot G:/.../snapshots/room-transition-64 --scratch G:/.../verification/room-transition-64
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

PROJECT = Path(__file__).resolve().parents[1]
SCRATCH_ROOT = Path("G:/middle_thing/godhand_windows").resolve()


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


def expected_stop(value):
    if value is None:
        return None
    module, raw = value.rsplit(":", 1) if ":" in value else ("main ELF", value)
    require(bool(module.strip()), "empty frontier module")
    result = (module.strip().casefold(), address(raw))
    require(result[1] != "00000000", "expected unfinished frontier must be nonzero")
    return result


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


def validate(snapshot_arg, scratch_arg, frontier=None):
    """Pure read-only validator, returning proof and its exact input guards."""
    supplied = Path(snapshot_arg).resolve()
    receipt_path = supplied / "analysis/source-snapshot.json" if supplied.is_dir() else supplied
    snapshot = receipt_path.parent.parent.resolve()
    scratch = Path(scratch_arg).resolve()
    require(under(snapshot, SCRATCH_ROOT) and under(scratch, SCRATCH_ROOT), "snapshot and scratch must be below project G scratch")
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
    for key, name in (("native", "native-verification.json"), ("windows", "windows-startup-qa-build.json"),
                      ("rng", "rng-differential.json"), ("frame_rng", "frame-rng-differential.json")):
        reports[key], facts[key] = inputs.report(analysis / name)
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
