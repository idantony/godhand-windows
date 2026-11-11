#ifndef GH_GAME_PLAYER_IDLE_TARGET_H
#define GH_GAME_PLAYER_IDLE_TARGET_H
#include "game_player_targets.h"
#include "game_action_buttons.h"
#include "game_init.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original 123BF0..123CDC after mode0 search. When transitioned is true,
 * caller takes original return1 and target_640 retains the typed target.
 * Otherwise caller resumes at 123CDC and reads player+622 again. */
typedef struct {
    gh_player_target target_640;
    int transitioned, prompt_emitted;
} gh_player_idle_target_result;

gh_player_target_status gh_player_idle_target_00123bf0(
    const gh_title_player_view *player,const gh_player_target *nearby,
    const gh_game_init_state *init,gh_game_action_buttons *buttons,
    int aux_6a0_present,int aux_698_present,
    gh_player_idle_target_result *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
