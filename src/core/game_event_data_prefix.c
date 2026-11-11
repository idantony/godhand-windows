#include "game_event_data_prefix.h"

static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint16_t half(const uint8_t *p) {
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static void put_word(uint8_t *p, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8));
}

static int span(size_t bytes, size_t offset, size_t length) {
    return offset <= bytes && length <= bytes - offset;
}

static int current(const gh_event_data_prefix *p) {
    return p && p->bound && p->resource && p->resource->loaded && p->components && p->components->constructed &&
           p->data == p->resource->allocation.pointer && p->bytes == p->resource->file_bytes &&
           p->resource->data_number == p->components->event_no;
}

static gh_game_event_status camera_cut(const gh_event_data_prefix *p, size_t index, gh_event_camera_cut *out) {
    if (index >= p->camera_count || !span(p->bytes, p->camera_table, (index + 1) * 12)) return GH_EVENT_BOUNDS;
    const uint8_t *record = p->data + p->camera_table + index * 12;
    size_t primary = word(record + 4), alternate = word(record + 8);
    if ((primary && !span(p->bytes, primary, 6)) || (alternate && !span(p->bytes, alternate, 6))) return GH_EVENT_BOUNDS;
    *out = (gh_event_camera_cut){word(record), primary, alternate,
                                 primary ? p->data + primary : NULL, alternate ? p->data + alternate : NULL};
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_data_bind_prefix_002977e8(
    gh_event_data_prefix *prefix, gh_event_components *components,
    const gh_event_resource *resource, uint32_t *frontier) {
    if (frontier) *frontier = 0x2977e8;
    if (!prefix || prefix->bound || !components || !components->constructed || !resource || !resource->loaded ||
        !resource->allocation.pointer || resource->file_bytes > resource->allocated_bytes ||
        resource->data_number != components->event_no) return GH_EVENT_ARGUMENT;
    if (resource->file_bytes < 0x5c) return GH_EVENT_BOUNDS;

    gh_event_component_view env, camera, actor;
    if (gh_event_component_at(components, 0, &env) || gh_event_component_at(components, 1, &camera) ||
        gh_event_component_at(components, 2, &actor) || !env.bytes || !camera.bytes || !actor.bytes ||
        env.original_offset != 0xd200 || env.size != 0x44 || camera.original_offset != 0xd1c8 || camera.size != 0x18 ||
        actor.original_offset != 0xcf00 || actor.size != 0x1c) return GH_EVENT_ARGUMENT;

    const uint8_t *data = resource->allocation.pointer;
    gh_event_data_prefix next = {
        .resource = resource, .components = components, .data = data, .bytes = resource->file_bytes,
        .camera_table = word(data + 4), .actor_info_table = word(data + 8),
        .camera_count = data[1], .actor_count = data[2], .bound = 1
    };
    if (frontier) *frontier = 0x297824;
    if (next.camera_count && (!next.camera_table || !span(next.bytes, next.camera_table, next.camera_count * 12u))) {
        return GH_EVENT_BOUNDS;
    }
    for (size_t cut = 0; cut < next.camera_count; ++cut) {
        gh_event_camera_cut view;
        gh_game_event_status status = camera_cut(&next, cut, &view);
        if (status != GH_EVENT_OK) return status;
    }
    if (frontier) *frontier = 0x297848;
    if (next.actor_count && (!next.actor_info_table || !span(next.bytes, next.actor_info_table, next.actor_count * 8u))) {
        return GH_EVENT_BOUNDS;
    }

    put_word(env.bytes + 8, next.camera_count);
    put_word(actor.bytes + 4, next.actor_count);
    // The raw guest pointer fields stay null. Use the typed views in prefix.
    put_word(camera.bytes + 0x14, 0);
    put_word(actor.bytes + 0x0c, 0);
    *prefix = next;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_camera_cut_at(const gh_event_data_prefix *prefix, size_t index, gh_event_camera_cut *out) {
    if (!current(prefix) || !out) return GH_EVENT_ARGUMENT;
    return camera_cut(prefix, index, out);
}

gh_game_event_status gh_event_camera_cut_length_00004e60(const gh_event_data_prefix *prefix, size_t index, uint16_t *length) {
    if (!length) return GH_EVENT_ARGUMENT;
    gh_event_camera_cut cut;
    gh_game_event_status status = gh_event_camera_cut_at(prefix, index, &cut);
    if (status != GH_EVENT_OK) return status;
    const uint8_t *track = cut.primary ? cut.primary : cut.alternate;
    if (!track) return GH_EVENT_UNSUPPORTED;
    *length = (uint16_t)(track[4] | (unsigned)track[5] << 8);
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_info_at(const gh_event_data_prefix *prefix, size_t index, const uint8_t **out) {
    if (!current(prefix) || !out) return GH_EVENT_ARGUMENT;
    if (index >= prefix->actor_count || !span(prefix->bytes, prefix->actor_info_table, (index + 1) * 8)) return GH_EVENT_BOUNDS;
    *out = prefix->data + prefix->actor_info_table + index * 8;
    return GH_EVENT_OK;
}

typedef struct {
    gh_event_actor_cut_lane previous;
    size_t last_sequence;
    uint8_t last_cut;
    uint8_t seen_sequence;
} lane_history;

static gh_game_event_status resolve_lane(const gh_event_data_prefix *p, const uint8_t *fields,
                                        uint8_t index, int inherit, lane_history *history,
                                        gh_event_actor_cut_lane *out) {
    const size_t primary = word(fields), sequence = word(fields + 4);
    if ((primary && !span(p->bytes, primary, 6)) || (sequence && !span(p->bytes, sequence, 1))) {
        return GH_EVENT_BOUNDS;
    }
    gh_event_actor_cut_lane lane = {
        .primary_offset = primary, .sequence_offset = sequence, .source_cut = fields[8],
        .timing_cut = fields[9], .primary_length = half(fields + 10)
    };
    const uint16_t length = primary ? half(p->data + primary + 4) : 0;
    if (sequence) {
        lane.source_cut = lane.timing_cut = index;
        lane.primary_length = length;
        history->last_cut = index;
        history->seen_sequence = 1;
        history->last_sequence = sequence;
    }
    if (inherit) {
        if (!primary && !index) return GH_EVENT_UNSUPPORTED;
        if (!primary && history->previous.sequence_offset) {
            lane.sequence_offset = history->previous.sequence_offset;
            lane.source_cut = history->previous.source_cut;
            lane.timing_cut = history->previous.timing_cut;
            lane.primary_length = history->previous.primary_length;
        } else {
            if (!history->seen_sequence) return GH_EVENT_UNSUPPORTED;
            lane.sequence_offset = history->last_sequence;
            lane.source_cut = history->last_cut;
            lane.timing_cut = index;
            lane.primary_length = length;
        }
    }
    // If this row supplied a sequence, the original last-index now refers
    // to this row's resolved sequence, even when the carry branch replaced it.
    if (sequence) history->last_sequence = lane.sequence_offset;
    lane.primary = primary ? p->data + primary : NULL;
    lane.sequence = lane.sequence_offset ? p->data + lane.sequence_offset : NULL;
    history->previous = lane;
    *out = lane;
    return GH_EVENT_OK;
}

static gh_game_event_status actor_cut(const gh_event_data_prefix *p, size_t actor, size_t target,
                                      gh_event_actor_cut *out) {
    const uint8_t *rows = p->data + p->actor_cut_table + actor * p->camera_count * 0x1c;
    lane_history history[2] = {0};
    gh_event_actor_cut resolved;
    for (size_t index = 0; index <= target; ++index) {
        const uint8_t *row = rows + index * 0x1c;
        resolved.flags = word(row);
        for (unsigned lane = 0; lane < 2; ++lane) {
            gh_game_event_status status = resolve_lane(p, row + 4 + lane * 12, (uint8_t)index,
                                                       (row[0] >> lane) & 1, history + lane,
                                                       resolved.lanes + lane);
            if (status != GH_EVENT_OK) return status;
        }
    }
    *out = resolved;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_cuts_bind_00297864(gh_event_data_prefix *p, uint32_t *frontier) {
    if (frontier) *frontier = 0x297864;
    if (!current(p)) return GH_EVENT_ARGUMENT;
    if (p->actor_cuts_bound) {
        if (frontier) *frontier = 0;
        return GH_EVENT_OK;
    }

    gh_event_data_prefix next = *p;
    next.actor_cut_table = word(next.data + 0x0c);
    const size_t bytes = (size_t)next.actor_count * next.camera_count * 0x1c;
    if (bytes && (!next.actor_cut_table || !span(next.bytes, next.actor_cut_table, bytes))) return GH_EVENT_BOUNDS;
    if (next.camera_count) {
        for (size_t actor = 0; actor < next.actor_count; ++actor) {
            gh_event_actor_cut unused;
            gh_game_event_status status = actor_cut(&next, actor, next.camera_count - 1u, &unused);
            if (status != GH_EVENT_OK) return status;
        }
    }
    next.actor_cuts_bound = 1;
    *p = next;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_cut_at(const gh_event_data_prefix *p, size_t actor, size_t cut,
                                         gh_event_actor_cut *out) {
    if (!current(p) || !p->actor_cuts_bound || !out) return GH_EVENT_ARGUMENT;
    if (actor >= p->actor_count || cut >= p->camera_count ||
        !span(p->bytes, p->actor_cut_table, (size_t)p->actor_count * p->camera_count * 0x1c)) return GH_EVENT_BOUNDS;
    return actor_cut(p, actor, cut, out);
}

typedef struct {
    uint16_t header_word;
    uint8_t component;
    uint32_t callsite, component_offset;
    size_t stride;
} sequence_binding;

static const sequence_binding tail_bindings[] = {
    {0x50, 3, 0x297880, 0xda08, 0x0c}, // Vertex
    {0x18, 4, 0x2978a0, 0xd1e8, 0x2c}, // Effect sequence
    {0x1c, 4, 0x2978b8, 0xd1e8, 0}, // Effect data registration
    {0x20, 5, 0x2978d4, 0xd910, 0x60}, // Scroll light
    {0x24, 6, 0x2978f0, 0xd860, 0x1c}, // Object light
    {0x28, 7, 0x29790c, 0xd268, 0x2e8}, // Filter
    {0x30, 8, 0x297928, 0xd9f0, 0}, // SE
    {0x2c, 9, 0x297944, 0xd1b0, 0}, // BGM
    {0x48, 10, 0x297960, 0xd898, 0}, // Scroll layer
    {0x54, 11, 0x29797c, 0xda20, 0}, // Vibration
    {0x44, 12, 0x297998, 0xda40, 0}, // Wait
    {0x4c, 13, 0x2979b4, 0xd248, 0x10}, // Fade
    {0x58, 14, 0x2979d0, 0xcf90, 0x44} // Additional light
};

static size_t section_end(const gh_event_data_prefix *prefix, size_t offset) {
    size_t end = prefix->bytes;
    for (unsigned field = 4; field <= 0x58; field += 4) {
        const size_t next = word(prefix->data + field);
        if (next > offset && next < end) end = next;
    }
    return end;
}

static gh_game_event_status bind_sequence(gh_event_data_prefix *p, const sequence_binding *binding,
                                         gh_event_component_view *component, size_t offset) {
    gh_event_sequence_view result = {0};
    if (offset) {
        if (!binding->stride) return GH_EVENT_UNSUPPORTED;
        const size_t end = section_end(p, offset);
        if (offset < 0x5c || !span(end, offset, 0x10)) return GH_EVENT_BOUNDS;
        const uint8_t *header = p->data + offset;
        if (word(header) != 0x00514553u) return GH_EVENT_FORMAT;
        const uint32_t count = word(header + 4);
        if (count > (end - offset - 0x10) / binding->stride) return GH_EVENT_BOUNDS;
        result = (gh_event_sequence_view){header + 0x10, offset, count * binding->stride, binding->stride, count};
    }
    put_word(component->bytes + 0x10, 0);
    put_word(component->bytes + 0x14, result.count);
    p->sequences[binding->component] = result;
    p->sequence_mask |= (uint16_t)(1u << binding->component);
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_data_bind_tail_00297880(
    gh_event_data_prefix *p, gh_event_components *components, gh_event_effect_data_register registration,
    void *context, uint32_t *frontier) {
    if (frontier) *frontier = 0x297880;
    if (!current(p) || !p->actor_cuts_bound || p->components != components) return GH_EVENT_ARGUMENT;
    const unsigned total = sizeof(tail_bindings) / sizeof(*tail_bindings);
    if (p->tail_step > total || (p->effect_register &&
        (p->effect_register != registration || p->effect_context != context))) return GH_EVENT_ARGUMENT;
    for (; p->tail_step < total; ++p->tail_step) {
        const sequence_binding *binding = tail_bindings + p->tail_step;
        if (frontier) *frontier = binding->callsite;
        const size_t offset = word(p->data + binding->header_word);
        gh_event_component_view component;
        if (gh_event_component_at(components, binding->component, &component) ||
            !component.bytes || component.size < 0x18 || component.original_offset != binding->component_offset) {
            return GH_EVENT_ARGUMENT;
        }
        if (binding->header_word == 0x1c) {
            if (!offset) continue; // EffEventDataRegist's genuine null return.
            const size_t end = section_end(p, offset);
            if (offset < 0x5c || !span(end, offset, 4)) return GH_EVENT_BOUNDS;
            if (!registration) return GH_EVENT_UNSUPPORTED;
            uint32_t child = 0;
            const gh_game_event_status status = registration(context, p->data + offset, end - offset, &child);
            if (status != GH_EVENT_OK) {
                if (frontier && child) *frontier = child;
                return status;
            }
            p->effect_register = registration;
            p->effect_context = context;
        } else {
            const gh_game_event_status status = bind_sequence(p, binding, &component, offset);
            if (status != GH_EVENT_OK) return status;
        }
    }
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_sequence_at(const gh_event_data_prefix *p, unsigned component,
                                        gh_event_sequence_view *out) {
    if (!current(p) || !out || component >= GH_EVENT_COMPONENT_COUNT) return GH_EVENT_ARGUMENT;
    if (!(p->sequence_mask & (1u << component))) return GH_EVENT_UNSUPPORTED;
    *out = p->sequences[component];
    return GH_EVENT_OK;
}

static gh_game_event_status actor_creation_flags(gh_event_components *components, uint32_t *flags) {
    gh_event_component_view actor;
    if (!components || !components->constructed || gh_event_component_at(components, 2, &actor) ||
        !actor.bytes || actor.original_offset != 0xcf00 || actor.size != 0x1c) return GH_EVENT_ARGUMENT;
    *flags = word(actor.bytes);
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_creation_poll_00297a68(gh_event_components *components, int *ready) {
    if (!ready) return GH_EVENT_ARGUMENT;
    uint32_t flags;
    gh_game_event_status status = actor_creation_flags(components, &flags);
    if (status == GH_EVENT_OK) *ready = flags & 1;
    return status;
}

gh_game_event_status gh_event_actor_creation_end_normal_000012a8(gh_event_components *components, int *normal) {
    if (!normal) return GH_EVENT_ARGUMENT;
    uint32_t flags;
    gh_game_event_status status = actor_creation_flags(components, &flags);
    if (status == GH_EVENT_OK) *normal = ((flags >> 1) ^ 1u) & 1u;
    return status;
}
