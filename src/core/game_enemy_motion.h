#ifndef GH_GAME_ENEMY_MOTION_H
#define GH_GAME_ENEMY_MOTION_H
#include "game_enemy_hp.h"
#include "game_actor_motion.h"
#include "game_actor_motion_step.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_enemy_hp *hp;gh_actor_motion motion;uint32_t stage,frontier;size_t motion_offset,sequence_offset;int initial_ended;} gh_enemy_motion;
/* Actual2022D0 EM202/20A appearance0/default andactualR100appearance3
 * data selection, scalarwrites andshared2A8578. No animation sampleack. */
gh_actor_motion_status gh_enemy_motion_initialize_002036b0(gh_enemy_motion *,gh_enemy_hp *,const struct gh_motion_ik_globals *,gh_motion_effect_consumer,void *);
/* Actualinitial157C18 then conditionalvirtualDC/hierarchy149EB8. Required
 * canonicalrate explicit; next152A20 needs canonical ray/solverbindings. */
gh_actor_motion_status gh_enemy_motion_initial_step_00204abc(gh_enemy_motion *,float canonical_rate,gh_static_transform_policy,int *original_ended);
#ifdef __cplusplus
}
#endif
#endif
