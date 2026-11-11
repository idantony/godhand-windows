#ifndef GH_GAME_PLAYER_IDLE_INPUT_H
#define GH_GAME_PLAYER_IDLE_INPUT_H
#include "game_player_frame.h"
#include "game_player_targets.h"
#include "game_action_buttons.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual123938(player,0), not the param2!=0 animation-gated caller.
 * The state owns canonical typed69C/6A4 values across calls; use begin, never
 * memset a live state to start the next invocation. Bindings address must remain stable through an invocation. Original return is published only on a genuinely completed branch. */
typedef struct gh_player_idle_input_bindings {gh_player_frame_bindings frame;gh_player_target_bindings targets;gh_game_action_buttons *action_buttons;} gh_player_idle_input_bindings;
typedef struct gh_player_idle_input {gh_player_idle_input_bindings b;const gh_player_idle_input_bindings *source_bindings;uint32_t phase,frontier;uint8_t bound;gh_player_frame_effect_request effect;gh_player_target pickup,interaction;} gh_player_idle_input;
/* Native invocation boundary only, preserves typed targetfield values.
 * Caller still executes original12BFB8 if original_return==0. */
gh_player_frame_status gh_player_idle_input_begin(gh_player_idle_input *);
gh_player_frame_status gh_game_player_idle_cleanup_0012bfb8(gh_player_idle_input *);
gh_player_frame_status gh_game_player_idle_input_00123938(gh_player_idle_input *,const gh_player_idle_input_bindings *,int *original_return);
#ifdef __cplusplus
}
#endif
#endif
