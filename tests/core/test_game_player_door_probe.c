#include "game_player_door_probe.h"
#include "game_actor_model.h"
#include "game_ray_static.h"
#include "game_action_button_set.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static float fl(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static void sf(uint8_t*p,float v){memcpy(p,&v,4);}
static void put(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static gh_player_door_status real_action_set(void*ctx,const gh_player_door_action*r,uint32_t*f){
 gh_action_set_request request={.priority=r->priority,.life=r->life,.unknown20=r->unknown20,.callback_id=r->callback_entry,.argument=(void*)r->argument,.kind=r->kind};int emitted;unsigned entry;gh_action_set_status e=gh_action_button_set_001f7858(ctx,&request,&emitted,&entry,f);return e?GH_PLAYER_DOOR_CALLEE:GH_PLAYER_DOOR_OK;
}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t i;gh_disc_entry e;C(!gh_disc_find(d,"st1/r100.dat",&i)&&!gh_disc_entry_at(d,i,&e));uint8_t*bytes=malloc((size_t)e.size);C(bytes&&!gh_disc_read(d,i,0,bytes,(size_t)e.size));gh_bundle room;C(!gh_bundle_open(bytes,(size_t)e.size,&room));gh_scene_at_manager at={0};gh_heap*heap;C(!gh_heap_create(4096,&heap)&&!gh_scene_at_initialize_units_002c0f90(&at,&room,heap,0x100,1));
 gh_at_native_unit u;C(!gh_scene_at_first_unit_002c30c0(&at,&u));unsigned candidates=0,positive=0;float positive_point[3]={0},positive_position[3]={0},positive_yaw=0;
 while(u.bytes){if(u.bytes[0x35]==1&&u.bytes[0x7a]==1){candidates++;const uint8_t*s=u.bytes+4;float p[3]={0,fl(s+4)+fl(s+8)*.5f,0};if(s[1]==1){for(unsigned j=0;j<4;j++){p[0]+=fl(s+0x10+j*8)*.25f;p[2]+=fl(s+0x14+j*8)*.25f;}}else{p[0]=fl(s+0x10);p[2]=fl(s+0x14);}float out[3]={77,88,99},yaw=123;int found;uint32_t pc;gh_scene_at_status status=gh_scene_at_find_jump_002c2b58(&at,&room,0x100,p,out,&yaw,&found,&pc);fprintf(stderr,"candidate handle%u index%u type%u pc%08X status%d found%d point%g,%g,%g\n",u.handle,u.bytes[0x79],s[1],pc,status,found,p[0],p[1],p[2]);C(!status);if(found){positive++;memcpy(positive_point,p,12);memcpy(positive_position,out,12);positive_yaw=yaw;}}
 C(!gh_scene_at_next_unit_002c30c8(&at,u,&u));}
 fprintf(stderr,"candidates%u positive%u\n",candidates,positive);
 C(candidates==1&&positive==1&&w((const uint8_t*)positive_position)==0x41931eb8&&w((const uint8_t*)positive_position+4)==0x409d70a4&&w((const uint8_t*)positive_position+8)==0xc1a0f5c2&&w((const uint8_t*)&positive_yaw)==0x403ba866);
 int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned m=0;m<4;m++){C(!fesetround(modes[m]));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_INVALID);int flags=fetestexcept(FE_ALL_EXCEPT);float o[3]={7,8,9},y=10;int yes;uint32_t f;C(!gh_scene_at_find_jump_002c2b58(&at,&room,0x100,positive_point,o,&y,&yes,&f)&&yes&&!memcmp(o,positive_position,12)&&!memcmp(&y,&positive_yaw,4)&&fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==flags);}
 C(!fesetround(FE_TONEAREST));
 float rejected[3]={7,8,9},rejected_yaw=10;int yes;uint32_t failure;C(!gh_scene_at_find_jump_002c2b58(&at,&room,0x101,positive_point,rejected,&rejected_yaw,&yes,&failure)&&!yes&&rejected[0]==7&&rejected[1]==8&&rejected[2]==9&&rejected_yaw==10);
 gh_at_native_unit candidate={0};C(!gh_scene_at_first_unit_002c30c0(&at,&u));while(u.bytes){if(u.bytes[0x35]==1&&u.bytes[0x7a]==1){candidate=u;break;}C(!gh_scene_at_next_unit_002c30c8(&at,u,&u));}C(candidate.bytes);put(candidate.bytes+0x48,1);C(gh_scene_at_find_jump_002c2b58(&at,&room,0x100,positive_point,rejected,&rejected_yaw,&yes,&failure)==GH_AT_UNSUPPORTED&&failure==0x2c1d68&&rejected[0]==7);put(candidate.bytes+0x48,0);
 gh_bundle_entry jump;C(!gh_bundle_find(&room,"JMP",0,&jump));uint32_t original_count=w(jump.bytes);put((uint8_t*)jump.bytes,0xffffffff);C(gh_scene_at_find_jump_002c2b58(&at,&room,0x100,positive_point,rejected,&rejected_yaw,&yes,&failure)==GH_AT_BOUNDS&&failure==0x2c2b58&&rejected[0]==7);put((uint8_t*)jump.bytes,original_count);


 gh_at_link original_head=at.heads[15];at.heads[15]=(gh_at_link){GH_AT_LINK_UNIT,0x7fffffff};C(gh_scene_at_find_jump_002c2b58(&at,&room,0x100,positive_point,rejected,&rejected_yaw,&yes,&failure)==GH_AT_BOUNDS&&failure==0x2c30c8&&rejected[0]==7);at.heads[15]=original_head;
 float miss[3]={1000000,1000000,1000000},out[3]={77,88,99},yaw=123;int found;uint32_t pc;C(!gh_scene_at_find_jump_002c2b58(&at,&room,0x100,miss,out,&yaw,&found,&pc)&&!found&&out[0]==77&&out[1]==88&&out[2]==99&&yaw==123);
 uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view player;C(!gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&player));player_bytes[0x54a]=100;for(unsigned k=0;k<3;k++)sf(player.alias_f0+4*k,miss[k]);gh_game_init_state*init=malloc(sizeof(*init));C(init);gh_game_init_state_initialize(init);gh_boot_work work={.room=0x100};gh_game_scene_tables tables={0};C(!gh_scene_actors_0012e670(&tables,&room));gh_player_door_bindings b={.player=&player,.current_player=&player,.init=init,.scene_at=&at,.room=&room,.work=&work,.actors=&tables,.numeric_policy=GH_CAMERA_NATIVE_BINARY32_REFERENCE};gh_player_door_probe probe={0};C(!gh_player_door_probe_begin(&probe));C(!gh_player_door_probe_002dc920_002dcad8(&probe,&b)&&probe.complete&&tables.ray.native_queries>0);

 gh_player_door_probe positive_probe={0};for(unsigned k=0;k<3;k++)sf(player.alias_f0+4*k,positive_point[k]);sf(player.bytes+0x104,positive_yaw);C(!gh_player_door_probe_begin(&positive_probe));uint64_t before_ray=tables.ray.native_queries;C(gh_player_door_probe_002dc920_002dcad8(&positive_probe,&b)==GH_PLAYER_DOOR_UNSUPPORTED&&positive_probe.frontier==0x1f7858&&positive_probe.stage==1&&!positive_probe.complete&&tables.ray.native_queries==before_ray&&!memcmp(player.bytes+0x660,positive_position,12)&&!memcmp(player.bytes+0x670,&positive_yaw,4)&&positive_probe.pending.priority==4&&positive_probe.pending.life==8&&positive_probe.pending.callback_entry==0x2dca58);
 for(unsigned k=0;k<3;k++)C(gh_player_door_probe_002dc920_002dcad8(&positive_probe,&b)==GH_PLAYER_DOOR_UNSUPPORTED&&positive_probe.frontier==0x1f7858&&tables.ray.native_queries==before_ray);gh_player_door_probe copied=positive_probe;C(gh_player_door_probe_002dc920_002dcad8(&copied,&b)==GH_PLAYER_DOOR_ARGUMENT);

 /* Original R100 collision surface supplies a near-wall positive fixture;
  * pose is explicitly derived for this test, not an observed game camera. */
 unsigned wall_checked=0;for(unsigned polygon=0;polygon<h(tables.akg.bytes+0x28)&&!wall_checked;polygon++){
  const uint8_t*t=tables.actor_links[4].view.bytes+(size_t)polygon*20;if(w(t+16)&2)continue;unsigned attr=h(t+12);const uint8_t*a=tables.actor_links[1].view.bytes+(size_t)attr*16;if(!(w(a+4)&0x20))continue;
  const uint8_t*n=tables.actor_links[3].view.bytes+(size_t)h(t+8)*12;float nx=fl(n),ny=fl(n+4),nz=fl(n+8);if(fabsf(ny)>0.00001f||fabsf(nx)+fabsf(nz)<.9f)continue;
  unsigned count=(w(t+16)&1)?4:3;float center[3]={0};for(unsigned v=0;v<count;v++){const uint8_t*q=tables.actor_links[2].view.bytes+(size_t)h(t+2*v)*12;for(unsigned k=0;k<3;k++)center[k]+=fl(q+4*k)/(float)count;}
  memset(player.bytes+0x80,0,64);sf(player.bytes+0x80,-nz);sf(player.bytes+0x88,nx);sf(player.bytes+0x94,1);sf(player.bytes+0xa0,-nx);sf(player.bytes+0xa8,-nz);sf(player.bytes+0xb0,center[0]);sf(player.bytes+0xb4,center[1]-.5f);sf(player.bytes+0xb8,center[2]);sf(player.bytes+0xbc,1);sf(player.bytes+0x104,0);
  gh_player_door_probe wall={0};C(!gh_player_door_probe_begin(&wall));gh_player_door_status ds=gh_player_door_probe_002dc920_002dcad8(&wall,&b);
  if(ds==GH_PLAYER_DOOR_UNSUPPORTED&&wall.frontier==0x1f7858&&wall.stage==3){
   C(wall.pending.priority==4&&wall.pending.life==9&&wall.pending.callback_entry==0x2dccd0&&fabsf(fl(player.bytes+0x664)-(center[1]-.5f))<.0001f);
   C(fabsf(fl(player.bytes+0x660)-(center[0]+nx*.5f))<.0005f&&fabsf(fl(player.bytes+0x668)-(center[2]+nz*.5f))<.0005f);
   uint64_t queries=tables.ray.native_queries;uint8_t published[12];memcpy(published,player.bytes+0x660,12);for(unsigned k=0;k<3;k++)C(gh_player_door_probe_002dc920_002dcad8(&wall,&b)==GH_PLAYER_DOOR_UNSUPPORTED&&wall.frontier==0x1f7858&&tables.ray.native_queries==queries&&!memcmp(published,player.bytes+0x660,12));wall_checked=1;fprintf(stderr,"actualwall polygon%u nx%g nz%g yaw%g\n",polygon,nx,nz,wall.yaw);
  }
 }
 C(wall_checked==1);

 /* Same pending original action now uses the REAL canonical publisher;
  * no recorder/empty success callback stands in forActionButton_set. */
 gh_game_action_buttons actions={0};gh_game_action_buttons_reset_001f72c0(&actions);b.publish=real_action_set;b.context=&actions;
 for(unsigned k=0;k<3;k++)sf(player.alias_f0+4*k,miss[k]);
 C(!gh_player_door_probe_002dc920_002dcad8(&positive_probe,&b)&&positive_probe.complete&&actions.entries[0].callback_id10==0x2dca58&&actions.entries[0].priority==4&&actions.entries[0].life==8&&actions.heads[4].kind==GH_ACTION_LINK_PROMPT);
 (void)positive_point;(void)positive_position;(void)positive_yaw;put(player_bytes+0x15f4,0x400000);C(w(player_bytes+0x15f4)==0x400000);C(!gh_player_door_probe_begin(&probe));b.actors=NULL;C(!gh_player_door_probe_002dc920_002dcad8(&probe,&b)&&probe.complete);
 gh_scene_ray_triangle_state_release(&tables);C(!gh_scene_at_units_release(&at)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(init);free(bytes);gh_disc_close(d);puts("PASS actualR100 Doorjump readonly candidates and genuine near-wall ray miss/gates");return 0;}
