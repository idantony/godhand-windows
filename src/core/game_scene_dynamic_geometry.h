#ifndef GH_GAME_SCENE_DYNAMIC_GEOMETRY_H
#define GH_GAME_SCENE_DYNAMIC_GEOMETRY_H
#include "game_scene_dynamic.h"
#include "game_dynamic_corners.h"
#include "game_actor_matrix.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual41D558+1C=12DCB8. Flag4 selects borrowed externalmatrix, notdirty.
 * Writes real8worldcorners/AABB only after complete finite-reference work;
 * preserves originalflags/attributes/Wlanes and marks hostworld_ready. */
gh_scene_dynamic_status gh_scene_dynamic_box_update_0012dcb8(gh_scene_dynamic_owner *,struct gh_scene_dynamic_box *,const gh_dynamic_corner_tables *,gh_static_transform_policy,uint32_t *frontier);
gh_scene_dynamic_status gh_scene_dynamic_quad_update_0012d1a8(gh_scene_dynamic_owner *,struct gh_scene_dynamic_box *,const gh_dynamic_corner_tables *,gh_static_transform_policy,uint32_t *frontier);
/* Original12E840 update traversal with persistent per-owner native cursor.
 * Work594 debug branch remains exact2DB6B0 frontier; retry doesnotreplay
 * completed earlierboxes. Complete success alone starts a new nextframe. */
gh_scene_dynamic_status gh_scene_dynamic_step_0012e840(gh_scene_dynamic_owner *,const gh_dynamic_corner_tables *,const gh_boot_work *,gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
