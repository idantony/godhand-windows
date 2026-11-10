#include "game_cloth_collision.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static float rf(const uint8_t*p){float f;memcpy(&f,p,4);return f;}
static uint32_t u32(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float add(float a,float b){volatile float f=a+b;return f;}
static float mul(float a,float b){volatile float f=a*b;return f;}
static float divide(float a,float b){volatile float f=a/b;return f;}
static float dot(const float*a,const float*b){return add(add(mul(a[0],b[0]),mul(a[1],b[1])),mul(a[2],b[2]));}
static float length(const float*v){volatile float f=sqrtf(dot(v,v));return f;}
static gh_cloth_status body(const gh_cloth_geometry*g,float*a,float*b,uint32_t*f){
 float delta[3],unit[3];for(unsigned k=0;k<3;k++)delta[k]=add(a[k],-b[k]);if(delta[0]==0&&delta[1]==0&&delta[2]==0)delta[1]=1;float len=length(delta),square=dot(delta,delta);if(!isfinite(len)||!isfinite(square)||!(square>0)){*f=0x10753c;return GH_CLOTH_NUMERIC;}float invlen=divide(1,len),inverse=divide(1,square);for(unsigned k=0;k<3;k++)unit[k]=mul(delta[k],invlen);
 for(unsigned i=0;i<g->count;i++){const uint8_t*r=g->records[i];if(u32(r)){*f=0x107808;return GH_CLOTH_UNSUPPORTED;}float center[3],fromSecond[3],fromFirst[3];memcpy(center,r+0x90,12);float radius=rf(r+0xb0),radius2=mul(radius,radius);for(unsigned k=0;k<3;k++){fromSecond[k]=add(center[k],-b[k]);fromFirst[k]=add(center[k],-a[k]);}
  float projection=dot(fromSecond,delta),factor=mul(projection,inverse),away[3];for(unsigned k=0;k<3;k++)away[k]=add(add(mul(unit[k],factor),b[k]),-center[k]);float distance2=dot(away,away);
  if(distance2<radius2&&(dot(fromSecond,fromSecond)<=radius2||dot(fromFirst,fromFirst)<=radius2||(projection>=0&&projection<square))){float penetration=add(radius,-sqrtf(distance2));if(away[0]==0&&away[1]==0&&away[2]==0)away[1]=1;float awayLength=length(away);if(!(awayLength>0)||!isfinite(awayLength)){*f=0x107740;return GH_CLOTH_NUMERIC;}float scale=divide(1,awayLength);for(unsigned k=0;k<3;k++){float shift=mul(mul(away[k],scale),penetration);a[k]=add(a[k],shift);b[k]=add(b[k],shift);}}
  for(unsigned k=0;k<3;k++)if(!isfinite(center[k])||!isfinite(a[k])||!isfinite(b[k])){*f=0x107bd4;return GH_CLOTH_NUMERIC;}if(!isfinite(radius)||!isfinite(distance2)||!isfinite(radius2)){*f=0x107bd4;return GH_CLOTH_NUMERIC;}
 }
 return GH_CLOTH_OK;
}
gh_cloth_status gh_cloth_segment_body_00107360(const gh_cloth_geometry*g,float*first,float*second,gh_static_transform_policy policy,uint32_t*f){
 if(!first||!second||!f)return GH_CLOTH_ARGUMENT;*f=0;if(!g||!g->count)return GH_CLOTH_OK;if(g->count>12)return GH_CLOTH_BOUNDS;if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x107360;return GH_CLOTH_UNSUPPORTED;}float a[3],b[3];memcpy(a,first,12);memcpy(b,second,12);for(unsigned k=0;k<3;k++)if(!isfinite(a[k])||!isfinite(b[k])){*f=0x107360;return GH_CLOTH_NUMERIC;}fenv_t env;if(feholdexcept(&env))return GH_CLOTH_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CLOTH_NUMERIC;}gh_cloth_status e=body(g,a,b,f);if(fesetenv(&env))return GH_CLOTH_NUMERIC;if(!e){memcpy(second,b,12);memcpy(first,a,12);}return e;
}
