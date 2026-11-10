#ifndef GH_GAME_RAY_STATIC_H
#define GH_GAME_RAY_STATIC_H
#include "game_ray_query.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native continuation of12F288/12F4C8/12F600/12F750. Borrows the SAME
 * actor tables including current mutable triangle flags and same190/194
 * dynamic boxes. Stages both attribute/actor outputs until all traversal
 * succeeds; geometry must already be updated by original12E840. No fake no-hit. */
gh_scene_ray_status gh_scene_ray_static_overlap(gh_game_scene_tables *,const gh_scene_ray_request *,int *,uint32_t *);
/* Shared1321D8 traversal primitives: caller supplies its original AABB;
 * visited mutation is restricted to clear-selected-range and test-and-visit. */
gh_scene_ray_status gh_scene_query_grid_0012f288(gh_game_scene_tables *,const float minimum[3],const float maximum[3],int *overlap,uint32_t *frontier);
gh_scene_ray_status gh_scene_query_clear_0012f4c8(gh_game_scene_tables *,uint32_t mask,uint32_t *frontier);
gh_scene_ray_status gh_scene_query_visit_0012fc20(gh_game_scene_tables *,uint16_t,int *skip,uint32_t *flags,uint32_t *frontier);
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
