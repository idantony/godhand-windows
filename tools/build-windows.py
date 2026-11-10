"""Build native Windows presentation tools; all outputs and caches stay outside the project."""
import argparse
import ast
import datetime
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess

PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_SCRATCH = Path(r"G:\middle_thing\godhand_windows")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", choices=("mc-frame", "title-ui", "native", "startup-qa"), default="mc-frame")
    parser.add_argument("--scratch", type=Path, default=DEFAULT_SCRATCH)
    parser.add_argument("--compiler", type=Path, default=PROJECT / ".tools/zig-x86_64-windows-0.17.1/zig.exe")
    parser.add_argument("--sdk-lib", type=Path, help="Windows SDK um/x64 import-library directory")
    args = parser.parse_args()
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
    compiler = args.compiler.resolve()
    build = scratch / "build" / ("windows-" + args.target)
    logs = scratch / "logs" / ("windows-" + args.target)
    for p in (build, logs, scratch / "cache/tmp", scratch / "cache/zig-global", scratch / "cache/zig-local"):
        p.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(TEMP=str(scratch / "cache/tmp"), TMP=str(scratch / "cache/tmp"),
               ZIG_GLOBAL_CACHE_DIR=str(scratch / "cache/zig-global"), ZIG_LOCAL_CACHE_DIR=str(scratch / "cache/zig-local"))
    # One source-of-truth core module list; never link stale objects from a
    # different ABI or silently trust a previous test executable.
    tree = ast.parse((PROJECT / "tools/build-native.py").read_text(encoding="utf-8"))
    modules = next(ast.literal_eval(n.value) for n in ast.walk(tree) if isinstance(n, ast.Assign)
                   and any(isinstance(t, ast.Name) and t.id == "modules" for t in n.targets))
    cpp_sources = ["src/platform/ui_text_renderer.cpp", "src/platform/ui_sprite_renderer.cpp",
                   "src/platform/ui_polygon_renderer.cpp", "src/platform/ui_fade_renderer.cpp", "src/platform/mc_renderer.cpp"]
    if args.target in ("native", "startup-qa"):
        for source in ("src/platform/native_input.c", "src/platform/video_player.c", "src/platform/audio_events.c",
                       "src/core/se_voice.c", "src/core/sound_control.c", "src/core/mc_exit.c",
                       "src/core/game_camera.c", "src/core/game_movie_pool.c", "src/platform/game_services.c"):
            if source not in {s for _, s in modules}:
                modules.append((Path(source).stem, source))
        cpp_sources += ["src/platform/window_host.cpp", "src/main.cpp"]
    elif args.target == "title-ui":
        cpp_sources += ["tools/probe-title-ui.cpp"]
    else:
        cpp_sources += ["tools/probe-mc-frame.cpp"]
    inputs = {"tools/build-native.py", "tools/build-windows.py", *cpp_sources, *(s for _, s in modules)}
    # Fingerprint actual project include dependencies. Unrelated headers being
    # written by another worker are not inputs to this executable.
    pending = list(inputs)
    while pending:
        name = pending.pop()
        source = PROJECT / name
        if source.suffix not in (".c", ".cpp", ".h"):
            continue
        for include in re.findall(r'^\s*#\s*include\s*"([^"\r\n]+)"', source.read_text(encoding="utf-8"), re.MULTILINE):
            header = (source.parent / include).resolve()
            if not header.is_file():
                raise SystemExit("Unresolved project include: " + name + " -> " + include)
            try:
                dependency = header.relative_to(PROJECT).as_posix()
            except ValueError:
                raise SystemExit("Project include escaped source tree: " + name + " -> " + include)
            if dependency not in inputs:
                inputs.add(dependency)
                pending.append(dependency)
    def hashes():
        return {p: hashlib.sha256((PROJECT / p).read_bytes()).hexdigest() for p in sorted(inputs)}
    initial = hashes()
    report_path = PROJECT / "analysis" / ("windows-" + args.target + "-build.json")
    report = {"created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "status": "running", "target": args.target, "steps": [], "is_playable_game": False}
    def save():
        report_path.parent.mkdir(exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    def run(name, command):
        result = subprocess.run([str(v) for v in command], cwd=PROJECT, env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
        log = logs / (name + ".log")
        log.write_text(result.stdout, encoding="utf-8")
        report["steps"].append({"name": name, "returncode": result.returncode, "log": str(log)})
        if result.returncode:
            report["status"] = "failed"
            save()
            print(result.stdout)
            raise SystemExit(result.returncode)
        print(name + ": PASS", flush=True)
    flags = ["-target", "x86_64-windows-gnu", "-O2", "-Wall", "-Wextra", "-Werror", "-ffp-contract=off",
             "-fno-fast-math", "-D_CRT_SECURE_NO_WARNINGS"]
    objects = []
    for name, source in modules:
        obj = build / (name + ".o")
        run("compile-" + name, [compiler, "cc", "-std=c11", *flags, "-c", source, "-o", obj])
        objects.append(obj)
    exe = build / {"native": "godhand-windows.exe", "startup-qa": "probe-native-startup.exe", "mc-frame": "probe-mc-frame.exe", "title-ui": "probe-title-ui.exe"}[args.target]
    if args.target == "startup-qa":
        flags.append("-DGH_NATIVE_STARTUP_QA=1")
    run("link", [compiler, "c++", "-std=c++17", *flags, *objects, *cpp_sources,
                 "-L" + str(sdk), "-ld3d11", "-ldxgi", "-ld3dcompiler", "-luser32", "-lwinmm", "-o", exe])
    if hashes() != initial:
        report["status"] = "failed-source-changed-during-build"
        save()
        raise SystemExit("Sources changed during build; rebuild the stabilized inputs")
    report.update(status="built", source_sha256=initial, compiler=str(compiler), sdk_lib=str(sdk),
                  executable=str(exe), executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                  limitation="Compilation only; execution and actual gameplay acceptance are separate")
    save()
    print(str(exe))


if __name__ == "__main__":
    main()
