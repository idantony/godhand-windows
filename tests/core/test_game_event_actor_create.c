#include "game_event_actor_create.h"


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

    CHECK(gh_task_threads_destroy(threads) == GH_THREAD_OK);
    gh_game_objects_destroy(objects);
    for (size_t i = 0; i < 5; ++i) CHECK(gh_room_object_unregister(&pools, handles[i], &scope) == GH_POOL_OK);
    CHECK(gh_game_room_pools_release(&pools) == GH_POOL_OK);
    CHECK(gh_game_event_resource_abort(&resource) == GH_EVENT_OK);
    CHECK(gh_game_event_module_abort(&module) == GH_EVENT_OK);
    CHECK(!gh_heap_used(heap) && gh_heap_destroy(heap) == GH_MEM_OK);
    free(room);
    free(el);
    free(elf);
    gh_disc_close(disc);
    puts("PASS real EV01 actor work/cuts, R100 registry suspend retry, EL00 holder, native task and strict factory");
    return 0;
}
