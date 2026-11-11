#ifndef GH_GAME_EVENT_QUIT_H
#define GH_GAME_EVENT_QUIT_H

#include "game_event_play_env.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gh_game_event *event;
    gh_event_components *components;
    gh_boot_work *work;
    gh_event_play_visit visit;
    void *context;
} gh_event_quit_bindings;

typedef struct {
    gh_game_event *event;
    gh_event_components *components;
    gh_boot_work *work;
    int32_t event_number;
    uint8_t next, started, completed;
} gh_event_quit;

/* Original 296EF0. Zero-initialize per invocation. Failed component callbacks
 * resume at their original call site. The PLAY caller still owns its transition
 * to state 4 after this operation succeeds. */
gh_game_event_status gh_event_quit_00296ef0(gh_event_quit *,
    const gh_event_quit_bindings *, uint32_t *frontier, int *relative);

#ifdef __cplusplus
}
#endif
#endif
