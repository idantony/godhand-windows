#include "game_camera.h"
#include <math.h>
#include <fenv.h>
#include <string.h>
static float f(uint32_t bits){float x;memcpy(&x,&bits,4);return x;}
static float add(float a,float b){volatile float x=a+b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float divide(float a,float b){volatile float x=a/b;return x;}
static void w32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void wf(uint8_t*p,float x){uint32_t n;memcpy(&n,&x,4);w32(p,n);}
/* Actual1F8180 eightoddterms; 42C350..370 exactcoefficientbits. */
static float atan_original(float y,float x){
 static const uint32_t c[]={0x3f7ffff5,0xbeaaa61c,0x3e4c40a6,0xbe0e6c63,0x3dc577df,0xbd6501c4,0x3cb31652,0xbb84d7e7};
 float ay=fabsf(y),ax=fabsf(x),pi=f(0x40490fdb);
 if(ax<f(0x3727c5ac))return ay<f(0x3727c5ac)?0:(y<0?f(0xbfc90fdb):f(0x3fc90fdb));
 if(ay<f(0x3727c5ac))return x<0?(y<0?-pi:pi):0;
 float r=divide(add(ay,-ax),add(ay,ax)),r2=mul(r,r),power=r,result=mul(r,f(c[0]));
 for(unsigned i=1;i<8;i++){power=mul(power,r2);result=add(result,mul(power,f(c[i])));}
 result=add(result,f(0x3f490fdb));return x<0?(y<0?add(result,-pi):add(pi,-result)):(y<0?-result:result);
}
/* SDK30A7B8->30A5D0 finiteYrotation; sinpolynomial3C3C90 then
 * signed sqrt(1-sin^2), matching the originalinstructionflow. */
static void y_sincos(float angle,float *sn,float *cs){
 static const uint32_t c[]={0x362e9c14,0xb94fb21f,0x3c08873e,0xbe2aaaa4};
 int negative=angle<0;float t=negative?add(f(0x3fc90fdb),angle):add(f(0x3fc90fdb),-angle),t2=mul(t,t),p[4];
 for(unsigned i=0;i<4;i++)p[i]=mul(mul(f(c[i]),t),t2);
 for(unsigned i=0;i<3;i++)p[i]=mul(p[i],t2);float s=add(t,p[3]);
 for(unsigned i=0;i<2;i++)p[i]=mul(p[i],t2);s=add(s,p[2]);p[0]=mul(p[0],t2);s=add(add(s,p[1]),p[0]);
 *cs=s;*sn=sqrtf(add(1,-mul(s,s)));if(negative)*sn=-*sn;
}
static gh_camera_status reset_fields(gh_game_pl_camera*s){
 fenv_t env;if(s->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE){s->failed_entry=0x1f8180;return GH_CAMERA_UNSUPPORTED;}
 if(feholdexcept(&env)){s->failed_entry=0x1f8180;return GH_CAMERA_NUMERIC;}
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);s->failed_entry=0x1f8180;return GH_CAMERA_NUMERIC;}
 /*138240 fixedtarget (0,0,0), eye=(40333333,40066666,40333333).
  *138358 subtracts fullvectors, computesyaw, rotatesaboutY(-yaw), thenpitch. */
 float x=f(0x40333333),y=f(0x40066666),z=f(0x40333333),yaw=atan_original(x,z),sn,cs;
 y_sincos(-yaw,&sn,&cs);
 float rotated_y=add(add(add(mul(0,x),mul(1,y)),mul(0,z)),0);
 float rotated_z=add(add(add(mul(-sn,x),mul(0,y)),mul(cs,z)),0);
 float pitch=-atan_original(rotated_y,rotated_z);
 if(!isfinite(pitch)||!isfinite(yaw)){(void)fesetenv(&env);s->failed_entry=0x1f8180;return GH_CAMERA_NUMERIC;}
 if(fesetenv(&env)){s->failed_entry=0x1f8180;return GH_CAMERA_NUMERIC;}
 uint8_t*p=s->fields;w32(p+0x200,0);w32(p+0x204,0);w32(p+0x208,0);
 w32(p+0x210,0x40333333);w32(p+0x214,0x40066666);w32(p+0x218,0x40333333);
 wf(p+0x220,pitch);wf(p+0x224,yaw);w32(p+0x228,0);
 /*147728 writesonlydeadlocal64-byte matrix, neverreadaftercall. Its SDK
  * calleesonlywriteoutputandVUworkingregisters, no persistent/nativeeffect. */
 w32(p+0x240,0);w32(p+0x244,0x3f800000);w32(p+0x248,0);w32(p+0x1c0,0);w32(p+0x250,0x41f80000);
 p[0x264]=0;w32(p+0x274,0);w32(p+0x268,0);w32(p+0x270,0);w32(p+0x26c,0);p[0x278]=0;
 w32(p+0x260,0);w32(p+0x254,0);w32(p+0x25c,0);w32(p+0x258,0);s->failed_entry=0;return GH_CAMERA_OK;
}
gh_camera_status gh_game_camera_construct_pl_00139bc8(gh_game_pl_camera*s,gh_game_camera*identity,gh_camera_numeric_policy policy){
 if(!s||!identity)return GH_CAMERA_ARGUMENT;if(s->constructed)return GH_CAMERA_BUSY;
 s->identity=identity;s->policy=policy;if(policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE){s->failed_entry=0x1f8180;return GH_CAMERA_UNSUPPORTED;}
 memset(s->fields,0,sizeof(s->fields));
 static const unsigned vectors[]={0x1d0,0x1e0,0x200,0x210,0x220,0x230,0x240,0x280,0x290,0x2a0,0x300,0x320,0x330,0x340};
 for(unsigned i=0;i<sizeof(vectors)/sizeof(*vectors);i++)w32(s->fields+vectors[i]+12,0x3f800000);
 for(unsigned matrix=0;matrix<7;matrix++)for(unsigned i=0;i<4;i++)w32(s->fields+matrix*0x40+i*20,0x3f800000);
 gh_camera_status status=reset_fields(s);if(status)return status;
 static const unsigned derived[]={0x370,0x380,0x390,0x3b0,0x3c0,0x3d0,0x400,0x410,0x440,0x450,0x480,0x490,0x4c0,0x4d0,0x500};
 for(unsigned i=0;i<sizeof(derived)/sizeof(*derived);i++)w32(s->fields+derived[i]+12,0x3f800000);
 w32(s->fields+0x35c,0x41dbb0);s->constructed=1;return GH_CAMERA_OK;
}
gh_camera_status gh_game_camera_reset_00138210(gh_game_pl_camera*s,const gh_game_camera*active){
 if(!s||!active||!s->constructed)return GH_CAMERA_ARGUMENT;
 if(active!=s->identity||u32(s->fields+0x35c)!=0x41dbb0){s->failed_entry=0x138210;return GH_CAMERA_UNSUPPORTED;}
 return reset_fields(s);
}
