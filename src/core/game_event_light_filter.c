#include "game_event_light_filter.h"

#include <math.h>
#include <string.h>

enum { OBJECT_LIGHT = 6, FILTER = 7 };

static uint32_t read_word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int read_component(gh_event_components *components, unsigned index,
                          uint32_t original_offset, size_t size, gh_event_component_view *out)
{
    return components && components->constructed &&
           !gh_event_component_at(components, index, out) &&
           out->bytes && out->original_offset == original_offset && out->size == size;
}

static gh_game_event_status read_track(const gh_event_light_filter *state,
    gh_event_components *components, const gh_event_data_prefix *data, unsigned index,
    gh_event_component_view *component, gh_event_sequence_view *track)
{
    if (!state || !data || !data->resource || !data->resource->loaded ||
        data->components != components ||
        (state->data && (state->data != data || state->components != components ||
         state->allocation != data->resource->allocation.pointer)))
        return GH_EVENT_ARGUMENT;
    if (!read_component(components, index, index == OBJECT_LIGHT ? 0xd860 : 0xd268,
                        index == OBJECT_LIGHT ? 0x34 : 0x5e8, component))
        return GH_EVENT_ARGUMENT;
    gh_game_event_status status = gh_event_sequence_at(data, index, track);
    if (status) return status;
    if (read_word(component->bytes + 0x14) != track->count) return GH_EVENT_FORMAT;
    return GH_EVENT_OK;
}

/* EV01/EV02 keys use the primary camera at frame zero. Original 8378
 * maps FFFF to lane FF, and the four timing repair calls keep these times. */
static gh_game_event_status mapped_keys(const gh_event_data_prefix *data,
                                        const gh_event_sequence_view *track, unsigned index)
{
    const unsigned event = data->components->event_no;
    const int object_light = index == OBJECT_LIGHT;
    if ((event != 1 && event != 2) || data->camera_count != (event == 1 ? 10 : 28))
        return GH_EVENT_UNSUPPORTED;
    const unsigned count = object_light ? event : event == 1 ? 10 : 25;
    const size_t stride = object_light ? 0x1c : 0x2e8;
    if (!track->records || track->count != count || track->stride != stride ||
        track->bytes != count * stride) return GH_EVENT_UNSUPPORTED;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *key = track->records + i * stride;
        unsigned cut = i;
        if (object_light && event == 1) cut = 1;
        if (!object_light && event == 2 && i >= 16) cut += 3;
        uint16_t length;
        gh_event_camera_cut camera;
        if (read_word(key) != (0xffffu | cut << 24) || read_word(key + 4) ||
            gh_event_camera_cut_at(data, cut, &camera) || !camera.primary ||
            gh_event_camera_cut_length_00004e60(data, cut, &length) || !length)
            return GH_EVENT_UNSUPPORTED;
    }
    return GH_EVENT_OK;
}

static void bind(gh_event_light_filter *state, gh_event_components *components,
                 const gh_event_data_prefix *data)
{
    state->data = data;
    state->allocation = data->resource->allocation.pointer;
    state->components = components;
}

gh_game_event_status gh_event_object_light_init_00008648(gh_event_light_filter *state,
    gh_event_components *components, const gh_event_data_prefix *data,
    gh_game_world_light *owner, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x8648;
    if (!state || !owner || !owner->constructed || !owner->ready ||
        (state->world_light && state->world_light != owner)) return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    gh_event_sequence_view track;
    *frontier = 0xbe58;
    gh_game_event_status status = read_track(state, components, data, OBJECT_LIGHT, &component, &track);
    if (status) return status;
    if ((status = mapped_keys(data, &track, OBJECT_LIGHT))) return status;

    memcpy(component.bytes, "OBJLIGHT", sizeof("OBJLIGHT"));
    memset(component.bytes + 0x18, 0, 0x1c);
    memcpy(component.bytes + 0x24, owner->scalar + 0x40, 0x10);
    bind(state, components, data);
    state->world_light = owner;
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_filter_init_00007040(gh_event_light_filter *state,
    gh_event_components *components, const gh_event_data_prefix *data,
    gh_scene_filters *owner, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x7040;
    if (!state || !owner || !owner->ready ||
        (state->filters && state->filters != owner)) return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    gh_event_sequence_view track;
    *frontier = 0xc250;
    gh_game_event_status status = read_track(state, components, data, FILTER, &component, &track);
    if (status) return status;
    if ((status = mapped_keys(data, &track, FILTER))) return status;

    memcpy(component.bytes, "FILTER", sizeof("FILTER"));
    memset(component.bytes + 0x18, 0, 0x2e8);
    memcpy(component.bytes + 0x28, owner->work.active140c8, 0x2d8);
    bind(state, components, data);
    state->filters = owner;
    *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status previous_key(gh_event_light_filter *state, unsigned index,
    gh_event_component_view *component, const uint8_t **key, uint32_t *frontier)
{
    gh_event_sequence_view track;
    *frontier = index == OBJECT_LIGHT ? 0xbcc8 : 0xc0c0;
    gh_game_event_status status = read_track(state, state->components, state->data,
                                             index, component, &track);
    if (status) return status;
    if ((status = mapped_keys(state->data, &track, index))) return status;
    gh_event_component_view camera, env;
    if (!read_component(state->components, 1, 0xd1c8, 0x18, &camera) ||
        !read_component(state->components, 0, 0xd200, 0x44, &env))
        return GH_EVENT_ARGUMENT;
    const unsigned cut = (unsigned)camera.bytes[6] | (unsigned)camera.bytes[7] << 8;
    float frame;
    const uint32_t frame_bits = read_word(camera.bytes + 0x0c);
    memcpy(&frame, &frame_bits, sizeof(frame));
    if (camera.bytes[4] || cut >= state->data->camera_count ||
        cut != read_word(env.bytes + 0x10) || !isfinite(frame) || frame < 0)
        return GH_EVENT_UNSUPPORTED;
    *key = component->bytes + 0x18;
    const uint8_t *next = NULL;
    for (unsigned i = 0; i < track.count; ++i) {
        const uint8_t *candidate = track.records + i * track.stride;
        if (candidate[3] <= cut) *key = candidate;
        if (candidate[3] > cut || (candidate[3] == cut && frame == 0)) {
            next = candidate;
            break;
        }
    }
    if (((*key)[8] & 2u) || (next && (next[8] & 2u))) {
        *frontier = index == OBJECT_LIGHT ? 0x88b0 : 0x75d0;
        return GH_EVENT_UNSUPPORTED;
    }
    if (((*key)[8] & 1u) && next && next != *key) {
        *frontier = index == OBJECT_LIGHT ? 0x87a8 : 0x72a0;
        return GH_EVENT_UNSUPPORTED;
    }
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_object_light_move_000086b0(gh_event_light_filter *state,
                                                          uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x86b0;
    if (!state || !state->world_light || !state->world_light->ready) return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    const uint8_t *key;
    gh_game_event_status status = previous_key(state, OBJECT_LIGHT, &component, &key, frontier);
    if (status) return status;
    memcpy(state->world_light->scalar + 0x40, key + 0x0c, 0x0c);
    *frontier = 0;
    return GH_EVENT_OK;
}

/* REL71A0 applies the chosen key, with the origin's 0x64-byte field override. */
static void apply_filter_key(gh_scene_filters *filters, const uint8_t *component,
                             const uint8_t *key)
{
    uint8_t *active = filters->work.active140c8;
    memcpy(active, key + 0x10, 0x2d8);
    memcpy(active + 0x208, component + 0x230, 0x64);
}

gh_game_event_status gh_event_filter_move_000070d0(gh_event_light_filter *state,
                                                     uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x70d0;
    if (!state || !state->filters || !state->filters->ready) return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    const uint8_t *key;
    gh_game_event_status status = previous_key(state, FILTER, &component, &key, frontier);
    if (status) return status;
    apply_filter_key(state->filters, component.bytes, key);
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_filter_quit_000075a0(gh_event_light_filter *state,
    const gh_game_event_config *config, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x75a0;
    if (!config || !config->initialized) return GH_EVENT_ARGUMENT;
    if (!(config->flags & 0x100u)) {
        gh_event_component_view component;
        *frontier = 0x71a0;
        if (!state || !state->filters || !state->filters->ready ||
            !state->data || !state->data->resource || !state->data->resource->loaded ||
            state->allocation != state->data->resource->allocation.pointer ||
            !read_component(state->components, FILTER, 0xd268, 0x5e8, &component))
            return GH_EVENT_ARGUMENT;
        apply_filter_key(state->filters, component.bytes, component.bytes + 0x18);
    }
    *frontier = 0;
    return GH_EVENT_OK;
}
