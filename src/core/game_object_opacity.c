/* Actual patchedSLUS2A87E8: source-level numeric reference. */
#include "game_object_opacity.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static float add(float a,float b){volatile float n=a+b;return n;}
static float sub(float a,float b){volatile float n=a-b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float distance(float x,float z,float px,float pz){float a=sub(x,px),b=sub(z,pz);return sqrtf(add(mul(a,a),mul(b,b)));}
static gh_camera_status fail(uint32_t*f,uint32_t pc,gh_camera_status e){if(f)*f=pc;return e;}
static void publish(uint8_t*p,float alpha,int transparent){memcpy(p+0x24c,&alpha,4);uint32_t flags=word(p+0x250);put(p+0x250,transparent?flags|16u:flags&~16u);}
gh_camera_status gh_game_object_opacity_002a87e8(const gh_title_player_view*v,const float*override,const gh_boot_work*w,const gh_object_opacity_environment*b,gh_camera_numeric_policy policy,uint32_t*f){
 if(f)*f=0;if(!v||!v->bytes||v->size<0x300||!w)return GH_CAMERA_ARGUMENT;uint8_t*p=v->bytes;
 if(word((const uint8_t*)w+0x5e4)&0x60000040u){publish(p,1,0);return GH_CAMERA_OK;}
 if(!b||!b->init)return fail(f,0x2db6b0,GH_CAMERA_UNSUPPORTED);
 if(!b->init->active_camera)return GH_CAMERA_OK; /* original leaves opacity unchanged */
 if(!b->camera||!b->camera->constructed||b->camera->identity!=&b->init->camera||b->init->active_camera!=b->camera->identity)return fail(f,0x2a884c,GH_CAMERA_UNSUPPORTED);
 if(!b->player_binding_known||!b->current_player||!b->current_player->bytes||!b->current_player->alias_f0)return fail(f,0x2db6b0,GH_CAMERA_UNSUPPORTED);
 const float*pos=override?override:(const float*)NULL;float self[3];if(!pos){if(!v->alias_f0)return fail(f,0x2a885c,GH_CAMERA_ARGUMENT);memcpy(self,v->alias_f0,12);pos=self;}
 if(policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(f,0x2a8888,GH_CAMERA_UNSUPPORTED);
 float camera[3],player[3];memcpy(camera,b->camera->fields+0x210,12);memcpy(player,b->current_player->alias_f0,12);
 for(unsigned i=0;i<3;i++)if(!isfinite(camera[i])||!isfinite(player[i])||!isfinite(pos[i]))return fail(f,0x2a8888,GH_CAMERA_NUMERIC);
 fenv_t env;if(feholdexcept(&env))return fail(f,0x2a8888,GH_CAMERA_NUMERIC);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x2a8888,GH_CAMERA_NUMERIC);}
 float pd=distance(camera[0],camera[2],player[0],player[2]),sd=distance(camera[0],camera[2],pos[0],pos[2]),t=sub(sd,2),alpha=mul(t,mul(t,t));unsigned kind=p[0x2fe]|(unsigned)p[0x2ff]<<8;
 if(kind>=0x200&&kind<0x300&&add(pos[1],1.7f)<camera[1])alpha=1;
 int valid=isfinite(pd)&&isfinite(sd)&&isfinite(alpha),transparent=0;
 if(valid){if(pd<=sd||alpha>=1)alpha=1;else if(alpha<.15f)alpha=0;else transparent=1;}
 if(fesetenv(&env))return fail(f,0x2a8888,GH_CAMERA_NUMERIC);if(!valid)return fail(f,0x2a88dc,GH_CAMERA_NUMERIC);publish(p,alpha,transparent);return GH_CAMERA_OK;
}
