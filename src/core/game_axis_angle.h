#ifndef GH_GAME_AXIS_ANGLE_H
#define GH_GAME_AXIS_ANGLE_H
#include "game_actor_matrix.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual15AF30: alignaxis using originalatan/SDK rotations, transpose, rotate
 * aboutX, multiply, then pivot-R*pivot. No generic Rodrigues substitution.
 * pivot is borrowed bytes (mutable3BD880 owner at callsites), memcpy-read. */
gh_actor_matrix_status gh_axis_angle_matrix_0015af30(float angle,const float axis[3],const void *pivot16bytes,float out[16],gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
