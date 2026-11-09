#include "game_room_module.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {gh_room_module *module;int returned;gh_room_module_status result;} context;
static void load(void*p,int32_t arg){context*c=p;(void)arg;c->result=gh_room_module_load_002be708(c->module,0x20);c->returned=1;}
int main(int argc,char**argv){
 gh_disc*d;gh_heap*h;gh_dvd_jobs dvd;gh_task_threads*t;gh_task_record32 records[4]={0};
 gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};
 CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK&&gh_heap_create(1024*1024,&h)==GH_MEM_OK);
 gh_dvd_jobs_init(&dvd,d);CHECK(gh_task_threads_create(&manager,&t)==GH_THREAD_OK);
 gh_room_module module={.input={.dvd=&dvd,.heap=h,.threads=t}};context c={.module=&module};int32_t slot;
 CHECK(gh_room_module_load_002be708(&module,0x207)==GH_ROOM_MODULE_UNSUPPORTED&&!module.bound&&!gh_heap_used(h));
 for(unsigned corrupt=1;;corrupt--){
  c.returned=0;CHECK(gh_task_threads_start(t,1,load,&c,0,&slot)==GH_THREAD_OK);unsigned visits=0;
  while(!c.returned&&visits<8){
   CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
   if(!visits){CHECK(module.input.allocation.pointer&&module.input.job&&!module.bound);CHECK(gh_room_module_release(&module)==GH_ROOM_MODULE_BUSY);}
   CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);
   /* Corrupt only this native read allocation after actual DVD completion.
    * No source disc or reference file is changed. */
   if(!visits&&corrupt)((uint8_t*)module.input.allocation.pointer)[0x100]^=1;
   visits++;
  }
  CHECK(c.returned&&visits==3&&module.input.file_bytes==24129);
  if(corrupt)CHECK(c.result==GH_ROOM_MODULE_VERSION&&!module.bound&&module.frontier==0x394dd8);
  else {
   CHECK(c.result==GH_ROOM_MODULE_OK&&module.bound&&module.room==0x20);
   CHECK(module.hook4.kind==GH_SCENE_HOOK_ABSENT&&!module.hook4.call);
   CHECK(module.hook8.kind==GH_SCENE_HOOK_UNRESOLVED&&module.hook12.kind==GH_SCENE_HOOK_UNRESOLVED);
   CHECK(module.init_offset==0x1a0&&module.move_offset==0x220&&module.end_offset==0x248);
   CHECK(gh_room_module_load_002be708(&module,0x20)==GH_ROOM_MODULE_BUSY);
  }
  CHECK(gh_room_module_release(&module)==GH_ROOM_MODULE_OK&&!module.bound&&!gh_heap_used(h));if(!corrupt)break;
 }
 CHECK(gh_task_threads_destroy(t)==GH_THREAD_OK&&gh_heap_destroy(h)==GH_MEM_OK);gh_disc_close(d);
 puts("PASS realR020 native module identity,3fiber visits, corrupt-source rejection, unresolved exports, release");return 0;
}
