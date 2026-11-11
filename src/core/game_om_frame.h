#ifndef GH_GAME_OM_FRAME_H
#define GH_GAME_OM_FRAME_H
#include "game_om_initialize.h"
#include "game_object_step.h"
#include "game_object_opacity.h"
#include "game_om_damage.h"
#include "game_collision_solid_step.h"
#include "game_actor_motion.h"
#ifdef __cplusplus
extern "C" {
#endif
/* SAME factory actor companion, no second registry/model. Persistent634
 * replaces original door's actor pointer; reset only the invocation cursor
 * between completed calls, not this borrowed identity. */
typedef gh_object_step_status (*gh_om_break_effect)(void *, const gh_actor_model *,
 const uint32_t world_xyz[3], uint32_t yaw_bits, uint32_t *frontier);
typedef gh_object_step_status (*gh_om_break_collision)(void *, uint64_t key, uint32_t *frontier);
typedef gh_object_step_status (*gh_om_remove_sound)(void *, gh_actor_model *, uint32_t *frontier);
typedef struct {
 gh_game_init_state *init;
 const float *canonical_rate;
 gh_game_collision_solids *solids;
 void *context;
 gh_om_break_effect break_effect;
 gh_motion_sound_consumer break_sound;
 gh_om_break_collision break_collision;
 gh_om_remove_sound remove_sound;
 const struct gh_motion_ik_globals *ik_globals;
} gh_om_frame_runtime;
typedef struct {const gh_om_factory *factory;gh_om_object *object;
 const gh_title_player_view *contact634;uint32_t handle,entry,frontier;
 unsigned node_index;uint8_t phase,complete,bound;
 gh_object_opacity_environment environment;uint8_t environment_bound,opacity_complete;
 gh_om_damage_call damage_calls[2];unsigned hit_index;
 gh_om_frame_runtime runtime;gh_collision_solid_frame solid_frame;float runtime_rate;
 gh_motion_sound_request break_sound;
 uint8_t runtime_bound,special_phase;} gh_om_frame_state;
gh_object_step_status gh_om_frame_move(gh_om_frame_state *,const gh_om_factory *,const gh_object_move_request *,const gh_boot_work *,gh_static_transform_policy,uint32_t *frontier);
/* Borrow same canonical init/PlCamera/Work5F4 publisher for original2A87E8. */
gh_object_step_status gh_om_frame_bind_environment(gh_om_frame_state *,const gh_object_opacity_environment *);
gh_object_step_status gh_om_frame_bind_runtime(gh_om_frame_state *,const gh_om_frame_runtime *);
gh_object_step_status gh_om_frame_next_call(gh_om_frame_state *);
/* Read-only checked borrow of the SAME initializer model. Never reconstruct
 * raw25C pointers or copy another actor/model. Output unchanged on failure. */
gh_object_step_status gh_om_frame_model_at(const gh_om_frame_state *,gh_actor_model **out);
/* Borrow without requiring or executing original Move. */
gh_object_step_status gh_om_factory_model_at(const gh_om_factory *,uint32_t handle,gh_actor_model **out);
/* Actual1BD430 contact consumer. Call once per original occurrence. */
gh_object_step_status gh_om_frame_contacts_001bd430(gh_om_frame_state *,uint32_t *frontier);
/* Original156488 bit conversion (exponent31 maps to finitefloat, not IEEE
 * half Inf/NaN). Original149220 uses this for MD+3E/+40 UV velocities. */
uint32_t gh_om_half_bits_00156488(uint16_t);
#ifdef __cplusplus
}
#endif
#endif
