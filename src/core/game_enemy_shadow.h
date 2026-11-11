#ifndef GH_GAME_ENEMY_SHADOW_H
#define GH_GAME_ENEMY_SHADOW_H
#include "game_enemy_pose.h"
#include "game_player_shadow.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_enemy_pose *pose;gh_player_shadow shadow;uint32_t stage,frontier;} gh_enemy_shadow;
/* Actual2022D0 ordinary20A branch14EF50(self,self780,3C3F68).
 * Packetconstruction only; neither shadow rendering nor finalenemyInit. */
gh_actor_motion_status gh_enemy_shadow_initialize_00204b10(gh_enemy_shadow *,gh_enemy_pose *);
#ifdef __cplusplus
}
#endif
#endif
