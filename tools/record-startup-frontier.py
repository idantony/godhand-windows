"""Record an unfinished Windows startup observation without running the game.

Pass the expected original frontier (module:address, or bare main ELF address)
or an expected native error (for example world_gpu_error:2). Old process
receipts need previously recorded stdout/stderr digests. An optional process
digest provides an independent anchor for the embedded output fingerprints.
"""
import argparse
import datetime
import importlib.util
import json
import math
from pathlib import Path
import re

PROJECT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("milestone_proof", PROJECT / "tools/record-native-milestone.py")
proof = importlib.util.module_from_spec(spec)
spec.loader.exec_module(proof)

NATIVE_ERRORS = ("renderer_error", "world_gpu_error", "window_error", "effect_post_error")


def fact(inputs, path, expected):
    return {"path": str(path), "sha256": inputs.checked(path, expected)}


def output_fact(inputs, scratch, name, hashes, expected):
    path = scratch / name
    proof.require(path.resolve().parent == scratch, f"output escapes scratch: {name}")
    recorded = hashes.get(name)
    proof.require(recorded is not None or expected is not None,
                  f"{name} needs a launch fingerprint or an independent SHA256")
    result = fact(inputs, path, recorded if recorded is not None else expected)
    if expected is not None:
        inputs.checked(path, expected)
    return result


def native_error(value):
    if value is None:
        return None
    field, sep, raw = value.partition(":")
    proof.require(bool(sep) and field in NATIVE_ERRORS and re.fullmatch(r"[1-9][0-9]*", raw),
                  "native error requires an error field and positive decimal code")
    return field, int(raw)


def argument(command, flag):
    proof.require(command.count(flag) == 1, f"process must contain one {flag}")
    index = command.index(flag) + 1
    proof.require(index < len(command), f"process missing value for {flag}")
    return command[index]


def capture_observations(inputs, scratch, capture, hashes, command, trace):
    """Read matching world and final surfaces from a completed range capture."""
    prefix = "QA_WORLD_CAPTURE "
    frames = [json.loads(line[len(prefix):]) for line in trace if line.startswith(prefix)]
    present_prefix = "QA_CAPTURE_PRESENTED "
    present_records = [line[len(present_prefix):] for line in trace if line.startswith(present_prefix)]
    proof.require(all(re.fullmatch(r"[1-9][0-9]*", value) for value in present_records),
                  "invalid captured presentation serial")
    presented = {int(value) for value in present_records}
    proof.require(len(presented) == len(present_records), "duplicate captured presentation serial")
    first = int(argument(command, "--qa-capture-frame"))
    limit = int(argument(command, "--qa-capture-count")) if "--qa-capture-count" in command else 1
    proof.require(first > 0 and 1 <= len(frames) <= limit <= 120,
                  "missing or excessive capture metadata")
    size = 512 * 448 * 4
    last = first - 1
    observations = []
    for frame in frames:
        proof.require(isinstance(frame, dict) and type(frame.get("scene_serial")) is int and
                      frame["scene_serial"] > last and frame.get("world_bytes") == size and
                      type(frame.get("fade_visible")) is int and frame["fade_visible"] in (0, 1) and
                      isinstance(frame.get("fade_rgba"), str) and re.fullmatch(r"[0-9A-Fa-f]{8}", frame["fade_rgba"]),
                      "invalid or out-of-order world capture metadata")
        last = frame["scene_serial"]
        name = capture.name if not observations else f"{capture.stem}.scene-{last}.rgba"
        surfaces = {}
        for layer, filename in (("composite", name), ("world", name + ".world.rgba")):
            artifact = output_fact(inputs, scratch, filename, hashes, None)
            pixels = Path(artifact["path"]).read_bytes()
            proof.require(len(pixels) == size, f"unexpected RGBA surface size: {filename}")
            artifact["nonblack_rgb_pixels"] = sum(
                pixels[i:i + 3] != b"\0\0\0" for i in range(0, size, 4))
            surfaces[layer] = artifact
        observation = {"scene_serial": last, "fade_visible": frame["fade_visible"],
                       "fade_rgba": frame["fade_rgba"], **surfaces}
        if frame.get("presentation_tracking") is True:
            observation["presented"] = last in presented
        observations.append(observation)
    proof.require(presented <= {frame["scene_serial"] for frame in observations if frame.get("presented")},
                  "presentation marker has no matching tracked capture")
    return observations


def trace_record(trace, prefix):
    records = [line[len(prefix):] for line in trace if line.startswith(prefix)]
    proof.require(len(records) <= 1, f"multiple {prefix.strip()} observations")
    if not records:
        return None
    record = json.loads(records[0])
    proof.require(isinstance(record, dict), f"invalid {prefix.strip()} observation")
    return record


def control_observation(command, trace):
    if "--qa-control" not in command:
        return None
    mode = argument(command, "--qa-control")
    status_prefix = "QA_CONTROL_STATUS "
    records = [line[len(status_prefix):] for line in trace if line.startswith(status_prefix)]
    proof.require(len(records) <= 1, "multiple control status observations")
    if not records:
        return None
    status = re.fullmatch(r"mode=(walk|run|square|approach-square) saw_event=([01]) pressed=([01]) motion_presented=([01])",
                          records[0])
    proof.require(status is not None and status[1] == mode, "invalid control status observation")
    observations = [line for line in trace if line.startswith("QA_CONTROL ")]
    proof.require(len(observations) <= 16 and all(line.startswith(f"QA_CONTROL mode={mode} ")
                  for line in observations), "invalid control trace observations")
    present_prefix = "QA_CONTROL_PRESENTED "
    presented = [line[len(present_prefix):] for line in trace if line.startswith(present_prefix)]
    motion = status[4] == "1"
    proof.require(len(presented) == int(motion), "control motion status and presentation differ")
    result = {"mode": mode, "saw_event": status[2] == "1", "pressed": status[3] == "1",
              "motion_presented": motion, "input_trace": observations}
    if presented:
        marker = r"mode=(walk|run|square|approach-square) handle=([1-9][0-9]*) scene=([1-9][0-9]*)"
        attacks = mode in ("square", "approach-square")
        if attacks:
            number = r"([0-9]+(?:\.[0-9]+)?(?:e[+-]?[0-9]+)?)"
            marker += rf" entry=0010D3A8 sample_before={number} sample={number}"
        match = re.fullmatch(marker, presented[0])
        proof.require(match is not None and match[1] == mode and result["pressed"],
                      "invalid presented control motion observation")
        result["presented_motion"] = {"handle": int(match[2]), "scene_serial": int(match[3])}
        if attacks:
            before, sample = float(match[4]), float(match[5])
            proof.require(math.isfinite(before) and math.isfinite(sample) and 0 <= before < sample,
                          "Square animation sample did not advance")
            result["presented_motion"].update(entry="0010D3A8", sample_before=before, sample=sample)
    return result


def validate(snapshot_arg, scratch_arg, *, project=PROJECT, expected_frontier=None,
             expected_native_error=None, process_sha=None, stdout_sha=None, stderr_sha=None,
             capture_name=None, capture_sha=None, capture_series=False):
    snapshot, scratch, project = (Path(p).resolve() for p in (snapshot_arg, scratch_arg, project))
    stop = proof.expected_stop(expected_frontier)
    error = native_error(expected_native_error)
    proof.require((stop is None) != (error is None), "provide exactly one expected stop or native error")
    proof.require(capture_name is not None or capture_sha is None, "capture SHA256 needs a capture name")
    inputs, executable, exe_sha, source_fact, build_fact = proof.startup_build_inputs(snapshot, scratch, project)

    process_path = scratch / "startup-process.json"
    proof.require(process_path.resolve().parent == scratch, "process receipt escapes scratch")
    if process_sha is not None:
        inputs.checked(process_path, process_sha)
    process, process_fact = inputs.report(process_path)
    proof.require("launch_error" not in process and process.get("scope") == "actual QA execution; unfinished source port",
                  "process did not record an actual unfinished startup QA run")
    proof.require(process.get("source_snapshot") == source_fact and process.get("windows_build") == build_fact,
                  "process source/build receipt fingerprints differ")
    proof.require(process.get("executable_sha256_before") == exe_sha == process.get("executable_sha256_after"),
                  "process executable fingerprint differs before/after")
    runner = fact(inputs, proof.source_path(snapshot, "tools/run-startup-qa.py"), process.get("runner_sha256"))
    proof.require(runner["sha256"] == process.get("runner_sha256"), "missing runner fingerprint")
    command = process.get("command")
    proof.require(isinstance(command, list) and command and all(isinstance(x, str) for x in command),
                  "invalid process command")
    proof.require(Path(command[0]).resolve() == executable and
                  all(command.count(flag) == 1 for flag in ("--hidden", "--muted", "--qa-new-game")),
                  "unexpected process executable or QA flags")
    proof.require(Path(argument(command, "--profile")).resolve() == scratch / "profile-absent",
                  "process profile belongs to another run")
    capture = Path(argument(command, "--qa-capture")).resolve()
    proof.require(capture == scratch / "first-scene-candidate.rgba", "unexpected process capture location")
    lock_path = scratch / "startup-run.lock"
    proof.require(lock_path.resolve().parent == scratch, "launch reservation escapes scratch")
    lock, lock_fact = inputs.report(lock_path)
    proof.require(lock.get("command") == command, "process/launch reservation mismatch")
    timestamps = [lock.get("created_utc"), process.get("created_utc"), process.get("finished_utc")]
    proof.require(all(isinstance(value, str) for value in timestamps), "missing process timestamps")
    dates = [datetime.datetime.fromisoformat(value) for value in timestamps]
    proof.require(all(value.tzinfo is not None for value in dates) and dates == sorted(dates),
                  "out-of-order process timestamps")

    hashes = process.get("outputs_sha256", {})
    proof.require(isinstance(hashes, dict), "invalid process output fingerprints")
    stdout = output_fact(inputs, scratch, "startup-newgame.json", hashes, stdout_sha)
    stderr = output_fact(inputs, scratch, "startup-newgame.stderr.log", hashes, stderr_sha)
    qa, _ = inputs.report(Path(stdout["path"]))
    proof.require(qa.get("playable_game_verified") is False and qa.get("prefix_mode") is False
                  and all(qa.get(flag) is True for flag in ("hidden", "muted", "synthetic_input")),
                  "unexpected startup QA scope or execution flags")
    code = process.get("exit_code")
    proof.require(type(code) is int and code != 0 and type(qa.get("exit_code")) is int
                  and qa["exit_code"] == code, "startup/process nonzero exit mismatch")
    proof.require(all(type(qa.get(k, 0)) is int and qa.get(k, 0) >= 0 for k in NATIVE_ERRORS[:3])
                  and all(k in qa for k in NATIVE_ERRORS[:3]), "missing or invalid native errors")
    if "effect_post_error" in qa:
        proof.require(type(qa["effect_post_error"]) is int and qa["effect_post_error"] >= 0,
                      "invalid effect postprocess error")
    proof.require(type(qa.get("ticks")) is int and qa["ticks"] > 0, "missing positive QA tick count")
    if stop is not None:
        proof.require(code == 4 and not any(qa.get(k, 0) for k in NATIVE_ERRORS),
                      "original frontier requires exit4 with no native renderer errors")
        proof.require(isinstance(qa.get("frontier_module"), str) and
                      qa["frontier_module"].strip().casefold() == stop[0] and
                      proof.address(qa.get("frontier")) == stop[1], "unexpected original frontier")
        classification = "original-frontier"
    else:
        proof.require(qa.get(error[0]) == error[1], "unexpected native error")
        classification = "native-renderer-error"

    artifacts = {"executable": {"path": str(executable), "sha256": exe_sha}}
    if capture_name is not None:
        stem = re.escape(capture.stem)
        proof.require(isinstance(capture_name, str) and
                      re.fullmatch(rf"{stem}(?:[.]scene-[0-9]+)?[.]rgba(?:[.]world[.]rgba)?", capture_name),
                      "capture must name a raw frame in this run")
        artifacts["capture"] = output_fact(inputs, scratch, capture_name, hashes, capture_sha)
    fields = ("ticks", "room", "game_state", "scene_stage", "scene_frame_serial", "scene_frames_presented",
              "frontier_module", "frontier", "host_error", "subsystem_error", "renderer_error",
              "renderer_frontier", "world_gpu_error", "world_frontier", "world_vu_pc",
              "effect_post_error", "window_error", "exit_code", "world_packets_submitted",
              "player_updates_completed", "om_updates_completed", "last_move_handle", "last_move_entry")
    observation = {key: qa[key] for key in fields if key in qa}
    trace = Path(stderr["path"]).read_text(encoding="utf-8", errors="replace").splitlines()
    control = control_observation(command, trace)
    if control is not None:
        observation["control_probe"] = control
    placement = trace_record(trace, "QA_ENEMY_PLACEMENT ")
    if placement is not None:
        proof.require(type(placement.get("handle")) is int and placement["handle"] > 0 and
                      placement["handle"] == qa.get("last_move_handle"),
                      "enemy placement does not match the last actor")
        observation["enemy_placement"] = placement
    task = trace_record(trace, "QA_SCENARIO_TASK ")
    if task is not None:
        proof.require(type(task.get("slot")) is int and 0 <= task["slot"] < 16 and
                      type(task.get("stage")) is int and 0 <= task["stage"] <= 7,
                      "invalid failed Scenario task observation")
        observation["scenario_task"] = task
    render = trace_record(trace, "GH_ACTOR_FRONTIER ")
    if render is not None:
        proof.require(type(render.get("handle")) is int and render["handle"] > 0,
                      "invalid failed Draw actor handle")
        render_pc = proof.address(render.get("frontier"))
        if render_pc != "00000000":
            proof.require(render_pc == proof.address(qa.get("frontier")),
                          "Draw diagnostic differs from the recorded original frontier")
        observation["actor_render"] = render
    result = {"schema": 1, "tool": "record-startup-frontier", "status": "recorded-unfinished-observation",
              "classification": classification, "playable_game_verified": False, "verification_success": False,
              "created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "source_snapshot": source_fact, "windows_build": build_fact, "process": process_fact,
              "launch_reservation": lock_fact, "runner": runner, "stdout": stdout, "stderr": stderr,
              "artifacts": artifacts, "observation": observation,
              "expected_frontier": expected_frontier, "expected_native_error": expected_native_error,
              "limitations": ["Records matching local evidence, without running the game or certifying completion.",
                              "An external process SHA256 can anchor output fingerprints against coordinated edits."]}
    if capture_series:
        result["captures"] = capture_observations(inputs, scratch, capture, hashes, command, trace)
    inputs.recheck()
    return result, inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=PROJECT, help="original source project named by the snapshot")
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--scratch", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    expected = parser.add_mutually_exclusive_group(required=True)
    expected.add_argument("--expected-frontier")
    expected.add_argument("--expected-native-error")
    parser.add_argument("--process-sha256")
    parser.add_argument("--stdout-sha256")
    parser.add_argument("--stderr-sha256")
    parser.add_argument("--capture", help="optional matching raw frame basename")
    parser.add_argument("--capture-sha256")
    parser.add_argument("--capture-series", action="store_true",
                        help="verify captured world/final pairs and count nonblack RGB pixels")
    args = parser.parse_args()
    try:
        output = args.output.resolve()
        proof.require(proof.under(output, proof.SCRATCH_ROOT) and output.name.endswith("-private.json")
                      and output.parent.is_dir() and not output.exists(), "output must be a fresh G scratch *-private.json")
        receipt, inputs = validate(args.snapshot, args.scratch, project=args.project,
                                   expected_frontier=args.expected_frontier,
                                   expected_native_error=args.expected_native_error,
                                   process_sha=args.process_sha256, stdout_sha=args.stdout_sha256,
                                   stderr_sha=args.stderr_sha256, capture_name=args.capture,
                                   capture_sha=args.capture_sha256, capture_series=args.capture_series)
        inputs.recheck()
        with output.open("x", encoding="utf-8") as stream:
            json.dump(receipt, stream, indent=2)
            stream.write("\n")
        print(output)
    except (proof.ProofError, OSError, ValueError) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
