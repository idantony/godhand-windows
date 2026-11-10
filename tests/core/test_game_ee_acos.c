#include "game_ee_acos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#include <float.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static float bits(uint32_t u){float x;memcpy(&x,&u,4);return x;}
static uint32_t word(float x){uint32_t u;memcpy(&u,&x,4);return u;}
/* Independent extended-precision oracle: exactfloat product then explicit
 * binary32 rounding at original COP1 boundaries; transcendental result uses
 * longdouble atan2, not the native eight-term polynomial implementation. */
static long double oracle(float x){float square=(float)((long double)x*x);if(square!=0&&square<FLT_MIN)square=0;float radical=(float)(1-(long double)square);float root=(float)sqrtl(fabsl((long double)radical));return atan2l((long double)root,x);}
int main(void){
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};float input[10200];unsigned count=0;C(!fesetround(FE_TONEAREST));
 for(unsigned i=0;i<=10000;i++)input[count++]=(float)((long double)((int)i-5000)/5000);
 for(unsigned sign=0;sign<2;sign++)for(int ulp=-32;ulp<=32;ulp++)input[count++]=bits((uint32_t)((int64_t)0x3f800000+ulp)|(sign?0x80000000u:0));
 input[count++]=0;input[count++]=-0.0f;input[count++]=2;input[count++]=-2;input[count++]=4;input[count++]=-4;input[count++]=FLT_MIN;
 uint32_t golden[10200];unsigned total=0;
 for(unsigned mode=0;mode<4;mode++)for(unsigned i=0;i<count;i++){
  C(!fesetround(FE_TONEAREST));long double expected=oracle(input[i]);C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_INVALID|FE_DIVBYZERO));int flags=fetestexcept(FE_ALL_EXCEPT);float angle=-123;uint32_t frontier=123;
  C(!gh_game_ee_acos_reference_001f8140(input[i],&angle,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier)&&!frontier&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==flags);C(!fesetround(FE_TONEAREST));C(fabsl((long double)angle-expected)<.000002L);if(!mode)golden[i]=word(angle);else C(golden[i]==word(angle));total++;
 }
 /* Root66 actualWindows childstage2/angularconstraint input3F800001,
  * radicalB4800000 (-2^-22). Preserve the original nonzero response. */
 float observed=bits(0x3f800001);float observedSquare=(float)((long double)observed*observed);float observedRadical=(float)(1-(long double)observedSquare);C(word(observedRadical)==0xb4800000u);
 float a=-123,b=-123;uint32_t f;C(!gh_game_ee_acos_reference_001f8140(bits(0x3f800001),&a,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&f)&&a>.0004f&&a<.0006f);C(!gh_game_ee_acos_reference_001f8140(bits(0xbf800001),&b,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&f)&&b>3.14f&&b<3.1415f);C(!gh_game_ee_acos_reference_001f8140(1,&a,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&f)&&a==0);C(!gh_game_ee_acos_reference_001f8140(-1,&a,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&f)&&word(a)==0x40490fdb);
 const float bad[]={NAN,INFINITY,-INFINITY,FLT_MAX,bits(1)};for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){a=123;C(gh_game_ee_acos_reference_001f8140(bad[i],&a,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&f)==GH_CAMERA_NUMERIC&&f==0x1f8140&&a==123);}
 a=123;C(gh_game_ee_acos_reference_001f8140(.5f,&a,GH_CAMERA_EXACT_UNAVAILABLE,&f)==GH_CAMERA_UNSUPPORTED&&f==0x1f8140&&a==123);C(gh_game_ee_acos_reference_001f8140(.5f,NULL,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&f)==GH_CAMERA_ARGUMENT);C(!fesetround(FE_TONEAREST));printf("PASS EE1F8140 numericreference %u cases independentlongdouble oracle/4FEnv; +/-1 32ULP neighborhoods; COP1negativeSQRTabs nonzero-notclamp; atomicstrictdomain\n",total);return 0;
}
