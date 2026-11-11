#ifndef GH_GAME_PLAYER_SQUARE_ATTACK_H
#define GH_GAME_PLAYER_SQUARE_ATTACK_H
#include "game_player_frame.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 const gh_player_frame_bindings *bindings;
 gh_player_motion_request motion;
 uint32_t frontier;
 uint8_t stage,skill;
} gh_player_square_attack;
/* Original ComboObj_startNext, bounded to ordinary skill0 and neutral Triangle skill29.
 * Caller already completed the attack target/distance prefix of 10D3A8.
 * On success the same motion owner is ready to advance. */
gh_player_frame_status gh_player_square_attack_begin_00126918(
 gh_player_square_attack *,const gh_player_frame_bindings *);
#ifdef __cplusplus
}
#endif
#endif
