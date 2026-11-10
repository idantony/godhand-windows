#ifndef GH_GAME_PLAYER_RUN_H
#define GH_GAME_PLAYER_RUN_H
#include "game_player_frame.h"
#include "game_player_run_state.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual PMFstate0/substate2 target10C5A8. Original68/6C or6B4/6B8 motion,
 * canonicalHeat/typedtuning, sampledrootdelta; no nativecontroller velocity.
 * Positiveaux and destinationmode4 remain strict at originalcallees. */
gh_player_frame_status gh_game_player_run_0010c5a8(gh_player_run_call *,const gh_player_frame_bindings *);
gh_player_frame_status gh_player_run_begin(gh_player_run_call *);
#ifdef __cplusplus
}
#endif
#endif
