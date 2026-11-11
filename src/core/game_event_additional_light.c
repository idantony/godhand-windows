#include "game_event_additional_light.h"

#include <math.h>
#include <string.h>

enum {
    COMPONENT = 14, ROW_SIZE = 0x44, RECORD_SIZE = 0x70, RECORDS = 256,
    EV01_CUTS = 10, EV01_ROWS = 42
};

static uint16_t half(const uint8_t *p)
{
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static uint32_t word(const uint8_t *p)
{
    uint32_t result;
    memcpy(&result, p, sizeof(result));
    return result;
}

static float number(const uint8_t *p)
{
    float result;
    memcpy(&result, p, sizeof(result));
    return result;
}

static void put(uint8_t *p, uint32_t value)
{
    memcpy(p, &value, sizeof(value));
}

static void put_half(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static uint8_t *record(gh_game_world_light *world, unsigned i)
{
    return world->scalar + 0x80 + i * RECORD_SIZE;
}

static gh_game_event_status track_at(gh_event_link *link, const gh_event_data_prefix *data,
    gh_event_sequence_view *track, gh_event_component_view *component)
{
    if (!link || !data || !data->resource || !data->resource->loaded ||
        !link->components.constructed || data->components != &link->components ||
        gh_event_component_at(&link->components, COMPONENT, component) ||
        component->original_offset != 0xcf90 || component->size != 0x1c)
        return GH_EVENT_ARGUMENT;
    gh_game_event_status status = gh_event_sequence_at(data, COMPONENT, track);
    if (status) return status;
    if (word(component->bytes + 0x14) != track->count) return GH_EVENT_FORMAT;
    if (link->components.event_no != 1 || data->camera_count != EV01_CUTS ||
        track->count != EV01_ROWS || track->stride != ROW_SIZE ||
        track->bytes != EV01_ROWS * ROW_SIZE || !track->records)
        return GH_EVENT_UNSUPPORTED;
    return GH_EVENT_OK;
}

static gh_game_event_status map_keys(const gh_event_data_prefix *data,
    gh_event_sequence_view track, uint16_t cut_start[EV01_CUTS + 1])
{
    for (unsigned cut = 0; cut < EV01_CUTS; ++cut) {
        gh_event_camera_cut camera;
        uint16_t length;
        if (gh_event_camera_cut_at(data, cut, &camera) || !camera.primary ||
            gh_event_camera_cut_length_00004e60(data, cut, &length) || !length ||
            length > INT16_MAX || length > UINT16_MAX - cut_start[cut])
            return GH_EVENT_UNSUPPORTED;
        cut_start[cut + 1] = (uint16_t)(cut_start[cut] + length);
    }
    unsigned last_time = 0;
    for (unsigned i = 0; i < track.count; ++i) {
        const uint8_t *key = track.records + i * ROW_SIZE;
        const unsigned cut = key[3], frame = half(key + 6), id = half(key + 0x10);
        /* 6978 keeps FFFF, 8378 maps it to camera-global FF. The four timing
         * repair calls leave these primary keys inside their original cuts. */
        if (word(key) != (0xffffu | cut << 24) || half(key + 4) ||
            cut >= EV01_CUTS || frame >= (unsigned)(cut_start[cut + 1] - cut_start[cut]) ||
            cut_start[cut] + frame < last_time ||
            key[0x0c] || (key[0x0d] & ~4u) || key[0x0e] != 2 || key[0x0f] ||
            id < 1 || id > 3)
            return GH_EVENT_UNSUPPORTED;
        for (unsigned j = 0; j < 9; ++j) {
            if (!isfinite(number(key + 0x14 + j * 4))) return GH_EVENT_FORMAT;
        }
        last_time = cut_start[cut] + frame;
    }
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_additional_light_init_000040d8(gh_event_additional_light *state,
    gh_event_link *link, const gh_event_data_prefix *data, gh_game_world_light *world,
    uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x40d8;
    if (!state || !world || !world->constructed || !world->ready ||
        (state->world_light && state->world_light != world)) return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    gh_event_sequence_view track;
    *frontier = 0xc8b8;
    gh_game_event_status status = track_at(link, data, &track, &component);
    if (status) return status;
    gh_event_additional_light bound = { .link = link, .data = data, .world_light = world,
        .allocation = data->resource->allocation.pointer };
    status = map_keys(data, track, bound.cut_start);
    if (status) return status;
    memcpy(component.bytes, "ADDLIGHT", sizeof("ADDLIGHT"));
    put(world->scalar + 0x78, 0);
    memset(record(world, 0), 0, RECORD_SIZE * RECORDS);
    for (unsigned i = 0; i < RECORDS; ++i) put_half(record(world, i), 0xff);
    memset(world->native_light_owners, 0, sizeof(world->native_light_owners));
    *state = bound;
    *frontier = 0;
    return GH_EVENT_OK;
}

/* 2D9158 removes a matching event ID, copying only the assignment fields
 * while shifting the dense list. 2D90B0 and 2D8810 use full records. */
static void erase(gh_game_world_light *world, unsigned id)
{
    unsigned count = word(world->scalar + 0x78);
    for (unsigned i = 0; i < count; ++i) {
        uint8_t *p = record(world, i);
        if (half(p) == 0xff || half(p + 2) != id || word(p + 0x54)) continue;
        for (unsigned j = i; j + 1 < count; ++j) {
            uint8_t *dst = record(world, j);
            const uint8_t *src = record(world, j + 1);
            memcpy(dst, src, 8);
            for (unsigned v = 0; v < 3; ++v)
                memcpy(dst + 0x10 + v * 0x10, src + 0x10 + v * 0x10, 12);
            memcpy(dst + 0x40, src + 0x40, 0x24);
            world->native_light_owners[j] = world->native_light_owners[j + 1];
        }
        put_half(record(world, --count), 0xff);
        world->native_light_owners[count] = NULL;
        put(world->scalar + 0x78, count);
    }
}

static void refresh_preset(gh_game_world_light *world, const uint8_t *preset, size_t preset_count,
    int copy_room_lights)
{
    unsigned count = word(world->scalar + 0x78), kept = 0;
    for (unsigned i = 0; i < count; ++i) {
        uint8_t *src = record(world, i);
        if (half(src) == 0xff || !half(src + 2)) continue;
        uint8_t saved[RECORD_SIZE];
        memcpy(saved, src, sizeof(saved));
        put_half(src, 0xff);
        memcpy(record(world, kept), saved, sizeof(saved));
        world->native_light_owners[kept++] = world->native_light_owners[i];
    }
    /* 2D8810's Event gate follows the compaction and bypasses the count
     * write along with the room-preset loop. */
    if (!copy_room_lights) return;
    for (size_t i = 0; i < preset_count; ++i) {
        const uint8_t *src = preset + i * RECORD_SIZE;
        if (half(src) == 0xff) continue;
        memcpy(record(world, kept), src, RECORD_SIZE);
        world->native_light_owners[kept++] = NULL;
    }
    put(world->scalar + 0x78, kept);
}

static void set_key(gh_event_additional_light *state, const uint8_t *key,
    const uint8_t *preset, size_t preset_count, int copy_room_lights)
{
    gh_game_world_light *world = state->world_light;
    const unsigned id = half(key + 0x10);
    erase(world, id);
    const unsigned i = word(world->scalar + 0x78);
    uint8_t *p = record(world, i);
    memset(p, 0, RECORD_SIZE);
    put_half(p, key[0x0e]);
    put_half(p + 2, (uint16_t)id);
    for (unsigned j = 0; j < 3; ++j) {
        uint32_t *v = state->link->vectors_cf30_cf50[j];
        for (unsigned k = 0; k < 3; ++k) v[k] = word(key + 0x14 + j * 12 + k * 4);
        v[3] = 0x3f800000;
        memcpy(p + 0x10 + j * 0x10, v, 0x10);
    }
    memcpy(p + 0x40, key + 0x38, 4);
    put(p + 0x50, 1);
    put(p + 0x58, 0xf0800000);
    put_half(p + 0x5c, half(key + 0x42));
    put_half(p + 0x5e, half(key + 0x40));
    memcpy(p + 0x60, key + 0x3c, 4);
    world->native_light_owners[i] = NULL;
    put(world->scalar + 0x78, i + 1);
    if (preset) refresh_preset(world, preset, preset_count, copy_room_lights);
}

static unsigned key_time(const gh_event_additional_light *state, const uint8_t *key)
{
    return state->cut_start[key[3]] + half(key + 6);
}

static void interpolate(gh_event_additional_light *state, gh_event_sequence_view track,
    unsigned cut, float frame)
{
    const float now = state->cut_start[cut] + frame;
    gh_game_world_light *world = state->world_light;
    const unsigned count = word(world->scalar + 0x78);
    for (unsigned i = 0; i < count; ++i) {
        uint8_t *p = record(world, i);
        const unsigned id = half(p + 2);
        if (!id || word(p + 0x54)) continue;
        const uint8_t *previous = NULL, *next = NULL;
        for (unsigned j = 0; j < track.count; ++j) {
            const uint8_t *key = track.records + j * ROW_SIZE;
            if (half(key + 0x10) != id) continue;
            if (key_time(state, key) <= now) previous = key;
            else {
                next = key;
                break;
            }
        }
        if (!previous || !next || !(previous[0x0d] & 4u)) continue;
        const float end = (float)key_time(state, next), start = (float)key_time(state, previous);
        const volatile float remainder = (end - now) / (end - start);
        const float ratio = 1.0f - remainder;
        for (unsigned v = 0; v < 3; ++v) {
            for (unsigned k = 0; k < 3; ++k) {
                const size_t key_offset = 0x14 + v * 12 + k * 4;
                const float a = number(previous + key_offset), b = number(next + key_offset);
                float value = a;
                if (a != b) {
                    const volatile float left = a * (1.0f - ratio), right = b * ratio;
                    value = left + right;
                }
                memcpy(p + 0x10 + v * 0x10 + k * 4, &value, sizeof(value));
            }
        }
    }
}

gh_game_event_status gh_event_additional_light_move_00004140(gh_event_additional_light *state,
    const gh_boot_work *work, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x4140;
    if (!state || !work || !state->world_light || !state->world_light->ready || !state->data ||
        !state->data->resource || state->allocation != state->data->resource->allocation.pointer)
        return GH_EVENT_ARGUMENT;
    gh_event_sequence_view track;
    gh_event_component_view component, camera, env;
    gh_game_event_status status = track_at(state->link, state->data, &track, &component);
    if (status) return status;
    if (gh_event_component_at(&state->link->components, 1, &camera) ||
        gh_event_component_at(&state->link->components, 0, &env))
        return GH_EVENT_ARGUMENT;
    const unsigned cut = half(camera.bytes + 6);
    const float frame = number(camera.bytes + 0x0c);
    if (camera.bytes[4] || cut >= EV01_CUTS || cut != word(env.bytes + 0x10) ||
        !isfinite(frame) || frame < 0 ||
        frame > state->cut_start[cut + 1] - state->cut_start[cut] - 1) {
        *frontier = 0x4ed0;
        return GH_EVENT_UNSUPPORTED;
    }
    gh_game_world_light *world = state->world_light;
    const unsigned count = word(world->scalar + 0x78);
    if (count > RECORDS) return GH_EVENT_BOUNDS;
    unsigned adds = 0;
    for (unsigned i = 0; i < track.count; ++i) {
        const uint8_t *key = track.records + i * ROW_SIZE;
        adds += key[3] == cut && (float)half(key + 6) == frame;
    }
    if (adds > RECORDS - count) return GH_EVENT_BOUNDS;
    const uint32_t system_flags = word((const uint8_t *)work + 0x5e4);
    const int copy_room_lights = !(system_flags & 0x20000000u);
    const uint8_t *preset = NULL;
    size_t preset_count = 0;
    if (adds && !(system_flags & 0x08000000u)) {
        *frontier = 0x2d8810;
        const unsigned index = word(world->scalar + 0x16288);
        if (gh_game_world_light_preset_records(world, index, &preset, &preset_count))
            return GH_EVENT_BOUNDS;
        if (preset && copy_room_lights) {
            size_t kept = 0;
            for (unsigned i = 0; i < count; ++i) {
                const uint8_t *p = record(world, i);
                if (half(p) != 0xff && half(p + 2)) ++kept;
            }
            if (kept + preset_count > RECORDS - adds) return GH_EVENT_BOUNDS;
        }
    }
    /* No deletion/restore rows in EV01. The cut-start purge precedes every
     * exact-frame add, including repeated IDs in file order. */
    if (number(env.bytes + 0x14) == 0) {
        for (unsigned i = 0; i < RECORDS; ++i) {
            const uint8_t *p = record(world, i);
            if ((word(p + 0x58) & 0x80000u) && !word(p + 0x54)) erase(world, half(p + 2));
        }
    }
    for (unsigned i = 0; i < track.count; ++i) {
        const uint8_t *key = track.records + i * ROW_SIZE;
        if (key[3] == cut && (float)half(key + 6) == frame)
            set_key(state, key, preset, preset_count, copy_room_lights);
    }
    interpolate(state, track, cut, frame);
    *frontier = 0;
    return GH_EVENT_OK;
}
