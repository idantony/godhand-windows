#include "game_event_accessory_move.h"

#include "game_actor_external_parent.h"
#include "game_model_texture_scroll.h"
#include "game_scene_at_geometry.h"

#include <fenv.h>
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
    if (!links || links->child != model || links->count != 7 || !links->parent || !links->parent->ready ||
        !shared || !shared->initialized || word(p + 0x30a4) != 4 || word(p + 0x30b4) != UINT32_MAX ||
        !model->root.alias_f0 || word(model->root.alias_f0 + 12) != 0x3f800000) return GH_EVENT_UNSUPPORTED;
    const gh_title_player_view *parent = &links->parent->root;
    if (!parent->bytes || parent->size < 0x254) return GH_EVENT_BOUNDS;

    const gh_actor_part *parts[7];
    for (unsigned i = 0; i < 7; ++i) {
        const uint8_t *parent_part;
        if (gh_actor_model_part_at(model, i, &parts[i]) || !parts[i]->alias_d0 ||
            gh_actor_model_external_parts_at(model, i, &parent_part) || !parent_part) return GH_EVENT_BOUNDS;
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
    for (unsigned i = 0; i < 7; ++i) memcpy(parts[i]->alias_d0, shared->bytes, 12);
    for (unsigned i = 0; i < 7; ++i) memcpy(parts[i]->bytes + 0xe0, shared->bytes, 12);
    memcpy(p + 0x240, parent->bytes + 0x240, 16);
    store(p + 0x250, (word(p + 0x250) & ~0x10u) | (word(parent->bytes + 0x250) & 0x10u));
    return GH_EVENT_OK;
}

gh_game_event_status gh_game_event_accessory_move_002b6930(gh_event_accessory_move *s,
    gh_event_actor_factory *actor, const gh_player_shared_vector *shared, uint32_t *frontier) {
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x2b6930;
    if (!s || !actor || actor->kind != 0x60d || !actor->model.ready ||
        !actor->model.root.bytes || actor->model.root.size < GH_EVENT_ACTOR_BYTES ||
        actor->model.root.bytes != actor->object.view.bytes || (s->actor && s->actor != actor)) return GH_EVENT_ARGUMENT;
    uint8_t *p = actor->model.root.bytes;
    if (!s->stage) {
        if (p[0x2f4] == 4) {
            *frontier = 0x2b6968;
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
        if (status) return status == GH_ACTOR_MODEL_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE;
        s->stage = 3;
    }
    if (word(p + 0x4dc)) *frontier = 0x14efa8;
    else if (p[0x1390]) *frontier = 0x2df860;
    else if (p[0x2fa0]) *frontier = 0x2b6a40;
    else if (p[0x2ff0]) *frontier = 0x2b79e0;
    else if (p[0x4d8]) *frontier = 0x2b6da0;
    else {
        if (!actor->model.root.alias_f0) return GH_EVENT_BOUNDS;
        if (p + 0x490 != actor->model.root.alias_f0) memcpy(p + 0x490, actor->model.root.alias_f0, 12);
        *s = (gh_event_accessory_move){0};
        *frontier = 0;
        return GH_EVENT_OK;
    }
    return GH_EVENT_UNSUPPORTED;
}
