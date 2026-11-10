#include "game_collision_pair_queries.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static uint32_t bits(float n){uint32_t b;memcpy(&b,&n,4);return b;}
static void identity(uint8_t*p){memset(p,0,64);for(unsigned i=0;i<4;i++)put(p+i*20,0x3f800000);}
int main(void){struct test{uint32_t point[3],distance;int hit;}cases[]={{{0x3f000000,0x3f800000,0},0x3f000000,1},{{0x3f800000,0x3f800000,0},0x3f800000,1},{{0x3fa00000,0x3f800000,0},0x3fa00000,0},{{0,0xbf800000,0},0x3f800000,1},{{0,0xbfa00000,0},0x3fa00000,0},{{0,0x3f800000,0},0,1},{{0,0x40400000,0},0x3f800000,1}};int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned mode=0;mode<4;mode++){C(!fesetround(modes[mode]));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_INVALID);int exceptions=fetestexcept(FE_ALL_EXCEPT);uint8_t m[64];identity(m);for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){int hit=-1;uint32_t distance=0xdeadbeef,pc;C(!gh_collision_query_sphere_cylinder_002bb670((const uint8_t*)cases[i].point,0x3f000000,m,0x3f000000,0x40000000,GH_STATIC_BINARY32_REFERENCE,&hit,&distance,&pc)&&!pc&&hit==cases[i].hit&&distance==cases[i].distance&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);}
 /* Independent long-double closest-point oracle on exact quarter-grid.
  * Original normalization may differ by a few binary32 ULP from geometric
  * distance; hit comparison is tested away from the rounding boundary. */
 fenv_t oracle_env;C(!fegetenv(&oracle_env));for(int xi=-8;xi<=8;xi++)for(int yi=-8;yi<=16;yi++)for(int zi=-4;zi<=4;zi++){float x=xi*0.25f,y=yi*0.25f,z=zi*0.25f;uint32_t p[]={bits(x),bits(y),bits(z)},d,pc;int hit;C(!gh_collision_query_sphere_cylinder_002bb670((const uint8_t*)p,0x3f000000,m,0x3f000000,0x40000000,GH_STATIC_BINARY32_REFERENCE,&hit,&d,&pc));long double closest=y<0?0:y>2?2:y;long double dx=x,dy=(long double)y-closest,dz=z;long double exact=sqrtl(dx*dx+dy*dy+dz*dz);float got;memcpy(&got,&d,4);C(fabsl((long double)got-exact)<=0.000001L);if(fabsl(exact-1)>0.000001L)C(hit==(exact<=1));}
 C(!fesetenv(&oracle_env));
 /* Exactaxis rotation+translation, not a vertical-only cylinder shortcut. */
 identity(m);put(m,0);put(m+4,0x3f800000);put(m+16,0xbf800000);put(m+20,0);put(m+48,0x40400000);put(m+52,0x40800000);put(m+56,0x40a00000);uint32_t point[]={0x40000000,0x40a00000,0x40a00000},distance,pc;int hit;C(!gh_collision_query_sphere_cylinder_002bb670((const uint8_t*)point,0x3f000000,m,0x3f000000,0x40000000,GH_STATIC_BINARY32_REFERENCE,&hit,&distance,&pc)&&hit&&distance==0x3f800000);
 identity(m);uint32_t degenerate[]={0x3f800000,0,0};C(!gh_collision_query_sphere_cylinder_002bb670((const uint8_t*)degenerate,0x3f000000,m,0x3f000000,0,GH_STATIC_BINARY32_REFERENCE,&hit,&distance,&pc)&&hit&&distance==0x3f800000);degenerate[0]=0x7fc00000;hit=77;distance=0x12345678;C(gh_collision_query_sphere_cylinder_002bb670((const uint8_t*)degenerate,0x3f000000,m,0x3f000000,0,GH_STATIC_BINARY32_REFERENCE,&hit,&distance,&pc)==GH_PAIR_QUERY_UNSUPPORTED&&pc==0x1566c0&&hit==77&&distance==0x12345678&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);
 }puts("PASS actual2BB670/1566C0 point-segment endpoint/interior/collinear/degenerate; <=radius equality, rotatedtranslatedcylinder, atomicunknown4FEnv");return 0;}
