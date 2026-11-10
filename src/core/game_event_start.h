#ifndef GH_GAME_EVENT_START_H
#define GH_GAME_EVENT_START_H
#include "game_event_environment.h"
#include "game_scene_meshes.h"
#include "game_om_factory.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Resolver must return SAME canonical handle owner bytes, including player/child;
 * ID filtering happens AFTER resolution, never by an OM-only cast. */
typedef gh_game_event_status (*gh_event_object_view)(void *,uint32_t handle,gh_title_player_view *,uint32_t *frontier);
/* Required true2B65B0 provider: failure must be retry-safe, or retain its own
 * continuation. Returning OK means the original menu task was started. */
typedef gh_game_event_status (*gh_event_system_menu_start)(void *,uint32_t *frontier);
typedef struct {gh_game_event *event;const gh_game_event_config *config;gh_boot_work *work;
 gh_screen_fade *fade;const gh_screen_fade_context *fade_context;
 gh_game_scene_tables *tables;gh_room_pools *pools;const gh_scene_meshes *meshes;
 gh_event_object_view object_view;gh_event_system_menu_start system_menu_start;void *context;} gh_event_start_bindings;
/* Host continuation only: preserves completed native prefix if a required
 * consumer fails. Borrow all owners until completion; no second registry. */
typedef struct {const gh_game_event *event;const gh_boot_work *work;uint32_t stage,frontier;int detail;} gh_event_start_state;
gh_game_event_status gh_event_clear_object_hidden_00308a38(const gh_event_start_bindings *,uint32_t *);
gh_game_event_status gh_event_hide_scenario_objects_002c5068(const gh_event_start_bindings *,int,uint32_t *);
gh_game_event_status gh_event_start_002965f0(gh_event_start_state *,const gh_event_start_bindings *,int *done);
#ifdef __cplusplus
}
#endif
#endif
