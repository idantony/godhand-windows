#ifndef GH_GAME_SERVICES_H
#define GH_GAME_SERVICES_H
#include "../core/game_start.h"
#include "../core/game_init.h"
#include "../core/game_stage.h"
#include "../core/game_children.h"
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
} gh_game_services_bindings;
typedef struct {
 uint32_t frontier;int detail;uint8_t state;unsigned scenario_active;
 uint32_t id_entries,id_runs;int cockpit_ready,room_loaded,room_pools_ready;
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
 uint32_t *main_word4;gh_game_main_record *main_records;size_t main_record_count;
 uint8_t *scene_buffer_a,*scene_buffer_b;size_t scene_buffer_a_size,scene_buffer_b_size;
} gh_game_services_scene_view;
gh_game_services_status gh_game_services_get_scene_view(gh_game_services *,gh_game_services_scene_view *);
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
/* Caller firstcancels/finishes Main usingthese bindings. Later externalIDruns
 * mustbereleasedbeforeCockpit; errorsretainownerforretry. DestroysScenario
 * beforeRootbackend, views beforeassets. Doesnotown/rootruntime,assets,save,
 * borrowedstage or preexistingchildren/textures. Freesowneronlyonsuccess. */
gh_game_services_status gh_game_services_destroy(gh_game_services *);
#ifdef __cplusplus
}
#endif
#endif
