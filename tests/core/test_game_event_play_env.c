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

static void set_frame(uint8_t *bytes, float frame) {
    uint32_t bits;
    memcpy(&bits, &frame, sizeof(bits));
    set_word(bytes, bits);
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

static void verify_camera(gh_event_link *link, const gh_event_data_prefix *prefix,
    gh_event_component_view env) {
    gh_event_component_view camera;
    CHECK(!gh_event_component_at(&link->components, 1, &camera));
    uint32_t frontier;
    gh_event_camera_track track;
    gh_game_init_state init = {0};
    gh_game_motion_camera motion_camera = {0};
    CHECK(!gh_game_motion_camera_construct_00138be8(&motion_camera, GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    init.camera.mode = 1;
    init.active_camera = &init.camera;
    CHECK(!gh_event_camera_init_play_00004bd8(&link->components, &init, &motion_camera, &frontier));
    CHECK(init.camera.mode == 4 && init.active_camera == &motion_camera.identity);
    CHECK(init.camera.previous_mode == 1 && init.camera.previous == &init.camera);

    gh_boot_work work = {0};
    ((uint8_t *)&work)[0x569] = 2;
    const int32_t width = 512;
    const uint32_t height = 0x43e00000, aspect = 0x3f800000;
    const uint8_t initialized = 1;
    const gh_camera_projection_display display = {
        &width, &height, &aspect, &initialized, GH_CAMERA_NATIVE_BINARY32_REFERENCE
    };
    unsigned frames = 0;
    for (unsigned cut = 0; cut < prefix->camera_count; ++cut) {
        set_word(env.bytes + 0x10, cut);
        uint16_t length;
        CHECK(!gh_event_camera_cut_length_00004e60(prefix, cut, &length));
        for (unsigned frame = 0; frame < length; ++frame) {
            set_frame(env.bytes + 0x14, (float)frame);
            CHECK(!gh_event_camera_select_00004c08(link, prefix, &track, &frontier));
            CHECK(!frontier && track.apply_motion && track.frame == (float)frame && track.track.frames == length);
            CHECK(!camera.bytes[4] && camera.bytes[6] == cut && !get(camera.bytes + 8));
            gh_motion_camera_sample sample;
            CHECK(!gh_motion_camera_sample_00139180(&track.track, track.frame, &sample, &frontier));
            for (unsigned axis = 0; axis < 3; ++axis) {
                CHECK(sample.eye[axis] == channel_at(&track.track, axis, frame));
                CHECK(sample.target[axis] == channel_at(&track.track, axis + 3, frame));
            }
            CHECK(sample.roll == channel_at(&track.track, 6, frame));
            CHECK(sample.fov == channel_at(&track.track, 7, frame));

            CHECK(!gh_game_motion_camera_set_001390b8(&motion_camera, &track));
            CHECK(!gh_game_motion_camera_move_00138600(&motion_camera, &init, &work, &display, &frontier));
            const uint8_t *fields = motion_camera.camera.fields;
            CHECK(!frontier && !motion_camera.move_stage);
            CHECK(!memcmp(fields + 0x210, sample.eye, sizeof(sample.eye)));
            CHECK(!memcmp(fields + 0x200, sample.target, sizeof(sample.target)));
            CHECK(get_frame(fields + 0x250) == sample.fov);
            const unsigned last = frame + 1 == length;
            CHECK(get(fields + 0x3e4) == (last ? 10u : 8u));
            CHECK(get_frame(fields + 0x3a8) == (float)(frame + !last));
            /* The eye maps to the view origin and the target is in front. */
            float matrix[16];
            memcpy(matrix, fields + 0x140, sizeof(matrix));
            for (unsigned axis = 0; axis < 3; ++axis) {
                float at_eye = matrix[12 + axis];
                for (unsigned i = 0; i < 3; ++i) at_eye += matrix[4 * i + axis] * sample.eye[i];
                CHECK(at_eye > -0.0001f && at_eye < 0.0001f);
            }
            CHECK(matrix[2] * (sample.target[0] - sample.eye[0]) +
                  matrix[6] * (sample.target[1] - sample.eye[1]) +
                  matrix[10] * (sample.target[2] - sample.eye[2]) > 0);
            ++frames;
        }
    }
    CHECK(frames == 1209);

    set_word(env.bytes + 0x10, 0);
    set_frame(env.bytes + 0x14, 241.5f);
    const uint32_t flags = get(env.bytes + 0x28);
    set_word(env.bytes + 0x28, flags | 0x40000000u);
    CHECK(!gh_event_camera_select_00004c08(link, prefix, &track, &frontier));
    CHECK(!track.apply_motion && track.frame == 119 && !camera.bytes[4]);
    set_word(env.bytes + 0x28, flags);

    uint8_t *row = (uint8_t *)prefix->resource->allocation.pointer + prefix->camera_table;
    const uint32_t primary = get(row + 4);
    CHECK(primary && !get(row + 8));
    set_word(row + 4, 0);
    set_word(row + 8, primary);
    CHECK(!gh_event_camera_select_00004c08(link, prefix, &track, &frontier));
    CHECK(track.apply_motion && track.frame == 1.5f && camera.bytes[4] == 1 && get(camera.bytes + 8) == 2);
    set_word(row + 8, 0);
    set_word(row + 4, primary);
    CHECK(!gh_game_camera_restore_00138d40(&init) && init.active_camera == &init.camera);
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
    gh_event_link link = {0};
    CHECK(!gh_game_event_link_002be890(&link, &module, elf, elf_bytes));
    gh_event_components *components = &link.components;
    CHECK(gh_event_components_construct_00297680(components, 1) == GH_EVENT_OK);

    gh_event_component_view env, track;
    CHECK(gh_event_component_at(components, 0, &env) == GH_EVENT_OK);
    CHECK(gh_event_component_at(components, 13, &track) == GH_EVENT_OK);
    memset(env.bytes + 0x10, 0xa5, 16);
    env.bytes[0x20] = 0x5a;
    raw_work[0x5db] = 0xc2;
    gh_display_state display = {.initialized = 1, .screen_width = 512, .gs_y_origin = 1824};
    gh_screen_fade fade = {0};
    uint32_t frontier = 0;
    CHECK(gh_event_env_init_play_00005930(components, &config, &work, &fade, NULL, &frontier) == GH_EVENT_ARGUMENT);
    CHECK(frontier == 0x307208 && raw_work[0x5db] == 0xc2 && get(env.bytes + 0x10) == 0xa5a5a5a5);
    CHECK(gh_event_env_init_play_00005930(components, &config, &work, &fade, &display, &frontier) == GH_EVENT_OK);
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
    CHECK(gh_event_fade_init_absent_00006c70(components, header, 0x4f, &frontier) == GH_EVENT_BOUNDS);
    CHECK(frontier == 0x297f80 && track.bytes[0x18] == 0xa5);
    header[0x4c] = 0x10;
    CHECK(gh_event_fade_init_absent_00006c70(components, header, sizeof(header), &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(track.bytes[0x18] == 0xa5);
    header[0x4c] = 0;
    CHECK(gh_event_fade_init_absent_00006c70(components, header, sizeof(header), &frontier) == GH_EVENT_OK);
    CHECK(track.bytes[0x0f] == 0x56 && track.bytes[0x19] == 0x78);
    CHECK(!get(track.bytes + 0x10) && !get(track.bytes + 0x14) && !track.bytes[0x18]);

    const gh_screen_fade_context context = {512, 1824, 0, 0};
    gh_fade_draw draw;
    for (unsigned tick = 0; tick <= 15; ++tick) {
        CHECK(gh_event_fade_move_absent_00006ca0(components, &fade, &frontier) == GH_EVENT_OK);
        CHECK(gh_screen_fade_step_00307388(&fade, &context, &draw) == GH_FADE_OK);
        CHECK(draw.visible && draw.rgba == ((255 - tick * 17) << 24));
        CHECK(draw.test == (tick < 15 ? 0x30000u : 0x70000u));
    }
    CHECK(!fade.state && gh_screen_fade_step_00307388(&fade, &context, &draw) == GH_FADE_OK && !draw.visible);
    raw_work[0x564] = 1;
    CHECK(gh_event_env_init_play_00005930(components, &config, &work, &fade, &display, &frontier) == GH_EVENT_OK);
    CHECK(fade.time == 30);
    track.bytes[0x18] = 1;
    CHECK(gh_event_fade_move_absent_00006ca0(components, &fade, &frontier) == GH_EVENT_OK && !fade.state && !track.bytes[0x18]);

    gh_event_resource loaded = {0};
    CHECK(!gh_game_event_resource_begin_00297540(&loaded, &event, &dvd, heap));
    ready = 0;
    for (unsigned i = 0; i < 8 && !ready; ++i) {
        CHECK(!gh_dvd_pump(&dvd));
        CHECK(!gh_game_event_resource_poll_002975d0(&loaded, &ready));
    }
    CHECK(ready && loaded.loaded);
    gh_event_data_prefix prefix = {0};
    CHECK(!gh_event_data_bind_prefix_002977e8(&prefix, components, &loaded, &frontier));
    CHECK(prefix.camera_count == 10);
    verify_camera(&link, &prefix, env);

    gh_event_timing timing = {0};
    gh_heap *small = NULL;
    CHECK(!gh_heap_create(64, &small));
    int original = 1;
    CHECK(!gh_event_timing_prepare_000054c0(&timing, components, small, &original));
    CHECK(!original && !timing.allocation.pointer && !gh_heap_used(small));
    CHECK(!gh_heap_destroy(small));
    size_t before = gh_heap_used(heap);
    CHECK(!gh_event_timing_prepare_000054c0(&timing, components, heap, &original) && original);
    CHECK(timing.count == 10 && gh_heap_used(heap) == before + 160 && !get(env.bytes + 4));
    const uint8_t *timing_bytes = timing.allocation.pointer;
    for (size_t cut = 0; cut < 10; ++cut) {
        for (size_t byte = 0; byte < 16; ++byte) {
            unsigned expected = byte == 1 ? (cut == 9 ? 255 : (unsigned)cut + 1) : 0;
            CHECK(timing_bytes[cut * 16 + byte] == expected);
        }
    }
    CHECK(!gh_event_timing_prepare_000054c0(&timing, components, heap, &original) && original);
    CHECK(timing_bytes == timing.allocation.pointer && gh_heap_used(heap) == before + 160);
    CHECK(!gh_event_timing_abort(&timing) && gh_heap_used(heap) == before);

    gh_event_play_state play = {0};
    const gh_event_play_bindings play_bindings = {
        .event = &event, .components = components, .config = &config,
        .work = &work, .fade = &fade, .display = &display
    };
    event.fields.state = 3;
    event.fields.phase = 0;
    raw_work[0x564] = 0;
    CHECK(gh_event_play_00296c28(&play, &play_bindings, &original, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(!original && frontier == 0x296d08 && !event.fields.phase);
    CHECK(!(event.fields.flags & 4) && get(env.bytes + 0x34) == 0x298088);
    CHECK((raw_work[0x5e4] & 1) && fade.time == 15);
    CHECK(!gh_screen_fade_step_00307388(&fade, &context, &draw) && fade.count == 1);
    CHECK(gh_event_play_00296c28(&play, &play_bindings, &original, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(fade.count == 1 && frontier == 0x296d08);
    uint8_t object_bytes[0x300] = {0};
    uint8_t save_bytes[0xaf] = {0};
    object_bytes[0x2fe] = 0x67;
    object_bytes[0x2ff] = 0x06;
    object_bytes[0x250] = 0x40;
    gh_title_player_view object = {.bytes=object_bytes,.size=sizeof(object_bytes)};
    gh_core_save_view save = {save_bytes, sizeof(save_bytes)};
    gh_actor_model object_model = {.root = object};
    CHECK(!gh_event_env_extra_move_object_00006b08(components, &object_model, save, &frontier));
    CHECK(object_bytes[0x250] == 0x40);
    save_bytes[0xae] = 1;
    CHECK(!gh_event_env_extra_move_object_00006b08(components, &object_model, save, &frontier));
    CHECK(object_bytes[0x250] == 0x42);
    CHECK(!gh_game_event_resource_abort(&loaded));

    CHECK(gh_game_event_module_abort(&module) == GH_EVENT_OK);
    CHECK(!gh_heap_used(heap) && gh_heap_destroy(heap) == GH_MEM_OK);
    free(elf);
    gh_disc_close(disc);
    puts("PASS actual EV01 config, Fade and all 1209 camera motion and projection frames");
    return 0;
}
