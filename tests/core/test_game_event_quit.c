#include "game_event_quit.h"
#include "game_event_data_prefix.h"
#include "game_event_link.h"
#include "game_event_resource.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { \
    if (!(condition)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); exit(1); } \
} while (0)

static uint32_t get(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void put(uint8_t *p, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8));
}

typedef struct {
    uint32_t sites[16];
    unsigned count;
    uint32_t stop;
} fixture;

static gh_game_event_status visit(void *context, uint32_t site, int *original_end,
    uint32_t *frontier, int *relative) {
    fixture *f = context;
    CHECK(f->count < sizeof(f->sites) / sizeof(f->sites[0]));
    CHECK(!*original_end);
    f->sites[f->count++] = site;
    if (site == f->stop) {
        *frontier = 0x306200;
        *relative = 0;
        return GH_EVENT_UNSUPPORTED;
    }
    return GH_EVENT_OK;
}

typedef struct {
    gh_event_quit *cursor;
    const gh_event_quit_bindings *bindings;
} play_fixture;

static gh_game_event_status visit_play_quit(void *context, uint32_t site, int *original_end,
    uint32_t *frontier, int *relative) {
    play_fixture *f = context;
    CHECK(site == 0x296ef0 && !*original_end);
    return gh_event_quit_00296ef0(f->cursor, f->bindings, frontier, relative);
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
    CHECK(!get(env.bytes + 0x38));
    uint8_t *raw = (uint8_t *)&work;
    put(raw + 0x570, 5);
    put(raw + 0x5e4, 0xe0000001u);
    put(env.bytes + 0x28, get(env.bytes + 0x28) | 0x20u);
    put(env.bytes + 0x34, 0x298088); /* MoveObj remains independent. */
    event.fields.state = 3;
    event.fields.phase = 1;
    event.fields.flags |= 4;

    gh_event_quit cursor = {0};
    fixture fixture_data = {.stop = 0x296f68};
    gh_event_quit_bindings bindings = {&event, components, &work, NULL, NULL};
    int relative;
    CHECK(gh_event_quit_00296ef0(&cursor, &bindings, &frontier, &relative) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x4d50 && relative && event.fields.skip_ok == 1);
    bindings.visit = visit;
    bindings.context = &fixture_data;
    play_fixture play_data = {&cursor, &bindings};
    gh_event_play_state play = {.quit_pending = 1};
    const gh_event_play_bindings play_bindings = {
        .event = &event, .components = components, .work = &work,
        .visit = visit_play_quit, .context = &play_data
    };
    int original_done = -1;
    CHECK(gh_event_play_00296c28(&play, &play_bindings, &original_done, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x306200 && !play.frontier_relative && fixture_data.count == 3);
    CHECK(!original_done && event.fields.state == 3 && event.fields.phase == 1 && play.quit_pending);
    CHECK(event.fields.flags & 4);
    CHECK(get(raw + 0x5e4) == 0xe0000001u && get(raw + 0x570) == 5);

    gh_boot_work other_work = work;
    bindings.work = &other_work;
    CHECK(gh_event_quit_00296ef0(&cursor, &bindings, &frontier, &relative) == GH_EVENT_ARGUMENT);
    bindings.work = &work;
    fixture_data.stop = 0;
    event.fields.unknown06 = 8;
    event.fields.unknown07 = 9;
    CHECK(!gh_event_play_00296c28(&play, &play_bindings, &original_done, &frontier));
    CHECK(!frontier && original_done && cursor.completed && !(event.fields.flags & 4));
    const uint32_t expected[] = {0x296f3c, 0x296f48, 0x296f68, 0x296f68,
        0x296f74, 0x296f8c, 0x296fa4, 0x296fb0, 0x296fbc};
    CHECK(fixture_data.count == sizeof(expected) / sizeof(*expected));
    for (unsigned i = 0; i < fixture_data.count; ++i)
        CHECK(fixture_data.sites[i] == expected[i]);
    CHECK(get(raw + 0x5e4) == 0xe0000000u && get(raw + 0x570) == 2);
    CHECK(event.fields.state == 4 && !event.fields.phase && !event.fields.unknown06 &&
          !event.fields.unknown07 && !play.quit_pending);
    CHECK(!gh_event_quit_00296ef0(&cursor, &bindings, &frontier, &relative));
    CHECK(fixture_data.count == sizeof(expected) / sizeof(*expected));

    gh_event_quit inactive = {0};
    event.fields.skip_ok = 0;
    bindings.components = NULL;
    bindings.work = NULL;
    CHECK(!gh_event_quit_00296ef0(&inactive, &bindings, &frontier, &relative));
    CHECK(!inactive.started && !event.fields.skip_ok);

    CHECK(!gh_game_event_resource_abort(&resource));
    CHECK(!gh_game_event_module_abort(&module));
    CHECK(!gh_heap_used(heap) && !gh_heap_destroy(heap));
    gh_disc_close(disc);
    free(elf);
    puts("PASS EV01 DVD Quit order, partial retry, exact leaf frontiers, Wait and Work flag writes");
    return 0;
}
