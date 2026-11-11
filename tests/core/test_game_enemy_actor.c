#include "enemy_se_banks.h"
#include "game_enemy_route_graph.h"
#include "game_enemy_request.h"
#include "game_enemy_move.h"
#include <math.h>
#include "game_camera_envelope.h"
#include "game_world_light.h"
#include "game_ray_static.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
static uint64_t hash(const uint8_t*p,size_t n){uint64_t h=UINT64_C(14695981039346656037);for(size_t i=0;i<n;i++)h=(h^p[i])*UINT64_C(1099511628211);return h;}
static uint32_t word(const uint8_t *p)
{
 uint32_t result;memcpy(&result,p,4);return result;
}
static void put(uint8_t *p,uint32_t n){memcpy(p,&n,4);}
static float field(const uint8_t *p){float f;memcpy(&f,p,4);return f;}

/* Device-free fixture: execute the canonical resolver's original no-bank
 * branch. A real ready match would require the production attached provider. */
static gh_actor_motion_status lookup_missing_turn_sound(void *context,
    const gh_motion_sound_request *request,uint32_t *handle,uint32_t *frontier)
{
    int32_t dependency,bank;
    C(request->bank==1&&request->program==0x11&&request->actor->size>=0x568);
    memcpy(&dependency,request->actor->bytes+0x564,4);
    C(dependency==0x20a&&!gh_enemy_se_resolve_002cb418(context,dependency,&bank));
    C(bank==-1);
    *handle=0;*frontier=0;
    return GH_ACTOR_MOTION_OK;
}
static void verify_move(gh_enemy_actor *enemy,const void *elf,size_t elf_bytes,const gh_scene_nearcheck *navigation,gh_disc *disc)
{
 gh_boot_work *work=enemy->b.entry.work;
 gh_game_init_state *init=enemy->b.entry.init;
 gh_game_title_player player={0};
 int present;
 C(!gh_game_player_construct_00312f00(&player,enemy->b.entry.pools,work,&present)&&present);
 uint8_t *p=enemy->entry.object.view.bytes;
 const uint8_t *origin=enemy->entry.object.view.alias_f0;
 float actor_xyz[3];memcpy(actor_xyz,origin,12);
 float player_xyz[]={actor_xyz[0]+2,actor_xyz[1]+3,actor_xyz[2]};
 float prior[]={player_xyz[0]-1,player_xyz[1],player_xyz[2]};
 memcpy(player.view.alias_f0,player_xyz,12);
 memcpy(player.view.bytes+0x630,prior,12);
 put(player.view.bytes+0x104,0);
 const float world[16]={1,0,0,0,0,1,0,0,0,0,1,0,player_xyz[0],player_xyz[1],player_xyz[2],1};
 memcpy(player.view.bytes+0x80,world,sizeof(world));
 uint64_t before=hash(p,enemy->entry.object.view.size);

 gh_enemy202_move state={0};
 put((uint8_t *)work+0x5d8,0x40000000);
 C(!gh_enemy202_move_00208be0(&state,enemy,NULL,elf,elf_bytes)&&state.original_returned);
 C(hash(p,enemy->entry.object.view.size)==before);
 put((uint8_t *)work+0x5d8,0);
 state=(gh_enemy202_move){0};
 put(init->object_pool+0xacd0,2);
 C(!gh_enemy202_move_00208be0(&state,enemy,NULL,elf,elf_bytes)&&state.original_returned);
 C(hash(p,enemy->entry.object.view.size)==before&&word(init->object_pool+0xacd0)==2);
 put(init->object_pool+0xacd0,0);

 gh_hitarea_object *body=&enemy->hitareas.units[0];
 uint32_t body_flags=word(body->bytes+0x60);
 uint32_t cooldown=word(p+0x54c);
 put(body->bytes+0x60,body_flags|1);
 put(p+0x54c,0);
 state=(gh_enemy202_move){0};
 C(gh_enemy202_move_00208be0(&state,enemy,&player.view,elf,elf_bytes)==GH_ACTOR_MOTION_UNSUPPORTED);
 C(state.frontier==0x20a178&&!state.original_returned);
 put(body->bytes+0x60,body_flags);
 put(p+0x54c,cooldown);

 /* Attack-unit writes must be observed on the same real canonical object. */
 gh_hitarea_object *attack=&enemy->hitareas.units[1];
 memcpy(attack->bytes+0x46,&(uint16_t){0x2f},2);
 put(attack->bytes+0x4c,50);
 put(p+0x16dc,2);
 put(p+0x17d0,1);
 put((uint8_t*)work+0x210,1);
 uint64_t queries=enemy->b.actors->ray.native_queries;
 put(p+0x5a8,0x3f800000);
 float before_timer=field(p+0x16e4);
 gh_actor_motion_status status=gh_enemy202_move_00208be0(&state,enemy,&player.view,elf,elf_bytes);
 if(state.frontier!=0x2a7cd0)fprintf(stderr,"actual Move status%d frontier%08x stage%u\n",status,state.frontier,state.stage);
 C(status==GH_ACTOR_MOTION_UNSUPPORTED&&state.frontier==0x2a7cd0&&!state.original_returned);
 C(state.dispatch_pc==0x20f3c0&&word(p+0x16d8)==0);
 C(init->navigation_height_initialized&&init->navigation_height_bits==0x3f000000);
 C(state.visibility_done&&state.path_target_ready&&!state.visibility_hit);
 C(word(p+0x16d0)&0x200000u);
 C(fabsf(state.path_target[0]-(player_xyz[0]+1))<1e-5f);
 C(state.path_target[1]==player_xyz[1]&&fabsf(state.path_target[2]-(player_xyz[2]+1))<1e-5f);
 C(field(p+0x524)==player_xyz[1]+0.5f&&p[0x530]==1);
 C(!(word(p+0x16d4)&0x08000000u));
 C(enemy->b.actors->ray.native_queries==queries+3&&enemy->b.actors->ray.mask174==7&&enemy->b.actors->ray.flags178==0x60);
 queries=enemy->b.actors->ray.native_queries;
 C(attack->bytes[0x46]==0&&word(attack->bytes+0x4c)==0);
 C(word(p+0x16dc)==1&&fabsf(field(p+0x16e4)-(before_timer-1))<1e-5f);
 C(fabsf(field(p+0x764)+1.570796327f)<1e-6f);
 float expected=1.570796327f-field(p+0x104);
 if(expected>3.141592654f)expected-=6.283185307f;
 C(fabsf(field(p+0x75c)-expected)<1e-6f);
 C(field(p+0x760)==fabsf(field(p+0x75c))&&field(p+0x768)==fabsf(field(p+0x764)));
 C(fabsf(field(p+0x1690)-(player_xyz[0]+3))<1e-5f&&fabsf(field(p+0x618)-25)<1e-4f);
 before=hash(p,enemy->entry.object.view.size);
 C(gh_enemy202_move_00208be0(&state,enemy,&player.view,elf,elf_bytes)==GH_ACTOR_MOTION_UNSUPPORTED);
 C(state.frontier==0x2a7cd0&&hash(p,enemy->entry.object.view.size)==before&&fegetround()==FE_TOWARDZERO
     &&enemy->b.actors->ray.native_queries==queries);
 put(p+0x5b4,20);put(p+0x5b8,0x3f000000);put(p+0x24c,0x3e800000);
 put(p+0x16f8,2);
 C(!gh_enemy202_move_bind_navigation(&state,navigation));
 status=gh_enemy202_move_00208be0(&state,enemy,&player.view,elf,elf_bytes);
 C(status==GH_ACTOR_MOTION_UNSUPPORTED&&state.frontier==0x26ef60&&!state.original_returned);
 C(state.substate67_stage==8&&state.substate67_motion.complete&&state.substate67_motion.blend_frames==10);
 C(p[0x2f6]==1&&field(p+0x430)==1&&field(p+0x438)==9&&field(p+0x43c)==10);
 C((word(p+0x16d0)&0x408002u)==0x408000u&&field(p+0x24c)==1);
 C(word(p+0x5b4)==0&&field(p+0x5b8)==90);
 C(state.post_body_index==2&&word(p+0x16f8)==1&&(word(p+0x16d0)&0x1000u));
 C(navigation->vertex_count==88&&state.target_query.owner==navigation&&state.origin_query.owner==navigation);
 C(state.target_query.complete&&state.origin_query.complete&&state.target_vertex==48&&state.origin_vertex==72);
 C(enemy->b.actors->ray.native_queries==queries+11&&fegetround()==FE_TOWARDZERO);
 queries=enemy->b.actors->ray.native_queries;
 before=hash(p,enemy->entry.object.view.size);
 C(gh_enemy202_move_00208be0(&state,enemy,&player.view,elf,elf_bytes)==GH_ACTOR_MOTION_UNSUPPORTED);
 C(state.frontier==0x26ef60&&hash(p,enemy->entry.object.view.size)==before&&enemy->b.actors->ray.native_queries==queries);
 C(p[0x530]==0&&!(word(p+0x16d4)&0x08000000u)&&word(p+0x510)==0x41b9ff90&&word(p+0x4d0)==72&&word(p+0x4d4)==45&&word(p+0x4d8)==46);
 C(!memcmp(p+0x520,navigation->vertices.bytes+45*32,12)&&!memcmp(p+0x16a0,p+0x520,12));
 gh_game_pl_camera camera={0};
 gh_game_camera *saved_camera=init->active_camera;
 C(!gh_game_camera_construct_pl_00139bc8(&camera,&init->camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
 init->active_camera=&init->camera;
 float eye[4]={actor_xyz[0],actor_xyz[1]+4,actor_xyz[2],1};
 float target[4]={eye[0],eye[1],eye[2]+3,1};
 memcpy(camera.fields+0x210,eye,16);memcpy(camera.fields+0x200,target,16);
 gh_object_opacity_environment opacity={init,&camera,&player.view,1};
 C(!gh_enemy202_move_bind_opacity(&state,&opacity));
 put(p+0x16fc,0x3f000000);put(p+0x1708,0x3f000000);
 put(p+0x1758,0x3e800000);put(p+0x16f4,0x3e800000);
 put(p+0x480,0);put(p+0x484,0x3f800000);put(p+0x488,0);
 C(!fesetround(FE_UPWARD));
 init->enemies[0x518]=1;
 C(gh_enemy202_move_00208be0(&state,enemy,&player.view,elf,elf_bytes)==GH_ACTOR_MOTION_UNSUPPORTED);
 C(state.frontier==0x306528&&state.substate67_stage==19&&!(word(p+0x16d0)&0x100u));
 init->enemies[0x518]=0;
 status=gh_enemy202_move_00208be0(&state,enemy,&player.view,elf,elf_bytes);
 if(status||!state.original_returned)fprintf(stderr,"camera tail frontier%08x stage%u\n",state.frontier,state.substate67_stage);
 C(status==GH_ACTOR_MOTION_OK&&state.original_returned&&state.frontier==0&&state.substate67_stage==32);
 C(state.wall_call.complete&&!state.wall_call.hit&&state.floor_call.complete&&state.floor_hit);
 C(state.floor_report.attributes&&word(p+0x474)==1&&enemy->b.actors->ray.native_queries>queries);
 C((word(p+0x16d0)&0x4000000u)&&field(p+0x1710)==0);
 C(fabsf(field(enemy->entry.object.view.alias_f0+4)-0.364175f)<1e-6f);
 C(field(p+0x550)==0&&fabsf(field(p+0x554)-0.014175f)<1e-6f&&field(p+0x558)==0);
 C(!memcmp(p+0x490,enemy->entry.object.view.alias_f0,12));
 C(fabsf(field(p+0x16f4)-(0.25f-0.19634955f))<1e-6f);
 C(!(word(p+0x3ac)&0xffffu)&&state.post_node_index==8);
 for(unsigned i=0;i<6;i++){
  C(enemy->nodes.nodes[i].bytes);
  C((word(enemy->nodes.nodes[i].bytes+0x380)&1u)==(i%3!=0));
 }
 for(unsigned i=9;i<11;i++){
  C(enemy->nodes.nodes[i].bytes&&!(word(enemy->nodes.nodes[i].bytes+0x380)&1u));
 }
 C(fegetround()==FE_UPWARD);
 C(fabsf(field(p+0x1758)-0.2125f)<1e-6f&&field(p+0x484)==1);
 C((word(p+0x258)&0x80000000u)&&p[0x1868]&&field(p+0x16fc)==0.5f&&field(p+0x1708)==0.5f);
 const gh_actor_part *part;
 C(!gh_actor_model_part_at(&enemy->entry.initialize.model.model,1,&part));
 C(!(word(part->bytes+0x154)&0x800u));
 for(unsigned i=3;i<=4;i++){
  C(!gh_actor_model_part_at(&enemy->entry.initialize.model.model,i,&part));
  C((word(part->bytes+0x154)&0x800u)&&fabsf(field(part->bytes+0x150)-field(p+0x16f4)*.5f)<1e-6f);
 }

 queries=enemy->b.actors->ray.native_queries;
 before=hash(p,enemy->entry.object.view.size);
 C(!gh_enemy202_move_00208be0(&state,enemy,&player.view,elf,elf_bytes));
 C(state.original_returned&&hash(p,enemy->entry.object.view.size)==before&&enemy->b.actors->ray.native_queries==queries);
 C(!fesetround(FE_TOWARDZERO));
 uint32_t route[16];
 memset(route,0xa5,sizeof(route));
 float cost=-1;
 C(!gh_enemy_route_graph_0031be88(navigation,state.origin_vertex,state.target_vertex,16,route,16,&cost));
 uint32_t expected_route[]={72,45,46,84,47,48,0xffff};
 C(!memcmp(route,expected_route,sizeof(expected_route))&&route[7]==0xa5a5a5a5u&&fabsf(cost-23.249788f)<1e-5f);
 C(fegetround()==FE_TOWARDZERO);
 C(!gh_enemy_route_graph_0031be88(navigation,72,72,16,route,2,&cost)&&cost==0&&route[0]==72&&route[1]==0xffff);
 route[0]=0xdead;
 C(!gh_enemy_route_graph_0031be88(navigation,72,48,3,route,16,&cost)&&cost==0x1.fffffep127f&&route[0]==0xdead);
 uint8_t *edge_flags=navigation->owned_pairs+68*12+8;
 C(word(edge_flags)==0);
 put(edge_flags,0x20000000u);
 memset(route,0xff,sizeof(route));
 C(!gh_enemy_route_graph_0031be88(navigation,72,48,16,route,16,&cost));
 C(route[1]!=45);
 put(edge_flags,0x80000000u);
 memset(route,0xff,sizeof(route));
 C(!gh_enemy_route_graph_0031be88(navigation,72,48,16,route,16,&cost));
 C(route[1]!=45);
 put(edge_flags,0xc0000000u);
 C(!gh_enemy_route_graph_0031be88(navigation,72,48,16,route,16,&cost));
 C(!memcmp(route,expected_route,sizeof(expected_route)));
 put(edge_flags,0);
 /* Ordinary post-event Work permits the actual clip root deltas. The
  * player moves closer in the fixture; neither enemy phase nor pose is reset. */
 put((uint8_t*)work+0x5e4,0);
 float prior_x=field(origin),prior_z=field(origin+8),horizontal=0;
 put((uint8_t*)work+0x55c,0x3f800000u); /* Original turn-rate input, independent of motion rate. */
 float turn_start=0;
 for(unsigned frame=2;frame<=382;frame++){
  if(frame==135){
   const float close[3]={actor_xyz[0]+1,actor_xyz[1],actor_xyz[2]};
   memcpy(player.view.alias_f0,close,12);
   memcpy(player.view.bytes+0x630,close,12);
  }
  gh_enemy202_move next={0};
  C(!gh_enemy202_move_bind_navigation(&next,navigation));
  C(!gh_enemy202_move_bind_opacity(&next,&opacity));
  put((uint8_t*)work+0x210,frame);
  status=gh_enemy202_move_00208be0(&next,enemy,&player.view,elf,elf_bytes);
  if(frame==382){
   C(status==GH_ACTOR_MOTION_UNSUPPORTED&&next.frontier==0x2cb418&&!next.original_returned);
   C(next.substate67_stage==49&&p[0x2f5]==0&&p[0x2f6]==3);
   C(enemy->motion.motion.sequence_pending==3&&enemy->motion.motion.pending_sound.bank==1);
   printf("turn sequence frontier bank=%d program=%04x\n",enemy->motion.motion.pending_sound.bank,
          (uint16_t)enemy->motion.motion.pending_sound.program);
   C(fabsf(field(p+0x104)-turn_start)>0.1f);
   queries=enemy->b.actors->ray.native_queries;
   before=hash(p,enemy->entry.object.view.size);
   C(gh_enemy202_move_00208be0(&next,enemy,&player.view,elf,elf_bytes)==GH_ACTOR_MOTION_UNSUPPORTED);
   C(next.frontier==0x2cb418&&hash(p,enemy->entry.object.view.size)==before&&enemy->b.actors->ray.native_queries==queries);
   gh_audio_registry *registry;
   gh_sound_control control={0};
   gh_dvd_jobs dvd;
   gh_dvd_jobs_init(&dvd,disc);
   C(!gh_audio_registry_create(&registry)&&!gh_audio_startup_load(registry,disc));
   C(!gh_sound_control_from_startup(&control,disc,registry));
   gh_enemy_se_bindings banks={.control=&control,.registry=registry,.disc=disc,.dvd=&dvd,
       .default_heap=enemy->b.default_heap,.room_heap=enemy->b.entry.room_heap};
   gh_enemy_se_banks *owner;
   C(!gh_enemy_se_banks_from_startup(&banks,&owner));
   enemy->motion.motion.b.sound=lookup_missing_turn_sound;
   enemy->motion.motion.b.sound_context=owner;
   const uint32_t yaw=word(p+0x104);
   C(!gh_enemy202_move_00208be0(&next,enemy,&player.view,elf,elf_bytes));
   C(next.original_returned&&!next.frontier&&word(p+0x104)==yaw&&field(p+0x430)==13);
   enemy->motion.motion.b.sound=NULL;
   enemy->motion.motion.b.sound_context=NULL;
   /* The focused player constructor leaves HP zero. Set a living test
    * target and explicitly bind its known-empty auxiliary for this decision. */
   const uint16_t hp=1;
   memcpy(player.view.bytes+0x54a,&hp,2);
   gh_enemy202_move live={0};
   C(!gh_enemy202_move_bind_navigation(&live,navigation));
   C(!gh_enemy202_move_bind_opacity(&live,&opacity));
   C(!gh_enemy202_move_bind_player_aux_698(&live,NULL));
   gh_game_mt expected=init->random;
   uint32_t draw;
   C(!gh_game_mt_next_0030f348(&expected,&draw)&&(draw&1u));
   put((uint8_t*)work+0x210,383);
   C(!gh_enemy202_move_00208be0(&live,enemy,&player.view,elf,elf_bytes));
   C(live.original_returned&&!live.frontier&&!memcmp(&init->random,&expected,sizeof(expected)));
   C(p[0x2f4]==0&&p[0x2f5]==8&&p[0x2f6]==0&&p[0x2f7]==1);
   C((word(p+0x16d4)&0x08000001u)==0x08000001u);
   float starting_yaw=0;
   for(unsigned frame=384;frame<=388;frame++){
    gh_enemy202_move following={0};
    C(!gh_enemy202_move_bind_navigation(&following,navigation));
    C(!gh_enemy202_move_bind_opacity(&following,&opacity));
    C(!gh_enemy202_move_bind_player_aux_698(&following,NULL));
    put((uint8_t*)work+0x210,frame);
    gh_actor_motion_status step=gh_enemy202_move_00208be0(&following,enemy,&player.view,elf,elf_bytes);
    C(following.pending_ai_pc==0x214298&&p[0x2f5]==8&&p[0x2f6]==1&&p[0x2f7]==1);
    if(frame==388){
     C(step==GH_ACTOR_MOTION_UNSUPPORTED&&following.frontier==0x2cba48&&!following.original_returned);
     C(following.substate67_stage==55&&enemy->motion.motion.sequence_pending==4);
     const uint64_t unchanged=hash(p,enemy->entry.object.view.size);
     C(gh_enemy202_move_00208be0(&following,enemy,&player.view,elf,elf_bytes)==GH_ACTOR_MOTION_UNSUPPORTED);
     C(hash(p,enemy->entry.object.view.size)==unchanged);
     break;
    }
    C(!step&&following.original_returned&&!following.frontier);
    C(field(p+0x430)==2*(frame-383)&&field(p+0x600)>0.988f&&field(p+0x600)<0.990f);
    if(frame==384)starting_yaw=field(p+0x104);
    if(frame==387)C(starting_yaw-field(p+0x104)>0.16f);
   }
   C(!gh_enemy_se_banks_destroy(owner));
   gh_audio_registry_destroy(registry);
   break;
  }
  C(!status&&next.original_returned&&!next.frontier);
  if(frame==131)C(p[0x2f6]==2&&field(p+0x430)==1);
  if(frame==132)C(p[0x2f6]==3&&field(p+0x430)==1&&word(p+0x5f0)==1);
  if(frame==137)C(p[0x2f6]==3&&word(p+0x5f0)==0&&field(p+0x510)<8&&init->enemies[0x53a]==2);
  if(frame==280)C(p[0x2f6]==5&&field(p+0x430)==18);
  if(frame==368)C(p[0x2f4]==0&&p[0x2f5]==0&&p[0x2f6]==0&&p[0x2f7]==0);
  if(frame==369){C(p[0x2f5]==0&&p[0x2f6]==2&&field(p+0x430)==1);turn_start=field(p+0x104);}
  if(frame==370)C(p[0x2f5]==0&&p[0x2f6]==3&&field(p+0x430)==1);
  float x=field(origin),z=field(origin+8);
  horizontal+=fabsf(x-prior_x)+fabsf(z-prior_z);
  prior_x=x;prior_z=z;
 }
 C(horizontal>0.1f);
 init->active_camera=saved_camera;
 C(!gh_game_title_player_release(&player));
}
int main(int argc,char**argv){C(argc==2);gh_disc*disc;C(!gh_disc_open(argv[1],&disc));size_t roomBytes,emBytes,coreBytes;uint8_t*roomData=load(disc,"st1/r100.dat",&roomBytes),*emData=load(disc,"em/em02.dat",&emBytes),*coreData=load(disc,"etc/core_usa.bin",&coreBytes);size_t elfBytes=(size_t)gh_disc_boot_elf_size(disc);uint8_t*elf=malloc(elfBytes);C(elf&&!gh_disc_read_boot_elf(disc,0,elf,elfBytes));uint64_t roomHash=hash(roomData,roomBytes),emHash=hash(emData,emBytes),coreHash=hash(coreData,coreBytes);gh_bundle roomBundle,coreBundle;C(!gh_bundle_open(roomData,roomBytes,&roomBundle)&&!gh_bundle_open(coreData,coreBytes,&coreBundle));gh_bundle_entry idd,ems;C(!gh_id_find_language_data(&coreBundle,&idd)&&!gh_bundle_find(&roomBundle,"EMS",0,&ems));
 gh_heap*heap;C(!gh_heap_create(64*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&roomBundle,heap));gh_game_init_state*init=calloc(1,sizeof(*init));C(init);gh_game_init_state_initialize(init);uint32_t frontier;C(!gh_hitarea_objects_construct_001fde00(init,&frontier));gh_boot_work work={0};work.room=0x100;work.phase569=2;uint32_t seDisabled=0x20000000;memcpy((uint8_t*)&work+0x5e4,&seDisabled,4);uint8_t saveBytes[GH_CORE_SAVE_BYTES];gh_core_save_view save={saveBytes,sizeof(saveBytes)};C(!gh_core_save_system_init(save));uint8_t state=0;float rate=1;C(!gh_game_init_002a5b18(init,&work,&state,&rate,save));rate=1;
 gh_id_state*ids=calloc(1,sizeof(*ids));C(ids&&!gh_id_system_init_002acc70(ids,&idd));gh_id_runtime*runtime;C(!gh_id_runtime_create(ids,1,&runtime)&&!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));gh_game_cockpit*cockpit;C(!gh_game_cockpit_create(runtime,ids,&init->random,&cockpit)&&!gh_game_cockpit_initialize_00140c88(cockpit,save));
 gh_game_math_globals math={0};C(!gh_game_math_globals_from_elf(&math,elf,elfBytes)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));gh_motion_ik_globals ik={0};C(!gh_motion_ik_globals_from_elf(&ik,elf,elfBytes));gh_camera_envelope camera={0};C(!gh_game_camera_envelope_from_elf(&camera,elf,elfBytes,&frontier));gh_enemy_node_names names={0};C(!gh_enemy_node_names_from_elf(&names,elf,elfBytes));
 gh_game_world_light light={0};C(!gh_game_world_light_construct_002d6d80(&light)&&!gh_game_world_light_initialize_with_heap_002d6f40(&light,&roomBundle,heap));
 gh_game_scene_tables actors={0};C(!gh_scene_actors_0012e670(&actors,&roomBundle));gh_collision_pairs pairs={0};C(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));gh_game_collision_solids solids={0};gh_game_collision_solids_reset_00134690(&solids);
 gh_stage_slots globalSlots={0},roomSlots={0};gh_game_objects *global,*objects;C(!gh_game_objects_create(&globalSlots,0,&global)&&!gh_game_objects_create(&roomSlots,1,&objects)&&!gh_stage_slots_reset(&globalSlots,heap)&&!gh_stage_slots_reset(&roomSlots,heap));int added;C(!gh_game_objects_add_pinned_001fece0(objects,0x202,emData,emBytes,&added)&&added&&!gh_game_objects_set_file_extent(objects,0x202,emData,emBytes));
 gh_game_prepare prepare={0};uint8_t gameState=0;C(!gh_game_prepare_002a5d40(&prepare,save,0x100,&gameState));gh_room_state room;C(!gh_room_state_select_002bebc8(&prepare,0x100,0,&room));gh_em_scene_placements em={0};int loaded;C(!gh_em_scene_load_native_00294b98(&em,&room,&ems,&work,0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&loaded)&&loaded);
 gh_enemy_request request;int spawn;int16_t savedHP;C(!gh_enemy_request_from_room_00294ca0(&room,0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&request,&spawn,&savedHP,&frontier)&&spawn&&request.object_kind==0x202&&request.dependency_kind==0x20a&&request.appearance==3&&request.flags==0x4000000&&!savedHP);
 printf("actualR100first requestXYZ=%08x/%08x/%08x yaw=%08x\n",request.position_xyz[0],request.position_xyz[1],request.position_xyz[2],request.yaw_bits);
 uint8_t scenario=0,serial=255;gh_enemy_actor_bindings b={.entry={.init=init,.pools=&pools,.global_objects=global,.room_objects=objects,.work=&work,.room_heap=heap,.packet_heap=heap,.fog_rgb=light.scalar+4,.save=save,.scenario14=&scenario,.factory_serial=&serial,.policy=GH_STATIC_BINARY32_REFERENCE},.cockpit=cockpit,.ik=&ik,.actors=&actors,.atan_coeff=camera.atan_coeff,.canonical_rate=&rate,.solids=&solids,.default_heap=heap,.node_names=&names,.shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE},.numeric_policy=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE};
 gh_enemy_actor*enemy=calloc(1,sizeof(*enemy));C(enemy);gh_game_title_player*object=NULL;C(!fesetround(FE_TOWARDZERO));gh_actor_motion_status result=gh_enemy_actor_enter_002905d0(enemy,&b,&request,0,0,&object);printf("nativeEntry status%d frontier%08x stage%u original=%d\n",result,enemy->frontier,enemy->stage,object!=NULL);
 C(!result&&object==&enemy->entry.object&&enemy->stage==13&&enemy->final.complete&&fegetround()==FE_TOWARDZERO&&serial==0&&init->enemy_actors[0]==object&&enemy->entry.initialize.model.model.part_count==31&&enemy->entry.initialize.model.model.script.mesh_count==13);
 C(!gh_enemy_apply_saved_vitality_00294f34(object,savedHP));gh_game_mt after=init->random;C(!gh_enemy_actor_enter_002905d0(enemy,&b,&request,0,0,&object)&&!memcmp(&after,&init->random,sizeof(after))&&serial==0);gh_enemy_request other=request;other.entry_number++;C(gh_enemy_actor_enter_002905d0(enemy,&b,&other,0,0,&object)==GH_ACTOR_MOTION_BUSY&&!object);
 gh_bundle_entry pfd;C(!gh_bundle_find(&roomBundle,"PFD",0,&pfd));
 gh_scene_nearcheck navigation={0};
 C(!gh_scene_nearcheck_bind_0031c1d0(&navigation,&pfd));
 verify_move(enemy,elf,elfBytes,&navigation,disc);
 gh_scene_nearcheck_release(&navigation);
 C(!gh_enemy_actor_shutdown_native(enemy)&&!init->enemy_top&&!init->enemy_last&&gh_collision_solid_units_empty(&solids)&&!gh_collision_pair_shapes_have_live(&pairs));free(enemy);C(!fesetround(FE_TONEAREST));
 gh_scene_ray_triangle_state_release(&actors);C(!gh_game_object_pool_release_001fdfa8(init,&frontier)&&!gh_collision_pairs_release(&pairs));C(!gh_game_cockpit_destroy(cockpit));gh_id_runtime_destroy(runtime);free(ids);gh_game_objects_destroy(objects);gh_game_objects_destroy(global);C(!gh_game_world_light_release_presets(&light)&&!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));C(roomHash==hash(roomData,roomBytes)&&emHash==hash(emData,emBytes)&&coreHash==hash(coreData,coreBytes));free(init);free(elf);free(coreData);free(emData);free(roomData);gh_disc_close(disc);puts("PASS actualR100 firstEMS->sameRoomSave/request->202/20A fulloriginal2022D0 Initreturn1/nativeEntryactor; 31parts/13nodes/realIKrays/6HitArea/9shapes/sourceimmutable/idempotent/scopedcleanup; original Move gates/heading/HitAreas/state0 rank; LOS and target ray blocked R100 PFD route 72->45->46->84->47->48, waypoint45, 387 original Move returns with horizontal motion, 2705D8 choice, 211C08 turn, canonical 2CB418 no-bank branch, and one real MT word for the living-target 268068 choice, scaled 214298 yaw, next attached sound 2CBA48");return 0;}
