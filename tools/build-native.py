"""Build and run the native source-port modules; all intermediates stay on G:."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

PROJECT = Path(__file__).resolve().parents[1]
SCRATCH = Path(r"G:\middle_thing\godhand_windows")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-tests", action="store_true")
    parser.add_argument("--hardware-audio", action="store_true", help="Run muted real-device voice and MC-flow checks")
    parser.add_argument("--movie-codec", type=Path, help="Also run the muted Movie0 native codec/device check")
    parser.add_argument("--compiler", type=Path, default=PROJECT / ".tools" / "zig-x86_64-windows-0.17.1" / "zig.exe", help="Zig executable; permits reusing an installed toolchain")
    parser.add_argument("--scratch", type=Path, default=SCRATCH, help="External build/cache root")
    args = parser.parse_args()
    compiler, scratch = args.compiler.resolve(), args.scratch.resolve()
    try:
        scratch.relative_to(PROJECT)
    except ValueError:
        pass
    else:
        raise SystemExit("Build scratch must be outside the permanent project tree")
    output = scratch / "build" / "native"
    logs = scratch / "logs" / "native"
    (PROJECT / "analysis").mkdir(parents=True, exist_ok=True)
    card_scratch = scratch / "build" / "memory-card-tests" / "scratch"
    for path in (output, logs, card_scratch, scratch / "cache" / "tmp", scratch / "cache" / "zig-global", scratch / "cache" / "zig-local"):
        path.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update({"TEMP": str(scratch / "cache" / "tmp"), "TMP": str(scratch / "cache" / "tmp"),
                "ZIG_GLOBAL_CACHE_DIR": str(scratch / "cache" / "zig-global"),
                "ZIG_LOCAL_CACHE_DIR": str(scratch / "cache" / "zig-local"), "PYTHONDONTWRITEBYTECODE": "1"})
    manifest = {"created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                "compiler": str(compiler), "compiler_version": subprocess.check_output([str(compiler), "version"], text=True).strip(),
                "target": "x86_64-windows-gnu", "status": "running", "steps": [],
                "is_playable_game": False, "purpose": "Verify recovered game functions and native original-disc resource access"}
    report = PROJECT / "analysis" / "native-verification.json"

    def run(label, command):
        if len(command)>1 and command[0]==compiler and command[1] in ("cc", "c++") and "-c" not in command:
            command = command + ["-lwinmm"]
        started = time.monotonic()
        result = subprocess.run([str(item) for item in command], cwd=PROJECT, env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
        log = logs / (label + ".log")
        log.write_text(result.stdout, encoding="utf-8")
        step = {"name": label, "command": [str(item) for item in command], "returncode": result.returncode,
                "seconds": round(time.monotonic() - started, 3), "log": str(log), "output": result.stdout[-8000:]}
        manifest["steps"].append(step)
        report.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        print(label + ": " + ("PASS" if result.returncode == 0 else "FAIL"), flush=True)
        if result.returncode:
            manifest["status"] = "failed"
            report.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            print(result.stdout, flush=True)
            raise SystemExit(result.returncode)

    common = [compiler, "cc", "-target", "x86_64-windows-gnu", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
              "-ffp-contract=off", "-fno-fast-math", "-D_CRT_SECURE_NO_WARNINGS"]
    modules = [("rng", "src/core/rng.c"), ("boot", "src/core/boot.c"),
               ("disc", "src/platform/disc.c"), ("r207", "src/rooms/r207.c"),
               ("memory", "src/platform/memory.c"), ("tasks", "src/core/tasks.c"),
               ("task_threads", "src/platform/task_threads.c"), ("tim2", "src/assets/tim2.c"),
               ("model", "src/assets/model.c"), ("startup_task", "src/core/startup_task.c"),
               ("dvd_jobs", "src/core/dvd_jobs.c"), ("core_bundle", "src/assets/core_bundle.c"),
               ("sound_bank", "src/assets/sound_bank.c"), ("font_state", "src/core/font_state.c"),
               ("menu_state", "src/core/menu_state.c"), ("audio_startup", "src/platform/audio_startup.c"),
               ("startup_host", "src/platform/startup_host.c"), ("font_texture", "src/assets/font_texture.c"),
               ("game_start", "src/core/game_start.c"), ("memory_card", "src/platform/memory_card.c"),
               ("mc_boot", "src/core/mc_boot.c"), ("mc_boot_id", "src/core/mc_boot_id.c"),
               ("font_draw", "src/core/font_draw.c"), ("game_gate", "src/core/game_gate.c"),
               ("id_base", "src/core/id_base.c"), ("packed_message", "src/core/packed_message.c"),
               ("game_init", "src/core/game_init.c"), ("mc_task", "src/core/mc_task.c"),
               ("id_jobs", "src/core/id_jobs.c"), ("id_jobs_texture", "src/core/id_jobs_texture.c"),
               ("id_jobs_primitive", "src/core/id_jobs_primitive.c"), ("mc_control", "src/core/mc_control.c"),
               ("game_stage", "src/core/game_stage.c"), ("game_children", "src/core/game_children.c"),
               ("id_text", "src/core/id_text.c"), ("id_path", "src/core/id_path.c"),
               ("mc_error", "src/core/mc_error.c"), ("mc_file_load", "src/core/mc_file_load.c"),
               ("game_cockpit", "src/core/game_cockpit.c"), ("game_objects", "src/core/game_objects.c"),
               ("game_prepare", "src/core/game_prepare.c"), ("game_room_state", "src/core/game_room_state.c"),
               ("game_room_pack", "src/core/game_room_pack.c"), ("game_room_pools", "src/core/game_room_pools.c"),
               ("movie_task", "src/core/movie_task.c"), ("game_camera", "src/core/game_camera.c"),
               ("game_movie_pool", "src/core/game_movie_pool.c"), ("game_services", "src/platform/game_services.c"),
               ("mc_exit", "src/core/mc_exit.c"), ("mc_control_flow", "src/core/mc_control_flow.c"),
               ("audio_events", "src/platform/audio_events.c"), ("se_voice", "src/core/se_voice.c"),
               ("sound_control", "src/core/sound_control.c"), ("video_player", "src/platform/video_player.c"),
               ("native_input", "src/platform/native_input.c")]
    extra_tests = [
        ("nested-tasks", "tests/platform/test_nested_tasks.c", "none"),
        ("id-path", "tests/core/test_id_path.c", "iso"),
        ("mc-error", "tests/core/test_mc_error.c", "iso"),
        ("mc-file-load", "tests/core/test_mc_file_load.c", "none"),
        ("game-cockpit", "tests/core/test_game_cockpit.c", "both"),
        ("game-objects", "tests/core/test_game_objects.c", "both"),
        ("game-prepare", "tests/core/test_game_prepare.c", "none"),
        ("game-room-state", "tests/core/test_game_room_state.c", "elf"),
        ("game-room-pack", "tests/core/test_game_room_pack.c", "iso"),
        ("game-room-pools", "tests/core/test_game_room_pools.c", "iso"),
        ("movie-task", "tests/core/test_movie_task.c", "none"),
        ("game-camera", "tests/core/test_game_camera.c", "elf"),
        ("game-movie-pool", "tests/core/test_game_movie_pool.c", "none"),
        ("game-services", "tests/platform/test_game_services.c", "iso"),
        ("mc-exit", "tests/core/test_mc_exit.c", "iso"),
        ("memory-card-end", "tests/platform/test_memory_card_end.c", "none"),
        ("mc-control-flow", "tests/core/test_mc_control_flow.c", "iso"),
        ("audio-events", "tests/platform/test_audio_events.c", "iso"),
        ("se-voice", "tests/core/test_se_voice.c", "iso"),
        ("sound-control", "tests/core/test_sound_control.c", "iso"),
        ("video-player", "tests/platform/test_video_player.c", "movie"),
        ("native-input", "tests/platform/test_native_input.c", "iso"),
    ]
    source_inputs = {"tools/build-native.py", "tests/test_cpp_abi.cpp", "tests/core/test_rng.c",
                     "tests/core/test_boot.c", "tests/platform/test_disc.c", "tests/rooms/test_r207.c",
                     "tests/platform/test_memory.c", "tests/core/test_tasks.c",
                     "tests/platform/test_task_threads.c", "tests/assets/test_tim2.c", "tools/decode-tim2.c",
                     "tests/assets/test_model.c", "tests/core/test_startup_task.c", "tests/core/test_dvd_jobs.c",
                     "tests/assets/test_core_bundle.c", "tests/assets/test_sound_bank.c",
                     "tests/core/test_font_state.c", "tests/core/test_menu_state.c",
                     "tests/platform/test_audio_startup.c", "tests/platform/test_startup_host.c", "tools/run-startup.c",
                     "tests/assets/test_font_texture.c", "tools/font-texture-qa.c", "tests/core/test_game_start.c",
                     "tests/platform/test_memory_card.c", "tests/core/test_mc_boot_id.c",
                     "tests/core/test_font_draw.c", "tests/core/test_game_gate.c",
                     "tests/core/test_id_base.c", "tests/core/test_packed_message.c", "tests/core/test_game_init.c",
                     "tests/core/test_mc_task.c", "tests/core/test_id_jobs.c", "tests/core/test_id_texture.c",
                     "tests/core/test_mc_control.c", "tests/core/test_game_stage.c", "tests/core/test_game_children.c",
                     "tests/core/test_id_text.c"}
    shared_headers = {"id_jobs_texture": "src/core/id_jobs.h", "id_jobs_primitive": "src/core/id_jobs.h"}
    source_inputs.update(source for _, source, _ in extra_tests)
    for name, source in modules:
        source_inputs.add(source)
        source_inputs.add(shared_headers.get(name, str(Path(source).with_suffix(".h")).replace("\\", "/")))
    initial_sources = {name: hashlib.sha256((PROJECT / name).read_bytes()).hexdigest() for name in sorted(source_inputs)}
    objects = {}
    for name, source in modules:
        objects[name] = output / (name + ".o")
        run("compile-" + name, common + ["-c", source, "-o", objects[name]])
    run("compile-rng-tests", common + ["-Isrc/core", objects["rng"], "tests/core/test_rng.c", "-o", output / "test_rng.exe"])
    run("compile-disc-tests", common + [objects["disc"], "tests/platform/test_disc.c", "-o", output / "test_disc.exe"])
    run("compile-boot-tests", common + ["-Isrc/core", objects["boot"], "tests/core/test_boot.c", "-o", output / "test_boot.exe"])
    run("compile-room-tests", common + [objects["r207"], "tests/rooms/test_r207.c", "-o", output / "test_r207.exe"])
    run("compile-memory-tests", common + [objects["memory"], objects["r207"], "tests/platform/test_memory.c", "-o", output / "test_memory.exe"])
    run("compile-task-tests", common + ["-Isrc/core", objects["tasks"], "tests/core/test_tasks.c", "-o", output / "test_tasks.exe"])
    run("compile-task-thread-tests", common + ["-Isrc/platform", objects["tasks"], objects["task_threads"], "tests/platform/test_task_threads.c", "-o", output / "test_task_threads.exe"])
    run("compile-tim2-tests", common + [objects["tim2"], "tests/assets/test_tim2.c", "-o", output / "test_tim2.exe"])
    run("compile-tim2-decoder", common + [objects["tim2"], "tools/decode-tim2.c", "-o", output / "decode-tim2.exe"])
    run("compile-model-tests", common + [objects["model"], "tests/assets/test_model.c", "-o", output / "test_model.exe"])
    run("compile-startup-tests", common + ["-Isrc/core", objects["startup_task"], "tests/core/test_startup_task.c", "-o", output / "test_startup_task.exe"])
    run("compile-dvd-tests", common + ["-Isrc/core", objects["dvd_jobs"], objects["disc"], objects["memory"], objects["r207"], objects["tasks"], objects["task_threads"], "tests/core/test_dvd_jobs.c", "-o", output / "test_dvd_jobs.exe"])
    run("compile-bundle-tests", common + [objects["core_bundle"], objects["tim2"], objects["disc"], "tests/assets/test_core_bundle.c", "-o", output / "test_core_bundle.exe"])
    run("compile-sound-bank-tests", common + ["-Isrc/assets", "-Isrc/core", objects["sound_bank"], objects["disc"], "tests/assets/test_sound_bank.c", "-o", output / "test_sound_bank.exe"])
    run("compile-font-state-tests", common + [objects["font_state"], objects["core_bundle"], objects["disc"], "tests/core/test_font_state.c", "-o", output / "test_font_state.exe"])
    run("compile-menu-state-tests", common + ["-Isrc/core", objects["menu_state"], objects["core_bundle"], objects["dvd_jobs"], objects["disc"], objects["memory"], objects["tasks"], objects["task_threads"], "tests/core/test_menu_state.c", "-o", output / "test_menu_state.exe"])
    run("compile-audio-startup-tests", common + ["-Isrc/platform", objects["audio_startup"], objects["sound_bank"], objects["disc"], "tests/platform/test_audio_startup.c", "-o", output / "test_audio_startup.exe"])
    host_objects = list(objects.values())
    run("compile-startup-host-tests", common + host_objects + ["tests/platform/test_startup_host.c", "-o", output / "test_startup_host.exe"])
    run("compile-startup-host-cli", common + host_objects + ["-municode", "tools/run-startup.c", "-o", output / "run-startup.exe"])
    font_objects = [objects[name] for name in ("font_texture", "font_state", "core_bundle", "disc")]
    run("compile-font-texture-tests", common + font_objects + ["tests/assets/test_font_texture.c", "-o", output / "test_font_texture.exe"])
    run("compile-font-qa-cli", common + font_objects + ["tools/font-texture-qa.c", "-o", output / "font-texture-qa.exe"])
    run("compile-game-start-tests", common + ["-Isrc/core"] + [objects[name] for name in ("game_start", "dvd_jobs", "memory", "disc", "tasks", "task_threads")] + ["tests/core/test_game_start.c", "-o", output / "test_game_start.exe"])
    run("compile-memory-card-tests", common + [objects[name] for name in ("memory_card", "mc_boot", "memory", "tasks", "task_threads")] + ["tests/platform/test_memory_card.c", "-o", output / "test_memory_card.exe"])
    run("compile-mc-id-tests", common + ["-Isrc/core"] + [objects[name] for name in ("mc_boot_id", "menu_state", "core_bundle", "dvd_jobs", "memory", "tasks", "task_threads", "disc")] + ["tests/core/test_mc_boot_id.c", "-o", output / "test_mc_boot_id.exe"])
    run("compile-font-draw-tests", common + [objects["font_draw"], objects["font_state"], objects["core_bundle"], objects["disc"], "tests/core/test_font_draw.c", "-o", output / "test_font_draw.exe"])
    run("compile-game-gate-tests", common + ["-Isrc/core"] + [objects[name] for name in ("game_gate", "game_start", "dvd_jobs", "memory", "tasks", "task_threads", "disc")] + ["tests/core/test_game_gate.c", "-o", output / "test_game_gate.exe"])
    run("compile-id-base-tests", common + ["-Isrc/core"] + [objects[name] for name in ("id_base", "id_path", "mc_boot_id", "menu_state", "core_bundle", "dvd_jobs", "memory", "tasks", "task_threads", "disc")] + ["tests/core/test_id_base.c", "-o", output / "test_id_base.exe"])
    run("compile-packed-message-tests", common + [objects[name] for name in ("packed_message", "id_base", "id_path", "font_state", "core_bundle", "menu_state", "disc")] + ["tests/core/test_packed_message.c", "-o", output / "test_packed_message.exe"])
    run("compile-game-init-tests", common + ["-Isrc/core", objects["game_init"], "tests/core/test_game_init.c", "-o", output / "test_game_init.exe"])
    run("compile-mc-task-tests", common + ["-Isrc/core"] + host_objects + ["tests/core/test_mc_task.c", "-o", output / "test_mc_task.exe"])
    id_objects = [objects[name] for name in ("id_jobs", "id_jobs_texture", "id_jobs_primitive", "id_base", "id_path", "menu_state", "font_state", "font_texture", "core_bundle", "disc", "tim2")]
    run("compile-id-jobs-tests", common + id_objects + ["tests/core/test_id_jobs.c", "-o", output / "test_id_jobs.exe"])
    run("compile-id-texture-tests", common + id_objects + ["tests/core/test_id_texture.c", "-o", output / "test_id_texture.exe"])
    run("compile-mc-control-tests", common + ["-Isrc/core"] + host_objects + ["tests/core/test_mc_control.c", "-o", output / "test_mc_control.exe"])
    run("compile-game-stage-tests", common + ["-Isrc/core"] + host_objects + ["tests/core/test_game_stage.c", "-o", output / "test_game_stage.exe"])
    run("compile-game-children-tests", common + ["-Isrc/core"] + [objects[name] for name in ("game_children", "id_base", "id_path", "core_bundle")] + ["tests/core/test_game_children.c", "-o", output / "test_game_children.exe"])
    run("compile-id-text-tests", common + [objects["id_text"], objects["font_draw"], objects["packed_message"]] + id_objects + ["tests/core/test_id_text.c", "-o", output / "test_id_text.exe"])
    for name, source, _ in extra_tests:
        run("compile-" + name + "-tests", common + ["-Isrc/core", "-Isrc/platform"] + list(objects.values()) + [source, "-o", output / ("test_" + name.replace("-", "_") + ".exe")])
    run("compile-core-dll", common + ["-shared", "-Wl,--export-all-symbols"] + list(objects.values()) + ["-o", output / "godhand_core.dll"])
    cpp = [compiler, "c++", "-target", "x86_64-windows-gnu", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror"]
    run("compile-cpp-abi", cpp + list(objects.values()) + ["tests/test_cpp_abi.cpp", "-o", output / "test_cpp_abi.exe"])
    if not args.skip_tests:
        run("test-rng", [output / "test_rng.exe"])
        run("test-boot", [output / "test_boot.exe"])
        run("test-room", [output / "test_r207.exe"])
        run("test-memory", [output / "test_memory.exe"])
        run("test-tasks", [output / "test_tasks.exe"])
        run("test-task-threads", [output / "test_task_threads.exe"])
        run("test-tim2", [output / "test_tim2.exe", PROJECT / "reference" / "render" / "face_hm00.tm2"])
        run("test-model", [output / "test_model.exe", PROJECT / "reference" / "model" / "om0e.scr"])
        run("test-startup-task", [output / "test_startup_task.exe"])
        run("test-cpp-abi", [output / "test_cpp_abi.exe"])
        baseline = json.loads((PROJECT / "analysis" / "disc-baseline.json").read_text(encoding="utf-8"))
        run("test-original-disc", [output / "test_disc.exe", baseline["source_iso"]["path"], PROJECT / "reference" / "overlays" / "r207.rel", PROJECT / "reference" / "SLUS_215.03"])
        run("test-dvd-jobs", [output / "test_dvd_jobs.exe", baseline["source_iso"]["path"], PROJECT / "reference" / "overlays" / "r207.rel"])
        run("test-core-bundle", [output / "test_core_bundle.exe", baseline["source_iso"]["path"]])
        run("test-sound-bank", [output / "test_sound_bank.exe", baseline["source_iso"]["path"]])
        run("test-font-state", [output / "test_font_state.exe", baseline["source_iso"]["path"]])
        run("test-menu-state", [output / "test_menu_state.exe", baseline["source_iso"]["path"]])
        run("test-audio-startup", [output / "test_audio_startup.exe", baseline["source_iso"]["path"]])
        run("test-startup-host", [output / "test_startup_host.exe", baseline["source_iso"]["path"]] + (["--hardware-audio"] if args.hardware_audio else []))
        run("test-font-texture", [output / "test_font_texture.exe", baseline["source_iso"]["path"]])
        run("test-game-start", [output / "test_game_start.exe", baseline["source_iso"]["path"], PROJECT / "reference" / "SLUS_215.03"])
        run("test-memory-card", [output / "test_memory_card.exe", card_scratch])
        run("test-mc-id", [output / "test_mc_boot_id.exe", baseline["source_iso"]["path"]])
        run("test-font-draw", [output / "test_font_draw.exe", baseline["source_iso"]["path"]])
        run("test-game-gate", [output / "test_game_gate.exe", baseline["source_iso"]["path"]])
        run("test-id-base", [output / "test_id_base.exe", baseline["source_iso"]["path"]])
        run("test-packed-message", [output / "test_packed_message.exe", baseline["source_iso"]["path"]])
        run("test-game-init", [output / "test_game_init.exe"])
        run("test-mc-task", [output / "test_mc_task.exe", baseline["source_iso"]["path"]])
        run("test-id-jobs", [output / "test_id_jobs.exe", baseline["source_iso"]["path"]])
        run("test-id-texture", [output / "test_id_texture.exe", baseline["source_iso"]["path"]])
        run("test-mc-control", [output / "test_mc_control.exe", baseline["source_iso"]["path"]])
        run("test-game-stage", [output / "test_game_stage.exe", baseline["source_iso"]["path"]])
        run("test-game-children", [output / "test_game_children.exe", PROJECT / "reference" / "SLUS_215.03"])
        run("test-id-text", [output / "test_id_text.exe", baseline["source_iso"]["path"]])
        for name, _, inputs in extra_tests:
            if (name in ("se-voice", "mc-control-flow") and not args.hardware_audio) or (inputs == "movie" and not args.movie_codec):
                manifest.setdefault("skipped_checks", []).append({"name": "test-" + name, "reason": "Explicit hardware/codec opt-in not supplied"})
                continue
            arguments = []
            if inputs in ("iso", "both", "movie"):
                arguments.append(baseline["source_iso"]["path"])
            if inputs in ("elf", "both"):
                arguments.append(PROJECT / "reference" / "SLUS_215.03")
            if inputs == "movie":
                arguments.append(args.movie_codec.resolve())
            run("test-" + name, [output / ("test_" + name.replace("-", "_") + ".exe")] + arguments)
    manifest["status"] = "passed" if not args.skip_tests else "build-only"
    final_sources = {name: hashlib.sha256((PROJECT / name).read_bytes()).hexdigest() for name in sorted(source_inputs)}
    if initial_sources != final_sources:
        manifest["status"] = "failed-source-changed-during-build"
        report.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        raise SystemExit("Source changed during compilation/testing; rebuild the stabilized inputs")
    manifest["source_sha256"] = final_sources
    manifest["module_names"] = [name for name, _ in modules]
    manifest["artifacts"] = [{"path": str(path), "size": path.stat().st_size,
                              "sha256": hashlib.sha256(path.read_bytes()).hexdigest()} for path in sorted(output.glob("*")) if path.is_file()]
    manifest["limitations"] = ["Developer startup-task execution slice reaches an explicit next-task frontier; no complete boot/game loop or game renderer yet",
                               "Audio startup commits real owned banks and AFS indexes, without voices or audible output",
                               "Serial task sleep/start/exit use native fibers; state4, async EE syscalls, interrupt/priority preemption remain unsupported",
                               "Original cHeap capacity/fragmentation and final GS indexed texture blending remain unverified",
                               "PS2 numerical reference is an implementation model, not a hardware capture",
                               "No full-ELF retail matching claim for the patched disc"]
    report.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("Report: " + str(report), flush=True)


if __name__ == "__main__":
    main()
