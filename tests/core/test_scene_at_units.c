#include "scene_at_units.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
int main(int argc,char**argv){gh_disc*d;size_t index;gh_disc_entry entry;C(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&index)&&!gh_disc_entry_at(d,index,&entry));uint8_t*p=malloc((size_t)entry.size),*copy=malloc((size_t)entry.size);C(p&&copy&&!gh_disc_read(d,index,0,p,(size_t)entry.size));memcpy(copy,p,(size_t)entry.size);gh_bundle b;C(!gh_bundle_open(p,(size_t)entry.size,&b));int modes[]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO};
 for(unsigned mode=0;mode<4;mode++){gh_heap*heap;gh_scene_at_manager s={0};C(!gh_heap_create(4096,&heap)&&!fesetround(modes[mode]));feraiseexcept(FE_INEXACT);int before=fetestexcept(FE_ALL_EXCEPT);
  C(gh_scene_at_initialize_units_002c0f90(&s,&b,heap,0x100,0)==GH_AT_UNSUPPORTED&&s.frontier==0x1f88a8&&!s.ready&&gh_heap_used(heap)==0x138);C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==before);
  C(!gh_scene_at_initialize_units_002c0f90(&s,&b,heap,0x100,1)&&s.ready&&s.units[0].count==24&&!s.units[1].count&&!s.units[2].count&&!s.units[3].count&&gh_heap_used(heap)==0x138);C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==before);
  gh_at_native_unit zero,one,two,three;C(!gh_scene_at_get_jump_002c2ae0(&s,0,&zero)&&zero.bytes&&!gh_scene_at_get_jump_002c2ae0(&s,1,&one)&&one.bytes&&!gh_scene_at_get_jump_002c2ae0(&s,2,&two)&&!two.bytes&&!gh_scene_at_get_jump_002c2ae0(&s,3,&three)&&three.bytes);
  C(zero.bytes[0x39]==4&&zero.bytes[0x3a]==0x20&&zero.bytes[0x3b]==8&&one.bytes[0x39]==0&&one.bytes[0x3a]==0x31&&!one.bytes[0x3b]);C(three.bytes[0x35]==1&&three.bytes[0x79]==3&&three.bytes[0x36]==0xff&&three.bytes[0x37]==0xff&&three.bytes[0x39]==4&&s.jump_slots[0].bytes==three.bytes);
  C(three.bytes[0x45]==89&&three.bytes[0x46]==40&&word(three.bytes+20)==0x3fa66666&&word(three.bytes+24)==0xc29fe147&&word(three.bytes+36)==0x4089999a&&word(three.bytes+40)==0xc299e147);
  int original;C(!gh_scene_at_set_enabled(&s,zero,0,&original)&&original&&!(zero.bytes[0x34]&1));C(!gh_scene_at_set_enabled(&s,zero,1,&original)&&original&&(zero.bytes[0x34]&1));gh_at_native_unit foreign=zero;foreign.bytes++;C(gh_scene_at_set_enabled(&s,foreign,0,&original)==GH_AT_ARGUMENT);
  C(!gh_scene_at_set_jump_info_002c2750(&s,zero,0xff,0xa38,0,0)&&zero.bytes[0x44]==0xff&&zero.bytes[0x3c]==0x38&&zero.bytes[0x3d]==0xa&&!zero.bytes[0x40]);
  C(!memcmp(copy,p,(size_t)entry.size)&&!gh_scene_at_units_release(&s)&&!s.native_units&&!s.ready&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));feclearexcept(FE_ALL_EXCEPT);
 }
 fesetround(FE_TONEAREST);free(copy);free(p);gh_disc_close(d);puts("PASS actualR10024sourceunits/JMP4 typedlinks, self-target isolatedallocation, strictfrontier/resume, explicitreferenceFEnv, nativeget/set ownership and realrelease");return 0;}
