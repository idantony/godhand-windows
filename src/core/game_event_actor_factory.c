#include "game_event_actor_factory.h"

#include <string.h>

static uint32_t word(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void put(uint8_t *p, uint32_t value) {
    memcpy(p, &value, sizeof(value));
}

static void vf0(uint8_t *p) {
    memset(p, 0, 12);
    put(p + 12, 0x3f800000);
}

static gh_game_event_status construct_003181a8(gh_event_actor_factory *s) {
    const gh_event_actor_factory_bindings *b = &s->bindings;
    if (word((const uint8_t *)b->work + 0x594) & 0x100000) return GH_EVENT_OK;
    gh_r207_allocation allocation = {0};
    gh_mem_error error = gh_heap_allocate(b->resources.heap, GH_EVENT_ACTOR_BYTES, 16, &allocation);
    if (error == GH_MEM_CAPACITY) return GH_EVENT_OK;
    if (error) return GH_EVENT_CALLEE;
    uint8_t *bytes = allocation.pointer;
    memset(bytes, 0, GH_EVENT_ACTOR_BYTES);
    gh_title_player_view view;
    if (gh_game_actor_construct_0014e990(bytes, GH_EVENT_ACTOR_BYTES, &view)) {
        (void)gh_heap_release(b->resources.heap, allocation);
        return GH_EVENT_CALLEE;
    }
    put(bytes + 0x214, 0x44b528);
    view.original_vtable214 = 0x44b528;
    view.initial_memory = GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;
    static const unsigned row_offsets[] = {0, 0x10, 0x30, 0x40, 0x50, 0x80};
    for (unsigned row = 0; row < 44; ++row) {
        for (unsigned i = 0; i < sizeof(row_offsets) / sizeof(row_offsets[0]); ++i) {
            vf0(bytes + 0x13a0 + row * 0xa0 + row_offsets[i]);
        }
    }
    static const unsigned tail_offsets[] = {0x2f80, 0x2fb0, 0x2fd0, 0x2fe0, 0x30c0, 0x30d0};
    for (unsigned i = 0; i < sizeof(tail_offsets) / sizeof(tail_offsets[0]); ++i) vf0(bytes + tail_offsets[i]);
    for (unsigned i = 0; i < 8; ++i) vf0(bytes + 0x3000 + i * 0x10);
    s->object = (gh_game_title_player){
        .allocation = allocation, .heap = b->resources.heap, .view = view, .ready = 1
    };
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_factory_bind(gh_event_actor_factory *s,
    const gh_event_actor_factory_bindings *b) {
    if (!s || !b || s->bound || !b->pools || !b->global_objects || !b->room_objects ||
        b->global_objects == b->room_objects || !b->work || !b->factory_serial || !b->resources.dvd ||
        !b->resources.dvd->disc || !b->resources.heap || !b->resources.threads) return GH_EVENT_ARGUMENT;
    s->bindings = *b;
    s->bound = 1;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_factory_run(void *opaque, const gh_event_actor_create_request *request,
    int *original_nonnull, uint32_t *frontier) {
    gh_event_actor_factory *s = opaque;
    if (frontier) *frontier = 0x312a80;
    if (!s || !s->bound || !request || !original_nonnull) return GH_EVENT_ARGUMENT;
    *original_nonnull = 0;
    const uint16_t kind = request->create_kind;
    const uint16_t source = request->source_kind;
    const int variant = (source == 0x607 && (kind == 0x691 || kind == 0x694 || kind == 0x697)) ||
                        (source == 0x605 && (kind == 0x690 || kind == 0x693 || kind == 0x696)) ||
                        (source == 0x600 && (kind == 0x6f0 || kind == 0x6f2 || kind == 0x6f6)) ||
                        (source == 0x602 && (kind == 0x6f1 || kind == 0x6f3 || kind == 0x6f7)) ||
                        (source == 0x60a && (kind == 0x609 || kind == 0x6f4));
    if (((source < 0x600 || source > 0x602) && (source < 0x60a || source > 0x60d) &&
        (source < 0x605 || source > 0x607) && source != 0x630 && source != 0x631 && source != 0x667) ||
        request->resource_kind != source ||
        request->create_argument != 0xffff || (kind != source && !variant)) {
        return GH_EVENT_UNSUPPORTED;
    }
    if (s->stage && (s->kind != kind || s->actor_index != request->actor_index)) return GH_EVENT_ARGUMENT;
    const gh_event_actor_factory_bindings *b = &s->bindings;
    if (!s->stage) {
        if (frontier) *frontier = 0x30fb70;
        const gh_room_vector *all, *objects;
        if (gh_room_registry_view(b->pools, 0x754c30, &all) ||
            gh_room_registry_view(b->pools, 0x754c58, &objects)) return GH_EVENT_ARGUMENT;
        if (all->count == all->capacity || objects->count == objects->capacity) {
            if (frontier) *frontier = 0;
            return GH_EVENT_OK;
        }
        if (frontier) *frontier = 0x3181a8;
        gh_game_event_status status = construct_003181a8(s);
        if (status) return status;
        if (!s->object.ready) {
            if (frontier) *frontier = 0;
            return GH_EVENT_OK;
        }
        s->kind = kind;
        s->actor_index = request->actor_index;
        uint8_t *p = s->object.view.bytes;
        p[0x2fc] = (*b->factory_serial)++;
        memcpy(p + 0x2fe, &kind, sizeof(kind));
        memcpy(p + 0x300, &kind, sizeof(kind));
        put(p + 0x2d0, 0x800000);
        s->stage = 1;
    }
    if (s->stage == 1) {
        if (frontier) *frontier = 0x1fea48;
        const gh_object_status result = gh_game_objects_acquire_001fea48(b->room_objects, kind, &s->resource);
        if (result == GH_OBJECT_OK) s->room_reference = 1;
        else if (result != GH_OBJECT_MISSING) return GH_EVENT_CALLEE;
        s->stage = s->resource.bytes ? 3 : 2;
    }
    if (s->stage == 2) {
        uint32_t child = 0;
        const gh_object_status result = gh_game_objects_load_wait_001fe8a0(&s->load, b->global_objects,
            kind, &b->resources, 1, &s->resource, &child);
        if (frontier) *frontier = child ? child : 0x1fe8a0;
        if (result) return GH_EVENT_CALLEE;
        if (!s->resource.bytes) {
            // Original 312B00 calls delObj at 312778 before returning NULL.
            if (frontier) *frontier = 0x312778;
            return GH_EVENT_UNSUPPORTED;
        }
        s->global_reference = 1;
        uint8_t *p = s->object.view.bytes;
        put(p + 0x250, word(p + 0x250) | 0x2000000);
        s->stage = 3;
    }
    if (s->stage == 3) {
        if (frontier) *frontier = 0x14b638;
        if (!s->resource.file_extent_known || !s->resource.file_extent ||
            s->resource.file_extent > s->resource.size || s->object.view.bytes[0x2b1]) return GH_EVENT_BOUNDS;
        // Constructor's mesh count is zero, so 312B00's even-mesh loop does not run.
        uint8_t *p = s->object.view.bytes;
        put(p + 0x250, word(p + 0x250) | 0x10000);
        s->stage = 4;
    }
    if (s->stage == 4) {
        if (frontier) *frontier = 0x30fcc0;
        if (gh_room_object_register(b->pools, &s->object, s, GH_ROOM_OBJECT_EVENT_ACTOR, 9, &s->handle)) {
            // Original registration failure enters the actual derived destructor.
            return GH_EVENT_UNSUPPORTED;
        }
        s->stage = 5;
    }
    *original_nonnull = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_factory_view(const gh_event_actor_factory *s, uint32_t handle,
    gh_title_player_view *view) {
    if (!s || !view || !handle || handle != s->handle || s->stage != 5) return GH_EVENT_ARGUMENT;
    void *object;
    if (gh_room_object_lookup(s->bindings.pools, handle, GH_ROOM_OBJECT_EVENT_ACTOR, s, &object) ||
        object != &s->object) return GH_EVENT_ARGUMENT;
    *view = s->object.view;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_factory_initialize_model(gh_event_actor_factory *s,
    const gh_event_actor_create_request *request, int *original_nonnull, uint32_t *frontier) {
    if (frontier) *frontier = 0x2b6768;
    if (!s || !request || !original_nonnull || s->stage != 5 || request->actor_index != s->actor_index ||
        request->create_kind != s->kind) return GH_EVENT_ARGUMENT;
    *original_nonnull = 0;
    uint8_t *p = s->object.view.bytes;
    if (p[0x4d0]) return GH_EVENT_UNSUPPORTED;

    if (!s->init_stage) {
        if (frontier) *frontier = 0x31ccf0;
        gh_bundle bundle;
        if (gh_bundle_open(s->resource.bytes, s->resource.file_extent, &bundle)) return GH_EVENT_BOUNDS;
        gh_bundle_error geometry = gh_bundle_find(&bundle, "MD", 0, &s->geometry);
        if (geometry != GH_BUNDLE_OK && geometry != GH_BUNDLE_NOT_FOUND) return GH_EVENT_BOUNDS;
        gh_bundle_error texture = gh_bundle_find(&bundle, "TM3", 0, &s->texture);
        if (texture == GH_BUNDLE_NOT_FOUND) texture = gh_bundle_find(&bundle, "T32", 0, &s->texture);
        if (texture != GH_BUNDLE_OK && texture != GH_BUNDLE_NOT_FOUND) return GH_EVENT_BOUNDS;
        // 2B6688: EL00 and its three variants are in 44B378. EL01 and
        // EL02 (+variants) are absent from all three tables and take profile 0.
        const unsigned profile = request->source_kind == 0x600 || request->source_kind == 0x605 ||
            s->kind == 0x60a || s->kind == 0x6f4 ||
            s->kind == 0x630 || s->kind == 0x631 ? 3 : 0;
        put(p + 0x4dc, profile);
        if (profile == 3) {
            if (frontier) *frontier = 0x14ef50;
            if (gh_game_actor_shadow_initialize_0014ef50(&s->shadow, &s->object.view, 0x4f0, 0x3c3f68)) {
                return GH_EVENT_CALLEE;
            }
            put(p + 0x4a8, word(p + 0x4a8) | 0x20000000u);
        }
        put(p + 0x254, word(p + 0x254) | 0x08000000u);
        s->init_stage = 1;
    }
    if (!s->geometry.bytes) {
        if (frontier) *frontier = 0;
        return GH_EVENT_OK;
    }
    if (s->init_stage == 1) {
        if (frontier) *frontier = 0x149738;
        if (!s->bindings.packet_heap || !s->bindings.fog_rgb ||
            gh_actor_model_bind_packet_inputs(&s->model, s->bindings.packet_heap, s->bindings.fog_rgb)) {
            return GH_EVENT_UNSUPPORTED;
        }
        int original = 0;
        gh_actor_model_status status = gh_actor_model_initialize_00149738(&s->model, &s->object.view,
            s->bindings.work, s->bindings.pools, &s->geometry, &s->texture, 0, 0,
            GH_STATIC_BINARY32_REFERENCE, &original);
        if (status) {
            if (frontier && s->model.frontier) *frontier = s->model.frontier;
            return status == GH_ACTOR_MODEL_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE;
        }
        if (!original) {
            if (frontier) *frontier = 0;
            return GH_EVENT_OK;
        }
        s->init_stage = 2;
    }
    *original_nonnull = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_factory_initialize(void *opaque,
    const gh_event_actor_create_request *request, uint32_t *handle, uint32_t *frontier) {
    gh_event_actor_factory *s = opaque;
    if (!handle) return GH_EVENT_ARGUMENT;
    *handle = 0;
    int original = 0;
    gh_game_event_status model = gh_event_actor_factory_initialize_model(s, request, &original, frontier);
    if (model || !original) return model;
    uint8_t *p = s->object.view.bytes;
    if (s->init_stage == 2) {
        // REL task's VF0 XYZ, Adjust_theta(0), and its unmodified 1.0 scale.
        memset(s->object.view.alias_f0, 0, 12);
        memset(p + 0x490, 0, 12);
        memset(p + 0x100, 0, 12);
        for (unsigned i = 0; i < 3; ++i) put(p + 0x110 + i * 4, 0x3f800000);
        // R0_Init 2B6918.
        p[0x2f4] = 1;
        p[0x2f5] = p[0x2f6] = p[0x2f7] = 0;
        s->init_stage = 3;
    }
    if (s->init_stage == 3) {
        if (frontier) *frontier = 0x149eb8;
        gh_actor_model_status status = gh_actor_model_update_hierarchy_00149eb8(&s->model,
            GH_STATIC_BINARY32_REFERENCE);
        if (status) {
            if (frontier && s->model.frontier) *frontier = s->model.frontier;
            return status == GH_ACTOR_MODEL_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE;
        }
        put(p + 0x250, word(p + 0x250) | 0x8000u);
        put(p + 0x2f0, word(p + 0x2f0) | 0x1000u);
        s->init_stage = 4;
    }
    *handle = s->handle;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_factory_abort(gh_event_actor_factory *s) {
    if (!s) return GH_EVENT_ARGUMENT;
    const gh_event_actor_factory_bindings *b = &s->bindings;
    if (gh_actor_model_release(&s->model)) return GH_EVENT_CALLEE;
    if (s->handle) {
        if (gh_room_object_unregister(b->pools, s->handle, s)) return GH_EVENT_CALLEE;
        s->handle = 0;
    }
    if (s->room_reference) {
        if (gh_game_objects_release_reference_001ff0b0(b->room_objects, s->kind)) return GH_EVENT_CALLEE;
        s->room_reference = 0;
    }
    if (s->global_reference) {
        if (gh_game_objects_release_reference_001ff0b0(b->global_objects, s->kind)) return GH_EVENT_CALLEE;
        s->global_reference = 0;
    }
    if (gh_game_title_player_release(&s->object)) return GH_EVENT_CALLEE;
    memset(s, 0, sizeof(*s));
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_roster_bind(gh_event_actor_roster *s,
    const gh_event_actor_factory_bindings *b, gh_event_actor_factory *rows, size_t capacity) {
    if (!s || s->rows || !b || !rows || !capacity || capacity > 255) return GH_EVENT_ARGUMENT;
    for (size_t i = 0; i < capacity; ++i) if (rows[i].bound) return GH_EVENT_ARGUMENT;
    gh_game_event_status status = gh_event_actor_factory_bind(&rows[0], b);
    if (status) return status;
    *s = (gh_event_actor_roster){*b, rows, capacity};
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_roster_run(void *opaque, const gh_event_actor_create_request *request,
    int *nonnull, uint32_t *frontier) {
    gh_event_actor_roster *s = opaque;
    if (!s || !request || request->actor_index >= s->capacity) return GH_EVENT_ARGUMENT;
    gh_event_actor_factory *row = &s->rows[request->actor_index];
    if (!row->bound) {
        gh_game_event_status status = gh_event_actor_factory_bind(row, &s->bindings);
        if (status) return status;
    }
    return gh_event_actor_factory_run(row, request, nonnull, frontier);
}

gh_game_event_status gh_event_actor_roster_initialize(void *opaque, const gh_event_actor_create_request *request,
    uint32_t *handle, uint32_t *frontier) {
    gh_event_actor_roster *s = opaque;
    if (!s || !request || request->actor_index >= s->capacity) return GH_EVENT_ARGUMENT;
    return gh_event_actor_factory_initialize(&s->rows[request->actor_index], request, handle, frontier);
}

gh_game_event_status gh_event_actor_roster_abort(gh_event_actor_roster *s) {
    if (!s) return GH_EVENT_ARGUMENT;
    for (size_t i = s->capacity; i; --i) {
        gh_game_event_status status = gh_event_actor_factory_abort(&s->rows[i - 1]);
        if (status) return status;
    }
    *s = (gh_event_actor_roster){0};
    return GH_EVENT_OK;
}
