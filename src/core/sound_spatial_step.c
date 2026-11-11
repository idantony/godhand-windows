/* Recovered actual2CA518 chain, MIT LICENSE.upstream. PrimarypatchedELF
 * evidence: analysis/sound-spatial-step-evidence.json. Nochip/emulatorruntime. */
#include "sound_spatial_step.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
_Static_assert(offsetof(gh_boot_work,system_flags)==0x58c,"747A2C Work58C");
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float number(uint32_t n){float f;memcpy(&f,&n,4);return f;}
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
static float add(float a,float b){volatile float r=a+b;return r;}
static float sub(float a,float b){volatile float r=a-b;return r;}
static float mul(float a,float b){volatile float r=a*b;return r;}
static float divide(float a,float b){volatile float r=a/b;return r;}
static gh_sound_scene_status fail(uint32_t*f,uint32_t pc,gh_sound_scene_status e){if(f)*f=pc;return e;}
static void cross(float*out,const float*a,const float*b){out[0]=sub(mul(a[1],b[2]),mul(a[2],b[1]));out[1]=sub(mul(a[2],b[0]),mul(a[0],b[2]));out[2]=sub(mul(a[0],b[1]),mul(a[1],b[0]));}
static int normalize(float*v){float n=add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]));float length=sqrtf(n);if(!(length>0)||!isfinite(length))return 0;float q=divide(1,length);for(unsigned i=0;i<3;i++)v[i]=mul(v[i],q);return 1;}
gh_sound_scene_status gh_sound_listener_update_002cc150(gh_sound_control*s,const gh_boot_work*w,const gh_game_init_state*init,const gh_game_pl_camera*camera,const gh_title_player_view*player,gh_camera_numeric_policy policy,uint32_t*f){
 if(f)*f=0;if(!s||!s->initialized||!w||!init)return GH_SOUND_SCENE_ARGUMENT;
 uint32_t flags=word((const uint8_t*)w+0x5e4);s->listener_mode110=(flags&0x20080040u)?0:1;
 int effective=player?s->listener_mode110:0;
 if(!effective){
  if(!camera||!camera->constructed||!camera->identity||init->active_camera!=camera->identity)return fail(f,0x2cc150,GH_SOUND_SCENE_UNSUPPORTED);
  for(unsigned i=0;i<4;i++)s->listener_position_c0[i]=word(camera->fields+0x210+i*4);
  for(unsigned i=0;i<16;i++)s->listener_matrix_d0[i]=word(camera->fields+0x140+i*4);
  return GH_SOUND_SCENE_OK;
 }
 if(!player->bytes||player->size<GH_TITLE_PLAYER_BYTES||player->alias_f0!=player->bytes+0xb0)return GH_SOUND_SCENE_ARGUMENT;
 if(policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(f,0x30a2b0,GH_SOUND_SCENE_UNSUPPORTED);
 fenv_t env;if(feholdexcept(&env))return fail(f,0x2cc150,GH_SOUND_SCENE_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x2cc150,GH_SOUND_SCENE_UNSUPPORTED);}
 const float offset[]={0,1.5f,-.1f,1};float position[4],aim[3],z[3],x[3],y[3],matrix[16]={0};int good=1;
 for(unsigned r=0;r<4;r++){float v=mul(number(word(player->bytes+0x80+r*4)),offset[0]);for(unsigned k=1;k<4;k++)v=add(v,mul(number(word(player->bytes+0x80+(k*4+r)*4)),offset[k]));position[r]=v;if(!isfinite(v))good=0;}
 for(unsigned i=0;i<3;i++){aim[i]=number(word(player->alias_f0+i*4));if(i==1)aim[i]=add(aim[i],1.5f);z[i]=sub(aim[i],position[i]);if(!isfinite(aim[i]))good=0;}
 const float up[]={0,1,0};cross(x,up,z);if(!normalize(x)||!normalize(z))good=0;cross(y,z,x);
 if(good){matrix[15]=1;for(unsigned r=0;r<3;r++){matrix[r*4]=x[r];matrix[r*4+1]=y[r];matrix[r*4+2]=z[r];}
  for(unsigned r=0;r<3;r++)matrix[12+r]=-add(add(mul(matrix[r],position[0]),mul(matrix[4+r],position[1])),mul(matrix[8+r],position[2]));
  for(unsigned i=0;i<16;i++)if(!isfinite(matrix[i]))good=0;
 }
 if(fesetenv(&env))return fail(f,0x2cc150,GH_SOUND_SCENE_UNSUPPORTED);if(!good)return fail(f,0x30a8c8,GH_SOUND_SCENE_UNSUPPORTED);
 for(unsigned i=0;i<4;i++)s->listener_position_c0[i]=bits(position[i]);for(unsigned i=0;i<16;i++)s->listener_matrix_d0[i]=bits(matrix[i]);return GH_SOUND_SCENE_OK;
}
gh_sound_scene_status gh_sound_spatial_step_002ca518(gh_sound_scene*s,const gh_boot_work*w,const gh_game_init_state*init,const gh_game_pl_camera*camera,const gh_title_player_view*player,gh_camera_numeric_policy policy,uint32_t*f){
 if(f)*f=0;if(!s||!s->startup_bound||!s->control||!s->control->initialized||!w)return GH_SOUND_SCENE_ARGUMENT;
 gh_sound_scene_status e=gh_sound_listener_update_002cc150(s->control,w,init,camera,player,policy,f);if(e)return e;
 gh_sound_control*c=s->control;
 c->previous_mask54=c->event_mask50;c->flags_ac=c->flags_b0;c->event_mask50=c->event_mask58;
 s->ratios74[0]=s->fade_ratio_bits;s->ratios88[0]=s->default_ratio_bits;
 if(s->bmh.bytes)return fail(f,0x2d22f0,GH_SOUND_SCENE_UNSUPPORTED);
 if(!(w->system_flags&0x80000000u)&&s->ts.bytes&&c->ts_head2c&&player)return fail(f,0x2c8ad8,GH_SOUND_SCENE_UNSUPPORTED);
 /* Actual2CB200 isJRra/NOP. Originvoices areNOTiteratedorflushed by2CA518;
  * theirONErealengine isadvanced at the separateoriginalsoundframe boundary. */
 return GH_SOUND_SCENE_OK;
}
