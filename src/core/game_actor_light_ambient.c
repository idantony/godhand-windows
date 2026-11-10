#include "game_actor_light_ambient.h"
#include <math.h>
#include <fenv.h>
#include <string.h>
static int normal_or_zero(float n){return isfinite(n)&&(n==0||fpclassify(n)==FP_NORMAL);}
gh_actor_model_status gh_actor_ambient_fast_002d8008(const gh_game_world_light*s,
 void*normal,void*color,const void*unused_position,const float tint[4],
 gh_static_transform_policy policy,uint32_t*pc){
 (void)unused_position;uint32_t local=0;if(!pc)pc=&local;*pc=0;
 if(!s||!s->constructed||!normal||!color||!tint||policy!=GH_STATIC_BINARY32_REFERENCE)return GH_ACTOR_MODEL_ARGUMENT;
 float ambient[4],t[4],result[4];memcpy(ambient,s->scalar+0x30,16);memcpy(t,tint,16);
 fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;
 if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}
 gh_actor_model_status e=GH_ACTOR_MODEL_OK;
 for(unsigned k=0;k<4;k++){
  if(!normal_or_zero(ambient[k])||!normal_or_zero(t[k])){e=GH_ACTOR_MODEL_UNSUPPORTED;break;}
  volatile float n=ambient[k]*t[k];result[k]=n;
  if(!normal_or_zero(result[k])){e=GH_ACTOR_MODEL_UNSUPPORTED;break;}
 }
 if(fesetenv(&env))return GH_ACTOR_MODEL_UNSUPPORTED;
 if(e){*pc=0x30a4d0;return e;}
 const uint32_t one=0x3f800000;memcpy((uint8_t*)normal+0x3c,&one,4);memcpy((uint8_t*)color+0x30,result,16);
 return GH_ACTOR_MODEL_OK;
}
