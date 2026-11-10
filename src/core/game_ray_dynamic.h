#ifndef GH_GAME_RAY_DYNAMIC_H
#define GH_GAME_RAY_DYNAMIC_H
#include "game_ray_query.h"
#include "game_scene_dynamic.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual130700 consumes already prepared SAME actor ray scratch. Outputs must
 * be caller-owned staging buffers. Dynamic reports use triangle_index=FFFF,
 * attribute_index=face. Actor identities are canonical allocation bytes.
 * Does not update geometry, pump tasks, or commit external hit vectors. */
gh_scene_ray_status gh_scene_ray_dynamic_00130700(gh_scene_dynamic_owner *,const gh_scene_ray_request *,uint32_t *);
#ifdef __cplusplus
}
#endif
#endif
