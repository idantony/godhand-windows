#ifndef GH_GAME_EVENT_LIGHT_FILTER_H
#define GH_GAME_EVENT_LIGHT_FILTER_H
#include "game_event_data_prefix.h"
#include "game_scene_filters.h"
#include "game_world_light.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const gh_event_data_prefix *data;
    const void *allocation;
    gh_event_components *components;
    gh_game_world_light *world_light;
    gh_scene_filters *filters;
} gh_event_light_filter;

/* Canonical component scratch holds the original origin keys. The EV01 mapping
 * is validated without rewriting serialized SEQ bytes. Frontiers are REL offsets. */
gh_game_event_status gh_event_object_light_init_00008648(gh_event_light_filter *,
    gh_event_components *, const gh_event_data_prefix *, gh_game_world_light *, uint32_t *);
gh_game_event_status gh_event_filter_init_00007040(gh_event_light_filter *,
    gh_event_components *, const gh_event_data_prefix *, gh_scene_filters *, uint32_t *);
gh_game_event_status gh_event_object_light_move_000086b0(gh_event_light_filter *, uint32_t *);
gh_game_event_status gh_event_filter_move_000070d0(gh_event_light_filter *, uint32_t *);

#ifdef __cplusplus
}
#endif
#endif
