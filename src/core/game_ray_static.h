#ifndef GH_GAME_RAY_STATIC_H
#define GH_GAME_RAY_STATIC_H
#include "game_ray_query.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native continuation of12F288/12F4C8/12F600/12F750. Borrows the SAME
 * actor tables including current mutable triangle flags. No fake no-hit. */
gh_scene_ray_status gh_scene_ray_static_overlap(gh_game_scene_tables *,const gh_scene_ray_request *,int *,uint32_t *);
void gh_scene_ray_triangle_state_release(gh_game_scene_tables *);
gh_scene_ray_status gh_scene_ray_triangle_flags(const gh_game_scene_tables *,uint16_t,uint32_t *);
#ifdef __cplusplus
}
#endif
#endif
