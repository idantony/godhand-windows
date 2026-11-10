#ifndef GH_GAME_EFFECT_EXP_H
#define GH_GAME_EFFECT_EXP_H
#include "game_effect_create.h"
#ifdef __cplusplus
extern "C" {
#endif
/* ActualSDK3A1C78 finite Gaussian domain [-80,0] ->3A28F8.
 * Original binary32 operation order, CVT.W truncation reference, exact ROM
 * constants; no libc exp. Atomic result and callerFEnv preservation. */
gh_effect_status gh_effect_exp_reference_003a1c78(float,float *,gh_static_transform_policy,uint32_t *);
#ifdef __cplusplus
}
#endif
#endif
