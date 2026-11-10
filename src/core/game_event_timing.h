#ifndef GH_GAME_EVENT_TIMING_H
#define GH_GAME_EVENT_TIMING_H

#include "game_event_components.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Event.rel +54C0 allocates one original 16-byte row per EventEnv cut. */
typedef struct {
    uint8_t kind, next_cut;
    uint8_t reserved[2];
    uint32_t reserved_word;
    float frames_seen, start_frame;
} gh_event_cut_timing;

typedef struct {
    gh_heap *heap;
    gh_r207_allocation allocation;
    const gh_event_components *components;
    uint32_t count;
    int32_t event_number;
} gh_event_timing;

/* Keep Env+4 as a typed allocation. The original return is zero for an
 * allocation failure, which 2979F0 converts to the Event abort bit. */
gh_game_event_status gh_event_timing_prepare_000054c0(
    gh_event_timing *, gh_event_components *, gh_heap *, int *original_return);
gh_game_event_status gh_event_timing_abort(gh_event_timing *);

#ifdef __cplusplus
}
#endif
#endif
