#include "game_actor_matrix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
static void rotate(long double*v,unsigned axis,long double a){unsigned i=(axis+1)%3,j=(axis+2)%3;long double x=v[i],y=v[j];v[i]=cosl(a)*x-sinl(a)*y;v[j]=sinl(a)*x+cosl(a)*y;}
int main(void){const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};const unsigned order[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};float p[3]={2,-4,8},a[3]={.25f,-1,.5f},scale[3]={2,.5f,-4},m[16],golden[7][16];uint32_t f;unsigned checked=0;
 for(unsigned rounding=0;rounding<4;rounding++){C(!fesetround(modes[rounding])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));for(unsigned mode=0;mode<7;mode++){C(!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));C(!gh_actor_matrix_compose_00147a48(p,a,scale,mode,m,GH_STATIC_BINARY32_REFERENCE,&f)&&!f);C(fegetround()==modes[rounding]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);if(!rounding)memcpy(golden[mode],m,sizeof(m));else C(!memcmp(golden[mode],m,sizeof(m)));
 // Independent highprecision point rotation (Rodrigues in axis planes), no
 // matrix multiplication or original polynomial mirrored in this oracle.
 for(unsigned test=0;test<8;test++){long double v[3]={(test&1)?1.5L:-.5L,(test&2)?2.L:-1.L,(test&4)?.25L:-2.L},o[3];for(unsigned k=0;k<3;k++){o[k]=v[k];v[k]*=scale[k];}if(mode<6)for(unsigned s=0;s<3;s++)rotate(v,order[mode][s],a[order[mode][s]]);for(unsigned k=0;k<3;k++){v[k]+=p[k];long double got=(long double)m[k]*o[0]+(long double)m[4+k]*o[1]+(long double)m[8+k]*o[2]+m[12+k];C(fabsl(got-v[k])<.00005L);checked++;}}
 }
 // Captured offline original VU scalar polynomial vectors, propagated through
 // real SDK Y-axis construction; exactbits distinguish libc trig substitution.
 C(!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));float z[3]={0,0,0},yaw[3]={0,.25f,0},unit[3]={1,1,1};C(!gh_actor_matrix_compose_00147a48(z,yaw,unit,0,m,GH_STATIC_BINARY32_REFERENCE,&f));float pure[16];C(!gh_actor_matrix_rotation_00147728(yaw,0,pure,GH_STATIC_BINARY32_REFERENCE,&f));C(!memcmp(pure,m,64));C(bits(m[0])==0x3f780aa6&&bits(m[2])==0xbe7d5767&&bits(m[8])==0x3e7d5767&&bits(m[10])==0x3f780aa6);
 float raw[4]={7,-7,0,123},wrapped[4];C(!gh_actor_euler_adjust_001f7da0(raw,wrapped,GH_STATIC_BINARY32_REFERENCE,&f)&&wrapped[3]==123&&wrapped[0]>0&&wrapped[0]<1&&wrapped[1]<0&&wrapped[1]>-1&&wrapped[2]==0);C(fegetround()==modes[rounding]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
 float before[16];for(unsigned k=0;k<16;k++)m[k]=before[k]=123;a[1]=1000;C(gh_actor_matrix_compose_00147a48(p,a,scale,0,m,GH_STATIC_BINARY32_REFERENCE,&f)==GH_ACTOR_MATRIX_UNSUPPORTED&&f==0x30a5d0&&!memcmp(before,m,sizeof(m)));a[1]=-1;C(fegetround()==modes[rounding]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
 }
 for(unsigned rounding=0;rounding<4;rounding++){float a[16],b[16],m[16],before[16];for(unsigned i=0;i<16;i++){a[i]=(float)((int)i-8)*.125f;b[i]=(float)((int)(i%5)-2)*.25f;m[i]=before[i]=123;}C(!fesetround(modes[rounding])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));uint32_t pc;C(!gh_actor_matrix_product_00147548(a,b,m,GH_STATIC_BINARY32_REFERENCE,&pc)&&!pc);C(fegetround()==modes[rounding]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);for(unsigned c=0;c<4;c++)for(unsigned r=0;r<4;r++){long double expected=0;for(unsigned k=0;k<4;k++)expected+=(long double)a[k*4+r]*b[c*4+k];C((long double)m[c*4+r]==expected);}float alias[16];memcpy(alias,a,64);C(!gh_actor_matrix_product_00147548(alias,b,alias,GH_STATIC_BINARY32_REFERENCE,&pc)&&!memcmp(alias,m,64));uint32_t nan=0x7fc00000;memcpy(a+5,&nan,4);C(gh_actor_matrix_product_00147548(a,b,before,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MATRIX_UNSUPPORTED&&pc==0x30a2e0);for(unsigned k=0;k<16;k++)C(before[k]==123);C(fegetround()==modes[rounding]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);}
 C(!fesetround(FE_TONEAREST));printf("PASS %u independentlongdouble point coordinates, all6 SDKorders/nonuniformnegativeScale/bitgoldenYaw/defaultidentity/atomicout/4FEnv; binary32reference notEEbitexact\n",checked);return 0;}
