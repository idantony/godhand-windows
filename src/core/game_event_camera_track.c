#include "game_event_camera_track.h"

#include <math.h>
#include <string.h>

static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void store(uint8_t *p, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (8 * i));
}

static float read_frame(const uint8_t *p) {
    uint32_t bits = word(p);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static gh_game_event_status motion_status(gh_motion_status status) {
    switch (status) {
        case GH_MOTION_OK: return GH_EVENT_OK;
        case GH_MOTION_ARGUMENT: return GH_EVENT_ARGUMENT;
        case GH_MOTION_BOUNDS: return GH_EVENT_BOUNDS;
        case GH_MOTION_FORMAT: return GH_EVENT_FORMAT;
        case GH_MOTION_UNSUPPORTED: return GH_EVENT_UNSUPPORTED;
        default: return GH_EVENT_CALLEE;
    }
}

gh_game_event_status gh_event_camera_select_00004c08(gh_event_link *link,
    const gh_event_data_prefix *data, gh_event_camera_track *out, uint32_t *frontier) {
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x4c08;
    if (!link || !data || !out || data->components != &link->components || !link->components.constructed ||
        !link->module || !link->module->loaded || link->module->job != link->module_job ||
        !link->module->allocation.pointer || link->module->allocation.pointer != link->components.module) {
        return GH_EVENT_ARGUMENT;
    }

    gh_event_component_view env, camera;
    if (gh_event_component_at(&link->components, 0, &env) || env.original_offset != 0xd200 || env.size != 0x44 ||
        gh_event_component_at(&link->components, 1, &camera) || camera.original_offset != 0xd1c8 || camera.size != 0x18) {
        return GH_EVENT_ARGUMENT;
    }
    gh_event_symbol_binding debug;
    if (gh_game_event_link_symbol(link, "EventDebug", &debug) || debug.imported || debug.original_address != 0xd1e0 ||
        link->module->file_bytes < 0xd1e4) return GH_EVENT_FORMAT;
    if (word((const uint8_t *)link->components.module + 0xd1e0) & 2u) {
        *frontier = 0x4c34;
        return GH_EVENT_UNSUPPORTED;
    }

    const uint32_t cut = word(env.bytes + 0x10);
    const float frame = read_frame(env.bytes + 0x14);
    gh_event_camera_cut source;
    gh_game_event_status status = gh_event_camera_cut_at(data, cut, &source);
    if (status) return status;
    if (!isfinite(frame) || frame < 0 || frame >= 16777216.0f || (!source.primary && !source.alternate)) {
        return GH_EVENT_FORMAT;
    }

    const int alternate = !source.primary;
    gh_event_camera_track selected = {0};
    status = motion_status(gh_motion_open_mtb3(data->data, data->bytes,
        alternate ? source.alternate_offset : source.primary_offset, &selected.track));
    if (status) return status;
    if (selected.track.frames > INT16_MAX || selected.track.count < 8) return GH_EVENT_FORMAT;

    const uint32_t repeats = alternate ? (uint32_t)frame / selected.track.frames : 0;
    const float last = (float)(selected.track.frames - 1u);
    selected.frame = alternate ? frame - (float)(repeats * selected.track.frames) : (frame < last ? frame : last);
    selected.apply_motion = (word(env.bytes + 0x28) & 0x40000000u) == 0;
    uint32_t frame_bits;
    memcpy(&frame_bits, &selected.frame, sizeof(frame_bits));
    camera.bytes[4] = (uint8_t)alternate;
    camera.bytes[6] = (uint8_t)cut;
    camera.bytes[7] = (uint8_t)(cut >> 8);
    store(camera.bytes + 8, repeats);
    store(camera.bytes + 0x0c, frame_bits);
    store(camera.bytes + 0x10, 0); /* Borrowed reference lives in selected.track. */
    *out = selected;
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_motion_camera_sample_00139180(const gh_motion_view *track,
    float frame, gh_motion_camera_sample *out, uint32_t *frontier) {
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x139180;
    if (!track || track->count < 8 || !out) return GH_EVENT_ARGUMENT;

    float values[8];
    for (unsigned i = 0; i < 8; ++i) {
        gh_motion_cache cache;
        uint32_t child = 0;
        gh_motion_status status = gh_motion_sample_reference(track, i, frame, values + i, &cache, &child);
        if (status) {
            *frontier = child ? child : 0x1391f4 + i * 0x2c;
            return motion_status(status);
        }
    }
    *out = (gh_motion_camera_sample){
        .eye = {values[0], values[1], values[2]},
        .target = {values[3], values[4], values[5]},
        .roll = values[6], .fov = values[7]
    };
    *frontier = 0;
    return GH_EVENT_OK;
}
