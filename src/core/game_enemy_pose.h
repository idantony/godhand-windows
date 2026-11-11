#ifndef GH_GAME_ENEMY_POSE_H
#define GH_GAME_ENEMY_POSE_H
#include "game_enemy_motion.h"
#include "game_actor_motion_solve.h"
#include "game_actor_finish_model.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_enemy_motion *motion;gh_motion_solve solve;uint32_t stage,frontier;} gh_enemy_pose;
/* Same canonicalactors/ray/IK/coefficientbindings asplayer solver. No empty
 * rayprovider and no separatecollision/model owner. Follows152A20→14A038;
 * shadow14EF50 next remainsstrict. */
gh_actor_motion_status gh_enemy_pose_initialize_00204af8(gh_enemy_pose *,gh_enemy_motion *,const gh_motion_solve_bindings *,gh_static_transform_policy);
#ifdef __cplusplus
}
#endif
#endif
