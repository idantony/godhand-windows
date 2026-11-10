"""Build and run the native source-port modules; all intermediates stay on G:."""
import argparse
import concurrent.futures
import datetime
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import threading

PROJECT = Path(__file__).resolve().parents[1]
SCRATCH = Path(r"G:\middle_thing\godhand_windows")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-tests", action="store_true")
    parser.add_argument("--jobs", type=int, default=4, help="Independent compiler processes (1..16); device tests remain serial")
    parser.add_argument("--hardware-audio", action="store_true", help="Run muted real-device voice, ADX, BGM and scene-flow checks")
    parser.add_argument("--adx-reference-wave", type=Path, help="Independent vgmstream WAV for the full native ADX sample comparison")
    parser.add_argument("--movie-codec", type=Path, help="Also run the muted Movie0 native codec/device check")
    parser.add_argument("--compiler", type=Path, default=PROJECT / ".tools" / "zig-x86_64-windows-0.17.1" / "zig.exe", help="Zig executable; permits reusing an installed toolchain")
    parser.add_argument("--scratch", type=Path, default=SCRATCH, help="External build/cache root")
    args = parser.parse_args()
    if not 1 <= args.jobs <= 16:
        parser.error("--jobs must be in 1..16")
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

    report_lock = threading.Lock()

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
        with report_lock:
            manifest["steps"].append(step)
            if result.returncode:
                manifest["status"] = "failed"
            report.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            print(label + ": " + ("PASS" if result.returncode == 0 else "FAIL"), flush=True)
        if result.returncode:
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
               ("native_input", "src/platform/native_input.c"), ("frame_state", "src/core/frame_state.c"),
               ("screen_fade", "src/core/screen_fade.c"), ("game_scene_setup", "src/core/game_scene_setup.c"),
               ("sound_scene", "src/core/sound_scene.c"), ("game_collision_pairs", "src/core/game_collision_pairs.c"),
               ("game_room_module", "src/core/game_room_module.c"), ("game_effects", "src/core/game_effects.c"),
               ("game_scene_filters", "src/core/game_scene_filters.c"), ("bgm_requests", "src/core/bgm_requests.c"),
               ("adx", "src/assets/adx.c"), ("adx_stream", "src/platform/adx_stream.c"),
               ("sound_scene_tick", "src/core/sound_scene_tick.c"),
               ("game_world_light", "src/core/game_world_light.c"), ("game_model_packets", "src/core/game_model_packets.c"),
               ("game_collision_solids", "src/core/game_collision_solids.c"), ("game_scene_tables", "src/core/game_scene_tables.c"),
               ("game_title_player", "src/core/game_title_player.c"), ("game_checkpoint", "src/core/game_checkpoint.c"),
               ("game_scene_at", "src/core/game_scene_at.c"), ("game_action_buttons", "src/core/game_action_buttons.c"),
               ("game_scene_spawns", "src/core/game_scene_spawns.c"), ("game_scene_placements", "src/core/game_scene_placements.c"),
               ("game_scene_ui", "src/core/game_scene_ui.c"), ("game_event", "src/core/game_event.c"),
               ("display_state", "src/core/display_state.c"),
               ("game_camera_step", "src/core/game_camera_step.c"),
               ("game_camera_projection", "src/core/game_camera_projection.c"),
               ("game_scene_finish", "src/core/game_scene_finish.c"),
               ("game_scene_step", "src/core/game_scene_step.c"),
               ("game_scene_aux", "src/core/game_scene_aux.c"), ("ui_order", "src/core/ui_order.c"),
               ("game_object_step", "src/core/game_object_step.c"),
               ("game_render_projection", "src/core/game_render_projection.c"),
               ("game_render_steps", "src/core/game_render_steps.c"),
               ("game_ray_query", "src/core/game_ray_query.c"),
               ("game_scene_at_step", "src/core/game_scene_at_step.c"),
               ("game_scene_at_age", "src/core/game_scene_at_age.c"),
               ("game_hitarea_frame", "src/core/game_hitarea_frame.c"),
               ("game_player_frame", "src/core/game_player_frame.c"),
               ("game_scene_at_damage", "src/core/game_scene_at_damage.c"),
               ("sdk_rotation_trig", "src/core/sdk_rotation_trig.c"),
               ("game_player_springs_frame", "src/core/game_player_springs_frame.c"),
               ("game_scene_post", "src/core/game_scene_post.c"), ("cockpit_fan", "src/core/cockpit_fan.c"),
               ("id_number", "src/core/id_number.c"),
               ("game_door", "src/core/game_door.c"),
               ("game_message_step", "src/core/game_message_step.c"),
               ("sound_spatial_step", "src/core/sound_spatial_step.c"),
               ("game_system_menu", "src/core/game_system_menu.c"),
               ("game_scene_exit", "src/core/game_scene_exit.c"),
               ("sound_scene_exit", "src/core/sound_scene_exit.c"),
               ("game_scene_meshes", "src/core/game_scene_meshes.c"),
               ("game_scene_meshes_finish", "src/core/game_scene_meshes_finish.c"),
               ("static_mesh", "src/assets/static_mesh.c"),
               ("static_mesh_transform", "src/assets/static_mesh_transform.c"),
               ("static_model_constants", "src/assets/static_model_constants.c"),
               ("static_mesh_program10", "src/assets/static_mesh_program10.c"),
               ("static_mesh_clip10", "src/assets/static_mesh_clip10.c"),
               ("static_mesh_program10_packets", "src/assets/static_mesh_program10_packets.c"),
               ("game_object_static", "src/core/game_object_static.c"),
               ("scene_at_units", "src/core/scene_at_units.c"),
               ("game_collision_scroll", "src/core/game_collision_scroll.c"),
               ("game_scenario_exec", "src/core/game_scenario_exec.c"),
               ("game_scenario_soft_event", "src/core/game_scenario_soft_event.c"),
               ("r100_init", "src/rooms/r100_init.c"),
               ("game_object_sound", "src/core/game_object_sound.c"),
               ("game_scene_exit_leaves", "src/core/game_scene_exit_leaves.c"),
               ("game_world_light_select", "src/core/game_world_light_select.c"),
               ("game_world_light_step", "src/core/game_world_light_step.c"),
               ("tm3", "src/assets/tm3.c"),
               ("game_scene_meshes_material", "src/core/game_scene_meshes_material.c"),
               ("game_room_arena", "src/core/game_room_arena.c"),
               ("static_world_queue", "src/platform/static_world_queue.c"),
               ("game_om_factory", "src/core/game_om_factory.c"),
               ("game_player_factory", "src/core/game_player_factory.c"),
               ("game_actor_scroll_links", "src/core/game_actor_scroll_links.c"),
               ("game_ray_static", "src/core/game_ray_static.c"),
               ("game_player_init", "src/core/game_player_init.c"),
               ("game_actor_model", "src/core/game_actor_model.c"),
               ("skinned_mesh", "src/assets/skinned_mesh.c"),
               ("game_actor_packets", "src/core/game_actor_packets.c"),
               ("game_actor_model_nodes", "src/core/game_actor_model_nodes.c"),
               ("game_actor_hierarchy", "src/core/game_actor_hierarchy.c"),
               ("game_actor_matrix", "src/core/game_actor_matrix.c"),
               ("motion", "src/assets/motion.c"),
               ("game_actor_motion", "src/core/game_actor_motion.c"),
               ("game_actor_motion_call", "src/core/game_actor_motion_call.c"),
               ("game_event_player_reset", "src/core/game_event_player_reset.c"),
               ("game_actor_motion_ik", "src/core/game_actor_motion_ik.c"),
               ("game_actor_motion_step", "src/core/game_actor_motion_step.c"),
               ("game_actor_inverse", "src/core/game_actor_inverse.c"),
               ("game_actor_blend", "src/core/game_actor_blend.c"),
               ("sdk_sine", "src/core/sdk_sine.c"),
               ("game_actor_sequence", "src/core/game_actor_sequence.c"),
               ("game_actor_motion_solve", "src/core/game_actor_motion_solve.c"),
               ("game_actor_two_bone", "src/core/game_actor_two_bone.c"),
               ("game_actor_finish_model", "src/core/game_actor_finish_model.c"),
               ("game_actor_palette", "src/core/game_actor_palette.c"),
               ("game_actor_external_parent", "src/core/game_actor_external_parent.c"),
               ("game_effect_release", "src/core/game_effect_release.c"),
               ("skinned_mesh_blend", "src/assets/skinned_mesh_blend.c"),
               ("game_player_node_cache", "src/core/game_player_node_cache.c"),
               ("game_player_post_init", "src/core/game_player_post_init.c"),
               ("game_player_spawn", "src/core/game_player_spawn.c"),
               ("game_event_environment", "src/core/game_event_environment.c"),
               ("game_event_start", "src/core/game_event_start.c"),
               ("game_event_clear", "src/core/game_event_clear.c"),
               ("game_event_module", "src/core/game_event_module.c"),
               ("game_event_resource", "src/core/game_event_resource.c"),
               ("r100_scripts", "src/rooms/r100_scripts.c"),
               ("r100_roof_task", "src/rooms/r100_roof_task.c"),
               ("r100_counter_task", "src/rooms/r100_counter_task.c"),
               ("r100_init_tail", "src/rooms/r100_init_tail.c"),
               ("game_cockpit_tutorial", "src/core/game_cockpit_tutorial.c"),
               ("game_player_abort", "src/core/game_player_abort.c"),
               ("game_scene_dynamic", "src/core/game_scene_dynamic.c"),
               ("game_dynamic_corners", "src/core/game_dynamic_corners.c"),
               ("game_scene_dynamic_geometry", "src/core/game_scene_dynamic_geometry.c"),
               ("game_ray_dynamic", "src/core/game_ray_dynamic.c"),
               ("enemy_se_banks", "src/core/enemy_se_banks.c"),
               ("game_object_sounds", "src/core/game_object_sounds.c"),
               ("game_enemy_resources", "src/core/game_enemy_resources.c"),
               ("game_om_door_geometry", "src/core/game_om_door_geometry.c"),
               ("game_om_initialize", "src/core/game_om_initialize.c"),
               ("game_om303", "src/core/game_om303.c"),
               ("game_om_base", "src/core/game_om_base.c"),
               ("game_om304", "src/core/game_om304.c"),
               ("game_om305", "src/core/game_om305.c"),
               ("game_om307", "src/core/game_om307.c"),
               ("game_om30a", "src/core/game_om30a.c"),
               ("game_om30e", "src/core/game_om30e.c"),
               ("game_om310", "src/core/game_om310.c"),
               ("game_om320", "src/core/game_om320.c"),
               ("game_om322", "src/core/game_om322.c"),
               ("game_om323", "src/core/game_om323.c"),
               ("game_om324", "src/core/game_om324.c"),
               ("game_om33f", "src/core/game_om33f.c"),
               ("game_om346", "src/core/game_om346.c"),
               ("game_om354", "src/core/game_om354.c"),
               ("game_om35d", "src/core/game_om35d.c"),
               ("game_om309", "src/core/game_om309.c"),
               ("game_om3eb", "src/core/game_om3eb.c"),
               ("game_enemy_placements", "src/core/game_enemy_placements.c"),
               ("game_om_fragments", "src/core/game_om_fragments.c"),
               ("game_camera_follow", "src/core/game_camera_follow.c"),
               ("game_camera_envelope", "src/core/game_camera_envelope.c"),
               ("game_camera_shake", "src/core/game_camera_shake.c"),
               ("game_player_child", "src/core/game_player_child.c"),
               ("game_player_child_attach", "src/core/game_player_child_attach.c"),
               ("game_player_child_tail", "src/core/game_player_child_tail.c"),
               ("game_math_globals", "src/core/game_math_globals.c"),
               ("game_player_springs", "src/core/game_player_springs.c"),
               ("game_player_shadow", "src/core/game_player_shadow.c"),
               ("game_collision_solid_units", "src/core/game_collision_solid_units.c"),
               ("game_player_init_tail", "src/core/game_player_init_tail.c"),
               ("game_hitarea_objects", "src/core/game_hitarea_objects.c"),
               ("game_hitarea_shapes", "src/core/game_hitarea_shapes.c"),
               ("game_collision_pair_shapes", "src/core/game_collision_pair_shapes.c"),
               ("game_collision_pair_queries", "src/core/game_collision_pair_queries.c"),
               ("game_scene_at_geometry", "src/core/game_scene_at_geometry.c"),
               ("id_entry_reset", "src/core/id_entry_reset.c"), ("id_layout_rotation", "src/core/id_layout_rotation.c"),
               ("id_animation", "src/core/id_animation.c"), ("id_texture_polygon", "src/core/id_texture_polygon.c"),
               ("r020_title", "src/rooms/r020_title.c"), ("r100", "src/rooms/r100.c")]
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
        ("game-room-arena", "tests/core/test_game_room_arena.c", "iso"),
        ("game-player-factory", "tests/core/test_game_player_factory.c", "iso"),
        ("game-player-postfactory", "tests/core/test_game_player_postfactory.c", "iso"),
        ("game-player-init", "tests/core/test_game_player_init.c", "iso"),
        ("game-actor-model", "tests/core/test_game_actor_model.c", "iso"),
        ("game-actor-packets", "tests/core/test_game_actor_packets.c", "iso"),
        ("game-actor-model-nodes", "tests/core/test_game_actor_model_nodes.c", "iso"),
        ("game-actor-matrix", "tests/core/test_game_actor_matrix.c", "none"),
        ("motion", "tests/assets/test_motion.c", "iso"),
        ("game-actor-motion", "tests/core/test_game_actor_motion.c", "iso"),
        ("game-actor-motion-call", "tests/core/test_game_actor_motion_call.c", "iso"),
        ("game-event-player-reset", "tests/core/test_game_event_player_reset.c", "iso"),
        ("game-actor-motion-ik", "tests/core/test_game_actor_motion_ik.c", "iso"),
        ("game-actor-motion-step", "tests/core/test_game_actor_motion_step.c", "iso"),
        ("game-actor-blend", "tests/core/test_game_actor_blend.c", "elf"),
        ("game-actor-motion-blend", "tests/core/test_game_actor_motion_blend.c", "iso"),
        ("game-actor-motion-solve", "tests/core/test_game_actor_motion_solve.c", "iso"),
        ("game-actor-two-bone", "tests/core/test_game_actor_two_bone.c", "iso"),
        ("game-actor-finish-model", "tests/core/test_game_actor_finish_model.c", "iso"),
        ("game-actor-palette", "tests/core/test_game_actor_palette.c", "iso"),
        ("game-effect-release", "tests/core/test_game_effect_release.c", "iso"),
        ("skinned-mesh-blend", "tests/assets/test_skinned_mesh_blend.c", "iso"),
        ("game-player-node-cache", "tests/core/test_game_player_node_cache.c", "both"),
        ("game-player-motion-request", "tests/core/test_game_player_motion_request.c", "both"),
        ("game-player-post-init", "tests/core/test_game_player_post_init.c", "none"),
        ("game-player-spawn", "tests/core/test_game_player_spawn.c", "iso"),
        ("game-event-environment", "tests/core/test_game_event_environment.c", "elf"),
        ("game-event-start", "tests/core/test_game_event_start.c", "iso"),
        ("game-event-clear", "tests/core/test_game_event_clear.c", "iso"),
        ("game-event-module", "tests/core/test_game_event_module.c", "iso"),
        ("game-event-resource", "tests/core/test_game_event_resource.c", "iso"),
        ("r100-scripts", "tests/core/test_r100_scripts.c", "iso"),
        ("r100-roof-task", "tests/core/test_r100_roof_task.c", "iso"),
        ("r100-counter-task", "tests/core/test_r100_counter_task.c", "iso"),
        ("r100-init-tail", "tests/core/test_r100_init_tail.c", "iso"),
        ("game-cockpit-tutorial", "tests/core/test_game_cockpit_tutorial.c", "iso"),
        ("game-player-abort", "tests/core/test_game_player_abort.c", "iso"),
        ("game-scene-dynamic", "tests/core/test_game_scene_dynamic.c", "both"),
        ("game-scene-dynamic-geometry", "tests/core/test_game_scene_dynamic_geometry.c", "both"),
        ("game-ray-dynamic", "tests/core/test_game_ray_dynamic.c", "elf"),
        ("game-ray-mixed", "tests/core/test_game_ray_mixed.c", "both"),
        ("enemy-se-banks", "tests/core/test_enemy_se_banks.c", "iso"),
        ("game-object-sounds", "tests/core/test_game_object_sounds.c", "iso"),
        ("game-enemy-resources", "tests/core/test_game_enemy_resources.c", "iso"),
        ("game-om-door-geometry", "tests/core/test_game_om_door_geometry.c", "elf"),
        ("game-om-initialize", "tests/core/test_game_om_initialize.c", "both"),
        ("game-om303", "tests/core/test_game_om303.c", "both"),
        ("game-om304", "tests/core/test_game_om304.c", "both"),
        ("game-om305", "tests/core/test_game_om305.c", "both"),
        ("game-om307", "tests/core/test_game_om307.c", "both"),
        ("game-om30a", "tests/core/test_game_om30a.c", "both"),
        ("game-om30e", "tests/core/test_game_om30e.c", "both"),
        ("game-om310", "tests/core/test_game_om310.c", "both"),
        ("game-om320", "tests/core/test_game_om320.c", "both"),
        ("game-om322", "tests/core/test_game_om322.c", "both"),
        ("game-om323", "tests/core/test_game_om323.c", "both"),
        ("game-om324", "tests/core/test_game_om324.c", "both"),
        ("game-om33f", "tests/core/test_game_om33f.c", "both"),
        ("game-om346", "tests/core/test_game_om346.c", "both"),
        ("game-om354", "tests/core/test_game_om354.c", "both"),
        ("game-om35d", "tests/core/test_game_om35d.c", "both"),
        ("game-om309", "tests/core/test_game_om309.c", "both"),
        ("game-om3eb", "tests/core/test_game_om3eb.c", "iso"),
        ("game-enemy-placements", "tests/core/test_game_enemy_placements.c", "iso"),
        ("game-collision-pair-step", "tests/core/test_game_collision_pair_step.c", "elf"),
        ("game-collision-pair-queries", "tests/core/test_game_collision_pair_queries.c", "none"),
        ("game-collision-sphere-box", "tests/core/test_game_collision_sphere_box.c", "elf"),
        ("game-collision-cylinder-cylinder", "tests/core/test_game_collision_cylinder_cylinder.c", "none"),
        ("game-collision-cylinder-box", "tests/core/test_game_collision_cylinder_box.c", "none"),
        ("game-collision-pair-dispatch", "tests/core/test_game_collision_pair_dispatch.c", "elf"),
        ("game-collision-kind2-pair", "tests/core/test_game_collision_kind2_pair.c", "none"),
        ("game-collision-kind2-box", "tests/core/test_game_collision_kind2_box.c", "none"),
        ("game-collision-box-box", "tests/core/test_game_collision_box_box.c", "none"),
        ("game-camera-follow", "tests/core/test_game_camera_follow.c", "iso"),
        ("game-camera-envelope", "tests/core/test_game_camera_envelope.c", "iso"),
        ("game-camera-shake", "tests/core/test_game_camera_shake.c", "none"),
        ("game-player-child", "tests/core/test_game_player_child.c", "iso"),
        ("game-player-child-attach", "tests/core/test_game_player_child_attach.c", "iso"),
        ("game-player-child-tail", "tests/core/test_game_player_child_tail.c", "both"),
        ("game-math-globals", "tests/core/test_game_math_globals.c", "elf"),
        ("game-player-springs", "tests/core/test_game_player_springs.c", "both"),
        ("game-player-shadow", "tests/core/test_game_player_shadow.c", "iso"),
        ("game-collision-solid-units", "tests/core/test_game_collision_solid_units.c", "none"),
        ("game-player-init-tail", "tests/core/test_game_player_init_tail.c", "iso"),
        ("game-hitarea-objects", "tests/core/test_game_hitarea_objects.c", "none"),
        ("game-hitarea-shapes", "tests/core/test_game_hitarea_shapes.c", "elf"),
        ("game-player-hitarea-tail", "tests/core/test_game_player_hitarea_tail.c", "both"),
        ("game-scene-at-native", "tests/core/test_game_scene_at_native.c", "iso"),
        ("game-om-factory", "tests/core/test_game_om_factory.c", "iso"),
        ("game-object-acquire", "tests/core/test_game_object_acquire.c", "none"),
        ("game-object-resources", "tests/core/test_game_object_resources.c", "iso"),
        ("game-actor-scroll-links", "tests/core/test_game_actor_scroll_links.c", "iso"),
        ("game-ray-static", "tests/core/test_game_ray_static.c", "iso"),
        ("movie-task", "tests/core/test_movie_task.c", "none"),
        ("game-camera", "tests/core/test_game_camera.c", "elf"),
        ("game-movie-pool", "tests/core/test_game_movie_pool.c", "none"),
        ("game-services", "tests/platform/test_game_services.c", "iso"),
        ("mc-exit", "tests/core/test_mc_exit.c", "iso"),
        ("memory-card-end", "tests/platform/test_memory_card_end.c", "none"),
        ("mc-control-flow", "tests/core/test_mc_control_flow.c", "iso"),
        ("audio-events", "tests/platform/test_audio_events.c", "iso"),
        ("se-voice", "tests/core/test_se_voice.c", "iso"),
        ("se-stop", "tests/core/test_se_stop.c", "iso"),
        ("se-bank2", "tests/core/test_se_bank2.c", "iso"),
        ("sound-control", "tests/core/test_sound_control.c", "iso"),
        ("video-player", "tests/platform/test_video_player.c", "movie"),
        ("native-input", "tests/platform/test_native_input.c", "iso"),
        ("frame-state", "tests/core/test_frame_state.c", "none"),
        ("screen-fade", "tests/core/test_screen_fade.c", "none"),
        ("game-scene-setup", "tests/core/test_game_scene_setup.c", "none"),
        ("sound-scene", "tests/core/test_sound_scene.c", "iso"),
        ("game-collision-pairs", "tests/core/test_game_collision_pairs.c", "none"),
        ("game-room-module", "tests/core/test_game_room_module.c", "iso"),
        ("game-effects", "tests/core/test_game_effects.c", "iso"),
        ("game-scene-filters", "tests/core/test_game_scene_filters.c", "iso"),
        ("bgm-requests", "tests/core/test_bgm_requests.c", "iso"),
        ("bgm-automatic", "tests/core/test_bgm_automatic.c", "iso"),
        ("se-voice-stop-bank", "tests/core/test_se_voice_stop_bank.c", "iso"),
        ("title-texture", "tests/core/test_title_texture.c", "iso"),
        ("adx", "tests/assets/test_adx.c", "adx-reference"),
        ("adx-stream", "tests/platform/test_adx_stream.c", "iso"),
        ("sound-scene-tick", "tests/core/test_sound_scene_tick.c", "iso"),
        ("game-world-light", "tests/core/test_game_world_light.c", "iso"),
        ("game-world-light-r100", "tests/core/test_game_world_light_r100.c", "iso"),
        ("game-world-light-step", "tests/core/test_game_world_light_step.c", "iso"),
        ("game-model-packets", "tests/core/test_game_model_packets.c", "none"),
        ("game-collision-solids", "tests/core/test_game_collision_solids.c", "none"),
        ("game-scene-tables", "tests/core/test_game_scene_tables.c", "iso"),
        ("game-environment", "tests/core/test_game_environment.c", "iso"),
        ("title-level-texture", "tests/core/test_title_level_texture.c", "iso"),
        ("game-title-player", "tests/core/test_game_title_player.c", "none"),
        ("game-checkpoint", "tests/core/test_game_checkpoint.c", "none"),
        ("game-scene-at", "tests/core/test_game_scene_at.c", "iso"),
        ("game-action-buttons", "tests/core/test_game_action_buttons.c", "none"),
        ("game-scene-spawns", "tests/core/test_game_scene_spawns.c", "none"),
        ("game-scene-placements", "tests/core/test_game_scene_placements.c", "iso"),
        ("game-scene-ui", "tests/core/test_game_scene_ui.c", "iso"),
        ("game-event", "tests/core/test_game_event.c", "elf"),
        ("display-state", "tests/core/test_display_state.c", "elf"),
        ("game-camera-step", "tests/core/test_game_camera_step.c", "iso"),
        ("game-camera-projection", "tests/core/test_game_camera_projection.c", "both"),
        ("game-scene-finish", "tests/core/test_game_scene_finish.c", "iso"),
        ("game-scene-step", "tests/core/test_game_scene_step.c", "iso"),
        ("game-nearcheck", "tests/core/test_game_nearcheck.c", "iso"),
        ("game-object-move-live", "tests/core/test_game_object_move_live.c", "both"),
        ("game-scene-aux", "tests/core/test_game_scene_aux.c", "none"),
        ("ui-order", "tests/core/test_ui_order.c", "none"),
        ("game-object-step", "tests/core/test_game_object_step.c", "iso"),
        ("game-render-projection", "tests/core/test_game_render_projection.c", "iso"),
        ("game-render-steps", "tests/core/test_game_render_steps.c", "iso"),
        ("game-children-step", "tests/core/test_game_children_step.c", "none"),
        ("game-children-draw", "tests/core/test_game_children_draw.c", "none"),
        ("game-ray-query", "tests/core/test_game_ray_query.c", "iso"),
        ("game-scene-at-step", "tests/core/test_game_scene_at_step.c", "iso"),
        ("game-scene-at-age", "tests/core/test_game_scene_at_age.c", "iso"),
        ("game-hitarea-frame", "tests/core/test_game_hitarea_frame.c", "both"),
        ("game-player-frame", "tests/core/test_game_player_frame.c", "elf"),
        ("game-player-frame-physics", "tests/core/test_game_player_frame_physics.c", "both"),
        ("game-scene-at-damage", "tests/core/test_game_scene_at_damage.c", "iso"),
        ("sdk-rotation-trig", "tests/core/test_sdk_rotation_trig.c", "none"),
        ("game-scene-post", "tests/core/test_game_scene_post.c", "iso"),
        ("game-cockpit-frame", "tests/core/test_game_cockpit_frame.c", "iso"),
        ("game-door", "tests/core/test_game_door.c", "iso"),
        ("game-message-step", "tests/core/test_game_message_step.c", "iso"),
        ("game-objects-update", "tests/core/test_game_objects_update.c", "none"),
        ("sound-spatial-step", "tests/core/test_sound_spatial_step.c", "iso"),
        ("game-system-menu", "tests/core/test_game_system_menu.c", "iso"),
        ("game-scene-exit", "tests/core/test_game_scene_exit.c", "iso"),
        ("scene-exit-cleanup", "tests/core/test_scene_exit_cleanup.c", "iso"),
        ("game-scene-meshes", "tests/core/test_game_scene_meshes.c", "iso"),
        ("static-mesh", "tests/assets/test_static_mesh.c", "iso"),
        ("tm3", "tests/assets/test_tm3.c", "iso"),
        ("static-mesh-transform", "tests/assets/test_static_mesh_transform.c", "iso"),
        ("static-model-constants", "tests/assets/test_static_model_constants.c", "none"),
        ("static-mesh-program10", "tests/assets/test_static_mesh_program10.c", "iso"),
        ("static-mesh-clip10", "tests/assets/test_static_mesh_clip10.c", "none"),
        ("static-mesh-program10-packets", "tests/assets/test_static_mesh_program10_packets.c", "iso"),
        ("game-object-static", "tests/core/test_game_object_static.c", "iso"),
        ("scene-at-units", "tests/core/test_scene_at_units.c", "iso"),
        ("game-collision-scroll", "tests/core/test_game_collision_scroll.c", "iso"),
        ("game-scenario-exec", "tests/core/test_game_scenario_exec.c", "none"),
        ("game-scenario-soft-event", "tests/core/test_game_scenario_soft_event.c", "none"),
        ("r100-init", "tests/rooms/test_r100_init.c", "iso"),
        ("static-world-queue", "tests/platform/test_static_world_queue.c", "iso"),
        ("game-object-sound", "tests/core/test_game_object_sound.c", "iso"),
        ("sound-scene-exit", "tests/core/test_sound_scene_exit.c", "iso"),
        ("id-entry-reset", "tests/core/test_id_entry_reset.c", "iso"),
        ("id-layout-rotation", "tests/core/test_id_layout_rotation.c", "iso"),
        ("id-animation", "tests/core/test_id_animation.c", "iso"),
        ("id-number", "tests/core/test_id_number.c", "iso"),
        ("id-alignment", "tests/core/test_id_alignment.c", "iso"),
        ("r020-title", "tests/rooms/test_r020_title.c", "iso"),
        ("r100", "tests/rooms/test_r100.c", "iso"),
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
    shared_headers = {"game_room_arena": "src/core/game_room_pools.h","game_object_static": "src/core/game_object_step.h","id_jobs_texture": "src/core/id_jobs.h", "id_jobs_primitive": "src/core/id_jobs.h",
                      "game_enemy_placements": "src/core/game_scene_placements.h",
                      "game_scene_meshes_finish": "src/core/game_scene_meshes.h",
                      "game_actor_hierarchy": "src/core/game_actor_model.h",
                      "game_world_light_select": "src/core/game_world_light.h",
                      "game_scene_meshes_material": "src/core/game_scene_meshes.h"}
    source_inputs.update(source for _, source, _ in extra_tests)
    # Capture transitive/shared ABI inputs too: a module's public header can
    # borrow a second owner's layout without sharing its source filename.
    source_inputs.update(path.relative_to(PROJECT).as_posix() for path in (PROJECT / "src").rglob("*.h"))
    source_inputs.update({"tests/assets/oracle_static_mesh_transform.py",
                          "tests/assets/oracle_actor_bind_pose.py",
                          "tests/assets/oracle_player_springs.py",
                          "tests/assets/oracle_static_mesh_program10.py",
                          "tests/assets/oracle_static_mesh_clip10.py"})
    for name, source in modules:
        source_inputs.add(source)
        source_inputs.add(shared_headers.get(name, str(Path(source).with_suffix(".h")).replace("\\", "/")))
    initial_sources = {name: hashlib.sha256((PROJECT / name).read_bytes()).hexdigest() for name in sorted(source_inputs)}
    def compile_module(module):
        name, source = module
        target = output / (name + ".o")
        run("compile-" + name, common + ["-c", source, "-o", target])
        return name, target
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as workers:
        objects = dict(workers.map(compile_module, modules))
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
    # Object-pool shutdown now owns real shape/list destruction too; use the
    # coherent core closure instead of maintaining a second partial link list.
    run("compile-game-init-tests", common + ["-Isrc/core"] + host_objects + ["tests/core/test_game_init.c", "-o", output / "test_game_init.exe"])
    run("compile-mc-task-tests", common + ["-Isrc/core"] + host_objects + ["tests/core/test_mc_task.c", "-o", output / "test_mc_task.exe"])
    id_objects = [objects[name] for name in ("id_jobs", "id_jobs_texture", "id_jobs_primitive", "id_base", "id_path", "id_animation", "id_layout_rotation", "menu_state", "font_state", "font_texture", "core_bundle", "disc", "tim2")]
    run("compile-id-jobs-tests", common + id_objects + ["tests/core/test_id_jobs.c", "-o", output / "test_id_jobs.exe"])
    run("compile-id-texture-tests", common + id_objects + ["tests/core/test_id_texture.c", "-o", output / "test_id_texture.exe"])
    run("compile-mc-control-tests", common + ["-Isrc/core"] + host_objects + ["tests/core/test_mc_control.c", "-o", output / "test_mc_control.exe"])
    run("compile-game-stage-tests", common + ["-Isrc/core"] + host_objects + ["tests/core/test_game_stage.c", "-o", output / "test_game_stage.exe"])
    run("compile-game-children-tests", common + ["-Isrc/core"] + [objects[name] for name in ("game_children", "game_render_steps", "id_base", "id_path", "core_bundle")] + ["tests/core/test_game_children.c", "-o", output / "test_game_children.exe"])
    run("compile-id-text-tests", common + [objects["id_text"], objects["font_draw"], objects["packed_message"]] + id_objects + ["tests/core/test_id_text.c", "-o", output / "test_id_text.exe"])
    def compile_extra_test(test):
        name, source, _ = test
        run("compile-" + name + "-tests", common + ["-Isrc/core", "-Isrc/platform", "-Isrc/rooms", "-Isrc/assets"] + list(objects.values()) + [source, "-o", output / ("test_" + name.replace("-", "_") + ".exe")])
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as workers:
        list(workers.map(compile_extra_test, extra_tests))
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
            if ((name in ("se-voice", "se-stop", "se-bank2", "se-voice-stop-bank", "mc-control-flow", "bgm-requests", "bgm-automatic", "adx-stream", "r020-title", "game-services", "game-scene-exit", "scene-exit-cleanup", "game-object-sound", "sound-scene-exit") and not args.hardware_audio)
                    or (inputs == "movie" and not args.movie_codec)
                    or (inputs == "adx-reference" and not args.adx_reference_wave)):
                manifest.setdefault("skipped_checks", []).append({"name": "test-" + name, "reason": "Explicit hardware/codec opt-in not supplied"})
                continue
            arguments = []
            if inputs in ("iso", "both", "movie", "adx-reference"):
                arguments.append(baseline["source_iso"]["path"])
            if inputs in ("elf", "both"):
                arguments.append(PROJECT / "reference" / "SLUS_215.03")
            if inputs == "movie":
                arguments.append(args.movie_codec.resolve())
            if inputs == "adx-reference":
                arguments.append(args.adx_reference_wave.resolve())
            if name in ("enemy-se-banks", "game-object-sounds", "game-enemy-resources") and args.hardware_audio:
                arguments.append("--hardware-audio")
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
    manifest["limitations"] = ["Developer native startup/MC/movie/Main execution reaches an explicit original-callee frontier; full boot/game frame/scene rendering incomplete",
                               "Real limited MC voices and Movie0 device output use explicit native references; full BGM/SPU2 audio behavior incomplete",
                               "Serial task sleep/start/exit use native fibers; state4, async EE syscalls, interrupt/priority preemption remain unsupported",
                               "Original cHeap capacity/fragmentation and final GS indexed texture blending remain unverified",
                               "PS2 numerical reference is an implementation model, not a hardware capture",
                               "No full-ELF retail matching claim for the patched disc"]
    report.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("Report: " + str(report), flush=True)


if __name__ == "__main__":
    main()
