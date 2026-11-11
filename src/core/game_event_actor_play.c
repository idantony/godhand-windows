#include "game_event_actor_play.h"
#include "game_actor_model_nodes.h"
#include "game_actor_motion_step.h"
#include "game_event_play_env.h"

#include <math.h>
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

static gh_game_event_status fail(gh_event_actor_play *s, uint32_t *frontier, uint32_t pc,
    int relative, gh_game_event_status status) {
    s->frontier_relative = (uint8_t)relative;
    if (frontier) *frontier = pc;
    return status;
}

static int same(const gh_event_actor_play_bindings *a, const gh_event_actor_play_bindings *b) {
    return a->creation == b->creation && a->roster == b->roster && a->extra == b->extra &&
        a->motions == b->motions && a->motion_capacity == b->motion_capacity && a->rate == b->rate &&
        a->ik_globals == b->ik_globals;
}

static gh_game_event_status environment(const gh_event_actor_play_bindings *b, gh_event_component_view *env) {
    if (!b || !b->creation || !b->roster || !b->extra || !b->extra->bound ||
        b->extra->bindings.creation != b->creation || b->extra->bindings.roster != b->roster ||
        !b->creation->bindings.data || b->roster->capacity < b->creation->bindings.data->actor_count ||
        gh_event_component_at(b->creation->bindings.components, 0, env) || env->original_offset != 0xd200 ||
        !env->bytes || env->size != 0x44) return GH_EVENT_ARGUMENT;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_play_init_00001468(gh_event_actor_play *s,
    const gh_event_actor_play_bindings *b, uint32_t *frontier) {
    if (!s) return GH_EVENT_ARGUMENT;
    gh_event_component_view env;
    int ready;
    if (environment(b, &env) || gh_event_actor_creation_poll_00297a68(b->creation->bindings.components, &ready) ||
        !ready || (s->initialized && !same(&s->bindings, b))) {
        return fail(s, frontier, 0x1468, 1, GH_EVENT_ARGUMENT);
    }
    for (size_t i = 0; i < b->creation->bindings.data->actor_count; ++i) {
        const uint8_t *row;
        size_t cut;
        if (gh_event_actor_create_work_at(b->creation, i, &row, &cut)) {
            return fail(s, frontier, 0x1468, 1, GH_EVENT_BOUNDS);
        }
        // Each original REL+2B60 call is jr ra / nop and touches no row fields.
    }
    s->bindings = *b;
    s->initialized = 1;
    return fail(s, frontier, 0, 0, GH_EVENT_OK);
}

static int select_primary(const gh_event_actor_cut_lane *lane, float frame, uint16_t cut_length,
    size_t *offset, float *selected_frame) {
    if (lane->sequence) return 0; // Positive alternate lane has its own original timing/loop branch.
    *offset = 0;
    *selected_frame = 0;
    if (!lane->primary) return 1;
    const uint16_t length = half(lane->primary + 4);
    if (!length) return 0;
    const float last = (float)length - 1.0f;
    if (frame <= last || cut_length <= length) {
        *offset = lane->primary_offset;
        *selected_frame = frame <= last ? frame : last;
    }
    return 1;
}

static gh_game_event_status hide_inactive(const gh_event_actor_extra *extra, size_t actor,
    uint8_t *object, const uint8_t *row) {
    put(object + 0x250, word(object + 0x250) | 0x8002u);
    const uint32_t associated = word(row + 0x30);
    if (!associated) return GH_EVENT_OK;
    if (associated == 0x101 && actor == 0) {
        if (extra->weapon) {
            const gh_title_player_view *weapon = extra->weapon->weapon;
            if (!weapon || !weapon->bytes || weapon->size < 0x254) return GH_EVENT_BOUNDS;
            put(weapon->bytes + 0x250, word(weapon->bytes + 0x250) | 0x8002u);
        }
        return GH_EVENT_OK;
    }
    if (associated == 0x60d && actor == extra->olivia_accessory.actor_index && extra->accessory_handle == extra->olivia_accessory.handle &&
        extra->accessory_handle && extra->olivia_accessory.object.view.bytes) {
        uint8_t *auxiliary = extra->olivia_accessory.object.view.bytes;
        put(auxiliary + 0x250, word(auxiliary + 0x250) | 0x8002u);
        return GH_EVENT_OK;
    }
    return GH_EVENT_UNSUPPORTED;
}

static int reset_pose(gh_actor_model *model, const uint8_t *row, int first_frame) {
    uint8_t *root = model->root.bytes;
    const int transition = model->first_node && first_frame && !(word(root + 0x250) & 0x8000u);
    root[0x2f4] = 4;
    root[0x2f5] = root[0x2f6] = root[0x2f7] = 0;
    if (!model->first_node) {
        put(root + 0x250, (word(root + 0x250) & ~0x8000u) | 2u);
    } else {
        if (first_frame) put(root + 0x2f0, word(root + 0x2f0) | 1u);
        put(root + 0x250, word(root + 0x250) & ~0x8002u);
    }
    memset(model->root.alias_d0, 0, 12);
    memset(root + 0x100, 0, 12);
    for (unsigned k = 0; k < 3; ++k) put(root + 0x110 + k * 4, 0x3f800000);
    for (unsigned i = 0; i < model->parts_allocated; ++i) {
        uint8_t *part = model->parts[i].bytes;
        if (i != 0x1d || !(row[0x24] & 8)) memset(part + 0x100, 0, 12);
        for (unsigned k = 0; k < 3; ++k) put(part + 0x110 + k * 4, 0x3f800000);
        memset(part + 0x130, 0, 12);
    }
    return transition;
}

static gh_game_event_status activate_related(gh_event_actor_play *s, gh_actor_model *model,
    const uint8_t *row, float frame, uint32_t *frontier) {
    const gh_event_actor_extra *extra = s->bindings.extra;
    const uint32_t associated = word(row + 0x30);
    if (!associated) return GH_EVENT_OK;
    if (associated == 0x60d && s->row == extra->olivia_accessory.actor_index && extra->accessory_handle == extra->olivia_accessory.handle &&
        extra->accessory_handle && extra->olivia_accessory.object.view.bytes) {
        uint8_t *p = extra->olivia_accessory.object.view.bytes;
        put(p + 0x250, word(p + 0x250) & ~0x8002u);
        return GH_EVENT_OK;
    }
    if (associated != 0x101 || s->row) return fail(s, frontier, 0x3810, 1, GH_EVENT_UNSUPPORTED);
    gh_game_player_child *child = extra->bindings.child;
    if (!child || !extra->bindings.attachment || child->model.external_parent != model ||
        child->model.external_owner != extra->bindings.attachment ||
        child->model.external_parent_index != 17) {
        return fail(s, frontier, 0x100ed8, 0, GH_EVENT_UNSUPPORTED);
    }
    uint8_t *p = child->object.view.bytes;
    if (frame == 0) {
        if (!s->cut) return fail(s, frontier, 0x3810, 1, GH_EVENT_UNSUPPORTED);
        gh_event_actor_cut previous;
        if (gh_event_actor_cut_at(s->bindings.creation->bindings.data, s->row, s->cut - 1, &previous)) {
            return fail(s, frontier, 0x3810, 1, GH_EVENT_BOUNDS);
        }
        const unsigned bit = s->rows[s->row].transition ? 2 : previous.lanes[0].primary ? 0 : 1;
        put(p + 0x2f0, word(p + 0x2f0) | bit);
    }
    put(p + 0x250, word(p + 0x250) & ~0x8002u);
    put(p + 0x20d4, 17);
    const unsigned costume = gh_core_save_costume(extra->bindings.save);
    put(p + 0x2088, costume == 2 || costume == 3 ? 11 : costume == 4 || costume == 5 ? 7 : 12);
    if (extra->weapon) {
        const gh_title_player_view *weapon = extra->weapon->weapon;
        if (!weapon || !weapon->bytes || weapon->size < 0x254) return GH_EVENT_BOUNDS;
        put(weapon->bytes + 0x250, word(weapon->bytes + 0x250) & ~0x8002u);
    }
    return GH_EVENT_OK;
}

static gh_game_event_status move_active(gh_event_actor_play *s, gh_actor_model *model,
    const uint8_t *row, float frame, uint32_t *frontier) {
    const gh_event_actor_play_bindings *b = &s->bindings;
    const gh_event_data_prefix *data = b->creation->bindings.data;
    gh_event_actor_play_row *play_row = &s->rows[s->row];
    uint8_t *p = model->root.bytes;
    if (s->row_stage == 2) {
        if (!b->motions || s->row >= b->motion_capacity || !b->rate) {
            return fail(s, frontier, 0x157568, 0, GH_EVENT_UNSUPPORTED);
        }
        gh_actor_motion *motion = &b->motions[s->row];
        const int same_motion = motion->bound && motion->motion.offset == play_row->primary_offset;
        if (!same_motion) {
            float selected;
            memcpy(&selected, &play_row->frame_bits, 4);
            if (selected >= 1 || (motion->bound && (motion->b.model != model ||
                motion->b.resource != data->data || motion->sequence_valid || motion->step_stage))) {
                return fail(s, frontier, 0x157568, 0, GH_EVENT_UNSUPPORTED);
            }
            *motion = (gh_actor_motion){0};
        }
        if (!motion->complete) {
            const gh_actor_motion_bindings binding = {
                .model = model, .resource = data->data, .resource_bytes = data->bytes, .work = model->work,
                .ik_globals = b->ik_globals, .save = b->creation->bindings.save, .save_binding_known = 1
            };
            gh_actor_motion_status status = gh_actor_motion_begin_initial_00157568(motion, &binding,
                play_row->primary_offset, SIZE_MAX);
            if (status) return fail(s, frontier, motion->frontier ? motion->frontier : 0x157568,
                0, status == GH_ACTOR_MOTION_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE);
        }
        if (motion->b.model != model || motion->b.resource != data->data) return GH_EVENT_ARGUMENT;
        put(p + 0x430, play_row->frame_bits);
        s->row_stage = 3;
    }
    if (s->row_stage == 3) {
        int ended;
        uint32_t pc = 0;
        gh_actor_motion_status status = gh_actor_motion_step_00157c18(&b->motions[s->row], *b->rate,
            GH_STATIC_BINARY32_REFERENCE, &ended, &pc);
        if (status) return fail(s, frontier, pc ? pc : 0x157c18, 0,
            status == GH_ACTOR_MOTION_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE);
        memcpy(model->root.alias_f0, p + 0x310, 12);
        memcpy(p + 0x100, p + 0x340, 12);
        memcpy(p + 0x110, p + 0x370, 12);
        memcpy(p + 0x320, p + 0x310, 12);
        put(p + 0x24c, 0x3f800000);
        memset(p + 0x330, 0, 12);
        s->row_stage = 4;
    }
    if (s->row_stage == 4) {
        if (model->first_node) {
            gh_game_event_status status = activate_related(s, model, row, frame, frontier);
            if (status) return status;
        }
        s->row_stage = 5;
    }
    if (s->row_stage == 5) {
        if (!(row[0x24] & 2)) {
            if (play_row->secondary_offset) return fail(s, frontier, 0x3258, 1, GH_EVENT_UNSUPPORTED);
            static const uint32_t tint[] = {0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000};
            if (gh_actor_model_set_all_node_tints(model, tint)) return fail(s, frontier, 0x3258, 1, GH_EVENT_CALLEE);
            put(p + 0x250, word(p + 0x250) & ~0x10u);
        }
        s->row_stage = 6;
    }
    uint32_t child = 0;
    gh_game_event_status status = gh_event_env_extra_move_object_00006b08(b->creation->bindings.components,
        &model->root, b->creation->bindings.save, &child);
    if (status) return fail(s, frontier, child ? child : 0x6b08, child ? 0 : 1, status);
    play_row->previous_present = 1;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_play_move_000014c8(gh_event_actor_play *s,
    const gh_event_actor_play_bindings *b, uint32_t *frontier) {
    if (!s) return GH_EVENT_ARGUMENT;
    gh_event_component_view env;
    if (!s->initialized || !b || !same(&s->bindings, b) || environment(b, &env)) {
        return fail(s, frontier, 0x14c8, 1, GH_EVENT_ARGUMENT);
    }
    if (word(env.bytes + 0x28) & 0x40000000u) return fail(s, frontier, 0, 0, GH_EVENT_OK);
    const gh_event_data_prefix *data = b->creation->bindings.data;
    const uint32_t cut = word(env.bytes + 0x10), bits = word(env.bytes + 0x14);
    float frame;
    memcpy(&frame, &bits, sizeof(frame));
    uint16_t length;
    if (!isfinite(frame) || frame < 0 || gh_event_camera_cut_length_00004e60(data, cut, &length) ||
        (s->active && (s->cut != cut || s->frame_bits != bits))) {
        return fail(s, frontier, 0x14c8, 1, GH_EVENT_ARGUMENT);
    }
    if (!s->active) {
        gh_game_player_child *child = b->extra->bindings.child;
        if (child) {
            if (!child->object.view.bytes || child->object.view.size < 0x254) {
                return fail(s, frontier, 0x2db6c0, 0, GH_EVENT_BOUNDS);
            }
            uint8_t *p = child->object.view.bytes;
            put(p + 0x250, word(p + 0x250) | 2u);
        }
        s->cut = cut;
        s->frame_bits = bits;
        s->active = 1;
    }
    for (; s->row < data->actor_count; ++s->row) {
        const uint32_t handle = b->creation->actor_handles[s->row];
        if (!handle) continue;
        gh_event_actor_factory *actor = &b->roster->rows[s->row];
        gh_title_player_view view;
        if (!actor->model.ready || gh_event_actor_factory_view(actor, handle, &view) ||
            actor->model.root.bytes != view.bytes) return fail(s, frontier, 0x2b68, 1, GH_EVENT_ARGUMENT);
        const uint8_t *read_row;
        size_t row_offset;
        if (gh_event_actor_create_work_at(b->creation, s->row, &read_row, &row_offset)) {
            return fail(s, frontier, 0x2b68, 1, GH_EVENT_BOUNDS);
        }
        uint8_t *row = (uint8_t *)b->creation->work.pointer + s->row * 0x48;
        gh_event_actor_play_row *play_row = &s->rows[s->row];
        if (!s->row_stage) {
            gh_event_actor_cut record;
            if (gh_event_actor_cut_at(data, s->row, cut, &record)) {
                return fail(s, frontier, 0x2c20, 1, GH_EVENT_BOUNDS);
            }
            float selected, second;
            if (!select_primary(&record.lanes[0], frame, length, &play_row->primary_offset, &selected)) {
                return fail(s, frontier, 0x2c20, 1, GH_EVENT_UNSUPPORTED);
            }
            if (!select_primary(&record.lanes[1], frame, length, &play_row->secondary_offset, &second)) {
                return fail(s, frontier, 0x2dd0, 1, GH_EVENT_UNSUPPORTED);
            }
            memcpy(&play_row->frame_bits, &selected, 4);
            memcpy(&play_row->secondary_frame_bits, &second, 4);
            row[9] = play_row->primary_offset ? 0 : 0xff;
            const uint16_t selected_cut = play_row->primary_offset ? (uint16_t)cut : 0;
            memcpy(row + 0x0a, &selected_cut, sizeof(selected_cut));
            put(row + 0x0c, 0);
            put(row + 0x10, play_row->frame_bits);
            put(row + 0x1c, play_row->secondary_frame_bits);
            s->row_stage = 1;
        }
        if (play_row->primary_offset) {
            if (s->row_stage == 1) {
                play_row->transition = (uint8_t)reset_pose(&actor->model, read_row, frame == 0);
                s->row_stage = 2;
            }
            gh_game_event_status status = move_active(s, &actor->model, read_row, frame, frontier);
            if (status) return status;
        } else {
            gh_game_event_status status = hide_inactive(b->extra, s->row, view.bytes, read_row);
            if (status) return fail(s, frontier, 0x3810, 1, status);
            play_row->previous_present = 0;
        }
        s->row_stage = 0;
    }
    s->row = 0;
    s->active = 0;
    return fail(s, frontier, 0, 0, GH_EVENT_OK);
}
