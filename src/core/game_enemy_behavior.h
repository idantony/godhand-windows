#ifndef GH_GAME_ENEMY_BEHAVIOR_H
#define GH_GAME_ENEMY_BEHAVIOR_H
#include "game_enemy_timers.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_enemy_timers *timers;uint32_t stage,frontier;} gh_enemy_behavior;
/* Actual20A behaviorbytes +stage-level76C factor; knownmultiplication1*x
 * is exact binary32. Originalroom803..805 special274140 remainsstrict. */
gh_actor_motion_status gh_enemy_behavior_initialize_002064fc(gh_enemy_behavior *,gh_enemy_timers *);
#ifdef __cplusplus
}
#endif
#endif
