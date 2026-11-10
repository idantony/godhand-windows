#ifndef GH_GAME_SCENE_BOX_VISIBILITY_H
#define GH_GAME_SCENE_BOX_VISIBILITY_H
#include "game_scene_post.h"
#include "../assets/static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual1F92C0 transformed center/halfextent box and original entry-face
 * tests. Both byteviews borrowed from SAME actor260/80; no pointer casts.
 * Explicit finite binary32 reference, not EE/VU capture. */
gh_scene_post_status gh_scene_box_rays_001f92c0(const void *box24,const void *matrix64,
 const gh_scene_model_ray *,size_t count,gh_static_transform_policy,int *original_hit,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
