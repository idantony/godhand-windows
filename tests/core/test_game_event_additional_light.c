#include "game_event_data_prefix.h"
#include "game_event_camera_track.h"
#include "game_effects.h"
#include "game_event_additional_light.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); exit(1); } \
} while (0)

static void put_word(uint8_t *p, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8));
}

static uint8_t *load(gh_disc *disc, const char *path, size_t *bytes) {
    size_t index;
    gh_disc_entry entry;
    CHECK(gh_disc_find(disc, path, &index) == GH_DISC_OK);
    CHECK(gh_disc_entry_at(disc, index, &entry) == GH_DISC_OK && entry.size <= SIZE_MAX);
    *bytes = (size_t)entry.size;
    uint8_t *data = malloc(*bytes);
    CHECK(data && gh_disc_read(disc, index, 0, data, *bytes) == GH_DISC_OK);
    return data;
}

static gh_game_event_status register_effects(void *context, const uint8_t *data, size_t size, uint32_t *frontier) {
    gh_game_effects *effects = context;
    gh_effect_status status = gh_game_effects_register_event_002fa0b0(effects, data, size);
    if (status) *frontier = effects->frontier;
    return status == GH_EFFECT_OK ? GH_EVENT_OK : GH_EVENT_CALLEE;
}

static uint16_t half(const uint8_t *p) {
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static uint32_t word(const uint8_t *p) {
    uint32_t result;
    memcpy(&result, p, sizeof(result));
    return result;
}

static float number(const uint8_t *p) {
    float result;
    memcpy(&result, p, sizeof(result));
    return result;
}

static unsigned key_time(const uint8_t *key, const unsigned *cut_start) {
    return cut_start[key[3]] + half(key + 6);
}

static unsigned verify_record(const uint8_t *record, const uint8_t *key,
    const uint8_t *next, const unsigned *cut_start, unsigned tick)
{
    CHECK(half(record) == key[0x0e] && half(record + 2) == half(key + 0x10));
    CHECK(word(record + 4) == 0 && word(record + 0x50) == 1 &&
        word(record + 0x54) == 0 && word(record + 0x58) == 0xf0800000);
    CHECK(word(record + 0x40) == word(key + 0x38));
    CHECK(half(record + 0x5c) == half(key + 0x42) && half(record + 0x5e) == half(key + 0x40));
    CHECK(word(record + 0x60) == word(key + 0x3c));
    unsigned changed = 0;
    for (unsigned v = 0; v < 3; ++v) {
        for (unsigned k = 0; k < 3; ++k) {
            const unsigned offset = 0x14 + 12 * v + 4 * k;
            const float start = number(key + offset);
            float expected = start;
            if (next && (key[0x0d] & 4)) {
                const float end = number(next + offset);
                const float duration = (float)(key_time(next, cut_start) - key_time(key, cut_start));
                const float remain = (float)(key_time(next, cut_start) - tick) / duration;
                const float weight = 1.0f - remain;
                if (start != end) expected = (1.0f - weight) * start + weight * end;
            }
            const uint8_t *value = record + 0x10 + v * 0x10 + k * 4;
            uint32_t expected_bits;
            memcpy(&expected_bits, &expected, sizeof(expected_bits));
            CHECK(word(value) == expected_bits);
            changed += expected != start;
        }
        CHECK(word(record + 0x1c + v * 0x10) == 0x3f800000);
    }
    return changed;
}

int main(int argc, char **argv) {
    CHECK(argc == 2);
    gh_disc *disc = NULL;
    CHECK(gh_disc_open(argv[1], &disc) == GH_DISC_OK);
    const size_t elf_bytes = (size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf = malloc(elf_bytes);
    CHECK(elf && gh_disc_read_boot_elf(disc, 0, elf, elf_bytes) == GH_DISC_OK);
    gh_game_event event = {0};
    CHECK(gh_game_event_from_elf(&event, elf, elf_bytes) == GH_EVENT_OK);
    event.fields.data_number = 1;

    gh_heap *heap = NULL;
    gh_boot_work work = {0};
    gh_event_module module = {0};
    gh_event_resource resource = {0};
    gh_dvd_jobs dvd;
    CHECK(gh_heap_create(2 * 1024 * 1024, &heap) == GH_MEM_OK);
    gh_dvd_jobs_init(&dvd, disc);
    CHECK(gh_game_event_module_begin_00297378(&module, &event, &work, &dvd, heap) == GH_EVENT_OK);
    int ready = 0;
    for (unsigned i = 0; i < 8 && !ready; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_game_event_module_poll_00297428(&module, &ready) == GH_EVENT_OK);
    }
    CHECK(ready && module.loaded);

    gh_event_link link = {0};
    CHECK(!gh_game_event_link_002be890(&link, &module, elf, elf_bytes));
    gh_event_components *components = &link.components;
    CHECK(gh_event_components_construct_00297680(components, 1) == GH_EVENT_OK);
    CHECK(gh_game_event_resource_begin_00297540(&resource, &event, &dvd, heap) == GH_EVENT_OK);
    ready = 0;
    for (unsigned i = 0; i < 1024 && !ready; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_game_event_resource_poll_002975d0(&resource, &ready) == GH_EVENT_OK);
    }
    CHECK(ready && resource.loaded && resource.file_bytes == 873872);

    gh_event_data_prefix prefix = {0};
    uint32_t frontier;
    CHECK(!gh_event_data_bind_prefix_002977e8(&prefix, components, &resource, &frontier));
    CHECK(!gh_event_actor_cuts_bind_00297864(&prefix, &frontier));
    size_t core_size, room_size;
    uint8_t *core_data = load(disc, "etc/core.dat", &core_size);
    uint8_t *room_data = load(disc, "st1/r100.dat", &room_size);
    gh_bundle room;
    CHECK(gh_bundle_open(room_data, room_size, &room) == GH_BUNDLE_OK);
    gh_game_effects *effects = calloc(1, sizeof(*effects));
    uint8_t *esp = calloc(1, 0x100180), *emitters = calloc(1, 0x26060);
    CHECK(effects && esp && emitters);
    CHECK(gh_game_effects_initialize_00305a68(effects, esp, 0x100180, emitters, 0x26060,
                                             core_data, core_size, &room) == GH_EFFECT_OK);
    CHECK(!gh_event_data_bind_tail_00297880(&prefix, components, register_effects, effects, &frontier));
    gh_game_world_light *light = calloc(1, sizeof(*light));
    CHECK(light && !gh_game_world_light_construct_002d6d80(light));
    CHECK(!gh_game_world_light_initialize_with_heap_002d6f40(light, &room, heap));
    const uint8_t *preset;
    size_t preset_count;
    CHECK(!gh_game_world_light_preset_records(light, 0, &preset, &preset_count));
    CHECK(preset && preset_count == 13);
    /* Prime the live records with the R100 data before the original PLAY
     * clear. The normal scene selector's player attachments are outside
     * this fixture. The Event's 2D8810 refresh copies the same preset. */
    memcpy(light->scalar + 0x80, preset, preset_count * 0x70);
    put_word(light->scalar + 0x78, (uint32_t)preset_count);

    gh_event_component_view env, camera, component;
    CHECK(!gh_event_component_at(components, 0, &env));
    CHECK(!gh_event_component_at(components, 1, &camera));
    CHECK(!gh_event_component_at(components, 14, &component));
    gh_event_sequence_view track;
    CHECK(!gh_event_sequence_at(&prefix, 14, &track));
    CHECK(track.count == 42 && track.stride == 0x44 && track.bytes == 42 * 0x44);
    uint8_t serialized[42 * 0x44], global[0x78];
    memcpy(serialized, track.records, sizeof(serialized));
    memcpy(global, light->scalar, sizeof(global));
    gh_event_additional_light state = {0};
    CHECK(!gh_event_additional_light_init_000040d8(&state, &link, &prefix, light, &frontier));
    CHECK(!frontier && !memcmp(component.bytes, "ADDLIGHT", sizeof("ADDLIGHT")));
    CHECK(!word(light->scalar + 0x78) && !memcmp(global, light->scalar, sizeof(global)));
    for (unsigned i = 0; i < 256; ++i) {
        const uint8_t *record = light->scalar + 0x80 + i * 0x70;
        CHECK(record[0] == 0xff);
        for (unsigned j = 1; j < 0x70; ++j) CHECK(!record[j]);
        CHECK(!light->native_light_owners[i]);
    }
    unsigned cut_start[11] = {0};
    for (unsigned cut = 0; cut < 10; ++cut) {
        uint16_t length;
        CHECK(!gh_event_camera_cut_length_00004e60(&prefix, cut, &length));
        cut_start[cut + 1] = cut_start[cut] + length;
    }

    const uint8_t *current[4] = {0};
    unsigned order[3] = {0}, count = 0, next_row = 0, changed = 0;
    for (unsigned cut = 0; cut < 10; ++cut) {
        put_word(env.bytes + 0x10, cut);
        for (unsigned tick = cut_start[cut]; tick < cut_start[cut + 1]; ++tick) {
            const float frame = (float)(tick - cut_start[cut]);
            uint32_t frame_bits;
            memcpy(&frame_bits, &frame, sizeof(frame_bits));
            put_word(env.bytes + 0x14, frame_bits);
            gh_event_camera_track camera_track;
            CHECK(!gh_event_camera_select_00004c08(&link, &prefix, &camera_track, &frontier));
            CHECK(!frontier && camera_track.frame == frame);
            while (next_row < track.count) {
                const uint8_t *key = track.records + next_row * track.stride;
                if (key_time(key, cut_start) != tick) break;
                const unsigned id = half(key + 0x10);
                unsigned pos = 0;
                while (pos < count && order[pos] != id) ++pos;
                if (pos == count) ++count;
                while (++pos < count) order[pos - 1] = order[pos];
                order[count - 1] = id;
                current[id] = key;
                ++next_row;
            }
            CHECK(!gh_event_additional_light_move_00004140(&state, &work, &frontier) && !frontier);
            CHECK(word(light->scalar + 0x78) == count + (count ? preset_count : 0));
            for (unsigned i = 0; i < count; ++i) {
                const unsigned id = order[i];
                const uint8_t *next = NULL;
                for (unsigned j = next_row; j < track.count; ++j) {
                    const uint8_t *key = track.records + j * track.stride;
                    if (half(key + 0x10) == id) { next = key; break; }
                }
                changed += verify_record(light->scalar + 0x80 + i * 0x70,
                    current[id], next, cut_start, tick);
                CHECK(!light->native_light_owners[i]);
            }
            if (count) CHECK(!memcmp(light->scalar + 0x80 + count * 0x70, preset, preset_count * 0x70));
        }
    }
    CHECK(cut_start[10] == 1209 && next_row == 42 && changed > 0);
    CHECK(!memcmp(track.records, serialized, sizeof(serialized)));
    CHECK(!memcmp(light->scalar, global, sizeof(global)));
    camera.bytes[4] = 1;
    CHECK(gh_event_additional_light_move_00004140(&state, &work, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x4ed0 && word(light->scalar + 0x78) == count + preset_count);

    /* Second invocation at the first positive key with original 2D90B0's
     * Work gate set. Existing room preset records stay out of the live pool. */
    CHECK(!gh_event_additional_light_init_000040d8(&state, &link, &prefix, light, &frontier));
    CHECK(!word(light->scalar + 0x78));
    put_word((uint8_t *)&work + 0x5e4, 0x08000000);
    put_word(env.bytes + 0x10, 1);
    put_word(env.bytes + 0x14, 0);
    gh_event_camera_track first;
    CHECK(!gh_event_camera_select_00004c08(&link, &prefix, &first, &frontier));
    CHECK(!gh_event_additional_light_move_00004140(&state, &work, &frontier));
    CHECK(!frontier && word(light->scalar + 0x78) == 3);

    /* Event.playStart sets this different gate. 2D8810 runs its compaction
     * and skips appending the 13 R100 room lights. */
    CHECK(!gh_event_additional_light_init_000040d8(&state, &link, &prefix, light, &frontier));
    put_word((uint8_t *)&work + 0x5e4, 0x20000001);
    for (unsigned cut = 0; cut < 10; ++cut) {
        put_word(env.bytes + 0x10, cut);
        for (unsigned tick = cut_start[cut]; tick < cut_start[cut + 1]; ++tick) {
            float frame = (float)(tick - cut_start[cut]);
            uint32_t bits;
            memcpy(&bits, &frame, sizeof(bits));
            put_word(env.bytes + 0x14, bits);
            CHECK(!gh_event_camera_select_00004c08(&link, &prefix, &first, &frontier));
            CHECK(!gh_event_additional_light_move_00004140(&state, &work, &frontier));
            CHECK(!frontier && word(light->scalar + 0x78) == (cut ? 3u : 0u));
            CHECK(light->scalar[0x80 + 3 * 0x70] == 0xff);
        }
    }

    CHECK(!gh_game_world_light_release_presets(light));
    free(light);
    free(effects);
    free(esp);
    free(emitters);
    free(core_data);
    free(room_data);
    CHECK(!gh_game_event_resource_abort(&resource));
    CHECK(!gh_game_event_module_abort(&module));
    CHECK(!gh_heap_used(heap) && !gh_heap_destroy(heap));
    gh_disc_close(disc);
    free(elf);
    printf("PASS actual EV01 AdditionalLight 1209 frames, 42 keys, %u interpolated component writes, canonical R100 list\n", changed);
    return 0;
}
