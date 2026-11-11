#include "game_event_actor_release.h"
#include "game_effect_create.h"

#include <string.h>

static uint16_t half(const uint8_t *p) {
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static uint32_t word(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void put(uint8_t *p, uint32_t value) {
    memcpy(p, &value, sizeof(value));
}

gh_object_status gh_event_actor_release_slot_002cb340(void *opaque, int32_t slot, uint32_t *frontier) {
    gh_sound_control *sound = opaque;
    if (frontier) *frontier = 0x2cb340;
    if (!sound || !sound->initialized || !sound->recent_initialized) return GH_OBJECT_ARGUMENT;
    if (slot != -1) return GH_OBJECT_UNSUPPORTED;
    // Active native voices have nonnegative bank IDs. 2CBE68 matches none.
    // 2CC4B8(-1) points at cSnd+1B0, then 2CFC98 replaces nonzero with 3.
    gh_sound_recent *alias = &sound->recent[10];
    if (alias->bank || alias->program) {
        alias->bank = 3;
        alias->program = 0;
    }
    if (frontier) *frontier = 0;
    return GH_OBJECT_OK;
}

static int same(const gh_event_actor_release_bindings *a, const gh_event_actor_release_bindings *b) {
    return a->creation == b->creation && a->roster == b->roster && a->extra == b->extra &&
        a->play == b->play && a->sound == b->sound && a->effects == b->effects;
}

gh_game_event_status gh_event_actor_quit_00001588(gh_event_actor_release *s,
    const gh_event_actor_release_bindings *b, uint32_t *frontier) {
    if (frontier) *frontier = 0x1588;
    if (!s || !b || !b->creation || !b->roster || !b->extra || !b->play || !b->sound ||
        (s->bound && !same(&s->bindings, b))) return GH_EVENT_ARGUMENT;
    if (s->quit) {
        if (frontier) *frontier = 0;
        return GH_EVENT_OK;
    }
    if (!b->play->initialized || b->play->active || b->play->bindings.creation != b->creation ||
        b->play->bindings.roster != b->roster || b->play->bindings.extra != b->extra ||
        b->creation->task_live || !b->creation->bindings.data || b->extra->finish_stage != 2 ||
        b->creation->actor_cursor != b->creation->bindings.data->actor_count) return GH_EVENT_ARGUMENT;
    for (size_t i = 0; i < b->creation->actor_cursor; ++i) {
        const uint8_t *row;
        size_t cut;
        if (gh_event_actor_create_work_at(b->creation, i, &row, &cut)) return GH_EVENT_BOUNDS;
        // REL3608 is jr ra / nop, so each original row quit leaves its state intact.
    }
    s->bindings = *b;
    s->bound = s->quit = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status delete_actor(gh_event_actor_factory *actor, gh_sound_control *sound,
    uint32_t *frontier) {
    if (frontier) *frontier = 0x312778;
    if (!actor->bound || (!actor->handle && actor->normal_release_stage < 3) || !actor->object.view.bytes ||
        actor->object.view.original_vtable214 != 0x44b528 || actor->model.external_borrowers ||
        actor->model.external_owner || actor->model.external_parts || actor->model.sound_borrowers) {
        return GH_EVENT_ARGUMENT;
    }
    const gh_event_actor_factory_bindings *b = &actor->bindings;
    if (!actor->normal_release_stage && (actor->room_reference || actor->global_reference)) {
        if (frontier) *frontier = 0x1fe9b8;
        gh_game_objects *objects = actor->room_reference ? b->room_objects : b->global_objects;
        if (!actor->room_reference) {
            gh_object_view view;
            const gh_object_status alternate = gh_game_objects_find(b->room_objects, actor->kind, &view);
            if (alternate != GH_OBJECT_MISSING) return GH_EVENT_UNSUPPORTED;
        }
        int ready = 0;
        uint32_t child = 0;
        if (gh_game_objects_check_ready_001ff238(objects, actor->kind, &b->resources, &ready, &child) || !ready) {
            if (child && frontier) *frontier = child;
            return GH_EVENT_CALLEE;
        }
        if (gh_game_objects_release_event_001fed78(objects, actor->kind, actor->resource.bytes,
            gh_event_actor_release_slot_002cb340, sound, &child)) {
            if (frontier) *frontier = child ? child : 0x1fed78;
            return GH_EVENT_CALLEE;
        }
        actor->room_reference = actor->global_reference = 0;
    }
    if (!actor->normal_release_stage) actor->normal_release_stage = 1;
    if (actor->normal_release_stage == 1) {
        if (frontier) *frontier = 0x1496c0;
        if (gh_actor_model_release(&actor->model)) return GH_EVENT_CALLEE;
        actor->normal_release_stage = 2;
    }
    if (frontier) *frontier = 0x30f5a0;
    if (actor->normal_release_stage == 2) {
        if (gh_room_object_unregister(b->pools, actor->handle, actor)) return GH_EVENT_CALLEE;
        actor->handle = 0;
        actor->normal_release_stage = 3;
    }
    if (gh_game_title_player_release(&actor->object)) return GH_EVENT_CALLEE;
    memset(actor, 0, sizeof(*actor));
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status check_effect_borrows(const gh_event_actor_release *s,
    const gh_event_actor_factory *actor, uint32_t *frontier) {
    if (!s->bindings.effects) return GH_EVENT_OK;
    if (frontier) *frontier = 0x312778;
    int borrowed;
    if (gh_effect_create_actor_has_live_refs(s->bindings.effects, &actor->object.view, &borrowed))
        return GH_EVENT_CALLEE;
    return borrowed ? GH_EVENT_UNSUPPORTED : GH_EVENT_OK;
}

static gh_game_event_status release_associated(gh_event_actor_release *s, size_t index, uint32_t *frontier) {
    gh_event_actor_extra *extra = s->bindings.extra;
    if (index == 0 && extra->retargeted) {
        if (frontier) *frontier = 0x100ed8;
        gh_game_player_child *child = extra->bindings.child;
        if (!extra->previous.attached || !extra->previous.parent || !extra->previous.parent_model ||
            !child || extra->previous.parent_model->root.bytes != extra->bindings.player->bytes ||
            gh_game_player_child_attach_00100ed8(extra->bindings.attachment, child, extra->previous.parent,
                extra->previous.parent_model, 17, extra->bindings.save)) return GH_EVENT_UNSUPPORTED;
        uint8_t *p = child->object.view.bytes;
        put(p + 0x2f0, word(p + 0x2f0) | 1);
        put(p + 0x250, word(p + 0x250) & ~0x10000u);
        extra->retargeted = 0;
    }
    if (index == 0 && extra->weapon) {
        if (frontier) *frontier = 0x1cf160;
        return GH_EVENT_UNSUPPORTED; // Original release reattaches a positive weapon to the player.
    }
    if (index == extra->olivia_accessory.actor_index && extra->accessory_handle) {
        gh_game_event_status status = check_effect_borrows(s, &extra->olivia_accessory, frontier);
        if (status) return status;
        if (frontier) *frontier = 0x312778;
        if (gh_actor_model_clear_external_parts(&extra->accessory_parts)) return GH_EVENT_CALLEE;
        status = delete_actor(&extra->olivia_accessory, s->bindings.sound, frontier);
        if (status) return status;
        extra->accessory_handle = 0;
    }
    // Original row deletion can leave soon-to-be-deleted children with a
    // stale guest parent. Remove only the matching native borrows before free.
    gh_actor_model *target = &s->bindings.roster->rows[index].model;
    for (size_t i = 0; i < sizeof(extra->row_parts) / sizeof(*extra->row_parts); ++i) {
        gh_actor_external_parent_set *link = &extra->row_parts[i];
        if (link->count && (link->child == target || link->parent == target) &&
            gh_actor_model_clear_external_parts(link)) return GH_EVENT_CALLEE;
    }
    if (extra->pendulum_parent == target) extra->pendulum_parent = NULL;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_start_release_00297a88(gh_event_actor_release *s, uint32_t *frontier) {
    if (frontier) *frontier = 0x297a88;
    if (!s || !s->quit || !s->bindings.creation->bindings.data) return GH_EVENT_ARGUMENT;
    if (s->release_started) {
        if (frontier) *frontier = 0;
        return GH_EVENT_OK;
    }
    gh_event_actor_create *creation = s->bindings.creation;
    for (; s->row < creation->actor_cursor; ++s->row) {
        const uint8_t *row;
        size_t cut;
        if (gh_event_actor_create_work_at(creation, s->row, &row, &cut)) return GH_EVENT_BOUNDS;
        const uint32_t handle = creation->actor_handles[s->row];
        if (!handle) continue;
        gh_event_actor_factory *actor = &s->bindings.roster->rows[s->row];
        if (actor->handle != handle && actor->normal_release_stage < 3) return GH_EVENT_ARGUMENT;
        if (!(row[0x24] & 1)) {
            uint8_t *p = actor->object.view.bytes;
            p[0x2f4] = 1;
            p[0x2f5] = p[0x2f6] = p[0x2f7] = 0;
            continue;
        }
        gh_game_event_status status = check_effect_borrows(s, actor, frontier);
        if (status) return status;
        status = release_associated(s, s->row, frontier);
        if (status) return status;
        if (word(row + 0x40) != actor->kind) return GH_EVENT_ARGUMENT;
        // Original setId before the virtual destructor, including costume variants.
        uint8_t *p = actor->object.view.bytes;
        p[0x2fe] = (uint8_t)actor->kind;
        p[0x2ff] = (uint8_t)(actor->kind >> 8);
        put(p + 0x2d0, 0x800000);
        status = delete_actor(actor, s->bindings.sound, frontier);
        if (status) return status;
        creation->actor_handles[s->row] = 0;
        if (!s->row) creation->first_actor_handle = 0;
        gh_event_actor_play *play = s->bindings.play;
        if (play->bindings.motions && s->row < play->bindings.motion_capacity) {
            play->bindings.motions[s->row] = (gh_actor_motion){0};
        }
    }
    s->release_started = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_wait_release_00297aa8(gh_event_actor_release *s,
    int *done, uint32_t *frontier) {
    if (frontier) *frontier = 0x297aa8;
    if (!s || !s->release_started || !done) return GH_EVENT_ARGUMENT;
    *done = 0;
    if (!s->waited) {
        gh_event_actor_create *creation = s->bindings.creation;
        for (size_t i = 0; i < creation->actor_cursor; ++i) {
            const uint8_t *row;
            size_t cut;
            if (gh_event_actor_create_work_at(creation, i, &row, &cut)) return GH_EVENT_BOUNDS;
            if (!(row[0x24] & 4)) continue;
            int unloaded;
            if (frontier) *frontier = 0x1fefd0;
            if (gh_game_objects_is_unloaded_001fefd0(creation->bindings.objects, word(row + 4), &unloaded)) {
                return GH_EVENT_CALLEE;
            }
            if (!unloaded) {
                if (frontier) *frontier = 0;
                return GH_EVENT_OK;
            }
        }
        s->waited = 1;
    }
    *done = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_release_work_00297ad0(gh_event_actor_release *s, uint32_t *frontier) {
    if (frontier) *frontier = 0x1428;
    if (!s || !s->waited) return GH_EVENT_ARGUMENT;
    if (s->complete) {
        if (frontier) *frontier = 0;
        return GH_EVENT_OK;
    }
    gh_event_actor_create *creation = s->bindings.creation;
    const gh_event_actor_create_bindings *b = &creation->bindings;
    if (!s->work_released) {
        gh_event_component_view actor;
        if (gh_event_component_at(b->components, 2, &actor) || actor.original_offset != 0xcf00) return GH_EVENT_ARGUMENT;
        if (creation->work.pointer && gh_heap_release(b->heap, creation->work)) return GH_EVENT_CALLEE;
        creation->work = (gh_r207_allocation){0};
        put(actor.bytes + 4, 0);
        s->work_released = 1;
    }
    if (frontier) *frontier = 0x0c60;
    gh_event_component_view env;
    if (gh_event_component_at(b->components, 0, &env) || env.original_offset != 0xd200) return GH_EVENT_ARGUMENT;
    if (!(word(env.bytes + 0x28) & 0x40000000u)) {
        const gh_room_vector *vector;
        if (gh_room_registry_view(b->pools, 0x754c30, &vector)) return GH_EVENT_ARGUMENT;
        for (; s->unsuspend_cursor < vector->count; ++s->unsuspend_cursor) {
            const uint8_t *entry = (const uint8_t *)vector->allocation.pointer + s->unsuspend_cursor * 4;
            gh_title_player_view object;
            uint32_t child = 0;
            gh_game_event_status status = b->object_view(b->context, word(entry), &object, &child);
            if (status) {
                if (child && frontier) *frontier = child;
                return status;
            }
            if (!object.bytes || object.size < 0x300) return GH_EVENT_BOUNDS;
            const uint16_t kind = half(object.bytes + 0x2fe);
            if (kind && (kind < 0x300 || kind >= 0x500 || kind == 0x376)) {
                put(object.bytes + 0x250, word(object.bytes + 0x250) & ~0x8000u);
            }
        }
    }
    s->complete = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}
