#ifndef GH_GAME_PLAYER_WALK_STATE_H
#define GH_GAME_PLAYER_WALK_STATE_H
#include "game_player_init_tail.h"
/* Host continuation for ONE original10C300 call, not another player/motion.
 * Parameters are retained while a real callee is suspended or unsupported. */
typedef struct gh_player_walk_call {
 const void *source_bindings;
 const void *init_owner,*current_owner,*input_owner,*input_bindings_owner,*tuning_owner;
 gh_player_motion_request motion;
 uint32_t phase,frontier;
 int motion_ended,input_result;
 uint8_t bound,complete,input_started;
} gh_player_walk_call;
#endif
