#include "game_actor_motion.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t rd(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(uint8_t*p,uint32_t x){memcpy(p,&x,4);}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t rn,n;uint8_t*r=load(d,"st1/r100.dat",&rn),*data=load(d,"pl/pl00.dat",&n);gh_disc_close(d);gh_bundle room,bundle;gh_bundle_entry md,texture;C(!gh_bundle_open(r,rn,&room)&&!gh_bundle_open(data,n,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));
 gh_heap*heap;C(!gh_heap_create(16*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));uint8_t raw[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view view;C(!gh_title_player_construct_00107e40(raw,sizeof(raw),&view));gh_boot_work work={0};gh_actor_model model={0};int original;
 C(gh_actor_model_initialize_00149738(&model,&view,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)==GH_ACTOR_MODEL_UNSUPPORTED&&model.parts_ready&&model.part_count==32); /* realmodel constructor prefix, not animation */
 gh_actor_motion_bindings b={.model=&model,.resource=data,.resource_bytes=n,.work=&work};gh_actor_motion state={0};size_t motion=rd(data+0x28),seq=rd(data+0x2c);
 C(gh_actor_motion_begin_002a8578(&state,&b,motion,seq,0)==GH_ACTOR_MOTION_UNSUPPORTED&&state.frontier==0x306200); /* cold570 zero genuinely needs effectservice */
 raw[0x570]=raw[0x571]=255;wr(model.parts[0].bytes+0x100,0x3f800000);wr(model.parts[0].bytes+0x154,0);wr(model.parts[0].alias_d0,0x42c80000);
 C(gh_actor_motion_begin_002a8578(&state,&b,motion,seq,0)==GH_ACTOR_MOTION_UNSUPPORTED&&state.frontier==0x152448&&state.detail==18&&state.stage==5&&!state.complete);
 C(!rd(model.parts[0].bytes+0x100)&&rd(model.parts[0].bytes+0x110)==0x3f800000&&rd(model.parts[0].alias_d0)!=0x42c80000&&rd(raw+0x444)==rd(raw+0x5a8)&&!rd(raw+0x428)&&!rd(raw+0x42c));
 C(state.caches[0].method==1&&state.caches[0].next_frame>0&&state.caches[0].first_value!=0);uint32_t before=rd(model.parts[0].alias_d0);wr(model.parts[0].alias_d0,0x42c80000);C(gh_actor_motion_begin_002a8578(&state,&b,motion,seq,0)==GH_ACTOR_MOTION_UNSUPPORTED&&state.detail==18&&rd(model.parts[0].alias_d0)==0x42c80000);wr(model.parts[0].alias_d0,before);
 gh_actor_motion_bindings foreign=b;foreign.resource=data+1;C(gh_actor_motion_begin_002a8578(&state,&foreign,motion,seq,0)==GH_ACTOR_MOTION_BUSY);
 C(!gh_actor_model_release(&model)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(r);free(data);puts("PASS actualPL00 motionbinding same32parts, trueeffect306200 frontier, real158198 pose reset/1595F8 decodedcaches/152448 flags then actualIKpart18 frontier; no sampledpose completion claim");return 0;}
