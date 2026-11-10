#ifndef GH_SDK_ROTATION_TRIG_H
#define GH_SDK_ROTATION_TRIG_H
#include "game_actor_matrix.h"
#ifdef __cplusplus
extern "C" {
#endif
/* ActualSDK30A5D0 +30A650/700/7B8/870 preparation, independently audited
 * fromID1F7EF0. Explicit nearestbinary32 arithmetic; VUSQRT magnitude uses
 * pinnedPCSX2reference. Negative radicand is ABS, neverzero-clamped.
 * No VUstatusflags, denormals, overflow orhardwarebitidentity claimed. */
gh_actor_matrix_status gh_sdk_rotation_sincos_reference_0030a5d0(float theta,float *sine,float *cosine,gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
