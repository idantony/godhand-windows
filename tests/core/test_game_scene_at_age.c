#include "game_scene_at_age.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static uint32_t expected(uint32_t old){return (old&0x31ffffffu)|((old&0x80000000u)?0x40000000u:0);}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;size_t index;gh_disc_entry entry;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&index)&&!gh_disc_entry_at(d,index,&entry));
 uint8_t*data=malloc((size_t)entry.size),*copy=malloc((size_t)entry.size);C(data&&copy&&!gh_disc_read(d,index,0,data,(size_t)entry.size));memcpy(copy,data,(size_t)entry.size);
 gh_bundle room;C(!gh_bundle_open(data,(size_t)entry.size,&room));gh_heap*heap;gh_scene_at_manager manager={0};C(!gh_heap_create(4096,&heap)&&!gh_scene_at_initialize_units_002c0f90(&manager,&room,heap,0x100,1));
 gh_at_native_unit units[32],u;size_t count=0;C(!gh_scene_at_first_unit_002c30c0(&manager,&u));while(u.bytes){C(count<32);units[count++]=u;C(!gh_scene_at_next_unit_002c30c8(&manager,u,&u));}C(count>=24);
 int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};uint32_t frontier=0;
 for(unsigned m=0;m<4;m++)for(uint32_t flags=0;flags<128;flags++){
  C(!fesetround(modes[m]));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_DIVBYZERO);int exceptions=fetestexcept(FE_ALL_EXCEPT);
  uint32_t old=0x013579bdu|(flags<<25);for(size_t i=0;i<count;i++)put(units[i].bytes+0x50,old);
  gh_scene_at_age age={0};C(!gh_scene_at_age_002c1540(&age,&manager,&frontier)&&!frontier&&!age.active);
  for(size_t i=0;i<count;i++)C(word(units[i].bytes+0x50)==expected(old));C(fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);
 }
 /* Poison a later slot traversal, repair it, and prove already-aged PREV bits
  * survive retry. Replaying the completed prefix would clear those bits. */
 for(size_t i=0;i<count;i++)put(units[i].bytes+0x50,0xffffffffu);
 gh_at_link saved=manager.heads[0];manager.heads[0]=(gh_at_link){GH_AT_LINK_SLOT,0};gh_scene_at_age age={0};
 C(gh_scene_at_age_002c1540(&age,&manager,&frontier)==GH_AT_BOUNDS&&frontier==0x2c30c8&&age.completed>0);
 size_t completed=age.completed;C(gh_scene_at_age_002c1540(&age,&manager,&frontier)==GH_AT_BOUNDS&&age.completed==completed);
 manager.heads[0]=saved;C(!gh_scene_at_age_002c1540(&age,&manager,&frontier)&&!age.active);
 for(size_t i=0;i<count;i++)C(word(units[i].bytes+0x50)==0x71ffffffu);
 C(!gh_scene_at_age_002c1540(&age,&manager,&frontier));for(size_t i=0;i<count;i++)C(word(units[i].bytes+0x50)==0x31ffffffu);
 C(!memcmp(data,copy,(size_t)entry.size)&&!gh_scene_at_units_release(&manager)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
 feclearexcept(FE_ALL_EXCEPT);C(!fesetround(FE_TONEAREST));free(copy);free(data);gh_disc_close(d);
 puts("PASS actualR100 full tagged unit list:2C1540 128flags/fourFEnv, same mutable units, preserves completed prefix across link failure/repair, next true call ages again, immutable source/release");return 0;
}
