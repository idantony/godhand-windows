"""Publish an allowlisted progress summary, without local data or code dumps."""
import datetime
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
    if native["status"] != "passed" or differential["status"] != "passed":
        raise SystemExit("Refusing to publish a passing milestone from failed verification")
    progress = {
        "schema_version": 1,
        "verified_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "stage": "readable-source-and-native-module-validation",
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
            "room_r207": {"source_functions_recovered": 6, "scope": "Allocation/release, initialization and original empty move", "crt_helpers_pending": 3}},
        "tests": {"native_build": native["status"],
                  "passed_steps": [step["name"] for step in native["steps"] if step["name"].startswith("test-") and step["returncode"] == 0],
                  "rng_differential": {"distinct_seeds": differential["distinct_seeds"], "calls_compared": differential["function_calls_compared"],
                                       "comparison": differential["comparison"], "hardware_capture": False}},
        "remaining": ["Subystem/task initialization and main-loop integration", "Asset object/model/animation loading",
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
