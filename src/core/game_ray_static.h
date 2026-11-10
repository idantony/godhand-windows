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
/* Shared original1365B8 quad primitive. Caller owns the binary32/FEnv scope
 * and nearest/report state. Returns -1 numeric/argument limitation, 0 miss,
 * 1 intersection, writing three coordinates through out. */
int gh_scene_ray_quad_intersection_001365b8(const float from[3],const float to[3],
 const float vertices[4][3],const float normal[3],float out[3]);
#ifdef __cplusplus
}
#endif
#endif
