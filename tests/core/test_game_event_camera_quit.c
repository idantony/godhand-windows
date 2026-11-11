#include "game_event_play_env.h"
#include "game_event_camera_track.h"
#include "game_event_data_prefix.h"
#include "game_event_resource.h"
#include "game_event_timing.h"
#include "game_motion_camera.h"

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

static void set_word(uint8_t *bytes, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes[i] = (uint8_t)(value >> (i * 8));
}

static float get_frame(const uint8_t *bytes) {
    float value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static float channel_at(const gh_motion_view *view, unsigned channel, unsigned frame) {
    const uint8_t *entry = view->bytes + view->offset + 8 + channel * 12;
    const unsigned method = (get(entry) >> 12) & 15;
    float constant;
    memcpy(&constant, entry + 8, sizeof(constant));
    if (!method) return constant;
    CHECK(method == 4);
    const uint8_t *data = view->bytes + view->offset + get(entry + 8);
    float base, multiplier;
    memcpy(&base, data, sizeof(base));
    memcpy(&multiplier, data + 4, sizeof(multiplier));
    const unsigned key = data[8 + frame * 2] | (unsigned)data[9 + frame * 2] << 8;
    const volatile float product = (float)key * multiplier;
    return base + product;
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
    gh_event_link link = {0};
    CHECK(!gh_game_event_link_002be890(&link, &module, elf, elf_bytes));
    gh_event_components *components = &link.components;
    CHECK(gh_event_components_construct_00297680(components, 1) == GH_EVENT_OK);

    uint32_t frontier;
    gh_event_resource resource = {0};
    CHECK(!gh_game_event_resource_begin_00297540(&resource, &event, &dvd, heap));
    ready = 0;
    for (unsigned i = 0; i < 8 && !ready; ++i) {
        CHECK(!gh_dvd_pump(&dvd));
        CHECK(!gh_game_event_resource_poll_002975d0(&resource, &ready));
    }
    CHECK(ready);
    gh_event_data_prefix prefix = {0};
    CHECK(!gh_event_data_bind_prefix_002977e8(&prefix, components, &resource, &frontier));
    CHECK(prefix.camera_count == 10);
    gh_event_component_view env;
    CHECK(!gh_event_component_at(components, 0, &env));
    work.phase569 = 2;

    gh_game_init_state init = {0};
    gh_game_pl_camera player = {0};
    gh_game_motion_camera motion = {0};
    CHECK(!gh_game_camera_construct_pl_00139bc8(&player, &init.camera, GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    CHECK(!gh_game_motion_camera_construct_00138be8(&motion, GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    init.camera.mode = 1;
    init.active_camera = &init.camera;
    CHECK(!gh_event_camera_init_play_00004bd8(components, &init, &motion, &frontier));
    CHECK(init.camera.mode == 4 && init.active_camera == &motion.identity);
    const int32_t width = 512;
    const uint32_t height = 0x43e00000, aspect = 0x3f800000;
    uint8_t initialized = 0;
    gh_event_camera_quit_bindings bindings = {
        .components = components, .data = &prefix, .config = &config, .init = &init, .motion = &motion,
        .player_camera = &player, .work = &work,
        .display = {&width, &height, &aspect, &initialized, GH_CAMERA_NATIVE_BINARY32_REFERENCE}
    };
    gh_event_camera_quit skipped = {0};
    set_word(env.bytes + 0x28, 0x40000000);
    CHECK(!gh_event_camera_quit_play_00004d50(&skipped, &bindings, &frontier) && !frontier);
    CHECK(init.active_camera == &motion.identity && !motion.motion.bytes);
    set_word(env.bytes + 0x28, 0);

    gh_event_camera_quit quit = {0};
    CHECK(gh_event_camera_quit_play_00004d50(&quit, &bindings, &frontier) != GH_EVENT_OK);
    CHECK(frontier == 0x2a48a8 && quit.stage == 1 && init.active_camera == &motion.identity);
    CHECK(motion.motion.offset == 0x20d0 && motion.motion.frames > 1);
    CHECK(get_frame(motion.camera.fields + 0x3a8) == (float)(motion.motion.frames - 1));
    memset(player.fields + 0x254, 0xa5, 0x28);
    initialized = 1;
    CHECK(!gh_event_camera_quit_play_00004d50(&quit, &bindings, &frontier));
    CHECK(!frontier && quit.stage == 4 && init.active_camera == &init.camera);
    CHECK(init.camera.mode == 1 && init.camera.previous_mode == 4 && init.camera.previous == &motion.identity);
    for (unsigned axis = 0; axis < 3; ++axis) {
        const unsigned last = motion.motion.frames - 1;
        CHECK(get_frame(motion.camera.fields + 0x210 + axis * 4) == channel_at(&motion.motion, axis, last));
        CHECK(get_frame(motion.camera.fields + 0x200 + axis * 4) == channel_at(&motion.motion, axis + 3, last));
    }
    CHECK(get_frame(motion.camera.fields + 0x250) == channel_at(&motion.motion, 7, motion.motion.frames - 1));
    CHECK(get(motion.camera.fields + 0x3e4) == 10);
    for (unsigned i = 0x254; i < 0x27c; ++i) {
        const int untouched = (i >= 0x265 && i <= 0x267) || (i >= 0x279 && i <= 0x27b);
        CHECK(player.fields[i] == (untouched ? 0xa5 : 0));
    }
    player.fields[0x264] = 0x5a;
    CHECK(!gh_event_camera_quit_play_00004d50(&quit, &bindings, &frontier) && player.fields[0x264] == 0x5a);
    gh_event_camera_quit_bindings foreign = bindings;
    foreign.player_camera = &motion.camera;
    CHECK(gh_event_camera_quit_play_00004d50(&quit, &foreign, &frontier) == GH_EVENT_ARGUMENT);

    gh_game_event_config keep = config;
    keep.flags |= 0x80;
    CHECK(!gh_event_camera_init_play_00004bd8(components, &init, &motion, &frontier));
    gh_event_camera_quit retained = {0};
    bindings.config = &keep;
    set_word(motion.camera.fields + 0x274, 0x3f800000);
    CHECK(!gh_event_camera_quit_play_00004d50(&retained, &bindings, &frontier) && !frontier);
    CHECK(init.active_camera == &motion.identity && init.camera.mode == 4);
    CHECK(!get(motion.camera.fields + 0x274) && player.fields[0x264] == 0x5a);

    CHECK(!gh_game_event_resource_abort(&resource));
    CHECK(!gh_game_event_module_abort(&module));
    CHECK(!gh_heap_used(heap) && !gh_heap_destroy(heap));
    gh_disc_close(disc);
    free(elf);
    puts("PASS EV01 REL4D50 final MTb3 key oracle, 138600 retry, canonical 138D40 switch and 138528 shake reset");
    return 0;
}
