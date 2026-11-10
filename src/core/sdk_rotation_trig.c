/* Semantic reconstruction from patchedELF SDK30A5D0, no runtimeVU interpreter. */
#include "sdk_rotation_trig.h"
#include <math.h>
#include <float.h>
#include <fenv.h>
#include <string.h>
static float bits(uint32_t n){float f;memcpy(&f,&n,4);return f;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static int normal(float n){return isfinite(n)&&(n==0||fabsf(n)>=FLT_MIN);}
gh_actor_matrix_status gh_sdk_rotation_sincos_reference_0030a5d0(float theta,float*s,float*c,gh_static_transform_policy policy,uint32_t*f){
 if(!s||!c||s==c||!f)return GH_ACTOR_MATRIX_ARGUMENT;*f=0;
 if(policy!=GH_STATIC_BINARY32_REFERENCE||!normal(theta)||fabsf(theta)>bits(0x40490fdb)){*f=0x30a5d0;return GH_ACTOR_MATRIX_UNSUPPORTED;}
 fenv_t saved;if(feholdexcept(&saved)){*f=0x30a5d0;return GH_ACTOR_MATRIX_UNSUPPORTED;}if(fesetround(FE_TONEAREST)){fesetenv(&saved);*f=0x30a5d0;return GH_ACTOR_MATRIX_UNSUPPORTED;}
 const uint32_t coeff[4]={0x362e9c14,0xb94fb21f,0x3c08873e,0xbe2aaaa4};
 float t=theta<0?add(bits(0x3fc90fdb),theta):add(bits(0x3fc90fdb),-theta),square=mul(t,t),v[4];
 /* ActualmaskedVF8 lanes produce t^9,t^7,t^5,t^3 respectively. */
 for(unsigned i=0;i<4;i++)v[i]=mul(mul(bits(coeff[i]),t),square);
 for(unsigned i=0;i<3;i++)v[i]=mul(v[i],square);float polynomial=add(t,v[3]);
 for(unsigned i=0;i<2;i++)v[i]=mul(v[i],square);polynomial=add(polynomial,v[2]);
 v[0]=mul(v[0],square);polynomial=add(polynomial,v[1]);polynomial=add(polynomial,v[0]);
 float radicand=add(1,-mul(polynomial,polynomial)),sine=0;
 gh_actor_matrix_status status=GH_ACTOR_MATRIX_OK;
 if(!normal(polynomial)||!normal(radicand)){status=GH_ACTOR_MATRIX_UNSUPPORTED;*f=0x30a5d0;}
 else {sine=sqrtf(fabsf(radicand));if(theta<0)sine=-sine;if(!normal(sine)){status=GH_ACTOR_MATRIX_UNSUPPORTED;*f=0x30a5d0;}}
 if(fesetenv(&saved)){*f=0x30a5d0;return GH_ACTOR_MATRIX_UNSUPPORTED;}
 if(!status){*s=sine;*c=polynomial;}return status;
}
