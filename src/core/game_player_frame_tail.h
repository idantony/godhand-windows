#ifndef GH_GAME_PLAYER_FRAME_TAIL_H
#define GH_GAME_PLAYER_FRAME_TAIL_H
#include "game_init.h"
#include "game_title_player.h"
#include "game_effect_release.h"
#include "../assets/static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PLAYER_TAIL_OK,GH_PLAYER_TAIL_ARGUMENT,GH_PLAYER_TAIL_BUSY,GH_PLAYER_TAIL_NUMERIC,GH_PLAYER_TAIL_UNSUPPORTED,GH_PLAYER_TAIL_CALLEE} gh_player_frame_tail_status;
/* SAME native target644 publisher. expected_guest_token is a diagnostic token,
 * never a host pointer. knownNULL requires explicit proven publication. */
typedef struct {const gh_title_player_view *view;uint32_t expected_guest_token;uint8_t known;} gh_player_tail_target_slot;
typedef struct {
 gh_game_title_player *player;const gh_boot_work *work;gh_game_init_state *init;
 gh_core_save_view save;uint8_t save_binding_known;const float *previous_world_position;
 gh_player_tail_target_slot *target_644;
 const gh_title_player_view *aux_698,*aux_6a0;uint8_t auxiliary_bindings_known;
 uint8_t *cockpit_flags_1ca0;const gh_effect_release_bindings *effects;
 gh_static_transform_policy policy;
} gh_player_frame_tail_bindings;
/* Continuation metadata only. Original mutable state stays in borrowed owners. */
typedef struct {gh_player_frame_tail_bindings b;uint32_t stage,frontier;uint8_t bound,complete,target_phase,effect_bank;unsigned effect_index;int32_t target_kind;const gh_title_player_view *target_view;uint32_t target_token;float previous[3],heat_maximum;uint8_t heat_level,heat_started;} gh_game_player_frame_tail;
/* One original invocation; refuses to erase a pending suffix. */
gh_player_frame_tail_status gh_player_frame_tail_begin(gh_game_player_frame_tail *);
/* Actual108AB0 suffix at1097E0, immediately after successful123160.
 * Repeated call after success is idempotent until explicit begin. */
gh_player_frame_tail_status gh_game_player_frame_tail_001097e0(gh_game_player_frame_tail *,const gh_player_frame_tail_bindings *);
#ifdef __cplusplus
}
#endif
#endif
