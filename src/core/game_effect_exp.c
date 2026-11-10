#include "game_effect_exp.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static float value(uint32_t bits){float n;memcpy(&n,&bits,4);return n;}
static uint32_t encoding(float n){uint32_t bits;memcpy(&bits,&n,4);return bits;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float sub(float a,float b){volatile float n=a-b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float divide(float a,float b){volatile float n=a/b;return n;}
static float body(float x){
 uint32_t bits=encoding(x),absolute=bits&0x7fffffffu;unsigned sign=bits>>31;if(absolute<=0x317fffffu)return add(x,1.0f);
 int32_t k=0;float hi=0,lo=0;
 if(absolute>0x3eb17218u){if(absolute<=0x3f851591u){hi=sub(x,value(sign?0xbf317180u:0x3f317180u));lo=value(sign?0xb717f7d1u:0x3717f7d1u);k=1-(int32_t)(2*sign);}else {k=(int32_t)add(mul(x,value(0x3fb8aa3bu)),value(sign?0xbf000000u:0x3f000000u));float integer=(float)k;hi=sub(x,mul(integer,value(0x3f317180u)));lo=mul(integer,value(0x3717f7d1u));}x=sub(hi,lo);}
 float square=mul(x,x),polynomial=add(mul(square,value(0x3331bb4cu)),value(0xb5ddea0eu));polynomial=add(mul(square,polynomial),value(0x388ab355u));polynomial=add(mul(square,polynomial),value(0xbb360b61u));polynomial=add(mul(square,polynomial),value(0x3e2aaaabu));float c=sub(x,mul(square,polynomial));
 if(!k)return sub(1.0f,sub(divide(mul(x,c),sub(c,2.0f)),x));float result=sub(1.0f,sub(sub(lo,divide(mul(x,c),sub(2.0f,c))),hi));uint32_t scaled=encoding(result)+((uint32_t)k<<23);return value(scaled);
}
gh_effect_status gh_effect_exp_reference_003a1c78(float input,float*out,gh_static_transform_policy policy,uint32_t*frontier){if(!out||!frontier)return GH_EFFECT_ARGUMENT;*frontier=0;if(policy!=GH_STATIC_BINARY32_REFERENCE||!isfinite(input)||input>0||input< -80.0f){*frontier=0x3a1c78;return GH_EFFECT_UNSUPPORTED;}fenv_t env;if(feholdexcept(&env)){*frontier=0x3a28f8;return GH_EFFECT_UNSUPPORTED;}if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);*frontier=0x3a28f8;return GH_EFFECT_UNSUPPORTED;}float result=body(input);int valid=isfinite(result)&&result>0;if(fesetenv(&env)){*frontier=0x3a28f8;return GH_EFFECT_UNSUPPORTED;}if(!valid){*frontier=0x3a28f8;return GH_EFFECT_UNSUPPORTED;}*out=result;return GH_EFFECT_OK;}
