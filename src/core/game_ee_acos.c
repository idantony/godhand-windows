/* PatchedELF1F8140, not a VU sqrt. Primary numeric-reference rule:
 * PCSX2 pin3f617368c23ea163900fa7ad84170aaba44d9720 pcsx2/FPU.cpp
 * SQRT_S334..344: negative normal -> sqrt(abs) plus EE invalid cause/sticky.
 * Consulted as semantics only, no implementation copied. */
#include "game_ee_acos.h"
#include <math.h>
#include <float.h>
#include <fenv.h>
#include <string.h>
static float mul(float a,float b){volatile float n=a*b;return n;}
static float sub(float a,float b){volatile float n=a-b;return n;}
gh_camera_status gh_game_ee_acos_reference_001f8140(float x,float*out,gh_camera_numeric_policy policy,uint32_t*f){
 if(f)*f=0;if(!out)return GH_CAMERA_ARGUMENT;
 if(policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE){if(f)*f=0x1f8140;return GH_CAMERA_UNSUPPORTED;}
 if(!isfinite(x)||(x!=0&&fabsf(x)<FLT_MIN)){if(f)*f=0x1f8140;return GH_CAMERA_NUMERIC;}
 fenv_t env;if(feholdexcept(&env)){if(f)*f=0x1f8140;return GH_CAMERA_NUMERIC;}if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);if(f)*f=0x1f8140;return GH_CAMERA_NUMERIC;}
 float square=mul(x,x),angle=0;gh_camera_status e=GH_CAMERA_OK;
 if(!isfinite(square))e=GH_CAMERA_NUMERIC;
 else{
  /* PinnedEE MUL flushes a subnormal product to signedzero. */
  if(square!=0&&square<FLT_MIN)square=0;
  float radicand=sub(1,square);uint32_t bits;memcpy(&bits,&radicand,4);
  /* EE SQRT sign-magnitude rule, not a clamp: even |x|>1 can yield
   * a nonzero angle. FCR31 invalid flags have no native owner here. */
  float root;if((bits&0x7f800000u)==0){bits&=0x80000000u;memcpy(&root,&bits,4);}else root=sqrtf(fabsf(radicand));
  e=gh_game_angle_001f8180(root,x,&angle,policy,f);
 }
 if(fesetenv(&env))e=GH_CAMERA_NUMERIC;if(e){if(f&&!*f)*f=0x1f8140;return e;}*out=angle;return GH_CAMERA_OK;
}
