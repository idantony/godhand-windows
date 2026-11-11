#ifndef GH_GAME_EVENT_RELEASE_STAGE_H
#define GH_GAME_EVENT_RELEASE_STAGE_H

#include "game_event_actor_release.h"
#include "game_event_timing.h"
#include "game_event_environment.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Remaining owner-dependent calls. The original instruction address is
 * carried separately from the target so a missing provider is diagnosable. */
typedef gh_game_event_status (*gh_event_release_leaf)(void *, uint32_t site,
    uint32_t *frontier, int *relative);

typedef struct {
    gh_game_event *event;
    gh_boot_work *work;
    const gh_game_event_config *config;
    gh_event_actor_release *actors;
    gh_event_timing *timing;
    gh_screen_fade *fade;
    const gh_display_state *display;
    gh_event_release_leaf leaf;
    void *context;
    const uint8_t *background_rgb_604700; /* borrow SAME cWorldLight first3 bytes */
    uint16_t *screen_state_5cae40; /* shared native owner; original stores15 */
} gh_event_release_bindings;

typedef struct {
    const gh_game_event *owner;
    uint8_t initial_step, cleanup_step, in_cleanup, complete;
} gh_event_release_stage;

gh_game_event_status gh_event_release_00297038(gh_event_release_stage *,
    const gh_event_release_bindings *, uint32_t *frontier, int *relative);

#ifdef __cplusplus
}
#endif
#endif
