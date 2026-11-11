#ifndef GH_GAME_ENEMY_DAMAGE_H
#define GH_GAME_ENEMY_DAMAGE_H
#include "game_enemy_actor.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Original 0202 VT+AC arithmetic for the effect-7 player hit. Call at
 * 20A5DC with the signed power already selected by 20A178. Reads the
 * canonical Take ledger source and GameInit Heat, then updates current HP. */
gh_actor_motion_status gh_enemy202_damage_0028fb88(
    gh_enemy_actor *, const gh_title_player_view *current_player, int32_t selected_power,
    uint32_t *frontier);
typedef struct {
    float position[4], scale;
    uint32_t bank, index;
    int32_t mode;
} gh_enemy202_impact_request;
typedef struct {
    gh_enemy202_impact_request impact;

    uint8_t phase, reacted;
} gh_enemy202_hit_call;
/* Ordered positive first effect-7 response at 20A178. The caller keeps
 * this cursor across strict stops and only resets it for a new Move. */
gh_actor_motion_status gh_enemy202_hit_0020a178(
    gh_enemy202_hit_call *, gh_enemy_actor *, const gh_title_player_view *current_player,
    unsigned save_rank, uint32_t *frontier);

/* 208BE0 result-1 continuation for first effect 7. Thresholds are the
 * original CoreSave rank words at 3BF160. */
gh_actor_motion_status gh_enemy202_hit_rank_001fa280(
    gh_enemy_actor *, const uint8_t *thresholds, uint32_t *frontier);
gh_actor_motion_status gh_enemy202_hit_stun_0028ed28(gh_enemy_actor *, uint32_t *frontier);
/* Position and stage the original unattached 306838 request. The effect
 * consumer owns allocation. A pending request keeps the three MT draws. */
gh_actor_motion_status gh_enemy202_impact_00271810(gh_enemy202_hit_call *,
    gh_enemy_actor *, const gh_title_player_view *, uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
