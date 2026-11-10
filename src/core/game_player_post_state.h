#ifndef GH_GAME_PLAYER_POST_STATE_H
#define GH_GAME_PLAYER_POST_STATE_H
#include "game_player_node_cache.h"
#include "game_effect_release.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PLAYER_POST_OK,GH_PLAYER_POST_ARGUMENT,GH_PLAYER_POST_UNSUPPORTED,GH_PLAYER_POST_CALLEE} gh_player_post_status;
/* Continuation of one original invocation, not another player/model owner. */
typedef struct {const uint8_t *player_bytes;uint32_t frontier;uint8_t effect_phase,effect_action,visibility_phase;} gh_game_player_post_state;
gh_player_post_status gh_player_state_effect_0012bb38(gh_game_player_post_state *,const gh_title_player_view *,const gh_effect_release_bindings *);
/* Actual cached mesh references replace raw1550/1554..1570 pointer tokens.
 * The149AD8 positive mapping branch remains precise until its real consumer.
 * NULL references are the cache's known current values, not raw0 guesses. */
gh_player_post_status gh_player_mesh_visibility_001236e8(gh_game_player_post_state *,const gh_game_player_node_cache *,gh_core_save_view);
/* Exact138F88 AND result, used by123908 whose caller discards the value. */
gh_player_post_status gh_player_camera_mode_00123908(const gh_game_init_state *,uint32_t *original_mask);
#ifdef __cplusplus
}
#endif
#endif
