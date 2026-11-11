#ifndef GH_GAME_PLAYER_SQUARE_INPUT_H
#define GH_GAME_PLAYER_SQUARE_INPUT_H
#include "game_player_frame.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original 123FC8..124094 plus shared 1244CC/1244D8 state writes.
 * Caller reaches this leaf with the Square buffer present and higher
 * priority inputs already processed. caller_mode is original 123938 A1. */
gh_player_frame_status gh_player_square_input_00123fc8(
    const gh_title_player_view *,gh_core_save_view,const gh_game_init_state *,
    int caller_mode,int *original_return,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
