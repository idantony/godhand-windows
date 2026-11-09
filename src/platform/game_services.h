#ifndef GH_GAME_SERVICES_H
#define GH_GAME_SERVICES_H
#include "../core/game_start.h"
#include "../core/game_init.h"
#include "../core/game_stage.h"
#include "../core/game_children.h"
#include "../core/id_base.h"
#include "../core/sound_control.h"
#include "../core/game_movie_pool.h"
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
} gh_game_services_bindings;
typedef struct {
 uint32_t frontier;int detail;uint8_t state;unsigned scenario_active;
 uint32_t id_entries,id_runs;int cockpit_ready,room_loaded,room_pools_ready;
} gh_game_services_snapshot;
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
