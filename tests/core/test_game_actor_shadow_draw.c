#define main original_actor_nodes_test
#include "test_game_actor_model_nodes.c"
#undef main
#include "game_actor_shadow_draw.h"
static void putf(void*p,float x){memcpy(p,&x,4);}
static unsigned rays;
static gh_scene_ray_status miss(void*ctx,gh_game_scene_tables*t,const gh_scene_ray_request*r,int*hit,uint32_t*f){(void)ctx;(void)t;C(r->mask==2&&r->segmented==1);rays++;*hit=0;*f=0;return GH_RAY_OK;}
static gh_scene_ray_status plane(void*ctx,gh_game_scene_tables*t,const gh_scene_ray_request*r,int*hit,uint32_t*f){(void)ctx;(void)t;C(r->segmented==1);memcpy(r->hit_position,r->from,12);r->hit_position[1]=0;r->hit_normal[0]=r->hit_normal[2]=0;r->hit_normal[1]=1;*hit=1;*f=0;rays++;return GH_RAY_OK;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t size;uint8_t*room=load(d,"st1/r100.dat",&size);gh_bundle rb;C(!gh_bundle_open(room,size,&rb));size_t es=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(es);C(elf&&!gh_disc_read_boot_elf(d,0,elf,es));uint8_t*data=load(d,"pl/pl00.dat",&size);gh_disc_close(d);gh_bundle bundle;gh_bundle_entry md,texture;C(!gh_bundle_open(data,size,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(32*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&rb,heap));
 _Alignas(16)uint8_t bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view view;C(!gh_title_player_construct_00107e40(bytes,sizeof(bytes),&view));gh_boot_work work={0};gh_actor_model model={0};uint32_t fog=0;int original;C(!gh_actor_model_bind_packet_inputs(&model,heap,&fog)&&!gh_actor_model_initialize_00149738(&model,&view,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)&&original&&model.ready);
 gh_game_pl_camera camera={.constructed=1};for(unsigned k=0;k<4;k++){putf(camera.fields+k*20,1);putf(camera.fields+0x80+k*20,k==3?1:.1f);}gh_game_world_light*light=calloc(1,sizeof(*light));C(light);light->constructed=light->ready=1;putf(light->scalar+0x14,1000);/* Explicitnumericpose/lightfixture, notoriginalbootglobals. */
 gh_player_shadow shadow={0};C(!gh_game_player_shadow_initialize_0014ef50(&shadow,&view));
 gh_shadow_templates templates={0};C(!gh_shadow_templates_from_elf(&templates,elf,es));C(gh_shadow_templates_from_elf(&templates,elf,es)==GH_ACTOR_MODEL_BUSY);
 gh_camera_follow follow={0};follow.bound=1;follow.camera=&camera;follow.up_global[1]=1;
 gh_player_shared_vector pivot={.initialized=1};putf(pivot.bytes+12,1);
 gh_game_scene_tables *actors=calloc(1,sizeof(*actors));C(actors);
 putf(bytes+0x24c,1);putf(bytes+0x110,1);putf(bytes+0x114,0);putf(bytes+0x118,0);
 gh_shadow_bindings b={.model=&model,.shadow=&shadow,.work=&work,.camera=&camera,.world_light=light,.actors=actors,.up_owner=&follow,.pivot_owner=&pivot,.templates=&templates,.ray=miss,.policy=GH_STATIC_BINARY32_REFERENCE};
 gh_shadow_call call={0};uint32_t pc=0;C(gh_actor_shadow_draw_0014efa8(&call,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x31a600&&rays==8&&call.slot==12&&call.group.ready&&call.group.bank==2);
 for(unsigned k=0;k<12;k++)C(!call.group.quads[k].enabled);
 C(gh_actor_shadow_draw_0014efa8(&call,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&rays==8);
 gh_shadow_call copied=call;C(gh_actor_shadow_draw_0014efa8(&copied,&b,&pc)==GH_ACTOR_MODEL_BUSY);
 gh_shadow_quad q={.size=.25f,.quaternion={0,0,0,1},.alpha=128};
 for(unsigned k=0;k<4;k++){putf(camera.fields+k*20,1);putf(camera.fields+0xc0+k*20,1);}
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned k=0;k<4;k++){C(!fesetround(modes[k]));C(!gh_shadow_quad_project_0015ec60(&q,&camera,GH_STATIC_BINARY32_REFERENCE,&pc)&&q.enabled&&fegetround()==modes[k]);C(q.xy_fixed4[0][0]==4&&q.xy_fixed4[2][0]==-4&&q.z_integer[0]==0);}
 C(!fesetround(FE_TONEAREST));putf(light->scalar+0x16230,1);for(unsigned k=0;k<3;k++)putf(camera.fields+0xc0+k*20,.0001f);b.ray=plane;call=(gh_shadow_call){0};rays=0;C(gh_actor_shadow_draw_0014efa8(&call,&b,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x31a600&&rays==8&&call.slot==12);C(call.group.quads[0].enabled&&call.group.quads[0].alpha<=112);for(unsigned k=0;k<3;k++)putf(camera.fields+0xc0+k*20,1);
 C(!fesetround(FE_TONEAREST));q.position[0]=2;C(!gh_shadow_quad_project_0015ec60(&q,&camera,GH_STATIC_BINARY32_REFERENCE,&pc)&&!q.enabled);
 C(!gh_actor_model_release(&model)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(actors);free(light);free(data);free(elf);free(room);puts("PASS actual PL00/ELF shadow pattern+12 slots, segmented rays, one required OT group, retry identity, independent projection and FEnv; no GPU ack");return 0;
}
