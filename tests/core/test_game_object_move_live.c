#include "game_object_step.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
/* Protocol fixtures only: these callbacks validate typed dispatch/resume,
 * not the recovered player body or successful gameplay. Production supplies
 * the real selected body and keeps unsupported methods explicit. */
typedef struct {gh_room_pools*pools;gh_title_player_view views[2];uint32_t handles[2];unsigned calls,prefix,timers;uint8_t phase,blocked,reload_blocked;} context;
static uint32_t timer(void*opaque){context*c=opaque;return ++c->timers;}
static gh_object_step_status resolve(void*opaque,uint32_t handle,gh_title_player_view*out,uint32_t*pc){
 context*c=opaque;void*raw;*pc=0;
 if(c->reload_blocked&&handle==c->handles[1]){*pc=0x30f16c;return GH_OBJSTEP_UNSUPPORTED;}
 if(gh_room_object_lookup(c->pools,handle,999,c,&raw)){*pc=0x30f138;return GH_OBJSTEP_UNSUPPORTED;}
 for(unsigned i=0;i<2;i++)if(handle==c->handles[i]&&raw==&c->views[i]){*out=c->views[i];return GH_OBJSTEP_OK;}
 return GH_OBJSTEP_ARGUMENT;
}
static gh_object_step_status invoke(void*opaque,const gh_object_move_request*r,const gh_boot_work*work,uint32_t*pc){
 context*c=opaque;(void)work;c->calls++;C(!r->this_delta&&r->original_entry==0x108ab0&&r->actor.bytes==c->views[0].bytes&&r->handle==c->handles[0]);
 if(!c->phase){c->prefix++;c->phase=1;}
 if(c->blocked){*pc=0x108ab0;return GH_OBJSTEP_UNSUPPORTED;}
 put(c->pools->vectors[0].allocation.pointer,c->handles[1]);c->reload_blocked=1;c->phase=2;return GH_OBJSTEP_OK;
}
static uint8_t*readfile(const char*path,size_t*n){FILE*f=fopen(path,"rb");C(f&&!fseek(f,0,SEEK_END));long size=ftell(f);C(size>52&&!fseek(f,0,SEEK_SET));uint8_t*p=malloc((size_t)size);C(p&&fread(p,1,(size_t)size,f)==(size_t)size);fclose(f);*n=(size_t)size;return p;}
int main(int argc,char**argv){C(argc==3);size_t elf_size;uint8_t*elf=readfile(argv[2],&elf_size);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t index;gh_disc_entry entry;C(!gh_disc_find(d,"st1/r100.dat",&index)&&!gh_disc_entry_at(d,index,&entry));uint8_t*room=malloc((size_t)entry.size);C(room&&!gh_disc_read(d,index,0,room,(size_t)entry.size));gh_bundle bundle;C(!gh_bundle_open(room,(size_t)entry.size,&bundle));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap));
 gh_game_init_state*init=calloc(1,sizeof(*init));C(init);gh_game_object_step state={0};C(!gh_game_object_step_construct_002c53a8(&state,init));gh_boot_work work={0};context c={.pools=&pools,.blocked=1};uint8_t actors[2][0x5b0]={{0}};
 for(unsigned i=0;i<2;i++){C(!gh_game_object_construct_002a7338(actors[i],sizeof(actors[i]),&c.views[i]));put(actors[i]+0x214,0x41d220);put(actors[i]+0x250,0);put(actors[i]+0x2f0,0xf0000003u+i*4);C(!gh_room_object_register(&pools,&c.views[i],&c,999,i?8:1,&c.handles[i]));}
 gh_object_move_bindings b={resolve,invoke,&c,elf,elf_size};C(!gh_game_object_step_bind_live(&state,&b));uint32_t pc;
 C(gh_game_move_objects_0030f090(&state,&pools,&work,timer,&c,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x108ab0&&c.calls==1&&c.prefix==1&&c.timers==1&&state.move_native_phase==1);
 C(gh_game_object_step_bind_live(&state,&b)==GH_OBJSTEP_ARGUMENT);
 /* Original loop gate was read once; changing it during a paused callee
  * cannot discard the invocation. Completion still reloads the changed slot. */
 put((uint8_t*)&work+0x5d8,0x20000);c.blocked=0;
 C(gh_game_move_objects_0030f090(&state,&pools,&work,timer,&c,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x30f16c&&c.calls==2&&c.prefix==1&&c.timers==1&&state.move_native_phase==2);
 C(w(actors[0]+0x2f0)==0xf0000003&&w(actors[1]+0x2f0)==0xf0000007);
 c.reload_blocked=0;C(!gh_game_move_objects_0030f090(&state,&pools,&work,timer,&c,&pc)&&!pc&&c.calls==2&&c.prefix==1&&c.timers==2&&!state.move_stage&&w(actors[0]+0x2f0)==0xf0000003&&w(actors[1]+0x2f0)==0xf0000004);
 /* Original skipflags do not dereference a virtual descriptor. */
 put((uint8_t*)&work+0x5d8,0);put(actors[1]+0x250,0x8000);put(actors[1]+0x214,UINT32_MAX);C(!gh_game_move_objects_0030f090(&state,&pools,&work,timer,&c,&pc)&&c.calls==2&&c.timers==4);
 put(actors[1]+0x250,0x80000000);C(!gh_game_move_objects_0030f090(&state,&pools,&work,timer,&c,&pc)&&c.calls==2&&c.timers==6);
 put(actors[1]+0x250,0);C(gh_game_move_objects_0030f090(&state,&pools,&work,timer,&c,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x30f15c&&c.calls==2&&c.timers==7);
 /* Unsupported current call is deliberately not completed for cleanup. */
 for(unsigned i=0;i<2;i++)C(!gh_room_object_unregister(&pools,c.handles[i],&c));C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(init);free(elf);free(room);gh_disc_close(d);puts("PASS actual ELF Move descriptor, typed same registry, paused body/gate, postcall slot reload, late retry no duplicate invoke, flags/bounds/timer; protocol fixtures only");return 0;
}
