#ifndef GH_GAME_ENEMY_MOTION_H
#define GH_GAME_ENEMY_MOTION_H
#include "game_enemy_entry_tail.h"
#include "game_enemy_hp.h"
#include "game_actor_motion.h"
#include "game_actor_motion_step.h"
#include "game_collision_solid_units.h"
#ifdef __cplusplus
extern "C" {
#endif
void gh_enemy_start_action_00272570(uint8_t *actor);
typedef struct {gh_enemy_hp *hp;gh_actor_motion motion;uint32_t stage,frontier;size_t motion_offset,sequence_offset;int initial_ended;} gh_enemy_motion;
/* Actual2022D0 EM202/20A appearance0/default andactualR100appearance3
 * data selection, scalarwrites andshared2A8578. No animation sampleack. */
gh_actor_motion_status gh_enemy_motion_initialize_002036b0(gh_enemy_motion *,gh_enemy_hp *,const struct gh_motion_ik_globals *,gh_motion_effect_consumer,void *);
/* Actualinitial157C18 then conditionalvirtualDC/hierarchy149EB8. Required
 * canonicalrate explicit; next152A20 needs canonical ray/solverbindings. */
gh_actor_motion_status gh_enemy_motion_initial_step_00204abc(gh_enemy_motion *,float canonical_rate,gh_static_transform_policy,int *original_ended);
typedef struct {
 gh_game_cockpit *cockpit;
 gh_game_collision_solids *solids;
 const struct gh_motion_ik_globals *ik;
 gh_motion_sound_consumer sound;
 void *sound_context;
 float rate;
} gh_enemy_devil_bindings;
typedef struct {
 gh_enemy_devil_entry *owner;
 gh_enemy_devil_bindings bindings;
 gh_enemy_entry_tail tail;
 gh_enemy_hp hp;
 gh_enemy_motion motion;
 gh_motion_sound_request cue;
 uint32_t stage,frontier;
} gh_enemy_devil_initialize;
/* Continues the published Devil factory through the first model pose.
 * A successful prefix remains paused at the original 152A20 solver. */
gh_actor_motion_status gh_enemy_devil_initialize_0028ebf0(gh_enemy_devil_initialize *,
 gh_enemy_devil_entry *,const gh_enemy_devil_bindings *);
#ifdef __cplusplus
}
#endif
#endif
