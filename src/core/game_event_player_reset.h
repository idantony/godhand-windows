#ifndef GH_GAME_EVENT_PLAYER_RESET_H
#define GH_GAME_EVENT_PLAYER_RESET_H
#include "game_event_clear.h"
#include "game_actor_motion_call.h"
#include "game_actor_motion_solve.h"
#include "game_actor_finish_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original player6A8 is a cOmWeapon, NOT Work5F8/PL01 hair. Same canonical
 * weapon600 parent,604 index and610 held offset are represented by typed links.
 * known==1/current==NULL must come from the real constructor/no-weapon state;
 * missing knowledge cannot be inferred from removed raw guestpointer zero. */
typedef struct {gh_title_player_view *weapon;gh_actor_model *weapon_model;
 const gh_title_player_view *parent;gh_actor_model *parent_model;int32_t parent_part;} gh_event_weapon_attachment;
typedef struct {uint8_t known;const gh_event_weapon_attachment *current;} gh_event_weapon_slot;
typedef struct {const gh_event_clear_bindings *prefix;gh_actor_motion *motion;
 gh_actor_motion_bindings motion_bindings;gh_motion_solve_bindings solve;
 gh_game_init_state *init;const float *canonical_rate;const gh_event_weapon_slot *weapon_slot;
 gh_static_transform_policy policy;} gh_event_player_reset_bindings;
typedef struct {gh_event_player_reset_bindings b;gh_event_player_clear prefix;
 gh_actor_motion_call motion_call;gh_motion_solve solve;gh_event_weapon_attachment weapon;
 uint32_t stage,frontier;int detail,ended;uint8_t active,complete,weapon_captured,weapon_present;} gh_event_player_reset;
gh_game_event_status gh_event_weapon_follow_001cf3a8(const gh_event_weapon_attachment *,gh_static_transform_policy,uint32_t *frontier);
/* Explicit NEW original12C418 invocation. Never resets/replaces SAME motion,
 * player/model/IK/global owners; only this call cursor is reset. */
gh_game_event_status gh_event_player_reset_open(gh_event_player_reset *,const gh_event_player_reset_bindings *);
gh_game_event_status gh_event_player_reset_step_0012c418(gh_event_player_reset *);
#ifdef __cplusplus
}
#endif
#endif
