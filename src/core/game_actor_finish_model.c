#include "game_actor_finish_model.h"
#include "game_actor_matrix.h"
#include "game_actor_external_parent.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t rd(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float rf(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static gh_actor_model_status fail(gh_actor_model*m,uint32_t pc,unsigned detail,gh_actor_model_status e){m->frontier=pc;m->detail=detail;return e;}
gh_actor_model_status gh_actor_finish_model_0014a038(gh_actor_model*m,gh_static_transform_policy policy){
 if(!m||!m->root.bytes||m->root.size<0x2b5)return GH_ACTOR_MODEL_ARGUMENT;m->frontier=0;m->detail=0;uint8_t*r=m->root.bytes;uint32_t flags=rd(r+0x250);
 if(!(flags&0x20))return GH_ACTOR_MODEL_OK;
 if(!m->parts_ready||m->part_count>255||m->parts_allocated<m->part_count||r[0x2b4]!=m->part_count)return fail(m,0x14a038,0,GH_ACTOR_MODEL_BOUNDS);
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(m,0x14a038,0,GH_ACTOR_MODEL_UNSUPPORTED);
 const uint8_t*external=NULL;if(m->external_owner){if(gh_actor_model_external_parent_at(m,0,&external))return fail(m,0x1501d0,0,GH_ACTOR_MODEL_UNSUPPORTED);}else if(m->external_parent)return fail(m,0x1501d0,0,GH_ACTOR_MODEL_UNSUPPORTED);
 fenv_t env;if(fegetenv(&env)||fesetround(FE_TONEAREST))return fail(m,0x14a038,0,GH_ACTOR_MODEL_UNSUPPORTED);gh_actor_model_status e=GH_ACTOR_MODEL_OK;uint32_t frontier=0;
 if(!(flags&0x800)){
  if(!m->root.alias_f0){e=fail(m,0x147a48,0,GH_ACTOR_MODEL_ARGUMENT);goto finish;}float pos[3],angles[3],scale[3],matrix[16];memcpy(pos,m->root.alias_f0,12);memcpy(angles,r+0x100,12);memcpy(scale,r+0x110,12);
  if(gh_actor_matrix_compose_00147a48(pos,angles,scale,r[0x140],matrix,policy,&frontier)){e=fail(m,frontier?frontier:0x147a48,0,GH_ACTOR_MODEL_UNSUPPORTED);goto finish;}memcpy(r+0x80,matrix,64);memcpy(r+0x120,r+0x110,12);
 }
 for(unsigned i=0;i<m->part_count;i++){
  gh_actor_part*part=&m->parts[i];if(!part->bytes){e=fail(m,0x14a038,i,GH_ACTOR_MODEL_BOUNDS);break;}uint32_t pf=rd(part->bytes+0x154);if(pf&16)continue;
  if(!isfinite(rf(r+0x27c))){e=fail(m,0x14a038,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}
  if(rf(r+0x27c)>0&&!(pf&0x100000)){e=fail(m,0x147568,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}
  if((flags&0x40004)==0x40000){e=fail(m,0x1501d0,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}
  const uint8_t*parent=i==0&&external?external:part->parent_is_model?r:part->parent>=0&&(unsigned)part->parent<m->part_count?m->parts[part->parent].bytes:NULL;
  if(!parent){e=fail(m,0x1501d0,i,GH_ACTOR_MODEL_BOUNDS);break;}
  float pm[16],local[16],world[16];memcpy(pm,parent+0x80,64);memcpy(local,part->bytes,64);
  if(gh_actor_matrix_product_00147548(pm,local,world,policy,&frontier)){e=fail(m,frontier?frontier:0x1501d0,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}memcpy(part->bytes+0x80,world,64);
  if(pf&0x800){e=fail(m,0x159fc8,i,GH_ACTOR_MODEL_UNSUPPORTED);break;}
 }
finish:if(fesetenv(&env))return fail(m,0x14a038,0,GH_ACTOR_MODEL_UNSUPPORTED);return e;
}
