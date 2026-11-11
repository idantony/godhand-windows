#include "r003_event.h"
#include "game_player_abort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(e) do{if(!(e)){fprintf(stderr,"line %d: %s\n",__LINE__,#e);return 1;}} while(0)

typedef struct {gh_room_module module;gh_room_pack pack;int done,error;} load_state;

static void load(void *context,int32_t argument)
{
    (void)argument;
    load_state *s=context;
    s->error=gh_game_room_pack_load_0031cc48(&s->pack,3);
    if(!s->error)s->error=gh_room_module_load_002be708(&s->module,3);
    s->done=1;
}

typedef struct {gh_game_player_abort state;gh_player_abort_bindings bindings;} abort_state;

static gh_soft_event_status abort_player(void *context,uint32_t *frontier)
{
    abort_state *s=context;
    const gh_player_abort_status status=gh_game_player_abort_00126770(&s->state,&s->bindings);
    *frontier=s->state.frontier;
    return status?GH_SOFT_EVENT_CALLEE:GH_SOFT_EVENT_OK;
}

typedef struct {
    gh_game_door door;
    gh_game_door_bindings bindings;
    unsigned end_calls,jump_calls,door_calls;
    uint8_t door_ready;
} flow_state;

static gh_game_event_status end_soft(void *context,uint32_t *frontier)
{
    flow_state *flow=context;
    ++flow->end_calls;
    *frontier=flow->end_calls==1?0x2c3c10:0;
    return *frontier?GH_EVENT_UNSUPPORTED:GH_EVENT_OK;
}

static gh_door_status door_leaf(void *context,uint32_t address,uint32_t *frontier)
{
    flow_state *flow=context;
    ++flow->door_calls;
    *frontier=address;
    if(!flow->door_ready)return GH_DOOR_UNSUPPORTED;
    if(address!=0x12ac60&&address!=0x12b9f0)return GH_DOOR_ARGUMENT;
    *frontier=0;
    return GH_DOOR_OK;
}

static gh_door_status begin_jump(void *context,uint16_t room,const float position[3],float yaw,
    uint8_t kind,uint32_t flags,uint8_t force,uint32_t *frontier)
{
    flow_state *flow=context;
    ++flow->jump_calls;
    const gh_door_status status=gh_game_door_request_001ffeb8(&flow->door,&flow->bindings,
        room,position,yaw,kind,flags,force);
    *frontier=flow->door.frontier;
    return status;
}

int main(int argc,char **argv)
{
    gh_disc *disc;
    gh_heap *heap;
    CHECK(argc==2&&!gh_disc_open(argv[1],&disc)&&!gh_heap_create(16*1024*1024,&heap));
    size_t index;
    gh_disc_entry entry;
    CHECK(!gh_disc_find(disc,"event/ev02.dat",&index)&&!gh_disc_entry_at(disc,index,&entry)&&entry.size);
    printf("R003 reserve event/ev02.dat %llu bytes\n",(unsigned long long)entry.size);
    gh_dvd_jobs dvd;
    gh_dvd_jobs_init(&dvd,disc);
    gh_task_record32 records[16]={0};
    gh_task_manager manager={.records=records,.capacity=16,.count=16,.current_index=-1};
    gh_task_threads *threads;
    CHECK(!gh_task_threads_create(&manager,&threads));
    load_state loader={.module={.input={.dvd=&dvd,.heap=heap,.threads=threads}},
        .pack={.dvd=&dvd,.heap=heap,.threads=threads}};
    int32_t slot;
    CHECK(!gh_task_threads_start(threads,-1,load,&loader,0,&slot)&&slot==2);
    for(unsigned i=0;i<20&&!loader.done;++i)
        CHECK(!gh_tasks_run_matching_002d52a0(&manager).status&&!gh_dvd_pump(&dvd));
    CHECK(loader.done&&!loader.error&&loader.module.bound&&loader.module.input.file_bytes==5005&&
        loader.module.init_offset==0x1a0&&loader.module.move_offset==0x228&&!loader.module.end_offset&&
        loader.pack.file_bytes==1031104&&loader.module.hook4.kind==GH_SCENE_HOOK_ABSENT&&
        loader.module.hook8.kind==GH_SCENE_HOOK_UNRESOLVED&&loader.module.hook12.kind==GH_SCENE_HOOK_UNRESOLVED);
    gh_bundle bundle;
    CHECK(!gh_game_room_pack_bundle(&loader.pack,&bundle));

    const size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);
    void *elf=malloc(elf_size);
    gh_game_event event;
    gh_game_event_config config={0};
    gh_display_state display={0};
    gh_screen_fade fade={0};
    CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_size)&&!gh_game_event_from_elf(&event,elf,elf_size)&&
        !gh_game_event_config_from_elf(&config,elf,elf_size)&&!gh_display_state_from_elf(&display,elf,elf_size)&&
        !gh_display_static_initialize_002a48a8(&display,1,0xffff));
    gh_boot_work work={.room=3};
    uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0};
    gh_game_title_player player={.ready=1};
    CHECK(!gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&player.view));
    gh_sound_control sound={.initialized=1};
    abort_state abort={.bindings={.player=&player,.sound=&sound}};
    uint8_t scenario_bytes[0x114]={0};
    gh_scene_scenario_view scenario={scenario_bytes,sizeof(scenario_bytes),&manager};
    flow_state flow={.bindings={.work=&work,.scenario=scenario,.player=&player,.external=door_leaf}};
    flow.bindings.context=&flow;
    CHECK(!gh_game_door_from_elf(&flow.door,elf,elf_size));
    gh_event_reserve_work reserve={0};
    gh_r003_bindings bindings={.module=&loader.module,.heap=heap,.disc=disc,.scenario=scenario,.threads=threads,
        .reserve=&reserve,.soft={.scenario=scenario,.work=&work,.player_abort_00126770=abort_player,.context=&abort},
        .play={.event=&event,.config=&config,.work=&work,.player=&player.view,.fade=&fade,
            .display=&display,.elf=elf,.elf_bytes=elf_size},
        .end_soft=end_soft,.begin_jump=begin_jump,.context=&flow};

    gh_r003_event no_reserve={0};
    CHECK(!gh_r003_event_init_000001a0(&no_reserve,&bindings)&&no_reserve.slot==-1&&
        !gh_r003_event_release(&no_reserve));
    reserve=(gh_event_reserve_work){.flags=0xb,.mode=2,.destination_room=0x105};
    const uint32_t position[]={0xc0875c29,0xc03c28f6,0x429a6666},yaw=0xc010a3d7;
    memcpy(reserve.position,position,sizeof(position));
    memcpy(&reserve.yaw,&yaw,sizeof(yaw));
    memcpy((uint8_t *)&work+0x5e4,&(uint32_t){0x10},4);
    gh_r003_event task={0};
    CHECK(!gh_r003_event_init_000001a0(&task,&bindings)&&task.slot==2&&!event.fields.flags);
    const uint32_t initial[]={0,0x3f000000,0x3c23d70a};
    CHECK(!memcmp(task.work.pointer,initial,sizeof(initial)));
    const void *allocated=task.work.pointer;
    CHECK(!gh_r003_event_init_000001a0(&task,&bindings)&&task.work.pointer==allocated&&task.slot==2);
    CHECK(!gh_tasks_run_matching_002d52a0(&manager).status&&task.event_number==2&&
        !task.status&&!event.fields.flags&&reserve.flags==0xb);
    memset((uint8_t *)&work+0x5e4,0,4);
    CHECK(!gh_tasks_run_matching_002d52a0(&manager).status&&!task.status&&
        event.fields.data_number==2&&(event.fields.flags&1)&&scenario_bytes[0xe0]==4&&reserve.flags==0xb);
    CHECK(!gh_tasks_run_matching_002d52a0(&manager).status&&!task.status&&!flow.end_calls);
    CHECK(gh_r003_event_release(&task)==GH_R003_UNSUPPORTED);

    /* Isolate the original post-play cursor. The fixture has no EV02 playback
     * provider; EndSoft and Door's external leaves report readiness explicitly. */
    event.fields.flags&=~1u;
    CHECK(!gh_tasks_run_matching_002d52a0(&manager).status&&task.status==GH_R003_UNSUPPORTED&&
        task.frontier==0x2c3c10&&reserve.flags==0xb&&flow.end_calls==1&&!flow.jump_calls);
    work.flags|=0x100;
    CHECK(!gh_tasks_run_matching_002d52a0(&manager).status&&task.status==GH_R003_UNSUPPORTED&&
        task.frontier==0x12ac60&&reserve.flags==0xa&&flow.end_calls==2&&flow.jump_calls==1&&!task.finished);
    CHECK(flow.door.target_room==0x105&&flow.door.point==0xff&&flow.door.kind==0&&
        !flow.door.flags&&flow.door.force3a==1&&flow.door.yaw_bits==yaw&&
        !memcmp(flow.door.position_bits,position,sizeof(position)));

    reserve.position[0]=1; /* A suspended Door request retains its captured arguments. */
    flow.door_ready=1;
    CHECK(!gh_tasks_run_matching_002d52a0(&manager).status&&!task.status&&task.finished&&
        !task.frontier&&flow.end_calls==2&&flow.jump_calls==2&&flow.door_calls==3);
    CHECK(flow.door.target_room==0x105&&flow.door.yaw_bits==yaw&&
        !memcmp(flow.door.position_bits,position,sizeof(position))&&work.room==3);
    CHECK(!gh_r003_event_release(&task)&&!gh_room_module_release(&loader.module)&&
        !gh_game_room_pack_release(&loader.pack)&&!gh_task_threads_destroy(threads)&&
        !gh_heap_used(heap)&&!gh_heap_destroy(heap));
    free(elf);
    gh_disc_close(disc);
    puts("PASS actual R003 DAT/REL and EV02 PlayStart; deferred R105 native Door request with original pose and retry");
    return 0;
}
