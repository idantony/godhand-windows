#include "game_event_wait_vib.h"
#include "game_effects.h"

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

    gh_event_components components = {0};
    CHECK(gh_event_components_bind(&components, &module, elf, elf_bytes) == GH_EVENT_OK);
    CHECK(gh_event_components_construct_00297680(&components, 1) == GH_EVENT_OK);
    CHECK(gh_game_event_resource_begin_00297540(&resource, &event, &dvd, heap) == GH_EVENT_OK);
    ready = 0;
    for (unsigned i = 0; i < 1024 && !ready; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_game_event_resource_poll_002975d0(&resource, &ready) == GH_EVENT_OK);
    }
    CHECK(ready && resource.loaded && resource.file_bytes == 873872);

    gh_event_data_prefix prefix = {0};
    uint32_t frontier = 0;
    int relative = -1;
    CHECK(gh_event_data_bind_prefix_002977e8(&prefix, &components, &resource, &frontier) == GH_EVENT_OK);
    CHECK(gh_event_actor_cuts_bind_00297864(&prefix, &frontier) == GH_EVENT_OK);
    CHECK(gh_event_wait_vib_visit(&components, &prefix, &work, 0x296d98, &frontier, &relative) ==
        GH_EVENT_UNSUPPORTED && frontier == 0x297998 && !relative);

    size_t core_size, room_size;
    uint8_t *core = load(disc, "etc/core.dat", &core_size);
    uint8_t *room_data = load(disc, "st1/r100.dat", &room_size);
    gh_bundle room;
    CHECK(gh_bundle_open(room_data, room_size, &room) == GH_BUNDLE_OK);
    gh_game_effects *effects = calloc(1, sizeof(*effects));
    uint8_t *esp = calloc(1, 0x100180), *emitters = calloc(1, 0x26060);
    CHECK(effects && esp && emitters);
    CHECK(gh_game_effects_initialize_00305a68(effects, esp, 0x100180, emitters, 0x26060,
        core, core_size, &room) == GH_EFFECT_OK);
    CHECK(gh_event_data_bind_tail_00297880(&prefix, &components, register_effects, effects, &frontier) == GH_EVENT_OK);

    gh_event_component_view vib, wait;
    CHECK(gh_event_component_at(&components, 11, &vib) == GH_EVENT_OK);
    CHECK(gh_event_component_at(&components, 12, &wait) == GH_EVENT_OK);
    uint8_t *raw = resource.allocation.pointer;
    CHECK(!memcmp(raw + 0x44, (const uint8_t[4]){0}, 4) && !memcmp(raw + 0x54, (const uint8_t[4]){0}, 4));
    memset(vib.bytes, 0xa1, 16);
    memset(wait.bytes, 0xb2, 16);
    memset(work.pad570, 0xab, sizeof(work.pad570));
    uint8_t original_work[sizeof(work)];
    memcpy(original_work, &work, sizeof(work));
    CHECK(gh_event_wait_vib_visit(&components, &prefix, &work, 0x296d8c, &frontier, &relative) == GH_EVENT_OK);
    CHECK(!frontier && !relative && !memcmp(vib.bytes, "VIB", 4) && vib.bytes[4] == 0xa1);
    CHECK(!memcmp(original_work, &work, sizeof(work)));
    CHECK(gh_event_wait_vib_visit(&components, &prefix, &work, 0x296e9c, &frontier, &relative) == GH_EVENT_OK);
    CHECK(!memcmp(original_work, &work, sizeof(work)));
    CHECK(gh_event_wait_vib_visit(&components, &prefix, &work, 0x296d98, &frontier, &relative) == GH_EVENT_OK);
    CHECK(!memcmp(wait.bytes, "WAIT", 5) && wait.bytes[5] == 0xb2);
    put_word(original_work + 0x570, 2);
    CHECK(!memcmp(original_work, &work, sizeof(work)));
    CHECK(gh_event_set_frame_0030d168(&work, -7) == GH_EVENT_OK);
    CHECK(!memcmp(original_work, &work, sizeof(work)));
    CHECK(gh_event_set_frame_0030d168(&work, 9) == GH_EVENT_OK && work.pad570[0] == 9);
    CHECK(gh_event_wait_vib_visit(&components, &prefix, &work, 0x296ea8, &frontier, &relative) == GH_EVENT_OK);
    CHECK(!memcmp(original_work, &work, sizeof(work)));

    put_word(vib.bytes + 0x14, 1);
    CHECK(gh_event_wait_vib_visit(&components, &prefix, &work, 0x296e9c, &frontier, &relative) ==
        GH_EVENT_BOUNDS && frontier == 0x29797c && !relative);
    put_word(vib.bytes + 0x14, 0);
    gh_event_data_prefix foreign = prefix;
    foreign.components = NULL;
    CHECK(gh_event_wait_vib_visit(&components, &foreign, &work, 0x296d98, &frontier, &relative) == GH_EVENT_ARGUMENT);
    CHECK(!memcmp(original_work, &work, sizeof(work)));

    free(effects);
    free(esp);
    free(emitters);
    free(core);
    free(room_data);
    CHECK(gh_game_event_resource_abort(&resource) == GH_EVENT_OK);
    CHECK(gh_game_event_module_abort(&module) == GH_EVENT_OK);
    CHECK(!gh_heap_used(heap) && gh_heap_destroy(heap) == GH_MEM_OK);
    gh_disc_close(disc);
    free(elf);
    puts("PASS actual EV01 empty VIB/WAIT init/move, canonical Work+570 and strict track identity");
    return 0;
}
