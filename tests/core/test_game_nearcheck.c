#include "game_scene_step.h"
#include "game_ray_static.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static uint16_t half(const uint8_t*p){uint16_t n;memcpy(&n,p,2);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void ph(uint8_t*p,uint16_t n){memcpy(p,&n,2);}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;size_t index;gh_disc_entry entry;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&index)&&!gh_disc_entry_at(d,index,&entry));
 uint8_t*data=malloc((size_t)entry.size),*copy=malloc((size_t)entry.size);C(data&&copy&&!gh_disc_read(d,index,0,data,(size_t)entry.size));memcpy(copy,data,(size_t)entry.size);gh_disc_close(d);
 gh_bundle room;gh_bundle_entry pfd;C(!gh_bundle_open(data,(size_t)entry.size,&room)&&!gh_bundle_find(&room,"PFD",0,&pfd));
 gh_scene_nearcheck n={0};C(!gh_scene_nearcheck_bind_0031c1d0(&n,&pfd)&&n.pair_count>=3&&n.vertex_count>0&&n.vertex_count<65535&&n.owned_pairs&&n.pairs.bytes==n.owned_pairs);
 uint16_t a=0xffff,b=0xffff;for(uint32_t i=0;i<n.pair_count;i++){uint16_t x=half(n.pairs.bytes+(size_t)i*12),y=half(n.pairs.bytes+(size_t)i*12+2);if(x<n.vertex_count&&y<n.vertex_count){a=x;b=y;break;}}C(a!=0xffff&&b!=0xffff);
 gh_game_scene_tables tables={0};C(!gh_scene_actors_0012e670(&tables,&room));gh_scene_ray_request request={.mask=1,.flags=0x40,.segmented=0};memcpy(request.from,n.vertices.bytes+(size_t)a*32,16);memcpy(request.to,n.vertices.bytes+(size_t)b*32,16);int hit;uint32_t frontier;
 C(!gh_scene_ray_query_00131e10(&tables,&request,&hit,&frontier));uint32_t expected=0x13579bdfu|(hit?0x80000000u:0);
 uint32_t original_count=n.pair_count;n.pair_count=3;gh_boot_work work={0};
 for(unsigned mode=0;mode<4;mode++){int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned i=0;i<3;i++){ph(n.owned_pairs+i*12,a);ph(n.owned_pairs+i*12+2,b);put(n.owned_pairs+i*12+8,0x13579bdf);}n.cursor=0;uint64_t queries=tables.ray.native_queries;
  C(!fesetround(modes[mode]));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_DIVBYZERO);int exceptions=fetestexcept(FE_ALL_EXCEPT);
  C(!gh_scene_nearcheck_step_002a6ff8(&n,&work,&tables,&frontier)&&!frontier&&!n.active&&n.cursor==2&&tables.ray.native_queries==queries+2);
  C(word(n.owned_pairs+8)==expected&&word(n.owned_pairs+20)==expected&&word(n.owned_pairs+32)==0x13579bdf&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);
 }
 /* First completed ray/flag/cursor survive a second-ray host bounds error. */
 n.cursor=0;ph(n.owned_pairs+14,(uint16_t)n.vertex_count);uint64_t queries=tables.ray.native_queries;
 C(gh_scene_nearcheck_step_002a6ff8(&n,&work,&tables,&frontier)==GH_SCENE_STEP_BOUNDS&&frontier==0x131e10&&n.active&&n.iteration==1&&n.cursor==1&&tables.ray.native_queries==queries+1);
 C(gh_scene_nearcheck_step_002a6ff8(&n,&work,&tables,&frontier)==GH_SCENE_STEP_BOUNDS&&tables.ray.native_queries==queries+1);
 ph(n.owned_pairs+14,b);C(!gh_scene_nearcheck_step_002a6ff8(&n,&work,&tables,&frontier)&&!n.active&&tables.ray.native_queries==queries+2&&word(n.owned_pairs+8)==expected);
 n.cursor=INT32_MAX;queries=tables.ray.native_queries;C(!gh_scene_nearcheck_step_002a6ff8(&n,&work,&tables,&frontier)&&n.cursor==-1&&tables.ray.native_queries==queries);
 C(!gh_scene_nearcheck_step_002a6ff8(&n,&work,&tables,&frontier)&&n.cursor==1&&tables.ray.native_queries==queries+1);
 ph(n.owned_pairs+12,0xffff);n.cursor=0;queries=tables.ray.native_queries;C(!gh_scene_nearcheck_step_002a6ff8(&n,&work,&tables,&frontier)&&n.cursor==2&&tables.ray.native_queries==queries+1);
 work.flags598=0x80000;C(!gh_scene_nearcheck_step_002a6ff8(&n,&work,&tables,&frontier)&&n.cursor==2&&tables.ray.native_queries==queries+1);
 uint8_t*owned=n.owned_pairs;gh_scene_nearcheck_reset_0031be78(&n);C(!n.pair_count&&n.owned_pairs==owned&&n.cursor==2);C(!gh_scene_nearcheck_bind_0031c1d0(&n,&pfd)&&n.pair_count==original_count&&n.owned_pairs==owned&&word(n.owned_pairs+8)==expected);
 C(!memcmp(data,copy,(size_t)entry.size));gh_scene_nearcheck_release(&n);C(!n.owned_pairs&&!n.pairs.bytes&&!n.source&&!n.pair_count&&n.cursor==2);gh_scene_ray_triangle_state_release(&tables);
 feclearexcept(FE_ALL_EXCEPT);C(!fesetround(FE_TONEAREST));free(copy);free(data);
 puts("PASS actualR100 PFD/nativeNearCheck: mask1/flags40 straightquery, two visits/flags, signedwrap, sentinel/disable, paused second-query retry, same mutable pair owner and source immutability/lifetime");return 0;
}
