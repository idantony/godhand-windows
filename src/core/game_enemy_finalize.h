#ifndef GH_GAME_ENEMY_FINALIZE_H
#define GH_GAME_ENEMY_FINALIZE_H
#include "game_enemy_solids.h"
#include "game_enemy_hitareas.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_enemy_hitareas *hitareas;uint32_t stage,frontier,final_random;uint8_t complete;} gh_enemy_finalize;
/* Actual2022D0 final20A suffix:26B9E8 defaultreturn, flags/caches/position,
 * ONEsameMTword andoriginal unsignedDIVU3, then genuineoriginalreturn1.
 * This is initialization only; enemyframe/AI/draw are separate consumers. */
gh_actor_motion_status gh_enemy_finalize_00207d40(gh_enemy_finalize *,gh_enemy_hitareas *,int *original_return);
typedef struct {
 gh_enemy_devil_pose *pose;
 gh_enemy_node_cache nodes;
 gh_enemy_timers timers;
 gh_enemy_behavior behavior;
 gh_enemy_hitareas hitareas;
 gh_enemy_finalize final;
 uint32_t stage,frontier;
} gh_enemy_devil_finish;
/* Original 2022D0 suffix, after the Devil's attached effect. Returns original
 * success only after all native HitArea objects and final scalar writes. */
gh_actor_motion_status gh_enemy_devil_finish_002055b8(gh_enemy_devil_finish *,
 gh_enemy_devil_pose *,const gh_enemy_node_names *,const gh_hitarea_shape_bindings *,int *);
#ifdef __cplusplus
}
#endif
#endif
