#ifndef GH_STARTUP_HOST_H
#define GH_STARTUP_HOST_H
#include "audio_startup.h"
#include "task_threads.h"
#include "../core/startup_task.h"
#include "../core/font_state.h"
#include "../core/menu_state.h"
#include "../core/mc_task.h"
#include "../core/mc_control.h"
#include "video_player.h"
#include "native_input.h"
#include "../core/se_voice.h"
#include "../core/sound_control.h"
#include "../core/game_movie_pool.h"
#include "../core/screen_fade.h"
#include "../core/bgm_requests.h"
#include "../core/display_state.h"
#include "../core/game_cockpit.h"
#include "../core/game_children.h"
#include "../core/game_object_step.h"
#include "../core/game_actor_shadow_draw.h"
#include "../core/game_actor_model_draw.h"
#include "../core/game_effects_trans.h"
#include "../core/game_scene_message_draw.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_startup_host gh_startup_host;
typedef gh_id_error (*gh_host_mc_frame_begin)(void *);
typedef gh_id_error (*gh_host_fade_consumer)(void *,const gh_fade_draw *);
typedef struct {
 void *context;
 gh_id_error (*begin)(void *,uint32_t *frontier);
 gh_id_error (*draw)(void *,const gh_id_base *,const gh_id_draw_job *,uint32_t *frontier);
 gh_id_error (*fan)(void *,const gh_cockpit_fan *,uint32_t *frontier);
 gh_id_error (*cover)(void *,const gh_child_cover_quad *,uint32_t *frontier);
 gh_scene_message_emit message;
 gh_id_error (*radar)(void *,const gh_cockpit_radar_quad *,uint32_t *frontier);
} gh_host_scene_ui_callbacks;
typedef struct gh_startup_continuation {
    int enabled;
    int muted; /* QA/user master mute, never simulated audio completion */
    gh_se_dsp_policy audio_dsp_policy; /* explicit native reference profile */
    const char *ffmpeg_path; /* copied; missing codec/presenter is a frontier */
    gh_video_present movie_present;
    void *movie_present_context;
} gh_startup_continuation;
typedef enum {
    GH_HOST_RUNNING, GH_HOST_FRONTIER, GH_HOST_COMPLETE, GH_HOST_FAILED
} gh_host_phase;
typedef enum {
    GH_HOST_OK, GH_HOST_ARGUMENT, GH_HOST_MEMORY, GH_HOST_DISC,
    GH_HOST_THREADS, GH_HOST_ASSET, GH_HOST_AUDIO, GH_HOST_ENTRY_UNAVAILABLE
} gh_host_error;
typedef struct gh_startup_snapshot {
    gh_host_phase phase;
    gh_host_error error;
    int subsystem_error;
    unsigned failed_operation;
    uint32_t unavailable_entry;
    uint64_t ticks;
    uint32_t flags;
    unsigned loaded_bundles;
    int startup_returned;
    const gh_font_state *font;
    const gh_menu_state *menu;
    const gh_id_state *id;
    const gh_audio_registry *audio;
    uint8_t game_state;
    int game_assets_loaded;
    float game_rate;
    const char *frontier_module; /* NULL=main ELF, otherwise module-relative entry */
    const gh_id_base *mc_ui;
    const gh_packed_message_state *mc_messages;
    uint32_t game_texture_banks;
    uint64_t mc_ui_epoch,mc_frame_serial;
    int mc_ui_live,mc_frame_ready;
    uint32_t mc_phase,mc_subphase,mc_present_mask;
    int32_t mc_error;
    int mc_done,normal_continuation;
    const gh_boot_work *work; /* borrowed read-only, exact input fields */
    const gh_sound_control *sound_control;
    const gh_movie_pool *movie_pool;
    gh_video_status movie_video;
    const gh_screen_fade *fade;
    uint64_t fade_draw_serial;
    const gh_bgm_requests *bgm;
    const gh_display_state *display;
    uint64_t scene_frame_serial;int scene_frame_ready;
    uint8_t title_live,title_mode,title_phase,title_subphase;
    uint32_t title_timer,title_game_selection,title_level;
    uint64_t title_move_serial;
    uint64_t ui_resource_epoch; /*native borrowed-resource/cache lifetime*/
    uint16_t current_room,next_room;
    /* Read-only developer frontier diagnostics; no actor pointers escape. */
    uint32_t scene_stage,hitarea_phase,hitarea_slot,hitarea_node;
    uint8_t scenario_task_failed,scenario_task_stage;
    int scenario_task_slot;
    uint64_t player_updates_completed,child_updates_completed,om_updates_completed;
    uint32_t last_move_handle,last_move_entry,player_update_stage,child_update_stage;
    uint32_t player_handle,player_state_entry,player_update_frontier,player_attack_target;
    float player_world_position[3],player_motion_sample;
    uint8_t player_pose_ready,player_state[4],player_square_buffer;
    uint32_t r100_first_enemy_handle;
    uint16_t r100_first_enemy_hp,r100_first_enemy_player_hit_kind;
    int32_t r100_first_enemy_player_hit_power;
    float r100_first_enemy_position[3];
    uint32_t child_update_frontier,child_forces_complete,child_angles_complete,child_distances_complete,child_finish_complete;
    uint32_t emitter_active,esp_active,emitter_samples;
    uint32_t emitter_slots[4],emitter_banks[4],emitter_kinds[4],emitter_flags[4],emitter_known[4],emitter_types[4];
    uint32_t current_actor_ready,current_actor_kind,current_actor_flags,current_actor_state;
    uint32_t current_actor_5b0,current_actor_5b4,current_actor_5b8,current_actor_hp548,current_actor_hp54a,current_actor_54c,current_actor_5c0[3];
    uint32_t current_enemy_placement_ready,current_enemy_stamp,current_enemy_dependency,current_enemy_appearance;
    uint32_t current_enemy_spawn_flags,current_enemy_phase,current_enemy_variant,current_enemy_serial;
    uint32_t current_enemy_distance_bits,current_enemy_route_bits;
    uint32_t current_enemy_flags_3ac,current_enemy_flags_16d0,current_enemy_flags_16d4;
    uint32_t current_enemy_reaction_set_17bd;
    uint32_t current_b_samples,current_b_index[4],current_b_group[4],current_b_generation[4],current_b_serial[4];
    uint32_t current_b_source_present[4],current_b_source_known[4],current_b_source_kind[4],current_b_words[4][28];
    uint32_t hitarea_shape_index,hitarea_shape_flags,hitarea_shape_kind;
    int32_t hitarea_hit_count;uint8_t hitarea_diagnostic_ready,hitarea_hit_samples;
    uint32_t hitarea_hit_indices[4],hitarea_hit_kinds[4],hitarea_hit_distance_bits[4];
    uint32_t hitarea_actor_present,hitarea_actor_kind;
    uint32_t hitarea_actor_flags,hitarea_actor_cooldown_bits,hitarea_source_unit_flags,hitarea_source_excluded_token,hitarea_source_is_player;
    uint32_t hitarea_hit_unit_known[4],hitarea_hit_unit_slots[4],hitarea_hit_actor_present[4],hitarea_hit_actor_kinds[4],hitarea_hit_actor_tokens[4];
    uint32_t hitarea_hit_actor_flags[4],hitarea_hit_actor_cooldown_bits[4],hitarea_hit_same_actor[4];
    uint16_t id_vertex_z; /* draw env +30 used by original 2C5EA8 */
    uint8_t id_vertex_z_known;
    uint8_t event_state,event_phase;
    int32_t event_number,event_callee_error,event_clear_detail;
    uint32_t event_flags,event_config_flags,event_clear_stage,event_audio_flags;
    uint32_t event_camera_cuts,event_actor_records,event_timing_rows;
    uint32_t event_actor_handle,event_actor_parts,event_actors_initialized,event_actor_accessory_handle;
    uint8_t event_actor_jacket_attached;
    int32_t event_cut;
    float event_cut_frame;
    uint8_t event_motion_camera_active;
    uint32_t effect_frontier, effect_emitter_slot, effect_emitter_serial;
    uint32_t effect_emitter_mode, effect_emitter_class, effect_emitter_step;
    uint32_t effect_esp_slot, effect_esp_step;
} gh_startup_snapshot;
/* Developer execution slice beginning at original task0030D358 in slot3 of
 * the four-slot manager. Does not run the earlier boot/GS/IOP initialization
 * or claim the whole game has booted. Language0..6, flags598 and heap capacity
 * are explicit host inputs, not recovered default cHeap policies.
 * Original disc stays read-only; resource views live until host destruction.
 * Same OS thread and creating caller fiber only, as task_threads requires. */
gh_host_error gh_startup_host_create(const char *iso,int32_t language,
    uint32_t flags598,size_t heap_capacity,gh_startup_host **);
/* Explicit native save-profile location. Opening never creates or modifies
 * it. The shorter create uses relative userdata/memory_card for this developer
 * host; absent directory is genuinely no card when Control later queries it. */
gh_host_error gh_startup_host_create_with_profile(const char *iso,const char *profile,
    int32_t language,uint32_t flags598,size_t heap_capacity,gh_startup_host **);
/* Configure actual consumers before the first visit. Without one, the host
 * returns an explicit draw frontier in original order. Callback success must
 * mean the actual consumer ran; collecting a job is not rendered success. */
gh_host_error gh_startup_host_set_mc_consumers(gh_startup_host *,gh_mc_draw_consumer,
    void *draw_context,gh_id_post_message,void *post_context);
/* Explicit opt-in before tick0. Default retains first-frame prefix mode.
 * Full mode requires the named DSP profile and real device; rendering/video
 * consumers are never replaced with empty successful callbacks. */
gh_host_error gh_startup_host_set_continuation(gh_startup_host *,const gh_startup_continuation *);
/* Optional actual renderer frame setup, before Move/Trans. No yielding allowed
 * inside consumers. Snapshot frame_serial advances only after a successful
 * complete draw reaches its actual task Sleep, so present once per serial.
 * UI epoch changes on creation/normal release; borrowed UI views can expire. */
gh_host_error gh_startup_host_set_mc_frame_begin(gh_startup_host *,gh_host_mc_frame_begin,void *);
/* Actual307388 runs after task traversal and before DVD/audio. A visible fade
 * requires a real native draw consumer; status queries never advance time. */
gh_host_error gh_startup_host_set_fade_consumer(gh_startup_host *,gh_host_fade_consumer,void *);
/* Optional native scene packet builders, before tick0. A completed original
 * state4 traversal is required before scene_frame_ready; partial queues at a
 * world/render frontier are never reported as complete scene frames. */
gh_host_error gh_startup_host_set_scene_ui_consumers(gh_startup_host *,const gh_host_scene_ui_callbacks *);
/* Original static-model packet groups join the SAME frame's OT as UI. The
 * callback must build the real native packet inputs; no empty draw ack. */
gh_host_error gh_startup_host_set_world_consumer(gh_startup_host *,gh_object_static_emit,void *);
/* Recovered actor shadows join that same frame's OT. An absent consumer
 * retains31A600; a collector alone is not a complete rendered frame. */
gh_host_error gh_startup_host_set_shadow_consumer(gh_startup_host *,gh_shadow_emit,void *);
/* Recovered dynamic-model node/setup groups use the same single frame OT. */
gh_host_error gh_startup_host_set_actor_consumer(gh_startup_host *,gh_actor_draw_emit,void *);
gh_host_error gh_startup_host_set_effect_consumer(gh_startup_host *,gh_effect_trans_emit,void *);
/* Same-thread rendering callback only; borrow the canonical texture manager's
 * material view until its resource epoch/cache expires. Never creates a second
 * manager, reads the disc or acknowledges a GPU operation. */
gh_host_error gh_startup_host_resolve_tex0(gh_startup_host *,uint64_t,const gh_stage_material **);
/* Owner-fiber only, normally once before each timed engine tick. Writes raw
 * 32-bit and logical64-bit original work fields via memcpy, no host-long casts.
 * Does not invent physical input, replay, analog or pad-lock policy. */
gh_host_error gh_startup_host_set_input(gh_startup_host *,const gh_native_input_snapshot *);
/* Advances selected tasks, then the actual DVD queue in original relative
 * order. A tick is a scheduler visit, not a timed/video frame. Audio startup
 * performs actual synchronous native commits; no fake per-frame RPC polling.
 * Unrecovered next entries stop at FRONTIER instead of successful empty tasks.
 * Recovered next tasks may run too; independent recovered tasks drain before
 * reporting another task's frontier. COMPLETE never means a playable game.
 * A call from the wrong owner returns FAILED without mutating host state;
 * snapshot still reports the session's actual execution phase. */
gh_host_phase gh_startup_host_tick(gh_startup_host *);
gh_host_error gh_startup_host_snapshot(const gh_startup_host *,gh_startup_snapshot *);
gh_host_error gh_startup_host_destroy(gh_startup_host *);
#ifdef __cplusplus
}
#endif
#endif
