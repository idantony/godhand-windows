#ifndef GH_GAME_CLOTH_POINT_H
#define GH_GAME_CLOTH_POINT_H
#include "game_cloth_templates.h"
#include "game_player_child_tail.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual1067B8, original flags80-clear anchoredpoint bodyresponse.
 * Original flags field is NOTtested here; sphere centers/radii are consumed
 * for everyrecord. Same mutable3BD880 pivot must be borrowed. */
gh_cloth_status gh_cloth_point_body_001067b8(const gh_cloth_geometry *,float point[3],const float anchor[3],gh_player_shared_vector *,gh_static_transform_policy,int *original_hit,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
