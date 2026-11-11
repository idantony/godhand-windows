#ifndef GH_GAME_ENEMY_MANAGER_H
#define GH_GAME_ENEMY_MANAGER_H
#include "game_init.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Samecanonical64 slots; native sidecars replace only guest list pointers.
 * This is EntryEm's registration prefix BEFORE virtual9C(2022D0). */
gh_init_status gh_enemy_manager_find_free_00290958(const gh_game_init_state *,int *slot);
gh_init_status gh_enemy_manager_insert_00290048(gh_game_init_state *,unsigned slot,void *actor,int32_t kind,uint32_t *frontier);
/* 290250's live-list rate writes and the following 291D48 actor-kind scan.
 * Preflight the native list before writing actor +5A8/+444. */
gh_init_status gh_enemy_manager_step_rates_00290250(gh_game_init_state *,float world_rate,
 int *has_special_music_actor);
#ifdef __cplusplus
}
#endif
#endif
