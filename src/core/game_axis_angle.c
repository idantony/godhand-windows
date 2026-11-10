#include "game_axis_angle.h"
#include "sdk_rotation_trig.h"
#include "game_camera.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static float mul(float a,float b){volatile float f=a*b;return f;}
static float add(float a,float b){volatile float f=a+b;return f;}
static gh_actor_matrix_status rotation(unsigned axis,float angle,float*out,gh_static_transform_policy policy,uint32_t*f){float sine,cosine;gh_actor_matrix_status e=gh_sdk_rotation_sincos_reference_0030a5d0(angle,&sine,&cosine,policy,f);if(e)return e;memset(out,0,64);out[15]=1;out[axis*5]=1;if(axis==0){out[5]=out[10]=cosine;out[6]=sine;out[9]=add(0,-sine);}else if(axis==1){out[0]=out[10]=cosine;out[2]=add(0,-sine);out[8]=sine;}else{out[0]=out[5]=cosine;out[1]=sine;out[4]=add(0,-sine);}return GH_ACTOR_MATRIX_OK;}
static gh_actor_matrix_status body(float angle,const float*axis,const float*pivot,float*out,gh_static_transform_policy policy,uint32_t*f){
 float elevation,azimuth=0,aligned[16],temporary[16],r[16],inverse[16];
 if(axis[0]==0&&axis[2]==0){if(gh_game_angle_001f8180(axis[1],0,&elevation,GH_CAMERA_NATIVE_BINARY32_REFERENCE,f))return GH_ACTOR_MATRIX_UNSUPPORTED;if(rotation(2,-elevation,aligned,policy,f))return GH_ACTOR_MATRIX_UNSUPPORTED;}
 else{if(gh_game_angle_001f8180(axis[2],axis[0],&azimuth,GH_CAMERA_NATIVE_BINARY32_REFERENCE,f))return GH_ACTOR_MATRIX_UNSUPPORTED;if(rotation(1,azimuth,aligned,policy,f))return GH_ACTOR_MATRIX_UNSUPPORTED;float square=add(mul(axis[0],axis[0]),mul(axis[2],axis[2])),horizontal=sqrtf(square);if(!isfinite(horizontal)){*f=0x15afe0;return GH_ACTOR_MATRIX_UNSUPPORTED;}if(gh_game_angle_001f8180(axis[1],horizontal,&elevation,GH_CAMERA_NATIVE_BINARY32_REFERENCE,f))return GH_ACTOR_MATRIX_UNSUPPORTED;if(rotation(2,-elevation,r,policy,f)||gh_actor_matrix_product_00147548(r,aligned,temporary,policy,f))return GH_ACTOR_MATRIX_UNSUPPORTED;memcpy(aligned,temporary,64);}
 for(unsigned col=0;col<4;col++)for(unsigned row=0;row<4;row++)inverse[col*4+row]=aligned[row*4+col];
 if(rotation(0,angle,r,policy,f)||gh_actor_matrix_product_00147548(r,aligned,temporary,policy,f)||gh_actor_matrix_product_00147548(inverse,temporary,out,policy,f))return GH_ACTOR_MATRIX_UNSUPPORTED;
 for(unsigned row=0;row<3;row++){float transformed=mul(out[row],pivot[0]);transformed=add(transformed,mul(out[4+row],pivot[1]));transformed=add(transformed,mul(out[8+row],pivot[2]));transformed=add(transformed,mul(out[12+row],pivot[3]));out[12+row]=add(pivot[row],-transformed);}out[15]=1;for(unsigned i=0;i<16;i++)if(!isfinite(out[i])){*f=0x15af30;return GH_ACTOR_MATRIX_UNSUPPORTED;}return GH_ACTOR_MATRIX_OK;
}
gh_actor_matrix_status gh_axis_angle_matrix_0015af30(float angle,const float*axis,const void*pivotBytes,float*out,gh_static_transform_policy policy,uint32_t*f){
 if(!axis||!pivotBytes||!out||!f)return GH_ACTOR_MATRIX_ARGUMENT;*f=0;if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x15af30;return GH_ACTOR_MATRIX_UNSUPPORTED;}float v[3],pivot[4],result[16];memcpy(v,axis,12);memcpy(pivot,pivotBytes,16);if(!isfinite(angle)){*f=0x15af30;return GH_ACTOR_MATRIX_UNSUPPORTED;}for(unsigned i=0;i<3;i++)if(!isfinite(v[i])){*f=0x15af30;return GH_ACTOR_MATRIX_UNSUPPORTED;}for(unsigned i=0;i<4;i++)if(!isfinite(pivot[i])){*f=0x15af30;return GH_ACTOR_MATRIX_UNSUPPORTED;}fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MATRIX_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ACTOR_MATRIX_UNSUPPORTED;}gh_actor_matrix_status e=body(angle,v,pivot,result,policy,f);if(fesetenv(&env)){*f=0x15af30;return GH_ACTOR_MATRIX_UNSUPPORTED;}if(!e)memcpy(out,result,64);return e;
}
