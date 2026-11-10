#include "game_actor_finish_model.h"
#include "game_actor_matrix.h"
#include "game_actor_external_parent.h"
#include "game_actor_inverse.h"
#include "game_actor_blend.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t rd(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float rf(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static gh_actor_model_status fail(gh_actor_model*m,uint32_t pc,unsigned detail,gh_actor_model_status e){m->frontier=pc;m->detail=detail;return e;}
gh_actor_model_status gh_actor_finish_model_resume_0014a038(gh_actor_finish_model_call*c,gh_actor_model*m,gh_static_transform_policy policy){
 if(!c||!m||!m->root.bytes||m->root.size<0x2b5)return GH_ACTOR_MODEL_ARGUMENT;m->frontier=0;m->detail=0;uint8_t*r=m->root.bytes;
 if(c->model&&(c->model!=m||c->root_bytes!=r||c->part_count!=m->part_count))return fail(m,0x14a038,0,GH_ACTOR_MODEL_ARGUMENT);
 if(c->stage>5)return fail(m,0x14a038,0,GH_ACTOR_MODEL_ARGUMENT);
 if(!c->model){c->model=m;c->root_bytes=r;c->part_count=m->part_count;c->root_flags=rd(r+0x250);}
 uint32_t flags=c->root_flags;if(c->stage==5)return GH_ACTOR_MODEL_OK;
 if(!(flags&0x20)){c->stage=5;return GH_ACTOR_MODEL_OK;}
 if(!m->parts_ready||m->part_count>255||m->parts_allocated<m->part_count||r[0x2b4]!=m->part_count)return fail(m,0x14a038,0,GH_ACTOR_MODEL_BOUNDS);
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(m,0x14a038,0,GH_ACTOR_MODEL_UNSUPPORTED);
 const uint8_t*external=NULL;if(m->external_owner){if(gh_actor_model_external_parent_at(m,0,&external))return fail(m,0x1501d0,0,GH_ACTOR_MODEL_UNSUPPORTED);}else if(m->external_parent)return fail(m,0x1501d0,0,GH_ACTOR_MODEL_UNSUPPORTED);
 fenv_t env;if(fegetenv(&env)||fesetround(FE_TONEAREST))return fail(m,0x14a038,0,GH_ACTOR_MODEL_UNSUPPORTED);gh_actor_model_status e=GH_ACTOR_MODEL_OK;uint32_t frontier=0;
 if(!c->stage&&!(flags&0x800)){
  if(!m->root.alias_f0){e=fail(m,0x147a48,0,GH_ACTOR_MODEL_ARGUMENT);goto finish;}float pos[3],angles[3],scale[3],matrix[16];memcpy(pos,m->root.alias_f0,12);memcpy(angles,r+0x100,12);memcpy(scale,r+0x110,12);
  if(gh_actor_matrix_compose_00147a48(pos,angles,scale,r[0x140],matrix,policy,&frontier)){e=fail(m,frontier?frontier:0x147a48,0,GH_ACTOR_MODEL_UNSUPPORTED);goto finish;}memcpy(r+0x80,matrix,64);memcpy(r+0x120,r+0x110,12);
 }
 if(!c->stage)c->stage=1;
 while(c->part_index<m->part_count){
  unsigned i=c->part_index;
  gh_actor_part*part=&m->parts[i];if(!part->bytes){e=fail(m,0x14a038,i,GH_ACTOR_MODEL_BOUNDS);break;}if(c->stage==1){c->part_flags=rd(part->bytes+0x154);if(c->part_flags&16){c->part_index++;continue;}c->stage=2;}
  uint32_t pf=c->part_flags;
  if(!isfinite(rf(r+0x27c))){e=fail(m,0x14a038,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}
  const uint8_t*parent=i==0&&external?external:part->parent_is_model?r:part->parent>=0&&(unsigned)part->parent<m->part_count?m->parts[part->parent].bytes:NULL;
  if(!parent){e=fail(m,rf(r+0x27c)>0&&!(pf&0x100000)?0x147568:0x1501d0,i,GH_ACTOR_MODEL_BOUNDS);break;}
  if(c->stage==2&&rf(r+0x27c)>0&&!(pf&0x100000)){
   float parent_world[16],inverse[16],world_before[16],scratch[16],current[16],captured[16],blended[16];memcpy(parent_world,parent+0x80,64);
   if(gh_actor_matrix_inverse_00147568(parent_world,inverse,policy,&frontier)){e=fail(m,frontier?frontier:0x147568,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}
   memcpy(world_before,part->bytes+0x80,64);
   if(gh_actor_matrix_product_00147548(inverse,world_before,scratch,policy,&frontier)){e=fail(m,frontier?frontier:0x30a2e0,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}
   /* Actual14A11C/120 A0=A1=s0;14A128 A2=s0+40. The computed
    * inverse(parentWorld)*world scratch is NOT the156D70 input. Preserve
    * solved LOCAL0 as current; do not substitute the sensible-looking scratch. */
   if(!m->blend_globals){e=fail(m,0x156d70,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}
   memcpy(current,part->bytes,64);memcpy(captured,part->bytes+0x40,64);
   if(gh_actor_matrix_blend_00156d70(rf(r+0x27c),current,captured,blended,m->blend_globals,policy,&frontier)){e=fail(m,frontier?frontier:0x156d70,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}
   memcpy(part->bytes,blended,64);
  }
  if(c->stage==2)c->stage=3;
  if(c->stage==3){
  if((flags&0x40004)==0x40000){e=fail(m,0x1501d0,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}
  float pm[16],local[16],world[16];memcpy(pm,parent+0x80,64);memcpy(local,part->bytes,64);
  if(gh_actor_matrix_product_00147548(pm,local,world,policy,&frontier)){e=fail(m,frontier?frontier:0x1501d0,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}memcpy(part->bytes+0x80,world,64);c->stage=4;
  }
  if(pf&0x800){e=fail(m,0x159fc8,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}
  c->part_index++;c->stage=1;
 }
 if(!e)c->stage=5;
finish:if(fesetenv(&env))return fail(m,0x14a038,0,GH_ACTOR_MODEL_UNSUPPORTED);return e;
}

gh_actor_model_status gh_actor_finish_model_0014a038(gh_actor_model*m,gh_static_transform_policy policy){
 gh_actor_finish_model_call call={0};return gh_actor_finish_model_resume_0014a038(&call,m,policy);
}
