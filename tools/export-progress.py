"""Publish an allowlisted progress summary, without local data or code dumps."""
import datetime
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(name):
    return json.loads((ROOT / "analysis" / name).read_text(encoding="utf-8-sig"))


def main():
    native = read("native-verification.json")
    differential = read("rng-differential.json")
    assets = read("asset-layout.json")
    baseline = read("disc-baseline.json")
    window = read("native-startup-window-verification.json")
    window_build = read(window["build_report"])
    if native["status"] != "passed" or differential["status"] != "passed":
        raise SystemExit("Refusing to publish a passing milestone from failed verification")
    if not native.get("source_sha256"):
        raise SystemExit("Rebuild native modules to record source fingerprints before exporting progress")
    for name, expected in native["source_sha256"].items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != expected:
            raise SystemExit("Native verification is stale for changed source: " + name)
    if window["status"] != "passed" or hashlib.sha256((ROOT / "analysis" / window["build_report"]).read_bytes()).hexdigest() != window["build_report_sha256"]:
        raise SystemExit("Native window execution proof does not match its build")
    for name, expected in window_build["source_sha256"].items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != expected:
            raise SystemExit("Window verification is stale for changed source: " + name)
    if hashlib.sha256(Path(window_build["executable"]).read_bytes()).hexdigest() != window["executable_sha256"]:
        raise SystemExit("Window execution binary changed after verification")
    current_dll = Path(differential["native_dll"])
    if hashlib.sha256(current_dll.read_bytes()).hexdigest() != differential["native_sha256"]:
        raise SystemExit("RNG differential report is stale for the rebuilt DLL")
    progress = {
        "schema_version": 1,
        "verified_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "stage": "native-windows-startup-mc-movie-main-integration",
        "playable": False,
        "original_game_scene_observed_in_native_port": False,
        "strategy": "Recover readable original logic and implement native platform dependencies",
        "input": {"serial": "SLUS-21503", "version": "1.00", "region": "NTSC-U",
                  "variant": "User-provided Uncensor Patch v1.0",
                  "iso_sha256": baseline["source_iso"]["sha256"], "boot_elf_sha256": baseline["boot"]["sha256"],
                  "retail_elf_identity": False},
        "assets": {"indexed": assets["afs"]["named_file_count"], "overlays": assets["afs"]["overlay_count"],
                   "overlays_matching_upstream_manifest": assets["upstream_overlay_comparison"]["matching"],
                   "native_read_only_access_tested": True},
        "native_modules": {
            "rng": {"original_functions": ["0x0031CF78", "0x0031CF30"], "numeric_policy": "EE chop adaptation, independent of host FEnv"},
            "boot": {"original_function": "0x0030B298", "platform_callbacks_required": 12, "real_platform_adapters_complete": False},
            "room_r207": {"source_functions_recovered": 9, "scope": "Allocation/release, initialization, original empty move and verified empty-table lifecycle", "crt_helpers_pending": 0},
            "memory": {"native_allocation_tested": True, "r207_uses_real_host_backend_in_test": True,
                       "original_cHeap_capacity_and_fragmentation_verified": False},
            "tasks": {"original_functions": ["0x002D5B30", "0x002D5440", "0x002D52A0"],
                      "scope": "Slot choice, attribute inheritance, filtered visitation and native serial fiber start/sleep/exit",
                      "native_serial_backend_tested": True, "full_EE_kernel_emulated": False},
            "dvd_jobs": {"original_functions": ["0x00200CB0", "0x00201098", "0x00201108"],
                         "scope": "Real allocation/sector reads, original priority order, pressure drain and task waits"},
            "startup_task": {"original_function": "0x0030D358", "real_native_backends_connected": True,
                             "scope": "Sound-bank commit, core/language DVD reads, font/menu/ID initialization",
                             "normal_path_next_entry": "0x002AE8A8", "next_entry_available": True,
                             "current_frontier": {"module": "main ELF", "address": window["execution"]["frontier"], "operation": "Next original scene initialization callee"},
                             "alternate_original_branch": {"GameMain_case0_and_parallel_asset_loader_tested": True,
                                                           "full_native_services_tested": True,
                                                           "next_unavailable_entry": "0x" + window["execution"]["frontier"]},
                             "earlier_boot_and_complete_main_loop_run": False},
            "font": {"initializer": "0x002A4970", "scope": "Bounded MST/MSA views, glyph metrics and restricted atlas/glyph RGBA"},
            "menu_id": {"original_functions": ["0x002AE990", "0x002ACC70"],
                        "scope": "Original MSD/IDD/TBL binding, reset and global identity"},
            "game_assets": {"original_function": "0x0030D5C8", "object_table_records": 41,
                            "scope": "Actual core/room-jump/object/player reads with original branch and task waits",
                            "integrated_from_complete_game_boot": False},
            "game_gates": {"original_functions": ["0x002A6CA8", "0x002A6DA8"],
                           "scope": "Disc-change countdown and modal input conditions; draw dependencies remain mandatory"},
            "memory_card": {"scope": "Original67-entry visible UI, absent/empty-profile Control, native input mapping, real SFX, normal exit and same-slot Movie tail restart",
                            "full_Main_UI_or_save_load_complete": False},
            "id_base": {"scope": "Bounded table lookup, typed640-entry/100-job pool, actual67-entry MC initialization/link/reset and packed-message environment",
                        "full_UI_rendering_complete": False, "original_MC_boot_prompts_rendered": True},
            "id_jobs": {"scope": "Actual MC zero-rotation layout/move/color animation, ordered visible consumers, T32 indexed8 decoding and restricted original texture quads",
                        "complete_MC_window_rendered": True},
            "game_init": {"original_function": "0x002A5B18", "scope": "Concrete world/camera/enemy/object/heat/input/save initialization and separate MT generator",
                          "scene_loading_or_combat_complete": False},
            "sound": {"startup": "0x002CA798", "scope": "Owned banks, AFD-to-AFS indexes and actual MC SFX through native ADPCM/XAudio2; explicit reference DSP",
                      "ready_flags": ["0x7F", "0x7F", "0x0F"], "real_device_voice_execution_tested": True, "QA_muted": True, "full_game_audio_complete": False},
            "asset_views": {"scope": "Bounded core bundles, restricted indexed4 TIM2 RGBA, SCR mesh descriptors and SES/ADX headers",
                            "model_vertices_animation_or_audio_decode_complete": False}},
        "native_windows_execution": {
            "scope": window["scope"], "hidden": True, "muted": True,
            "synthetic_input_fixture": True, "actual_user_input_observed": False,
            "ticks": window["execution"]["ticks"],
            "original_MC_frames_presented": window["execution"]["mc_frames_presented"],
            "Movie0_frames_presented": window["execution"]["movie_frames_presented"],
            "video_EOF_and_audio_done": window["execution"]["movie_complete"],
            "original_GameMain_state": window["execution"]["game_state"],
            "assets_loaded": window["execution"]["assets_loaded"],
            "frontier": window["execution"]["frontier"], "full_game_scene_or_gameplay_observed": False},
        "tests": {"native_build": native["status"],
                  "built_modules": native["module_names"],
                  "passed_steps": [step["name"] for step in native["steps"] if step["name"].startswith("test-") and step["returncode"] == 0],
                  "rng_differential": {"distinct_seeds": differential["distinct_seeds"], "calls_compared": differential["function_calls_compared"],
                                       "comparison": differential["comparison"], "hardware_capture": False}},
        "remaining": ["Original r020 title overlay binding and logic", "Earlier subsystem initialization and full main-loop integration", "Asset object/model/animation loading",
                      "Native rendering, audio, input and save adapters", "Full game progression and timing validation"],
        "progress_policy": "Upstream PS2 matching coverage is not native port coverage; no synthetic completion percentage",
        "sources": {"upstream": "https://github.com/LucasPicoli/god-hand-decomp", "upstream_commit": "eaae18d0dfab1f9a0df276ae5ca1581ad326faf3",
                    "method": "https://opengoal.dev/docs/developing/decompiling/getting_started/"}}
    target = ROOT / "docs" / "progress.json"
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(progress, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("Curated progress exported: " + str(target))


if __name__ == "__main__":
    main()
