#ifndef GH_GAME_PLAYER_WALK_H
#define GH_GAME_PLAYER_WALK_H
#include "game_player_frame.h"
#include "game_player_walk_state.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual state0/substate1 PMF target10C300. Normal5D8!=4 route consumes
 * original resource58/5C, blend10 and sampled motion rootdelta; no host speed.
 * Positive auxiliary6A0 and special5D8==4 destination-turn branch remain
 * strict at their real callees. Bindings must stay stable through the call. */
gh_player_frame_status gh_game_player_walk_0010c300(gh_player_walk_call *,const gh_player_frame_bindings *);
/* Starts a NEW original invocation only after the last one completed. */
gh_player_frame_status gh_player_walk_begin(gh_player_walk_call *);
#ifdef __cplusplus
}
#endif
#endif
