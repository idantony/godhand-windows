#ifndef GH_GAME_SCENE_AT_DAMAGE_H
#define GH_GAME_SCENE_AT_DAMAGE_H
#include "scene_at_units.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2C2490: first enabled kind11 containing region, copies exactly3words
 * from unit5C. Uses canonical native tagged-list identities, never rawtokens. */
gh_scene_at_status gh_scene_at_damage_region_002c2490(gh_scene_at_manager *,const float point[3],uint32_t output[3],int *original_hit,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
