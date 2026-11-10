#ifndef GH_SDK_SINE_H
#define GH_SDK_SINE_H
#include "game_actor_motion.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual3A19F8: smallkernel or singlepi/2 reduction3A3768 +cos3A3C80.
 * Bounded |x|<=pi/2, sufficient for15A6A0 shortestquaternion interpolation. */
gh_actor_motion_status gh_sdk_sine_003a19f8(float,float *,gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
