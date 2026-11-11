#include "r100_exit.h"
#include "r100_init.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)

static uint32_t word(const void *p){uint32_t value;memcpy(&value,p,4);return value;}
static void put(void *p,uint32_t value){memcpy(p,&value,4);}

typedef struct {
    gh_game_door *door;
    const gh_game_door_bindings *bindings;
    unsigned calls,leaves;
} door_fixture;

static gh_door_status empty_player_aux(void *context,uint32_t entry,uint32_t *frontier)
{
    door_fixture *fixture=context;
    const uint8_t *player=fixture->bindings->player->view.bytes;
    C(entry==(fixture->leaves?0x12b9f0:0x12ac60));
    C(!word(player+(fixture->leaves?0x6a0:0x698)));
    ++fixture->leaves;
    *frontier=0;
    return GH_DOOR_OK;
}

static gh_door_status begin_room(void *context,uint16_t room,const float position[3],
    float yaw,uint8_t kind,uint32_t flags,uint8_t force,uint32_t *frontier)
{
    door_fixture *fixture=context;
    ++fixture->calls;
    C(room==3&&!position[0]&&!position[1]&&!position[2]&&!yaw&&!kind&&flags==2&&!force);
    gh_door_status status=gh_game_door_request_001ffeb8(fixture->door,
        fixture->bindings,room,position,yaw,kind,flags,force);
    *frontier=fixture->door->frontier;
    return status;
}

int main(int argc,char **argv)
{
    gh_disc *disc;
    gh_disc_entry file;
    size_t index;
    C(argc==2&&gh_disc_open(argv[1],&disc)==GH_DISC_OK);
    C(gh_disc_find(disc,"st1/r100.dat",&index)==GH_DISC_OK&&gh_disc_entry_at(disc,index,&file)==GH_DISC_OK);
    uint8_t *asset=malloc((size_t)file.size);
    C(asset&&gh_disc_read(disc,index,0,asset,(size_t)file.size)==GH_DISC_OK);
    gh_bundle room;
    gh_bundle_entry jumps;
    C(gh_bundle_open(asset,(size_t)file.size,&room)==GH_BUNDLE_OK);
    C(gh_bundle_find(&room,"JMP",0,&jumps)==GH_BUNDLE_OK&&word(jumps.bytes)==4);
    C(jumps.bytes[12]==5&&jumps.bytes[13]==1);
    gh_game_scene_tables actors={0};
    C(gh_scene_actors_0012e670(&actors,&room)==GH_TABLE_OK);
    gh_heap *heap;
    gh_scene_at_manager at={0};
    gh_room_pools pools={0};
    C(gh_heap_create(16*1024*1024,&heap)==GH_MEM_OK);
    C(gh_game_room_pools_initialize_002a5d80(&pools,&room,heap)==GH_POOL_OK);
    C(gh_scene_at_initialize_units_002c0f90(&at,&room,heap,0x100,1)==GH_AT_OK);
    gh_boot_work work={.room=0x100};
    gh_room_state room_state={0};
    gh_game_prepare *prepare=calloc(1,sizeof(*prepare));
    C(prepare&&gh_room_state_select_002bebc8(prepare,0x100,0,&room_state)==GH_ROOM_OK);
    gh_r100_init init={0};
    gh_scene_meshes meshes={0};
    const gh_r100_init_bindings init_bindings={.work=&work,.room_state=&room_state,.at=&at,
        .pools=&pools,.meshes=&meshes,.default_heap=heap};
    C(gh_r100_initialize_00000208(&init,&init_bindings)==GH_R100_INIT_UNSUPPORTED);
    C(init.jump_callback_relative==0xa38&&init.frontier==0x9b0);

    gh_at_native_unit exit_unit;
    C(gh_scene_at_get_jump_002c2ae0(&at,0,&exit_unit)==GH_AT_OK&&exit_unit.bytes&&exit_unit.bytes[0x3b]==8&&exit_unit.bytes[0x3a]==0x20&&exit_unit.bytes[0x39]==4);
    gh_game_title_player player={0};
    int original;
    C(gh_game_player_construct_00312f00(&player,&pools,&work,&original)==GH_PLAYER_OK&&original);
    /* Authored door polygon has ray-endpoint activation, not origin activation. */
    const float x=2.65f,y=0.0f,z=-80.2f;
    const float matrix[]={1,0,0,0,0,1,0,0,0,0,1,0,x,y,z,1};
    memcpy(player.view.bytes+0x80,matrix,sizeof(matrix));
    C(player.view.alias_f0==player.view.bytes+0xb0);

    gh_game_door door={0};
    C(gh_game_door_from_disc(&door,disc)==GH_DOOR_OK);
    const size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_size);
    gh_display_state display;
    C(elf&&gh_disc_read_boot_elf(disc,0,elf,elf_size)==GH_DISC_OK);
    C(gh_display_state_from_elf(&display,elf,elf_size)==GH_DISPLAY_OK);
    C(gh_display_static_initialize_002a48a8(&display,1,0xffff)==GH_DISPLAY_OK);
    free(elf);
    gh_screen_fade fade={0};
    gh_game_scene_step step={0};
    gh_game_loop loop={0};
    gh_task_record32 records[4]={{0}};
    gh_task_manager manager={records,4,0,-1,4,0,0,0};
    gh_task_threads *threads;
    uint8_t scenario[0x114]={0};
    C(gh_task_threads_create(&manager,&threads)==GH_THREAD_OK);
    gh_scene_scenario_view view={.bytes=scenario,.size=sizeof(scenario),.manager=&manager};
    gh_menu_state menu={0};
    gh_game_scene_ui messages={0};
    C(gh_scene_message_initialize_002aea18(&messages,&menu,&room,1)==GH_SCENE_UI_OK);
    door_fixture door_call={.door=&door};
    const gh_game_door_bindings door_bindings={.work=&work,.loop=&loop,.scenario=view,
        .messages=&messages,.player=&player,.scene_step=&step,.fade=&fade,.display=&display,
        .external=empty_player_aux,.context=&door_call};
    door_call.bindings=&door_bindings;

    gh_game_action_buttons buttons={0};
    gh_game_action_buttons_reset_001f72c0(&buttons);
    gh_game_init_state *game_init=malloc(sizeof(*game_init));
    C(game_init);
    gh_game_init_state_initialize(game_init);
    size_t language_index;
    gh_disc_entry language_file;
    C(gh_disc_find(disc,"etc/core_usa.bin",&language_index)==GH_DISC_OK);
    C(gh_disc_entry_at(disc,language_index,&language_file)==GH_DISC_OK);
    uint8_t *language_bytes=malloc((size_t)language_file.size);
    C(language_bytes&&gh_disc_read(disc,language_index,0,language_bytes,(size_t)language_file.size)==GH_DISC_OK);
    gh_bundle language;
    gh_bundle_entry idd;
    gh_id_state *ids=calloc(1,sizeof(*ids));
    gh_id_runtime *runtime;
    C(ids&&gh_bundle_open(language_bytes,(size_t)language_file.size,&language)==GH_BUNDLE_OK);
    C(gh_id_find_language_data(&language,&idd)==GH_MENU_OK&&gh_id_system_init_002acc70(ids,&idd)==GH_MENU_OK);
    C(gh_id_runtime_create(ids,1,&runtime)==GH_ID_OK);
    C(gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST)==GH_ID_OK);
    uint8_t save_bytes[GH_CORE_SAVE_BYTES];
    gh_core_save_view save={save_bytes,sizeof(save_bytes)};
    gh_game_mt random={.index=625};
    gh_game_cockpit *cockpit;
    C(gh_core_save_system_init(save)==GH_INIT_OK);
    C(gh_game_cockpit_create(runtime,ids,&random,&cockpit)==GH_ID_OK);
    C(gh_game_cockpit_initialize_00140c88(cockpit,save)==GH_ID_OK);

    gh_event_reserve_work deferred;
    memset(&deferred,0xa5,sizeof(deferred));
    gh_r100_exit owner={.bindings={.work=&work,.room=&room_state,.at=&at,.buttons=&buttons,
        .scenario=view,.threads=threads,.event_work=&deferred,.begin_room=begin_room,.context=&door_call}};
    const gh_scene_at_step_callbacks callbacks={.context=&owner,.spawn=gh_r100_exit_spawn_001f7858};
    const gh_action_step_bindings action={.buttons=&buttons,.work=&work,.cockpit=cockpit,
        .current_player=&player.view,.init=game_init,.auxiliary_binding_known=1,
        .callback=gh_r100_exit_action_002c2138,.context=&owner};
    uint32_t frontier;
    gh_scene_at_status status=gh_scene_at_step_native_002c1618(&at,&actors,&player.view,&work,&callbacks,&original,&frontier);
    if(status||!(buttons.entries[0].flags&1))
        fprintf(stderr,"AT status=%u frontier=%x x=%x y=%x z=%x unitflags=%x\n",status,frontier,
            actors.ray.toc0[0],actors.ray.toc0[1],actors.ray.toc0[2],word(exit_unit.bytes+0x50));
    C(status==GH_AT_OK&&!original&&!frontier&&buttons.entries[0].callback_argument==exit_unit.bytes);
    C(buttons.entries[0].life==0x20&&buttons.entries[0].kind==2&&buttons.entries[0].priority==4);
    gh_action_button_frame action_frame={0};
    C(gh_action_button_frame_begin(&action_frame)==GH_ACTION_STEP_OK);
    C(gh_action_button_step_001f7360(&action_frame,&action)==GH_ACTION_STEP_OK);
    C(!owner.pending&&!records[2].running&&deferred.flags==0xa5a5a5a5u);

    C(gh_scene_at_step_native_002c1618(&at,&actors,&player.view,&work,&callbacks,&original,&frontier)==GH_AT_OK);
    put((uint8_t*)&work+0x1a4,8);
    put((uint8_t*)&work+8,0x11223344);
    C(gh_action_button_frame_begin(&action_frame)==GH_ACTION_STEP_OK);
    C(gh_action_button_step_001f7360(&action_frame,&action)==GH_ACTION_STEP_OK&&action_frame.complete);
    C(owner.pending&&owner.slot==2&&records[2].running==2);
    C(word(scenario+0x64)==0&&word(scenario+0x68)==0x747618&&!(word(room_state.active+4)&0x400000));
    C(!word((uint8_t*)&work+8)&&!word((uint8_t*)&work+0x1a4)&&(word(exit_unit.bytes+0x50)&0x02000000));
    C(!(word((uint8_t*)&work+0x5d8)&0x02000000)&&!(buttons.entries[0].flags&1));
    C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
    C(!owner.pending&&!owner.frontier&&!owner.status&&door_call.calls==1&&door_call.leaves==2);
    C((word(room_state.active+4)&0x400000)&&deferred.mode==2&&deferred.flags==11&&deferred.destination_room==0x105);
    C(word(&deferred.position[0])==0xc0875c29&&word(&deferred.position[1])==0xc03c28f6);
    C(word(&deferred.position[2])==0x429a6666&&word(&deferred.yaw)==0xc010a3d7);
    C(door.target_room==3&&door.flags==2&&!door.force3a&&!door.kind);
    C(fade.state&&step.cooldown_1b0==30&&(word((uint8_t*)&work+0x5e4)&0x2000000));
    C(gh_task_threads_destroy(threads)==GH_THREAD_OK);
    C(gh_game_title_player_release(&player)==GH_PLAYER_OK);
    C(gh_r100_init_release(&init)==GH_R100_INIT_OK);
    C(gh_scene_at_units_release(&at)==GH_AT_OK&&gh_game_room_pools_release(&pools)==GH_POOL_OK);
    C(gh_scene_actors_reset_0012e800(&actors)==GH_TABLE_OK&&gh_heap_destroy(heap)==GH_MEM_OK);
    C(gh_game_cockpit_destroy(cockpit)==GH_ID_OK&&gh_id_runtime_destroy(runtime)==GH_ID_OK);
    free(ids);free(language_bytes);free(game_init);
    free(prepare);free(asset);gh_disc_close(disc);
    puts("PASS actual R100 exit geometry and button, 2C2138 Scenario task, A38 deferred R105 and room3 Door begin");
    return 0;
}
