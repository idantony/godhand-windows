#ifndef GH_GAME_ACTOR_LIGHT_NORMALIZE_H
#define GH_GAME_ACTOR_LIGHT_NORMALIZE_H
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Pure 30A370 VU xyz normalization, W=0. Explicit binary32 reference profile,
 * including finite signed zero input (DIV1/0 saturates to positive FLT_MAX).
 * Subnormal/overflow arithmetic remains strict; output is atomic on failure. */
gh_actor_model_status gh_actor_light_normalize_0030a370(const float input[3],float output[4],
 gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
