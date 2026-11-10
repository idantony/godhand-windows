#include "game_scene_at_geometry.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static float number(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static int begin(fenv_t*e){if(feholdexcept(e))return 0;if(fesetround(FE_TONEAREST)){(void)fesetenv(e);return 0;}return 1;}
gh_scene_at_status gh_scene_at_apply_world_001508d8(const float*m,const float*p,float*out){if(!m||!p||!out)return GH_AT_ARGUMENT;for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))return GH_AT_UNSUPPORTED;for(unsigned i=0;i<3;i++)if(!isfinite(p[i]))return GH_AT_UNSUPPORTED;fenv_t e;if(!begin(&e))return GH_AT_BOUNDS;float v[4];int good=1;for(unsigned i=0;i<4;i++){float x=mul(m[i],p[0]);x=add(x,mul(m[4+i],p[1]));x=add(x,mul(m[8+i],p[2]));v[i]=add(x,m[i+12]);if(!isfinite(v[i]))good=0;}if(fesetenv(&e))return GH_AT_BOUNDS;if(!good)return GH_AT_UNSUPPORTED;memcpy(out,v,sizeof(v));return GH_AT_OK;}
static int quad(const uint8_t*s,const float*p){float ax=number(s+0x10),az=number(s+0x14),dx=sub(p[0],ax),dz=sub(p[2],az),bx=sub(number(s+0x18),ax),bz=sub(number(s+0x1c),az),cx=sub(number(s+0x20),ax),cz=sub(number(s+0x24),az),ex=sub(number(s+0x28),ax),ez=sub(number(s+0x2c),az);return mul(bx,dz)<=mul(bz,dx)&&mul(ez,dx)<=mul(ex,dz)&&mul(sub(bz,cz),sub(dx,cx))<=mul(sub(bx,cx),sub(dz,cz))&&mul(sub(ex,cx),sub(dz,cz))<=mul(sub(ez,cz),sub(dx,cx));}
gh_scene_at_status gh_scene_at_region_contains_001f83e8(const uint8_t*s,const float*p,int*out){if(!s||!p||!out)return GH_AT_ARGUMENT;if(s[1]!=1&&s[1]!=2){*out=0;return GH_AT_OK;}for(unsigned i=0;i<3;i++)if(!isfinite(p[i]))return GH_AT_UNSUPPORTED;for(unsigned i=4;i<0x30;i+=4)if(!isfinite(number(s+i)))return GH_AT_UNSUPPORTED;fenv_t env;if(!begin(&env))return GH_AT_BOUNDS;float base=number(s+4),height=number(s+8);int inside=0;if(base<=p[1]&&p[1]<=add(base,height)){if(s[1]==1)inside=quad(s,p);else{float dx=sub(p[0],number(s+0x10)),dz=sub(p[2],number(s+0x14));inside=sqrtf(add(mul(dx,dx),mul(dz,dz)))<number(s+0xc);}}if(fesetenv(&env))return GH_AT_BOUNDS;*out=inside;return GH_AT_OK;}
