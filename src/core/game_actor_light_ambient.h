#ifndef GH_GAME_ACTOR_LIGHT_AMBIENT_H
#define GH_GAME_ACTOR_LIGHT_AMBIENT_H
#include "game_actor_model.h"
#include "game_world_light.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2D8008 leaves position entirely unread. Only normalMatrix[15]=1 and
 * colorMatrix[12..15]=WorldLight30*tint are written. Other words, including
 * arbitrary retained packet values, are neither recomputed nor normalized.
 * Finite binary32 reference subset; failure commits no output bytes. */
gh_actor_model_status gh_actor_ambient_fast_002d8008(const gh_game_world_light *,
 void *normal_matrix64,void *color_matrix64,const void *unused_position,const float tint[4],
 gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
