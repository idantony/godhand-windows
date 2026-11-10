#include "game_event_actor_create.h"
#include "game_event_actor_factory.h"
#include "game_event_actor_extra.h"
#include "game_player_node_cache.h"


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

typedef struct {
    gh_room_pools *pools;
    unsigned calls;
    uint32_t fail;
    const uint32_t *handles;
    const uint16_t *kinds;
    size_t count;
} object_scope;

static gh_game_event_status object_view(void *opaque, uint32_t handle, gh_title_player_view *out,
                                       uint32_t *frontier) {
    object_scope *scope = opaque;
    ++scope->calls;
    if (handle == scope->fail) {
        *frontier = 0x123456;
        return GH_EVENT_UNSUPPORTED;
    }
    for (size_t i = 0; i < scope->count; ++i) {
        if (handle != scope->handles[i]) continue;
        void *object = NULL;
        if (gh_room_object_lookup(scope->pools, handle, scope->kinds[i], scope, &object)) {
            return GH_EVENT_ARGUMENT;
        }
        *out = *(gh_title_player_view *)object;
        return GH_EVENT_OK;
    }
    return GH_EVENT_ARGUMENT;
}

static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
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
    CHECK(gh_heap_create(32 * 1024 * 1024, &heap) == GH_MEM_OK);
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

    gh_event_component_view actor;
    CHECK(gh_event_component_at(&components, 2, &actor) == GH_EVENT_OK);
    gh_event_data_prefix prefix = {0};
    uint32_t frontier = 0;
    CHECK(gh_event_data_bind_prefix_002977e8(&prefix, &components, &resource, &frontier) == GH_EVENT_OK);
    CHECK(gh_event_actor_cuts_bind_00297864(&prefix, &frontier) == GH_EVENT_OK);
    CHECK(prefix.actor_count == 9 && prefix.camera_count == 10);

    size_t room_bytes, el_bytes;
    uint8_t *room = load(disc, "st1/r100.dat", &room_bytes);
    uint8_t *el = load(disc, "el/el00.dat", &el_bytes);
    gh_bundle room_bundle;
    CHECK(gh_bundle_open(room, room_bytes, &room_bundle) == GH_BUNDLE_OK);
    gh_room_pools pools = {0};
    CHECK(gh_game_room_pools_initialize_002a5d80(&pools, &room_bundle, heap) == GH_POOL_OK);
    const uint16_t kinds[] = {0x100, 0x309, 0x376, 0x500, 0x1};
    uint8_t raw[5][0x300] = {{0}};
    gh_title_player_view views[5] = {{0}};
    uint32_t handles[5];
    object_scope scope = {.pools = &pools, .handles = handles, .kinds = kinds, .count = 5};
    for (size_t i = 0; i < 5; ++i) {
        views[i].bytes = raw[i];
        views[i].size = sizeof(raw[i]);
        raw[i][0x2fe] = i == 4 ? 0 : (uint8_t)kinds[i];
        raw[i][0x2ff] = (uint8_t)(kinds[i] >> 8);
        put_word(raw[i] + 0x250, 0x200400);
        CHECK(gh_room_object_register(&pools, &views[i], &scope, kinds[i], 1, &handles[i]) == GH_POOL_OK);
    }

    gh_stage_slots holders = {0};
    gh_game_objects *objects;
    CHECK(gh_game_objects_create(&holders, 0, &objects) == GH_OBJECT_OK);
    CHECK(gh_stage_slots_reset(&holders, NULL) == GH_STAGE_OK);
    int original = 0;
    CHECK(gh_game_objects_add_pinned_001fece0(objects, 0x600, el, el_bytes, &original) == GH_OBJECT_OK && original);

    gh_task_record32 tasks[4] = {{0}};
    gh_task_manager manager = {.records = tasks, .capacity = 4, .count = 4, .current_index = -1};
    gh_task_threads *threads;
    CHECK(gh_task_threads_create(&manager, &threads) == GH_THREAD_OK);
    uint8_t save[GH_CORE_SAVE_BYTES] = {0};
    save[0xae] = 6;
    gh_event_actor_create state = {0};
    gh_event_actor_create_bindings bindings = {
        .components = &components, .data = &prefix, .heap = heap, .pools = &pools,
        .objects = objects, .global_threads = threads, .save = {save, sizeof(save)},
        .object_view = object_view, .context = &scope
    };
    const size_t before = gh_heap_used(heap);
    scope.fail = handles[2];
    put_word(actor.bytes, 2);
    CHECK(gh_event_actor_create_start_002979f0(&state, &bindings, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x123456 && state.object_cursor == 2 && scope.calls == 3);
    CHECK(gh_heap_used(heap) == before + 648 && word(raw[0] + 0x250) == 0x208400);
    CHECK(word(raw[1] + 0x250) == 0x200400 && word(raw[2] + 0x250) == 0x200400);
    const uint8_t *row;
    size_t cut;
    CHECK(gh_event_actor_create_work_at(&state, 8, &row, &cut) == GH_EVENT_OK);
    CHECK(row == (uint8_t *)state.work.pointer + 8 * 72 && cut == 0x9f0 && !word(row + 0x44));
    CHECK(gh_event_actor_create_work_at(&state, 9, &row, &cut) == GH_EVENT_BOUNDS);

    scope.fail = 0;
    CHECK(gh_event_actor_create_start_002979f0(&state, &bindings, &frontier) == GH_EVENT_OK);
    CHECK(!frontier && scope.calls == 6 && state.task_slot == 2 && tasks[2].running == 2);
    CHECK(word(raw[2] + 0x250) == 0x208400 && word(raw[3] + 0x250) == 0x208400);
    CHECK(word(raw[4] + 0x250) == 0x200400 && !word(actor.bytes));
    CHECK(gh_event_actor_create_task_status(&state, &frontier) == GH_EVENT_OK && !frontier);
    CHECK(gh_tasks_run_matching_002d52a0(&manager).status == GH_TASK_OK);
    CHECK(gh_event_actor_create_task_status(&state, &frontier) == GH_EVENT_UNSUPPORTED && frontier == 0x312a80);
    CHECK(gh_event_actor_create_work_at(&state, 0, &row, &cut) == GH_EVENT_OK && cut == 0x130);
    CHECK(word(row + 4) == 0x600 && word(row + 0x40) == 0x6f6 && row[0x24] == 1);
    CHECK(state.pending.loaded_resource.bytes == el && state.pending.loaded_resource.size == el_bytes);
    CHECK(state.pending.source_kind == 0x600 && state.pending.resource_kind == 0x600 &&
          state.pending.create_kind == 0x6f6 && state.pending.create_argument == 0xffff);
    CHECK(gh_event_actor_creation_poll_00297a68(&components, &ready) == GH_EVENT_OK && !ready);
    CHECK(gh_event_actor_create_abort(&state) == GH_EVENT_OK && gh_heap_used(heap) == before && !tasks[2].running);

    CHECK(gh_stage_slots_reset(&holders, NULL) == GH_STAGE_OK);
    gh_stage_slots room_holders = {0};
    gh_game_objects *room_objects;
    CHECK(gh_game_objects_create(&room_holders, 1, &room_objects) == GH_OBJECT_OK);
    CHECK(gh_stage_slots_reset(&room_holders, heap) == GH_STAGE_OK);
    gh_event_actor_factory factory = {0};
    uint8_t serial = 255;
    const uint32_t fog = 0x325476;
    gh_event_actor_factory_bindings factory_bindings = {
        .pools = &pools, .global_objects = objects, .room_objects = room_objects,
        .work = &work, .factory_serial = &serial,
        .resources = {.dvd = &dvd, .heap = heap, .threads = threads},
        .packet_heap = heap, .fog_rgb = &fog
    };
    CHECK(gh_event_actor_factory_bind(&factory, &factory_bindings) == GH_EVENT_OK);
    gh_event_actor_create_request request = {
        .source_kind = 0x600, .resource_kind = 0x600, .create_kind = 0x600, .create_argument = 0xffff
    };
    put_word((uint8_t *)&work + 0x594, 0x100000);
    int nonnull = -1;
    CHECK(gh_event_actor_factory_run(&factory, &request, &nonnull, &frontier) == GH_EVENT_OK);
    CHECK(!nonnull && !frontier && !factory.stage && serial == 255 && gh_heap_used(heap) == before);
    put_word((uint8_t *)&work + 0x594, 0);
    save[0xae] = 0;
    bindings.factory = gh_event_actor_factory_run;
    bindings.factory_context = &factory;
    bindings.initialize = gh_event_actor_factory_initialize;
    CHECK(gh_event_actor_create_start_002979f0(&state, &bindings, &frontier) == GH_EVENT_OK);
    CHECK(gh_tasks_run_matching_002d52a0(&manager).status == GH_TASK_OK);
    CHECK(factory.stage == 2 && factory.load.stage == 1 && !factory.handle && !serial);
    CHECK(gh_event_actor_create_task_status(&state, &frontier) == GH_EVENT_OK && !frontier);
    CHECK(!word(actor.bytes));
    for (unsigned i = 0; i < 1024 && !factory.handle; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_tasks_run_matching_002d52a0(&manager).status == GH_TASK_OK);
    }
    CHECK(factory.handle && factory.stage == 5 && factory.global_reference && !factory.room_reference);
    CHECK(factory.resource.file_extent == el_bytes && !memcmp(factory.resource.bytes, el, el_bytes));
    gh_game_event_status task_status = gh_event_actor_create_task_status(&state, &frontier);
    if (task_status != GH_EVENT_UNSUPPORTED || frontier != 0x297a48) {
        fprintf(stderr, "Actor task status %u frontier %06x model stage %u detail %u init stage %u\n",
            task_status, frontier, factory.model.stage, factory.model.detail, factory.init_stage);
    }
    CHECK(task_status == GH_EVENT_UNSUPPORTED && frontier == 0x297a48);
    CHECK(state.first_actor_handle == factory.handle && factory.init_stage == 4 && state.frontier_rel == 0x1a30);
    CHECK(factory.model.ready && factory.model.parts_ready && factory.model.root.bytes == factory.object.view.bytes);
    const unsigned model_parts = factory.model.parts_allocated;
    CHECK(model_parts == 32 && factory.model.part_count == model_parts);
    CHECK(factory.shadow.object == &factory.object.view && factory.shadow.pattern_original == 0x3c3f68);
    CHECK(factory.geometry.bytes && factory.texture.bytes);
    CHECK(gh_event_actor_creation_poll_00297a68(&components, &ready) == GH_EVENT_OK && !ready);
    gh_title_player_view published;
    CHECK(gh_event_actor_factory_view(&factory, factory.handle, &published) == GH_EVENT_OK);
    CHECK(published.size == 0x30e0 && published.original_vtable214 == 0x44b528 && published.bytes[0x2fc] == 255);
    CHECK(published.bytes[0x2ff] == 6 && word(published.bytes + 0x2d0) == 0x800000);
    CHECK((word(published.bytes + 0x250) & 0x2018000) == 0x2018000 &&
          word(published.bytes + 0x13ac) == 0x3f800000);
    CHECK(word(published.bytes + 0x13cc) == 0 && word(published.bytes + 0x30dc) == 0x3f800000);
    CHECK(!word(published.bytes + 0x304) && !word(published.bytes + 0x2c8));
    CHECK(word(published.bytes + 0x4dc) == 3 && (word(published.bytes + 0x4a8) & 0x20000000u));
    CHECK(published.bytes[0x2f4] == 1 && (word(published.bytes + 0x2f0) & 0x1000u));
    CHECK(!word(published.alias_f0) && !word(published.bytes + 0x100) && word(published.bytes + 0x110) == 0x3f800000);
    CHECK(pools.vectors[0].count == 6 && pools.vectors[3].count == 1);
    CHECK(word((uint8_t *)pools.vectors[0].allocation.pointer + 20) == factory.handle &&
          word(pools.vectors[3].allocation.pointer) == factory.handle);
    CHECK(gh_event_actor_factory_view(&factory, handles[0], &published) == GH_EVENT_ARGUMENT);

    size_t pl00_bytes, pl01_bytes;
    uint8_t *pl00 = load(disc, "pl/pl00.dat", &pl00_bytes);
    uint8_t *pl01 = load(disc, "pl/pl01.dat", &pl01_bytes);
    CHECK(gh_game_objects_add_pinned_001fece0(objects, 0x100, pl00, pl00_bytes, &original) == GH_OBJECT_OK && original);
    CHECK(gh_game_objects_add_pinned_001fece0(objects, 0x101, pl01, pl01_bytes, &original) == GH_OBJECT_OK && original);
    CHECK(gh_game_objects_set_file_extent(objects, 0x101, pl01, pl01_bytes) == GH_OBJECT_OK);
    gh_game_title_player player = {0};
    gh_game_player_factory parent = {0};
    gh_player_factory_bindings parent_bindings = {
        .player = &player, .pools = &pools, .global_objects = objects, .room_objects = room_objects,
        .work = &work, .disc = disc, .save = {save, sizeof(save)}, .factory_serial = &serial
    };
    CHECK(gh_game_player_factory_create_0030fa98(&parent, &parent_bindings, &original) == GH_PLAYER_OK && original);
    gh_actor_model parent_model = {0};
    gh_bundle parent_bundle;
    gh_bundle_entry parent_md, parent_texture;
    CHECK(gh_bundle_open(pl00, pl00_bytes, &parent_bundle) == GH_BUNDLE_OK);
    CHECK(gh_bundle_find(&parent_bundle, "MD", 0, &parent_md) == GH_BUNDLE_OK);
    CHECK(gh_bundle_find(&parent_bundle, "TM3", 0, &parent_texture) == GH_BUNDLE_OK);
    CHECK(gh_actor_model_bind_packet_inputs(&parent_model, heap, &fog) == GH_ACTOR_MODEL_OK);
    CHECK(gh_actor_model_initialize_00149738(&parent_model, &player.view, &work, &pools, &parent_md,
        &parent_texture, 0, 0, GH_STATIC_BINARY32_REFERENCE, &original) == GH_ACTOR_MODEL_OK && original);

    gh_game_player_child child = {0};
    gh_player_child_bindings child_bindings = {
        .pools = &pools, .global_objects = objects, .room_objects = room_objects, .work = &work,
        .factory_serial = &serial, .resources = factory_bindings.resources
    };
    CHECK(gh_game_player_child_create_0030fa98(&child, &child_bindings, &original) == GH_PLAYER_OK && original);
    CHECK(gh_actor_model_bind_packet_inputs(&child.model, heap, &fog) == GH_ACTOR_MODEL_OK);
    CHECK(gh_game_player_child_initialize_00100378(&child, &original) == GH_PLAYER_UNSUPPORTED);
    CHECK(child.frontier == 0x100420 && child.model.ready);
    gh_player_child_attachment attachment = {0};
    CHECK(gh_game_player_child_attach_00100ed8(&attachment, &child, &parent, &parent_model,
        17, bindings.save) == GH_PLAYER_OK);
    CHECK(attachment.attached && parent_model.external_borrowers == 1);
    put_word(child.object.view.bytes + 0x250, 0x8002);
    put_word(child.object.view.bytes + 0x2f0, 0x20);
    gh_event_weapon_slot weapon = {0};
    gh_event_actor_extra extra = {0};
    gh_event_actor_extra_bindings extra_bindings = {
        .factory = &factory, .player = &player.view, .child = &child, .attachment = &attachment,
        .weapon_slot = &weapon, .save = bindings.save
    };
    CHECK(gh_event_actor_extra_bind(&extra, &extra_bindings) == GH_EVENT_OK);
    CHECK(gh_event_actor_extra_run(&extra, &state.pending, factory.handle, &original, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x1cf160 && !original && !extra.mesh_cursor && !parent_model.external_borrowers);
    CHECK(child.model.external_parent == &factory.model && factory.model.external_borrowers == 1);
    CHECK(attachment.parent_part == factory.model.parts + 17 && child.model.external_owner == &attachment);
    CHECK(word(child.object.view.bytes + 0x250) == 0x10000 && word(child.object.view.bytes + 0x2f0) == 0x21);
    CHECK(word(state.pending.work + 0x34) == 0xffff && !word(state.pending.work + 0x30));
    CHECK(gh_event_actor_factory_abort(&factory) == GH_EVENT_CALLEE && factory.handle);
    weapon.known = 1;
    CHECK(gh_event_actor_extra_run(&extra, &state.pending, factory.handle, &original, &frontier) == GH_EVENT_OK);
    CHECK(original && !frontier && extra.mesh_cursor == 3 && word(state.pending.work + 0x30) == 0x101);
    const uint8_t hidden_names[][8] = {"RH_01", "LH_01", "LH_02"};
    for (size_t i = 0; i < 3; ++i) {
        gh_player_mesh_reference ref;
        CHECK(gh_actor_model_find_mesh_0014b730(&factory.model, hidden_names[i], 8, &ref, &original) == GH_PLAYER_NODES_OK);
        CHECK(original && (word(ref.bytes + 0x380) & 1));
    }
    CHECK(gh_event_actor_extra_abort(&extra) == GH_EVENT_OK);
    CHECK(child.model.external_parent == &parent_model && attachment.parent_model == &parent_model &&
          parent_model.external_borrowers == 1 && !factory.model.external_borrowers);
    CHECK(word(child.object.view.bytes + 0x250) == 0x8002 && word(child.object.view.bytes + 0x2f0) == 0x20);
    CHECK(gh_event_actor_create_abort(&state) == GH_EVENT_OK);
    CHECK(gh_event_actor_factory_abort(&factory) == GH_EVENT_OK);
    CHECK(pools.vectors[0].count == 7 && pools.vectors[3].count == 2);

    gh_event_actor_factory *rows = calloc(9, sizeof(*rows));
    CHECK(rows);
    gh_event_actor_roster roster = {0};
    CHECK(gh_event_actor_roster_bind(&roster, &factory_bindings, rows, 9) == GH_EVENT_OK);
    gh_game_math_globals math = {0};
    CHECK(gh_game_math_globals_from_elf(&math, elf, elf_bytes) == GH_MATH_GLOBALS_OK);
    CHECK(gh_game_math_static_initialize_003079c8(&math, &work, 1, 0xffff) == GH_MATH_GLOBALS_OK);
    extra_bindings.factory = &rows[0];
    extra_bindings.roster = &roster;
    extra_bindings.creation = &state;
    extra_bindings.math = &math;
    extra_bindings.boot_elf = elf;
    extra_bindings.boot_elf_size = elf_bytes;
    CHECK(gh_event_actor_extra_bind(&extra, &extra_bindings) == GH_EVENT_OK);
    gh_event_component_view env;
    CHECK(gh_event_component_at(&components, 0, &env) == GH_EVENT_OK);
    put_word(env.bytes + 0x28, 0x40000000); // Original gate skips earlier, already tested suspension.
    bindings.factory = gh_event_actor_roster_run;
    bindings.initialize = gh_event_actor_roster_initialize;
    bindings.factory_context = &roster;
    bindings.extra = gh_event_actor_extra_run;
    bindings.extra_context = &extra;
    bindings.row_limit = 9;
    bindings.finish = gh_event_actor_extra_finish;
    CHECK(gh_event_actor_create_start_002979f0(&state, &bindings, &frontier) == GH_EVENT_OK);
    task_status = GH_EVENT_OK;
    ready = 0;
    for (unsigned i = 0; i < 1024 && !task_status && !ready; ++i) {
        CHECK(gh_dvd_pump(&dvd) == GH_DVD_OK);
        CHECK(gh_tasks_run_matching_002d52a0(&manager).status == GH_TASK_OK);
        task_status = gh_event_actor_create_task_status(&state, &frontier);
        CHECK(gh_event_actor_creation_poll_00297a68(&components, &ready) == GH_EVENT_OK);
    }
    if (task_status || state.actor_cursor != 9) {
        fprintf(stderr, "EL roster status %u frontier %06x REL %04x row %zu details %u/%u\n",
            task_status, frontier, state.frontier_rel, state.actor_cursor, rows[1].model.detail, rows[2].model.detail);
    }
    CHECK(!task_status && !frontier && !state.frontier_rel && state.actor_cursor == 9 && ready);
    CHECK(word(actor.bytes) == 1 && rows[0].model.external_borrowers == 3 && !state.task_live);
    CHECK(gh_event_actor_creation_end_normal_000012a8(&components, &original) == GH_EVENT_OK && original);
    CHECK(extra.row_parts[0].count == 3 && extra.row_parts[1].count == 4);
    CHECK(pools.vectors[0].count == 17 && pools.vectors[3].count == 12);
    CHECK(word((uint8_t *)pools.vectors[0].allocation.pointer + 16 * 4) == child.handle);
    CHECK(rows[3].kind == 0x60a && rows[3].shadow.initialized && rows[3].object.view.bytes[0x2fa0] == 1);
    CHECK(extra.accessory_handle && extra.olivia_accessory.kind == 0x60d && extra.olivia_accessory.model.ready);
    CHECK(extra.olivia_accessory.init_stage == 2 && extra.olivia_accessory.object.view.bytes[0x2b5] == 0xfd);
    CHECK(word(extra.olivia_accessory.object.view.bytes + 0x30a4) == 4 && extra.accessory_parts.count == 7);
    size_t accessory_bytes;
    uint8_t *accessory = load(disc, "el/el0d.dat", &accessory_bytes);
    CHECK(extra.olivia_accessory.resource.file_extent == accessory_bytes &&
          !memcmp(extra.olivia_accessory.resource.bytes, accessory, accessory_bytes));
    free(accessory);
    CHECK(gh_event_actor_create_work_at(&state, 3, &row, &cut) == GH_EVENT_OK);
    CHECK(word(row + 0x30) == 0x60d && (row[0x24] & 8));
    CHECK(rows[4].kind == 0x60b && rows[4].model.ready && extra.pendulum_parent == &rows[3].model);
    CHECK(word(rows[4].object.view.bytes + 0x1394) == 0x2a0 && rows[4].object.view.bytes[0x1390] == 1);
    CHECK(word(rows[4].object.view.bytes + 0x2f20) == 10 && rows[4].object.view.bytes[0x2b5] == 0xfe);
    CHECK(extra.pendulum_profile.loaded && extra.pendulum_profile.original_scratch == 0x61af10);
    const uint8_t *pendulum = rows[4].object.view.bytes + 0x13a0;
    CHECK(word(pendulum + 38 * 0xa0 + 4) == 0xbd75c28f &&
          word(pendulum + 38 * 0xa0 + 0x60) == 0x3d75c28f);
    CHECK(word(pendulum + 31 * 0xa0 + 0x60) == 0);
    static const uint16_t ids[] = {0x600, 0x601, 0x602, 0x60a, 0x60b, 0x60c, 0x630, 0x631, 0x667};
    for (unsigned i = 0; i < sizeof(ids) / sizeof(*ids); ++i) {
        CHECK(rows[i].kind == ids[i] && rows[i].handle == state.actor_handles[i] && rows[i].model.ready);
        CHECK(gh_event_actor_create_work_at(&state, i, &row, &cut) == GH_EVENT_OK);
        CHECK(word(row + 4) == ids[i] && word(row + 0x40) == ids[i]);
    }
    CHECK(rows[6].object.view.bytes[0x4d8] == 1 && rows[7].object.view.bytes[0x4d8] == 1);
    CHECK(rows[8].kind == 0x667 && rows[8].model.ready && !rows[8].object.view.bytes[0x4d8]);
    gh_actor_external_parent_set rejected = {0};
    const gh_actor_external_part_pair cycle = {0, 0};
    CHECK(gh_actor_model_bind_external_parts(&rejected, &rows[0].model, &rows[1].model, &cycle, 1) ==
          GH_ACTOR_MODEL_ARGUMENT && !rejected.count && rows[0].model.external_borrowers == 3);
    const char *el_paths[] = {"el/el01.dat", "el/el02.dat"};
    const uint8_t parts[][4] = {{0, 1, 2, 0}, {0, 1, 17, 18}};
    const uint8_t parents[][4] = {{2, 3, 4, 0}, {15, 16, 9, 10}};
    for (unsigned i = 0; i < 2; ++i) {
        size_t bytes;
        uint8_t *file = load(disc, el_paths[i], &bytes);
        gh_event_actor_factory *member = &rows[i + 1];
        CHECK(member->model.ready && member->model.part_count && !member->shadow.initialized);
        CHECK(member->resource.file_extent == bytes && !memcmp(member->resource.bytes, file, bytes));
        CHECK(gh_event_actor_factory_view(member, state.actor_handles[i + 1], &published) == GH_EVENT_OK);
        CHECK(!word(published.bytes + 0x4dc) && word(published.bytes + 0x30a4) == 2 - i);
        CHECK(word(published.bytes + 0x30b4) == UINT32_MAX && published.bytes[0x4e0] == 1);
        for (unsigned j = 0; j < i + 3; ++j) {
            const uint8_t *world;
            CHECK(gh_actor_model_external_parts_at(&member->model, parts[i][j], &world) == GH_ACTOR_MODEL_OK);
            CHECK(world == rows[0].model.parts[parents[i][j]].bytes);
        }
        CHECK(gh_actor_model_update_hierarchy_00149eb8(&member->model, GH_STATIC_BINARY32_REFERENCE) == GH_ACTOR_MODEL_OK);
        free(file);
    }
    CHECK(gh_event_actor_create_abort(&state) == GH_EVENT_OK);
    CHECK(gh_event_actor_roster_abort(&roster) == GH_EVENT_CALLEE);
    CHECK(gh_event_actor_extra_abort(&extra) == GH_EVENT_OK);
    CHECK(gh_event_actor_roster_abort(&roster) == GH_EVENT_OK);
    CHECK(pools.vectors[0].count == 7 && pools.vectors[3].count == 2);
    free(rows);
    gh_game_player_child_attachment_clear(&attachment);
    CHECK(!attachment.attached && !parent_model.external_borrowers);
    CHECK(gh_game_player_child_release_prefix(&child) == GH_PLAYER_OK);
    CHECK(gh_actor_model_release(&parent_model) == GH_ACTOR_MODEL_OK);
    CHECK(gh_game_player_factory_release_prefix(&parent) == GH_PLAYER_OK);

    CHECK(gh_task_threads_destroy(threads) == GH_THREAD_OK);
    gh_game_objects_destroy(room_objects);
    gh_game_objects_destroy(objects);
    for (size_t i = 0; i < 5; ++i) CHECK(gh_room_object_unregister(&pools, handles[i], &scope) == GH_POOL_OK);
    CHECK(gh_game_room_pools_release(&pools) == GH_POOL_OK);
    CHECK(gh_game_event_resource_abort(&resource) == GH_EVENT_OK);
    CHECK(gh_game_event_module_abort(&module) == GH_EVENT_OK);
    CHECK(!gh_heap_used(heap) && gh_heap_destroy(heap) == GH_MEM_OK);
    free(room);
    free(el);
    free(pl00);
    free(pl01);
    free(elf);
    gh_disc_close(disc);
    printf("PASS EV01 nine original actors and EL0D, EL00 parts=%u, pendulum and final ready\n", model_parts);
    return 0;
}
