#include "game_cloth_point.h"
#include "game_axis_angle.h"
#include "game_camera.h"
#include "game_ee_acos.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static float rf(const uint8_t*p){float f;memcpy(&f,p,4);return f;}
static float add(float a,float b){volatile float f=a+b;return f;}
static float mul(float a,float b){volatile float f=a*b;return f;}
static float divide(float a,float b){volatile float f=a/b;return f;}
static float square(const float*v){return add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]));}
static void normalize(float*v){float length=sqrtf(square(v));if(length>0){float scale=divide(1,length);for(unsigned k=0;k<3;k++)v[k]=mul(v[k],scale);}else v[0]=v[1]=v[2]=0;}
static gh_cloth_status body(const gh_cloth_geometry*g,float*point,const float*anchor,gh_player_shared_vector*pivot,gh_static_transform_policy policy,int*hit,uint32_t*f){
 float initial[3];for(unsigned k=0;k<3;k++)initial[k]=add(point[k],-anchor[k]);float initialSquare=square(initial),arm=sqrtf(initialSquare);if(!isfinite(arm)){*f=0x106838;return GH_CLOTH_NUMERIC;}
 for(unsigned i=0;i<g->count;i++){const uint8_t*r=g->records[i];float center[3],away[3];memcpy(center,r+0x90,12);float radius=rf(r+0xb0);if(!isfinite(radius)){*f=0x1068c8;return GH_CLOTH_NUMERIC;}for(unsigned k=0;k<3;k++){if(!isfinite(center[k])){*f=0x1068c8;return GH_CLOTH_NUMERIC;}away[k]=add(point[k],-center[k]);}
  if(square(away)<mul(radius,radius)){float direction[3];for(unsigned k=0;k<3;k++)direction[k]=add(center[k],-anchor[k]);float distance=sqrtf(square(direction));float numerator=add(add(mul(radius,radius),-mul(arm,arm)),-mul(distance,distance)),denominator=mul(mul(arm,-2),distance),cosine=divide(numerator,denominator);float angle;if(gh_game_ee_acos_reference_001f8140(cosine,&angle,GH_CAMERA_NATIVE_BINARY32_REFERENCE,f))return GH_CLOTH_UNSUPPORTED;
   if(angle<=0.01f||angle>=3.1315928f){normalize(direction);float length=add(radius,1);for(unsigned k=0;k<3;k++)point[k]=add(center[k],mul(direction[k],length));}
   else{float armVector[3];for(unsigned k=0;k<3;k++)armVector[k]=add(point[k],-anchor[k]);float axis[3]={add(mul(direction[1],armVector[2]),-mul(direction[2],armVector[1])),add(mul(direction[2],armVector[0]),-mul(direction[0],armVector[2])),add(mul(direction[0],armVector[1]),-mul(direction[1],armVector[0]))},matrix[16];if(gh_axis_angle_matrix_0015af30(angle,axis,pivot->bytes,matrix,policy,f))return GH_CLOTH_UNSUPPORTED;normalize(direction);for(unsigned k=0;k<3;k++)direction[k]=mul(direction[k],sqrtf(initialSquare));for(unsigned row=0;row<3;row++){float value=mul(matrix[row],direction[0]);value=add(value,mul(matrix[4+row],direction[1]));value=add(value,mul(matrix[8+row],direction[2]));value=add(value,matrix[12+row]);point[row]=add(anchor[row],value);}}
   for(unsigned k=0;k<3;k++)if(!isfinite(point[k])){*f=0x106b60;return GH_CLOTH_NUMERIC;}*hit=1;
  }
 }
 return GH_CLOTH_OK;
}
gh_cloth_status gh_cloth_point_body_001067b8(const gh_cloth_geometry*g,float*point,const float*anchor,gh_player_shared_vector*pivot,gh_static_transform_policy policy,int*out,uint32_t*f){
 if(!point||!anchor||!out||!f)return GH_CLOTH_ARGUMENT;*out=0;*f=0;if(!g||!g->count)return GH_CLOTH_OK;if(g->count>12)return GH_CLOTH_BOUNDS;if(!pivot||!pivot->initialized)return GH_CLOTH_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x1067b8;return GH_CLOTH_UNSUPPORTED;}float value[3],origin[3];memcpy(value,point,12);memcpy(origin,anchor,12);for(unsigned k=0;k<3;k++)if(!isfinite(value[k])||!isfinite(origin[k])){*f=0x1067b8;return GH_CLOTH_NUMERIC;}fenv_t env;if(feholdexcept(&env))return GH_CLOTH_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CLOTH_NUMERIC;}int hit=0;gh_cloth_status e=body(g,value,origin,pivot,policy,&hit,f);if(fesetenv(&env))return GH_CLOTH_NUMERIC;if(!e){memcpy(point,value,12);*out=hit;}return e;
}
