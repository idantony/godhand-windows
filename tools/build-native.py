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
               ("font_draw", "src/core/font_draw.c"), ("game_gate", "src/core/game_gate.c")]
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
                     "tests/core/test_font_draw.c", "tests/core/test_game_gate.c"}
    for _, source in modules:
        source_inputs.add(source)
        source_inputs.add(str(Path(source).with_suffix(".h")).replace("\\", "/"))
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
    host_objects = [objects[name] for name in ("startup_host", "audio_startup", "sound_bank", "startup_task", "dvd_jobs", "disc", "memory", "tasks", "task_threads", "font_state", "menu_state", "core_bundle")]
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
        run("test-original-disc", [output / "test_disc.exe", baseline["source_iso"]["path"], PROJECT / "reference" / "overlays" / "r207.rel"])
        run("test-dvd-jobs", [output / "test_dvd_jobs.exe", baseline["source_iso"]["path"], PROJECT / "reference" / "overlays" / "r207.rel"])
        run("test-core-bundle", [output / "test_core_bundle.exe", baseline["source_iso"]["path"]])
        run("test-sound-bank", [output / "test_sound_bank.exe", baseline["source_iso"]["path"]])
        run("test-font-state", [output / "test_font_state.exe", baseline["source_iso"]["path"]])
        run("test-menu-state", [output / "test_menu_state.exe", baseline["source_iso"]["path"]])
        run("test-audio-startup", [output / "test_audio_startup.exe", baseline["source_iso"]["path"]])
        run("test-startup-host", [output / "test_startup_host.exe", baseline["source_iso"]["path"]])
        run("test-font-texture", [output / "test_font_texture.exe", baseline["source_iso"]["path"]])
        run("test-game-start", [output / "test_game_start.exe", baseline["source_iso"]["path"], PROJECT / "reference" / "SLUS_215.03"])
        run("test-memory-card", [output / "test_memory_card.exe", card_scratch])
        run("test-mc-id", [output / "test_mc_boot_id.exe", baseline["source_iso"]["path"]])
        run("test-font-draw", [output / "test_font_draw.exe", baseline["source_iso"]["path"]])
        run("test-game-gate", [output / "test_game_gate.exe", baseline["source_iso"]["path"]])
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
