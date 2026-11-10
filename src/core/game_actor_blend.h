#ifndef GH_GAME_ACTOR_BLEND_H
#define GH_GAME_ACTOR_BLEND_H
#include "game_actor_motion_ik.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original156D70: scale/translation linear, rotation shortest15A6A0.
 * amount0 choosescurrent,1 choosescapturedOld. Both inputs arelocalmatrices. */
gh_actor_motion_status gh_actor_matrix_blend_00156d70(float amount,const float current[16],const float captured_old[16],float output[16],const gh_motion_ik_globals *,gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
