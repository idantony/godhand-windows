#include "r105_room.h"
#include "core_bundle.h"
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
        CHECK(gh_task_threads_cancel(state->bindings.threads,state->watcher_slot[i])==GH_THREAD_OK);
    CHECK(gh_r105_room_release(state)==GH_SCENE_OK);
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
    uint8_t player_bytes[0x15f8]={0};
    gh_title_player_view player={.bytes=player_bytes,.size=sizeof(player_bytes)};
    gh_boot_work work={.room=0x105};
    const uint32_t event_lock=0x40000000u;
    memcpy((uint8_t*)&work+0x5e4,&event_lock,4);
    gh_room_state room={.active=page,.size=sizeof(page)};
    const gh_r105_room_bindings b={.module=&module,.heap=heap,.work=&work,.room=&room,
        .scenario={scenario,sizeof(scenario),&manager},.threads=threads,.at=&at,
        .init=init,.player=&player,.save={save,sizeof(save)}};
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
    cancel_watchers(&bound);
    CHECK(!gh_scene_at_units_release(&at)&&!gh_room_module_release(&module)&&!gh_heap_used(heap));
    CHECK(!gh_task_threads_destroy(threads)&&!gh_heap_destroy(heap));
    free(data);
    free(init);
    gh_disc_close(disc);
    puts("PASS actual R105 Init allocation, two suspended original watchers, first-visit paper stop and real AT4 binding");
    return 0;
}
