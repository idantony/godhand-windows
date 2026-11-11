"""Build native Windows presentation tools; all outputs and caches stay outside the project."""
import argparse
import ast
import concurrent.futures
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import threading
from compiler_response import write_response_file
from compiled_objects import (OBJECT_CACHE_DIRECTORY, WINDOWS_COMPILER_FLAGS, ObjectCache,
                              c_driver, project_inputs, source_hashes)

PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_SCRATCH = Path(r"G:\middle_thing\godhand_windows")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", choices=("mc-frame", "title-ui", "native", "startup-qa", "startup-cpu-qa",
                        "model-qa", "world-qa", "scene-message-qa", "save-map-qa"), default="mc-frame")
    parser.add_argument("--scratch", type=Path, default=DEFAULT_SCRATCH)
    parser.add_argument("--jobs", type=int, default=4, help="Independent C compiler processes (1..16)")
    parser.add_argument("--compiler-cache", type=Path, default=DEFAULT_SCRATCH / "cache/zig-global",
                        help="Shared Zig toolchain cache")
    parser.add_argument("--compiler", type=Path, default=PROJECT / ".tools/zig-x86_64-windows-0.17.1/zig.exe")
    parser.add_argument("--sdk-lib", type=Path, help="Windows SDK um/x64 import-library directory")
    parser.add_argument("--object-cache", type=Path, default=DEFAULT_SCRATCH / OBJECT_CACHE_DIRECTORY,
                        help="Share unchanged compiled objects with Windows and native verification builds")
    parser.add_argument("--no-object-cache", action="store_true", help="Compile every object afresh")
    args = parser.parse_args()
    if not 1 <= args.jobs <= 16:
        parser.error("--jobs must be in 1..16")
    scratch = args.scratch.resolve()
    if scratch == PROJECT or PROJECT in scratch.parents:
        parser.error("Scratch must stay outside the permanent project tree")
    sdk = args.sdk_lib
    if sdk is None:
        versions = list(Path(r"C:\Program Files (x86)\Windows Kits\10\Lib").glob("*/um/x64"))
        versions = [v for v in versions if (v / "d3d11.lib").is_file()]
        if not versions:
            parser.error("Install Windows SDK or supply --sdk-lib")
        sdk = max(versions, key=lambda v: tuple(int(p) for p in v.parents[1].name.split(".")))
    sdk = sdk.resolve()
    if not sdk.is_dir():
        parser.error(f"Windows SDK library directory does not exist: {sdk}")
    compiler = args.compiler.resolve()
    cache_root = args.object_cache.resolve()
    if not args.no_object_cache and (cache_root == PROJECT or PROJECT in cache_root.parents):
        parser.error("Object cache must stay outside the source snapshot")
    build = scratch / "build" / ("windows-" + args.target)
    logs = scratch / "logs" / ("windows-" + args.target)
    for p in (build, logs, scratch / "cache/tmp", args.compiler_cache, scratch / "cache/zig-local"):
        p.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(TEMP=str(scratch / "cache/tmp"), TMP=str(scratch / "cache/tmp"),
               ZIG_GLOBAL_CACHE_DIR=str(args.compiler_cache.resolve()), ZIG_LOCAL_CACHE_DIR=str(scratch / "cache/zig-local"))
    # One source-of-truth core module list; never link stale objects from a
    # different ABI or silently trust a previous test executable.
    tree = ast.parse((PROJECT / "tools/build-native.py").read_text(encoding="utf-8"))
    modules = next(ast.literal_eval(n.value) for n in ast.walk(tree) if isinstance(n, ast.Assign)
                   and any(isinstance(t, ast.Name) and t.id == "modules" for t in n.targets))
    cpp_sources = ["src/platform/ui_text_renderer.cpp", "src/platform/ui_sprite_renderer.cpp",
                   "src/platform/ui_polygon_renderer.cpp", "src/platform/ui_fade_renderer.cpp", "src/platform/mc_renderer.cpp"]
    if args.target == "model-qa":
        modules = [("disc", "src/platform/disc.c"), ("core_bundle", "src/assets/core_bundle.c"),
                   ("tm3", "src/assets/tm3.c")]
        cpp_sources = ["src/platform/model_renderer.cpp", "tests/platform/test_model_renderer.cpp"]
    elif args.target == "world-qa":
        cpp_sources += ["src/platform/model_renderer.cpp", "tests/platform/test_world_composition.cpp"]
    elif args.target == "scene-message-qa":
        cpp_sources += ["tests/platform/test_scene_message_depth.cpp"]
    elif args.target == "save-map-qa":
        modules.append(("test_save_map_chapter", "tests/core/test_save_map_chapter.c"))
    elif args.target in ("native", "startup-qa", "startup-cpu-qa"):
        for source in ("src/platform/native_input.c", "src/platform/video_player.c", "src/platform/audio_events.c",
                       "src/core/se_voice.c", "src/core/sound_control.c", "src/core/mc_exit.c",
                       "src/core/game_camera.c", "src/core/game_movie_pool.c", "src/platform/game_services.c"):
            if source not in {s for _, s in modules}:
                modules.append((Path(source).stem, source))
        if args.target == "startup-cpu-qa":
            cpp_sources = ["tools/probe-startup-cpu.cpp"]
        else:
            cpp_sources += ["src/platform/model_renderer.cpp", "src/platform/window_host.cpp", "src/main.cpp"]
    elif args.target == "title-ui":
        cpp_sources += ["tools/probe-title-ui.cpp"]
    else:
        cpp_sources += ["tools/probe-mc-frame.cpp"]
    inputs = {"tools/build-native.py", "tools/build-windows.py", "tools/compiler_response.py",
              "tools/compiled_objects.py", *cpp_sources, *(s for _, s in modules)}
    if args.target in ("native","startup-qa"):
        inputs.add("tests/test_cpp_abi.cpp")
    # Fingerprint actual project include dependencies. Unrelated headers being
    # written by another worker are not inputs to this executable.
    initial, includes, time_sensitive = project_inputs(PROJECT, inputs)
    cache = None if args.no_object_cache else ObjectCache.for_build(cache_root, compiler, env)
    cache_writes = []
    report_path = PROJECT / "analysis" / ("windows-" + args.target + "-build.json")
    report = {"created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "status": "running", "target": args.target, "steps": [], "is_playable_game": False}
    report_lock = threading.Lock()
    def save():
        report_path.parent.mkdir(exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    def run(name, command):
        if name == "link":
            response = write_response_file(build / "link.rsp", command[2:])
            with report_lock:
                report.setdefault("response_files", []).append(response)
            command = command[:2] + ["@" + response["path"]]
        try:
            result = subprocess.run([str(v) for v in command], cwd=PROJECT, env=env,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
        except OSError as error:
            result = subprocess.CompletedProcess(command, 1, "Compiler launch failed: " + str(error))
        log = logs / (name + ".log")
        log.write_text(result.stdout, encoding="utf-8")
        with report_lock:
            report["steps"].append({"name": name, "returncode": result.returncode, "log": str(log)})
        if result.returncode:
            with report_lock:
                report["status"] = "failed"
                save()
            print(result.stdout)
            raise SystemExit(result.returncode)
        print(name + ": PASS", flush=True)

    def compile_object(name, command, source, output):
        key = None
        if cache:
            key, command = cache.prepare(PROJECT, command, source, initial, includes, time_sensitive)
        if key and cache.restore(key, output):
            log = logs / (name + ".log")
            log.write_text(f"Restored verified object from cache {key}\n", encoding="utf-8")
            with report_lock:
                report["steps"].append({"name": name, "returncode": 0, "log": str(log), "object_cache": key})
            print(name + ": PASS (cache)", flush=True)
            return
        run(name, command)
        if key:
            with report_lock:
                cache_writes.append((key, output))

    flags = list(WINDOWS_COMPILER_FLAGS)
    if args.target in ("native","startup-qa"):
        source = "tests/test_cpp_abi.cpp"
        output = build / "windows-public-headers.o"
        compile_object("check-windows-public-headers",
                       [compiler, "c++", "-std=c++17", *flags, "-c", source, "-o", output], source, output)
    if args.target=="startup-qa":
        # QA-only diagnostics must not break the ordinary native executable.
        source = "src/main.cpp"
        output = build / "native-driver-check.o"
        compile_object("check-native-driver",
                       [compiler, "c++", "-std=c++17", *flags, "-c", source, "-o", output], source, output)
    def compile_module(module):
        name, source = module
        obj = build / (name + ".o")
        compile_object("compile-" + name, c_driver(compiler) + ["-c", source, "-o", obj], source, obj)
        return obj
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        objects = list(executor.map(compile_module, modules))
    exe = build / {"native": "godhand-windows.exe", "startup-qa": "probe-native-startup.exe",
                   "startup-cpu-qa": "probe-startup-cpu.exe", "mc-frame": "probe-mc-frame.exe",
                   "title-ui": "probe-title-ui.exe", "model-qa": "test-model-renderer.exe",
                   "world-qa": "test-world-composition.exe", "scene-message-qa": "test-scene-message-depth.exe",
                   "save-map-qa": "test-save-map-chapter.exe"}[args.target]
    if args.target == "startup-qa":
        flags.append("-DGH_NATIVE_STARTUP_QA=1")
    def compile_cpp(source):
        name = "cxx-" + Path(source).stem
        obj = build / (name + ".o")
        compile_object("compile-" + name, [compiler, "c++", "-std=c++17", *flags, "-c", source, "-o", obj], source, obj)
        return obj
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        cpp_objects = list(executor.map(compile_cpp, cpp_sources))
    graphics_libraries = [] if args.target == "startup-cpu-qa" else ["-ld3d11", "-ldxgi", "-ld3dcompiler", "-luser32"]
    run("link", [compiler, "c++", "-std=c++17", *flags, *objects, *cpp_objects,
                 "-L" + str(sdk), *graphics_libraries, "-lwinmm", "-o", exe])
    if source_hashes(PROJECT, initial) != initial:
        report["status"] = "failed-source-changed-during-build"
        save()
        raise SystemExit("Sources changed during build; rebuild the stabilized inputs")
    if cache:
        for key, output in cache_writes:
            cache.store(key, output)
        report["object_cache"] = {
            "path": str(cache_root), "hits": sum("object_cache" in step for step in report["steps"]),
            "compiled": len(cache_writes),
        }
        print(f"Object cache: {report['object_cache']['hits']} reused, {len(cache_writes)} compiled", flush=True)
    report.update(status="built", source_sha256=initial, compiler=str(compiler), sdk_lib=str(sdk),
                  compiler_cache=str(args.compiler_cache.resolve()),
                  executable=str(exe), executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                  limitation="Compilation only; execution and actual gameplay acceptance are separate")
    save()
    print(str(exe))


if __name__ == "__main__":
    main()
