#include "game_actor_light_normalize.h"
#include <math.h>
#include <fenv.h>
#include <string.h>
#include <float.h>
static float mul(float a,float b){volatile float r=a*b;return r;}
static float add(float a,float b){volatile float r=a+b;return r;}
static int normal_or_zero(float n){return isfinite(n)&&(n==0||fpclassify(n)==FP_NORMAL);}
gh_actor_model_status gh_actor_light_normalize_0030a370(const float in[3],float out[4],
 gh_static_transform_policy policy,uint32_t *frontier){
 uint32_t local=0;if(!frontier)frontier=&local;*frontier=0;
 if(!in||!out||policy!=GH_STATIC_BINARY32_REFERENCE)return GH_ACTOR_MODEL_ARGUMENT;
 float source[3];memcpy(source,in,sizeof(source));
 fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;
 if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}
 gh_actor_model_status e=GH_ACTOR_MODEL_UNSUPPORTED;float result[4]={0,0,0,0};
 for(unsigned k=0;k<3;k++)if(!normal_or_zero(source[k]))goto done;
 float squares[3];for(unsigned k=0;k<3;k++){squares[k]=mul(source[k],source[k]);if(!normal_or_zero(squares[k])||(source[k]!=0&&squares[k]==0))goto done;}
 float xy=add(squares[0],squares[1]),sum=add(xy,squares[2]);if(!normal_or_zero(xy)||!normal_or_zero(sum))goto done;
 float length=sqrtf(sum);if(!normal_or_zero(length))goto done;
 float q;if(length==0)q=FLT_MAX;else{volatile float n=1/length;q=n;}
 if(!normal_or_zero(q))goto done;
 for(unsigned k=0;k<3;k++){result[k]=mul(source[k],q);if(!normal_or_zero(result[k]))goto done;}
 e=GH_ACTOR_MODEL_OK;
done:
 if(fesetenv(&env))e=GH_ACTOR_MODEL_UNSUPPORTED;
 if(e)*frontier=0x30a370;else memcpy(out,result,sizeof(result));
 return e;
}
