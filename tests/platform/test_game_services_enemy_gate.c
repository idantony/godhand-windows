#include "game_enemy_request.h"
#include "game_scene_setup.h"
#include "r100.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "line %d: %s\n", __LINE__, #expr); return 1; } } while (0)

typedef struct {
    gh_boot_work *work;
    gh_room_state *room;
    uint8_t *stop;
} preinit_context;

static gh_scene_status preinit(void *opaque, uint32_t *frontier)
{
    preinit_context *context=opaque;
    *frontier=0;
    return gh_r100_preinit_000001a0(context->work,context->room,context->stop);
}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    size_t index;
    gh_disc_entry file;
    CHECK(!gh_disc_find(disc,"st1/r100.dat",&index)&&!gh_disc_entry_at(disc,index,&file));
    uint8_t *raw=malloc((size_t)file.size);
    CHECK(raw&&!gh_disc_read(disc,index,0,raw,(size_t)file.size));
    gh_bundle bundle;
    gh_bundle_entry ems;
    CHECK(!gh_bundle_open(raw,(size_t)file.size,&bundle)&&!gh_bundle_find(&bundle,"EMS",0,&ems));
    gh_game_prepare prepare={0};
    uint8_t state=0;
    CHECK(!gh_game_prepare_002a5d40(&prepare,(gh_core_save_view){0},0x100,&state));
    gh_room_state room;
    CHECK(!gh_room_state_select_002bebc8(&prepare,0x100,0,&room));
    gh_boot_work work={.room=0x100};
    uint8_t stop=0xa5,scenario[0x114]={0};
    gh_task_manager manager={0};
    gh_em_scene_placements placements={.stop_flag=&stop};
    preinit_context context={&work,&room,&stop};
    uint32_t frontier;
    CHECK(!gh_scene_before_objects_002c33d8((gh_scene_scenario_view){scenario,sizeof(scenario),&manager},
          &work,&stop,(gh_scene_hook){GH_SCENE_HOOK_NATIVE,preinit,&context},&frontier)&&!frontier&&stop==1);
    CHECK(!gh_em_scene_room_init_00294b38(&placements,&work,scenario[0x14]));
    int loaded=0;
    CHECK(!gh_em_scene_bind_native_00294c68(&placements,&room,&ems,&work,scenario[0x14],
          GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&loaded)&&loaded&&stop==0&&room.active[8]==30);

    gh_enemy_request first,next;
    int spawn;
    int16_t hp;
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
          &first,&spawn,&hp,&frontier)&&spawn&&first.entry_number==0&&first.object_kind==0x202&&
          first.dependency_kind==0x20a&&first.appearance==3&&first.flags==0x04000000u);
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,1,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
          &next,&spawn,&hp,&frontier)&&!spawn);
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,2,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
          &next,&spawn,&hp,&frontier)&&spawn&&next.entry_number==3&&next.object_kind==0x202&&
          next.dependency_kind==0x200&&next.appearance==2);
    CHECK(gh_em_scene_bind_native_00294c68(&placements,&room,&ems,&work,scenario[0x14],
          GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&loaded)==GH_PLACE_UNSUPPORTED&&
          placements.frontier==0x294ca0&&loaded&&stop==0);
    free(raw);
    gh_disc_close(disc);
    puts("PASS actual R100 EMS PreInit stop consumption and canonical row order");
    return 0;
}
