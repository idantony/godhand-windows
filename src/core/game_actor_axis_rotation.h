#ifndef GH_GAME_ACTOR_AXIS_ROTATION_H
#define GH_GAME_ACTOR_AXIS_ROTATION_H
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original 1501D0 bit800 postmultiply sequence actually left-multiplies
 * world by the fixed global Y-axis quaternion matrix, then restores XYZ. */
gh_actor_model_status gh_actor_part_axis_world_001501d0(float world[16],float radians,
    gh_static_transform_policy,uint32_t *frontier);
/* World update for one canonical part, including the original
 * root40000/no-blend inherited scale path and optional fixed-axis rotation. */
gh_actor_model_status gh_actor_part_world_001501d0(gh_actor_part *,const uint8_t *parent,
    uint32_t root_flags,gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
