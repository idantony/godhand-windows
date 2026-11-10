#define main original_actor_nodes_test
#include "test_game_actor_model_nodes.c"
#undef main
#include "game_actor_model_trans.h"
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static void putf(void*p,float x){memcpy(p,&x,4);}
static float getf(const void*p){float x;memcpy(&x,p,4);return x;}
static int visible_oracle(const gh_actor_model*m,const gh_game_pl_camera*camera){
 const uint8_t*p=m->root.bytes;unsigned common=63;
 for(unsigned corner=0;corner<8;corner++){long double local[4],world[4],clip[4];for(unsigned k=0;k<3;k++)local[k]=(long double)getf(p+0x260+k*4)+((corner&(1u<<k))?1:-1)*(long double)getf(p+0x26c+k*4);local[3]=1;
  for(unsigned r=0;r<4;r++){world[r]=0;for(unsigned k=0;k<4;k++){float a;memcpy(&a,p+0x80+(k*4+r)*4,4);world[r]+=(long double)a*local[k];}}for(unsigned r=0;r<4;r++){clip[r]=0;for(unsigned k=0;k<4;k++){float a;memcpy(&a,camera->fields+0x80+(k*4+r)*4,4);clip[r]+=(long double)a*world[k];}}
  unsigned flags=0;for(unsigned k=0;k<3;k++){if(clip[k]>fabsl(clip[3]))flags|=1u<<(k*2);if(clip[k]<-fabsl(clip[3]))flags|=2u<<(k*2);}common&=flags;
 }return !common;
}
typedef struct {unsigned calls;gh_actor_trans_request request;} observe;
static gh_actor_model_status required(void*ctx,const gh_actor_trans_request*r,uint32_t*f){observe*x=ctx;x->calls++;x->request=*r;*f=r->entry;return GH_ACTOR_MODEL_UNSUPPORTED;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t size;uint8_t*room=load(d,"st1/r100.dat",&size);gh_bundle rb;C(!gh_bundle_open(room,size,&rb));size_t es=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(es);C(elf&&!gh_disc_read_boot_elf(d,0,elf,es));uint8_t*data=load(d,"pl/pl00.dat",&size);gh_disc_close(d);gh_bundle bundle;gh_bundle_entry md,texture;C(!gh_bundle_open(data,size,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(32*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&rb,heap));
 _Alignas(16)uint8_t bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view view;C(!gh_title_player_construct_00107e40(bytes,sizeof(bytes),&view));gh_boot_work work={0};gh_actor_model model={0};uint32_t fog=0;int original;C(!gh_actor_model_bind_packet_inputs(&model,heap,&fog)&&!gh_actor_model_initialize_00149738(&model,&view,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)&&original&&model.ready);
 gh_game_pl_camera camera={.constructed=1};for(unsigned k=0;k<4;k++){putf(camera.fields+k*20,1);putf(camera.fields+0x80+k*20,k==3?1:.1f);}uint8_t shift=0;gh_game_world_light*light=calloc(1,sizeof(*light));C(light);light->constructed=light->ready=1;putf(light->scalar+0x14,1000);/* Explicitnumericpose/lightfixture, notoriginalbootglobals. */
 gh_player_shadow shadow={0};C(!gh_game_player_shadow_initialize_0014ef50(&shadow,&view));gh_actor_trans_bindings b={.work=&work,.camera=&camera,.world_light=light,.depth_shift=&shift,.elf=elf,.elf_bytes=es,.shadow=&shadow,.numeric_policy=GH_STATIC_BINARY32_REFERENCE};gh_actor_trans_call call={0};uint32_t pc;
 /* Native shadow pointers are real even though removedraw44C/450 arezero. */
 C(!w(bytes+0x44c)&&!w(bytes+0x450)&&shadow.initialized);C(gh_actor_model_trans_0014a3e8(&call,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x14efa8&&call.phase==3&&!call.complete);uint16_t depth;memcpy(&depth,bytes+0x2ac,2);C(depth==10);
 observe observer={0};b.callee=required;b.context=&observer;call=(gh_actor_trans_call){0};C(gh_actor_model_trans_0014a3e8(&call,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x14efa8&&observer.calls==1&&observer.request.shadow==&shadow&&observer.request.model==&model);
 /* OriginalWork5E0 shadowdisable gate is explicit here, solely toreachthe
  * nextactualbody; no actualMain flagvalue is inferred from thisfixture. */
 put((uint8_t*)&work+0x5e0,0x400000);uint16_t max=100;memcpy(bytes+0x2ae,&max,2);bytes[0x2b7]=128;put(bytes+0x254,w(bytes+0x254)&~0xc8000000u);observer=(observe){0};call=(gh_actor_trans_call){0};C(gh_actor_model_trans_0014a3e8(&call,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x14ac90&&call.phase==7&&bytes[0x2b7]==153&&!(w(bytes+0x250)&0x200000)&&observer.calls==1&&observer.request.depth==60);
 for(unsigned retry=0;retry<3;retry++)C(gh_actor_model_trans_0014a3e8(&call,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x14ac90&&bytes[0x2b7]==153&&observer.calls==retry+2);gh_actor_trans_bindings foreign=b;foreign.camera=NULL;C(gh_actor_model_trans_0014a3e8(&call,&model,&foreign,&pc)==GH_ACTOR_MODEL_BUSY);
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned mode=0;mode<4;mode++){
  C(!fesetround(FE_TONEAREST));bytes[0x2b7]=128;gh_actor_trans_call current={0};observer=(observe){0};C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));int flags=fetestexcept(FE_ALL_EXCEPT);C(gh_actor_model_trans_0014a3e8(&current,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x14ac90&&bytes[0x2b7]==153&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==flags);
  int visible=9;C(!gh_actor_model_box_visible_001f8ad8(&model,&camera,&visible,&pc)&&visible==visible_oracle(&model,&camera)&&visible&&fegetround()==modes[mode]);
 }C(!fesetround(FE_TONEAREST));
 /* True outsidefrustum skip completeswithoutshadow ormodeldraw invocation,
  * while originaldepthwrite precedes thepredicate. */
 putf(camera.fields+0x80+48,100);call=(gh_actor_trans_call){0};observer=(observe){0};C(!gh_actor_model_trans_0014a3e8(&call,&model,&b,&pc)&&call.complete&&!pc&&!observer.calls&&!visible_oracle(&model,&camera));int visible=1;C(!gh_actor_model_box_visible_001f8ad8(&model,&camera,&visible,&pc)&&!visible);putf(camera.fields+0x80+48,0);
 /* PlayerWork5E0 signbit earlyout is independent from inheritedactor gates. */
 put((uint8_t*)&work+0x5e0,0x80000000);call=(gh_actor_trans_call){0};C(!gh_actor_model_trans_0014a3e8(&call,&model,&b,&pc)&&call.complete);put((uint8_t*)&work+0x5e0,0);put(bytes+0x5a0,w(bytes+0x5a0)|1);call=(gh_actor_trans_call){0};C(!gh_actor_model_trans_0014a3e8(&call,&model,&b,&pc)&&call.complete);put(bytes+0x5a0,w(bytes+0x5a0)&~1u);
 /* Reachedpositive lighting is requiredbeforepreTrans, not silently omitted. */
 put(bytes+0x254,w(bytes+0x254)|0x8000000);call=(gh_actor_trans_call){0};observer=(observe){0};C(gh_actor_model_trans_0014a3e8(&call,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x14d828&&observer.calls==1);put(bytes+0x254,w(bytes+0x254)&~0x8000000u);
 /* Explicitactualchild/OM vtable profiles onthissame numericmodel fixture:
  * predicates differ, while14E4C8 is verifiedactualJRRA/NOP. */
 uint32_t originalvt=w(bytes+0x214);put(bytes+0x214,0x41b6a0);put((uint8_t*)&work+0x5e0,0x200000);call=(gh_actor_trans_call){0};C(!gh_actor_model_trans_0014a3e8(&call,&model,&b,&pc)&&call.complete);put((uint8_t*)&work+0x5e0,0);b.shadow=NULL;call=(gh_actor_trans_call){0};C(gh_actor_model_trans_0014a3e8(&call,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x14ac90);
 put(bytes+0x214,0x4292f0);put((uint8_t*)&work+0x5e0,0x20000000);call=(gh_actor_trans_call){0};C(!gh_actor_model_trans_0014a3e8(&call,&model,&b,&pc)&&call.complete);put(bytes+0x214,originalvt);
 C(!gh_actor_model_release(&model)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(light);free(data);free(elf);free(room);puts("PASS actualPL00 wholemodel+ELFVT, depth/predicate/frustum independentlongdouble/truegates/shadowowner/alpha153+retry/4FEnv; exact14EFA8/14D828/14AC90frontiers, no body/GPUack");return 0;
}
