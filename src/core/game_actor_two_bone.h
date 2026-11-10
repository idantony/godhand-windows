#ifndef GH_GAME_ACTOR_TWO_BONE_H
#define GH_GAME_ACTOR_TWO_BONE_H
#include "game_actor_motion_ik.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual153158 reference. Borrows SAME canonical3BD8A0 vector and actual
 *42C350 polynomial coefficients (camera_envelope.atan_coeff), neverreseeded.
 *3A19F8 sine fastpath |halfangle|<=pi/4; largerangles retain that frontier.
 * Writes SAME two parts/worldposes only after complete numeric validation. */
/* Actual147C88 mode0 and150138 local rebuild after solvedworld→local.
 * Same original3A2558 rational asinf and1F8180 coefficientborrow. */
gh_actor_motion_status gh_actor_part_extract_rebuild_00147c88(gh_actor_model *,unsigned,const float atan_coeff[8],gh_static_transform_policy,uint32_t *frontier);
gh_actor_motion_status gh_actor_part_rebuild_00150138(gh_actor_model *,unsigned,gh_static_transform_policy,uint32_t *frontier);
gh_actor_motion_status gh_actor_two_bone_00153158(gh_actor_model *,unsigned first_part,const gh_boot_work *,const gh_motion_ik_globals *,const float atan_coeff[8],gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
