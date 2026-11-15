#include "r105_room.h"
#include "core_bundle.h"
#include "game_scene_at_step.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"fixture line %d: %s\n",__LINE__,#x);exit(1);}}while(0)

static uint32_t word(const void *p)
{
    uint32_t result;
    memcpy(&result,p,4);
    return result;
}

typedef struct {gh_room_module *module; int finished;gh_room_module_status status;} module_task;
static void load_module(void *context,int32_t arg)
{
    module_task *task=context;
    (void)arg;
    task->status=gh_room_module_load_002be708(task->module,0x105);
    task->finished=1;
}

static void cancel_watchers(gh_r105_room *state)
{
    for(unsigned i=0;i<2;i++)
        if(state->watcher_slot[i]>=0)
            CHECK(gh_task_threads_cancel(state->bindings.threads,state->watcher_slot[i])==GH_THREAD_OK);
    CHECK(gh_r105_room_release(state)==GH_SCENE_OK);
}

typedef struct {
    uint8_t stamps[2];
    float position[3];
    unsigned visits;
} at0_observer;

static gh_scene_status record_goto(void *context,uint8_t stamp,const float position[3],uint32_t *frontier)
{
    at0_observer *observer=context;
    CHECK(observer->visits<2&&!memcmp(observer->position,position,sizeof(observer->position)));
    observer->stamps[observer->visits++]=stamp;
    *frontier=0;
    return GH_SCENE_OK;
}

int main(int argc,char **argv)
{
    gh_disc *disc;
    gh_heap *heap;
    gh_dvd_jobs dvd={0};
    gh_task_threads *threads;
    gh_task_record32 records[16]={0};
    gh_task_manager manager={.records=records,.capacity=16,.count=16,.current_index=-1};
    CHECK(argc==2&&gh_disc_open(argv[1],&disc)==GH_DISC_OK&&gh_heap_create(2*1024*1024,&heap)==GH_MEM_OK);
    gh_dvd_jobs_init(&dvd,disc);
    CHECK(gh_task_threads_create(&manager,&threads)==GH_THREAD_OK);
    gh_room_module module={.input={.dvd=&dvd,.heap=heap,.threads=threads}};
    module_task load={.module=&module};
    int32_t slot;
    CHECK(gh_task_threads_start(threads,-1,load_module,&load,0,&slot)==GH_THREAD_OK&&slot>=0);
    for(unsigned visit=0;visit<8&&!load.finished;++visit){
        CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
        CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);
    }
    CHECK(load.finished&&load.status==GH_ROOM_MODULE_OK&&module.bound&&module.room==0x105);

    size_t index;
    gh_disc_entry file;
    CHECK(!gh_disc_find(disc,"st1/r105.dat",&index)&&!gh_disc_entry_at(disc,index,&file)&&file.size==1120320);
    uint8_t *data=malloc((size_t)file.size);
    CHECK(data&&!gh_disc_read(disc,index,0,data,(size_t)file.size));
    gh_bundle room_pack;
    CHECK(!gh_bundle_open(data,(size_t)file.size,&room_pack));
    gh_scene_at_manager at={0};
    CHECK(!gh_scene_at_initialize_units_002c0f90(&at,&room_pack,heap,0x105,1));
    gh_at_native_unit initial_at4;
    CHECK(!gh_scene_at_find_id_002c3118(&at,4,&initial_at4)&&initial_at4.bytes&&word(initial_at4.bytes+0x3c)==0);
    gh_at_native_unit none;
    CHECK(!gh_scene_at_find_id_002c3118(&at,0xfffff,&none)&&!none.bytes);

    gh_game_init_state *init=calloc(1,sizeof(*init));
    CHECK(init);
    uint8_t page[0x1650]={0},save[GH_CORE_SAVE_BYTES]={0},scenario[0x114]={0};
    _Alignas(16) uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0};
    gh_title_player_view player;
    CHECK(!gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&player));
    gh_boot_work work={.room=0x105};
    const uint32_t event_lock=0x40000000u;
    memcpy((uint8_t*)&work+0x5e4,&event_lock,4);
    gh_room_state room={.active=page,.size=sizeof(page)};
    at0_observer observer={.position={-6,-3,0.3f}};
    const gh_r105_room_bindings b={.module=&module,.heap=heap,.work=&work,.room=&room,
        .scenario={scenario,sizeof(scenario),&manager},.threads=threads,.at=&at,
        .init=init,.player=&player,.save={save,sizeof(save)},.context=&observer,
        .goto_player_00295f80=record_goto};
    gh_r105_room bound={0};
    CHECK(gh_r105_initialize_000001a8(&bound,&b)==GH_SCENE_UNSUPPORTED);
    CHECK(bound.frontier==0x12c8d8&&bound.stage==4&&word(page+4)==0x40000000u);
    CHECK(bound.watcher_slot[0]>=0&&bound.watcher_slot[1]>=0&&bound.watcher_slot[0]!=bound.watcher_slot[1]);
    const uint8_t *raw=bound.work.pointer;
    for(unsigned offset=0x50;offset<=0x120;offset+=16)
        CHECK(!word(raw+offset)&&!word(raw+offset+4)&&!word(raw+offset+8)&&word(raw+offset+12)==0x3f800000u);
    CHECK(gh_r105_initialize_000001a8(&bound,&b)==GH_SCENE_UNSUPPORTED&&bound.stage==4&&bound.work.pointer==raw);
    for(unsigned i=0;i<5;i++)CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
    CHECK(!bound.watcher_frontier[0]&&!bound.watcher_frontier[1]&&word(page+4)==0x40000000u);
    CHECK(gh_r105_room_release(&bound)==GH_SCENE_UNSUPPORTED);
    cancel_watchers(&bound);

    CHECK(gh_r105_initialize_000001a8(&bound,&b)==GH_SCENE_UNSUPPORTED);
    CHECK(bound.frontier==0x5f8&&bound.stage==6&&word(initial_at4.bytes+0x3c)==0x488&&initial_at4.bytes[0x44]==0xff);
    gh_at_native_unit at0;
    CHECK(!gh_scene_at_find_id_002c3118(&at,0,&at0)&&at0.bytes&&!(word(at0.bytes+0x50)&0x80000000u));
    CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&!observer.visits);
    gh_game_scene_tables tables={0};
    CHECK(!gh_scene_actors_0012e670(&tables,&room_pack));
    const gh_scene_at_step_callbacks callbacks={0};
    memcpy(player.alias_f0,observer.position,sizeof(observer.position));
    uint32_t frontier;
    int returned;
    CHECK(!gh_scene_at_step_native_002c1618(&at,&tables,&player,&work,&callbacks,&returned,&frontier));
    CHECK(!frontier&&!returned&&(word(at0.bytes+0x50)&0x80000000u));
    CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
    CHECK(!bound.watcher_frontier[0]&&bound.watcher_slot[0]<0&&observer.visits==2&&
        observer.stamps[0]==3&&observer.stamps[1]==5);
    CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&observer.visits==2);
    CHECK(!gh_scene_actors_reset_0012e800(&tables));
    cancel_watchers(&bound);
    CHECK(!gh_scene_at_units_release(&at)&&!gh_room_module_release(&module)&&!gh_heap_used(heap));
    CHECK(!gh_task_threads_destroy(threads)&&!gh_heap_destroy(heap));
    free(data);
    free(init);
    gh_disc_close(disc);
    puts("PASS actual R105 Init, real AT4 binding, and one real AT0 geometry entry: original stamps 3 then 5");
    return 0;
}
