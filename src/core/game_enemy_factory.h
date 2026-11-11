#ifndef GH_GAME_ENEMY_FACTORY_H
#define GH_GAME_ENEMY_FACTORY_H
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GH_EM202_BYTES 0x1980u
/* Constructor-only view; no registration, virtualInit or enemy readiness. */
gh_title_player_status gh_enemy_base_construct_0028eb00(void *,size_t,gh_title_player_view *);
gh_title_player_status gh_enemy202_construct_00201fa0(void *,size_t,gh_title_player_view *);
/* Original3130E0 allocation gate/zero/constructor. Caller owns same754220
 * heap; resultNULL is separate from native allocation errors. No shadow list. */
gh_title_player_status gh_enemy202_allocate_003130e0(gh_game_title_player *,gh_heap *,const gh_boot_work *,int *original_nonnull);
#ifdef __cplusplus
}
#endif
#endif
