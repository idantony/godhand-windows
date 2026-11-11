#include "game_event_data_prefix.h"
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
    CHECK(gh_heap_create(4 * 1024 * 1024, &heap) == GH_MEM_OK);
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

    gh_event_component_view env, actor;
    CHECK(gh_event_component_at(&components, 0, &env) == GH_EVENT_OK);
    CHECK(gh_event_component_at(&components, 2, &actor) == GH_EVENT_OK);
    uint8_t *raw = resource.allocation.pointer;
    const uint8_t saved = raw[0x67];
    raw[0x67] = 0xff; // Corrupt the first camera's primary offset only.
    gh_event_data_prefix prefix = {0};
    uint32_t frontier = 0;
    CHECK(gh_event_data_bind_prefix_002977e8(&prefix, &components, &resource, &frontier) == GH_EVENT_BOUNDS);
    CHECK(frontier == 0x297824 && !prefix.bound && !env.bytes[8] && !actor.bytes[4]);
    raw[0x67] = saved;
    CHECK(gh_event_data_bind_prefix_002977e8(&prefix, &components, &resource, &frontier) == GH_EVENT_OK);
    CHECK(!frontier && prefix.bound && prefix.camera_count == 10 && prefix.actor_count == 9);
    CHECK(env.bytes[8] == 10 && actor.bytes[4] == 9);

    const uint32_t camera_offsets[] = {0xb10, 0xd70, 0xf90, 0x1180, 0x1370, 0x1750, 0x1b10, 0x1b80, 0x1e00, 0x20d0};
    const char names[][8] = {"el00", "el01", "el02", "el0a", "el0b", "el0c", "el30", "el31", "el67"};
    gh_event_camera_cut cut;
    uint16_t length = 0;
    for (size_t i = 0; i < 10; ++i) {
        CHECK(gh_event_camera_cut_at(&prefix, i, &cut) == GH_EVENT_OK);
        CHECK(cut.primary_offset == camera_offsets[i] && cut.primary == raw + camera_offsets[i] && !cut.alternate);
        CHECK(gh_event_camera_cut_length_00004e60(&prefix, i, &length) == GH_EVENT_OK && length > 0);
        if (i == 0) CHECK(length == 120);
    }
    CHECK(gh_event_camera_cut_at(&prefix, 10, &cut) == GH_EVENT_BOUNDS);
    const uint8_t *info;
    for (size_t i = 0; i < 9; ++i) {
        CHECK(gh_event_actor_info_at(&prefix, i, &info) == GH_EVENT_OK && !memcmp(info, names[i], 8));
    }
    CHECK(gh_event_actor_info_at(&prefix, 9, &info) == GH_EVENT_BOUNDS);

    const uint8_t saved_cut_offset = raw[0x0f];
    raw[0x0f] = 0xff;
    CHECK(gh_event_actor_cuts_bind_00297864(&prefix, &frontier) == GH_EVENT_BOUNDS);
    CHECK(frontier == 0x297864 && !prefix.actor_cuts_bound);
    raw[0x0f] = saved_cut_offset;
    CHECK(gh_event_actor_cuts_bind_00297864(&prefix, &frontier) == GH_EVENT_OK);
    CHECK(!frontier && prefix.actor_cut_table == 0x130 && prefix.actor_cuts_bound);
    size_t present = 0, sum = 0;
    gh_event_actor_cut actor_cut;
    for (size_t i = 0; i < 9; ++i) {
        for (size_t j = 0; j < 10; ++j) {
            CHECK(gh_event_actor_cut_at(&prefix, i, j, &actor_cut) == GH_EVENT_OK);
            const uint8_t *row = raw + 0x130 + (i * 10 + j) * 28;
            size_t expected = (size_t)row[4] + ((size_t)row[5] << 8) + ((size_t)row[6] << 16);
            CHECK(actor_cut.lanes[0].primary_offset == expected);
            CHECK(actor_cut.lanes[0].primary == (expected ? raw + expected : NULL));
            CHECK(!actor_cut.flags && !actor_cut.lanes[0].sequence && !actor_cut.lanes[1].primary && !actor_cut.lanes[1].sequence);
            present += expected != 0;
            sum += expected;
        }
    }
    CHECK(present == 48 && sum == 0x1610290);
    CHECK(gh_event_actor_cut_at(&prefix, 9, 0, &actor_cut) == GH_EVENT_BOUNDS);
    CHECK(gh_event_actor_cut_at(&prefix, 0, 10, &actor_cut) == GH_EVENT_BOUNDS);

    // Separate bounded synthetic carry fixture. The primary/sequence data
    // pointers still target known ranges inside the actual event allocation.
    uint8_t *row1 = raw + 0x130 + 28, *row2 = row1 + 28, *row3 = row2 + 28;
    uint8_t original_rows[3 * 28];
    memcpy(original_rows, row1, sizeof(original_rows));
    put_word(row1 + 8, 0xd2d00);
    put_word(row1 + 16, 0xb10);
    put_word(row1 + 20, 0xd2d30);
    row2[0] = row3[0] = 3;
    put_word(row2 + 4, 0);
    put_word(row3 + 16, 0xd70);
    gh_event_data_prefix carry = prefix;
    carry.actor_cuts_bound = 0;
    CHECK(gh_event_actor_cuts_bind_00297864(&carry, &frontier) == GH_EVENT_OK);
    CHECK(gh_event_actor_cut_at(&carry, 0, 2, &actor_cut) == GH_EVENT_OK);
    CHECK(!actor_cut.lanes[0].primary && actor_cut.lanes[0].sequence_offset == 0xd2d00);
    CHECK(actor_cut.lanes[0].source_cut == 1 && actor_cut.lanes[0].timing_cut == 1 && actor_cut.lanes[0].primary_length == 106);
    CHECK(!actor_cut.lanes[1].primary && actor_cut.lanes[1].sequence_offset == 0xd2d30);
    CHECK(actor_cut.lanes[1].source_cut == 1 && actor_cut.lanes[1].timing_cut == 1 && actor_cut.lanes[1].primary_length == 120);
    CHECK(gh_event_actor_cut_at(&carry, 0, 3, &actor_cut) == GH_EVENT_OK);
    CHECK(actor_cut.lanes[0].primary_offset == 0x102b0 && actor_cut.lanes[0].sequence_offset == 0xd2d00);
    CHECK(actor_cut.lanes[0].source_cut == 1 && actor_cut.lanes[0].timing_cut == 3 && actor_cut.lanes[0].primary_length == 94);
    CHECK(actor_cut.lanes[1].primary_offset == 0xd70 && actor_cut.lanes[1].sequence_offset == 0xd2d30);
    CHECK(actor_cut.lanes[1].source_cut == 1 && actor_cut.lanes[1].timing_cut == 3);
    uint8_t *other_actor = raw + 0x130 + 10 * 28;
    other_actor[0] = 1; // First cut cannot borrow stale previous-actor state.
    carry.actor_cuts_bound = 0;
    CHECK(gh_event_actor_cuts_bind_00297864(&carry, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x297864 && !carry.actor_cuts_bound);
    other_actor[0] = 0;
    memcpy(row1, original_rows, sizeof(original_rows));

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
    const gh_effect_resource startup_bank = effects->resources[0];
    gh_effect_view original_emitter = effects->emitter_entries[0];
    CHECK(gh_event_data_bind_tail_00297880(&prefix, &components, NULL, NULL, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x2978b8 && !effects->event_registered_41c);
    gh_event_sequence_view sequence;
    CHECK(gh_event_sequence_at(&prefix, 3, &sequence) == GH_EVENT_OK);
    CHECK(!sequence.records && !sequence.count);
    CHECK(gh_event_sequence_at(&prefix, 4, &sequence) == GH_EVENT_OK && !sequence.records && !sequence.count);

    const uint8_t original_filter = raw[0xd2d30];
    raw[0xd2d30] ^= 1;
    CHECK(gh_event_data_bind_tail_00297880(&prefix, &components, register_effects, effects, &frontier) == GH_EVENT_FORMAT);
    CHECK(frontier == 0x29790c && effects->event_registered_41c && effects->event_data.bytes == raw + 0xcbda0);
    gh_effect_resource *bank6 = NULL;
    for (unsigned i = 0; i < 64; ++i) {
        if (effects->resources[i].kind == 6) bank6 = effects->resources + i;
    }
    CHECK(bank6 && bank6->references == 1);
    CHECK(bank6->tbl0.bytes == raw + 0xcbdc0 && bank6->tbl1.bytes == raw + 0xcc640);
    CHECK(effects->emd_tables[6].bytes == raw + 0xd2ce0 && effects->emitter_entries[0].bytes == original_emitter.bytes);
    CHECK(effects->resources[0].kind == startup_bank.kind && effects->resources[0].references == startup_bank.references &&
          effects->resources[0].tbl0.bytes == startup_bank.tbl0.bytes && effects->resources[0].tbl1.bytes == startup_bank.tbl1.bytes);
    raw[0xd2d30] = original_filter;
    CHECK(gh_event_data_bind_tail_00297880(&prefix, &components, register_effects, effects, &frontier) == GH_EVENT_OK);
    CHECK(!frontier && bank6->references == 1);
    CHECK(gh_event_sequence_at(&prefix, 6, &sequence) == GH_EVENT_OK && sequence.resource_offset == 0xd2d00 &&
          sequence.stride == 28 && sequence.count == 1);
    CHECK(gh_event_sequence_at(&prefix, 7, &sequence) == GH_EVENT_OK && sequence.resource_offset == 0xd2d30 &&
          sequence.stride == 0x2e8 && sequence.count == 10 && sequence.bytes == 0x1d10);
    for (unsigned component = 8; component < 14; ++component) {
        CHECK(gh_event_sequence_at(&prefix, component, &sequence) == GH_EVENT_OK && !sequence.records && !sequence.count);
    }
    CHECK(gh_event_sequence_at(&prefix, 14, &sequence) == GH_EVENT_OK && sequence.resource_offset == 0xd4a50 &&
          sequence.stride == 0x44 && sequence.count == 42 && sequence.records == raw + 0xd4a60);
    CHECK(gh_game_effects_register_event_002fa0b0(effects, effects->event_data.bytes, effects->event_data.size) == GH_EFFECT_OK);
    CHECK(bank6->references == 2);
    CHECK(gh_game_effects_register_event_002fa0b0(effects, raw, resource.file_bytes) == GH_EFFECT_UNSUPPORTED);
    CHECK(effects->frontier == 0x2fa0e8 && bank6->references == 2);
    // This fixture has no effect users. Retire the canonical owner before
    // releasing its borrowed event and room resources.
    free(effects);
    free(esp);
    free(emitters);
    free(core_data);
    free(room_data);

    for (unsigned flags = 0; flags < 4; ++flags) {
        actor.bytes[0] = (uint8_t)flags;
        CHECK(gh_event_actor_creation_poll_00297a68(&components, &ready) == GH_EVENT_OK && ready == (int)(flags & 1));
        int normal = -1;
        CHECK(gh_event_actor_creation_end_normal_000012a8(&components, &normal) == GH_EVENT_OK);
        CHECK(normal == (flags < 2));
    }
    CHECK(gh_game_event_resource_abort(&resource) == GH_EVENT_OK);
    CHECK(gh_event_camera_cut_at(&prefix, 0, &cut) == GH_EVENT_ARGUMENT);
    // The shared REL binds the positive EV02 tracks and bank 6 from R003.
    event.fields.data_number = 2;
    CHECK(!gh_event_components_construct_00297680(&components, 2));
    CHECK(!gh_game_event_resource_begin_00297540(&resource, &event, &dvd, heap));
    ready = 0;
    for (unsigned i = 0; i < 1024 && !ready; ++i) {
        CHECK(!gh_dvd_pump(&dvd));
        CHECK(!gh_game_event_resource_poll_002975d0(&resource, &ready));
    }
    CHECK(ready && resource.loaded && resource.file_bytes == 2292096);
    prefix = (gh_event_data_prefix){0};
    CHECK(!gh_event_data_bind_prefix_002977e8(&prefix, &components, &resource, &frontier));
    CHECK(prefix.camera_count == 28 && prefix.actor_count == 22);
    CHECK(!gh_event_actor_cuts_bind_00297864(&prefix, &frontier));
    core_data = load(disc, "etc/core.dat", &core_size);
    room_data = load(disc, "st0/r003.dat", &room_size);
    CHECK(!gh_bundle_open(room_data, room_size, &room));
    effects = calloc(1, sizeof(*effects));
    esp = calloc(1, 0x100180);
    emitters = calloc(1, 0x26060);
    CHECK(effects && esp && emitters);
    CHECK(!gh_game_effects_initialize_00305a68(effects, esp, 0x100180, emitters, 0x26060,
                                              core_data, core_size, &room));
    CHECK(!gh_event_data_bind_tail_00297880(&prefix, &components, register_effects, effects, &frontier));
    CHECK(!frontier && effects->event_registered_41c);
    CHECK(!gh_event_sequence_at(&prefix, 4, &sequence) && sequence.resource_offset == 0x2218e0 &&
          sequence.stride == 0x2c && sequence.count == 1 && sequence.bytes == 0x2c);
    CHECK(!gh_event_sequence_at(&prefix, 5, &sequence) && sequence.resource_offset == 0x2296b0 &&
          sequence.stride == 0x60 && sequence.count == 2 && sequence.bytes == 0xc0);
    CHECK(sequence.records[3] == 26 && sequence.records[0x63] == 27);
    CHECK(!gh_event_sequence_at(&prefix, 13, &sequence) && sequence.resource_offset == 0x22e090 &&
          sequence.stride == 0x10 && sequence.count == 14);
    CHECK(!gh_event_sequence_at(&prefix, 14, &sequence) && sequence.count == 90);
    CHECK(effects->event_data.bytes == (const uint8_t *)resource.allocation.pointer + 0x221920);
    free(effects);
    free(esp);
    free(emitters);
    free(core_data);
    free(room_data);
    CHECK(!gh_game_event_resource_abort(&resource));
    puts("PASS EV02 R003 actual 28 cuts, 22 actors, positive sequence binding and event bank");

    CHECK(gh_game_event_module_abort(&module) == GH_EVENT_OK);
    CHECK(!gh_heap_used(heap) && gh_heap_destroy(heap) == GH_MEM_OK);
    gh_disc_close(disc);
    free(elf);
    puts("PASS EV01 actual DVD complete 2977E8 bind, 90 ActorCut rows, dual-lane carry, tracks and same-owner effect bank 6");
    return 0;
}
