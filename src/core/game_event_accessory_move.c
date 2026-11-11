#include "game_event_accessory_move.h"

#include "game_actor_external_parent.h"
#include "game_model_texture_scroll.h"
#include "game_player_node_cache.h"
#include "game_scene_at_geometry.h"

#include <fenv.h>
#include <math.h>
#include <string.h>

static uint32_t word(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void store(uint8_t *p, uint32_t value) {
    memcpy(p, &value, sizeof(value));
}

static gh_game_event_status follow_parent(gh_actor_model *model,
    const gh_player_shared_vector *shared, uint32_t *frontier) {
    *frontier = 0x2b6fe8;
    uint8_t *p = model->root.bytes;
    gh_actor_external_parent_set *links = model->external_parts;
    static const uint8_t indices[3][7] = {{0, 1, 17, 18}, {0, 1, 2}, {0, 1, 2, 3, 4, 5, 6}};
    static const unsigned counts[] = {4, 3, 7};
    const unsigned mode = word(p + 0x30a4);
    if (mode != 1 && mode != 2 && mode != 4) return GH_EVENT_UNSUPPORTED;
    const unsigned profile = mode == 4 ? 2 : mode - 1;
    const unsigned count = counts[profile];
    if (!links || links->child != model || links->count != count || !links->parent || !links->parent->ready ||
        !shared || !shared->initialized || word(p + 0x30b4) != UINT32_MAX ||
        !model->root.alias_f0 || word(model->root.alias_f0 + 12) != 0x3f800000) return GH_EVENT_UNSUPPORTED;
    const gh_title_player_view *parent = &links->parent->root;
    if (!parent->bytes || parent->size < 0x254) return GH_EVENT_BOUNDS;

    const gh_actor_part *parts[7];
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *parent_part;
        const unsigned part = indices[profile][i];
        if (gh_actor_model_part_at(model, part, &parts[i]) || !parts[i]->alias_d0 ||
            gh_actor_model_external_parts_at(model, part, &parent_part) || !parent_part) return GH_EVENT_BOUNDS;
    }

    float matrix[16], offset[3], position[4];
    memcpy(matrix, parent->bytes + 0x80, sizeof(matrix));
    memcpy(offset, p + 0x30c0, sizeof(offset));
    if (gh_scene_at_apply_world_001508d8(matrix, offset, position)) {
        *frontier = 0x30a2b0;
        return GH_EVENT_CALLEE;
    }
    memcpy(model->root.alias_f0, position, sizeof(position));
    memcpy(p + 0x100, parent->bytes + 0x100, 12);
    for (unsigned i = 0; i < count; ++i) memcpy(parts[i]->alias_d0, shared->bytes, 12);
    for (unsigned i = 0; i < count; ++i) memcpy(parts[i]->bytes + 0xe0, shared->bytes, 12);
    memcpy(p + 0x240, parent->bytes + 0x240, 16);
    store(p + 0x250, (word(p + 0x250) & ~0x10u) | (word(parent->bytes + 0x250) & 0x10u));
    return GH_EVENT_OK;
}

/* 002B6E54/002B6EFC DIV.S, then CVT.W.S. Only integers 1 and 2 select
 * a different mesh. Keep the binary32 chop reference independent of caller FEnv. */
static unsigned hand_variant(float angle) {
    volatile float quotient = angle / 0x1.657184p-3f; // 3E32B8C2
    return quotient >= 1.0f && quotient < 3.0f ? (unsigned)quotient : 0;
}

static gh_game_event_status update_hands_002b6da0(gh_actor_model *model, uint32_t *frontier) {
    static const uint8_t names[2][3][8] = {
        {"RH_00", "RH_01", "RH_02"}, {"LH_00", "LH_01", "LH_02"}
    }; // 44B4D8..44B500
    *frontier = 0x2b6da0;
    if (model->root.bytes[0x2b4] != model->part_count) return GH_EVENT_BOUNDS;
    const int has_driver = model->part_count > 27;
    unsigned variants[2] = {0, 0};
    if (has_driver) {
        const gh_actor_part *driver;
        if (gh_actor_model_part_at(model, 27, &driver) || !driver->bytes) return GH_EVENT_BOUNDS;
        float x, z;
        memcpy(&x, driver->bytes + 0x100, sizeof(x));
        memcpy(&z, driver->bytes + 0x108, sizeof(z));
        if (!isfinite(x) || !isfinite(z)) return GH_EVENT_UNSUPPORTED;
        fenv_t environment;
        if (feholdexcept(&environment)) return GH_EVENT_CALLEE;
        if (fesetround(FE_TOWARDZERO)) {
            (void)fesetenv(&environment);
            return GH_EVENT_CALLEE;
        }
        variants[0] = hand_variant(x);
        variants[1] = hand_variant(z);
        if (fesetenv(&environment)) return GH_EVENT_CALLEE;
    }
    *frontier = 0x14a338;
    int found;
    for (unsigned hand = 0; hand < 2; ++hand) {
        for (unsigned mesh = 0; mesh < 3; ++mesh) {
            if (gh_actor_model_set_mesh_visible_0014a338(model, names[hand][mesh], 8,
                    !has_driver && mesh == 0, &found)) return GH_EVENT_CALLEE;
        }
        if (has_driver && gh_actor_model_set_mesh_visible_0014a338(model,
                names[hand][variants[hand]], 8, 1, &found)) return GH_EVENT_CALLEE;
    }
    return GH_EVENT_OK;
}

gh_game_event_status gh_game_event_simple_move_with_tails_002b6930(gh_event_accessory_move *s,
    gh_event_actor_factory *actor, const gh_player_shared_vector *shared,
    gh_event_simple_shadow shadow, void *shadow_context,
    gh_event_simple_tail tail, void *tail_context, uint32_t *frontier) {
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x2b6930;
    if (!s || !actor || !actor->model.ready ||
        !actor->model.root.bytes || actor->model.root.size < GH_EVENT_ACTOR_BYTES ||
        actor->model.root.bytes != actor->object.view.bytes ||
        actor->object.view.original_vtable214 != 0x44b528 || (s->actor && s->actor != actor)) return GH_EVENT_ARGUMENT;
    uint8_t *p = actor->model.root.bytes;
    if (!s->stage) {
        /* vtable 44B528 +5C is 14EA50. Its only nonempty cases are 1..5. */
        if (p[0x2f4] == 4 && p[0x2f5] >= 1 && p[0x2f5] <= 5) {
            static const uint32_t entries[] = {0x14ead8, 0x14eb90, 0x14ec88, 0x14ed60, 0x14ee60};
            *frontier = entries[p[0x2f5] - 1];
            return GH_EVENT_UNSUPPORTED;
        }
        s->actor = actor;
        fenv_t environment;
        if (feholdexcept(&environment)) return GH_EVENT_CALLEE;
        if (fesetround(FE_TONEAREST)) {
            (void)fesetenv(&environment);
            return GH_EVENT_CALLEE;
        }
        gh_actor_model_status status = gh_game_model_texture_scroll_0014e4d0(&actor->model, 1, &s->texture_node, frontier);
        if (fesetenv(&environment)) return GH_EVENT_CALLEE;
        if (status) return status == GH_ACTOR_MODEL_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE;
        s->stage = 1;
    }
    if (s->stage == 1) {
        if (p[0x4e0]) {
            gh_game_event_status status = follow_parent(&actor->model, shared, frontier);
            if (status) return status;
        }
        s->stage = 2;
    }
    if (s->stage == 2) {
        *frontier = 0x149eb8;
        gh_actor_model_status status = gh_actor_model_update_hierarchy_00149eb8(&actor->model, GH_STATIC_BINARY32_REFERENCE);
        if (status) {
            if (actor->model.frontier) *frontier = actor->model.frontier;
            return status == GH_ACTOR_MODEL_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE;
        }
        s->stage = 3;
    }
    if (s->stage == 3) {
        if (word(p + 0x4dc)) {
            *frontier = 0x14efa8;
            if (!shadow) return GH_EVENT_UNSUPPORTED;
            gh_game_event_status status = shadow(shadow_context, actor, frontier);
            if (status) return status;
        }
        s->stage = 4;
    }
    static const struct { unsigned offset; uint32_t entry; } calls[] = {
        {0x1390,0x2df860}, {0x2fa0,0x2b6a40}, {0x2ff0,0x2b79e0}
    };
    for (; s->stage < 7; ++s->stage) {
        unsigned index = s->stage - 4;
        if (!p[calls[index].offset]) continue;
        *frontier = calls[index].entry;
        if (!tail) return GH_EVENT_UNSUPPORTED;
        gh_game_event_status status = tail(tail_context, actor, calls[index].entry, frontier);
        if (status) return status;
    }
    if (s->stage == 7) {
        if (p[0x4d8]) {
            gh_game_event_status status = update_hands_002b6da0(&actor->model, frontier);
            if (status) return status;
        }
        s->stage = 8;
    }
    if (!actor->model.root.alias_f0) return GH_EVENT_BOUNDS;
    if (p + 0x490 != actor->model.root.alias_f0)
        memcpy(p + 0x490, actor->model.root.alias_f0, 12);
    *s = (gh_event_accessory_move){0};
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_game_event_simple_move_002b6930(gh_event_accessory_move *s,
    gh_event_actor_factory *actor, const gh_player_shared_vector *shared,
    gh_event_simple_shadow shadow, void *shadow_context, uint32_t *frontier) {
    return gh_game_event_simple_move_with_tails_002b6930(s, actor, shared,
        shadow, shadow_context, NULL, NULL, frontier);
}

gh_game_event_status gh_game_event_accessory_move_002b6930(gh_event_accessory_move *s,
    gh_event_actor_factory *actor, const gh_player_shared_vector *shared, uint32_t *frontier) {
    if (!actor || actor->kind != 0x60d) {
        if (frontier) *frontier = 0x2b6930;
        return GH_EVENT_ARGUMENT;
    }
    return gh_game_event_simple_move_002b6930(s, actor, shared, NULL, NULL, frontier);
}
