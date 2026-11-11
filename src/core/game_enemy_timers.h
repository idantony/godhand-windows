#ifndef GH_GAME_ENEMY_TIMERS_H
#define GH_GAME_ENEMY_TIMERS_H
#include "game_enemy_node_cache.h"
#include "game_enemy_weapon.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_enemy_held_weapon gh_enemy_held_weapon;
typedef struct {gh_enemy_node_cache *nodes;uint32_t stage,cursor,frontier,pending_word,words[3],timer_bits[3];uint8_t pending;gh_enemy_held_weapon *held;} gh_enemy_timers;
/* Actual30F518/31D000 shiftsMTword>>1 BEFORE signedCVT then*2^-31.
 * Timer expression is three orderedMUL5/MUL30/ADD150 referenceoperations. */
gh_actor_motion_status gh_enemy_mt_timer_reference(uint32_t mt_word,gh_em_numeric_policy,uint32_t *unit_bits,uint32_t *timer_bits);
/* 2A2 initialization: actual 31D000 draw, ordered MUL90/ADD90. */
gh_actor_motion_status gh_enemy2a2_mt_timer_reference(uint32_t mt_word,
 gh_em_numeric_policy,uint32_t *timer_bits);
/* Weaponless202/20A prefix205AE4 through3 realcanonicalMTtimer draws;
 * unported itemflag paths stay26AA30. Numericretry keepsalreadydrawnword. */
gh_actor_motion_status gh_enemy_timers_initialize_00205ae4(gh_enemy_timers *,gh_enemy_node_cache *,gh_em_numeric_policy);
gh_actor_motion_status gh_enemy_timers_with_weapon_00205ae4(gh_enemy_timers *,
    gh_enemy_node_cache *, gh_em_numeric_policy, gh_game_scene_tables *, gh_collision_scroll_mutation *,
    const gh_hitarea_shape_bindings *);
/* Typed original enemy +6EC. A partial factory never appears as an attached weapon. */
const gh_enemy_weapon_init *gh_enemy_timers_held_weapon(const gh_enemy_timers *);
gh_enemy_weapon_init *gh_enemy_timers_held_weapon_runtime(gh_enemy_timers *);
/* Drop only a previously transferred and fully retired weapon companion. */
gh_actor_motion_status gh_enemy_timers_finish_held_retirement(gh_enemy_timers *);
gh_actor_motion_status gh_enemy_timers_release_native(gh_enemy_timers *);
#ifdef __cplusplus
}
#endif
#endif
