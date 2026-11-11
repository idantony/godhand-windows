#ifndef GH_GAME_PLAYER_ATTACK_RECENT_H
#define GH_GAME_PLAYER_ATTACK_RECENT_H
#include "game_hitarea_frame.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original 10B2E8, argument0. Scan the five player give hit areas in
 * their original +5B0/+5B8/+5C0/+5C8/+5D0 order. */
gh_hitarea_status gh_player_attack_recent_0010b2e8(const gh_title_player_view *,
 const gh_hitarea_object give[5],int *original_hit,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
