#include "game_effect_sprite.h"
#include "game_esp14.h"
#include "game_actor_matrix.h"
#include "game_scene_at_geometry.h"
#include "../assets/static_mesh_vu_numeric.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static float number(const void*p){float n;memcpy(&n,p,4);return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static gh_effect_status fail(uint32_t*f,uint32_t pc,gh_effect_status s){*f=pc;return s;}
static int cvt_chop(float v,int32_t*out){if(!isfinite(v)||v < -2147483648.0f||v>=2147483648.0f)return 0;*out=(int32_t)v;return 1;}
static int inside_range(int32_t x){return ((uint32_t)x-0xa0u)<=0xfec0u;}
static gh_effect_status body(gh_game_effects*e,unsigned slot,uint32_t serial,const gh_effect_sprite_camera*b,gh_effect_sprite_projection*out,uint32_t*f){
 if(!e||!b||!out||!f)return GH_EFFECT_ARGUMENT;*f=0;if(b->policy!=GH_STATIC_BINARY32_REFERENCE)return fail(f,0x2fe0d8,GH_EFFECT_UNSUPPORTED);if(!b->camera||!b->init||!b->camera->constructed||b->camera->identity!=b->init->active_camera||b->camera->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(f,0x147528,GH_EFFECT_ARGUMENT);if(!b->display||!b->display->initialized)return fail(f,0x2fe0d8,GH_EFFECT_ARGUMENT);const gh_display_state*d=b->display;if(d->screen_width<=0||d->screen_width>2048||d->screen_lines<=0||d->screen_lines>2048)return fail(f,0x2fe0d8,GH_EFFECT_UNSUPPORTED);
 gh_effect_esp_view v;
 if(gh_effect_create_esp_at(e,slot,&v)||v.serial!=serial||!v.initialized||
    (v.type!=0x1a&&v.type!=0x12&&v.type!=0)||v.payload_size!=0x400)
    return fail(f,0x2fe0d8,GH_EFFECT_ARGUMENT);
 const uint32_t table=v.type==0x1a?0x44f270u:v.type==0x12?0x44eb70u:0x44fad0u;
 if(word(v.payload+0xf0)!=table)return fail(f,0x2fe0d8,GH_EFFECT_UNSUPPORTED);
 const int em00_attack=gh_effect_em00_attack_esp_is_bound(e,&v);
 if(v.type==0x12&&!em00_attack&&!gh_effect_knockdown_esp_is_bound(e,&v))return fail(f,0x2fe0d8,GH_EFFECT_UNSUPPORTED);
 if((v.actor||v.selector)&&!gh_esp14_nested_sprite_is_bound(v.record,v.actor,v.selector)&&
    !gh_effect_event_part_is_bound(v.record,v.actor,v.selector)&&!em00_attack&&!gh_effect_knockdown_esp_is_bound(e,&v))
    return fail(f,0x2fe0d8,GH_EFFECT_UNSUPPORTED);
 uint8_t*p=v.payload;uint32_t flags=word(p+0x11c);float projection[16],view[16],transform[16],world[16];
 memcpy(projection,b->camera->fields+0x100,64);memcpy(view,b->camera->fields+0x140,64);
 /* Current proven PlCamera138740 perspective shape; ctoridentity matrices
  * are not accepted as a completedprojection. No second ready flag/owner. */
 if(projection[8]!=2048||projection[9]!=2048||projection[11]!=1||projection[15]!=0)return fail(f,0x138740,GH_EFFECT_UNSUPPORTED);
 gh_effect_status status=gh_effect_node_world_matrix_native(e,0,slot,serial,world,b->policy,f);if(status)return status;
 if(flags&0x40000000u){
  if(!p[0x16])return fail(f,0x2b6328,GH_EFFECT_UNSUPPORTED);
  float local[16],position[4];
  memcpy(local,p+0x50,64);
  if(gh_scene_at_apply_world_001508d8(view,world+12,position))
   return fail(f,0x30a2b0,GH_EFFECT_UNSUPPORTED);
  if((flags&0xe0000000u)!=0xe0000000u){
   /* 147B60 appends the camera's pitch and roll in rotation order 4. */
   float camera_angles[3],rotation[16];
   memcpy(camera_angles,b->camera->fields+0x220,12);
   camera_angles[1]=0;
   if(gh_actor_matrix_rotation_00147728(camera_angles,4,rotation,b->policy,f)||
      gh_actor_matrix_product_00147548(local,rotation,local,b->policy,f))
    return fail(f,*f?*f:0x147b60,GH_EFFECT_UNSUPPORTED);
  }
  memcpy(local+12,position,12);
  if(gh_actor_matrix_product_00147548(projection,local,transform,b->policy,f))
   return fail(f,*f?*f:0x147548,GH_EFFECT_UNSUPPORTED);
 }else{
  float combined[16];
  memcpy(combined,b->camera->fields,64);
  if(gh_actor_matrix_product_00147548(combined,world,transform,b->policy,f))
   return fail(f,*f?*f:0x147548,GH_EFFECT_UNSUPPORTED);
 }
 if(b->cvt_policy!=GH_FRAME_CHOP_REFERENCE)return fail(f,0x2fddf8,GH_EFFECT_UNSUPPORTED);
 gh_effect_sprite_projection result={0};
 float center[4]={transform[12],transform[13],transform[14],transform[15]},q;
 int32_t distance;
 if(!cvt_chop(mul(center[3],10),&distance)||distance>INT32_MAX-50)return fail(f,0x2fddf8,GH_EFFECT_UNSUPPORTED);
 result.original_depth=distance+50;
 if(!gh_static_vu_reciprocal(center[3],&q))return fail(f,0x30a468,GH_EFFECT_UNSUPPORTED);
 int32_t center_fixed[3];
 for(unsigned j=0;j<3;j++)if(!gh_static_vu_ftoi4(mul(center[j],q),center_fixed+j))return fail(f,0x30a570,GH_EFFECT_UNSUPPORTED);
 if(center_fixed[2]<0||!inside_range(center_fixed[0])||!inside_range(center_fixed[1])){*out=result;return GH_EFFECT_OK;}
 memcpy(result.center_fixed4,center_fixed,sizeof(center_fixed));
 result.center_q=q;
 int32_t depth_delta=0;
 const float bias=number(p+0x254);
 if(bias!=0){
  if(!isfinite(bias))return fail(f,0x2fde60,GH_EFFECT_UNSUPPORTED);
  float direction[3],position[3],biased[4],combined[16];
  for(unsigned j=0;j<3;j++)direction[j]=add(world[12+j],-number(b->camera->fields+0x210+j*4));
  float squared=add(add(mul(direction[0],direction[0]),mul(direction[1],direction[1])),mul(direction[2],direction[2]));
  volatile float length=sqrtf(squared);
  if(!isfinite(length))return fail(f,0x1f7df8,GH_EFFECT_UNSUPPORTED);
  volatile float inverse=length>0?1.0f/length:0;
  for(unsigned j=0;j<3;j++)position[j]=add(world[12+j],mul(mul(direction[j],inverse),bias));
  if(gh_actor_matrix_product_00147548(projection,view,combined,b->policy,f)||
     gh_scene_at_apply_world_001508d8(combined,position,biased))
    return fail(f,*f?*f:0x30a2b0,GH_EFFECT_UNSUPPORTED);
  if(!gh_static_vu_reciprocal(biased[3],&q))return fail(f,0x30a468,GH_EFFECT_UNSUPPORTED);
  int32_t biased_z;
  if(!gh_static_vu_ftoi4(mul(biased[2],q),&biased_z))return fail(f,0x30a570,GH_EFFECT_UNSUPPORTED);
  /* Negative biased Z selects the mutable original 3C3BBC fallback. */
  if(biased_z<0)return fail(f,0x2fde60,GH_EFFECT_UNSUPPORTED);
  if(!cvt_chop(mul(biased[3],10),&distance)||distance>INT32_MAX-50)
    return fail(f,0x2fde60,GH_EFFECT_UNSUPPORTED);
  result.original_depth=distance+50;
  volatile float delta=(float)biased_z-(float)center_fixed[2];
  if(!cvt_chop(delta,&depth_delta))return fail(f,0x2fde60,GH_EFFECT_UNSUPPORTED);
 }
 if(flags&0x200u)return fail(f,0x147728,GH_EFFECT_UNSUPPORTED);float aspect=number(p+0x210),ax=number(p+0x214),ay=number(p+0x218);if(!isfinite(aspect)||!isfinite(ax)||!isfinite(ay))return fail(f,0x2fe0d8,GH_EFFECT_UNSUPPORTED);if(flags&0x10000000u)ax=add(1,-ax);if(flags&0x8000000u)ay=add(1,-ay);float right=mul(aspect,ax),left=mul(-aspect,add(1,-ax)),top=ay,bottom=-add(1,-ay);const float points[4][3]={{right,top,0},{left,top,0},{right,bottom,0},{left,bottom,0}};unsigned outside[4]={0};int x0=(2048-d->screen_width/2)*16,x1=(2048+d->screen_width-d->screen_width/2)*16,y0=d->gs_y_origin*16,y1=(d->gs_y_origin+d->screen_lines)*16;
 for(unsigned k=0;k<4;k++){float clip[4];if(gh_scene_at_apply_world_001508d8(transform,points[k],clip))return fail(f,0x30a2b0,GH_EFFECT_UNSUPPORTED);if(!gh_static_vu_reciprocal(clip[3],result.q+k))return fail(f,0x30a4e8,GH_EFFECT_UNSUPPORTED);for(unsigned j=0;j<3;j++)if(!gh_static_vu_ftoi4(mul(clip[j],result.q[k]),result.xyz_fixed4[k]+j))return fail(f,0x30a570,GH_EFFECT_UNSUPPORTED);int32_t*x=result.xyz_fixed4[k];uint32_t biased_bits=(uint32_t)x[2]+(uint32_t)depth_delta;
 memcpy(x+2,&biased_bits,4);if(x[2]<0||!inside_range(x[0])||!inside_range(x[1])){*out=result;return GH_EFFECT_OK;}outside[0]+=x[0]<x0;outside[1]+=x[0]>x1;outside[2]+=x[1]<y0;outside[3]+=x[1]>y1;}
 for(unsigned j=0;j<4;j++)if(outside[j]==4){*out=result;return GH_EFFECT_OK;}result.visible=1;*out=result;return GH_EFFECT_OK;
}
gh_effect_status gh_effect_sprite_project_002fe0d8(gh_game_effects*e,unsigned slot,uint32_t serial,const gh_effect_sprite_camera*b,gh_effect_sprite_projection*out,uint32_t*f){if(!f)return GH_EFFECT_ARGUMENT;*f=0;fenv_t env;if(feholdexcept(&env))return GH_EFFECT_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_EFFECT_UNSUPPORTED;}gh_effect_status status=body(e,slot,serial,b,out,f);if(fesetenv(&env))return GH_EFFECT_UNSUPPORTED;return status;}
gh_effect_status gh_effect_sprite_material_00151398(gh_stage_textures*t,gh_effect_view source,unsigned mode,const gh_stage_material**out,uint32_t*f){if(!f||!out)return GH_EFFECT_ARGUMENT;*f=0;*out=NULL;if(!t||!source.bytes)return fail(f,0x151398,GH_EFFECT_ARGUMENT);if(mode!=1)return fail(f,0x151398,GH_EFFECT_UNSUPPORTED);if(source.size>=4&&!memcmp(source.bytes,"TIM2",4)){
 gh_stage_status result=gh_stage_tim2_sprite_resolve(t,source.bytes,source.size,mode,out);
 return result?fail(f,0x151990,result==GH_STAGE_BOUNDS?GH_EFFECT_BOUNDS:GH_EFFECT_UNSUPPORTED):GH_EFFECT_OK;
 }if(source.size<48)return fail(f,0x151688,GH_EFFECT_BOUNDS);if(word(source.bytes)!=1||word(source.bytes+24))return fail(f,0x151688,GH_EFFECT_UNSUPPORTED);uint32_t channel;gh_stage_status status=gh_stage_texture_load(t,source.bytes,source.size,&channel);if(status)return fail(f,0x151688,status==GH_STAGE_BOUNDS?GH_EFFECT_BOUNDS:GH_EFFECT_UNSUPPORTED);gh_stage_channel registered;status=gh_stage_texture_channel(t,channel,&registered);if(status||registered.source!=source.bytes)return fail(f,0x151368,GH_EFFECT_ARGUMENT);status=gh_stage_tex0_material(t,registered.tex0,out);if(status)return fail(f,0x151368,status==GH_STAGE_BOUNDS?GH_EFFECT_BOUNDS:GH_EFFECT_UNSUPPORTED);if((*out)->source!=source.bytes||(*out)->source_bytes!=source.size)return fail(f,0x151368,GH_EFFECT_ARGUMENT);return GH_EFFECT_OK;}
