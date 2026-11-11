#include "game_event_data_prefix.h"
#include "game_event_link.h"
#include "game_event_side_play.h"
#include "game_scene_message_create.h"
#include "game_message_step.h"

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
    gh_boot_work work = {.language = 1};
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
    size_t lang_size, room_size;
    uint8_t *lang_data = load(disc, "etc/core_usa.bin", &lang_size);
    uint8_t *room_data = load(disc, "st1/r100.dat", &room_size);
    gh_bundle room, language;
    CHECK(!gh_bundle_open(room_data, room_size, &room));
    CHECK(!gh_bundle_open(lang_data, lang_size, &language));
    gh_menu_state menu = {0};
    gh_font_state fonts = {0};
    gh_game_scene_ui ui = {0};
    CHECK(!gh_menu_load_tables_002ae990(&menu, &language));
    CHECK(!gh_font_initialize(&fonts, &language, work.language));
    CHECK(!gh_scene_font_bind_002a4c38(&fonts, &room, work.language));
    CHECK(!gh_scene_message_initialize_002aea18(&ui, &menu, &room, work.language));
    CHECK(ui.room_data.size == 0x40 && ui.active_head == -1 && ui.free_head == 0);

    gh_event_message_play message = {0};
    int relative;
    CHECK(!gh_event_message_init_00008100(&message, &prefix, &config, &ui, 0, UINT16_MAX,
        gh_scene_event_message_create_002aeb68, &work, &frontier, &relative));
    CHECK(!frontier && message.message_id == 0x1000 && message.work_id == 2);
    CHECK(ui.active_head == 0 && ui.free_head == 1 && ui.generation_bd8 == 2);
    const gh_scene_message_block *block = ui.blocks;
    CHECK(block->flags == message.work_id && !block->mode && block->previous == -1 && block->next == -1);
    uint8_t expected[0xb0] = {0};
    put_word(expected + 0x14 - 0x0c, 0x1002);
    put_word(expected + 0x20 - 0x0c, UINT32_MAX);
    memset(expected + 0x24 - 0x0c, 0xff, 6);
    expected[0x30 - 0x0c] = 0xff;
    expected[0x64 - 0x0c + 1] = 0x10;
    put_word(expected + 0x6c - 0x0c, 0x3f800000);
    memset(expected + 0x88 - 0x0c, 0xff, 2);
    CHECK(!memcmp(block->payload, expected, sizeof(expected)));
    CHECK(ui.sources[0].bound && ui.sources[0].primary.bytes == ui.room_data.bytes + 0x0c);
    CHECK(ui.sources[0].primary.size == ui.room_data.size - 0x0c);
    CHECK(!ui.sources[0].ruby.bytes && !ui.sources[0].ruby.size);
    CHECK(!ui.sources[0].cursor_7c && !ui.sources[0].cursor_8c);
    const uint8_t *script = ui.sources[0].primary.bytes;
    const uint16_t first = (uint16_t)(script[0] | (unsigned)script[1] << 8);
    gh_font_glyph glyph;
    CHECK(!gh_font_glyph_at(&fonts, first, &glyph));

    put_word((uint8_t *)&work + 0x5d8, 0x80000);
    CHECK(!gh_scene_message_update_002aefa8(&ui, &work, &frontier) && !frontier);
    CHECK(!memcmp(block->payload, expected, sizeof(expected)));
    put_word((uint8_t *)&work + 0x5d8, 0);
    put_word((uint8_t *)&work + 0x590, 2);
    CHECK(!gh_scene_message_prepare_draw_002af028(&ui, &work, &frontier) && !frontier);
    CHECK(!memcmp(block->payload, expected, sizeof(expected)));
    put_word((uint8_t *)&work + 0x590, 0);
    CHECK(!gh_scene_message_prepare_draw_002af028(&ui, &work, &frontier) && !frontier);
    CHECK(block->payload[0xa6 - 0xc + 1] == 1 && block->payload[0xa8 - 0xc] == 0x82);
    CHECK(block->payload[0xaa - 0xc] == 0xe0 && block->payload[0xac - 0xc] == 0x88);
    CHECK(!gh_scene_message_update_002aefa8(&ui, &work, &frontier) && !frontier);
    CHECK(block->payload[0] == 1 && block->payload[1] == 2 && !block->payload[0x30 - 0xc]);
    CHECK(block->payload[0x10 - 0xc] == 1 && block->payload[0x14 - 0xc] == 0x0a);
    CHECK(!gh_scene_message_prepare_draw_002af028(&ui, &work, &frontier) && !frontier);
    CHECK(!gh_scene_message_update_002aefa8(&ui, &work, &frontier) && !frontier);
    CHECK(block->payload[0] == 2 && !block->payload[1] && block->payload[0x14 - 0xc] == 0x1a);
    CHECK(gh_scene_message_prepare_draw_002af028(&ui, &work, &frontier) == GH_SCENE_UI_UNSUPPORTED);
    CHECK(frontier == 0x2b1c80);

    CHECK(!gh_scene_message_update_002aefa8(&ui, &work, &frontier) && !frontier);
    CHECK(block->payload[0] == 3 && !block->payload[1] && ui.sources[0].cursor_8c == 2);
    CHECK(block->payload[0x68 - 0xc] == 30 && block->payload[0x90 - 0xc] == 30);
    CHECK(!ui.wait_frames_3c2550);
    for (unsigned frame = 0; frame < 5; ++frame) {
        CHECK(!gh_scene_message_update_002aefa8(&ui, &work, &frontier) && !frontier);
        CHECK(block->payload[0] == 3 && block->payload[1] == 1 && ui.sources[0].cursor_8c == 2);
        CHECK(ui.wait_frames_3c2550 == frame + 1 && block->payload[0xb2 - 0xc] == frame + 1);
    }
    put_word((uint8_t *)&work + 0x5e4, 0x20000010);
    put_word((uint8_t *)&work + 0x178, 0x10); /* English confirm input. */
    CHECK(!gh_scene_message_update_002aefa8(&ui, &work, &frontier) && !frontier);
    CHECK(ui.wait_frames_3c2550 == 6 && block->payload[0] == 3);
    put_word((uint8_t *)&work + 0x5e4, 0x20000000);
    CHECK(gh_scene_message_update_002aefa8(&ui, &work, &frontier) == GH_SCENE_UI_UNSUPPORTED);
    CHECK(frontier == 0x297bf8 && block->payload[0] == 2 && ui.sources[0].confirm_pending);
    CHECK(!ui.wait_frames_3c2550 && ui.sources[0].cursor_8c == 4);

    gh_event_timing timing = {0};
    int original = 0;
    CHECK(!gh_event_timing_prepare_000054c0(&timing, components, heap, &original) && original);
    gh_event_component_view env;
    CHECK(!gh_event_component_at(components, 0, &env));
    put_word(env.bytes + 0x10, 3);
    gh_event_cut_timing *rows = timing.allocation.pointer;
    rows[2].kind = 4;
    rows[3].kind = 2;
    CHECK(!gh_scene_message_update_with_timing_002aefa8(&ui, &work, &timing, &frontier) && !frontier);
    CHECK(rows[2].kind == 4 && rows[3].kind == 0 && rows[3].next_cut == 4);
    CHECK(!ui.sources[0].confirm_pending && ui.sources[0].cursor_8c == 4);
    CHECK(!gh_event_timing_abort(&timing));
    memcpy(expected, block->payload, sizeof(expected));
    CHECK(gh_scene_message_update_002aefa8(&ui, &work, &frontier) == GH_SCENE_UI_UNSUPPORTED);
    CHECK(frontier == 0x2b3b70 && ui.active_head == 0 && !memcmp(block->payload, expected, sizeof(expected)));
    CHECK(!gh_event_message_init_00008100(&message, &prefix, &config, &ui, 0, UINT16_MAX,
        gh_scene_event_message_create_002aeb68, &work, &frontier, &relative));
    CHECK(!frontier && ui.active_head == 0 && ui.generation_bd8 == 2);

    uint16_t id = 0x1234;
    CHECK(gh_scene_message_create_002aeb68(&ui, &work, 0x1001, &id, &frontier) == GH_SCENE_UI_BOUNDS);
    CHECK(frontier == 0x2aead0 && id == 0x1234 && ui.active_head == 0 && ui.free_head == 1);
    CHECK(!memcmp(block->payload, expected, sizeof(expected)));

    ui.generation_bd8 = 0xfffe;
    CHECK(!gh_scene_message_create_002aeb68(&ui, &work, 0x1000, &id, &frontier));
    CHECK(!frontier && id == 0 && ui.active_head == 1 && ui.free_head == 2);
    CHECK(ui.blocks[1].next == 0 && ui.blocks[0].previous == 1 && ui.blocks[0].payload[0] == 6);
    CHECK((ui.blocks[0].payload[0x14 - 0x0c] & 0x40) && ui.sources[0].bound && ui.sources[1].bound);
    for (unsigned i = 2; i < 16; ++i) {
        CHECK(!gh_scene_message_create_002aeb68(&ui, &work, 0x1000, &id, &frontier));
        CHECK(id == i - 1 && ui.active_head == (int32_t)i);
    }
    CHECK(ui.free_head == -1);
    CHECK(!gh_scene_message_create_002aeb68(&ui, &work, 0x1000, &id, &frontier));
    CHECK(!frontier && id == UINT16_MAX && ui.active_head == 15 && ui.blocks[15].payload[0] == 6);
    CHECK(!gh_scene_message_initialize_002aea18(&ui, &menu, &room, work.language));
    for (unsigned i = 0; i < 16; ++i) CHECK(!ui.sources[i].bound && !ui.sources[i].primary.bytes);

    free(lang_data);
    free(room_data);
    CHECK(!gh_game_event_resource_abort(&resource));
    CHECK(!gh_game_event_module_abort(&module));
    CHECK(!gh_heap_used(heap) && !gh_heap_destroy(heap));
    gh_disc_close(disc);
    free(elf);
    printf("PASS actual EV01 Message 1000 work2, English glyph width %d, A301 input hold and canonical timing row clear; full pool returns FFFF\n", glyph.width);
    return 0;
}
