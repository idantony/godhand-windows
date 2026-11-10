#include "game_event_actor_create.h"

#include <string.h>

static uint16_t half(const uint8_t *p) {
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void put(uint8_t *p, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8));
}

static gh_game_event_status component(gh_event_components *components, unsigned index,
                                      uint32_t offset, size_t size, gh_event_component_view *view) {
    if (!components || !components->constructed || gh_event_component_at(components, index, view) ||
        !view->bytes || view->original_offset != offset || view->size != size) return GH_EVENT_ARGUMENT;
    return GH_EVENT_OK;
}

static int same(const gh_event_actor_create_bindings *a, const gh_event_actor_create_bindings *b) {
    return a->components == b->components && a->data == b->data && a->heap == b->heap && a->pools == b->pools &&
           a->objects == b->objects && a->global_threads == b->global_threads && a->save.bytes == b->save.bytes &&
           a->save.size == b->save.size && a->object_view == b->object_view && a->context == b->context;
}

static gh_game_event_status suspend_unneeded(gh_event_actor_create *s, uint32_t *frontier) {
    const gh_event_actor_create_bindings *b = &s->bindings;
    gh_event_component_view env;
    if (component(b->components, 0, 0xd200, 0x44, &env)) return GH_EVENT_ARGUMENT;
    if (word(env.bytes + 0x28) & 0x40000000u) return GH_EVENT_OK;

    const gh_room_vector *objects;
    if (gh_room_registry_view(b->pools, 0x754c30, &objects)) return GH_EVENT_BOUNDS;
    for (; s->object_cursor < objects->count; ++s->object_cursor) {
        const uint8_t *entry = (const uint8_t *)objects->allocation.pointer + s->object_cursor * 4;
        gh_title_player_view object = {0};
        uint32_t child = 0;
        gh_game_event_status status = b->object_view(b->context, word(entry), &object, &child);
        if (status) {
            if (frontier && child) *frontier = child;
            return status;
        }
        if (!object.bytes || object.size < 0x300) return GH_EVENT_BOUNDS;
        const uint16_t kind = half(object.bytes + 0x2fe);
        if (kind && (kind < 0x300 || kind >= 0x500 || kind == 0x376)) {
            put(object.bytes + 0x250, word(object.bytes + 0x250) | 0x8000u);
        }
    }
    return GH_EVENT_OK;
}

static int hex_digit(uint8_t c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static gh_game_event_status el_kind(const uint8_t *info, uint16_t *kind) {
    const int hi = hex_digit(info[2]), lo = hex_digit(info[3]);
    if (info[0] != 'e' || info[1] != 'l' || hi < 0 || lo < 0) return GH_EVENT_UNSUPPORTED;
    *kind = (uint16_t)(0x600 + hi * 16 + lo);
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_create_work_at(const gh_event_actor_create *s,
    size_t actor, const uint8_t **work, size_t *cut_offset) {
    if (!s || !s->stage || !work || !cut_offset) return GH_EVENT_ARGUMENT;
    const gh_event_data_prefix *data = s->bindings.data;
    const uint8_t *info;
    if (!data->actor_cuts_bound || gh_event_actor_info_at(data, actor, &info)) return GH_EVENT_BOUNDS;
    const size_t stride = (size_t)data->camera_count * 0x1c;
    if (data->actor_cut_table > data->bytes ||
        (actor + 1) * stride > data->bytes - data->actor_cut_table) return GH_EVENT_BOUNDS;
    *work = (const uint8_t *)s->work.pointer + actor * 0x48;
    *cut_offset = data->actor_cut_table + actor * stride;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_create_task_status(const gh_event_actor_create *s, uint32_t *frontier) {
    if (frontier) *frontier = 0;
    if (!s || s->stage != 3) return GH_EVENT_ARGUMENT;
    if (frontier && s->task_error) *frontier = s->frontier;
    return s->task_error;
}

static gh_game_event_status first_request(gh_event_actor_create *s) {
    const gh_event_actor_create_bindings *b = &s->bindings;
    gh_event_component_view actor;
    s->frontier = 0x297a48;
    if (component(b->components, 2, 0xcf00, 0x1c, &actor)) return GH_EVENT_ARGUMENT;
    if (!b->data->actor_count) return GH_EVENT_UNSUPPORTED; // REL+10F0 player/weapon tail.
    const uint8_t *info;
    if (gh_event_actor_info_at(b->data, 0, &info)) return GH_EVENT_BOUNDS;
    uint16_t source;
    s->frontier = 0x2c4c58;
    gh_game_event_status status = el_kind(info, &source);
    if (status) return status;
    uint8_t *row = s->work.pointer;
    put(row + 4, source);
    row[8] = actor.bytes[8] = info[4];

    // Actual EV01 first EL00. REL+04D0 selects one of three costume remaps
    // from the jump table at CB10. REL+07B8 has no EL replacement.
    s->frontier = 0x297a48;
    if (source != 0x600) return GH_EVENT_UNSUPPORTED;
    const unsigned costume = gh_core_save_costume(b->save);
    const uint16_t create = costume == 2 || costume == 3 ? 0x6f0 :
                            costume == 4 || costume == 5 ? 0x6f2 :
                            costume == 6 || costume == 7 ? 0x6f6 : source;
    put(row + 0x40, create);
    gh_event_actor_create_request request = {
        .info = info, .work = row, .data = b->data, .source_kind = source,
        .create_kind = create, .resource_kind = source, .create_argument = 0xffff
    };
    s->frontier = 0x1fea00;
    const gh_object_status lookup = gh_game_objects_find(b->objects, source, &request.loaded_resource);
    if (lookup != GH_OBJECT_OK && lookup != GH_OBJECT_MISSING) return GH_EVENT_CALLEE;
    const uint16_t flags = half(row + 0x24) | (lookup == GH_OBJECT_MISSING ? 5 : 1);
    row[0x24] = (uint8_t)flags;
    row[0x25] = (uint8_t)(flags >> 8);
    s->pending = request;
    s->frontier = 0x312a80;
    return GH_EVENT_UNSUPPORTED;
}

static void create_task(void *context, int32_t argument) {
    gh_event_actor_create *s = context;
    s->task_entered = 1;
    s->task_error = argument == -1 ? first_request(s) : GH_EVENT_ARGUMENT;
    // Preserve the suspended task and its work at the native factory boundary.
    while (gh_task_threads_sleep(s->bindings.global_threads, 1) == GH_THREAD_OK) {}
    s->task_live = 0;
}

gh_game_event_status gh_event_actor_create_start_002979f0(
    gh_event_actor_create *s, const gh_event_actor_create_bindings *b, uint32_t *frontier) {
    if (frontier) *frontier = 0x297a1c;
    if (!s || !b || !b->data || b->data->components != b->components || !b->data->actor_cuts_bound ||
        !b->heap || !b->pools || !b->objects || !b->global_threads || !b->object_view ||
        !b->save.bytes || b->save.size < GH_CORE_SAVE_BYTES) return GH_EVENT_ARGUMENT;
    if (s->stage && !same(&s->bindings, b)) return GH_EVENT_ARGUMENT;
    gh_event_component_view actor;
    if (component(b->components, 2, 0xcf00, 0x1c, &actor) || word(actor.bytes + 4) != b->data->actor_count) {
        return GH_EVENT_ARGUMENT;
    }
    if (!s->stage) {
        const size_t bytes = (size_t)b->data->actor_count * 0x48;
        if (bytes && gh_heap_allocate(b->heap, bytes, 16, &s->work)) return GH_EVENT_CALLEE;
        if (bytes) memset(s->work.pointer, 0, bytes);
        put(actor.bytes + 0x14, 0); // Native allocation is s->work.
        s->bindings = *b;
        s->task_slot = -1;
        s->stage = 1;
    }
    if (s->stage == 1) {
        if (frontier) *frontier = 0x297a40;
        gh_game_event_status status = suspend_unneeded(s, frontier);
        if (status) return status;
        s->stage = 2;
    }
    if (s->stage == 2) {
        if (frontier) *frontier = 0x2d5470;
        int32_t slot = -1;
        if (gh_task_threads_start(b->global_threads, -1, create_task, s, -1, &slot) || slot < 0) return GH_EVENT_CALLEE;
        put(actor.bytes, word(actor.bytes) & ~2u);
        s->task_slot = slot;
        s->task_live = 1;
        s->stage = 3;
    }
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_create_abort(gh_event_actor_create *s) {
    if (!s) return GH_EVENT_ARGUMENT;
    if (s->task_live) {
        if (gh_task_threads_cancel(s->bindings.global_threads, s->task_slot)) return GH_EVENT_CALLEE;
        s->task_live = 0;
    }
    if (s->work.pointer && gh_heap_release(s->bindings.heap, s->work)) return GH_EVENT_CALLEE;
    memset(s, 0, sizeof(*s));
    return GH_EVENT_OK;
}
