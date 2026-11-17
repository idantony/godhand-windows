#include "game_event_actor_extra.h"
#include "game_actor_external_parent.h"
#include "game_player_node_cache.h"

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

gh_game_event_status gh_event_actor_extra_bind(gh_event_actor_extra *s, const gh_event_actor_extra_bindings *b) {
    if (!s || !b || s->bound || !b->factory || !b->factory->bound || !b->player || !b->player->bytes ||
        b->player->size < 0x6ac || half(b->player->bytes + 0x2fe) != 0x100 || !b->attachment ||
        !b->weapon_slot || !b->save.bytes || b->save.size < GH_CORE_SAVE_BYTES) return GH_EVENT_ARGUMENT;
    s->bindings = *b;
    s->bound = 1;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_extra_attach_jacket_00100ed8(gh_event_actor_extra *s, gh_actor_model *model) {
    if (!s || !s->bound || !model || !s->bindings.child) return GH_EVENT_ARGUMENT;
    const gh_event_actor_extra_bindings *b = &s->bindings;
    gh_game_player_child *child = b->child;
    gh_player_child_attachment *attachment = b->attachment;
    void *identity = NULL;
    if (!child->complete || child->b.pools != model->pools || child->object.view.size < 0x20d8 ||
        child->model.root.bytes != child->object.view.bytes || half(child->object.view.bytes + 0x2fe) != 0x101 ||
        gh_room_object_lookup(model->pools, child->handle, GH_ROOM_OBJECT_PLAYER_CHILD, &child->object, &identity) ||
        identity != &child->object || (attachment->attached && (attachment->child != child ||
        attachment->parent_model != child->model.external_parent || child->model.external_owner != attachment ||
        !attachment->parent_model || (s->retargeted ? attachment->parent_model != s->jacket_parent :
            attachment->parent_model->root.bytes != b->player->bytes) ||
        attachment->part_index < 0 || (uint32_t)attachment->part_index != child->model.external_parent_index)) ||
        (!attachment->attached && child->model.external_owner)) return GH_EVENT_ARGUMENT;

    const gh_actor_part *child_part, *parent_part;
    if (gh_actor_model_part_at(&child->model, 0, &child_part) ||
        gh_actor_model_part_at(model, 17, &parent_part)) return GH_EVENT_BOUNDS;
    if (word(child_part->bytes + 0x148) || word(child->object.view.bytes + 0x20a8) ||
        word(child->object.view.bytes + 0x20d0)) return GH_EVENT_UNSUPPORTED;
    uint8_t *p = child->object.view.bytes;
    const gh_player_child_attachment previous = *attachment;
    if (gh_actor_model_bind_external_parent(&child->model, 0, model, 17, attachment)) return GH_EVENT_CALLEE;
    if (!s->retargeted) {
        s->previous = previous;
        s->old_250 = word(p + 0x250);
        s->old_2f0 = word(p + 0x2f0);
        s->old_2088 = word(p + 0x2088);
        s->old_20d4 = word(p + 0x20d4);
    }
    *attachment = (gh_player_child_attachment){
        .child = child, .parent_model = model, .child_part = child_part, .parent_part = parent_part,
        .part_index = 17, .attached = 1
    };
    s->retargeted = 1;
    s->jacket_parent = model;
    const unsigned costume = gh_core_save_costume(b->save);
    put(p + 0x20d4, 17);
    put(p + 0x2088, costume == 2 || costume == 3 ? 11 : costume == 4 || costume == 5 ? 7 : 12);
    return GH_EVENT_OK;
}

static gh_game_event_status retarget_jacket(gh_event_actor_extra *s, gh_actor_model *model) {
    gh_game_event_status status = gh_event_actor_extra_attach_jacket_00100ed8(s, model);
    if (status) return status;
    uint8_t *p = s->bindings.child->object.view.bytes;
    // The original creation caller edits current flags around the 100ED8 setter.
    put(p + 0x250, (word(p + 0x250) & ~0x8002u) | 0x10000u);
    put(p + 0x2f0, word(p + 0x2f0) | 1);
    return GH_EVENT_OK;
}

static gh_game_event_status hide_mesh(gh_actor_model *model, const uint8_t name[8]) {
    gh_player_mesh_reference reference;
    int found = 0;
    if (gh_actor_model_find_mesh_0014b730(model, name, 8, &reference, &found)) return GH_EVENT_BOUNDS;
    if (!found) return GH_EVENT_OK; // Original return 0 is ignored by ExtraInitialize.
    for (unsigned node = 0; node < model->script.mesh_count; ++node) {
        const uint8_t *bytes;
        size_t mesh_index;
        if (gh_actor_model_node_at(model, node, &bytes, &mesh_index)) return GH_EVENT_BOUNDS;
        if (bytes == reference.bytes && mesh_index == reference.mesh_index) {
            return gh_actor_model_node_or_flags(model, node, 0x380, 1) ? GH_EVENT_CALLEE : GH_EVENT_OK;
        }
    }
    return GH_EVENT_BOUNDS;
}

static gh_game_event_status hide_hands(gh_actor_model *model, uint8_t *cursor) {
    static const uint8_t names[][8] = {"RH_01", "LH_01", "LH_02"};
    for (; *cursor < sizeof(names) / sizeof(*names); ++*cursor) {
        gh_game_event_status status = hide_mesh(model, names[*cursor]);
        if (status) return status;
    }
    return GH_EVENT_OK;
}

static void set_parent_info(gh_actor_model *model, const gh_game_math_globals *math, unsigned mode) {
    uint8_t *p = model->root.bytes;
    put(p + 0x30b4, UINT32_MAX);
    memcpy(p + 0x30c0, math->zero_vector, 12);
    memcpy(p + 0x30d0, math->zero_vector, 12);
    p[0x4e0] = 1;
    put(p + 0x30a4, mode);
}

static gh_game_event_status init_pendulum(gh_event_actor_extra *s, gh_actor_model *actor,
    gh_actor_model *parent, uint32_t *frontier) {
    uint8_t *p = actor->root.bytes;
    if (frontier) *frontier = 0x2de980;
    put(p + 0x1394, 0x2a0);
    p[0x1390] = 1;
    if (!s->pendulum_profile.loaded) {
        if (!s->bindings.boot_elf) return GH_EVENT_UNSUPPORTED;
        if (gh_event_spring_profile_from_elf(&s->pendulum_profile, 0x2a0, s->bindings.boot_elf,
            s->bindings.boot_elf_size)) return GH_EVENT_BOUNDS;
    }
    uint8_t *config = p + 0x2f20;
    put(config, 10);
    for (unsigned i = 1; i <= 13; ++i) put(config + i * 4, 0);
    put(config + 0x38, 6);
    put(config + 0x3c, 0x3ba3d70a);
    put(config + 0x40, 0x3f000000);
    put(config + 0x44, 1);
    put(config + 0x48, 0);
    put(config + 0x4c, 0x3dcccccd);
    put(config + 0x50, 0x3f000000);
    put(config + 0x54, 0);
    put(config + 0x58, 0);
    put(config + 0x5c, 0x100);
    put(config + 0x60, 0);
    put(config + 0x64, 0);
    put(config + 0x68, 0);
    put(config + 0x70, 0x3f860a92);
    if (gh_event_spring_records_001038e0(&s->pendulum_profile, actor, p + 0x13a0,
        44 * 0xa0, GH_STATIC_BINARY32_REFERENCE, frontier)) return GH_EVENT_CALLEE;
    // 2DF938 swaps the +58 borrowed object from self to the actual EL0A.
    s->pendulum_parent = parent;
    return GH_EVENT_OK;
}

static gh_game_event_status init_ev04_pendulum(gh_event_actor_extra *, gh_actor_model *,
    uint16_t, uint32_t *);

static gh_game_event_status attach_el_pair(gh_event_actor_extra *s,
    const gh_event_actor_create_request *request, uint32_t handle, int *original, uint32_t *frontier) {
    const gh_event_actor_extra_bindings *b = &s->bindings;
    if (!b->roster || !b->creation || request->data != b->creation->bindings.data ||
        request->actor_index >= b->roster->capacity || request->actor_index >= request->data->actor_count) {
        return GH_EVENT_ARGUMENT;
    }
    unsigned variant;
    uint16_t source;
    switch (request->source_kind) {
        case 0x601: variant = 0; source = 0x600; break;
        case 0x602: variant = 1; source = 0x600; break;
        case 0x60b: variant = 2; source = 0x60a; break;
        case 0x60c: variant = 3; source = 0x60a; break;
        case 0x606: variant = 4; source = 0x605; break;
        case 0x607: variant = 5; source = 0x605; break;
        case 0x642: variant = 6; source = 0x641; break;
        case 0x647: variant = 7; source = 0x646; break;
        case 0x611: variant = 8; source = 0x610; break;
        case 0x612: variant = 9; source = 0x610; break;
        case 0x616: variant = 10; source = 0x615; break;
        case 0x617: variant = 11; source = 0x615; break;
        case 0x621: variant = 12; source = 0x620; break;
        case 0x622: variant = 13; source = 0x620; break;
        case 0x626: variant = 14; source = 0x625; break;
        case 0x627: variant = 15; source = 0x625; break;
        default: return GH_EVENT_ARGUMENT;
    }
    gh_event_actor_factory *child = &b->roster->rows[request->actor_index];
    if (handle != child->handle || !handle || child->init_stage != 4 ||
        b->creation->actor_handles[request->actor_index] != handle) return GH_EVENT_ARGUMENT;
    uint8_t *p = child->object.view.bytes;
    uint8_t *stage = &s->row_stage[variant];
    if (!*stage) {
        put(request->work + 0x34, 0xffff);
        put(request->work + 0x38, 0);
        // ExtraInitialize assigns the source ID after costume-specific construction.
        p[0x2fe] = (uint8_t)request->source_kind;
        p[0x2ff] = 6;
        put(p + 0x2d0, 0x800000);
        *stage = 1;
    }
    if (*stage == 1) {
        if (variant == 1) {
            if (frontier) *frontier = 0x14a338;
            gh_game_event_status status = hide_hands(&child->model, &s->row_mesh_cursor[variant]);
            if (status) return status;
        }
        if (variant == 15) {
            if (frontier) *frontier = 0x14a338;
            static const uint8_t hand[8] = "LH_01";
            gh_game_event_status status = hide_mesh(&child->model, hand);
            if (status) return status;
        }
        *stage = 2;
    }
    if (*stage == 2) {
        if (frontier) *frontier = 0x297a48;
        gh_event_actor_factory *parent = NULL;
        for (size_t i = 0; i < request->data->actor_count && i < b->roster->capacity; ++i) {
            const uint8_t *row;
            size_t cut;
            if (gh_event_actor_create_work_at(b->creation, i, &row, &cut)) return GH_EVENT_BOUNDS;
            if (b->creation->actor_handles[i] && word(row + 4) == source && !row[8]) {
                parent = &b->roster->rows[i];
                if (parent->handle != b->creation->actor_handles[i]) return GH_EVENT_ARGUMENT;
                break;
            }
        }
        if (!parent) { // Actual REL+1640 search returned NULL; helper then returns zero.
            if (variant == 2) {
                // EL0B still enters its pendulum setter before the false-result destructor.
                if (frontier) *frontier = 0x2b8508;
                return GH_EVENT_UNSUPPORTED;
            }
            if (frontier) *frontier = 0;
            return GH_EVENT_OK;
        }
        if (!b->math || !b->math->initialized) return GH_EVENT_UNSUPPORTED;
        static const gh_actor_external_part_pair el01[] = {{0, 2}, {1, 3}, {2, 4}};
        static const gh_actor_external_part_pair el02[] = {{0, 15}, {1, 16}, {17, 9}, {18, 10}};
        const int wide = variant == 6 || (variant & 1);
        const gh_actor_external_part_pair *pairs = wide ? el02 : el01;
        if (gh_actor_model_bind_external_parts(&s->row_parts[variant], &child->model, &parent->model,
            pairs, wide ? 4 : 3)) return GH_EVENT_CALLEE;
        set_parent_info(&child->model, b->math, wide ? 1 : 2);
        *stage = 3;
    }
    if (*stage == 3 && source == 0x60a) {
        gh_event_component_view env;
        if (gh_event_component_at(b->creation->bindings.components, 0, &env)) return GH_EVENT_ARGUMENT;
        if (variant == 2) {
            put(request->work + 0x34, 0x2a0);
            gh_game_event_status status = init_pendulum(s, &child->model, s->row_parts[variant].parent, frontier);
            if (status) return status;
            p[0x2b5] = word(env.bytes) == 0x24 ? 2 : 0xfe;
        } else if (word(env.bytes) == 0x24) {
            p[0x2b5] = 4;
        }
        *stage = 4;
    }
    if (*stage == 3 && (variant == 10 || variant == 14)) {
        const uint16_t profile = variant == 10 ? 0x252 : 0x256;
        if (variant == 14) {
            if (frontier) *frontier = 0x14a338;
            static const uint8_t mesh[8] = "z_10";
            gh_game_event_status status = hide_mesh(&child->model, mesh);
            if (status) return status;
        }
        put(request->work + 0x34, profile);
        gh_game_event_status status = init_ev04_pendulum(s, &child->model, profile, frontier);
        if (status) return status;
        p[0x2b5] = 0xfe;
        *stage = 4;
    }
    *original = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status attach_olivia_accessory(gh_event_actor_extra *s,
    const gh_event_actor_create_request *request, uint32_t handle, int *original, uint32_t *frontier) {
    const gh_event_actor_extra_bindings *b = &s->bindings;
    if (!b->roster || !b->creation || request->data != b->creation->bindings.data ||
        request->actor_index >= b->roster->capacity || request->actor_index >= request->data->actor_count) {
        return GH_EVENT_ARGUMENT;
    }
    gh_event_actor_factory *actor = &b->roster->rows[request->actor_index];
    if (!handle || handle != actor->handle || b->creation->actor_handles[request->actor_index] != handle ||
        actor->init_stage != 4) return GH_EVENT_ARGUMENT;
    uint8_t *p = actor->object.view.bytes;
    if (!s->olivia_stage) {
        put(request->work + 0x34, 0xffff);
        put(request->work + 0x38, 0);
        p[0x2fe] = 0x0a;
        p[0x2ff] = 6;
        put(p + 0x2d0, 0x800000);
        s->olivia_stage = request->create_kind == 0x60a ? 1 : 4;
    }
    const gh_event_actor_create_request accessory_request = {
        .actor_index = request->actor_index, .source_kind = 0x60d, .create_kind = 0x60d,
        .resource_kind = 0x60d, .create_argument = 0xffff
    };
    gh_event_actor_factory *accessory = &s->olivia_accessory;
    if (s->olivia_stage == 1) {
        if (!accessory->bound) {
            gh_game_event_status status = gh_event_actor_factory_bind(accessory, &b->roster->bindings);
            if (status) return status;
        }
        int present = 0;
        gh_game_event_status status = gh_event_actor_factory_run(accessory, &accessory_request, &present, frontier);
        if (status) return status;
        s->olivia_stage = present ? 2 : 4;
    }
    if (s->olivia_stage == 2) {
        int initialized = 0;
        gh_game_event_status status = gh_event_actor_factory_initialize_model(accessory, &accessory_request,
            &initialized, frontier);
        if (status) return status;
        s->olivia_stage = initialized ? 3 : 4;
    }
    if (s->olivia_stage == 3) {
        if (frontier) *frontier = 0x297a48;
        if (!b->math || !b->math->initialized) return GH_EVENT_UNSUPPORTED;
        static const gh_actor_external_part_pair pairs[] = {{0, 2}, {1, 5}, {2, 6}, {3, 7}, {4, 11}, {5, 12}, {6, 13}};
        if (gh_actor_model_bind_external_parts(&s->accessory_parts, &accessory->model, &actor->model,
            pairs, sizeof(pairs) / sizeof(*pairs))) return GH_EVENT_CALLEE;
        set_parent_info(&accessory->model, b->math, 4);
        accessory->object.view.bytes[0x2b5] = 0xfd;
        put(request->work + 0x30, 0x60d);
        s->accessory_handle = accessory->handle; // REL row+2C stores this identity.
        s->olivia_stage = 4;
    }
    if (s->olivia_stage == 4) {
        gh_event_component_view env;
        if (gh_event_component_at(b->creation->bindings.components, 0, &env)) return GH_EVENT_ARGUMENT;
        p[0x2fa0] = 1; // SetBustFlag2B8500.
        request->work[0x24] |= 8;
        if (word(env.bytes) == 0x24) p[0x2b5] = 4;
        s->olivia_stage = 5;
    }
    *original = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status extra_standalone(gh_event_actor_extra *s,
    const gh_event_actor_create_request *request, uint32_t handle, int *original, uint32_t *frontier) {
    const gh_event_actor_extra_bindings *b = &s->bindings;
    if (!b->roster || !b->creation || request->data != b->creation->bindings.data ||
        request->actor_index >= b->roster->capacity || request->actor_index >= request->data->actor_count) {
        return GH_EVENT_ARGUMENT;
    }
    gh_event_actor_factory *actor = &b->roster->rows[request->actor_index];
    if (!handle || handle != actor->handle || b->creation->actor_handles[request->actor_index] != handle ||
        actor->init_stage != 4) return GH_EVENT_ARGUMENT;
    uint8_t *p = actor->object.view.bytes;
    put(request->work + 0x34, 0xffff);
    put(request->work + 0x38, 0);
    p[0x2fe] = (uint8_t)request->source_kind;
    p[0x2ff] = (uint8_t)(request->source_kind >> 8);
    const int prop = request->source_kind >= 0x300 && request->source_kind < 0x500;
    put(p + 0x2d0, request->packed_resource ? 0x40000000 : prop ? 0x20000000 : 0x800000);
    if (prop) {
        if (frontier) *frontier = 0x14b6f8;
        for (unsigned i = 0; i < p[0x2b1]; ++i) {
            const uint8_t *node;
            size_t mesh;
            if (gh_actor_model_node_at(&actor->model, i, &node, &mesh)) return GH_EVENT_BOUNDS;
            if (node[0x40c] >= 0x40 && gh_actor_model_node_or_flags(&actor->model, i, 0x380, 1))
                return GH_EVENT_CALLEE;
        }
    }
    if (request->source_kind == 0x650) p[0x2b5] = (uint8_t)-10;
    if (request->packed_resource || request->source_kind == 0x61a || request->source_kind == 0x630 || request->source_kind == 0x631 ||
        request->source_kind == 0x634 || request->source_kind == 0x639) p[0x4d8] = 1;
    static const uint8_t one_body_meshes[][9] = {"HD_01", "z_hana", "z_hana_m"};
    if (request->packed_resource || request->source_kind == 0x61a) {
        /* 2B83B8 sets one-body and hides the same three child meshes. */
        if (frontier) *frontier = 0x14a338;
        for (unsigned i = 0; i < sizeof(one_body_meshes) / sizeof(*one_body_meshes); ++i) {
            gh_game_event_status status = hide_mesh(&actor->model, one_body_meshes[i]);
            if (status) return status;
        }
    }
    if (request->source_kind == 0x630 || request->source_kind == 0x631) {
        if (frontier) *frontier = 0x14a338;
        uint8_t *cursor = &s->one_body_mesh[request->source_kind - 0x630];
        for (; *cursor < sizeof(one_body_meshes) / sizeof(*one_body_meshes); ++*cursor) {
            gh_game_event_status status = hide_mesh(&actor->model, one_body_meshes[*cursor]);
            if (status) return status;
        }
    }
    *original = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status init_ev04_pendulum(gh_event_actor_extra *s, gh_actor_model *actor,
    uint16_t kind, uint32_t *frontier)
{
    uint8_t *p = actor->root.bytes;
    put(p + 0x1394, kind);
    p[0x1390] = 1;
    if (frontier) *frontier = 0x2de980;
    const unsigned index = kind == 0x250 ? 0 : kind == 0x252 ? 1 : 2;
    gh_event_spring_profile *profile = &s->ev04_pendulums[index];
    if (!profile->loaded && gh_event_spring_profile_from_elf(profile, kind,
        s->bindings.boot_elf, s->bindings.boot_elf_size)) return GH_EVENT_BOUNDS;
    uint8_t *config = p + 0x2f20;
    memset(config, 0, 0x6c);
    put(config, profile->count);
    put(config + 0x38, kind == 0x250 ? 2 : kind == 0x252 ? 6 : 4);
    put(config + 0x3c, kind == 0x252 ? 0x3ba3d70a : 0x3c75c28f);
    put(config + 0x40, kind == 0x252 ? 0x3f000000 : 0x3f4ccccd);
    put(config + 0x44, 1);
    put(config + 0x4c, 0x3dcccccd);
    put(config + 0x50, 0x3f000000);
    put(config + 0x5c, 0x100);
    put(config + 0x70, 0x3f860a92);
    if (gh_event_spring_records_001038e0(profile, actor, p + 0x13a0,
        44 * 0xa0, GH_STATIC_BINARY32_REFERENCE, frontier)) return GH_EVENT_CALLEE;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status extra_el10(gh_event_actor_extra *s,
    const gh_event_actor_create_request *request, uint32_t handle, int *original, uint32_t *frontier)
{
    gh_game_event_status status = extra_standalone(s, request, handle, original, frontier);
    if (status) return status;
    gh_actor_model *actor = &s->bindings.roster->rows[request->actor_index].model;
    put(request->work + 0x34, 0x250);
    actor->root.bytes[0x2ff0] = 1; /* Event 4 takes SetRingFlag; Event 22 bypasses it. */
    return init_ev04_pendulum(s, actor, 0x250, frontier);
}

static gh_game_event_status initialize_ev04_accessory(gh_event_actor_extra *s,
    gh_event_actor_factory *actor, gh_event_spring_profile *profile, uint32_t *frontier)
{
    const int em53 = actor->kind == 0x253;
    if (frontier) *frontier = em53 ? 0x27a180 : 0x27ac70;
    uint8_t *root = actor->object.view.bytes;
    if (!actor->init_stage) {
        gh_bundle asset;
        if (gh_bundle_open(actor->resource.bytes, actor->resource.file_extent, &asset) ||
            gh_bundle_at(&asset, 0, &actor->geometry) || gh_bundle_at(&asset, 1, &actor->texture) ||
            memcmp(actor->geometry.tag, "MD\0", 4) || memcmp(actor->texture.tag, "TM3", 4)) return GH_EVENT_FORMAT;
        if (gh_actor_model_bind_packet_inputs(&actor->model, actor->bindings.packet_heap, actor->bindings.fog_rgb))
            return GH_EVENT_ARGUMENT;
        int original = 0;
        gh_actor_model_status status = gh_actor_model_initialize_00149738(&actor->model, &actor->object.view,
            actor->bindings.work, actor->bindings.pools, &actor->geometry, &actor->texture, 0, 0,
            GH_STATIC_BINARY32_REFERENCE, &original);
        if (status || !original) {
            if (frontier && actor->model.frontier) *frontier = actor->model.frontier;
            return status == GH_ACTOR_MODEL_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE;
        }
        put(root + 0x2f0, word(root + 0x2f0) | 0x40);
        root[0x531] = UINT8_MAX;
        put(root + 0x250, word(root + 0x250) | 0x10000);
        if (actor->model.parts_allocated) {
            const gh_actor_part *part;
            if (gh_actor_model_part_at(&actor->model, 0, &part)) return GH_EVENT_CALLEE;
            memset(part->bytes + 0xe0, 0, 12);
            memset(part->alias_d0, 0, 12);
        }
        actor->init_stage = 1;
    }
    if (actor->init_stage == 1) {
        if (frontier) *frontier = 0x149eb8;
        gh_actor_model_status status = gh_actor_model_update_hierarchy_00149eb8(&actor->model,
            GH_STATIC_BINARY32_REFERENCE);
        if (status) return status == GH_ACTOR_MODEL_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE;
        actor->init_stage = 2;
    }
    if (actor->init_stage == 2) {
        if (frontier) *frontier = em53 ? 0x27a600 : 0x27b0f8;
        if (!profile->loaded && gh_event_spring_profile_from_elf(profile, actor->kind,
            s->bindings.boot_elf, s->bindings.boot_elf_size)) return GH_EVENT_BOUNDS;
        uint8_t *config = root + 0x2130;
        put(config, profile->count);
        put(config + 0x3c, 0x3c75c28f);
        put(config + 0x40, 0x3f4ccccd);
        put(config + 0x44, 1);
        put(config + 0x4c, em53 ? 0x3f000000 : 0x3f4ccccd);
        put(config + 0x50, 0x3f000000);
        put(config + 0x5c, 0x100);
        put(config + 0x70, 0x3f860a92);
        if (gh_event_spring_records_001038e0(profile, &actor->model, root + 0x5b0, 44 * 0xa0,
            GH_STATIC_BINARY32_REFERENCE, frontier)) return GH_EVENT_CALLEE;
        actor->init_stage = 3;
    }
    return GH_EVENT_OK;
}

static gh_game_event_status extra_ev04_accessory(gh_event_actor_extra *s,
    const gh_event_actor_create_request *request, uint32_t handle, int *original, uint32_t *frontier)
{
    gh_game_event_status status = extra_standalone(s, request, handle, original, frontier);
    if (status) return status;
    *original = 0;
    const int em57 = request->source_kind == 0x625;
    const unsigned index = em57 ? 1 : request->source_kind == 0x61a ? 2 : 0;
    const uint16_t kind = em57 ? 0x628 : 0x253;
    const unsigned parent_part = em57 ? 17 : 2;
    uint8_t *work = request->work;
    gh_event_actor_factory *body = &s->bindings.roster->rows[request->actor_index];
    gh_event_actor_factory *accessory = &s->ev04_accessory[index];
    if (!accessory->bound) {
        status = gh_event_actor_factory_bind(accessory, &s->bindings.roster->bindings);
        if (status) return status;
    }
    gh_event_actor_create_request child = {
        .data = request->data, .actor_index = request->actor_index, .source_kind = kind,
        .create_kind = kind, .resource_kind = kind, .create_argument = UINT16_MAX
    };
    int made;
    status = gh_event_actor_factory_run(accessory, &child, &made, frontier);
    if (status) return status;
    if (made) {
        status = initialize_ev04_accessory(s, accessory, &s->ev04_accessory_springs[index], frontier);
        if (status) return status;
        if (frontier) *frontier = em57 ? 0x27b260 : 0x27a768;
        if (gh_actor_model_bind_external_parent(&accessory->model, 0, &body->model, parent_part, accessory))
            return GH_EVENT_CALLEE;
        uint8_t *raw = accessory->object.view.bytes;
        put(raw + 0x21b4, parent_part);
        put(raw + 0x2168, 11);
        put(raw + 0x2f0, word(raw + 0x2f0) | 1);
        put(raw + 0x250, word(raw + 0x250) | 0x10000);
    }
    put(work + 0x30, em57 ? 0x257 : 0x253);
    if (!em57) {
        body->object.view.bytes[0x2fa0] = 1; /* SetBustFlag. */
        work[0x24] |= 8;
    } else {
        if (frontier) *frontier = 0x14a338;
        static const uint8_t hands[][8] = {"RH_01", "LH_01", "LH_02"};
        for (unsigned i = 0; i < sizeof(hands) / sizeof(*hands); ++i) {
            status = hide_mesh(&body->model, hands[i]);
            if (status) return status;
        }
    }
    *original = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

/* REL 202C retargets the same GetJacket owner used by EL00. */
static gh_game_event_status extra_el05(gh_event_actor_extra *s,
    const gh_event_actor_create_request *request, uint32_t handle, int *original, uint32_t *frontier) {
    const gh_event_actor_extra_bindings *b = &s->bindings;
    if (!b->roster || !b->creation || request->data != b->creation->bindings.data ||
        request->actor_index >= b->roster->capacity || request->actor_index >= request->data->actor_count)
        return GH_EVENT_ARGUMENT;
    if (request->data->components->event_no != 2 || request->actor_index != 3)
        return GH_EVENT_UNSUPPORTED;
    gh_event_actor_factory *actor = &b->roster->rows[request->actor_index];
    if (!handle || handle != actor->handle || b->creation->actor_handles[request->actor_index] != handle ||
        actor->init_stage != 4 || !actor->model.ready)
        return GH_EVENT_ARGUMENT;
    if (!s->el05_stage) {
        put(request->work + 0x34, 0xffff);
        put(request->work + 0x38, 0);
        uint8_t *p = actor->object.view.bytes;
        p[0x2fe] = 5;
        p[0x2ff] = 6;
        put(p + 0x2d0, 0x800000);
        s->el05_stage = 1;
    }
    if (!b->child) {
        if (frontier) *frontier = 0;
        return GH_EVENT_OK;
    }
    if (s->el05_stage == 1) {
        if (frontier) *frontier = 0x100ed8;
        gh_game_event_status status = retarget_jacket(s, &actor->model);
        if (status) return status;
        put(request->work + 0x30, 0x102);
        s->el05_stage = 2;
    }
    if (s->el05_stage == 2) {
        static const uint8_t name[8] = "RH_01";
        if (frontier) *frontier = 0x14a338;
        gh_game_event_status status = hide_mesh(&actor->model, name);
        if (status) return status;
        s->el05_stage = 3;
    }
    *original = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_extra_run(void *opaque, const gh_event_actor_create_request *request,
    uint32_t handle, int *original_success, uint32_t *frontier) {
    gh_event_actor_extra *s = opaque;
    if (frontier) *frontier = 0x297a48;
    if (!s || !s->bound || !request || !request->work || !original_success) return GH_EVENT_ARGUMENT;
    *original_success = 0;
    const gh_event_actor_extra_bindings *b = &s->bindings;
    if (request->source_kind == 0x601 || request->source_kind == 0x602 ||
        request->source_kind == 0x60b || request->source_kind == 0x60c ||
        request->source_kind == 0x606 || request->source_kind == 0x607 ||
        request->source_kind == 0x642 || request->source_kind == 0x647) {
        return attach_el_pair(s, request, handle, original_success, frontier);
    }
    if (request->source_kind == 0x605) {
        return extra_el05(s, request, handle, original_success, frontier);
    }
    if (request->source_kind == 0x60a) {
        return attach_olivia_accessory(s, request, handle, original_success, frontier);
    }
    if (request->data && request->data->components &&
        (request->data->components->event_no == 4 ||
         (request->info && gh_event_actor_ev07_shared_source_kind(request->data->components->event_no,
            request->actor_index,request->source_kind,request->info[4])))) {
        switch (request->source_kind) {
        case 0x610:
            return extra_el10(s, request, handle, original_success, frontier);
        case 0x611: case 0x612: case 0x616: case 0x617:
        case 0x621: case 0x622: case 0x626: case 0x627:
            return attach_el_pair(s, request, handle, original_success, frontier);
        case 0x615: case 0x61a: case 0x625:
            return extra_ev04_accessory(s, request, handle, original_success, frontier);
        case 0x618:
            if (s->ev04_accessory[0].handle)
                s->ev04_el18_accessory = &s->ev04_accessory[0]; /* REL writes row+38. */
            /* fall through */
        case 0x620: case 0x660: case 0x661: case 0x662:
            return extra_standalone(s, request, handle, original_success, frontier);
        default: break;
        }
    }
    if (request->data&&request->data->components&&
        gh_event_actor_ev05_ev06_source_kind(request->data->components->event_no,request->actor_index,request->source_kind)) {
        if (request->source_kind==0x610)
            return extra_el10(s,request,handle,original_success,frontier);
        if (request->source_kind==0x611||request->source_kind==0x612)
            return attach_el_pair(s,request,handle,original_success,frontier);
        return extra_standalone(s,request,handle,original_success,frontier);
    }
    /* EV1B's first actor takes the original ExtraInitialize default for EL40. */
    if (request->source_kind == 0x640 && request->actor_index == 0 && request->data &&
        request->data->components && request->data->components->event_no == 0x1b) {
        return extra_standalone(s, request, handle, original_success, frontier);
    }
    if (((request->actor_index == 5 && request->source_kind == 0x338) ||
         (request->actor_index == 6 && request->source_kind == 0x3bc)) && request->data &&
        request->data->components && request->data->components->event_no == 0x1b) {
        return extra_standalone(s, request, handle, original_success, frontier);
    }
    if (request->packed_resource) return extra_standalone(s, request, handle, original_success, frontier);
    if (request->source_kind == 0x630 || request->source_kind == 0x631 ||
        request->source_kind == 0x634 || request->source_kind == 0x639 ||
        request->source_kind == 0x641 || request->source_kind == 0x646 ||
        (request->source_kind >= 0x650 && request->source_kind <= 0x652) ||
        request->source_kind == 0x659 || request->source_kind == 0x667 || request->source_kind == 0x367) {
        return extra_standalone(s, request, handle, original_success, frontier);
    }
    gh_event_actor_factory *factory = b->factory;
    if (request->actor_index || request->source_kind != 0x600 || request->create_kind != factory->kind ||
        handle != factory->handle || !handle || factory->init_stage != 4 || !factory->model.ready) {
        return GH_EVENT_ARGUMENT;
    }
    uint8_t *row = request->work;
    if (!s->stage) {
        put(row + 0x34, 0xffff);
        put(row + 0x38, 0);
        // Original setId14E690 before the EL00 branch.
        factory->object.view.bytes[0x2fe] = 0;
        factory->object.view.bytes[0x2ff] = 6;
        put(factory->object.view.bytes + 0x2d0, 0x800000);
        s->stage = 1;
    }
    if (!b->child) {
        if (frontier) *frontier = 0;
        return GH_EVENT_OK;
    }
    if (s->stage == 1) {
        if (frontier) *frontier = 0x100ed8;
        gh_game_event_status status = retarget_jacket(s, &factory->model);
        if (status) return status;
        s->stage = 2;
    }
    if (s->stage == 2) {
        if (frontier) *frontier = 0x1cf160;
        if (!b->weapon_slot->known) return GH_EVENT_UNSUPPORTED;
        const gh_event_weapon_attachment *weapon = b->weapon_slot->current;
        if (s->weapon_started && s->weapon != weapon) return GH_EVENT_ARGUMENT;
        if (weapon) {
            if (!b->attach_weapon) return GH_EVENT_UNSUPPORTED;
            if (!weapon->weapon || !weapon->weapon->bytes || weapon->weapon->size < 0x254) return GH_EVENT_BOUNDS;
            uint8_t *p = weapon->weapon->bytes;
            if (!s->weapon_started) {
                put(p + 0x250, word(p + 0x250) & ~0x8002u);
                s->weapon = weapon;
                s->weapon_started = 1;
            }
            const unsigned costume = gh_core_save_costume(b->save);
            const uint32_t position[] = {0, 0x3d408312, costume >= 2 && costume < 6 ? 0xbe1eb852 : 0xbe0f5c29};
            const uint32_t rotation[] = {0, 0, 0};
            uint32_t child = 0;
            gh_game_event_status status = b->attach_weapon(b->weapon_context, weapon, &factory->model, 2,
                position, rotation, &child);
            if (status) {
                if (child && frontier) *frontier = child;
                return status;
            }
        }
        s->weapon = weapon;
        // The typed bindings hold row+28/+2C/+3C's actual object identities.
        put(row + 0x30, 0x101);
        s->stage = 3;
    }
    if (s->stage == 3) {
        if (frontier) *frontier = 0x14a338;
        gh_game_event_status status = hide_hands(&factory->model, &s->mesh_cursor);
        if (status) return status;
        s->stage = 4;
    }
    *original_success = 1;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status move_object_last(gh_event_actor_extra *s, uint16_t kind, uint32_t known_handle,
    uint32_t *frontier) {
    const gh_event_actor_create_bindings *b = &s->bindings.creation->bindings;
    const gh_room_vector *vector;
    if (gh_room_registry_view(b->pools, 0x754c30, &vector)) return GH_EVENT_ARGUMENT;
    uint8_t *entries = vector->allocation.pointer;
    for (size_t i = 0; i < vector->count; ++i) {
        const uint32_t handle = word(entries + i * 4);
        if (known_handle) {
            if (handle != known_handle) continue;
        } else {
            gh_title_player_view object;
            uint32_t child = 0;
            gh_game_event_status status = b->object_view(b->context, handle, &object, &child);
            if (status) {
                if (child && frontier) *frontier = child;
                return status;
            }
            if (!object.bytes || object.size < 0x300) return GH_EVENT_BOUNDS;
            if (half(object.bytes + 0x2fe) != kind) continue;
        }
        memmove(entries + i * 4, entries + (i + 1) * 4, (vector->count - i - 1) * 4);
        put(entries + (vector->count - 1) * 4, handle);
        break;
    }
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_extra_finish(void *opaque, uint32_t *frontier) {
    gh_event_actor_extra *s = opaque;
    if (frontier) *frontier = 0x297a48;
    if (!s || !s->bound || !s->bindings.creation || !s->bindings.roster ||
        s->bindings.creation->actor_cursor != s->bindings.creation->bindings.data->actor_count) {
        return GH_EVENT_ARGUMENT;
    }
    const gh_event_actor_extra_bindings *b = &s->bindings;
    if (!s->finish_stage) {
        // Work+5F8's canonical jacket is the unique 0x101 object in EV01.
        if (b->child && !(word(b->child->object.view.bytes + 0x250) & 0x8000u)) {
            void *identity = NULL;
            if (gh_room_object_lookup(b->child->b.pools, b->child->handle, GH_ROOM_OBJECT_PLAYER_CHILD,
                &b->child->object, &identity) || identity != &b->child->object) return GH_EVENT_ARGUMENT;
            gh_game_event_status status = move_object_last(s, 0x101, b->child->handle, frontier);
            if (status) return status;
        }
        s->finish_stage = 1;
    }
    if (s->finish_stage == 1) {
        if (frontier) *frontier = 0x2db6b0;
        if (!b->weapon_slot->known) return GH_EVENT_UNSUPPORTED;
        const gh_event_weapon_attachment *weapon = b->weapon_slot->current;
        if (weapon) {
            if (!weapon->weapon || !weapon->weapon->bytes || weapon->weapon->size < 0x254) return GH_EVENT_BOUNDS;
            if (!(word(weapon->weapon->bytes + 0x250) & 0x8000u)) {
                gh_game_event_status status = move_object_last(s, 0x376, 0, frontier);
                if (status) return status;
            }
        }
        s->finish_stage = 2;
    }
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_extra_abort(gh_event_actor_extra *s) {
    if (!s) return GH_EVENT_ARGUMENT;
    for (size_t i = 0; i < sizeof(s->ev04_accessory) / sizeof(*s->ev04_accessory); ++i) {
        gh_event_actor_factory *actor = &s->ev04_accessory[i];
        if (actor->model.external_owner && gh_actor_model_clear_external_parent(&actor->model, actor))
            return GH_EVENT_CALLEE;
        if (gh_event_actor_factory_abort(actor)) return GH_EVENT_CALLEE;
    }
    if (gh_actor_model_clear_external_parts(&s->accessory_parts) ||
        gh_event_actor_factory_abort(&s->olivia_accessory)) return GH_EVENT_CALLEE;
    for (size_t i = sizeof(s->row_parts) / sizeof(*s->row_parts); i; --i) {
        if (gh_actor_model_clear_external_parts(&s->row_parts[i - 1])) return GH_EVENT_CALLEE;
    }
    if (s->retargeted) {
        gh_player_child_attachment *current = s->bindings.attachment;
        gh_game_player_child *child = s->bindings.child;
        if (!current->attached || current->child != child ||
            current->parent_model != s->jacket_parent || child->model.external_owner != current) {
            return GH_EVENT_CALLEE;
        }
        gh_actor_model_status status = s->previous.attached ?
            gh_actor_model_bind_external_parent(&child->model, 0, s->previous.parent_model,
                (unsigned)s->previous.part_index, current) :
            gh_actor_model_clear_external_parent(&child->model, current);
        if (status) return GH_EVENT_CALLEE;
        *current = s->previous;
        uint8_t *p = child->object.view.bytes;
        put(p + 0x250, (word(p + 0x250) & ~0x18002u) | (s->old_250 & 0x18002u));
        put(p + 0x2f0, (word(p + 0x2f0) & ~1u) | (s->old_2f0 & 1u));
        put(p + 0x2088, s->old_2088);
        put(p + 0x20d4, s->old_20d4);
    }
    memset(s, 0, sizeof(*s));
    return GH_EVENT_OK;
}
