#include "game_scenario_end_soft_event.h"
#include "disc.h"
#include "game_camera_follow.h"
#include "game_checkpoint.h"
#include "game_room_state.h"
#include <fenv.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static uint32_t get(const void *p) {
    uint32_t result;
    memcpy(&result, p, 4);
    return result;
}

static void put(void *p, uint32_t v) {
    memcpy(p, &v, 4);
}

typedef struct {
    gh_title_player_view player, child, om;
    uint8_t calls[9];
    unsigned count;
    unsigned fail_first_pose;
} fixture;

static void record(fixture *f, unsigned number) {
    CHECK(f->count < sizeof(f->calls));
    f->calls[f->count++] = (uint8_t)number;
}

static gh_game_event_status clear(void *ctx, const gh_title_player_view *p, uint32_t *frontier) {
    fixture *f = ctx;
    CHECK(p == &f->player);
    record(f, 1);
    *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status pose(void *ctx, uint32_t *frontier) {
    fixture *f = ctx;
    record(f, 2);
    if (f->fail_first_pose) {
        f->fail_first_pose = 0;
        *frontier = 0x30a2b0;
        return GH_EVENT_UNSUPPORTED;
    }
    return GH_EVENT_OK;
}

static gh_game_event_status camera_switch(void *ctx, uint32_t *frontier) {
    record(ctx, 3);
    *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status enemies(void *ctx, uint32_t *frontier) {
    record(ctx, 4);
    *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status object(void *ctx, uint32_t handle, gh_title_player_view *out, uint32_t *frontier) {
    fixture *f = ctx;
    CHECK(handle == 9);
    record(f, 5);
    *out = f->om;
    *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status player(void *ctx, const gh_title_player_view *p, uint32_t *frontier) {
    fixture *f = ctx;
    CHECK(p == &f->player);
    record(f, 6);
    const gh_soft_player_suspend_links links = {.child_5f8 = &f->child, .known_mask = 15};
    return gh_soft_player_unsuspend_0012c7d0(p, &links, frontier);
}

int main(int argc, char **argv) {
    CHECK(argc == 2);
    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1], &disc));
    size_t entry;
    CHECK(!gh_disc_find(disc, "event/ev01.dat", &entry));
    const size_t elf_size = (size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf = malloc(elf_size);
    CHECK(elf && !gh_disc_read_boot_elf(disc, 0, elf, elf_size));
    gh_game_event_config config = {0};
    CHECK(!gh_game_event_config_from_elf(&config, elf, elf_size));
    CHECK(!gh_game_event_config_set_00299320(&config, 1, elf, elf_size));
    CHECK(config.flags == 8);

    uint8_t p[GH_TITLE_PLAYER_BYTES] = {0}, child[0x254] = {0}, om[0x600] = {0};
    fixture f = {.child = {.bytes = child, .size = sizeof(child), .original_vtable214 = 0x41b6a0},
                 .om = {.bytes = om, .size = sizeof(om)}, .fail_first_pose = 1};
    CHECK(!gh_title_player_construct_00107e40(p, sizeof(p), &f.player));
    put(p + 0x250, UINT32_MAX);
    put(p + 0x5a0, UINT32_MAX);
    put(child + 0x214, 0x41b6a0);
    put(child + 0x250, UINT32_MAX);
    om[0x2ff] = 3;
    put(om + 0x250, UINT32_MAX);

    gh_boot_work work = {0};
    uint8_t *raw = (uint8_t *)&work;
    put(raw + 0x5d8, UINT32_MAX);
    put(raw + 0x5e0, UINT32_MAX);
    put(raw + 0x5e4, UINT32_MAX);
    uint8_t scenario[0x114] = {0};
    scenario[0x111] = 0xff;
    scenario[0xe0] = 4;
    scenario[0xe8] = 2;
    gh_task_record32 task = {.attributes = 3};
    gh_task_manager manager = {.records = &task, .capacity = 1, .current = &task, .count = 1, .flags = 3};
    uint32_t handle = 9;
    gh_room_pools pools = {.initialized = 1};
    pools.vectors[3] = (gh_room_vector){.allocation = {.pointer = &handle}, .count = 1, .capacity = 1};
    const gh_event_start_bindings room_objects = {
        .work = &work, .pools = &pools, .object_view = object, .context = &f
    };
    const gh_soft_end_bindings b = {
        .scenario = {scenario, sizeof(scenario), &manager}, .work = &work, .current_player = &f.player,
        .room_objects = &room_objects, .player_clear_0012c418 = clear,
        .camera_pose_0013b9b8 = pose, .camera_switch_00138d40 = camera_switch,
        .enemies_unsuspend_002948e8 = enemies, .player_unsuspend_0012c7d0 = player, .context = &f
    };
    uint32_t frontier;
    gh_soft_event_end nested = {0};
    CHECK(!gh_scenario_end_soft_event_002c3c10(&nested, &b, &frontier));
    CHECK(scenario[0xe8] == 1 && !f.count && get(raw + 0x5e4) == UINT32_MAX);
    gh_soft_event_end end = {0};
    CHECK(gh_scenario_end_soft_event_002c3c10(&end, &b, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x30a2b0 && !scenario[0xe8] && scenario[0x111] == 0xff);
    CHECK(get(raw + 0x5d8) == 0x010bffff && get(raw + 0x5e0) == 0xffefffff &&
          get(raw + 0x5e4) == 0xbaf6ffbf && task.attributes == 3 && manager.flags == 3);
    CHECK(!gh_scenario_end_soft_event_002c3c10(&end, &b, &frontier));
    const uint8_t expected[] = {1, 2, 2, 3, 2, 3, 4, 5, 6};
    CHECK(!frontier && f.count == sizeof(expected) && !memcmp(f.calls, expected, sizeof(expected)));
    CHECK(!scenario[0x111] && task.attributes == 2 && manager.flags == 2);
    CHECK(get(p + 0x250) == 0xffff7fff && get(p + 0x5a0) == 0xfffffff7 &&
          get(child + 0x250) == 0xffff7fff && get(om + 0x250) == 0xffff7fff);
    CHECK(!gh_scenario_end_soft_event_002c3c10(&end, &b, &frontier));
    CHECK(f.count == sizeof(expected));

    const gh_soft_player_suspend_links unknown_weapon = {.known_mask = 7};
    put(p + 0x250, UINT32_MAX);
    CHECK(gh_soft_player_unsuspend_0012c7d0(&f.player, &unknown_weapon, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x12c8b8 && get(p + 0x250) == UINT32_MAX);
    gh_game_init_state init;
    gh_game_init_state_initialize(&init);
    CHECK(!gh_soft_enemies_unsuspend_empty_002948e8(&init, &frontier));
    init.enemy_top = init.enemies;
    CHECK(gh_soft_enemies_unsuspend_empty_002948e8(&init, &frontier) == GH_EVENT_UNSUPPORTED);
    CHECK(frontier == 0x2948e8);
    gh_game_pl_camera camera = {0};
    init.active_camera = &init.camera;
    f.player.initial_memory = GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;
    CHECK(!gh_game_camera_construct_pl_00139bc8(&camera, &init.camera, GH_CAMERA_NATIVE_BINARY32_REFERENCE));
    gh_camera_follow follow = {0};
    CHECK(!gh_game_camera_bind_default_00139db8(&follow, &camera, &init, &f.player,
          &work, elf, elf_size, &frontier));
    const float position[3] = {10, 20, 30}, offsets[3] = {2, 3, 4}, distance = 5;
    const float target[3] = {12, 23, 34}, eye[3] = {12, 23, 29};
    memcpy(f.player.alias_f0, position, 12);
    memcpy(camera.fields + 0x400, offsets, 12);
    memset(camera.fields + 0x410, 0, 12);
    memcpy(camera.fields + 0x420, &distance, 4);
    put(camera.fields + 0x20c, 0xdeadbeef);
    put(camera.fields + 0x21c, 0xdeadbeef);
    put(camera.fields + 0x3dc, 0xdeadbeef);
    put(camera.fields + 0x520, 0xdeadbeef);
    gh_game_camera other_camera = {0};
    init.active_camera = &other_camera;
    const int modes[] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    for (unsigned i = 0; i < 4; ++i) {
        put(camera.fields + 0x51c, UINT32_MAX);
        CHECK(!fesetround(modes[i]) && !feclearexcept(FE_ALL_EXCEPT) && !feraiseexcept(FE_DIVBYZERO));
        CHECK(!gh_game_camera_reset_pose_0013b9b8(&follow, &init, &frontier));
        CHECK(!frontier && !memcmp(camera.fields + 0x200, target, 12) &&
              !memcmp(camera.fields + 0x210, eye, 12));
        CHECK(get(camera.fields + 0x20c) == 0xdeadbeef && get(camera.fields + 0x21c) == 0xdeadbeef &&
              get(camera.fields + 0x3dc) == 0xdeadbeef && get(camera.fields + 0x520) == 0xdeadbeef &&
              !get(camera.fields + 0x51c) && get(camera.fields + 0x250) == 0x41f80000);
        CHECK(init.active_camera == &other_camera && follow.player == &f.player);
        CHECK(fegetround() == modes[i] && fetestexcept(FE_ALL_EXCEPT) == FE_DIVBYZERO);
    }
    CHECK(!fesetround(FE_TONEAREST));
    init.active_camera = NULL;
    put(camera.fields + 0x51c, 7);
    CHECK(!gh_game_camera_reset_pose_0013b9b8(&follow, &init, &frontier) &&
          !frontier && get(camera.fields + 0x51c) == 7);
    /* A direct R100 post-event checkpoint bypasses scene-entry suppression. */
    size_t r100_index;
    gh_disc_entry r100_entry;
    CHECK(!gh_disc_find(disc,"st1/r100.dat",&r100_index)&&!gh_disc_entry_at(disc,r100_index,&r100_entry));
    uint8_t *r100=malloc((size_t)r100_entry.size);
    CHECK(r100&&!gh_disc_read(disc,r100_index,0,r100,(size_t)r100_entry.size));
    gh_bundle room_bundle;
    gh_bundle_entry placements;
    CHECK(!gh_bundle_open(r100,(size_t)r100_entry.size,&room_bundle));
    CHECK(!gh_bundle_find(&room_bundle,"EMS",0,&placements)&&placements.size>0x40);
    gh_game_prepare *prepare=calloc(1,sizeof(*prepare));
    uint8_t *destination=malloc(GH_CHECKPOINT_BYTES+2);
    uint8_t save_bytes[GH_CORE_SAVE_BYTES];
    gh_core_save_view save={save_bytes,sizeof(save_bytes)};
    uint8_t game_state=0;
    CHECK(prepare&&destination&&!gh_core_save_system_init(save));
    CHECK(!gh_game_prepare_002a5d40(prepare,save,0x100,&game_state));
    gh_room_state room;
    CHECK(!gh_room_state_select_002bebc8(prepare,0x100,0,&room));
    put(room.active+4,get(room.active+4)|0x01000000u);
    work.room=0x100;
    work.flags=0x08000000u;
    work.flags598=0x800u;
    uint16_t hp=73;
    memcpy(f.player.bytes+0x54a,&hp,2);
    put(init.heat,0x42f00000);put(init.heat+4,0x41a00000);
    memset(destination,0xa5,GH_CHECKPOINT_BYTES+2);
    gh_game_checkpoint checkpoint={0};
    const gh_checkpoint_bindings checkpoint_b={&work,save,prepare,&init,&f.player,
        destination+1,GH_CHECKPOINT_BYTES};
    CHECK(gh_game_checkpoint_002bf8f8(&checkpoint,&checkpoint_b)==GH_CHECKPOINT_SKIPPED);
    CHECK(!checkpoint.captured&&destination[1]==0xa5);
    CHECK(!gh_game_checkpoint_002c05b0(&checkpoint,&checkpoint_b));
    CHECK(checkpoint.captured&&!checkpoint.last_skipped&&!checkpoint.frontier);
    CHECK(get(save_bytes+0x84)==73&&destination[0]==0xa5&&destination[GH_CHECKPOINT_BYTES+1]==0xa5);
    CHECK(!memcmp(destination+1,save_bytes,GH_CORE_SAVE_BYTES));
    CHECK(!memcmp(destination+1+0x14a0,prepare->fields+0x1660,0x9c30));
    CHECK(!memcmp(destination+1+0xb0d0,f.player.alias_f0,12));
    CHECK(get(destination+1+0xb0e8)==0x41a00000&&get(destination+1+0xb0ec)==0x42f00000);
    free(destination);free(prepare);free(r100);
    free(elf);
    gh_disc_close(disc);
    puts("PASS EV01 boot ISO, nested EndSoftEvent, exact input masks, reset retry/order, canonical player/child/OM/task release, direct R100 full0 checkpoint");
    return 0;
}
