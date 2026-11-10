#include "game_event_play_env.h"
#include "game_event_data_prefix.h"
#include "game_event_resource.h"
#include "game_event_timing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); exit(1); } \
} while (0)

static uint32_t get(const uint8_t *bytes) {
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
           (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

int main(int argc, char **argv) {
    CHECK(argc == 2);
    gh_disc *disc = NULL;
    CHECK(gh_disc_open(argv[1], &disc) == GH_DISC_OK);
    size_t elf_bytes = (size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf = malloc(elf_bytes);
    CHECK(elf && gh_disc_read_boot_elf(disc, 0, elf, elf_bytes) == GH_DISC_OK);

    gh_game_event event = {0};
    gh_game_event_config config = {0};
    CHECK(gh_game_event_from_elf(&event, elf, elf_bytes) == GH_EVENT_OK);
    CHECK(gh_game_event_config_from_elf(&config, elf, elf_bytes) == GH_EVENT_OK);
    CHECK(gh_game_event_init_environment_002975f8(&event, &config, 1, elf, elf_bytes) == GH_EVENT_OK);
    CHECK(config.record_address == 0x3c055c && config.flags == 8);

    gh_boot_work work = {0};
    uint8_t *raw_work = (uint8_t *)&work;
    gh_heap *heap = NULL;
    CHECK(gh_heap_create(1024 * 1024, &heap) == GH_MEM_OK);
    gh_dvd_jobs dvd;
    gh_dvd_jobs_init(&dvd, disc);
    gh_event_module module = {0};
    CHECK(gh_game_event_module_begin_00297378(&module, &event, &work, &dvd, heap) == GH_EVENT_OK);
    int ready = 0;
    for (unsigned i = 0; i < 8 && !ready; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_game_event_module_poll_00297428(&module, &ready) == GH_EVENT_OK);
    }
    CHECK(ready && module.loaded && module.file_bytes == 80431);
    gh_event_components components = {0};
    CHECK(gh_event_components_bind(&components, &module, elf, elf_bytes) == GH_EVENT_OK);
    CHECK(gh_event_components_construct_00297680(&components, 1) == GH_EVENT_OK);

    gh_event_component_view env, track;
    CHECK(gh_event_component_at(&components, 0, &env) == GH_EVENT_OK);
    CHECK(gh_event_component_at(&components, 13, &track) == GH_EVENT_OK);
    memset(env.bytes + 0x10, 0xa5, 16);
    env.bytes[0x20] = 0x5a;
    raw_work[0x5db] = 0xc2;
    gh_display_state display = {.initialized = 1, .screen_width = 512, .gs_y_origin = 1824};
    gh_screen_fade fade = {0};
    uint32_t frontier = 0;
    CHECK(gh_event_env_init_play_00005930(&components, &config, &work, &fade, NULL, &frontier) == GH_EVENT_ARGUMENT);
    CHECK(frontier == 0x307208 && raw_work[0x5db] == 0xc2 && get(env.bytes + 0x10) == 0xa5a5a5a5);
    CHECK(gh_event_env_init_play_00005930(&components, &config, &work, &fade, &display, &frontier) == GH_EVENT_OK);
    CHECK(frontier == 0 && raw_work[0x5db] == 0x82 && env.bytes[0x20] == 0x5a);
    CHECK(get(env.bytes + 0x10) == UINT32_MAX && get(env.bytes + 0x14) == 0xbf800000);
    CHECK(!get(env.bytes + 0x18) && !get(env.bytes + 0x1c) && get(env.bytes + 0x2c) == 255);
    CHECK(fade.state == 9 && fade.time == 15 && fade.count == 0 && fade.color_from == 0xff000000 && !fade.color_to);

    size_t resource;
    uint8_t header[0x60];
    CHECK(gh_disc_find(disc, "event/ev01.dat", &resource) == GH_DISC_OK);
    CHECK(gh_disc_read(disc, resource, 0, header, sizeof(header)) == GH_DISC_OK);
    CHECK(!get(header + 0x4c));
    track.bytes[0x0f] = 0x56;
    memset(track.bytes + 0x10, 0xa5, 9);
    track.bytes[0x19] = 0x78;
    CHECK(gh_event_fade_init_absent_00006c70(&components, header, 0x4f, &frontier) == GH_EVENT_BOUNDS);
    CHECK(frontier == 0x297f80 && track.bytes[0x18] == 0xa5);
    header[0x4c] = 0x10;
    CHECK(gh_event_fade_init_absent_00006c70(&components, header, sizeof(header), &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(track.bytes[0x18] == 0xa5);
    header[0x4c] = 0;
    CHECK(gh_event_fade_init_absent_00006c70(&components, header, sizeof(header), &frontier) == GH_EVENT_OK);
    CHECK(track.bytes[0x0f] == 0x56 && track.bytes[0x19] == 0x78);
    CHECK(!get(track.bytes + 0x10) && !get(track.bytes + 0x14) && !track.bytes[0x18]);

    const gh_screen_fade_context context = {512, 1824, 0, 0};
    gh_fade_draw draw;
    for (unsigned tick = 0; tick <= 15; ++tick) {
        CHECK(gh_event_fade_move_absent_00006ca0(&components, &fade, &frontier) == GH_EVENT_OK);
        CHECK(gh_screen_fade_step_00307388(&fade, &context, &draw) == GH_FADE_OK);
        CHECK(draw.visible && draw.rgba == ((255 - tick * 17) << 24));
        CHECK(draw.test == (tick < 15 ? 0x30000u : 0x70000u));
    }
    CHECK(!fade.state && gh_screen_fade_step_00307388(&fade, &context, &draw) == GH_FADE_OK && !draw.visible);
    raw_work[0x564] = 1;
    CHECK(gh_event_env_init_play_00005930(&components, &config, &work, &fade, &display, &frontier) == GH_EVENT_OK);
    CHECK(fade.time == 30);
    track.bytes[0x18] = 1;
    CHECK(gh_event_fade_move_absent_00006ca0(&components, &fade, &frontier) == GH_EVENT_OK && !fade.state && !track.bytes[0x18]);

    gh_event_resource loaded = {0};
    CHECK(!gh_game_event_resource_begin_00297540(&loaded, &event, &dvd, heap));
    ready = 0;
    for (unsigned i = 0; i < 8 && !ready; ++i) {
        CHECK(!gh_dvd_pump(&dvd));
        CHECK(!gh_game_event_resource_poll_002975d0(&loaded, &ready));
    }
    CHECK(ready && loaded.loaded);
    gh_event_data_prefix prefix = {0};
    CHECK(!gh_event_data_bind_prefix_002977e8(&prefix, &components, &loaded, &frontier));
    CHECK(prefix.camera_count == 10);

    gh_event_timing timing = {0};
    gh_heap *small = NULL;
    CHECK(!gh_heap_create(64, &small));
    int original = 1;
    CHECK(!gh_event_timing_prepare_000054c0(&timing, &components, small, &original));
    CHECK(!original && !timing.allocation.pointer && !gh_heap_used(small));
    CHECK(!gh_heap_destroy(small));
    size_t before = gh_heap_used(heap);
    CHECK(!gh_event_timing_prepare_000054c0(&timing, &components, heap, &original) && original);
    CHECK(timing.count == 10 && gh_heap_used(heap) == before + 160 && !get(env.bytes + 4));
    const uint8_t *timing_bytes = timing.allocation.pointer;
    for (size_t cut = 0; cut < 10; ++cut) {
        for (size_t byte = 0; byte < 16; ++byte) {
            unsigned expected = byte == 1 ? (cut == 9 ? 255 : (unsigned)cut + 1) : 0;
            CHECK(timing_bytes[cut * 16 + byte] == expected);
        }
    }
    CHECK(!gh_event_timing_prepare_000054c0(&timing, &components, heap, &original) && original);
    CHECK(timing_bytes == timing.allocation.pointer && gh_heap_used(heap) == before + 160);
    CHECK(!gh_event_timing_abort(&timing) && gh_heap_used(heap) == before);

    gh_event_play_state play = {0};
    event.fields.state = 3;
    event.fields.phase = 0;
    raw_work[0x564] = 0;
    CHECK(gh_event_play_00296c28(&play, &event, &components, &config, &work,
        &fade, &display, &original, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(!original && frontier == 0x296d08 && !event.fields.phase);
    CHECK(!(event.fields.flags & 4) && get(env.bytes + 0x34) == 0x298088);
    CHECK((raw_work[0x5e4] & 1) && fade.time == 15);
    CHECK(!gh_screen_fade_step_00307388(&fade, &context, &draw) && fade.count == 1);
    CHECK(gh_event_play_00296c28(&play, &event, &components, &config, &work,
        &fade, &display, &original, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(fade.count == 1 && frontier == 0x296d08);
    uint8_t object_bytes[0x300] = {0};
    uint8_t save_bytes[0xaf] = {0};
    object_bytes[0x2fe] = 0x67;
    object_bytes[0x2ff] = 0x06;
    object_bytes[0x250] = 0x40;
    gh_title_player_view object = {.bytes=object_bytes,.size=sizeof(object_bytes)};
    gh_core_save_view save = {save_bytes, sizeof(save_bytes)};
    CHECK(!gh_event_env_extra_move_object_00006b08(&components, &object, save, &frontier));
    CHECK(object_bytes[0x250] == 0x40);
    save_bytes[0xae] = 1;
    CHECK(!gh_event_env_extra_move_object_00006b08(&components, &object, save, &frontier));
    CHECK(object_bytes[0x250] == 0x42);
    CHECK(!gh_game_event_resource_abort(&loaded));

    CHECK(gh_game_event_module_abort(&module) == GH_EVENT_OK);
    CHECK(!gh_heap_used(heap) && gh_heap_destroy(heap) == GH_MEM_OK);
    free(elf);
    gh_disc_close(disc);
    puts("PASS actual EV01 config and missing track, EventEnv PLAY opening reveal, Fade ownership and 15/30 timing");
    return 0;
}
