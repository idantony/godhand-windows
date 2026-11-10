#ifndef GH_GAME_ACTOR_MOTION_IK_H
#define GH_GAME_ACTOR_MOTION_IK_H
#include "game_actor_motion.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original3BD8A0/3BD8C0 are mutable .data. Canonical owner seeded ONCE,
 * borrowed by consumers thereafter; future original writers share these arrays.
 * Cycle41EE48 is original .rodata, copied as verified scalar metadata. */
typedef struct gh_motion_ik_globals {float xaxis[4],default_axis[4];unsigned cycle[3];uint8_t initialized;} gh_motion_ik_globals;
gh_actor_motion_status gh_motion_ik_globals_from_elf(gh_motion_ik_globals *,const void *,size_t);
/* Actual152448 setup only; sameparts inversebindC0, original15A9A0 basis /
 *15A190 matrix→quaternion and15ADA0 vectorrotation. Originalconstant vectors
 * and cyclicindices are read from bounded ELF PT_LOAD, not guessed pose rules.
 * Complete IK setup is not per-frame IK solve152A20 or animation sampling. */
/* Shared original15A190 kernel; SAME canonical cycle table, atomic output. */
gh_actor_motion_status gh_actor_matrix_quaternion_0015a190(const float matrix[16],float quaternion[4],const gh_motion_ik_globals *,gh_static_transform_policy,uint32_t *frontier);
gh_actor_motion_status gh_actor_motion_ik_00152448(gh_actor_model *,const gh_motion_view *,const gh_motion_ik_globals *globals,gh_static_transform_policy,uint32_t *frontier,unsigned *part);
#ifdef __cplusplus
}
#endif
#endif
