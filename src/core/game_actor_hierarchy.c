#include "game_actor_axis_rotation.h"
/* Original149EB8 finite initial-pose hierarchy update. */
#include "game_actor_model.h"
#include "game_actor_matrix.h"
#include "game_actor_external_parent.h"
#include "game_actor_blend.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float value(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}
static float add(float a,float b){volatile float n=a+b;return n;}
static gh_actor_model_status fail(gh_actor_model*s,uint32_t f,unsigned d,gh_actor_model_status e){s->frontier=f;s->detail=d;return e;}


gh_actor_model_status gh_actor_model_update_hierarchy_00149eb8(gh_actor_model*s,gh_static_transform_policy policy){
 if(!s||!s->bound||!s->parts_ready)return GH_ACTOR_MODEL_ARGUMENT;s->frontier=0;s->detail=0;uint8_t*r=s->root.bytes;
 const uint8_t *external_parents[255]={0};
 if(s->external_owner){
  if(s->external_parts||gh_actor_model_external_parent_at(s,0,&external_parents[0]))return fail(s,0x1501d0,0,GH_ACTOR_MODEL_UNSUPPORTED);
 }else if(s->external_parent)return fail(s,0x1501d0,0,GH_ACTOR_MODEL_UNSUPPORTED);
 if(s->external_parts)for(unsigned i=0;i<s->part_count;i++)
  if(gh_actor_model_external_parts_at(s,i,&external_parents[i]))return fail(s,0x1501d0,i,GH_ACTOR_MODEL_UNSUPPORTED);
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x149eb8,0,GH_ACTOR_MODEL_UNSUPPORTED);
 if(!isfinite(value(r+0x27c)))return fail(s,0x149eb8,0,GH_ACTOR_MODEL_UNSUPPORTED);uint32_t rootflags=value(r+0x27c)>0?word(r+0x250)|4u:word(r+0x250)&~4u;
 float rootmatrix[16],locals[255][16],adjusted[255][4];uint32_t frontier=0;
 if(!(rootflags&0x800)){float pos[3],angle[3],scale[3];memcpy(pos,s->root.alias_f0,12);memcpy(angle,r+0x100,12);memcpy(scale,r+0x110,12);if(gh_actor_matrix_compose_00147a48(pos,angle,scale,r[0x140],rootmatrix,policy,&frontier))return fail(s,frontier,0,GH_ACTOR_MODEL_UNSUPPORTED);}else{memcpy(rootmatrix,r+0x80,64);for(unsigned j=0;j<16;j++)if(!isfinite(rootmatrix[j]))return fail(s,0x1501d0,0,GH_ACTOR_MODEL_UNSUPPORTED);}
 fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x149eb8,0,GH_ACTOR_MODEL_UNSUPPORTED);
 for(unsigned i=0;i<s->part_count;i++){uint8_t*p=s->parts[i].bytes;uint32_t f=word(p+0x154);float raw[4];memcpy(raw,p+0x100,16);if(gh_actor_euler_adjust_001f7da0(raw,adjusted[i],policy,&frontier)){fesetenv(&saved);return fail(s,frontier,i,GH_ACTOR_MODEL_UNSUPPORTED);}
  if(!(f&8)){float angles[4]={0,0,0,1},wrapped[4],pos[3],scale[3];for(unsigned k=0;k<3;k++)angles[k]=add(adjusted[i][k],value(p+0x130+k*4));memcpy(pos,s->parts[i].alias_d0,12);memcpy(scale,p+0x110,12);if(gh_actor_euler_adjust_001f7da0(angles,wrapped,policy,&frontier)||gh_actor_matrix_compose_00147a48(pos,wrapped,scale,0,locals[i],policy,&frontier)){fesetenv(&saved);return fail(s,frontier,i,GH_ACTOR_MODEL_UNSUPPORTED);}}
  else{memcpy(locals[i],p,64);for(unsigned k=0;k<16;k++)if(!isfinite(locals[i][k])){fesetenv(&saved);return fail(s,0x150138,i,GH_ACTOR_MODEL_UNSUPPORTED);}}
  if(value(r+0x27c)>0&&!(f&0x100000)&&!(rootflags&0x20)){float old[16];memcpy(old,p+0x40,64);if(!s->blend_globals||gh_actor_matrix_blend_00156d70(value(r+0x27c),locals[i],old,locals[i],s->blend_globals,policy,&frontier)){fesetenv(&saved);return fail(s,frontier?frontier:0x156d70,i,GH_ACTOR_MODEL_UNSUPPORTED);}}
 }
 if(!(rootflags&0x800)){memcpy(r+0x80,rootmatrix,64);memcpy(r+0x120,r+0x110,12);}put(r,0x250,rootflags);
 for(unsigned i=0;i<s->part_count;i++){gh_actor_part*part=s->parts+i;uint8_t*p=part->bytes;uint32_t flags=word(p+0x154);memcpy(p+0x100,adjusted[i],16);if(!(flags&8)||(value(r+0x27c)>0&&!(flags&0x100000)&&!(rootflags&0x20)))memcpy(p,locals[i],64);if(!(flags&16)){const uint8_t*parent=external_parents[i]?external_parents[i]:part->parent_is_model?r:s->parts[part->parent].bytes;if(gh_actor_part_world_001501d0(part,parent,rootflags,policy,&frontier)){fesetenv(&saved);return fail(s,frontier?frontier:0x1501d0,i,GH_ACTOR_MODEL_UNSUPPORTED);}}}
 if(fesetenv(&saved))return fail(s,0x149eb8,0,GH_ACTOR_MODEL_UNSUPPORTED);return GH_ACTOR_MODEL_OK;
}
