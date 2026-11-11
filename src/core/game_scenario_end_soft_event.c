#include "game_scenario_end_soft_event.h"

static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void and_word(uint8_t *p, uint32_t mask) {
    for (unsigned i = 0; i < 4; ++i) p[i] &= (uint8_t)(mask >> (i * 8));
}

static gh_game_event_status call(gh_soft_end_leaf leaf, void *context,
    uint32_t address, uint32_t *frontier) {
    *frontier = address;
    if (!leaf) return GH_EVENT_UNSUPPORTED;
    uint32_t child = 0;
    gh_game_event_status status = leaf(context, &child);
    if (status && child) *frontier = child;
    return status;
}

static gh_game_event_status player_call(const gh_soft_end_bindings *b,
    gh_soft_end_player_leaf leaf, uint32_t address, uint32_t *frontier) {
    *frontier = 0x2db6b0;
    const gh_title_player_view *p = b->current_player;
    if (!p || !p->bytes || p->size < GH_TITLE_PLAYER_BYTES) return GH_EVENT_UNSUPPORTED;
    *frontier = address;
    if (address == 0x12c7d0 && (p->original_vtable214 != 0x41d220 ||
        word(p->bytes + 0x214) != 0x41d220)) {
        *frontier = 0x2c3df4;
        return GH_EVENT_UNSUPPORTED;
    }
    if (!leaf) return GH_EVENT_UNSUPPORTED;
    uint32_t child = 0;
    gh_game_event_status status = leaf(b->context, p, &child);
    if (status && child) *frontier = child;
    return status;
}

enum {
    PREFIX, KIND, CAMERA_FLAGS, CAMERA_POSE, CAMERA_SWITCH,
    PLAYER_CAMERA_POSE, PLAYER_CAMERA_SWITCH, CAMERA_FINISH,
    ENEMIES, OBJECTS, PLAYER, TASKS, COMPLETE
};

gh_game_event_status gh_scenario_end_soft_event_002c3c10(
    gh_soft_event_end *s, const gh_soft_end_bindings *b, uint32_t *frontier) {
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x2c3c10;
    if (!s || !b || !b->scenario.bytes || b->scenario.size < 0xea || s->stage > COMPLETE)
        return GH_EVENT_ARGUMENT;
    uint8_t *scenario = b->scenario.bytes;
    if (s->stage && (scenario != s->scenario.bytes ||
        b->scenario.size != s->scenario.size || b->scenario.manager != s->scenario.manager ||
        b->work != s->work || b->current_player != s->player))
        return GH_EVENT_ARGUMENT;
    if (s->stage == PREFIX) {
        const unsigned depth = scenario[0xe8] | (unsigned)scenario[0xe9] << 8;
        if (depth == 1 && (!b->work || !b->scenario.manager)) return GH_EVENT_ARGUMENT;
        s->scenario = b->scenario;
        s->work = b->work;
        s->player = b->current_player;
        if (depth && depth < 0x8000u) {
            const unsigned remaining = depth - 1;
            scenario[0xe8] = (uint8_t)remaining;
            scenario[0xe9] = (uint8_t)(remaining >> 8);
        }
        if (depth == 1) {
            uint8_t *work = (uint8_t *)b->work;
            s->kind = word(scenario + 0xe0);
            and_word(work + 0x5d8, 0x012bffffu);
            and_word(work + 0x5e4, s->kind == 4 ? 0xbaf6ffbfu : 0xbafeffbfu);
            and_word(work + 0x5e0, s->kind <= 3 ? 0xfdefffffu : 0xffefffffu);
            s->stage = KIND;
        } else {
            s->stage = COMPLETE;
        }
    }

    while (s->stage < COMPLETE) {
        gh_game_event_status status = GH_EVENT_OK;
        switch (s->stage) {
        case KIND:
            if (s->kind <= 3)
                status = call(b->effect_unsuspend_002fa470, b->context, 0x2fa470, frontier);
            else if (s->kind == 4)
                status = player_call(b, b->player_clear_0012c418, 0x12c418, frontier);
            if (status) return status;
            if (s->kind == 2 || s->kind > 5) {
                s->stage = ENEMIES;
                continue;
            }
            break;
        case CAMERA_FLAGS:
            *frontier = 0x2c3fc0;
            if (b->scenario.size <= 0x111) return GH_EVENT_BOUNDS;
            and_word((uint8_t *)b->work + 0x5d8, 0xffdfffffu);
            break;
        case CAMERA_POSE:
        case PLAYER_CAMERA_POSE:
            /* 124F60(0,1) calls the same pose setter unconditionally. */
            if (s->stage == PLAYER_CAMERA_POSE &&
                (!b->current_player || !b->current_player->bytes ||
                 b->current_player->size < GH_TITLE_PLAYER_BYTES)) {
                *frontier = 0x2db6b0;
                return GH_EVENT_UNSUPPORTED;
            }
            status = call(b->camera_pose_0013b9b8, b->context, 0x13b9b8, frontier);
            break;
        case CAMERA_SWITCH:
        case PLAYER_CAMERA_SWITCH:
            status = call(b->camera_switch_00138d40, b->context, 0x138d40, frontier);
            break;
        case CAMERA_FINISH:
            scenario[0x111] = 0;
            break;
        case ENEMIES:
            status = call(b->enemies_unsuspend_002948e8, b->context, 0x2948e8, frontier);
            break;
        case OBJECTS:
            *frontier = 0x2c5068;
            if (!b->room_objects) return GH_EVENT_UNSUPPORTED;
            if (b->room_objects->work != b->work) return GH_EVENT_ARGUMENT;
            status = gh_event_hide_scenario_objects_002c5068(b->room_objects, 0, frontier);
            if (status && !*frontier) *frontier = 0x2c5068;
            break;
        case PLAYER:
            status = player_call(b, b->player_unsuspend_0012c7d0, 0x12c7d0, frontier);
            break;
        case TASKS: {
            *frontier = 0x2c3dfc;
            gh_task_manager *manager = b->scenario.manager;
            if (manager->count > manager->capacity) return GH_EVENT_ARGUMENT;
            if (manager->current_index >= 0) {
                if (!manager->records || (size_t)manager->current_index >= manager->count ||
                    manager->current != &manager->records[manager->current_index])
                    return GH_EVENT_ARGUMENT;
                manager->current->attributes &= 0xfeu;
            }
            manager->flags &= ~1u;
            break;
        }
        }
        if (status) return status;
        ++s->stage;
    }
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_soft_player_unsuspend_0012c7d0(
    const gh_title_player_view *player, const gh_soft_player_suspend_links *links,
    uint32_t *frontier) {
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x12c7d0;
    if (!player || !player->bytes || player->size < GH_TITLE_PLAYER_BYTES ||
        !links || player->original_vtable214 != 0x41d220 ||
        word(player->bytes + 0x214) != 0x41d220)
        return GH_EVENT_ARGUMENT;

    const gh_title_player_view *const related[] = {
        links->child_5f8, links->object_698, links->object_6a0, links->weapon_6a8
    };
    static const uint32_t call_sites[] = {0x12c858, 0x12c878, 0x12c898, 0x12c8b8};
    static const unsigned slots[] = {0, 0x698, 0x6a0, 0x6a8};
    for (unsigned i = 0; i < 4; ++i) {
        *frontier = call_sites[i];
        if (!(links->known_mask & (1u << i))) return GH_EVENT_UNSUPPORTED;
        const gh_title_player_view *object = related[i];
        if (!object) {
            if (slots[i] && word(player->bytes + slots[i])) return GH_EVENT_UNSUPPORTED;
            continue;
        }
        /* 41B6A0 descriptor at +60: adjust 0, +64: 14E930. */
        if (!object->bytes || object->size < 0x254 ||
            object->original_vtable214 != 0x41b6a0 ||
            word(object->bytes + 0x214) != 0x41b6a0)
            return GH_EVENT_UNSUPPORTED;
    }

    and_word(player->bytes + 0x250, 0xffff7fffu);
    and_word(player->bytes + 0x5a0, 0xfffffff7u);
    for (unsigned i = 0; i < 4; ++i)
        if (related[i]) and_word(related[i]->bytes + 0x250, 0xffff7fffu);
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_soft_enemies_unsuspend_empty_002948e8(
    const gh_game_init_state *init, uint32_t *frontier) {
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x2948e8;
    if (!init) return GH_EVENT_ARGUMENT;
    if (init->enemy_top || word(init->enemies + 0x500))
        return GH_EVENT_UNSUPPORTED;
    *frontier = 0;
    return GH_EVENT_OK;
}
