#include "game_player_targets.h"
#include "game_ray_static.h"
#include "game_player_attack_recent.h"
#include "game_hitarea_internal.h"
#include "game_enemy_manager.h"
#include "game_enemy_factory.h"
#include "game_enemy_entry.h"
#include "game_om_base.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static void sh(void*p,uint16_t n){memcpy(p,&n,2);}
static float value(const void*p){float n;memcpy(&n,p,4);return n;}
typedef struct {uint8_t bytes[0x900];gh_title_player_view view;uint32_t handle;} item;
typedef struct {item *items;unsigned count;const gh_game_title_player *enemies;const uint32_t *enemy_handles;unsigned enemy_count;} resolver;
static gh_player_target_status resolve(void*opaque,uint32_t handle,gh_title_player_view*out,uint32_t*f){resolver*r=opaque;for(unsigned i=0;i<r->count;i++)if(r->items[i].handle==handle){*out=r->items[i].view;return GH_PLAYER_TARGET_OK;}for(unsigned i=0;i<r->enemy_count;i++)if(r->enemy_handles[i]==handle){*out=r->enemies[i].view;return GH_PLAYER_TARGET_OK;}if(f)*f=0x12a610;return GH_PLAYER_TARGET_UNSUPPORTED;}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t index;gh_disc_entry e;C(!gh_disc_find(d,path,&index)&&!gh_disc_entry_at(d,index,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,index,0,p,*n));return p;}
static void test_enemy_focus(const gh_player_target_bindings *b,item *om)
{
 gh_game_init_state *init=calloc(1,sizeof(*init));
 C(init);
 gh_game_init_state_initialize(init);
 for(unsigned i=0;i<64;i++)put(init->enemies+i*20+12,UINT32_MAX);
 gh_game_title_player enemies[2]={0};
 uint8_t enemy_bytes[2][GH_EM202_BYTES]={{0}};
 uint32_t handles[2],pc;
 const unsigned slots[]={11,3};
 for(unsigned i=0;i<2;i++){
  C(!gh_enemy202_construct_00201fa0(enemy_bytes[i],sizeof(enemy_bytes[i]),&enemies[i].view));
  sh(enemy_bytes[i]+0x2fe,0x202);put(enemy_bytes[i]+0x564,0x202);
  sh(enemy_bytes[i]+0x54a,74);
  C(!gh_room_object_register(b->pools,&enemies[i],init,GH_ROOM_OBJECT_ENEMY,9,handles+i));
  void *registered=NULL;
  C(!gh_room_object_lookup(b->pools,handles[i],GH_ROOM_OBJECT_ENEMY,init,&registered)&&registered==&enemies[i]);
  C(!gh_enemy_manager_insert_00290048(init,slots[i],&enemies[i],0x202,&pc));
 }
 resolver *r=b->context;
 r->enemies=enemies;r->enemy_handles=handles;r->enemy_count=2;
 C(init->enemy_top==init->enemies+slots[0]*20&&init->enemy_next[slots[0]]==slots[1]);
 C(init->enemy_last==init->enemies+slots[1]*20);
 const float origin[3]={1000000,0,1000000};
 memcpy(b->player->alias_f0,origin,12);
 const float straight[3]={1000000,0,1000002};
 const float off_axis[3]={1000001,0,1000001};
 memcpy(enemies[0].view.alias_f0,straight,12);
 memcpy(enemies[1].view.alias_f0,off_axis,12);
 gh_player_target target={0};
 const float cone=1.57079637f;
 uint64_t queries=b->actors->ray.native_queries;
 gh_player_target_status first=gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc);
 C(!first&&!pc&&
   target.handle==handles[0]&&target.view.bytes==enemy_bytes[0]);
 C(b->actors->ray.native_queries==queries+1); /* Earlier tighter sector rejects later target. */
 C(!memcmp(enemy_bytes[0]+0x1680,straight,12)&&!memcmp(enemy_bytes[1]+0x1680,off_axis,12));
 memcpy(enemies[1].view.alias_f0,straight,12);
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&target.handle==handles[1]);

 memcpy(enemies[1].view.alias_f0+4,&(float){1.501f},4);
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&target.handle==handles[0]);
 memcpy(enemies[1].view.alias_f0+4,&(float){1.5f},4);
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&target.handle==handles[1]);
 memcpy(enemies[1].view.alias_f0+4,&(float){1.501f},4);
 enemy_bytes[1][0x2f4]=1;
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&target.handle==handles[1]);
 enemy_bytes[1][0x2f4]=2;enemy_bytes[1][0x2f5]=2;
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&target.handle==handles[0]);
 enemy_bytes[1][0x617]=1;
 sh(enemy_bytes[0]+0x54a,0);
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&!target.handle);
 sh(enemy_bytes[0]+0x3ac,4);
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&target.handle==handles[0]);

 const float extended_position[3]={1000000,0,1000006};
 memcpy(enemies[0].view.alias_f0,extended_position,12);
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&!target.handle);
 put(enemy_bytes[0]+0x564,0x260);
 queries=b->actors->ray.native_queries;
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&
   target.handle==handles[0]&&b->actors->ray.native_queries==queries);
 put(enemy_bytes[0]+0x564,0x202);
 memcpy(enemies[0].view.alias_f0,straight,12);
 C(!gh_om_base_construct_001b6e90(om->bytes,sizeof(om->bytes),&om->view));
 sh(om->bytes+0x2fe,0x301);put(om->bytes+0x5b8,0x1000);
 memcpy(om->view.alias_f0,straight,12);
 enemy_bytes[0][0x617]=1;
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&target.handle==om->handle&&target.view.bytes==om->bytes);
 put(om->bytes+0x5b0,4);
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&!target.handle);
 put(om->bytes+0x5b0,0);put(om->bytes+0x5b8,0);
 /* First attack's fallback admits a hittable OM without 291010's 1000
  * flag, then 124C30 retains it without re-running visibility probes. */
 C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&!target.handle);
 C(!gh_player_attack_target_00124540(b,init,&target,&pc)&&!pc&&target.handle==om->handle&&
   w(b->player->bytes+0x640)==om->handle&&b->player->bytes[0x648]==20);
 queries=b->actors->ray.native_queries;
 C(!gh_player_attack_target_00124540(b,init,&target,&pc)&&target.handle==om->handle&&
   b->actors->ray.native_queries==queries);
 memcpy(enemies[1].view.alias_f0+4,&(float){0},4);
 put(om->bytes+0x250,0x8000);
 C(!gh_player_attack_target_00124540(b,init,&target,&pc)&&target.handle==handles[1]);
 put(om->bytes+0x250,0);
 b->player->bytes[0x648]=0;
 C(!gh_player_attack_target_00124540(b,init,&target,&pc)&&target.handle==om->handle);
 put(b->player->bytes+0x5d8,4);
 b->player->bytes[0x648]=0;
 queries=b->actors->ray.native_queries;
 C(!gh_player_attack_target_00124540(b,init,&target,&pc)&&target.handle==om->handle&&
   b->actors->ray.native_queries==queries+3);
 put(b->player->bytes+0x5d8,0);
 b->player->bytes[0x648]=0;
 put(b->player->bytes+0x640,0);

 gh_hitarea_object give[5];
 int result;
 C(!gh_hitarea_objects_construct_001fde00(init,&pc));
 gh_title_player_view owner=*b->player;
 for(unsigned i=0;i<5;i++)C(!gh_hitarea_object_create_001fe0e8(init,&owner,0,give+i,&result,&pc)&&result);
 C(!gh_player_attack_recent_0010b2e8(&owner,give,&result,&pc)&&!result&&!pc);
 init->hitarea_native->last_hit_actors[give[3].index]=&enemies[0].view;
 C(!gh_player_attack_recent_0010b2e8(&owner,give,&result,&pc)&&result&&!pc);
 put(enemy_bytes[1]+0x564,0x2a2);
 init->hitarea_native->last_hit_actors[give[0].index]=&enemies[1].view;
 C(!gh_player_attack_recent_0010b2e8(&owner,give,&result,&pc)&&!result&&!pc);
 init->hitarea_native->last_hit_actors[give[0].index]=NULL;
 for(unsigned i=0;i<5;i++)C(!gh_hitarea_object_release_empty_001fe278(give+i,&pc));
 C(gh_player_attack_recent_0010b2e8(&owner,give,&result,&pc)==GH_HITAREA_ARGUMENT&&pc==0x10b2e8);
 gh_hitarea_detach_after_pool_reset(init);
 enemy_bytes[0][0x617]=0;

 /* Pick a real R100 floor polygon by independently hitting it with the
  * original mode-0 ray. Offset actor positions by the original +1 Y bias. */
 const gh_game_scene_tables *a=b->actors;
 int blocked=0;
 for(unsigned i=0;i<half(a->akg.bytes+0x28)&&!blocked;i++){
  const uint8_t *triangle=a->actor_links[4].view.bytes+i*20;
  const unsigned count=(w(triangle+16)&1)?4:3;
  if(w(triangle+16)&2)continue;
  const uint8_t *normal=a->actor_links[3].view.bytes+half(triangle+8)*12;
  if(value(normal+4)<.99f)continue;
  float center[3]={0};
  for(unsigned j=0;j<count;j++){
   const uint8_t *vertex=a->actor_links[2].view.bytes+half(triangle+j*2)*12;
   for(unsigned k=0;k<3;k++)center[k]+=value(vertex+k*4)/(float)count;
  }
  gh_scene_ray_request ray={.mask=7,.flags=0x13c,.segmented=1};
  memcpy(ray.from,center,12);memcpy(ray.to,center,12);
  ray.from[1]+=.125f;ray.to[1]-=.125f;
  ray.from[3]=ray.to[3]=1;
  int hit;
  C(!gh_scene_ray_query_00131e10(b->actors,&ray,&hit,&pc));
  if(!hit)continue;
  float player_position[3]={center[0],center[1]-.875f,center[2]};
  float enemy_position[3]={center[0],center[1]-1.125f,center[2]};
  memcpy(b->player->alias_f0,player_position,12);
  memcpy(enemies[0].view.alias_f0,enemy_position,12);
  queries=b->actors->ray.native_queries;
  C(!gh_player_enemy_target_00291010(b,init,2.5f,cone,&target,&pc)&&!target.handle&&
    b->actors->ray.native_queries==queries+1);
  blocked=1;
 }
 C(blocked);
 for(unsigned i=0;i<2;i++)C(!gh_room_object_unregister(b->pools,handles[i],init));
 r->enemy_count=0;r->enemies=NULL;r->enemy_handles=NULL;
 free(init);
 puts("PASS attack target retention and original fallback, canonical five-slot recent-victim order");
 puts("PASS mode0 actual R100 registry, linked order and sector narrowing, exact height, visible OM fallback, real floor LOS");
}
int main(int argc,char**argv){C(argc==3);FILE*f=fopen(argv[2],"rb");C(f&&!fseek(f,0,SEEK_END));long n=ftell(f);C(n>52&&!fseek(f,0,SEEK_SET));uint8_t*elf=malloc((size_t)n);C(elf&&fread(elf,1,(size_t)n,f)==(size_t)n);fclose(f);float from[3]={0},to[3];uint32_t pc;double maximum=0;for(int x=-32;x<=32;x++)for(int z=-32;z<=32;z++){to[0]=(float)x;to[1]=0;to[2]=(float)z;for(int h=-1;h<=1;h++){float heading=(float)h*.7f,result;C(!gh_player_target_angle_002da038(elf,(size_t)n,from,to,heading,3.14159274f,&result,&pc));long double expected=x||z?atan2l(x,z)-(long double)heading:0;if(expected>3.141592653589793238L)expected-=6.283185307179586476L;if(expected< -3.141592653589793238L)expected+=6.283185307179586476L;double error=(double)fabsl((long double)result-expected);if(error>maximum)maximum=error;if(error>0.000001){fprintf(stderr,"math%d,%d,%d=%g expected%g\n",x,z,h,result,(double)expected);C(0);}}}
 int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned m=0;m<4;m++){C(!fesetround(modes[m]));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_DIVBYZERO);int flags=fetestexcept(FE_ALL_EXCEPT);float out;to[0]=.5f;to[2]=-1;C(!gh_player_target_angle_002da038(elf,(size_t)n,from,to,.3f,1,&out,&pc)&&fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==flags);to[0]=NAN;out=17;C(gh_player_target_angle_002da038(elf,(size_t)n,from,to,0,1,&out,&pc)==GH_PLAYER_TARGET_NUMERIC&&out==17&&fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==flags);}C(!fesetround(FE_TONEAREST));
 gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t bytes;uint8_t*data=load(d,"st1/r100.dat",&bytes);gh_disc_close(d);gh_bundle room;C(!gh_bundle_open(data,bytes,&room));gh_heap*heap;C(!gh_heap_create(16*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));gh_game_scene_tables actors={0};C(!gh_scene_actors_0012e670(&actors,&room));uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view player;C(!gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&player));float player_position[3]={1000000,0,1000000};memcpy(player.alias_f0,player_position,12);memcpy(player.bytes+0x490,player_position,12);item items[3]={0};resolver resolver_state={.items=items,.count=3};for(unsigned i=0;i<3;i++){C(!gh_game_object_construct_002a7338(items[i].bytes,sizeof(items[i].bytes),&items[i].view));C(!gh_room_object_register(&pools,&items[i],&resolver_state,3,8,&items[i].handle));memcpy(items[i].view.alias_f0,player_position,12);}sh(items[0].bytes+0x2fe,0x101);sh(items[1].bytes+0x2fe,0x361);sh(items[2].bytes+0x2fe,0x300);items[1].bytes[0x664]=1;gh_player_target_bindings b={&pools,&actors,&player,resolve,&resolver_state,elf,(size_t)n};gh_player_target target={0};C(!gh_player_small_object_gate_00129cc8(&b,&pc)&&!pc);C(!gh_player_pickup_target_0012a610(&b,&target,&pc)&&target.handle==items[1].handle&&target.view.bytes==items[1].bytes&&actors.ray.native_queries);items[1].bytes[0x664]=0;C(!gh_player_pickup_target_0012a610(&b,&target,&pc)&&!target.handle);items[1].bytes[0x664]=1;
 put(items[1].bytes+0x5b0,8);put(items[2].bytes+0x5b0,8);float p1[3]={1000000,0,1000001.5f},p2[3]={1000000,0,1000001};memcpy(items[1].view.alias_f0,p1,12);memcpy(items[2].view.alias_f0,p2,12);C(!gh_player_interact_target_0012b328(&b,&target,&pc)&&target.handle==items[2].handle);memcpy(items[1].view.alias_f0,p2,12);C(!gh_player_interact_target_0012b328(&b,&target,&pc)&&target.handle==items[2].handle);put(items[2].bytes+0x250,0x8000);C(!gh_player_interact_target_0012b328(&b,&target,&pc)&&target.handle==items[1].handle);put(items[2].bytes+0x250,0);
 /* RealR100 floor positive LOS rejects a valid pickup. Shift originals by.5
  * because original pickup ray adds.5 to both source/targetY. */
 int blocked=0;for(unsigned i=0;i<half(actors.akg.bytes+0x28)&&!blocked;i++){const uint8_t*t=actors.actor_links[4].view.bytes+i*20;unsigned count=(w(t+16)&1)?4:3;if(w(t+16)&2)continue;const uint8_t*normal=actors.actor_links[3].view.bytes+half(t+8)*12;if(value(normal+4)<.99f)continue;float center[3]={0};for(unsigned j=0;j<count;j++){const uint8_t*v=actors.actor_links[2].view.bytes+half(t+j*2)*12;for(unsigned k=0;k<3;k++)center[k]+=value(v+k*4)/(float)count;}gh_scene_ray_request ray={.mask=7,.flags=0x18};memcpy(ray.from,center,12);memcpy(ray.to,center,12);ray.from[1]+=.125f;ray.to[1]-=.125f;int hit;C(!gh_scene_ray_query_00131e10(&actors,&ray,&hit,&pc));if(!hit)continue;float pos[3]={center[0],center[1]-.375f,center[2]},candidate[3]={center[0],center[1]-.625f,center[2]};memcpy(player.alias_f0,pos,12);memcpy(items[1].view.alias_f0,candidate,12);C(!gh_player_pickup_target_0012a610(&b,&target,&pc)&&!target.handle);blocked=1;}C(blocked);
 sh(items[1].bytes+0x2fe,0x364);put(items[1].bytes+0x600,1);C(!gh_player_small_object_gate_00129cc8(&b,&pc));put(items[1].bytes+0x600,0);C(gh_player_small_object_gate_00129cc8(&b,&pc)==GH_PLAYER_TARGET_UNSUPPORTED&&pc==0x147568);test_enemy_focus(&b,&items[2]);for(unsigned i=0;i<3;i++)C(!gh_room_object_unregister(&pools,items[i].handle,&resolver_state));gh_scene_ray_triangle_state_release(&actors);C(!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(data);free(elf);printf("PASS realR100 registry/LOS positiveblock,typedIDfilter/pickup/interact/tielater/flags/strict364; SDKatan12675 longdouble oracle max%.9g,4FEnv\n",maximum);return 0;}
