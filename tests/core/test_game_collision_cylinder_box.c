#include "game_collision_pair_queries.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void identity(uint8_t*p){memset(p,0,64);for(unsigned i=0;i<4;i++)put(p+i*20,0x3f800000);}
int main(void){int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned mode=0;mode<4;mode++){C(!fesetround(modes[mode]));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_INVALID);int exceptions=fetestexcept(FE_ALL_EXCEPT);uint8_t c[64],b[64];identity(c);identity(b);put(b+52,0x3fc00000);uint32_t size[]={0x3f800000,0x3f800000,0x3f800000},pc;int hit;
 /* Exact-grid independent overlap of the original expanded face intervals;
  * includes corner falsepositives, not a geometric sphere/AABB oracle. */
 for(int xi=-4;xi<=4;xi++)for(int yi=-12;yi<=12;yi++)for(int zi=-4;zi<=4;zi++){float x=xi*.25f,y=yi*.25f,z=zi*.25f;put(c+48,bits(x));put(c+52,bits(y));put(c+56,bits(z));C(!gh_collision_query_cylinder_box_002bcb80(c,0x3e800000,0x40800000,b,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&!pc&&hit==(x>=-.75f&&x<=.75f&&z>=-.75f&&z<=.75f&&y+3.75f>=1.25f&&y+.25f<=2.75f)&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);}
 /* Both trimmed endpoints miss; original interior sphere samples hit. */
 identity(c);put(c,0);put(c+4,0xbf800000);put(c+16,0x3f800000);put(c+20,0);put(c+48,0xc0000000);put(c+52,0x40000000);C(!gh_collision_query_cylinder_box_002bcb80(c,0x3e000000,0x40800000,b,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&hit);put(c+56,0x3f800000);C(!gh_collision_query_cylinder_box_002bcb80(c,0x3e000000,0x40800000,b,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&!hit);
 /* Raw short-height trim remains reversed instead of clamped to a point. */
 identity(c);identity(b);put(b+52,0x3f800000);uint32_t thin[]={0x3f800000,0x3e800000,0x3f800000};C(!gh_collision_query_cylinder_box_002bcb80(c,0x3f000000,0x3e800000,b,thin,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&hit);
 identity(c);identity(b);uint32_t zero[3]={0};C(!gh_collision_query_cylinder_box_002bcb80(c,0x3e800000,0,b,zero,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&hit);put(c+48,0x41200000);hit=77;C(gh_collision_query_cylinder_box_002bcb80(c,0,0x40000000,b,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)==GH_PAIR_QUERY_UNSUPPORTED&&pc==0x2bc950&&hit==77);C(gh_collision_query_cylinder_box_002bcb80(c,bits(.0001f),0x40000000,b,size,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)==GH_PAIR_QUERY_UNSUPPORTED&&pc==0x2bc950&&hit==77&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);
 }puts("PASS actual2BCB80/2BC6C8/2BC950 trimmed sampledexpandedfaces;8100 independentintervalgrid, interiorsample/corner/shortheight/degenerate/countfrontier4FEnv");return 0;}
