#ifndef GH_GAME_ENEMY_HP_H
#define GH_GAME_ENEMY_HP_H
#include "game_enemy_entry_tail.h"
#include "game_scene_placements.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_enemy_entry_tail *tail;uint32_t stage,frontier,hp;uint8_t equipped_count;} gh_enemy_hp;
/* EM202/20A only. Exact integer branches; easyMUL/CVT requires explicit
 * PCSX2 defaultChopZero reference, not verifiedPS2hardware semantics. */
gh_actor_model_status gh_enemy_hp_initialize_00202b88(gh_enemy_hp *,gh_enemy_entry_tail *,gh_em_numeric_policy);
#ifdef __cplusplus
}
#endif
#endif
