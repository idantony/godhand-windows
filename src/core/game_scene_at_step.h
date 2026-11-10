#ifndef GH_GAME_SCENE_AT_STEP_H
#define GH_GAME_SCENE_AT_STEP_H
#include "game_scene_at.h"
#include "game_ray_query.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2C1618: even an empty interaction list first casts the player's ray.
 * Supported constructor-identity world matrix and actual outside-grid query;
 * never converts an unimplemented ray into a no-hit callback. Does not repeat
 * the separate2C14F8 cursor clear. Positive units retain their exact consumer. */
gh_scene_at_status gh_scene_at_step_002c1618(gh_scene_at_manager *,gh_game_scene_tables *,const gh_title_player_view *,int *original_return,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
