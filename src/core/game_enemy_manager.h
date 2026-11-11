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
#ifdef __cplusplus
}
#endif
#endif
