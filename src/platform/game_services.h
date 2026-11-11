#ifndef GH_GAME_SERVICES_H
#define GH_GAME_SERVICES_H
#include "../core/game_start.h"
#include "../core/game_init.h"
#include "../core/game_stage.h"
#include "../core/game_children.h"
#include "../core/game_scene_meshes.h"
#include "../core/id_base.h"
#include "../core/sound_control.h"
#include "../core/game_movie_pool.h"
#include "../core/game_prepare.h"
#include "../core/game_room_state.h"
#include "../core/game_room_pack.h"
#include "../core/game_room_pools.h"
#include "../core/game_camera.h"
#include "../core/game_motion_camera.h"
#include "../core/game_cockpit.h"
#include "../core/game_objects.h"
#include "../core/screen_fade.h"
#include "../core/game_room_module.h"
#include "../core/sound_scene.h"
#include "../core/game_collision_pairs.h"
#include "../core/game_effects.h"
#include "../core/game_scene_filters.h"
#include "../core/bgm_requests.h"
#include "../core/game_world_light.h"
#include "../core/game_actor_light_globals.h"
#include "../core/game_model_packets.h"
#include "../core/game_collision_solids.h"
#include "../core/game_scene_tables.h"
#include "../core/game_title_player.h"
#include "../core/game_checkpoint.h"
#include "../core/game_scene_ui.h"
#include "../core/game_scene_at.h"
#include "../core/game_action_buttons.h"
#include "../core/game_scene_spawns.h"
#include "../core/game_scene_placements.h"
#include "../core/game_scene_finish.h"
#include "../core/game_scene_step.h"
#include "../core/game_event.h"
#include "../core/display_state.h"
#include "../core/game_camera_step.h"
#include "../core/game_door.h"
#include "../rooms/r020_title.h"
#include "../core/game_camera_follow.h"
#include "../core/game_camera_envelope.h"
#include "../core/game_player_child_tail.h"
#include "../core/game_math_globals.h"
#include "../core/game_player_ground_pose.h"
#include "../core/game_player_opacity.h"
#include "../core/game_om_door_geometry.h"
#include "../core/game_player_node_cache.h"
#include "../core/game_actor_motion_ik.h"
#include "../core/game_event_environment.h"
#include "../core/packed_message.h"
#include "../core/game_event_clear.h"
#include "../core/game_event_actor_extra.h"
#include "../core/game_cloth_runtime.h"
#include "../core/game_actor_render.h"
#include "../core/game_object_step.h"
#include "../core/frame_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_game_services gh_game_services;
typedef enum {GH_SERVICES_OK,GH_SERVICES_ARGUMENT,GH_SERVICES_MEMORY,
 GH_SERVICES_BUSY,GH_SERVICES_THREAD,GH_SERVICES_ASSET,GH_SERVICES_FRONTIER} gh_game_services_status;
typedef struct {
 gh_game_assets *assets;gh_game_loop *loop;gh_game_init_state *init;
 gh_stage_case1 *stage;gh_game_children **children;
 gh_core_save_view save;float *rate;gh_boot_work *work;gh_task_threads *threads;
 gh_id_state *id,**id_global;gh_id_runtime **runtime;
 gh_font_state *font;gh_menu_state *menu;
 const void *core_bin;size_t core_bin_size;const gh_bundle *language;
 gh_sound_control *sound_control;gh_movie_pool *movie_pool; /*borrowRootuniqueowners*/
 gh_screen_fade *fade; /*actual747470, borrowedRootONEowner*/
 gh_audio_registry *audio_registry; /*sameactualstartup/voiceowner*/
 uint8_t *em_set_first_byte; /*canonical00586AB0fieldonly;notwholeEmconstructor*/
 gh_bgm_requests *bgm_requests; /*RootONEoriginal16requestpool,borrowed*/
 gh_sound_scene *sound_scene; /*RootONEearlycSndextraowner,borrowed*/
 gh_game_event *event; /*RootONE586AF0fromactualmappedbootELF*/
 gh_display_state *display; /*RootONEcanonicaldisplay/font/projectionglobals*/
 gh_r020_move_callbacks title_callbacks; /*borrowRootfrontend/providercontext*/
 const gh_r020_reference_profile *title_reference_profile; /*borrowuntilactualR020Initcopiesit*/
 uint8_t *factory_serial; /*ONEpersistent3C3D18byte, importedfromactualELF*/
 const void *boot_elf;size_t boot_elf_bytes; /*borrowactualimmutableimageforconstructorconstants*/
 gh_game_services_status (*start_loading_menu)(void *,uint32_t *frontier);
 void *loading_context;
 gh_camera_follow *camera_follow; /*RootONEcontinuation; bind onlyatactual139DB8*/
 gh_camera_envelope *camera_envelope; /*RootONE3BD540; borrow SAMEplayer modelparts*/
 gh_player_shared_vector *player_shared_vector; /*RootONE mutable3BD880 across rooms*/
 gh_game_math_globals *math_globals; /*RootONE actual3079C8 globals*/
 gh_om_door_shared *door_shared; /*RootONE mutable429290 six attributes*/
 gh_player_node_names *player_node_names;
 gh_motion_ik_globals *motion_ik_globals; /*RootONE mutable math vectors*/
 gh_game_event_config *event_config; /*same Root586B30, not a room copy*/
 gh_packed_message_state *packed; /*same Root packed/font state*/
 struct gh_enemy_se_banks *enemy_banks; /*same Root sound-control bank owner*/
 gh_cloth_templates *cloth_templates;gh_cloth_runtime_globals *cloth_globals; /*Rootpersistent ctor owners*/
 gh_player_ground_globals *player_ground_globals; /*RootONE original.data3BD470*/
 gh_player_opacity_globals *player_opacity_globals;gh_se_voice_engine *voices; /*borrow RootONE owners*/
 const gh_actor_light_globals *actor_light_globals; /*RootONE3BD870/890, no permodel reset*/
 gh_frame_random *frame_random; /*same auxiliary 747B10 owner as the frame driver*/
 gh_game_motion_camera *motion_camera; /*RootONE CamManager+530, shared by EVT and room scripts*/
} gh_game_services_bindings;
typedef struct {
 uint32_t frontier;int detail;uint8_t state;unsigned scenario_active;
 uint32_t id_entries,id_runs;int cockpit_ready,room_loaded,room_pools_ready;
 int frontier_is_r020;
} gh_game_services_snapshot;
typedef struct {uint32_t kind;int32_t handle;uint32_t field;} gh_game_main_record;
/* Borrowed canonicalowners for the next originalcase3 callee slice. No caller
 * maydestroy/replace these owners or retain them beyondservices lifetime.
 * RoomPack/Pools readiness remains explicit; exposing a view is not readiness. */
typedef struct {
 const gh_game_services_bindings *root;
 gh_game_prepare *prepare;gh_task_manager *scenario;gh_task_threads *scenario_threads;
 uint8_t *scenario_bytes;size_t scenario_bytes_size; /*nativepointerfieldsusetypedviews*/
 gh_room_state *room_state;gh_room_pack *room_pack;gh_room_pools *pools;
 gh_game_pl_camera *pl_camera;gh_game_cockpit *cockpit;
 gh_game_objects *objects,*room_objects;
 gh_room_module *module;gh_sound_scene *sound_scene;gh_collision_pairs *collision_pairs;
 gh_game_effects *effects;gh_scene_filters *filters;
 const struct gh_esp14_provider *model_effect;
 gh_game_world_light *world_light;gh_game_model_packets *model_packets;
 gh_game_player_child **work5f8; /*canonical typed child publication*/
 const gh_event_weapon_slot *event_weapon; /*canonical player+6A8 provenance*/
 gh_game_collision_solids *collision_solids;gh_game_scene_tables *scene_tables;
 gh_game_title_player *title_player; /*canonicaltypedWork.playerObj*/
 const gh_actor_model *player_model;
 gh_game_checkpoint *checkpoint;gh_game_scene_ui *scene_ui;
 gh_scene_at_manager *scene_at;gh_game_action_buttons *action_buttons;
 gh_em_scene_placements *em_placements;gh_om_scene_placements *om_placements;gh_acd_scene_binding *acd;
 gh_game_scene_step *step;gh_game_event *event;gh_display_state *display;gh_r020_title *title;
 gh_scene_hook *scenario_exit,*scenario_callback104;
 gh_game_door *door;uint16_t *door_wait;uint8_t *system_menu_enabled;
 gh_scene_meshes *meshes;
 uint32_t room_texture_channel;int room_texture_ready;
 uint32_t *main_word4;gh_game_main_record *main_records;size_t main_record_count;
 uint8_t *scene_buffer_a,*scene_buffer_b;size_t scene_buffer_a_size,scene_buffer_b_size;
} gh_game_services_scene_view;
gh_game_services_status gh_game_services_get_scene_view(gh_game_services *,gh_game_services_scene_view *);
/* Bind the Event controller's factory roster to the live room and resolver. */
gh_game_event_status gh_game_services_bind_event_actor_roster(gh_game_services *,
 gh_event_actor_roster *,gh_event_actor_factory *rows,size_t capacity);
gh_game_event_status gh_game_services_bind_event_actor_extra(gh_game_services *,
 gh_event_actor_extra *,const gh_event_actor_create *);
/* Read-only native execution diagnostics; counters are not game state. */
typedef struct {uint64_t player_updates_completed,child_updates_completed,om_updates_completed;
 uint32_t last_move_handle,last_move_entry,player_stage,player_frontier,child_stage,child_frontier;
 uint32_t player_handle,player_state_entry;
 uint32_t child_forces_complete,child_angles_complete,child_distances_complete,child_finish_complete;
 uint32_t emitter_active,esp_active,emitter_samples;
 uint32_t emitter_slots[4],emitter_banks[4],emitter_kinds[4],emitter_flags[4],emitter_known[4],emitter_types[4];
} gh_game_update_diagnostic;
gh_game_services_status gh_game_services_update_diagnostic(const gh_game_services *,gh_game_update_diagnostic *);

/* Resolve actual canonical actor identity before caller-specific ID filtering. */
gh_game_services_status gh_game_services_actor_view(gh_game_services *,uint32_t handle,gh_title_player_view *);
/* Original visibility bit writes on every node of the SAME registered model,
 * including a static scroll object's one canonical node. */
gh_game_services_status gh_game_services_mask_actor_nodes(gh_game_services *,uint32_t handle,
 uint32_t and_mask,uint32_t or_mask,uint32_t *frontier);
/* Dispatches a verified original Move descriptor on the SAME canonical actor.
 * Unknown methods/this-adjustments retain their exact original entry frontier. */
/* The caller owns the original Event/room condition and task. Re-entry
 * resumes the same RoomSave-backed resource and native actor construction. */
gh_game_services_status gh_game_services_enemy_set_all_00294ca0(gh_game_services *,uint32_t *,int *);
gh_game_services_status gh_game_services_move_actor(gh_game_services *,uint32_t handle,uint32_t original_entry,
 int16_t this_delta,const gh_title_player_view *,const gh_boot_work *,uint32_t *frontier);
typedef struct {const uint8_t *depth_shift;const gh_shadow_templates *shadow_templates;
 gh_shadow_emit shadow_emit;void *shadow_context;
 gh_actor_draw_emit actor_emit;void *actor_context;const gh_game_pl_camera *camera;
 gh_effect_trans_emit marker_emit;void *marker_context;} gh_game_actor_render_provider;
/* Move may produce an original shadow packet on Root's current OT. */
gh_game_services_status gh_game_services_move_actor_with_render(gh_game_services *,uint32_t handle,
 uint32_t original_entry,int16_t this_delta,const gh_title_player_view *,const gh_boot_work *,
 const gh_game_actor_render_provider *,uint32_t *frontier);
/* Actual nonstatic Trans descriptor on the SAME registered model. Draw
 * continuations live with that model; unsupported alternate/destructor bodies
 * retain their exact original entry. The provider borrows Root's one OT. */
gh_game_services_status gh_game_services_trans_actor(gh_game_services *,const gh_object_trans_request *,
 const gh_boot_work *,const gh_game_actor_render_provider *,uint32_t *frontier);
/* Borrow the same player and object resolver used by ordinary target selection. */
gh_player_target_bindings gh_game_services_scene_targets(gh_game_services *);
/* Inspect native script errors immediately after a real Scenario visit. */
gh_scene_step_status gh_game_services_task_status(gh_game_services *,uint32_t *frontier);
/* Actual12C418 on SAME player/model/motion, with one invocation cursor. */
gh_game_event_status gh_game_services_event_return_0031c7e0(gh_game_services *,
 const gh_game_event *, uint32_t *frontier);
gh_game_event_status gh_game_services_event_player_clear(gh_game_services *,const gh_event_clear_bindings *,uint32_t *frontier);
/* Native continuation/cache metadata only, after original state5 chooses the
 * next Main2/3 visit. Actual room allocations expire later at2A6210. */
gh_game_services_status gh_game_services_begin_next_room(gh_game_services *);
uint64_t gh_game_services_ui_resource_epoch(const gh_game_services *);
/* Borrow every existing canonical rootobject. Runtime/children use root-owned
 * pointer slots, never duplicate rootidentity. New owners and16taskScenario
 * storage are private. CancreatefromexecutingMain task on nativebackend.
 * Explicit pathREFERENCE_NEAREST and cameraBINARY32_REFERENCE numericprofile. */
gh_game_services_status gh_game_services_create(const gh_game_services_bindings *,gh_game_services **);
gh_game_services_status gh_game_services_state1(gh_game_services *);
gh_game_services_status gh_game_services_state2(gh_game_services *);
gh_game_services_status gh_game_services_state3(gh_game_services *);
void gh_game_services_inspect(const gh_game_services *,gh_game_services_snapshot *);
uint32_t gh_game_services_frontier(const gh_game_services *);
int gh_game_services_detail(const gh_game_services *);
/* NULL leavesexistingmainELF/driver resolver incharge; r020.rel explicitly
 * identifiesREL-relativeWait/Title/Jump entries, never guessesfromlowaddress. */
const char *gh_game_services_frontier_module(const gh_game_services *);
/* Caller firstcancels/finishes Main usingthese bindings. Later externalIDruns
 * mustbereleasedbeforeCockpit; errorsretainownerforretry. DestroysScenario
 * beforeRootbackend, views beforeassets. Doesnotown/rootruntime,assets,save,
 * borrowedstage or preexistingchildren/textures. Freesowneronlyonsuccess. */
gh_game_services_status gh_game_services_destroy(gh_game_services *);
#ifdef __cplusplus
}
#endif
#endif
