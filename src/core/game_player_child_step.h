#ifndef GH_GAME_PLAYER_CHILD_STEP_H
#define GH_GAME_PLAYER_CHILD_STEP_H
#include "game_player_child_tail.h"
#include "game_player_child_attach.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original100530 entry gate only. A skipped originalcall is completely
 * executed and returnsOK/skipped1. An activecall returns preciseUNSUPPORTED
 * at100590/100594 before any originalactor mutation; never falsely completes
 * hierarchy/cloth. Same canonical child view and Work bytes are borrowed. */
gh_title_player_status gh_game_player_child_move_gate_00100530(const gh_title_player_view *,const gh_boot_work *,int *original_skipped,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
