#ifndef GH_GAME_EVENT_ENV_FRAME_H
#define GH_GAME_EVENT_ENV_FRAME_H

#include "game_event_timing.h"
#include "game_event_data_prefix.h"
#include "game_event_environment.h"
#include "game_event_player_reset.h"
#include "game_player_child.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gh_game_title_player *work5f4;
    gh_game_player_child *const *work5f8;
    const gh_event_weapon_slot *weapon_6a8;
} gh_event_env_end_owners;

typedef struct {
    gh_event_timing *timing;
    gh_event_components *components;
    const gh_event_data_prefix *data;
    const gh_game_event_config *config;
    const gh_boot_work *work;
    gh_screen_fade *fade;
    const gh_display_state *display;
    const gh_event_env_end_owners *end_owners;
} gh_event_env_frame_bindings;

/* Publish the already scheduled current cut/frame before the ordered PLAY
 * consumers. The timing rows and EventEnv both belong to the loaded event. */
gh_game_event_status gh_event_env_update_frame_000059e0(gh_event_timing *,
    gh_event_components *,uint32_t *frontier);

/* After the PLAY consumers, schedule the next frame. Kind 0 uses actual
 * camera track lengths. A positive original_end means 64C8 finished its
 * object flag cleanup; callers still own the subsequent Event release. */
gh_game_event_status gh_event_env_calc_next_00005b78(const gh_event_env_frame_bindings *,
    int *original_end,uint32_t *frontier,int *relative);

/* Positive 64C8 body after fade readiness. All three bindings identify the
 * canonical Work or player slots; known NULL jacket/weapon is allowed. */
gh_game_event_status gh_event_env_finish_000064c8(const gh_event_env_end_owners *,
    uint32_t *frontier,int *relative);

#ifdef __cplusplus
}
#endif
#endif
