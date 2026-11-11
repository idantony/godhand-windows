#ifndef GH_GAME_SCENE_EXIT_H
#define GH_GAME_SCENE_EXIT_H
#include "game_system_menu.h"
#include "game_movie_pool.h"
#include "game_scene_setup.h"
#include "game_room_module.h"
#include "game_room_pools.h"
#include "game_scene_tables.h"
#include "game_scene_at.h"
#include "game_objects.h"
#include "game_event.h"
#include "game_prepare.h"
#include "screen_fade.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_SCENE_EXIT_OK,GH_SCENE_EXIT_ARGUMENT,GH_SCENE_EXIT_BOUNDS,
 GH_SCENE_EXIT_THREAD,GH_SCENE_EXIT_UNSUPPORTED,GH_SCENE_EXIT_CALLEE} gh_scene_exit_status;
/* These are actual unresolved operation boundaries. original_return is used
 * only for2CAB58 (ready iff1); an absent implementation is never success. */
typedef gh_scene_exit_status (*gh_scene_exit_external)(void*,uint32_t address,uint32_t original_argument,int *original_return,uint32_t *frontier);
typedef struct {
 gh_game_loop *loop;gh_core_save_view save;gh_screen_fade *fade;gh_movie_pool *movies;
 gh_game_system_menu *system_menu;gh_system_menu_bindings system_menu_bindings;
 gh_scene_scenario_view scenario;gh_task_threads *scenario_threads;
 gh_scene_hook *scenario_pending_f4,*scenario_exit_ec;
 gh_game_event *event;gh_room_module *module;gh_room_pools *pools;
 gh_game_objects *objects,*room_objects;gh_scene_at_manager *scene_at;
 uint8_t *scene_buffer_a,*scene_buffer_b;size_t scene_buffer_a_size,scene_buffer_b_size;
 gh_scene_exit_external external;void *context;
} gh_game_scene_exit_bindings;
typedef struct {uint32_t operation,frontier;uint8_t phase,scenario_stage;
 uint32_t scenario_cursor;int detail;uint64_t completed_calls;} gh_game_scene_exit;
/* Main state5@2A67D0 ordered per-frame caller. Persistentcursor only preserves
 * an interrupted host continuation; phase2 not-ready remains original samephase
 * andphase3/4 each consume a real Mainvisit. Does not destroy whole services. */
gh_scene_exit_status gh_game_scene_exit_002a67d0(gh_game_scene_exit*,const gh_game_scene_exit_bindings*);
/* Shared original Scenario end hooks and task retirement, also used by
 * the Door progression map before returning to the room transition. */
gh_scene_exit_status gh_game_scene_scenario_stop_002c3858(gh_game_scene_exit *,const gh_game_scene_exit_bindings *);
/* Concrete all-kill gates of305B90; native actor destruction is still required
 * for live/unpaused/unhidden bits. No table/resource reconstruction occurs. */
gh_scene_exit_status gh_scene_exit_effects_00305b90(uint8_t*,size_t,uint8_t*,size_t,uint32_t *);
#ifdef __cplusplus
}
#endif
#endif
