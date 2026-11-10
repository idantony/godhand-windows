#ifndef GH_GAME_SCENE_EXIT_LEAVES_H
#define GH_GAME_SCENE_EXIT_LEAVES_H
#include "game_collision_pairs.h"
#include "game_world_light.h"
#ifdef __cplusplus
extern "C" {
#endif
gh_collision_status gh_collision_pairs_clear_002a71f0(gh_collision_pairs *,uint32_t *frontier);
gh_collision_status gh_collision_pairs_free_002a7280(gh_collision_pairs *,int *original_return);
gh_light_status gh_world_light_exit_002d73e0(gh_game_world_light *);
#ifdef __cplusplus
}
#endif
#endif
