#ifndef GH_GAME_ACTOR_INVERSE_H
#define GH_GAME_ACTOR_INVERSE_H
#include "game_actor_matrix.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual147568 affine inverse. Originaldet==0 returnsidentity; VU dot
 * order is (x+z)+y. Native binary32 reference; alias-safe,atomic,FEnv preserved. */
gh_actor_matrix_status gh_actor_matrix_inverse_00147568(const float input[16],float output[16],gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
