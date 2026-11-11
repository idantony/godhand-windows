#ifndef GH_GAME_PLAYER_SPRINGS_FRAME_H
#define GH_GAME_PLAYER_SPRINGS_FRAME_H
#include "game_player_springs.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual107C68 refreshes the same44 spring records from current model world
 * matrices, preserving record W lanes and all unrelated history/parameters.
 * Native preflight rejects missing typed part overrides before any write.
 * This reset is not the later104010 simulation step. */
gh_player_springs_status gh_player_springs_reset_00107c68(gh_game_player_springs *,gh_static_transform_policy);
gh_player_springs_status gh_spring_records_reset_00107c68(gh_actor_model *,uint8_t *,
 const uint8_t *,unsigned,gh_static_transform_policy);
#ifdef __cplusplus
}
#endif
#endif
