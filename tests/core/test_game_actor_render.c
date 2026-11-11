#define main original_actor_nodes_test
#include "test_game_actor_model_nodes.c"
#undef main
#include "game_actor_render.h"
static void putf(void *p,float x){memcpy(p,&x,4);}
typedef struct {unsigned rays,attempts,groups;int accept;} observer;
static gh_scene_ray_status miss(void *context,gh_game_scene_tables *t,const gh_scene_ray_request *r,int *hit,uint32_t *f)
{observer *o=context;(void)t;C(r->mask==2&&r->flags==0&&r->segmented==1);o->rays++;*hit=0;*f=0;return GH_RAY_OK;}
/* Explicit test-only packet collector, never a production/GPU consumer. */
static gh_actor_model_status collect(void *context,const gh_shadow_group *g,uint32_t *f)
{observer *o=context;o->attempts++;C(g->ready&&g->bank==2&&g->primitive==0x5c);for(unsigned i=0;i<12;i++)C(!g->quads[i].enabled);if(!o->accept){*f=0x31a600;return GH_ACTOR_MODEL_UNSUPPORTED;}o->groups++;*f=0;return GH_ACTOR_MODEL_OK;}

int main(int argc,char **argv)
{
 C(argc==2);gh_disc *disc;C(!gh_disc_open(argv[1],&disc));size_t size;
 uint8_t *room=load(disc,"st1/r100.dat",&size);gh_bundle rb;C(!gh_bundle_open(room,size,&rb));
 size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);uint8_t *elf=malloc(elf_size);C(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_size));
 uint8_t *data=load(disc,"pl/pl00.dat",&size);gh_disc_close(disc);gh_bundle bundle;gh_bundle_entry md,texture;
 C(!gh_bundle_open(data,size,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));
 gh_heap *heap;gh_room_pools pools={0};C(!gh_heap_create(32*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&rb,heap));
 _Alignas(16)uint8_t bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view view;C(!gh_title_player_construct_00107e40(bytes,sizeof(bytes),&view));
 gh_boot_work work={0};gh_actor_model model={0};uint32_t fog=0;int original;
 C(!gh_actor_model_bind_packet_inputs(&model,heap,&fog)&&!gh_actor_model_initialize_00149738(&model,&view,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)&&original);
 gh_game_pl_camera camera={.constructed=1};for(unsigned i=0;i<4;i++){putf(camera.fields+i*20,1);putf(camera.fields+0x80+i*20,i==3?1:.1f);}
 gh_game_world_light *light=calloc(1,sizeof(*light));gh_game_scene_tables *actors=calloc(1,sizeof(*actors));C(light&&actors);light->constructed=light->ready=1;putf(light->scalar+0x14,1000);
 gh_player_shadow shadow={0};C(!gh_game_player_shadow_initialize_0014ef50(&shadow,&view));
 gh_shadow_templates templates={0};C(!gh_shadow_templates_from_elf(&templates,elf,elf_size));
 gh_camera_follow follow={.bound=1,.camera=&camera};follow.up_global[1]=1;
 gh_player_shared_vector pivot={.initialized=1};putf(pivot.bytes+12,1);uint8_t shift=0;
 observer o={0};gh_actor_render_bindings b={.work=&work,.camera=&camera,.light=light,.depth_shift=&shift,
  .elf=elf,.elf_bytes=elf_size,.shadow=&shadow,.actors=actors,.up_owner=&follow,.pivot_owner=&pivot,.templates=&templates,
  .emit_shadow=collect,.emit_context=&o,.ray=miss,.ray_context=&o,.policy=GH_STATIC_BINARY32_REFERENCE};
 gh_actor_render_call call={0};uint32_t pc=0;
 C(gh_actor_render_0014a3e8(&call,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x31a600&&o.rays==8&&o.attempts==1&&!o.groups);
 C(gh_actor_render_next_call(&call)==GH_ACTOR_MODEL_BUSY);
 C(gh_actor_render_0014a3e8(&call,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x31a600&&o.rays==8&&o.attempts==2&&!o.groups);
 gh_actor_render_call copied=call;C(gh_actor_render_0014a3e8(&copied,&model,&b,&pc)==GH_ACTOR_MODEL_BUSY);
 gh_actor_render_bindings foreign=b;foreign.depth_shift=NULL;C(gh_actor_render_0014a3e8(&call,&model,&foreign,&pc)==GH_ACTOR_MODEL_BUSY);
 gh_game_pl_camera cut_camera=camera;gh_actor_render_bindings cut=b;cut.camera=&cut_camera;
 C(gh_actor_render_0014a3e8(&call,&model,&cut,&pc)==GH_ACTOR_MODEL_BUSY&&call.bindings.camera==&camera);
 /* Preserve the strict numeric retry case using a genuinely unsupported
  * nonfinite input; the original VU supports an exact zero direction. */
 putf(light->scalar+0x161a0,NAN);
 o.accept=1;C(gh_actor_render_0014a3e8(&call,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x30a370&&o.rays==8&&o.groups==1&&call.shadow.complete&&call.trans.phase==7);
 uint8_t alpha=bytes[0x2b7];C(gh_actor_render_0014a3e8(&call,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x30a370&&o.groups==1&&o.rays==8&&bytes[0x2b7]==alpha);
 /* Fill the existing numerical light fixture, then resume without another
  * shadow insertion, frustum/palette replay or actor fade update. */
 for(unsigned i=0;i<3;i++)putf(light->scalar+0x161a0+i*16+i*4,-1);putf(light->scalar+0x16154,100);
 for(unsigned i=0;i<4;i++)for(unsigned k=0;k<4;k++)putf(light->scalar+(i<3?0x161d0+i*16:0x40)+k*4,(float)(i+1)*.2f);
 C(gh_actor_render_0014a3e8(&call,&model,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x14b4d0&&o.groups==1&&o.rays==8&&bytes[0x2b7]==alpha);
 /* An explicit original gate is a separate invocation fixture; a complete
  * gate resets the whole composition before the next invocation. */
 gh_actor_render_call gated={0};uint32_t gate=0x80000000u;memcpy((uint8_t*)&work+0x5e0,&gate,4);
 C(!gh_actor_render_0014a3e8(&gated,&model,&b,&pc)&&gated.trans.complete&&!pc);
 C(gh_actor_render_0014a3e8(&gated,&model,&cut,&pc)==GH_ACTOR_MODEL_BUSY&&gated.bindings.camera==&camera);
 C(!gh_actor_render_next_call(&gated)&&!gated.trans.active&&!gated.shadow.active&&!gated.draw.active);
 foreign.camera=&cut_camera;
 C(gh_actor_render_0014a3e8(&gated,&model,&foreign,&pc)==GH_ACTOR_MODEL_BUSY&&gated.bindings.camera==&camera);
 C(!gh_actor_render_0014a3e8(&gated,&model,&cut,&pc)&&gated.trans.complete&&!pc&&
   gated.bindings.camera==&cut_camera&&gated.trans.bindings.camera==&cut_camera);
 C(!gh_actor_model_release(&model)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));
 free(actors);free(light);free(data);free(elf);free(room);
 puts("PASS actualPL00 native trans/shadow/draw composition; rejected emit retry, one test-only OT group, no duplicate rays/fade, exact downstream frontiers/owner identity/new invocation; no GPU proof");return 0;
}
