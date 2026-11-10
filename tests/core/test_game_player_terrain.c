#define main original_ray_static_test
#include "test_game_ray_static.c"
#undef main
#include "game_player_terrain.h"
static void putf(uint8_t*p,float x){memcpy(p,&x,4);}
static void actor_pose(gh_title_player_view*v,const float*old,const float*current){for(unsigned k=0;k<3;k++){putf(v->bytes+0x490+k*4,old[k]);putf(v->alias_f0+k*4,current[k]);}}
int main(int argc,char**argv){
 C(argc==2);C(!original_ray_static_test(argc,argv));
 gh_disc*d;size_t ix;gh_disc_entry en;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&ix)&&!gh_disc_entry_at(d,ix,&en));
 uint8_t*room=malloc((size_t)en.size);C(room&&!gh_disc_read(d,ix,0,room,(size_t)en.size));gh_disc_close(d);
 gh_bundle b;gh_game_scene_tables s={0};C(!gh_bundle_open(room,(size_t)en.size,&b)&&!gh_scene_actors_0012e670(&s,&b));uint8_t*g=group_masks(&s);
 uint8_t bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view actor={.bytes=bytes,.size=sizeof(bytes),.alias_f0=bytes+0xb0};gh_player_terrain_bindings binding={.tables=&s,.actor=&actor};
 float pos[4]={777,888,999,111},normal[4]={77,88,99,222};uint32_t pc;int hit;gh_scene_ray_report report[2];
 /* ActualR100 floor polygons, with independent global doubleplane oracle. */
 unsigned floor_count=0;
 for(unsigned i=0;i<half(s.akg.bytes+0x28)&&floor_count<8;i++){
  const uint8_t*t=s.actor_links[4].view.bytes+(size_t)i*20;if(!(g[i]&2)||word(t+16)&2)continue;
  double n[3];point(s.actor_links[3].view,half(t+8),n);if(n[1]<.8)continue;
  gh_scene_ray_request seed;request_for(&s,i,&seed);float current[3];for(unsigned k=0;k<3;k++)current[k]=seed.from[k];actor_pose(&actor,current,current);
  gh_scene_ray_request ray={.mask=2,.flags=0x400};for(unsigned k=0;k<3;k++)ray.from[k]=ray.to[k]=current[k];ray.from[1]+=1;ray.to[1]-=20;answer expected=oracle(&s,g,&ray);if(!expected.hit)continue;
  gh_player_terrain_call call={0};memset(report,0xa5,sizeof(report));gh_scene_ray_report sentinel=report[1];
  C(!gh_player_terrain_floor_001329c0(&call,&binding,20,pos,normal,report,&hit,&pc)&&hit&&call.complete&&!pc&&pos[3]==111&&normal[3]==222&&!memcmp(&report[1],&sentinel,sizeof(sentinel)));
  for(unsigned k=0;k<3;k++)C(fabs(pos[k]-expected.position[k])<.0004);
  uint64_t queries=s.ray.native_queries;C(!gh_player_terrain_floor_001329c0(&call,&binding,20,pos,normal,report,&hit,&pc)&&s.ray.native_queries==queries);
  floor_count++;
 }
 C(floor_count>=4);
 /* Actualwall crossing: independent globalplane oracle verifies backoff. */
 unsigned walls=0;for(unsigned i=0;i<half(s.akg.bytes+0x28)&&walls<4;i++){
  if(!(g[i]&1))continue;gh_scene_ray_request q;request_for(&s,i,&q);q.mask=1;q.flags=0x200;
  answer expected=oracle(&s,g,&q);if(!expected.hit)continue;
  float oldp[3],curp[3];for(unsigned k=0;k<3;k++){oldp[k]=q.from[k]-(k==1?.8f:0);curp[k]=q.to[k]-(k==1?.8f:0);}actor_pose(&actor,oldp,curp);
  gh_player_terrain_call w={0};C(gh_player_terrain_wall_00132da0(&w,&binding,.4f,0x200,pos,NULL,0,NULL,&hit,&pc)==GH_RAY_UNSUPPORTED&&pc==0x1321d8);
  double len=0,direction[3];for(unsigned k=0;k<3;k++){direction[k]=(double)oldp[k]-curp[k];len+=direction[k]*direction[k];}len=sqrt(len);
  for(unsigned k=0;k<3;k++)C(fabs(w.candidate[k]-(expected.position[k]+direction[k]/len*.2))<.0005);
  walls++;
 }C(walls>=1);
 /* Zero displacement outsideworld: sphere still required evenafterreal raymiss. */
 float outside[3]={100000,100000,100000};actor_pose(&actor,outside,outside);gh_player_terrain_call wall={0};
 C(gh_player_terrain_wall_00132da0(&wall,&binding,.4f,0x200,pos,NULL,0,NULL,&hit,&pc)==GH_RAY_UNSUPPORTED&&pc==0x1321d8&&!hit&&wall.phase==3);
 C(wall.candidate[0]==outside[0]&&wall.candidate[1]==outside[1]+.8f);uint64_t q=s.ray.native_queries;
 for(unsigned k=0;k<3;k++)C(gh_player_terrain_wall_00132da0(&wall,&binding,.4f,0x200,pos,NULL,0,NULL,&hit,&pc)==GH_RAY_UNSUPPORTED&&pc==0x1321d8&&s.ray.native_queries==q);
 gh_player_terrain_bindings foreign=binding;gh_game_scene_tables other=s;foreign.tables=&other;C(gh_player_terrain_wall_00132da0(&wall,&foreign,.4f,0x200,pos,NULL,0,NULL,&hit,&pc)==GH_RAY_ARGUMENT);
 C(gh_player_terrain_wall_00132da0(&wall,&binding,.4f,0x224,pos,NULL,0,NULL,&hit,&pc)==GH_RAY_ARGUMENT);
 /* Original5A0bit4 gates beforebadalias/nonfiniteradius anddoesnotquery. */
 uint32_t four=4;memcpy(bytes+0x5a0,&four,4);actor.alias_f0=NULL;gh_player_terrain_call skip={0};C(!gh_player_terrain_wall_00132da0(&skip,&binding,NAN,0x200,pos,NULL,0,NULL,&hit,&pc)&&!hit&&skip.complete&&s.ray.native_queries==q);memset(bytes+0x5a0,0,4);actor.alias_f0=bytes+0xb0;
 /* Longfloor walk preserves completeintermediatequeries onlaterfailure. */
 float old[3]={100000,100000,100000},cur[3]={100003.5f,100000,100000};actor_pose(&actor,old,cur);gh_player_terrain_call walk={0};q=s.ray.native_queries;C(!gh_player_terrain_floor_001329c0(&walk,&binding,20,pos,normal,NULL,&hit,&pc)&&!hit&&walk.complete&&s.ray.native_queries==q+3&&!s.ray.mode181);
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned k=0;k<4;k++){C(!fesetround(modes[k])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));int flags=fetestexcept(FE_ALL_EXCEPT);gh_player_terrain_call a={0};C(!gh_player_terrain_floor_001329c0(&a,&binding,20,pos,normal,NULL,&hit,&pc)&&!hit&&fegetround()==modes[k]&&fetestexcept(FE_ALL_EXCEPT)==flags);}
 C(!fesetround(FE_TONEAREST));gh_scene_ray_triangle_state_release(&s);free(g);free(room);printf("terrain actualISO floors%u + spherefrontier/retry/mode181/FEnv PASS\n",floor_count);return 0;
}
