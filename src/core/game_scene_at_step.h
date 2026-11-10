#ifndef GH_GAME_SCENE_AT_STEP_H
#define GH_GAME_SCENE_AT_STEP_H
#include "game_scene_at.h"
#include "game_ray_query.h"
#include "game_title_player.h"
#include "scene_at_units.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2C1618: even an empty interaction list first casts the player's ray.
 * Finite general world matrix and the real canonical collision query;
 * never converts an unimplemented ray into a no-hit callback. Does not repeat
 * the separate2C14F8 cursor clear. Compatibility entry without Work preserves the positive-unit frontier. */
gh_scene_at_status gh_scene_at_step_002c1618(gh_scene_at_manager *,gh_game_scene_tables *,const gh_title_player_view *,int *original_return,uint32_t *frontier);
typedef struct {gh_at_native_unit unit;uint8_t group,mode;uint32_t entry_address,raw_argument,argument_mode;} gh_scene_at_task_request;
typedef struct {void *context;
 gh_scene_at_status (*dispatch)(void *,uint32_t original_entry,gh_at_native_unit,uint8_t trigger_mask,int *original_return,uint32_t *frontier);
 gh_scene_at_status (*spawn)(void *,const gh_scene_at_task_request *,uint32_t *frontier);
} gh_scene_at_step_callbacks;
/* One original update. Positive units use SAME native tagged lists and mutable
 * bytes. Failure preserves source prefix writes but is not a completed update;
 * caller must not restart an already-partially-executed external callback.
 * Work is canonical: trigger1A0,logical178,5D8/5E4 gates and5EC flags. */
gh_scene_at_status gh_scene_at_step_native_002c1618(gh_scene_at_manager *,gh_game_scene_tables *,const gh_title_player_view *,gh_boot_work *,const gh_scene_at_step_callbacks *,int *original_return,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
