#include "game_effect_exp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(v) do{if(!(v)){fprintf(stderr,"line%d %s\n",__LINE__,#v);exit(1);}}while(0)
static uint32_t bits(float v){uint32_t x;memcpy(&x,&v,4);return x;}
static float number(uint32_t x){float v;memcpy(&v,&x,4);return v;}
static float input(unsigned i){if(i<64){unsigned j=i&31;float variance=i<32?100.0f:25.0f;volatile float square=(float)(j*j),half=square*-.5f,result=half/variance;return result;}return number(0x80000000u|((i*UINT32_C(1664525)+UINT32_C(1013904223))%0x42a00001u));}
int main(int argc,char**argv){C(argc==1||(argc==2&&!strcmp(argv[1],"--oracle-vectors")));const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};int dump=argc==2;C(!fesetround(FE_TONEAREST));float values[4096];for(unsigned i=0;i<4096;i++)values[i]=input(i);for(unsigned i=0;i<4096;i++){uint32_t expected=0;for(unsigned m=0;m<4;m++){C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));float result=7;uint32_t frontier;C(!gh_effect_exp_reference_003a1c78(values[i],&result,GH_STATIC_BINARY32_REFERENCE,&frontier)&&!frontier&&fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);if(!m)expected=bits(result);else C(bits(result)==expected);}if(dump)printf("%08X %08X\n",bits(values[i]),expected);}C(!fesetround(FE_TONEAREST));float unchanged=123;uint32_t frontier;C(gh_effect_exp_reference_003a1c78(-81,&unchanged,GH_STATIC_BINARY32_REFERENCE,&frontier)==GH_EFFECT_UNSUPPORTED&&unchanged==123&&frontier==0x3a1c78);C(gh_effect_exp_reference_003a1c78(0,&unchanged,0,&frontier)==GH_EFFECT_UNSUPPORTED&&unchanged==123);if(!dump)puts("PASS originalSDK bounded exp4096inputs x4FEnv; unknown domains atomicstrict, no libc exp; independentraw oracle required separately");return 0;}
