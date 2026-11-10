#ifndef GH_GAME_EVENT_PLAY_ENV_H
#define GH_GAME_EVENT_PLAY_ENV_H

#include "game_event_components.h"
#include "game_event_environment.h"
#include "game_init.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Main 296CF4 resolves to Event.rel+5930. Uses the existing EventEnv and
 * shared Fade, then returns to the caller for ExtraPlayInit at 296CFC. */
gh_game_event_status gh_event_env_init_play_00005930(
    gh_event_components *components, const gh_game_event_config *config,
    gh_boot_work *work, gh_screen_fade *fade, const gh_display_state *display,
    uint32_t *frontier);

typedef struct {
    const gh_game_event *event;
    const gh_event_components *components;
    int32_t event_number;
    uint8_t next_initialization, next_move, quit_pending, frontier_relative;
} gh_event_play_state;

typedef gh_game_event_status (*gh_event_play_visit)(void *, uint32_t original_site,
    int *original_end, uint32_t *frontier, int *relative);

typedef struct {
    gh_game_event *event;
    gh_event_components *components;
    const gh_game_event_config *config;
    gh_boot_work *work;
    gh_screen_fade *fade;
    const gh_display_state *display;
    gh_event_play_visit visit;
    void *context;
} gh_event_play_bindings;

/* Original Init and Move visit order. Retain the next call across a failed
 * component visit so Fade, frame publication and producers run only once. */
gh_game_event_status gh_event_play_00296c28(gh_event_play_state *,
    const gh_event_play_bindings *, int *original_done, uint32_t *frontier);

/* The EV01 typed callback selected by 298060 -> 297B40 -> REL+6A80.
 * The actual call site in REL+6B08 supplies the current cut and actor. */
gh_game_event_status gh_event_env_extra_move_object_00006b08(
    gh_event_components *, gh_title_player_view *, gh_core_save_view, uint32_t *frontier);

/* Bind the absent EventFade track via 2979A0/297F80, then run the empty
 * mapping/active reset at REL+6C70. A nonzero track remains at 297F80. */
gh_game_event_status gh_event_fade_init_absent_00006c70(
    gh_event_components *components, const void *resource, size_t resource_bytes,
    uint32_t *frontier);

/* REL+6CA0 for the proven absent track. It calls +6FF8, which kills only
 * the fade previously activated by this component. */
gh_game_event_status gh_event_fade_move_absent_00006ca0(
    gh_event_components *components, gh_screen_fade *fade, uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
