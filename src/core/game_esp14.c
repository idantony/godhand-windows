#include "game_esp14.h"
#include "game_actor_model_nodes.h"
#include "game_actor_two_bone.h"
#include "id_layout_rotation.h"

#include <fenv.h>
#include <math.h>
#include <string.h>

static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void put(uint8_t *p, uint32_t n) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(n >> (i * 8));
}

static float number(uint32_t bits) {
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void store(uint8_t *p, float value) {
    memcpy(p, &value, sizeof(value));
}

static float multiply(float a, float b) {
    volatile float result = a * b;
    return result;
}

static float add(float a, float b) {
    volatile float result = a + b;
    return result;
}

static int signed_byte(uint8_t n) {
    return n < 128 ? n : (int)n - 256;
}

void gh_esp14_construct_tail_002ffd88(uint8_t p[0x400]) {
    put(p + 0x2b0, 0);
    put(p + 0xf0, GH_ESP14_VTABLE);
}

gh_effect_status gh_esp14_init_prefix_002f1d58(uint8_t p[0x400],
    gh_effect_view record, uint32_t parameter_40, uint32_t *frontier) {
    if (!frontier) return GH_EFFECT_ARGUMENT;
    *frontier = 0x2f1d58;
    if (!p || !record.bytes || record.size < 0x2f0) return GH_EFFECT_ARGUMENT;
    const uint8_t *r = record.bytes;
    if (r[0x133] != 0x14 || p[0x102] != 0x14 || word(p + 0xf0) != GH_ESP14_VTABLE)
        return GH_EFFECT_ARGUMENT;

    static const unsigned destination[] = {0x2bc, 0x2c4, 0x2cc, 0x2c0, 0x2c8, 0x2d0};
    static const uint32_t factor[] = {
        0x3c23d70a, 0x3c23d70a, 0x3dcccccd, 0x3c23d70a, 0x3c23d70a, 0x3dcccccd
    };
    float values[6];
    for (unsigned i = 0; i < 6; ++i) {
        values[i] = number(word(r + 0x13c + i * 4));
        if (!isfinite(values[i])) return GH_EFFECT_UNSUPPORTED;
    }

    fenv_t env;
    if (feholdexcept(&env)) return GH_EFFECT_UNSUPPORTED;
    if (fesetround(FE_TONEAREST)) {
        (void)fesetenv(&env);
        return GH_EFFECT_UNSUPPORTED;
    }
    for (unsigned i = 0; i < 6; ++i) values[i] = multiply(values[i], number(factor[i]));
    const float scale_s = add(multiply((float)signed_byte(r[0x190]), number(0x3dcccccd)), 1.0f);
    const float scale_t = add(multiply((float)signed_byte(r[0x191]), number(0x3dcccccd)), 1.0f);
    const float angle = multiply((float)signed_byte(r[0x192]), number(0x3d00adfd));
    const float angular_speed = add(multiply((float)signed_byte(r[0x193]), number(0x3ba3d70a)), 0.0f);
    if (fesetenv(&env)) return GH_EFFECT_UNSUPPORTED;

    put(p + 0x300, parameter_40);
    put(p + 0x2b4, r[0x12e]);
    put(p + 0x2dc, (uint32_t)signed_byte(r[0x18c]));
    put(p + 0x2e0, r[0x18d]);
    put(p + 0x2b8, (uint32_t)signed_byte(r[0x18e]));
    p[0x2e8] = r[0x18f];
    store(p + 0x2ec, scale_s);
    store(p + 0x2f0, scale_t);
    store(p + 0x2f4, angle);
    store(p + 0x2f8, angular_speed);
    for (unsigned i = 0; i < 6; ++i) store(p + destination[i], values[i]);
    *frontier = 0;
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_resource_00306c78(const gh_game_effects *owner,
    unsigned index, gh_esp14_resources *out, int *found) {
    if (!owner || !owner->ready || !out || !found) return GH_EFFECT_ARGUMENT;
    *found = 0;
    *out = (gh_esp14_resources){0};
    if (index >= 64 || !owner->emitter_entries[index].bytes) return GH_EFFECT_OK;
    const gh_effect_view source = owner->emitter_entries[index];
    if (source.size < 16 || word(source.bytes) != 0x004d4645) return GH_EFFECT_BOUNDS;
    const size_t geometry = word(source.bytes + 4);
    const size_t texture = word(source.bytes + 8);
    const size_t motions = word(source.bytes + 12);
    if (geometry < 16 || geometry >= texture || texture >= motions || motions >= source.size)
        return GH_EFFECT_BOUNDS;
    if (texture - geometry < 4 || motions - texture < 4 || source.size - motions < 8 ||
        word(source.bytes + geometry) != 0x00726373 ||
        word(source.bytes + texture) != 0x00334d54 ||
        word(source.bytes + motions) != 0x004d4d45)
        return GH_EFFECT_UNSUPPORTED;
    out->geometry = (gh_bundle_entry){.bytes = source.bytes + geometry, .size = texture - geometry};
    out->texture = (gh_bundle_entry){.bytes = source.bytes + texture, .size = motions - texture};
    out->motions = (gh_effect_view){source.bytes + motions, source.size - motions};
    *found = 1;
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_model_create_0030fa98(gh_esp14_model *s,
    const gh_esp14_model_bindings *b, int *found, uint32_t *frontier) {
    if (!frontier || !found) return GH_EFFECT_ARGUMENT;
    *frontier = 0x30fa98;
    *found = 0;
    if (!s || !b || !b->pools || !b->work || !b->factory_serial) return GH_EFFECT_ARGUMENT;
    if (s->handle) {
        if (s->pools != b->pools || !s->object.ready) return GH_EFFECT_ARGUMENT;
        *found = 1;
        *frontier = 0;
        return GH_EFFECT_OK;
    }
    const gh_room_vector *all, *objects;
    *frontier = 0x30fb70;
    if (gh_room_registry_view(b->pools, 0x754c30, &all) ||
        gh_room_registry_view(b->pools, 0x754c58, &objects)) return GH_EFFECT_ARGUMENT;
    const uint32_t flags = word((const uint8_t *)b->work + 0x594);
    if (all->count == all->capacity || objects->count == objects->capacity || (flags & 0x10000u)) {
        *frontier = 0;
        return GH_EFFECT_OK;
    }

    gh_room_arena_allocation storage;
    *frontier = 0x3139e0;
    if (gh_room_arena_allocate_002a9538(b->pools, 0x754c10, flags, 0x4e0, 16, &storage))
        return GH_EFFECT_UNSUPPORTED;
    if (!storage.pointer) {
        *frontier = 0;
        return GH_EFFECT_OK;
    }

    uint8_t *p = storage.pointer;
    memset(p, 0, storage.bytes);
    gh_title_player_view view;
    if (gh_game_actor_construct_0014e990(p, storage.bytes, &view)) {
        (void)gh_room_arena_free_002a9680(b->pools, storage);
        return GH_EFFECT_UNSUPPORTED;
    }
    put(p + 0x214, 0x44b840);
    view.original_vtable214 = 0x44b840;
    view.initial_memory = GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;
    p[0x2fc] = (*b->factory_serial)++;
    p[0x2fe] = p[0x300] = 0;
    p[0x2ff] = p[0x301] = 5;
    put(p + 0x2d0, 0x10000000);
    /* 2C8 is the guest 754240 arena pointer. The actual arena allocation
     * above owns 754C10 separately; no pointer token is dereferenced. */
    s->object = (gh_game_title_player){
        .allocation = {storage.pointer, &b->pools->arenas[2]},
        .heap = b->pools->heap, .view = view, .ready = 1,
        .allocation_domain = GH_PLAYER_ALLOCATION_ROOM_ARENA, .arena_allocation = storage
    };
    s->pools = b->pools;
    *frontier = 0x30fcc0;
    if (gh_room_object_register(b->pools, &s->object, s, GH_ESP_MODEL_KIND, 9, &s->handle)) {
        (void)gh_game_title_player_release(&s->object);
        s->pools = NULL;
        return GH_EFFECT_UNSUPPORTED;
    }
    *found = 1;
    *frontier = 0;
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_model_initialize_002f1b60(gh_esp14_model *s,
    const gh_esp14_model_bindings *b, const gh_esp14_resources *r,
    int *original, uint32_t *frontier) {
    if (!frontier || !original) return GH_EFFECT_ARGUMENT;
    *original = 0;
    *frontier = 0x2b8820;
    if (!s || !b || !r || !s->handle || s->pools != b->pools ||
        !s->object.ready || !b->packet_heap || !b->fog_rgb) return GH_EFFECT_ARGUMENT;
    uint8_t *p = s->object.view.bytes;
    if (!s->virtual_initialized) {
        p[0x4d4] = 0;
        put(p + 0x254, word(p + 0x254) | 0x08000000u);
        put(p + 0x4a8, word(p + 0x4a8) | 0x10000000u);
        s->virtual_initialized = 1;
    }
    *frontier = 0x149738;
    if (!s->model.bound && gh_actor_model_bind_packet_inputs(&s->model, b->packet_heap, b->fog_rgb))
        return GH_EFFECT_ARGUMENT;
    gh_actor_model_status status = gh_actor_model_initialize_00149738(&s->model,
        &s->object.view, b->work, b->pools, &r->geometry, &r->texture, 0, 0, b->policy, original);
    if (status) {
        if (s->model.frontier) *frontier = s->model.frontier;
        return status == GH_ACTOR_MODEL_ARGUMENT ? GH_EFFECT_ARGUMENT : GH_EFFECT_UNSUPPORTED;
    }
    *frontier = 0;
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_model_dispose(gh_esp14_model *s) {
    if (!s) return GH_EFFECT_ARGUMENT;
    if (gh_actor_model_release(&s->model)) return GH_EFFECT_UNSUPPORTED;
    if (s->handle) {
        gh_room_pool_status status = gh_room_object_unregister(s->pools, s->handle, s);
        if (status && !(s->model_destroyed && status == GH_POOL_BOUNDS))
            return GH_EFFECT_UNSUPPORTED;
        s->handle = 0;
    }
    if (gh_game_title_player_release(&s->object)) return GH_EFFECT_UNSUPPORTED;
    *s = (gh_esp14_model){0};
    return GH_EFFECT_OK;
}

static unsigned half(const uint8_t *p) {
    return p[0] | (unsigned)p[1] << 8;
}

static void put_half(uint8_t *p, unsigned n) {
    p[0] = (uint8_t)n;
    p[1] = (uint8_t)(n >> 8);
}

gh_effect_status gh_esp14_model_move_002b8850(gh_esp14_model *child,
    const gh_boot_work *work, gh_static_transform_policy policy, uint32_t *frontier) {
    if (!frontier) return GH_EFFECT_ARGUMENT;
    *frontier = 0x2b8850;
    if (!child || !child->object.ready || !child->model.ready || !work)
        return GH_EFFECT_ARGUMENT;
    const uint8_t *p = child->object.view.bytes;
    if (!(word((const uint8_t *)work + 0x5d8) & 0x10000000u)) {
        if (word(p + 0x4d0) && !p[0x4d4]) {
            *frontier = 0x1582a0;
            return GH_EFFECT_UNSUPPORTED;
        }
        if (word(p + 0x2f8) & 1u) {
            *frontier = 0x150128;
            return GH_EFFECT_UNSUPPORTED;
        }
        if (gh_actor_model_update_hierarchy_00149eb8(&child->model, policy)) {
            *frontier = child->model.frontier ? child->model.frontier : 0x149eb8;
            return GH_EFFECT_UNSUPPORTED;
        }
    }
    *frontier = 0;
    return GH_EFFECT_OK;
}

static gh_effect_status model_pose(const gh_esp14_bindings *b, gh_esp14_instance *s,
    uint8_t *p, uint32_t *frontier) {
    float xyz[4], matrix[16], rotation[3] = {0};
    gh_effect_status status = gh_effect_node_world_position_native(
        b->effects, 0, s->slot, s->serial, xyz, b->model.policy, frontier);
    if (status) return status;
    float scale[3] = {number(word(p + 0x20)), number(word(p + 0x24)), number(word(p + 0x20))};
    status = gh_effect_node_set_scale_native(b->effects, 0, s->slot, s->serial, scale, frontier);
    if (status) return status;
    if (number(word(p + 0x20)) != 0 && number(word(p + 0x24)) != 0 &&
        number(word(p + 0x28)) != 0) {
        status = gh_effect_node_world_matrix_native(
            b->effects, 0, s->slot, s->serial, matrix, b->model.policy, frontier);
        if (status) return status;
        *frontier = 0x147c88;
        if (!b->atan_coeff || gh_actor_matrix_extract_euler_00147c88(
            matrix, b->atan_coeff, rotation, b->model.policy, frontier))
            return GH_EFFECT_UNSUPPORTED;
    }
    uint8_t *child = s->child.object.view.bytes;
    memcpy(s->child.object.view.alias_f0, xyz, 12);
    memcpy(child + 0x110, scale, 12);
    memcpy(child + 0x100, rotation, 12);
    memcpy(child + 0x240, p + 0x188, 16);
    return GH_EFFECT_OK;
}

static gh_effect_status configure_model_packets(gh_esp14_model *child,
    const uint8_t *p, uint32_t *frontier) {
    static const uint64_t alpha[] = {
        UINT64_C(0x8000000044), UINT64_C(0x8000000048), UINT64_C(0x80000000a1),
        UINT64_C(0x8000000041), UINT64_C(0x8000000068), UINT64_C(0x80000000a4),
        UINT64_C(0x8000000009), UINT64_C(0x8000000042), UINT64_C(0x8000000006)
    };
    *frontier = 0x14d700;
    if (p[0x247] >= sizeof(alpha) / sizeof(*alpha)) return GH_EFFECT_UNSUPPORTED;
    const uint32_t flags = word(p + 0x11c);
    uint8_t *object = child->object.view.bytes;
    unsigned nodes = object[0x2b1];
    if (!nodes) return GH_EFFECT_ARGUMENT;
    for (unsigned i = 0; i < nodes; ++i) {
        const gh_actor_node_packets *packet;
        if (gh_actor_model_packet_at(&child->model, i, &packet) || !packet->ready || !packet->node)
            return GH_EFFECT_UNSUPPORTED;
        memcpy(packet->node + 0x50, &alpha[p[0x247]], 8);
        memcpy(packet->node + 0x100, &alpha[p[0x247]], 8);
        if (flags & 0x80000u) {
            static const uint64_t test = 0x51011;
            memcpy(packet->node + 0x30, &test, 8);
            memcpy(packet->node + 0xe0, &test, 8);
        }
    }
    const unsigned blend_flags = (flags >> 13) & 3;
    if (blend_flags) {
        static const uint8_t mask[] = {0, 2, 5, 1};
        put_half(object + 0x2ac, mask[blend_flags]);
        put(object + 0x2c0, 3);
    }
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_initialize_002f1d58(gh_esp14_instance *s,
    const gh_esp14_bindings *b, const gh_effect_esp_view *v, unsigned slot,
    uint32_t parameter_40, int *original, uint32_t *frontier) {
    if (!frontier || !original) return GH_EFFECT_ARGUMENT;
    *frontier = 0x2f1d58;
    *original = 0;
    if (!s || !b || !b->effects || !v || v->type != 0x14 || v->payload_size != 0x400 ||
        !v->record.bytes || v->record.size < 0x2f0 || !v->serial || v->actor || v->selector)
        return GH_EFFECT_ARGUMENT;
    gh_effect_esp_view canonical;
    if (gh_effect_create_esp_at(b->effects, slot, &canonical) ||
        canonical.serial != v->serial || canonical.payload != v->payload ||
        canonical.record.bytes != v->record.bytes) return GH_EFFECT_ARGUMENT;
    if (s->stage && (s->effects != b->effects || s->slot != slot || s->serial != v->serial))
        return GH_EFFECT_ARGUMENT;
    if (s->complete) {
        *original = s->stage == 7;
        *frontier = 0;
        return GH_EFFECT_OK;
    }

    gh_effect_status status;
    uint8_t *p = v->payload;
    const uint8_t *record = v->record.bytes;
    if (!s->stage) {
        status = gh_esp14_init_prefix_002f1d58(p, v->record, parameter_40, frontier);
        if (status) return status;
        s->effects = b->effects;
        s->slot = slot;
        s->serial = v->serial;
        s->stage = 1;
    }
    if (s->stage == 1) {
        *frontier = 0x306c78;
        int found = 0;
        status = gh_esp14_resource_00306c78(b->effects, word(p + 0x2b4), &s->resources, &found);
        if (status) return status;
        if (!found) {
            s->complete = 1;
            *frontier = 0;
            return GH_EFFECT_OK;
        }
        if (word(p + 0x2dc) != 0) {
            *frontier = 0x2f1c04;
            return GH_EFFECT_UNSUPPORTED;
        }
        status = gh_esp14_model_create_0030fa98(&s->child, &b->model, &found, frontier);
        if (status) return status;
        if (!found) {
            s->complete = 1;
            *frontier = 0;
            return GH_EFFECT_OK;
        }
        s->stage = 2;
    }
    if (s->stage == 2) {
        int model_return;
        status = gh_esp14_model_initialize_002f1b60(&s->child, &b->model, &s->resources,
            &model_return, frontier);
        if (status) return status;
        if (!model_return) {
            *frontier = 0x312708;
            return GH_EFFECT_UNSUPPORTED;
        }
        s->stage = 3;
    }
    if (s->stage == 3) {
        if (p[0x2e8]) {
            const unsigned bank = half(p + 0x23a);
            const gh_esp14_nested_request request = {
                .request = {bank, half(p + 0x23c), &s->child.object.view, -1, -1},
                .group_74 = p[0x2e8], .config_40 = word(p + 0x300), .config_48 = word(p + 0x104)
            };
            *frontier = 0x306528;
            if (bank >= 0x8000 || !b->nested_create) return GH_EFFECT_UNSUPPORTED;
            int ignored;
            status = b->nested_create(b->context, &request, &s->child.model, &ignored, frontier);
            if (status) {
                if (!*frontier) *frontier = 0x306528;
                return status;
            }
        }
        s->stage = 4;
    }
    if (s->stage == 4) {
        uint8_t *child = s->child.object.view.bytes;
        if (record[0x196]) child[0x2b5] = record[0x196];
        if (record[0x197]) {
            put(child + 0x250, word(child + 0x250) | 0x10000u);
            put_half(child + 0x2ae, 6000);
        }
        put(p + 0x11c, word(p + 0x11c) & ~0x40000000u);
        status = model_pose(b, s, p, frontier);
        if (status) return status;
        s->stage = 5;
    }
    if (s->stage == 5) {
        status = configure_model_packets(&s->child, p, frontier);
        if (status) return status;
        if (word(p + 0x2b8)) {
            *frontier = 0x306cc8;
            return GH_EFFECT_UNSUPPORTED;
        }
        if (word(p + 0x2ec) != 0x3f800000 || word(p + 0x2f0) != 0x3f800000 ||
            (word(p + 0x2f4) & 0x7fffffffu) || (word(p + 0x2f8) & 0x7fffffffu)) {
            *frontier = 0x14d7d0;
            return GH_EFFECT_UNSUPPORTED;
        }
        uint8_t *child = s->child.object.view.bytes;
        put(child + 0x254, word(child + 0x254) | 0x08000000u);
        s->stage = 6;
    }
    if (s->stage == 6) {
        status = gh_esp14_model_move_002b8850(&s->child, b->model.work, b->model.policy, frontier);
        if (status) return status;
        s->stage = 7;
    }
    s->complete = 1;
    *original = 1;
    *frontier = 0;
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_trans_002f2758(gh_esp14_instance *s,
    const gh_effect_esp_view *v, uint32_t *frontier) {
    if (!frontier) return GH_EFFECT_ARGUMENT;
    *frontier = 0x2f2758;
    if (!s || !v || v->type != 0x14 || v->serial != s->serial || !s->complete ||
        !v->payload || v->payload_size != 0x400) return GH_EFFECT_ARGUMENT;
    if (s->child.handle) {
        if (v->actor) {
            if (!v->actor->bytes || v->actor->size < 0x254) return GH_EFFECT_BOUNDS;
            uint8_t *p = s->child.object.view.bytes;
            put(p + 0x250, (word(p + 0x250) & ~0x8002u) |
                (word(v->actor->bytes + 0x250) & 0x8002u));
        } else if (word(v->payload + 0x114)) {
            return GH_EFFECT_UNSUPPORTED;
        }
    }
    *frontier = 0;
    return GH_EFFECT_OK;
}

static float take_random_signed(uint32_t *seed) {
    *seed = *seed * UINT32_C(0x19660d) + UINT32_C(0x3c6ef35f);
    float base = number((*seed & 0x7fffffu) | 0x3f800000u);
    return add(add(add(base, base), -2.0f), -1.0f);
}

gh_effect_status gh_esp14_position_init_00301c20(uint8_t p[0x400], gh_effect_view record,
    int mirrored, float position[3], uint32_t *frontier) {
    if (!frontier) return GH_EFFECT_ARGUMENT;
    *frontier = 0x301c20;
    if (!p || !record.bytes || record.size < 0x2f0 || !position) return GH_EFFECT_ARGUMENT;
    const uint8_t *r = record.bytes;
    if (r[0x133] != 0x14 || !(word(r + 4) & 0x200000u)) return GH_EFFECT_UNSUPPORTED;
    uint32_t seed = word(p + 0x220);
    float random = take_random_signed(&seed);
    float angle = multiply(add((float)signed_byte(r[0x2b3]),
                               multiply((float)signed_byte(r[0x2b8]), random)), number(0x3d00adfd));
    random = take_random_signed(&seed);
    const float hundredth = number(0x3c23d70a);
    float amplitude = add(multiply(multiply((float)signed_byte(r[0x2b9]), hundredth), random), 1);
    random = take_random_signed(&seed);
    float step = multiply(add(multiply(multiply((float)signed_byte(r[0x2ba]),
        number(0x38d1b717)), random), hundredth), number(word(r + 0x2bc)));
    float sine, cosine;
    *frontier = 0x1f7ef0;
    if (gh_id_sincos_reference(angle, &sine, &cosine)) return GH_EFFECT_UNSUPPORTED;
    if (mirrored) amplitude = -amplitude;
    const float waves[3] = {sine, cosine, cosine};
    float next[3];
    for (unsigned i = 0; i < 3; ++i) {
        float base = i == 0 && mirrored ? -position[i] : position[i];
        next[i] = add(base, multiply(multiply(number(word(r + 0x2a4 + 4 * i)),
                                              waves[i]), amplitude));
        if (!isfinite(next[i])) return GH_EFFECT_UNSUPPORTED;
    }
    if (!isfinite(step) || !isfinite(amplitude)) return GH_EFFECT_UNSUPPORTED;
    put(p + 0x220, seed);
    for (unsigned i = 0; i < 3; ++i) store(p + 0x260 + i * 4, angle);
    store(p + 0x26c, amplitude);
    store(p + 0x270, step);
    memcpy(position, next, 12);
    *frontier = 0;
    return GH_EFFECT_OK;
}

static gh_effect_status wave(const uint8_t *r, const float angles[3],
    float amplitude, float out[3], uint32_t *frontier) {
    for (unsigned i = 0; i < 3; ++i) {
        float sine, cosine;
        *frontier = i ? 0x1f7fb8 : 0x1f7ef0;
        if (gh_id_sincos_reference(angles[i], &sine, &cosine)) return GH_EFFECT_UNSUPPORTED;
        out[i] = multiply(multiply(number(word(r + 0x2a4 + i * 4)), i ? cosine : sine), amplitude);
        if (!isfinite(out[i])) return GH_EFFECT_UNSUPPORTED;
    }
    return GH_EFFECT_OK;
}

static gh_effect_status advance_wave(const uint8_t *r, float *amplitude,
    float *step, float angle[3], uint32_t *frontier) {
    const float thousandth = number(0x3a83126f);
    const unsigned value = half(r + 0x2b6);
    const int signed_value = value < 0x8000 ? (int)value : (int)value - 0x10000;
    *amplitude = multiply(add(*amplitude, multiply((float)signed_value, thousandth)),
        add(multiply((float)signed_byte(r[0x2b5]), thousandth), 1));
    *step = multiply(*step, add(multiply((float)signed_byte(r[0x2b4]), thousandth), 1));
    for (unsigned i = 0; i < 3; ++i) {
        const float next = add(angle[i], multiply((float)signed_byte(r[0x2b0 + i]), *step));
        *frontier = 0x1f7d48;
        if (gh_id_angle_reference_001f7d48(next, &angle[i])) return GH_EFFECT_UNSUPPORTED;
    }
    return GH_EFFECT_OK;
}

static gh_effect_status position_move(gh_game_effects *effects, unsigned slot,
    const gh_effect_esp_view *v, int scaled, float factor,
    gh_static_transform_policy policy, uint32_t *frontier) {
    if (!frontier) return GH_EFFECT_ARGUMENT;
    *frontier = 0x2fd610;
    if (!effects || !v || !v->payload || v->payload_size != 0x400 ||
        !v->record.bytes || v->record.size < 0x2f0 || policy != GH_STATIC_BINARY32_REFERENCE ||
        (scaled != 0 && scaled != 1) || !isfinite(factor)) return GH_EFFECT_ARGUMENT;
    const uint8_t *r = v->record.bytes;
    const uint32_t flags = word(r + 4);
    if (v->type != 0x14 || (flags & 0x10200000u) != 0x00200000u || (flags & 0x100000u))
        return GH_EFFECT_UNSUPPORTED;
    gh_effect_esp_view canonical;
    if (gh_effect_create_esp_at(effects, slot, &canonical) ||
        canonical.serial != v->serial || canonical.payload != v->payload)
        return GH_EFFECT_ARGUMENT;
    uint8_t *p = v->payload;
    float pos[3], accumulated[4], last_delta[4];
    memcpy(accumulated, p + 0x290, 16);
    memcpy(last_delta, p + 0x280, 16);
    for (unsigned i = 0; i < 3; ++i) {
        const float velocity = number(word(p + 0x124 + i * 4));
        pos[i] = add(number(word(p + 0x40 + i * 4)), scaled ? multiply(velocity, factor) : velocity);
    }
    if (scaled && half(p + 0x22e) == half(p + 0x230)) {
        for (unsigned i = 0; i < 4; ++i) {
            const float delta = multiply(last_delta[i], factor);
            if (i < 3) pos[i] = add(pos[i], delta);
            accumulated[i] = add(accumulated[i], delta);
            if (!isfinite(accumulated[i])) return GH_EFFECT_UNSUPPORTED;
        }
        gh_effect_status status = gh_effect_node_set_position_native(effects, 0, slot, v->serial, pos, frontier);
        if (status) return status;
        memcpy(p + 0x290, accumulated, 16);
        return GH_EFFECT_OK;
    }

    float angle[3], amplitude = number(word(p + 0x26c)), step = number(word(p + 0x270));
    float prior[3], current[3], next[3];
    memcpy(angle, p + 0x260, 12);
    gh_effect_status status = wave(r, angle, amplitude, prior, frontier);
    if (status) return status;
    for (unsigned i = 0; i < 3; ++i) pos[i] = add(add(pos[i], -accumulated[i]), -prior[i]);
    status = advance_wave(r, &amplitude, &step, angle, frontier);
    if (status) return status;
    status = wave(r, angle, amplitude, current, frontier);
    if (status) return status;
    float lookahead_angle[3], lookahead_amplitude = amplitude, lookahead_step = step;
    memcpy(lookahead_angle, angle, 12);
    status = advance_wave(r, &lookahead_amplitude, &lookahead_step, lookahead_angle, frontier);
    if (status) return status;
    status = wave(r, lookahead_angle, lookahead_amplitude, next, frontier);
    if (status) return status;
    for (unsigned i = 0; i < 3; ++i) {
        pos[i] = add(pos[i], current[i]);
        last_delta[i] = add(next[i], -current[i]);
    }
    status = gh_effect_node_set_position_native(effects, 0, slot, v->serial, pos, frontier);
    if (status) return status;
    memcpy(p + 0x260, angle, 12);
    store(p + 0x26c, amplitude);
    store(p + 0x270, step);
    memcpy(p + 0x280, last_delta, 12);
    memset(p + 0x290, 0, 12);
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_position_move_002fd610(gh_game_effects *effects, unsigned slot,
    const gh_effect_esp_view *v, int scaled, float factor,
    gh_static_transform_policy policy, uint32_t *frontier) {
    fenv_t env;
    if (feholdexcept(&env)) return GH_EFFECT_UNSUPPORTED;
    if (fesetround(FE_TONEAREST)) {
        (void)fesetenv(&env);
        return GH_EFFECT_UNSUPPORTED;
    }
    gh_effect_status status = position_move(effects, slot, v, scaled, factor, policy, frontier);
    return fesetenv(&env) ? GH_EFFECT_UNSUPPORTED : status;
}

static gh_effect_status move_after_base(gh_esp14_instance *s,
    const gh_esp14_bindings *b, const gh_effect_esp_view *v, int active,
    uint32_t *frontier) {
    if (!frontier) return GH_EFFECT_ARGUMENT;
    *frontier = 0x2f2250;
    if (!s || !b || !v || v->type != 0x14 || v->serial != s->serial ||
        !s->complete || s->stage != 7 || !s->child.handle || b->effects != s->effects)
        return GH_EFFECT_ARGUMENT;
    gh_effect_esp_view canonical;
    if (gh_effect_create_esp_at(s->effects, s->slot, &canonical) ||
        canonical.serial != s->serial || canonical.payload != v->payload) return GH_EFFECT_ARGUMENT;
    uint8_t *child = s->child.object.view.bytes;
    if (!active) {
        put(child + 0x250, word(child + 0x250) | 2u);
        *frontier = 0;
        return GH_EFFECT_OK;
    }
    uint8_t *p = v->payload;
    if (!v->target120_known_null || word(p + 0x120)) {
        *frontier = 0x307118;
        return GH_EFFECT_UNSUPPORTED;
    }
    if (p[0x2e9] || (word(p + 0x2cc) & 0x7fffffffu) ||
        (word(p + 0x2d0) & 0x7fffffffu)) {
        *frontier = 0x1491d0;
        return GH_EFFECT_UNSUPPORTED;
    }
    const gh_actor_node_packets *packets[255];
    const unsigned count = child[0x2b1];
    if (!count) return GH_EFFECT_ARGUMENT;
    for (unsigned i = 0; i < count; ++i) {
        if (gh_actor_model_packet_at(&s->child.model, i, &packets[i]) || !packets[i]->node) {
            *frontier = 0x14b6f8;
            return GH_EFFECT_UNSUPPORTED;
        }
    }
    float uv[2] = {
        add(number(word(packets[0]->node + 0x38c)), number(word(p + 0x2bc))),
        add(number(word(packets[0]->node + 0x390)), number(word(p + 0x2c0)))
    };
    for (unsigned i = 0; i < 2; ++i) {
        if (!isfinite(uv[i])) return GH_EFFECT_UNSUPPORTED;
        if (uv[i] > 2) uv[i] = add(uv[i], -2);
        if (uv[i] < -2) uv[i] = add(uv[i], 2);
    }
    volatile float fade = p[0x24c] ? (float)((int)p[0x24c] - (int)p[0x24d]) / (float)p[0x24c] : 1;
    const float alpha = multiply(number(word(p + 0x194)), fade);
    if (!isfinite(alpha)) return GH_EFFECT_UNSUPPORTED;
    gh_effect_status status = model_pose(b, s, p, frontier);
    if (status) return status;
    store(child + 0x24c, alpha);
    for (unsigned i = 0; i < count; ++i) memcpy(packets[i]->node + 0x38c, uv, 8);
    *frontier = 0;
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_move_after_base_002f2250(gh_esp14_instance *s,
    const gh_esp14_bindings *b, const gh_effect_esp_view *v, int active,
    uint32_t *frontier) {
    fenv_t env;
    if (feholdexcept(&env)) return GH_EFFECT_UNSUPPORTED;
    if (fesetround(FE_TONEAREST)) {
        (void)fesetenv(&env);
        return GH_EFFECT_UNSUPPORTED;
    }
    gh_effect_status status = move_after_base(s, b, v, active, frontier);
    return fesetenv(&env) ? GH_EFFECT_UNSUPPORTED : status;
}

int gh_esp14_nested_sprite_is_bound(gh_effect_view record, const gh_title_player_view *actor,
    const gh_title_player_view *selector) {
    if (!record.bytes || record.size < 0x2f0 || record.bytes[0x133] != 0 ||
        record.bytes[0x136] != 1 || record.bytes[0xbe] != 0xfe ||
        !actor || !selector || actor != selector || !actor->bytes || actor->size < 0x4e0)
        return 0;
    const uint32_t vtable = word(actor->bytes + 0x214);
    const int retained = vtable == 0x41eca0 && (word(actor->bytes + 0x250) & 0x80000000u);
    return actor->original_vtable214 == 0x44b840 && (vtable == 0x44b840 || retained) &&
        half(actor->bytes + 0x2fe) == 0x500;
}

gh_effect_status gh_esp14_nested_emitter_pose_00302bb4(gh_game_effects *effects,
    unsigned slot, const gh_effect_emitter_view *v, const float atan_coeff[8],
    gh_static_transform_policy policy, uint32_t *frontier) {
    if (!frontier) return GH_EFFECT_ARGUMENT;
    *frontier = 0x302bb4;
    if (!effects || !v || !gh_esp14_nested_sprite_is_bound(v->record, v->actor, v->selector) ||
        v->payload_size != 0x260 || !v->payload || !atan_coeff)
        return GH_EFFECT_ARGUMENT;
    gh_effect_emitter_view canonical;
    if (gh_effect_create_emitter_at(effects, slot, &canonical) ||
        canonical.serial != v->serial || canonical.payload != v->payload ||
        canonical.actor != v->actor || canonical.record.bytes != v->record.bytes)
        return GH_EFFECT_ARGUMENT;
    /* 302B44 selects the other actor/translation branch when record bit22
     * is set. 302BC8 and 302BDC append the explicit config pose to +80. */
    if (word(v->record.bytes + 4) & 0x400000u) return GH_EFFECT_UNSUPPORTED;
    for (unsigned i = 0; i < 3; ++i) {
        if ((word(v->payload + 0x1b0 + i * 4) & 0x7fffffffu) ||
            (word(v->payload + 0x1c0 + i * 4) & 0x7fffffffu)) {
            *frontier = 0x147728;
            return GH_EFFECT_UNSUPPORTED;
        }
    }
    float matrix[16], angles[3];
    memcpy(matrix, v->actor->bytes + 0x80, sizeof(matrix));
    *frontier = 0x147c88;
    if (gh_actor_matrix_extract_euler_00147c88(matrix, atan_coeff, angles, policy, frontier))
        return GH_EFFECT_UNSUPPORTED;
    gh_effect_status status = gh_effect_node_set_euler_native(effects, 1, slot, v->serial, angles, frontier);
    if (status) return status;
    status = gh_effect_node_set_position_native(effects, 1, slot, v->serial, matrix + 12, frontier);
    if (status) return status;
    put(v->payload + 0x114, word(v->payload + 0x114) | 0x00800000u);
    *frontier = 0;
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_nested_sprite_alpha_002fec38(const gh_effect_esp_view *v,
    float alpha, float fade, float *out, uint32_t *frontier) {
    if (!frontier) return GH_EFFECT_ARGUMENT;
    *frontier = 0x2ff1f0;
    if (!v || !v->payload || v->payload_size != 0x400 || !out ||
        !isfinite(alpha) || !isfinite(fade)) return GH_EFFECT_ARGUMENT;
    if (!gh_esp14_nested_sprite_is_bound(v->record, v->actor, v->selector) &&
        !gh_effect_event_part_is_bound(v->record, v->actor, v->selector)) return GH_EFFECT_UNSUPPORTED;
    const uint8_t *p = v->actor->bytes;
    float result;
    if (v->payload[0x11c] & 0x80u) {
        result = multiply(alpha, fade);
    } else if (word(p + 0x250) & 2u) {
        result = 0;
    } else {
        result = multiply(alpha, number(word(p + 0x24c)));
        static const unsigned offsets[] = {0x2b6, 0x2b7, 0x2bc};
        for (unsigned i = 0; i < 3; ++i) {
            volatile float factor = (float)p[offsets[i]] / 255.0f;
            result = multiply(result, factor);
        }
    }
    if (!isfinite(result)) return GH_EFFECT_UNSUPPORTED;
    *out = result;
    *frontier = 0;
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_destroy_hidden_002f27e8(gh_esp14_instance *s,
    gh_esp14_sound_detach detach, void *context, uint32_t *frontier) {
    if (!frontier) return GH_EFFECT_ARGUMENT;
    *frontier = 0x2f27e8;
    if (!s || !s->effects || !s->serial || !s->complete || s->stage != 7 ||
        !s->child.handle || !s->child.object.ready)
        return GH_EFFECT_ARGUMENT;
    if (s->esp_destroyed) {
        *frontier = 0;
        return GH_EFFECT_OK;
    }
    if (!s->child.model.ready) return GH_EFFECT_ARGUMENT;
    gh_effect_status status = gh_effect_create_esp_check_hidden_14(s->effects, s->slot, s->serial);
    if (status) return status;
    uint8_t *child = s->child.object.view.bytes;
    put(child + 0x250, word(child + 0x250) | 2u);
    if (!s->child_delete_requested) {
        if (!(word(child + 0x250) & 0x80000000u)) {
            *frontier = 0x2cbe18;
            if (!detach) return GH_EFFECT_UNSUPPORTED;
            uint32_t detail = 0;
            status = detach(context, &s->child.model, &detail);
            if (status) {
                if (detail) *frontier = detail;
                return status;
            }
            /* Actual VT44B840 +48 adjust0/+4C 14E928 is JR RA/NOP. */
            put(child + 0x250, word(child + 0x250) | 0x80000002u);
        }
        s->child_delete_requested = 1;
    }
    status = gh_effect_create_esp_finish_hidden_14(s, frontier);
    if (status) return status;
    s->esp_destroyed = 1;
    *frontier = 0;
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_child_destroy_002b8908(gh_esp14_instance *s,
    const gh_game_objects *room, const gh_game_objects *global, uint32_t *frontier) {
    if (!frontier) return GH_EFFECT_ARGUMENT;
    *frontier = 0x2b8908;
    if (!s || !room || !global || room == global || !s->child.object.ready ||
        !s->child.handle || !s->child_delete_requested || !s->esp_destroyed)
        return GH_EFFECT_ARGUMENT;
    gh_esp14_model *child = &s->child;
    if (child->model_destroyed) {
        *frontier = 0;
        return GH_EFFECT_OK;
    }
    uint8_t *p = child->object.view.bytes;
    if ((word(p + 0x250) & 0x80000002u) != 0x80000002u) return GH_EFFECT_ARGUMENT;

    /* 14E5D0 probes room, then global. 3139E0/3078E0 never loads a kind500
     * resource, so its actual present-holder branch is still strict. */
    gh_object_view resource;
    *frontier = 0x1fe9b8;
    gh_object_status lookup = gh_game_objects_find(room, 0x500, &resource);
    if (lookup == GH_OBJECT_MISSING) lookup = gh_game_objects_find(global, 0x500, &resource);
    if (lookup == GH_OBJECT_OK) {
        *frontier = 0x1fed78;
        return GH_EFFECT_UNSUPPORTED;
    }
    if (lookup != GH_OBJECT_MISSING) return GH_EFFECT_UNSUPPORTED;
    *frontier = 0x1496c0;
    if (gh_actor_model_release(&child->model)) return GH_EFFECT_UNSUPPORTED;
    /* 1496C0 reaches 150048; preserve its final base vtable while
     * the native retirement owner still pins the original actor bytes. */
    put(p + 0x214, 0x41eca0);
    child->model_destroyed = 1;
    *frontier = 0;
    return GH_EFFECT_OK;
}

gh_effect_status gh_esp14_collect_retired(gh_esp14_instance *s, int *collected,
    uint32_t *frontier) {
    if (!frontier || !collected) return GH_EFFECT_ARGUMENT;
    *frontier = 0x30f5a0;
    *collected = 0;
    if (!s || !s->effects || !s->child.pools) return GH_EFFECT_ARGUMENT;
    if (!s->esp_destroyed || !s->child.model_destroyed) {
        *frontier = 0;
        return GH_EFFECT_OK;
    }
    void *object = NULL;
    gh_room_pool_status status = gh_room_object_lookup(s->child.pools, s->child.handle,
        GH_ESP_MODEL_KIND, &s->child, &object);
    if (status == GH_POOL_OK) {
        *frontier = 0;
        return GH_EFFECT_OK;
    }
    if (status != GH_POOL_BOUNDS) return GH_EFFECT_ARGUMENT;

    int borrowed = 0;
    gh_effect_status result = gh_effect_create_actor_has_live_refs(s->effects,
        &s->child.object.view, &borrowed);
    if (result) return result;
    if (!borrowed) {
        if (gh_game_title_player_release(&s->child.object)) return GH_EFFECT_UNSUPPORTED;
        *s = (gh_esp14_instance){0};
        *collected = 1;
    }
    *frontier = 0;
    return GH_EFFECT_OK;
}
