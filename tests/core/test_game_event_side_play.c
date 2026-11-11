#include "game_event_data_prefix.h"
#include "game_event_audio.h"
#include "game_event_link.h"
#include "game_effects.h"
#include "game_event_side_play.h"
#include "game_stage.h"

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

static uint32_t word(const uint8_t *p) {
    uint32_t result;
    memcpy(&result, p, sizeof(result));
    return result;
}

static gh_game_event_status prefetch(void *context, const gh_event_audio_prefetch_request *request,
    uint32_t *frontier)
{
    gh_bgm_requests *bgm = context;
    gh_bgm_status status = gh_bgm_event_prefetch_002d1c78(bgm, request);
    *frontier = gh_bgm_requests_frontier(bgm);
    return status ? GH_EVENT_CALLEE : GH_EVENT_OK;
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
    gh_game_event_config config = {0};
    CHECK(!gh_game_event_config_from_elf(&config, elf, elf_bytes));
    CHECK(!gh_game_event_init_environment_002975f8(&event, &config, 1, elf, elf_bytes));
    CHECK(config.flags == 8);
    event.fields.data_number = 1;

    gh_heap *heap = NULL;
    gh_boot_work work = {0};
    gh_event_module module = {0};
    gh_event_resource resource = {0};
    gh_dvd_jobs dvd;
    CHECK(gh_heap_create(64 * 1024 * 1024, &heap) == GH_MEM_OK);
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
    static const uint32_t quit_imports[][2] = {
        {0x977c, 0x3a52f0}, {0x97ac, 0x2d8408}, {0x8b00, 0x2db518},
        {0x8b08, 0x2db620}, {0x81cc, 0x2aebf8}
    };
    unsigned imports_seen = 0;
    for (unsigned i = 0; i < link.reloc_count; ++i)
        for (unsigned j = 0; j < sizeof(quit_imports) / sizeof(*quit_imports); ++j)
            if (link.relocations[i].site == quit_imports[j][0]) {
                CHECK(link.relocations[i].resolved_original_address == quit_imports[j][1]);
                imports_seen |= 1u << j;
            }
    CHECK(imports_seen == 31);
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
    gh_event_component_view env, scr_light, layer, bgm_component;
    CHECK(!gh_event_component_at(components, 0, &env));
    CHECK(!gh_event_component_at(components, 5, &scr_light));
    CHECK(!gh_event_component_at(components, 9, &bgm_component));
    CHECK(!gh_event_component_at(components, 10, &layer));
    CHECK(!word(prefix.data + 0x20) && !word(prefix.data + 0x2c) && !word(prefix.data + 0x48));
    int relative;

    gh_game_world_light *light = calloc(1, sizeof(*light));
    gh_game_init_state *init = calloc(1, sizeof(*init));
    CHECK(light && init && !gh_game_world_light_construct_002d6d80(light));
    gh_game_init_state_initialize(init);
    CHECK(!gh_game_world_light_initialize_with_heap_002d6f40(light, &room, heap));
    uint8_t player_bytes[GH_TITLE_PLAYER_BYTES];
    gh_title_player_view player;
    CHECK(!gh_title_player_construct_00107e40(player_bytes, sizeof(player_bytes), &player));
    gh_light_player_parts parts = {.owner = player.bytes};
    CHECK(!gh_game_world_light_select_002d8408(light, 1, &parts));
    put_word(light->scalar + 0x16284, 0x10);
    memset(init->side_counter, 0x5a, sizeof(init->side_counter));
    uint8_t save_bytes[GH_CORE_SAVE_BYTES];
    gh_core_save_view save = {save_bytes, sizeof(save_bytes)};
    CHECK(!gh_core_save_system_init(save));
    put_word(save.bytes + 0x14, 0x92345678);
    gh_event_scr_light lighting = {0};
    CHECK(gh_event_scr_light_init_00008b20(&lighting, &prefix, light, init, save, NULL,
        &frontier, &relative) == GH_EVENT_UNSUPPORTED);
    CHECK(!relative && frontier == 0x2db6b0 && lighting.stage == 1);
    CHECK(word(scr_light.bytes + 0x18) == 1 && light->scalar[0x16170] == 0x1e);
    CHECK(!word(light->scalar + 0x16284) && word(save.bytes + 0x14) == 0x92345678);
    CHECK(!gh_event_scr_light_init_00008b20(&lighting, &prefix, light, init, save, &parts,
        &frontier, &relative) && !frontier);
    CHECK(!memcmp(scr_light.bytes, "SCRLIGHT", sizeof("SCRLIGHT")));
    CHECK(!word(light->scalar + 0x16288) && word(light->scalar + 0x78) == 13);
    CHECK(word(save.bytes + 0x14) == 0x12345678);
    for (unsigned i = 0; i < 16; ++i)
        CHECK(init->side_counter[i] == (i == 4 || i >= 8 ? 0 : 0x5a));
    uint8_t before[0x70];
    memcpy(before, light->scalar, sizeof(before));
    memset(light->scalar, 0xcc, sizeof(before));
    CHECK(!gh_event_scr_light_move_00008ef8(&lighting, &frontier) && !frontier);
    CHECK(!memcmp(light->scalar, before, 0x28) && !memcmp(light->scalar + 0x30, before + 0x30, 0xc));
    CHECK(!memcmp(light->scalar + 0x50, before + 0x50, 0xc));
    CHECK(!memcmp(light->scalar + 0x60, before + 0x60, 0xc));
    CHECK(word(light->scalar + 0x3c) == 0x3f800000 && word(light->scalar + 0x5c) == 0x3f800000);
    CHECK(word(light->scalar + 0x40) == 0xcccccccc);
    memcpy(light->scalar, before, sizeof(before));

    gh_room_pools pools = {0};
    CHECK(!gh_game_room_pools_initialize_002a5d80(&pools, &room, heap));
    gh_stage_textures *textures;
    CHECK(!gh_stage_textures_create(&textures));
    gh_game_scene_tables tables = {0};
    gh_scene_meshes meshes = {0};
    uint8_t serial;
    CHECK(!gh_scene_mesh_factory_serial_from_elf(elf, elf_bytes, &serial));
    meshes.factory_serial = &serial;
    gh_scene_mesh_construction_bindings construction = {textures, light, elf, elf_bytes};
    CHECK(!gh_scene_meshes_bind_construction(&meshes, &construction));
    CHECK(!gh_scene_meshes_construct_0031d1b0(&meshes, &tables, &work, &room, &pools, heap));
    CHECK(meshes.constructed_count == 156);
    uint32_t visible[8] = {0}, flags[156];
    for (unsigned i = 0; i < 156; ++i) {
        const gh_scene_mesh_object *object;
        CHECK(!gh_scene_meshes_object_at(&meshes, i, &object));
        uint8_t *bytes = object->allocation.pointer;
        flags[i] = (word(bytes + 0x250) & ~0x8002u) | (i % 3 ? 0x8002u : 0);
        put_word(bytes + 0x250, flags[i]);
        if (flags[i] & 2) visible[bytes[0x2fd] / 32] |= 0x80000000u >> (bytes[0x2fd] % 32);
    }
    CHECK(!gh_event_scr_layer_init_00008948(&prefix, &pools, &meshes, &frontier, &relative) && !frontier);
    CHECK(!memcmp(layer.bytes, "SCRLAYER", sizeof("SCRLAYER")));
    CHECK(!memcmp(layer.bytes + 0x58, visible, sizeof(visible)));
    for (unsigned i = 0; i < 156; ++i) {
        const gh_scene_mesh_object *object;
        CHECK(!gh_scene_meshes_object_at(&meshes, i, &object));
        CHECK(word((uint8_t *)object->allocation.pointer + 0x250) == (flags[i] & ~0x8002u));
    }
    uint8_t *first = meshes.first.allocation.pointer;
    put_word(first + 0x250, word(first + 0x250) | 0x8000u);
    CHECK(!gh_event_scr_layer_move_00008a40(&prefix, &pools, &meshes, &frontier, &relative));
    CHECK(word(first + 0x250) & 0x8000u);
    put_word(first + 0x250, word(first + 0x250) & ~0x8000u);

    gh_audio_registry *registry;
    gh_sound_control control = {0};
    gh_bgm_requests *bgm;
    CHECK(!gh_audio_registry_create(&registry) && !gh_audio_startup_load(registry, disc));
    CHECK(!gh_sound_control_from_startup(&control, disc, registry));
    CHECK(!gh_bgm_requests_create(registry, &control, disc, heap, &bgm));
    gh_adx_key key;
    CHECK(!gh_adx_key_from_elf(elf, elf_bytes, &key));
    CHECK(!gh_bgm_requests_enable_native(bgm, &key, 1, GH_BGM_GAIN_NATIVE_DB_REFERENCE));
    gh_event_audio_channel1 audio = {0};
    CHECK(!gh_event_audio_channel1_bind(&audio, &control, &dvd, heap));
    CHECK(!gh_event_audio_channel1_bind_registry(&audio, registry));
    CHECK(!gh_event_audio_channel1_bind_prefetch(&audio, prefetch, bgm));
    CHECK(!gh_event_audio_channel1_service(&audio, GH_EVENT_AUDIO_PREPARE_002D1900, 1, 0, &ready, &frontier));
    for (unsigned i = 0; i < 8 && gh_dvd_check(&dvd, audio.job); ++i) CHECK(!gh_dvd_pump(&dvd));
    CHECK(!gh_event_audio_channel1_tick_002d0068(&audio));
    CHECK(!gh_event_audio_channel1_tick_002d0068(&audio));
    CHECK(!gh_event_audio_channel1_service(&audio, GH_EVENT_AUDIO_START_002D1A30, 1, -1, &ready, &frontier));
    CHECK(gh_bgm_requests_active(bgm) == 1);
    control.flags_ac |= 0x10000000;
    control.flags_b0 |= 0x10000000;
    CHECK(!gh_event_bgm_init_000046d8(&prefix, bgm, &frontier, &relative) && !frontier);
    CHECK(!memcmp(bgm_component.bytes, "BGM", sizeof("BGM")));
    CHECK(!(control.flags_ac & 0x10000000u) && !(control.flags_b0 & 0x10000000u));
    gh_bgm_request_view playback;
    CHECK(!gh_bgm_requests_inspect(bgm, 0, &playback) && playback.linked &&
        playback.mode == 4 && playback.logical_id == 0x81 && !playback.transport.started);

    unsigned frames = 0;
    for (unsigned cut = 0; cut < 10; ++cut) {
        uint16_t count;
        CHECK(!gh_event_camera_cut_length_00004e60(&prefix, cut, &count));
        for (unsigned i = 0; i < count; ++i) {
            float frame = (float)i;
            put_word(env.bytes + 0x10, cut);
            memcpy(env.bytes + 0x14, &frame, sizeof(frame));
            put_word(env.bytes + 0x28, 1);
            CHECK(!gh_event_scr_light_move_00008ef8(&lighting, &frontier) && !frontier);
            CHECK(!gh_event_bgm_move_00004708(&prefix, &frontier) && !frontier);
            CHECK(!gh_event_scr_layer_move_00008a40(&prefix, &pools, &meshes, &frontier, &relative));
            gh_event_message_move_000081a8();
            ++frames;
        }
    }
    CHECK(frames == 1209 && gh_bgm_requests_active(bgm) == 1);
    CHECK(!gh_event_scr_layer_quit_00008af0(&prefix, &pools, &meshes, &frontier, &relative));
    CHECK(!frontier && !relative);
    for (unsigned i = 0; i < 156; ++i) {
        const gh_scene_mesh_object *object;
        CHECK(!gh_scene_meshes_object_at(&meshes, i, &object));
        const uint8_t *bytes = object->allocation.pointer;
        const unsigned id = bytes[0x2fd];
        const int hidden = (visible[id / 32] & (0x80000000u >> (id % 32))) != 0;
        CHECK(word(bytes + 0x250) == ((flags[i] & ~0x8002u) | (hidden ? 2u : 0)));
    }
    const unsigned first_id = first[0x2fd];
    put_word(layer.bytes + 0x38 + 4 * (first_id / 32), 0x80000000u >> (first_id % 32));
    put_word(layer.bytes + 0x58 + 4 * (first_id / 32), 0);
    CHECK(!gh_event_scr_layer_quit_00008af0(&prefix, &pools, &meshes, &frontier, &relative));
    CHECK((word(first + 0x250) & 0x8002u) == 0x8000u);

    gh_event_scr_light preserved = lighting;
    gh_game_event_config config_preserve = config;
    config_preserve.flags |= 0x100;
    put_word(save.bytes + 0x14, 0x92345678);
    memset(init->side_counter, 0x5a, sizeof(init->side_counter));
    put_word(light->scalar + 0x16288, 3);
    light->native_light_owners[255] = &player;
    CHECK(!gh_event_scr_light_quit_00009728(&preserved, &config_preserve, NULL, &frontier, &relative));
    CHECK(!frontier && word(save.bytes + 0x14) == 0x12345678 &&
        word(light->scalar + 0x16288) == 3 && light->native_light_owners[255] == &player);
    put_word(save.bytes + 0x14, 0x92345678);
    CHECK(gh_event_scr_light_quit_00009728(&lighting, &config, NULL, &frontier, &relative) == GH_EVENT_UNSUPPORTED);
    CHECK(!relative && frontier == 0x2db6b0 && lighting.quit_stage == 1);
    CHECK(word(save.bytes + 0x14) == 0x12345678 && !word(light->scalar + 0x78) &&
        !light->native_light_owners[255]);
    for (unsigned i = 0; i < 256; ++i) CHECK(word(light->scalar + 0x80 + i * 0x70) == 0xff);
    light->scalar[0x80 + 255 * 0x70 + 4] = 0x5a; /* A retry must not run the clear twice. */
    CHECK(!gh_event_scr_light_quit_00009728(&lighting, &config, &parts, &frontier, &relative));
    CHECK(!frontier && !word(light->scalar + 0x16288) && word(light->scalar + 0x78) == 13);
    CHECK(light->scalar[0x80 + 255 * 0x70 + 4] == 0x5a);
    for (unsigned i = 0; i < 16; ++i)
        CHECK(init->side_counter[i] == (i == 4 || i >= 8 ? 0 : 0x5a));

    gh_menu_state menu = {0};
    gh_game_scene_ui messages = {0};
    CHECK(!gh_scene_message_initialize_002aea18(&messages, &menu, &room, 1));
    gh_event_message_play message = {0};
    CHECK(gh_event_message_init_00008100(&message, &prefix, &config, &messages,
        0, 0xffff, NULL, NULL, &frontier, &relative) == GH_EVENT_UNSUPPORTED);
    CHECK(!relative && frontier == 0x2aeb68 && message.message_id == 0x1000 && message.stage == 1);
    CHECK(messages.active_head == -1 && messages.free_head == 0);
    gh_event_message_play tool_mode = {0};
    put_word(env.bytes + 0x0c, 1);
    CHECK(!gh_event_message_init_00008100(&tool_mode, &prefix, &config, &messages,
        0, 0xffff, NULL, NULL, &frontier, &relative));
    CHECK(!frontier && tool_mode.message_id == 0xffff && tool_mode.stage == 2);
    CHECK(!gh_event_message_quit_000081b0(&tool_mode, &frontier, &relative) && !frontier);

    CHECK(!gh_event_audio_channel1_shutdown(&audio));
    gh_bgm_requests_destroy(bgm);
    gh_audio_registry_destroy(registry);
    CHECK(!gh_scene_meshes_release_prefix(&meshes));
    gh_stage_textures_destroy(textures);
    CHECK(!gh_game_room_pools_release(&pools));
    CHECK(!gh_game_world_light_release_presets(light));
    free(init);
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
    puts("PASS EV01 side playback and Quit across 1209 frames, original WorldLight preset and both masks on 156 R100 scroll objects");
    return 0;
}
