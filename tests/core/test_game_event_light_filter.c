#include "game_event_data_prefix.h"
#include "game_event_camera_track.h"
#include "game_effects.h"
#include "game_event_light_filter.h"

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
    gh_scene_filters *filters = calloc(1, sizeof(*filters));
    CHECK(light && filters);
    CHECK(!gh_game_world_light_construct_002d6d80(light));
    CHECK(!gh_game_world_light_initialize_with_heap_002d6f40(light, &room, heap));
    CHECK(!gh_scene_filters_initialize_002993a0(filters, &room));

    gh_event_component_view env, camera, object_light, filter;
    CHECK(!gh_event_component_at(components, 0, &env));
    CHECK(!gh_event_component_at(components, 1, &camera));
    CHECK(!gh_event_component_at(components, 6, &object_light));
    CHECK(!gh_event_component_at(components, 7, &filter));
    gh_event_sequence_view lights, filter_keys;
    CHECK(!gh_event_sequence_at(&prefix, 6, &lights));
    CHECK(!gh_event_sequence_at(&prefix, 7, &filter_keys));
    CHECK(lights.count == 1 && lights.stride == 0x1c);
    CHECK(filter_keys.count == 10 && filter_keys.stride == 0x2e8);
    uint8_t original_light[0x10], original_filter[0x2d8], saved_keys[0x1d10], saved_light[0x1c];
    memcpy(original_light, light->scalar + 0x40, sizeof(original_light));
    memcpy(original_filter, filters->work.active140c8, sizeof(original_filter));
    memcpy(saved_keys, filter_keys.records, sizeof(saved_keys));
    memcpy(saved_light, lights.records, sizeof(saved_light));

    gh_event_light_filter state = {0};
    CHECK(!gh_event_object_light_init_00008648(&state, components, &prefix, light, &frontier));
    CHECK(!frontier && !memcmp(object_light.bytes, "OBJLIGHT", sizeof("OBJLIGHT")));
    CHECK(!memcmp(object_light.bytes + 0x24, original_light, sizeof(original_light)));
    CHECK(!gh_event_filter_init_00007040(&state, components, &prefix, filters, &frontier));
    CHECK(!frontier && !memcmp(filter.bytes, "FILTER", sizeof("FILTER")));
    CHECK(!memcmp(filter.bytes + 0x28, original_filter, sizeof(original_filter)));

    unsigned frames = 0;
    for (unsigned cut = 0; cut < prefix.camera_count; ++cut) {
        uint16_t length;
        CHECK(!gh_event_camera_cut_length_00004e60(&prefix, cut, &length));
        put_word(env.bytes + 0x10, cut);
        for (unsigned frame = 0; frame < length; ++frame) {
            float current = (float)frame;
            uint32_t frame_bits;
            memcpy(&frame_bits, &current, sizeof(frame_bits));
            put_word(env.bytes + 0x14, frame_bits);
            gh_event_camera_track track;
            CHECK(!gh_event_camera_select_00004c08(&link, &prefix, &track, &frontier));
            CHECK(!frontier && track.frame == current);
            CHECK(!gh_event_object_light_move_000086b0(&state, &frontier) && !frontier);
            CHECK(!memcmp(light->scalar + 0x40, cut ? lights.records + 0x0c : original_light, 0x0c));
            CHECK(!memcmp(light->scalar + 0x4c, original_light + 0x0c, 4));
            CHECK(!gh_event_filter_move_000070d0(&state, &frontier) && !frontier);
            uint8_t expected[0x2d8];
            memcpy(expected, filter_keys.records + cut * filter_keys.stride + 0x10, sizeof(expected));
            memcpy(expected + 0x208, original_filter + 0x208, 0x64);
            CHECK(!memcmp(filters->work.active140c8, expected, sizeof(expected)));
            ++frames;
        }
    }
    CHECK(frames == 1209);
    CHECK(!memcmp(lights.records, saved_light, sizeof(saved_light)));
    CHECK(!memcmp(filter_keys.records, saved_keys, sizeof(saved_keys)));
    camera.bytes[4] = 1;
    CHECK(gh_event_object_light_move_000086b0(&state, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0xbcc8 && !memcmp(light->scalar + 0x40, lights.records + 0x0c, 0x0c));

    CHECK(!gh_game_world_light_release_presets(light));
    free(light);
    free(filters);
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
    puts("PASS actual EV01 ObjectLight and Filter across 1209 primary camera frames with real R100 owners");
    return 0;
}
