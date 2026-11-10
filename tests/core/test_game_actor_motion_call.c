#include "game_actor_motion_call.h"
#include "game_actor_motion_ik.h"
#include "game_effect_release.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t rd(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(uint8_t*p,uint32_t x){memcpy(p,&x,4);}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
typedef struct {gh_effect_release_bindings effects;unsigned calls,completed,block42,block53;int32_t kinds[8];uint32_t modes[8];} bridge;
static gh_actor_motion_status release_effect(void*ctx,const gh_title_player_view*actor,int32_t kind,uint32_t mode,uint32_t*f){bridge*b=ctx;b->calls++;if((kind==42&&b->block42)||(kind==53&&b->block53)){if(kind==42)b->block42=0;else b->block53=0;*f=0x306200;return GH_ACTOR_MOTION_UNSUPPORTED;}
 unsigned slot=0;gh_effect_status e=gh_game_effect_release_00306200(&b->effects,actor,kind,mode,f,&slot);if(e)return GH_ACTOR_MOTION_CALLEE;C(b->completed<8);b->kinds[b->completed]=kind;b->modes[b->completed++]=mode;return GH_ACTOR_MOTION_OK;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t rn,n,cn;uint8_t*r=load(d,"st1/r100.dat",&rn),*data=load(d,"pl/pl00.dat",&n),*core=load(d,"etc/core.dat",&cn);size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en);C(elf&&!gh_disc_read_boot_elf(d,0,elf,en));gh_disc_close(d);
 gh_bundle room,bundle;gh_bundle_entry md,texture;C(!gh_bundle_open(r,rn,&room)&&!gh_bundle_open(data,n,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));gh_heap*heap;C(!gh_heap_create(16*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));uint8_t raw[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view view;C(!gh_title_player_construct_00107e40(raw,sizeof(raw),&view));gh_boot_work work={0};gh_actor_model model={0};int original;C(gh_actor_model_initialize_00149738(&model,&view,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)==GH_ACTOR_MODEL_UNSUPPORTED&&model.parts_ready);
 gh_game_effects*effects=calloc(1,sizeof(*effects));uint8_t*a=calloc(1,0x100180),*b=calloc(1,0x26060);C(a&&b&&effects&&!gh_game_effects_initialize_00305a68(effects,a,0x100180,b,0x26060,core,cn,&room));bridge observer={.effects={.effects=effects,.esp=a,.esp_bytes=0x100180,.emitters=b,.emitter_bytes=0x26060}};gh_motion_ik_globals globals={0};C(!gh_motion_ik_globals_from_elf(&globals,elf,en));gh_actor_motion current={0};gh_actor_motion_bindings bindings={.model=&model,.resource=data,.resource_bytes=n,.work=&work,.effect=release_effect,.context=&observer,.ik_globals=&globals};size_t motion=rd(data+0x28),seq=rd(data+0x2c);
 C(!gh_actor_motion_begin_002a8578(&current,&bindings,motion,motion,0)&&current.complete&&!current.sequence_valid&&observer.completed==1); /* cold570kind0 realcold306200 scan */
 gh_actor_motion_call call={0};C(!gh_actor_motion_call_open_002a8578(&call,&current,&bindings,motion,seq,0));C(!gh_actor_motion_call_step_002a8578(&call)&&call.complete&&current.complete&&current.sequence_valid&&current.sequence_offset==seq);
 wr(raw+0x430,0x40e00000);C(!gh_actor_motion_call_step_002a8578(&call)&&rd(raw+0x430)==0x40e00000); /* retryofcompletedcall, notnewinvocation */
 C(!gh_actor_motion_call_open_002a8578(&call,&current,&bindings,motion,seq,0));C(!gh_actor_motion_call_step_002a8578(&call)&&!rd(raw+0x430)); /* identicalNEWcall resetsframe */
 /* ExplicitmodifiedSEQ release-list fixture insidealreadyloadedprivateDAT.
  * SourceISO untouched; one-1 skip, thenkind42(mode2), scalar570kind53(mode0). */
 uint8_t saved[56];memcpy(saved,data+seq,56);data[seq+0x14]=0x20;data[seq+0x15]=0;data[seq+0x16]=2;data[seq+0x17]=0;wr(data+seq+0x20,UINT32_MAX);wr(data+seq+0x2c,42);data[seq+0x36]=2;raw[0x570]=53;raw[0x571]=0;wr(raw+0x574,0);observer.block42=observer.block53=1;unsigned completed=observer.completed;
 C(!gh_actor_motion_call_open_002a8578(&call,&current,&bindings,motion,motion,0));C(gh_actor_motion_call_step_002a8578(&call)==GH_ACTOR_MOTION_UNSUPPORTED&&call.frontier==0x306200&&call.index==1&&current.complete&&current.sequence_valid&&observer.completed==completed&&raw[0x570]==53);
 C(gh_actor_motion_call_open_002a8578(&call,&current,&bindings,motion,motion,0)==GH_ACTOR_MOTION_BUSY);wr(data+seq+0x2c,99);
 C(gh_actor_motion_call_step_002a8578(&call)==GH_ACTOR_MOTION_UNSUPPORTED&&call.stage==1&&call.pending_kind==53&&current.sequence_valid&&observer.completed==completed+1&&observer.kinds[completed]==42&&observer.modes[completed]==2);
 raw[0x570]=88;wr(raw+0x574,2);C(!gh_actor_motion_call_step_002a8578(&call)&&call.complete&&current.complete&&!current.sequence_valid&&observer.completed==completed+2&&observer.kinds[completed+1]==53&&observer.modes[completed+1]==0&&raw[0x570]==255&&raw[0x571]==255&&rd(raw+0x574)==2);
 C(current.b.model==&model&&current.b.resource==data&&current.motion.offset==motion);memcpy(data+seq,saved,56);
 C(!gh_actor_model_release(&model)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(effects);free(a);free(b);free(r);free(data);free(core);free(elf);puts("PASS SAME motion owner explicitnewcall vsretry, realR100/core cold306200 provider, oldSEQ/570 orderedrelease, injectedunavailableprefix retainsoriginalarguments/oldowner, actualrebind/IKcomplete; positiveSEQlist isexplicitfixture");return 0;}
