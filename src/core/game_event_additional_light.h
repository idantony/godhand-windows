#ifndef GH_GAME_EVENT_ADDITIONAL_LIGHT_H
#define GH_GAME_EVENT_ADDITIONAL_LIGHT_H

#include "game_event_data_prefix.h"
#include "game_event_link.h"
#include "game_world_light.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gh_event_link *link;
    const gh_event_data_prefix *data;
    gh_game_world_light *world_light;
    const void *allocation;
    uint16_t cut_start[11];
} gh_event_additional_light;

/* EV01's camera-global light track, with the same live WorldLight pool
 * consumed by actor lighting. Zero this state on a new event invocation. */
gh_game_event_status gh_event_additional_light_init_000040d8(gh_event_additional_light *,
    gh_event_link *, const gh_event_data_prefix *, gh_game_world_light *, uint32_t *frontier);
gh_game_event_status gh_event_additional_light_move_00004140(gh_event_additional_light *,
    const gh_boot_work *, uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
