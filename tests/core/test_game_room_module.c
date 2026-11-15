#include "game_room_module.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)

typedef struct {
 gh_room_module *module;
 uint16_t room;
 int returned;
 gh_room_module_status result;
} context;

static void load(void *opaque,int32_t argument)
{
 context *c=opaque;
 (void)argument;
 c->result=gh_room_module_load_002be708(c->module,c->room);
 c->returned=1;
}

int main(int argc,char **argv)
{
 static const struct {uint16_t room;size_t bytes;uint32_t init,move,end;} profiles[]={
  {0x20,24129,0x1a0,0x220,0x248},{0x100,25663,0x208,0x740,0},
  {3,5005,0x1a0,0x228,0},{0x101,14284,0x1a0,0x520,0},
  {0x102,23217,0x1a8,0x670,0},{0x105,12628,0x1a8,0x398,0},
 };
 gh_disc *disc;
 gh_heap *heap;
 gh_dvd_jobs dvd;
 gh_task_threads *threads;
 gh_task_record32 records[4]={0};
 gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};
 CHECK(argc==2&&gh_disc_open(argv[1],&disc)==GH_DISC_OK&&gh_heap_create(1024*1024,&heap)==GH_MEM_OK);
 gh_dvd_jobs_init(&dvd,disc);
 CHECK(gh_task_threads_create(&manager,&threads)==GH_THREAD_OK);
 gh_room_module module={.input={.dvd=&dvd,.heap=heap,.threads=threads}};
 CHECK(gh_room_module_load_002be708(&module,0x207)==GH_ROOM_MODULE_UNSUPPORTED&&!module.bound&&!gh_heap_used(heap));

 for(size_t i=0;i<sizeof(profiles)/sizeof(profiles[0]);++i){
  for(int corrupt=1;corrupt>=0;--corrupt){
   context c={.module=&module,.room=profiles[i].room};
   int32_t slot;
   CHECK(gh_task_threads_start(threads,1,load,&c,0,&slot)==GH_THREAD_OK);
   unsigned visits=0;
   while(!c.returned&&visits<8){
    CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
    if(!visits){
     CHECK(module.input.allocation.pointer&&module.input.job&&!module.bound);
     CHECK(gh_room_module_release(&module)==GH_ROOM_MODULE_BUSY);
    }
    CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);
    /* Mutate only this owned native buffer after the disc read. */
    if(!visits&&corrupt)((uint8_t*)module.input.allocation.pointer)[0x100]^=1;
    ++visits;
   }
   CHECK(c.returned&&visits==3&&module.input.file_bytes==profiles[i].bytes);
   if(corrupt){
    CHECK(c.result==GH_ROOM_MODULE_VERSION&&!module.bound&&module.frontier==0x394dd8);
   }else{
    CHECK(c.result==GH_ROOM_MODULE_OK&&module.bound&&module.room==profiles[i].room);
    CHECK(module.hook8.kind==GH_SCENE_HOOK_UNRESOLVED&&module.hook12.kind==GH_SCENE_HOOK_UNRESOLVED);
    CHECK(module.init_offset==profiles[i].init&&module.move_offset==profiles[i].move&&module.end_offset==profiles[i].end);
    if(module.room==0x102||module.room==0x105){
     const uint8_t *bytes=module.input.allocation.pointer;
     static const uint8_t original_preinit[]={0x08,0x00,0xe0,0x03,0,0,0,0};
     for(size_t j=0;j<sizeof(original_preinit);j++)CHECK(bytes[0x1a0+j]==original_preinit[j]);
     uint32_t frontier=0xffffffffu;
     CHECK(module.hook4.kind==GH_SCENE_HOOK_NATIVE&&module.hook4.call&&module.hook4.context==&module);
     CHECK(module.hook4.call(module.hook4.context,&frontier)==GH_SCENE_OK&&!frontier);
    }else{
     CHECK(module.hook4.kind==(module.room==0x100?GH_SCENE_HOOK_UNRESOLVED:GH_SCENE_HOOK_ABSENT)&&!module.hook4.call);
    }
    CHECK(gh_room_module_load_002be708(&module,profiles[i].room)==GH_ROOM_MODULE_BUSY);
   }
   CHECK(gh_room_module_release(&module)==GH_ROOM_MODULE_OK&&!module.bound&&!gh_heap_used(heap));
  }
 }
 CHECK(gh_task_threads_destroy(threads)==GH_THREAD_OK&&gh_heap_destroy(heap)==GH_MEM_OK);
 gh_disc_close(disc);
 puts("PASS actual R020/R100/R003/R101/R102/R105 module identity and release, corruption rejection, original R102/R105 PreInit");
 return 0;
}
