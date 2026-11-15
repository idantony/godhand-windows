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
CONTROL_PROBES = ("walk", "run", "square", "approach-square", "repeat-square", "repeat-triangle", "repeat-triangle-pair", "repeat-triangle-pair-continue")


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def require_frozen_runner(snapshot):
    frozen = proof.source_path(Path(snapshot).resolve(), "tools/run-startup-qa.py")
    proof.require(proof.digest(frozen) == proof.digest(Path(__file__)),
                  "QA launcher differs from the frozen snapshot; freeze the current launcher before running")


def launch_project(snapshot):
    if PROJECT != snapshot:
        return PROJECT
    proof.require(proof.under(snapshot, proof.SCRATCH_ROOT), "frozen launcher must be below project G scratch")
    receipt, _ = proof.Inputs().report(snapshot / "analysis/source-snapshot.json")
    source = Path(receipt.get("source_project", ""))
    proof.require(source.is_absolute() and source.resolve() != snapshot,
                  "frozen launcher has no original source project")
    return source.resolve()


def prepare(snapshot_arg, scratch_arg, iso_arg, ffmpeg_arg, max_ticks=1100, capture_frame=370,
            capture_count=1, control_probe=None, title_load=False, build_scratch=None, r003_controlled=False,
            game_over_retry=False, pad_tape=None, level_confirm_tick=None, after_victory_frames=None):
    """Read-only preflight. No launch or output creation occurs on rejection."""
    snapshot, scratch = Path(snapshot_arg).resolve(), Path(scratch_arg).resolve()
    build_scratch = scratch if build_scratch is None else Path(build_scratch).resolve()
    proof.require(proof.under(snapshot, proof.SCRATCH_ROOT) and proof.under(scratch, proof.SCRATCH_ROOT),
                  "snapshot and scratch must be below project G scratch")
    proof.require(type(max_ticks) is int and 1 <= capture_frame <= max_ticks <= 100000,
                  "require 1 <= capture-frame <= max-ticks <= 100000")
    proof.require(type(capture_count) is int and 1 <= capture_count <= MAX_CAPTURE_COUNT
                  and capture_frame + capture_count - 1 <= max_ticks,
                  f"require 1 <= capture-count <= {MAX_CAPTURE_COUNT} and range within max-ticks")
    proof.require(control_probe is None or control_probe in CONTROL_PROBES, "unknown control probe")
    proof.require(not (title_load and control_probe), "gameplay input requires NewGame")
    proof.require(not (r003_controlled and (title_load or control_probe)), "controlled R003 requires its own title run")
    proof.require(not (game_over_retry and title_load), "Game Over retry requires New Game")
    proof.require(pad_tape is None or not (title_load or r003_controlled or game_over_retry or control_probe),
                  "pad tape requires standalone New Game")
    proof.require(level_confirm_tick is None or (pad_tape is not None and type(level_confirm_tick) is int
                  and 1 <= level_confirm_tick < max_ticks), "level confirmation needs a pad tape and a later tick cap")
    proof.require(after_victory_frames is None or (pad_tape is not None and type(after_victory_frames) is int
                  and 1 <= after_victory_frames < max_ticks), "post-victory capture needs a pad tape and a later tick cap")
    if pad_tape is not None:
        pad_tape = Path(pad_tape).resolve()
        proof.require(pad_tape.is_file(), "pad tape must be an existing file")
    project = launch_project(snapshot)
    iso = Path(iso_arg or project / "God Hand (USA)(Uncensor Patch v1.0).iso").resolve()
    ffmpeg = Path(ffmpeg_arg or project / ".tools/ffmpeg-9.0.2-essentials_build/bin/ffmpeg.exe").resolve()
    _, executable, exe_sha, receipt_fact, report_fact = proof.startup_build_inputs(snapshot, build_scratch, project)
    proof.require(iso.is_file() and ffmpeg.is_file(), "ISO and ffmpeg must be existing files")
    profile = scratch / ("profile-load" if title_load else "profile-absent")
    capture = scratch / "first-scene-candidate.rgba"
    outputs = [scratch / name for name in ("startup-run.lock", "startup-newgame.json", "startup-newgame.stderr.log", "startup-process.json")]
    if game_over_retry:
        outputs.extend(capture.with_suffix(suffix) for suffix in (".game-over.rgba", ".retry.rgba"))
    if pad_tape:
        outputs.extend(Path(str(capture) + suffix) for suffix in (".battle-won.rgba", ".battle-won.rgba.world.rgba"))
    if after_victory_frames:
        outputs.extend(Path(str(capture) + suffix) for suffix in (".post-victory.rgba", ".post-victory.rgba.world.rgba"))
    if title_load:
        proof.require(profile.is_dir() and profile.resolve() == profile, "QA Load requires the local scratch profile-load directory")
    else:
        proof.require(not profile.exists(), "QA profile must be absent; refusing to touch existing profile")
    proof.require(not capture.exists() and not Path(str(capture) + ".world.rgba").exists() and not any(p.exists() for p in outputs), "QA outputs already exist; use a fresh scratch cohort")
    mode = "--qa-load" if title_load else "--qa-r003-controlled" if r003_controlled else "--qa-new-game"
    command = [str(executable), "--iso", str(iso), "--profile", str(profile), "--ffmpeg", str(ffmpeg),
               "--hidden", "--muted", mode, "--qa-capture", str(capture),
               "--qa-capture-frame", str(capture_frame), "--max-ticks", str(max_ticks)]
    if capture_count > 1:
        proof.require(not any(scratch.glob(capture.stem + ".scene-*.rgba")),
                      "QA range outputs already exist; use a fresh scratch cohort")
        command.extend(("--qa-capture-count", str(capture_count)))
    if control_probe:
        command.extend(("--qa-control", control_probe))
    if game_over_retry:
        command.append("--qa-game-over-retry")
    if pad_tape:
        command.extend(("--qa-pad-tape", str(pad_tape)))
    if level_confirm_tick is not None:
        command.extend(("--qa-level-confirm-tick", str(level_confirm_tick)))
    if after_victory_frames is not None:
        command.extend(("--qa-after-victory-frames", str(after_victory_frames)))
    return {"command": command, "scratch": scratch, "build_scratch": build_scratch, "executable": executable,
            "executable_sha256": exe_sha, "source_snapshot": receipt_fact, "windows_build": report_fact,
            "control_probe": control_probe, "r003_controlled": r003_controlled, "game_over_retry": game_over_retry,
            "pad_tape": pad_tape, "pad_tape_sha256": proof.digest(pad_tape) if pad_tape else None,
            "after_victory_frames": after_victory_frames}


def game_over_observations(log, capture, exit_code):
    marker = "QA_GAME_OVER_STATUS "
    records = log.splitlines()
    lines = [line[len(marker):] for line in records if line.startswith(marker)]
    proof.require(len(lines) == 1, "missing or duplicate Game Over retry status")
    status = json.loads(lines[0])
    fields = ("room", "menu_scene", "menu_epoch", "press_tick", "confirm_edge", "continue_tick",
              "continues_before", "continues_after", "handoff_tick", "first_reloaded_scene",
              "reloaded_scene", "reloaded_epoch", "player_hp")
    proof.require(isinstance(status, dict) and
                  all(type(status.get(key)) is int and status[key] >= 0 for key in fields),
                  "invalid Game Over retry status")
    room, menu, epoch, press, edge, count, before, after, handoff, first, reloaded, new_epoch, hp = (
        status[key] for key in fields)
    proof.require((room in (0x100, 0x105) if menu else room == 0) and edge in (0, 1) and
                  (not press or menu) and (not edge or press) and
                  (not count or (edge and count > press and after == ((before + 1) & 0xffffffff))) and
                  (not handoff or (count and handoff >= count)) and
                  (not first or (handoff and first > menu)) and
                  (not reloaded or (first and reloaded >= first and new_epoch > epoch and hp > 0)) and
                  bool(reloaded) == (exit_code == 0),
                  "Game Over retry lacks an ordered native input, continue, and new presentation")
    for name, suffix, serial in (("menu", ".game-over.rgba", menu), ("retry", ".retry.rgba", reloaded)):
        path = capture.with_suffix(suffix)
        tag = f"QA_GAME_OVER_CAPTURE {name} scene={serial} room={room:04X}"
        proof.require(path.is_file() == bool(serial) and
                      (not serial or (path.stat().st_size == 512 * 448 * 4 and records.count(tag) == 1)),
                      f"Game Over {name} capture does not match a presented frame")
    return status


def pad_tape_observations(log, capture, qa, after_victory_frames=None):
    marker = "QA_R100_PAD_TAPE "
    records = [line[len(marker):] for line in log.splitlines() if line.startswith(marker)]
    proof.require(len(records) == 1, "missing or duplicate R100 pad tape status")
    status = json.loads(records[0])
    fields = ("sampled_moves", "victory_scene", "stamp19_scene", "stamp20_scene", "stamp19_handle",
              "stamp20_handle", "death_mask", "scenario_flags", "player_hp")
    proof.require(isinstance(status, dict) and all(type(status.get(key)) is int and status[key] >= 0 for key in fields),
                  "invalid R100 pad tape status")
    moves, scene, first, second, first_handle, second_handle, mask, flags, hp = (status[key] for key in fields)
    if not after_victory_frames or qa["exit_code"] == 0:
        proof.require(bool(scene) == (qa["exit_code"] == 0), "R100 pad tape victory and exit disagree")
    if scene:
        proof.require(0 < first <= scene <= qa["scene_frames_presented"] and 0 < second <= scene and
                      moves and first_handle and second_handle and first_handle != second_handle and
                      mask == 3 and flags & 0x01000000 and 0 < hp < 0x8000,
                      "R100 victory lacks original deaths, Scenario flag or visible scene")
        for suffix in (".battle-won.rgba", ".battle-won.rgba.world.rgba"):
            path = Path(str(capture) + suffix)
            proof.require(path.is_file() and path.stat().st_size == 512*448*4, "missing complete R100 victory capture")
        if not after_victory_frames:
            proof.require(qa.get("room") == "0100" and qa.get("game_state") == 4, "R100 victory left the initial room")
    if after_victory_frames:
        marker = "QA_R100_POST_VICTORY "
        records = [line[len(marker):] for line in log.splitlines() if line.startswith(marker)]
        proof.require(len(records) == 1, "missing or duplicate R100 post-victory status")
        tail = json.loads(records[0])
        fields = ("frames", "scene", "player_hp", "scenario_flags")
        proof.require(isinstance(tail, dict) and all(type(tail.get(key)) is int and tail[key] >= 0 for key in fields),
                      "invalid R100 post-victory status")
        proof.require(bool(tail["scene"]) == (qa["exit_code"] == 0), "R100 post-victory capture and exit disagree")
        if tail["scene"]:
            proof.require(qa.get("room") == "0100" and qa.get("game_state") == 4 and
                          scene < tail["scene"] == qa.get("scene_frame_serial") and
                          tail["frames"] == after_victory_frames and 0 < tail["player_hp"] < 0x8000,
                          "R100 post-victory capture lacks the requested presented frames or living player")
            for suffix in (".post-victory.rgba", ".post-victory.rgba.world.rgba"):
                path = Path(str(capture) + suffix)
                proof.require(path.is_file() and path.stat().st_size == 512*448*4,
                              "missing complete R100 post-victory capture")
        status["post_victory"] = tail
    return status


def run(prepared, *, expected_tick_limit=None):
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
    pad_tape = prepared.get("pad_tape")
    if pad_tape:
        proof.require(proof.digest(pad_tape) == prepared["pad_tape_sha256"], "pad tape changed before launch")
        receipt["pad_tape_sha256_before"] = prepared["pad_tape_sha256"]
    with (scratch / "startup-newgame.json").open("xb") as out, (scratch / "startup-newgame.stderr.log").open("xb") as err:
        try:
            child = subprocess.run(prepared["command"], stdout=out, stderr=err)
            receipt["exit_code"] = child.returncode
        except OSError as error:
            receipt["launch_error"] = str(error)
    # Fingerprint even if the child reports a renderer failure below.
    capture = Path(prepared["command"][prepared["command"].index("--qa-capture") + 1])
    captures = [capture, Path(str(capture) + ".world.rgba")]
    game_over_retry = prepared.get("game_over_retry", False)
    if game_over_retry:
        captures.extend(capture.with_suffix(suffix) for suffix in (".game-over.rgba", ".retry.rgba"))
    if pad_tape:
        receipt["pad_tape_sha256_after"] = proof.digest(pad_tape)
        captures.extend(Path(str(capture) + suffix) for suffix in (".battle-won.rgba", ".battle-won.rgba.world.rgba"))
    after_victory_frames = prepared.get("after_victory_frames")
    if after_victory_frames:
        captures.extend(Path(str(capture) + suffix) for suffix in (".post-victory.rgba", ".post-victory.rgba.world.rgba"))
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
    if pad_tape:
        proof.require(receipt["pad_tape_sha256_before"] == receipt["pad_tape_sha256_after"],
                      "pad tape changed during execution")
    qa = json.loads((scratch / "startup-newgame.json").read_text(encoding="utf-8-sig"))
    proof.require(qa.get("hidden") is True and qa.get("muted") is True and qa.get("synthetic_input") is True
                  and qa.get("prefix_mode") is False, "QA did not confirm expected execution flags")
    control = prepared.get("control_probe")
    r003_controlled = prepared.get("r003_controlled", False)
    continue_after_pair = control == "repeat-triangle-pair-continue"
    pair = continue_after_pair or control == "repeat-triangle-pair"
    repeated_attack = pair or control in ("repeat-square", "repeat-triangle")
    allow_bounded = repeated_attack or r003_controlled or game_over_retry or pad_tape or expected_tick_limit is not None
    valid_exit_codes = (0, 3, 4) if allow_bounded else (0, 4)
    proof.require(qa.get("exit_code") == receipt["exit_code"] and receipt["exit_code"] in valid_exit_codes,
                  "unexpected child exit; inspect QA output")
    for field in ("renderer_error", "world_gpu_error", "window_error"):
        proof.require(qa.get(field) == 0, "QA reported " + field)
    summary = {"exit_code": receipt["exit_code"], "frontier": qa.get("frontier"), "ticks": qa.get("ticks"),
               "world_packets_submitted": qa.get("world_packets_submitted"), "receipt": str(scratch / "startup-process.json")}
    if expected_tick_limit is not None:
        command = prepared["command"]
        proof.require_clean_startup_tick_limit(
            qa, receipt["exit_code"], command[command.index("--max-ticks") + 1], expected_tick_limit)
        summary["classification"] = "tick-limit"
    if r003_controlled:
        proof.require(qa.get("effect_post_error") == 0, "controlled R003 reported effect_post_error")
        log = (scratch / "startup-newgame.stderr.log").read_text(encoding="utf-8-sig")
        summary.update(proof.controlled_room_observations(log, qa))
    if game_over_retry:
        log = (scratch / "startup-newgame.stderr.log").read_text(encoding="utf-8-sig")
        summary["game_over_retry"] = game_over_observations(log, capture, receipt["exit_code"])
    if pad_tape:
        log = (scratch / "startup-newgame.stderr.log").read_text(encoding="utf-8-sig")
        summary["pad_tape"] = pad_tape_observations(log, capture, qa, after_victory_frames)
    if repeated_attack:
        marker = "QA_REPEAT_STATUS "
        with (scratch / "startup-newgame.stderr.log").open(encoding="utf-8", errors="replace") as stream:
            status_lines = [line[len(marker):] for line in stream if line.startswith(marker)]
        proof.require(len(status_lines) == 1, "missing or duplicate repeated attack status")
        status = json.loads(status_lines[0])
        defeat_scene = status.get("defeat_scene")
        proof.require(type(defeat_scene) is int and defeat_scene >= 0
                      and (game_over_retry or continue_after_pair or bool(defeat_scene) == (receipt["exit_code"] == 0)),
                      "repeated attack exit and presented defeat disagree")
        if pair and defeat_scene:
            first, second = (status.get(key) for key in ("stamp19_scene", "stamp20_scene"))
            first_handle, second_handle = (status.get(key) for key in ("stamp19_handle", "stamp20_handle"))
            proof.require(type(first) is int and type(second) is int and 0 < first < second < defeat_scene
                          and type(first_handle) is int and type(second_handle) is int
                          and first_handle > 0 and second_handle > 0 and first_handle != second_handle
                          and status.get("room_dead_mask") == 3,
                          "pair success lacks two ordered presented defeats")
        if continue_after_pair:
            with (scratch / "startup-newgame.stderr.log").open(encoding="utf-8", errors="replace") as stream:
                marker = "QA_R100_EXIT_STATUS "
                exit_lines = [line[len(marker):] for line in stream if line.startswith(marker)]
            proof.require(len(exit_lines) == 1, "missing or duplicate R100 exit status")
            exit_status = json.loads(exit_lines[0])
            fields = ("door_tick", "continue_scene", "state5_tick", "room3_scene",
                      "door_presses", "continue_presses", "route_index", "battle_gate_waiting")
            proof.require(all(type(exit_status.get(k)) is int and exit_status[k] >= 0 for k in fields),
                          "invalid R100 exit status")
            door, menu, state5, room3 = (exit_status[k] for k in fields[:4])
            proof.require((not door or (defeat_scene and exit_status["door_presses"])) and
                          (not menu or (door and menu > defeat_scene)) and
                          (not state5 or (door and menu and state5 > door and exit_status["continue_presses"])) and
                          (not room3 or (state5 and room3 > menu)) and
                          (game_over_retry or bool(room3) == (receipt["exit_code"] == 0)),
                          "R100 continuation lacks ordered A38, presented Continue, state5 and R003 proof")
            status["r100_exit"] = exit_status
        summary[control.replace("-", "_")] = status
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--scratch", type=Path, required=True)
    parser.add_argument("--build-scratch", type=Path, help="reuse the exact build in this G: scratch cohort")
    parser.add_argument("--iso", type=Path)
    parser.add_argument("--ffmpeg", type=Path)
    parser.add_argument("--max-ticks", type=int, default=1100)
    parser.add_argument("--expect-tick-limit", action="store_true",
                        help="require a clean bounded exit at max-ticks with presented scene frames")
    parser.add_argument("--capture-frame", type=int, default=370)
    parser.add_argument("--capture-count", type=int, default=1,
                        help=f"capture up to {MAX_CAPTURE_COUNT} ready scene frames beginning at capture-frame")
    parser.add_argument("--control-probe", choices=CONTROL_PROBES,
                        help="send original pad input after EV01 and two neutral player updates")
    parser.add_argument("--title-load", action="store_true", help="select Load Game using the scratch profile-load directory")
    parser.add_argument("--r003-controlled", action="store_true", help="isolate R003 using the original A38 destination at title handoff")
    parser.add_argument("--game-over-retry", action="store_true", help="confirm the presented Game Over panel and capture the reloaded room")
    parser.add_argument("--pad-tape", type=Path, help="replay recorded R100 gamepad samples by completed player Move")
    parser.add_argument("--level-confirm-tick", type=int,
                        help="delay the original New Game difficulty confirmation to this replay tick")
    parser.add_argument("--after-victory-frames", type=int,
                        help="keep replaying R100 for this many presented frames after both opening enemies die")
    args = parser.parse_args()
    try:
        prepared = prepare(args.snapshot, args.scratch, args.iso, args.ffmpeg, args.max_ticks, args.capture_frame,
                           args.capture_count, args.control_probe, args.title_load, args.build_scratch, args.r003_controlled,
                           args.game_over_retry, args.pad_tape, args.level_confirm_tick, args.after_victory_frames)
        require_frozen_runner(args.snapshot)
        expected = args.max_ticks if args.expect_tick_limit else None
        print(json.dumps(run(prepared, expected_tick_limit=expected)), flush=True)
    except (proof.ProofError, OSError, ValueError) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
