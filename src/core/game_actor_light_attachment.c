#include "game_actor_light_attachment.h"
#include "game_actor_matrix.h"
#include <math.h>
#include <fenv.h>
#include <string.h>
static float number(const void*p){float n;memcpy(&n,p,4);return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static int normal_or_zero(float n){return isfinite(n)&&(n==0||fpclassify(n)==FP_NORMAL);}
static gh_actor_model_status fail(uint32_t *pc,uint32_t n,gh_actor_model_status e){*pc=n;return e;}
static gh_actor_model_status run(const gh_actor_model*m,const gh_actor_light_bindings*b,int relative,
 const float in[4],float out[4],float rotation[16],uint32_t*pc){
 const gh_actor_part*part=NULL;
 if(m->root.bytes[0x2b4]&&gh_actor_model_part_at(m,0,&part))return fail(pc,0x14d9b8,GH_ACTOR_MODEL_UNSUPPORTED);
 float matrix[16],origin[3];
 if(relative){
  if(!b||!b->camera||!b->camera->constructed)return fail(pc,0x147728,GH_ACTOR_MODEL_ARGUMENT);
  float angles[3];memcpy(angles,b->camera->fields+0x220,12);
  if(gh_actor_matrix_rotation_00147728(angles,0,matrix,GH_STATIC_BINARY32_REFERENCE,pc))return fail(pc,*pc?*pc:0x147728,GH_ACTOR_MODEL_UNSUPPORTED);
  if(part)memcpy(origin,part->bytes+0xb0,12);
  else{
   if(!b->pivot_owner||!b->pivot_owner->initialized)return fail(pc,0x14da0c,GH_ACTOR_MODEL_ARGUMENT);
   for(unsigned k=0;k<3;k++)origin[k]=number(b->pivot_owner->bytes+k*4);
  }
 }else{
  /* Original adds80 to the part pointer even when NULL. A missing native
   * part cannot be acknowledged as identity/zero translation. */
  if(!part)return fail(pc,0x30a2b0,GH_ACTOR_MODEL_UNSUPPORTED);
  memcpy(matrix,part->bytes+0x80,64);
 }
 for(unsigned k=0;k<4;k++)if(!normal_or_zero(in[k]))return fail(pc,0x30a2b0,GH_ACTOR_MODEL_UNSUPPORTED);
 for(unsigned k=0;k<16;k++)if(!normal_or_zero(matrix[k]))return fail(pc,0x30a2b0,GH_ACTOR_MODEL_UNSUPPORTED);
 for(unsigned row=0;row<4;row++){
  float a=mul(matrix[row],in[0]);if(!normal_or_zero(a))return fail(pc,0x30a2b0,GH_ACTOR_MODEL_UNSUPPORTED);
  for(unsigned column=1;column<4;column++){
   float product=mul(matrix[column*4+row],in[column]);if(!normal_or_zero(product))return fail(pc,0x30a2b0,GH_ACTOR_MODEL_UNSUPPORTED);
   a=add(a,product);if(!normal_or_zero(a))return fail(pc,0x30a2b0,GH_ACTOR_MODEL_UNSUPPORTED);
  }
  out[row]=a;
 }
 if(relative){
  for(unsigned k=0;k<3;k++){if(!normal_or_zero(origin[k]))return fail(pc,0x14da50,GH_ACTOR_MODEL_UNSUPPORTED);out[k]=add(out[k],origin[k]);if(!normal_or_zero(out[k]))return fail(pc,0x14da50,GH_ACTOR_MODEL_UNSUPPORTED);}
  memcpy(rotation,matrix,64);
 }
 return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_light_attachment_0014d9a4(const gh_actor_model*m,const gh_actor_light_bindings*b,int relative,
 const float in[4],float out[4],float camera_matrix[16],int*written,gh_static_transform_policy policy,uint32_t*pc){
 uint32_t local=0;if(!pc)pc=&local;*pc=0;
 if(!m||m->native_identity!=m||!m->ready||!m->root.bytes||m->root.size<0x2b5||!in||!out||!camera_matrix||!written||(relative!=0&&relative!=1)||policy!=GH_STATIC_BINARY32_REFERENCE)return GH_ACTOR_MODEL_ARGUMENT;
 fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;
 if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}
 float output[4],rotation[16];gh_actor_model_status e=run(m,b,relative,in,output,rotation,pc);
 if(fesetenv(&env))return GH_ACTOR_MODEL_UNSUPPORTED;
 if(!e){memcpy(out,output,16);if(relative)memcpy(camera_matrix,rotation,64);*written=relative;}
 return e;
}
