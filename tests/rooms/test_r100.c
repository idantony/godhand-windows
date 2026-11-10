#include "../../src/rooms/r100.h"
#include "../../src/core/game_room_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {gh_room_module*m;int done;gh_room_module_status error;} loader;
static void load(void*p,int32_t a){(void)a;loader*s=p;s->error=gh_room_module_load_002be708(s->m,0x100);s->done=1;}
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
int main(int argc,char**argv){gh_disc*d;gh_heap*h;gh_dvd_jobs dvd;gh_task_threads*t;gh_task_record32 records[4]={0};
 gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};C(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_heap_create(1024*1024,&h));
 gh_dvd_jobs_init(&dvd,d);C(!gh_task_threads_create(&manager,&t));gh_room_module m={.input={.dvd=&dvd,.heap=h,.threads=t}};
 for(unsigned corrupt=0;corrupt<2;corrupt++){loader s={.m=&m};int32_t slot;C(!gh_task_threads_start(t,1,load,&s,0,&slot));unsigned visits=0;
  while(!s.done&&visits<8){C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);C(!gh_dvd_pump(&dvd));
   if(!visits&&corrupt)((uint8_t*)m.input.allocation.pointer)[0x100]^=1;visits++;
  }C(s.done&&visits==3&&m.input.file_bytes==25663);
  if(corrupt)C(s.error==GH_ROOM_MODULE_VERSION&&!m.bound&&m.frontier==0x394dd8);
  else {C(!s.error&&m.bound&&m.room==0x100&&m.hook4.kind==GH_SCENE_HOOK_UNRESOLVED&&m.hook8.kind==GH_SCENE_HOOK_UNRESOLVED&&m.hook12.kind==GH_SCENE_HOOK_UNRESOLVED);
   C(m.init_offset==0x208&&m.move_offset==0x740&&!m.end_offset);const uint8_t*p=m.input.allocation.pointer;
   C(!word(p+0x3400)&&!word(p+0x3404)&&word(p+0x740)==0x03e00008&&!word(p+0x744));
  }C(!gh_room_module_release(&m)&&!m.bound&&!gh_heap_used(h));
 }
 gh_boot_work work={0};uint8_t room[0x1650]={0},stop=7;gh_room_state state={.active=room,.size=sizeof(room)};
 C(!gh_r100_preinit_000001a0(&work,&state,&stop)&&stop==1);
 const unsigned offsets[]={0x590,0x594};const uint32_t masks[]={0x200000,0x800};
 for(unsigned i=0;i<2;i++){put((uint8_t*)&work+offsets[i],masks[i]);stop=7;C(!gh_r100_preinit_000001a0(&work,&state,&stop)&&stop==7);put((uint8_t*)&work+offsets[i],0);}
 put(room+4,0x1000000);stop=7;C(!gh_r100_preinit_000001a0(&work,&state,&stop)&&stop==7);put(room+4,0);
 /* Poison adjacent fields; the raw REL reads Sys590,594 and RoomSave+4. */
 put((uint8_t*)&work+0x58c,0x200000);put((uint8_t*)&work+0x598,0x800);put(room,0x1000000);stop=7;
 C(!gh_r100_preinit_000001a0(&work,&state,&stop)&&stop==1&&!gh_r100_move_00000740());
 state.active=NULL;stop=7;C(gh_r100_preinit_000001a0(&work,&state,&stop)==GH_SCENE_ARGUMENT&&stop==7);
 C(!gh_task_threads_destroy(t)&&!gh_heap_destroy(h));gh_disc_close(d);puts("PASS pinnedR100 module/fiber waits/corrupt identity rejection/empty ctor list/real PreInit gates/actual Move return");return 0;
}
