#ifndef GH_GAME_ACTOR_MOTION_SOLVE_H
#define GH_GAME_ACTOR_MOTION_SOLVE_H
#include "game_actor_motion.h"
#include "game_ray_query.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_scene_ray_status (*gh_motion_solve_ray)(void *,gh_game_scene_tables *,const gh_scene_ray_request *,int *,uint32_t *frontier);
typedef struct {gh_actor_model *model;gh_boot_work *work;gh_game_scene_tables *actors;gh_motion_solve_ray ray;void *context;const struct gh_motion_ik_globals *ik_globals;const float *atan_coeff;} gh_motion_solve_bindings;
typedef struct {gh_actor_model *model;gh_boot_work *work;gh_game_scene_tables *actors;uint32_t index,stage,frontier;float target[4],hit[4];gh_scene_ray_request request;int original_hit;} gh_motion_solve;
/* Actual152A20 ordered positivechain prefix, preserving consumed400000 bit,
 * transformedtarget, realsegmentedmask2 ray, height smoothing and retries.
 * Positive root4000 branch executes153808 target/localmatrix +ordinary
 *1501D0 childworld,153158 two-bone solve and147C88/150138 local recovery.
 * Requiredglobals/atan_coeff missing keeps153158; complexmath keepsstrictfrontier. NULLray uses SAME canonical scene_ray_query (neverdefaultmiss).
 * Caller keeps state and all borrowed owners alive across strictfrontier. */
gh_actor_motion_status gh_actor_motion_solve_00152a20(gh_motion_solve *,const gh_motion_solve_bindings *,gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
