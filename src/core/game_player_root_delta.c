#include "game_player_root_delta.h"
#include "game_actor_matrix.h"
#include "game_scene_at_geometry.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static float num(const void*p){float n;memcpy(&n,p,4);return n;}
static gh_player_delta_status body(const gh_title_player_view*v,const gh_boot_work*w,int rotation,uint32_t*f){
 uint8_t*p=v->bytes;if(word((const uint8_t*)w+0x5e4)&0x20000000)return GH_PLAYER_DELTA_OK;
 if(rotation){float in[4],out[4];memcpy(in,p+0x100,16);for(unsigned i=0;i<3;i++){float delta=num(p+0x360+i*4);if(!isfinite(in[i])||!isfinite(delta))return GH_PLAYER_DELTA_NUMERIC;volatile float next=in[i]+delta;in[i]=next;}if(gh_actor_euler_adjust_001f7da0(in,out,GH_STATIC_BINARY32_REFERENCE,f))return GH_PLAYER_DELTA_UNSUPPORTED;memcpy(p+0x100,out,12);return GH_PLAYER_DELTA_OK;}
 if(!v->alias_f0)return GH_PLAYER_DELTA_ARGUMENT;
 uintptr_t start=(uintptr_t)p,alias=(uintptr_t)v->alias_f0;if(alias<start||alias-start>v->size||16>v->size-(alias-start))return GH_PLAYER_DELTA_ARGUMENT;
 float local[3],angles[3],matrix[16],world[4],next[3];for(unsigned i=0;i<3;i++){float delta=num(p+0x330+i*4),scale=num(p+0x110+i*4);if(!isfinite(delta)||!isfinite(scale))return GH_PLAYER_DELTA_NUMERIC;volatile float product=delta*scale;local[i]=product;}memcpy(angles,p+0x100,12);if(gh_actor_matrix_rotation_00147728(angles,0,matrix,GH_STATIC_BINARY32_REFERENCE,f))return GH_PLAYER_DELTA_UNSUPPORTED;if(gh_scene_at_apply_world_001508d8(matrix,local,world))return GH_PLAYER_DELTA_NUMERIC;for(unsigned i=0;i<3;i++){volatile float sum=num(v->alias_f0+i*4)+world[i];next[i]=sum;if(!isfinite(next[i]))return GH_PLAYER_DELTA_NUMERIC;}memcpy(v->alias_f0,next,12);return GH_PLAYER_DELTA_OK;
}
static gh_player_delta_status call(const gh_title_player_view*v,const gh_boot_work*w,gh_static_transform_policy policy,uint32_t*f,int rotation){
 if(!f)return GH_PLAYER_DELTA_ARGUMENT;*f=rotation?0x14f9f0:0x14f928;if(!v||!v->bytes||v->size<0x36c||!w)return GH_PLAYER_DELTA_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE)return GH_PLAYER_DELTA_UNSUPPORTED;
 fenv_t env;if(feholdexcept(&env))return GH_PLAYER_DELTA_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_PLAYER_DELTA_NUMERIC;}gh_player_delta_status e=body(v,w,rotation,f);if(fesetenv(&env))return GH_PLAYER_DELTA_NUMERIC;if(!e)*f=0;return e;
}
gh_player_delta_status gh_player_root_rotation_0014f9f0(const gh_title_player_view*v,const gh_boot_work*w,gh_static_transform_policy p,uint32_t*f){return call(v,w,p,f,1);}
gh_player_delta_status gh_player_root_translation_0014f928(const gh_title_player_view*v,const gh_boot_work*w,gh_static_transform_policy p,uint32_t*f){return call(v,w,p,f,0);}
