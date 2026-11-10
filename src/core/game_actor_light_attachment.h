#ifndef GH_GAME_ACTOR_LIGHT_ATTACHMENT_H
#define GH_GAME_ACTOR_LIGHT_ATTACHMENT_H
#include "game_actor_model_light.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Pure 14D9A4 attachment calculation. No second model/light/camera owner.
 * Input and output are FOUR components: original30A2B0 multiplies inputW.
 * Camera-relative mode returns the actual147728 matrix for caller to commit
 * into SAME WorldLight16240. No scalar owner is mutated by this helper. */
gh_actor_model_status gh_actor_light_attachment_0014d9a4(const gh_actor_model *,
 const gh_actor_light_bindings *,int camera_relative,const float input[4],float output[4],
 float camera_matrix[16],int *camera_matrix_written,gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
