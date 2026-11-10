#include "game_camera_shake.h"
#include "id_layout_rotation.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static float readf(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static void store(uint8_t*p,float x){memcpy(p,&x,4);}
static float add(float a,float b){volatile float x=a+b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static gh_camera_status stop(uint32_t*f,uint32_t pc,gh_camera_status e){if(f)*f=pc;return e;}
gh_camera_status gh_game_camera_shake_step_00138558(gh_game_pl_camera*c,uint32_t*f){if(f)*f=0;if(!c||!c->constructed)return GH_CAMERA_ARGUMENT;if(c->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return stop(f,0x138558,GH_CAMERA_UNSUPPORTED);if(!c->fields[0x278]&&!c->fields[0x264])return GH_CAMERA_OK;
 fenv_t env;if(feholdexcept(&env))return GH_CAMERA_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CAMERA_NUMERIC;}uint8_t result[0x530];memcpy(result,c->fields,sizeof(result));gh_camera_status e=GH_CAMERA_OK;
 const unsigned count[2]={0x278,0x264},amplitude[2]={0x268,0x254},decay[2]={0x270,0x25c},phase[2]={0x274,0x260},speed[2]={0x26c,0x258};
 for(unsigned i=0;i<2;i++)if(result[count[i]]){float a=readf(result+amplitude[i]),d=readf(result+decay[i]),p=readf(result+phase[i]),v=readf(result+speed[i]),next_phase=add(p,v),next_amplitude=mul(a,d),wrapped;if(!isfinite(a)||!isfinite(d)||!isfinite(p)||!isfinite(v)||!isfinite(next_amplitude)){e=stop(f,0x138558,GH_CAMERA_NUMERIC);break;}if(gh_id_angle_reference_001f7d48(next_phase,&wrapped)){e=stop(f,0x1f7d48,GH_CAMERA_UNSUPPORTED);break;}store(result+amplitude[i],next_amplitude);store(result+phase[i],wrapped);result[count[i]]=(uint8_t)(result[count[i]]-1u);}
 if(fesetenv(&env))return GH_CAMERA_NUMERIC;if(!e)memcpy(c->fields,result,sizeof(result));return e;
}
gh_camera_status gh_game_camera_shake_offsets_00138740(const gh_game_pl_camera*c,float*out,uint32_t*f){if(f)*f=0;if(!c||!c->constructed||!out)return GH_CAMERA_ARGUMENT;if(c->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return stop(f,0x138740,GH_CAMERA_UNSUPPORTED);fenv_t env;if(feholdexcept(&env))return GH_CAMERA_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CAMERA_NUMERIC;}float xy[2]={0,0};gh_camera_status e=GH_CAMERA_OK;
 const unsigned count[2]={0x264,0x278},phase[2]={0x260,0x274},amplitude[2]={0x254,0x268};for(unsigned i=0;i<2;i++)if(c->fields[count[i]]){float a=readf(c->fields+amplitude[i]),sn,cs;if(!isfinite(a)){e=stop(f,0x138740,GH_CAMERA_NUMERIC);break;}if(gh_id_sincos_reference(readf(c->fields+phase[i]),&sn,&cs)){e=stop(f,0x1f7ef0,GH_CAMERA_UNSUPPORTED);break;}xy[i]=mul(a,sn);if(!isfinite(xy[i])){e=stop(f,0x138740,GH_CAMERA_NUMERIC);break;}}
 if(fesetenv(&env))return GH_CAMERA_NUMERIC;if(!e)memcpy(out,xy,sizeof(xy));return e;
}
