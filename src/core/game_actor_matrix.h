#ifndef GH_GAME_ACTOR_MATRIX_H
#define GH_GAME_ACTOR_MATRIX_H
#include "../assets/static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_ACTOR_MATRIX_OK,GH_ACTOR_MATRIX_ARGUMENT,GH_ACTOR_MATRIX_UNSUPPORTED} gh_actor_matrix_status;
/* Actual147A48 = T*R*S;147728 SDK premultiply orders0..5, defaultsidentity.
 * Uses original polynomial/SQRT nativebinary32 reference, not libc trig or
 * VU emulation. InputEuler is NOT implicitlywrapped (original147728 isnot).
 * Finite normal boundedEuler[-pi,pi]; completeout/FEnv atomic onfailure. */
/* Actual147548->SDK30A2E0 orderedcolumnmajorproduct, alias-safe/atomic. */
gh_actor_matrix_status gh_actor_matrix_product_00147548(const float a[16],const float b[16],float out[16],gh_static_transform_policy,uint32_t *frontier);
/* Rotation-only original147728, no extraT/S or identity multiplication. */
gh_actor_matrix_status gh_actor_matrix_rotation_00147728(const float *euler,unsigned mode,float out[16],gh_static_transform_policy,uint32_t *frontier);
gh_actor_matrix_status gh_actor_matrix_compose_00147a48(const float *position,const float *euler,const float *scale,unsigned mode,float out[16],gh_static_transform_policy,uint32_t *frontier);
/* Original1F7DA0 XYZ-only wrap, W untouched, finite |angle|<=100 reference. */
gh_actor_matrix_status gh_actor_euler_adjust_001f7da0(const float in[4],float out[4],gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
