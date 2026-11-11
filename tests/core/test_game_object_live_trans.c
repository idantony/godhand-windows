#include "game_object_live_trans.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
typedef struct {gh_room_pools*pools;gh_title_player_view actors[3];uint32_t handles[3];unsigned resolves,invokes,fail_call,trace_count;gh_object_trans_request trace[12];uint32_t hidden_layers[8];} fixture;
static gh_object_step_status resolve(void*ctx,uint32_t handle,gh_title_player_view*out,uint32_t*f){fixture*x=ctx;x->resolves++;for(unsigned i=0;i<3;i++)if(x->handles[i]==handle){void*p;C(!gh_room_object_lookup(x->pools,handle,2,x,&p)&&p==x->actors+i);*out=x->actors[i];return GH_OBJSTEP_OK;}*f=0x30f69c;return GH_OBJSTEP_UNSUPPORTED;}
static gh_object_step_status unused_move(void*c,const gh_object_move_request*r,const gh_boot_work*w,uint32_t*f){(void)c;(void)r;(void)w;*f=0x30f15c;return GH_OBJSTEP_UNSUPPORTED;}
/* Typed provider contract fixture, not a modeldraw/destructor implementation.
 * Production without an actual provider retains the real14A3E8/14A910 frontier. */
static gh_object_step_status invoke(void*ctx,const gh_object_trans_request*r,const gh_boot_work*w,uint32_t*f){fixture*x=ctx;(void)w;x->invokes++;C(r->this_delta==0&&r->actor.bytes&&x->trace_count<12);x->trace[x->trace_count++]=*r;if(x->fail_call==x->invokes){*f=r->original_entry;return GH_OBJSTEP_UNSUPPORTED;}return GH_OBJSTEP_OK;}
static gh_object_step_status layer(void*ctx,const gh_scene_at_manager*at,uint8_t id,int*visible,uint32_t*f){fixture*x=ctx;(void)f;C(at->field68);*visible=!(x->hidden_layers[id>>5]&(0x80000000u>>(id&31)));return GH_OBJSTEP_OK;}
static void register_actor(fixture*x,unsigned i,uint8_t*raw,uint32_t flags){C(!gh_title_player_construct_00107e40(raw,GH_TITLE_PLAYER_BYTES,x->actors+i));put(raw+0x250,flags);raw[0x2fd]=(uint8_t)(i*35);uint16_t id=0x100;memcpy(raw+0x2fe,&id,2);C(!gh_room_object_register(x->pools,x->actors+i,x,2,9,x->handles+i));}
static void setup(gh_game_object_step*s,gh_game_init_state*init,fixture*x,const uint8_t*elf,size_t size,gh_scene_at_manager*at,int callback){memset(s,0,sizeof(*s));C(!gh_game_object_step_construct_002c53a8(s,init));gh_object_move_bindings live={resolve,unused_move,x,elf,size};C(!gh_game_object_step_bind_live(s,&live));gh_object_trans_bindings trans={.invoke=callback?invoke:NULL,.layer=callback?layer:NULL,.context=x,.scene_at=at};C(!gh_game_object_step_bind_trans(s,&trans));}
static void unregister_all(fixture*x){for(unsigned i=0;i<3;i++)if(x->handles[i]){void*p;if(!gh_room_object_lookup(x->pools,x->handles[i],2,x,&p))C(!gh_room_object_unregister(x->pools,x->handles[i],x));x->handles[i]=0;}}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t index;gh_disc_entry en;C(!gh_disc_find(d,"st1/r100.dat",&index)&&!gh_disc_entry_at(d,index,&en));uint8_t*room=malloc((size_t)en.size);C(room&&!gh_disc_read(d,index,0,room,(size_t)en.size));size_t es=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(es);C(elf&&!gh_disc_read_boot_elf(d,0,elf,es));gh_disc_close(d);gh_bundle bundle;C(!gh_bundle_open(room,(size_t)en.size,&bundle));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap));
 fixture x={.pools=&pools};uint8_t bytes[3][GH_TITLE_PLAYER_BYTES];gh_game_init_state init;gh_game_init_state_initialize(&init);gh_scene_at_manager at={.ready=1};gh_boot_work work={0};gh_game_object_step s;uint32_t pc;
 register_actor(&x,0,bytes[0],0);setup(&s,&init,&x,elf,es,&at,0);
 C(gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x14a3e8&&s.trans_live_phase==4&&word(bytes[0]+0x250)==0x200000&&x.resolves==1);
 uint32_t saved_handle=word(pools.vectors[0].allocation.pointer);pools.vectors[0].count=0;C(gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)==GH_OBJSTEP_POOLS&&pc==0x30f69c&&s.trans_live_phase==4);pools.vectors[0].count=1;put(pools.vectors[0].allocation.pointer,0xffffffff);C(gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)==GH_OBJSTEP_POOLS&&pc==0x30f69c&&x.resolves==1);put(pools.vectors[0].allocation.pointer,saved_handle);
 for(unsigned retry=0;retry<3;retry++)C(gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x14a3e8&&x.resolves==1&&s.trans_index==0);
 C(gh_game_object_step_bind_trans(&s,&(gh_object_trans_bindings){0})==GH_OBJSTEP_ARGUMENT);unregister_all(&x);
 /* Hidden does not even resolve the poisoned vtable. */
 memset(&x,0,sizeof(x));x.pools=&pools;register_actor(&x,0,bytes[0],0x8000);put(bytes[0]+0x214,0xffffffff);setup(&s,&init,&x,elf,es,&at,0);C(!gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)&&!pc&&!s.trans_stage&&word(bytes[0]+0x250)==0x8000);unregister_all(&x);
 /* Normal has a realNULL-mask pass; alternate bypasses layerquery. */
 memset(&x,0,sizeof(x));x.pools=&pools;register_actor(&x,0,bytes[0],0x80000);at.field68=1;setup(&s,&init,&x,elf,es,&at,0);C(gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x14a910&&s.trans_live_request.kind==GH_OBJECT_TRANS_ALTERNATE);unregister_all(&x);
 memset(&x,0,sizeof(x));x.pools=&pools;register_actor(&x,0,bytes[0],0);setup(&s,&init,&x,elf,es,&at,0);C(gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x2c3190&&s.trans_live_phase==2);unregister_all(&x);at.field68=0;
 /* Two nativeprovider invocations retain actualslotorder and do not replay
  * the first completed consumer after the second fails. */
 memset(&x,0,sizeof(x));x.pools=&pools;register_actor(&x,0,bytes[0],0);register_actor(&x,1,bytes[1],0x80000);setup(&s,&init,&x,elf,es,&at,1);x.fail_call=2;C(gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x14a910&&s.trans_index==1&&x.invokes==2&&x.trace[0].handle==x.handles[0]&&x.trace[1].handle==x.handles[1]);x.fail_call=0;C(!gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)&&x.invokes==3&&x.trace[2].handle==x.handles[1]&&!s.trans_stage);unregister_all(&x);
 /* MSB-first wordmask excludes one normallayer without invoking its body. */
 memset(&x,0,sizeof(x));x.pools=&pools;register_actor(&x,0,bytes[0],0);register_actor(&x,1,bytes[1],0);at.field68=1;x.hidden_layers[1]=0x10000000;setup(&s,&init,&x,elf,es,&at,1);C(!gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)&&x.invokes==1&&x.trace[0].handle==x.handles[0]);unregister_all(&x);at.field68=0;
 /* Two-frame originalretirement, then secondarydetach before strictbody. */
 memset(&x,0,sizeof(x));x.pools=&pools;register_actor(&x,0,bytes[0],0x80000000);setup(&s,&init,&x,elf,es,&at,0);C(!gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)&&word(bytes[0]+0x250)==0xc0000000);C(!gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)&&word(bytes[0]+0x250)==0xe0000000);C(gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x107ec8&&pools.vectors[3].count==0&&pools.vectors[0].count==1&&s.trans_live_request.argument==3);for(unsigned retry=0;retry<3;retry++)C(gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x107ec8&&!pools.vectors[3].count&&pools.vectors[0].count==1);unregister_all(&x);
 /* Contractfixture destructor does not unregister; originalcaller's erase
  * keeps the next replacement slot at the sameindex and processes it once. */
 memset(&x,0,sizeof(x));x.pools=&pools;register_actor(&x,0,bytes[0],0xe0000000);register_actor(&x,1,bytes[1],0);uint32_t removed=x.handles[0],kept=x.handles[1];setup(&s,&init,&x,elf,es,&at,1);C(!gh_game_objects_trans_static_0030f650(&s,&pools,&work,&pc)&&x.invokes==2&&x.trace[0].kind==GH_OBJECT_TRANS_DESTROY&&x.trace[0].argument==3&&x.trace[0].handle==removed&&x.trace[1].handle==kept&&pools.vectors[0].count==1&&word(pools.vectors[0].allocation.pointer)==kept&&pools.vectors[3].count==1);unregister_all(&x);
 C(!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(elf);free(room);puts("PASS actualISO registries +actualELF/playerVT14A3E8/14A910/107EC8 strictfrontiers, flags/layer/retire/detach/mixedslotorder/retry; providerfixtures are NOT modeldraw success");return 0;
}
