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
#include "../core/game_om_door_geometry.h"
#include "../core/game_player_node_cache.h"
#include "../core/game_actor_motion_ik.h"
#include "../core/game_event_environment.h"
#include "../core/packed_message.h"
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
 gh_game_world_light *world_light;gh_game_model_packets *model_packets;
 gh_game_player_child **work5f8; /*canonical typed child publication*/
 gh_game_collision_solids *collision_solids;gh_game_scene_tables *scene_tables;
 gh_game_title_player *title_player; /*canonicaltypedWork.playerObj*/
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
/* Resolve actual canonical actor identity before caller-specific ID filtering. */
gh_game_services_status gh_game_services_actor_view(gh_game_services *,uint32_t handle,gh_title_player_view *);
/* Inspect native script errors immediately after a real Scenario visit. */
gh_scene_step_status gh_game_services_task_status(gh_game_services *,uint32_t *frontier);
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
