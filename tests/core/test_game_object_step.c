#include "game_object_step.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
/* Clearly labeled platformtimer fixture; nofakeactor/update/draw callbacks. */
typedef struct{uint32_t ticks[2];unsigned calls;} fixture_timer;
static uint32_t count(void*p){fixture_timer*t=p;C(t->calls<2);return t->ticks[t->calls++];}
int main(int argc,char**argv){
 static gh_game_init_state init;gh_game_init_state_initialize(&init);memset(init.side_counter,0xa5,16);
 gh_game_object_step state={.profile_752cf8=12345};C(gh_game_object_step_construct_002c53a8(&state,&init)==GH_OBJSTEP_OK&&state.init==&init&&state.ratio10_bits==0x3f800000&&state.profile_752cf8==12345);
 C(!w(init.side_counter)&&!init.side_counter[4]&&!init.side_counter[5]&&init.side_counter[6]==0xa5&&init.side_counter[7]==0xa5&&w(init.side_counter+8)==0xa5a5a5a5&&w(init.side_counter+12)==0xa5a5a5a5);
 /* ExistingoriginalGameInit resets the SAME on/count/progress span; ratio
  * remainsits separate754C48scalar, not duplicated in a newsideowner. */
 uint8_t savedata[GH_CORE_SAVE_BYTES];gh_core_save_view save={savedata,sizeof(savedata)};C(gh_core_save_system_init(save)==GH_INIT_OK);gh_boot_work work={0};uint8_t started=0;float rate=0;state.ratio10_bits=0x3f000000;
 C(gh_game_init_002a5b18(&init,&work,&started,&rate,save)==GH_INIT_OK&&!init.side_counter[4]&&!w(init.side_counter+8)&&!w(init.side_counter+12)&&state.ratio10_bits==0x3f000000);
 static const struct{uint8_t direction;uint32_t n,p,en,ep;} cases[]={
 {1,3,0,3,1},{255,3,2,0,0},{0,3,2,3,1},{0,3,0,0,0},{1,0,44,0,44},{1,0xffffffff,44,0xffffffff,44},
 {1,3,0x7fffffff,3,0x80000000},{0,3,0x80000000,3,0x7fffffff},{0,3,0xffffffff,0,0}};
 for(unsigned k=0;k<sizeof(cases)/sizeof(cases[0]);k++){init.side_counter[4]=cases[k].direction;put(init.side_counter+8,cases[k].n);put(init.side_counter+12,cases[k].p);C(gh_game_scroll_counter_move_002c53c8(&state)==GH_OBJSTEP_OK&&w(init.side_counter+8)==cases[k].en&&w(init.side_counter+12)==cases[k].ep);}
 gh_disc*d;size_t idx;gh_disc_entry de;C(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK&&gh_disc_find(d,"st0/r020.dat",&idx)==GH_DISC_OK&&gh_disc_entry_at(d,idx,&de)==GH_DISC_OK);uint8_t*r=malloc((size_t)de.size);C(r&&gh_disc_read(d,idx,0,r,(size_t)de.size)==GH_DISC_OK);gh_bundle bundle;C(gh_bundle_open(r,(size_t)de.size,&bundle)==GH_BUNDLE_OK);gh_heap*heap;gh_room_pools pools={0};C(gh_heap_create(8*1024*1024,&heap)==GH_MEM_OK&&gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap)==GH_POOL_OK);
 C(!pools.vectors[0].count&&!pools.vectors[1].count&&pools.vectors[1].capacity>=1);
 init.side_counter[4]=1;put(init.side_counter+8,3);put(init.side_counter+12,0);put((uint8_t*)&work+0x5d8,0xf00c6000);fixture_timer t={{3,0xfffffffe},0};uint32_t frontier;
 C(gh_game_move_objects_0030f090(&state,&pools,&work,count,&t,&frontier)==GH_OBJSTEP_OK&&!frontier&&t.calls==2&&state.profile_752cf8==5&&state.ratio10_bits==0x3f800000&&w(init.side_counter+12)==1);
 C(gh_game_objects_trans_0030f650(&pools,&frontier)==GH_OBJSTEP_OK&&!frontier);
 /* Freeze stillprofiles twice, but preservesratio andsideprogress. */
 put((uint8_t*)&work+0x5d8,0x08000000);state.ratio10_bits=0x3e800000;t=(fixture_timer){{100,140},0};
 C(gh_game_move_objects_0030f090(&state,&pools,&work,count,&t,&frontier)==GH_OBJSTEP_OK&&t.calls==2&&state.profile_752cf8==0xffffffd8&&state.ratio10_bits==0x3e800000&&w(init.side_counter+12)==1);
 /* Pointerregistry1count is NOT sidecountercount; itsopaqueentriesarenot
  * traversed byoriginal2C53C8. Verifyrealallocatedpayload remainsunchanged. */
 pools.vectors[1].count=1;memset(pools.vectors[1].allocation.pointer,0xa5,4);put(init.side_counter+8,0);put(init.side_counter+12,77);put((uint8_t*)&work+0x5d8,0);t=(fixture_timer){{10,11},0};
 C(gh_game_move_objects_0030f090(&state,&pools,&work,count,&t,&frontier)==GH_OBJSTEP_OK&&w(init.side_counter+12)==77&&w(pools.vectors[1].allocation.pointer)==0xa5a5a5a5);pools.vectors[1].count=0;
 /* Positiveunknownobject isallowedonlywhereoriginalNO_OBJ_MOVEskipsit. */
 pools.vectors[0].count=1;memset(pools.vectors[0].allocation.pointer,0xa5,4);put((uint8_t*)&work+0x5d8,0x20000);t=(fixture_timer){{21,22},0};
 C(gh_game_move_objects_0030f090(&state,&pools,&work,count,&t,&frontier)==GH_OBJSTEP_OK&&t.calls==2);
 put((uint8_t*)&work+0x5d8,0);t=(fixture_timer){{31,32},0};
 C(gh_game_move_objects_0030f090(&state,&pools,&work,count,&t,&frontier)==GH_OBJSTEP_UNSUPPORTED&&frontier==0x30f138&&t.calls==1&&state.profile_752cf8==31&&w(pools.vectors[0].allocation.pointer)==0xa5a5a5a5);
 C(gh_game_objects_trans_0030f650(&pools,&frontier)==GH_OBJSTEP_UNSUPPORTED&&frontier==0x30f69c);pools.vectors[0].count=0;
 t=(fixture_timer){{40,41},0};C(gh_game_move_objects_0030f090(&state,&pools,&work,NULL,&t,&frontier)==GH_OBJSTEP_ARGUMENT&&!t.calls);
 C(gh_game_room_pools_release(&pools)==GH_POOL_OK&&gh_heap_used(heap)==0&&gh_heap_destroy(heap)==GH_MEM_OK);free(r);gh_disc_close(d);
 puts("PASS actualR020 MoveObj/ObjTrans emptyregistries, canonicalsidecounter/reset, signed32wrap, timerprofile andgates; unknownobjectsneveracknowledged");return 0;
}
