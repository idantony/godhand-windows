#ifndef GH_GAME_EVENT_ACTOR_PLAY_H
#define GH_GAME_EVENT_ACTOR_PLAY_H

#include "game_event_actor_extra.h"
#include "game_actor_motion.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const gh_event_actor_create *creation;
    gh_event_actor_roster *roster;
    const gh_event_actor_extra *extra;
    gh_actor_motion *motions;
    size_t motion_capacity;
    const float *rate;
    const struct gh_motion_ik_globals *ik_globals;
} gh_event_actor_play_bindings;

typedef struct {
    size_t primary_offset, secondary_offset;
    uint32_t frame_bits, secondary_frame_bits;
    uint8_t previous_present, transition;
} gh_event_actor_play_row;

typedef struct {
    gh_event_actor_play_bindings bindings;
    gh_event_actor_play_row rows[255];
    size_t row;
    uint32_t cut, frame_bits;
    uint8_t initialized, active, row_stage, frontier_relative;
} gh_event_actor_play;

gh_game_event_status gh_event_actor_play_init_00001468(gh_event_actor_play *,
    const gh_event_actor_play_bindings *, uint32_t *frontier);
gh_game_event_status gh_event_actor_play_move_000014c8(gh_event_actor_play *,
    const gh_event_actor_play_bindings *, uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
