#ifndef GH_GAME_SCENE_AT_GEOMETRY_H
#define GH_GAME_SCENE_AT_GEOMETRY_H
#include "game_scene_at_step.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual1508D8 tailcall30A2B0. Finite binary32 reference, pointW=1. */
gh_scene_at_status gh_scene_at_apply_world_001508d8(const float matrix[16],const float point[3],float output[4]);
/* Original shape is unit+4: type1 vertical convexquad, type2 cylinder.
 * Y band is inclusive; quad edges inclusive; radius comparison strict.
 * Unknown shape kinds genuinely returnfalse in original1F83E8. */
gh_scene_at_status gh_scene_at_region_contains_001f83e8(const uint8_t shape[0x30],const float point[3],int *inside);
#ifdef __cplusplus
}
#endif
#endif
