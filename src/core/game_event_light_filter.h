#ifndef GH_GAME_EVENT_LIGHT_FILTER_H
#define GH_GAME_EVENT_LIGHT_FILTER_H
#include "game_event_data_prefix.h"
#include "game_event_environment.h"
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

/* Canonical component scratch holds the original origin keys. The EV01/EV02 mapping
 * is validated without rewriting serialized SEQ bytes. Frontiers are REL offsets. */
/* REL04F08 and REL06C38 shared by the three cinematic light tracks.
 * Call inside the consumer's binary32 floating-point environment. */
gh_game_event_status gh_event_light_interpolation_factor_00004f08(const gh_event_data_prefix *,
    const uint8_t *, const uint8_t *, unsigned cut, float frame, float *);
float gh_event_light_lerp_00006c38(float a, float b, float factor);
gh_game_event_status gh_event_object_light_init_00008648(gh_event_light_filter *,
    gh_event_components *, const gh_event_data_prefix *, gh_game_world_light *, uint32_t *);
gh_game_event_status gh_event_filter_init_00007040(gh_event_light_filter *,
    gh_event_components *, const gh_event_data_prefix *, gh_scene_filters *, uint32_t *);
gh_game_event_status gh_event_object_light_move_000086b0(gh_event_light_filter *, uint32_t *);
gh_game_event_status gh_event_filter_move_000070d0(gh_event_light_filter *, uint32_t *);
gh_game_event_status gh_event_filter_quit_000075a0(gh_event_light_filter *,
    const gh_game_event_config *, uint32_t *);

#ifdef __cplusplus
}
#endif
#endif
