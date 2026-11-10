#include "game_collision_pair_queries.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void identity(uint8_t*p){memset(p,0,64);for(unsigned i=0;i<4;i++)put(p+i*20,0x3f800000);}
int main(void){int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned mode=0;mode<4;mode++){C(!fesetround(modes[mode]));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_INVALID);int exceptions=fetestexcept(FE_ALL_EXCEPT);uint8_t a[64],b[64];identity(a);identity(b);int hit,wrong;uint32_t pc;
 put(b+52,0x3f800000);C(!gh_collision_query_kind2_pair_002bcf28(a,0x3f000000,0x3e800000,b,0x3e000000,0x3e800000,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&!hit);C(!gh_collision_query_cylinder_cylinder_002bc470(a,0x3f000000,0x3e800000,b,0x3e000000,0x3e800000,GH_STATIC_BINARY32_REFERENCE,&wrong,&pc)&&wrong);
 fenv_t saved;C(!fegetenv(&saved));for(int xi=-8;xi<=8;xi++)for(int yi=-16;yi<=16;yi++)for(int zi=-4;zi<=4;zi++){float x=xi*.25f,y=yi*.25f,z=zi*.25f;put(b+48,bits(x));put(b+52,bits(y));put(b+56,bits(z));C(!gh_collision_query_kind2_pair_002bcf28(a,0x3f000000,0x40800000,b,0x3f000000,0x40000000,GH_STATIC_BINARY32_REFERENCE,&hit,&pc));long double lo=y,hi=(long double)y+2,gap=hi<0?-hi:lo>4?lo-4:0;long double distance=sqrtl((long double)x*x+(long double)z*z+gap*gap);if(fabsl(distance-1)>0.000001L)C(hit==(distance<=1));}C(!fesetenv(&saved));
 identity(b);put(b,0);put(b+4,0xbf800000);put(b+16,0x3f800000);put(b+20,0);put(b+48,0xc0000000);put(b+52,0x40000000);C(!gh_collision_query_kind2_pair_002bcf28(a,0x3e000000,0x40800000,b,0x3e000000,0x40800000,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&hit);put(b+56,0x3f000000);C(!gh_collision_query_kind2_pair_002bcf28(a,0x3e000000,0x40800000,b,0x3e000000,0x40800000,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&!hit);
 identity(b);C(!gh_collision_query_kind2_pair_002bcf28(a,0,0,b,0,0,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)&&hit);put(b+48,0x41200000);hit=77;C(gh_collision_query_kind2_pair_002bcf28(a,0,0x40000000,b,0,0x40000000,GH_STATIC_BINARY32_REFERENCE,&hit,&pc)==GH_PAIR_QUERY_UNSUPPORTED&&pc==0x2bc0f8&&hit==77&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);
 }puts("PASS actualkind2x2 2BCF28 fullendpoints/shared2BC0F8; trimfalsepositivecounterexample+20196 independentparallelgrid/interiorcross/countfrontier4FEnv");return 0;}
