#ifndef GH_GAME_ENEMY_ENTRY_TAIL_H
#define GH_GAME_ENEMY_ENTRY_TAIL_H
#include "game_enemy_entry.h"
#include "game_enemy_base_init.h"
#include "game_player_springs.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 gh_enemy_entry *entry;gh_enemy_base_initialize base;uint32_t stage,frontier,original_counter;
 gh_r207_allocation spring_records;gh_event_spring_profile spring_profile;uint8_t springs_ready;
} gh_enemy_entry_tail;
/* Resume actual2022D0 after208268 complete. Same canonicalCockpit/manager/MT.
 * CurrentEM202/20A continues through271410 default to202B88 HPselector. */
gh_actor_model_status gh_enemy_entry_continue_002022d0(gh_enemy_entry_tail *,gh_enemy_entry *,gh_game_cockpit *);
gh_actor_model_status gh_enemy_devil_continue_002022d0(gh_enemy_entry_tail *,gh_enemy_devil_entry *,gh_game_cockpit *);
gh_actor_model_status gh_enemy_entry_tail_abort(gh_enemy_entry_tail *);
#ifdef __cplusplus
}
#endif
#endif
