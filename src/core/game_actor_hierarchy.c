/* Original149EB8 finite initial-pose hierarchy update. */
#include "game_actor_model.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float value(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}
static void identity(uint8_t*p,size_t o){memset(p+o,0,64);for(unsigned j=0;j<4;j++)put(p,o+j*20,0x3f800000);}
static float mul(float a,float b){volatile float n=a*b;return n;}static float add(float a,float b){volatile float n=a+b;return n;}
static gh_actor_model_status fail(gh_actor_model*s,uint32_t f,unsigned d,gh_actor_model_status e){s->frontier=f;s->detail=d;return e;}

static int zero_unit(const uint8_t*p){for(unsigned j=0;j<3;j++)if((word(p+0x100+j*4)&0x7fffffffu)||(word(p+0x130+j*4)&0x7fffffffu)||word(p+0x110+j*4)!=0x3f800000u)return 0;return 1;}
gh_actor_model_status gh_actor_model_update_hierarchy_00149eb8(gh_actor_model*s,gh_static_transform_policy policy){
 if(!s||!s->bound||!s->parts_ready)return GH_ACTOR_MODEL_ARGUMENT;s->frontier=0;s->detail=0;uint8_t*r=s->root.bytes;
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x149eb8,0,GH_ACTOR_MODEL_UNSUPPORTED);
 if((word(r+0x250)&0x40004)==0x40000)return fail(s,0x1501d0,0,GH_ACTOR_MODEL_UNSUPPORTED);
 if(!(word(r+0x250)&0x800)&&!zero_unit(r))return fail(s,0x147a48,0,GH_ACTOR_MODEL_UNSUPPORTED);
 if(!isfinite(value(r+0x27c)))return fail(s,0x149eb8,0,GH_ACTOR_MODEL_UNSUPPORTED);
 for(unsigned j=0;j<3;j++)if(!isfinite(value(s->root.alias_f0+j*4)))return fail(s,0x147a48,0,GH_ACTOR_MODEL_UNSUPPORTED);
 for(unsigned i=0;i<s->part_count;i++){uint8_t*p=s->parts[i].bytes;uint32_t f=word(p+0x154);
  /* AdjustEuler precedes the skipgate, so only alreadyzero angles are a
   * verified no-op. Nonzero wrappedEuler is not silently accepted. */
  for(unsigned j=0;j<3;j++)if(word(p+0x100+j*4)&0x7fffffffu)return fail(s,0x1f7da0,i,GH_ACTOR_MODEL_UNSUPPORTED);
  if(!(f&8)&&!zero_unit(p))return fail(s,0x150138,i,GH_ACTOR_MODEL_UNSUPPORTED);
  if(value(r+0x27c)>0&&!(f&0x100000)&&!(word(r+0x250)&0x20))return fail(s,0x156d70,i,GH_ACTOR_MODEL_UNSUPPORTED);
  if(!(f&16)&&(f&0x800))return fail(s,0x159fc8,i,GH_ACTOR_MODEL_UNSUPPORTED);
  for(unsigned j=0;j<16;j++)if(!isfinite(value(p+j*4)))return fail(s,0x150138,i,GH_ACTOR_MODEL_UNSUPPORTED);
 }
 fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x149eb8,0,GH_ACTOR_MODEL_UNSUPPORTED);
 if(!(word(r+0x250)&0x800)){float pos[3];memcpy(pos,s->root.alias_f0,12);identity(r,0x80);memcpy(r+0xb0,pos,12);memcpy(r+0x120,r+0x110,12);}
 put(r,0x250,value(r+0x27c)>0?word(r+0x250)|4u:word(r+0x250)&~4u);
 for(unsigned i=0;i<s->part_count;i++){gh_actor_part*part=s->parts+i;uint8_t*p=part->bytes;uint32_t flags=word(p+0x154);if(!(flags&8)){float pos[3];memcpy(pos,part->alias_d0,12);identity(p,0);memcpy(p+0x30,pos,12);}if(!(flags&16)){const uint8_t*parent=part->parent_is_model?r:s->parts[part->parent].bytes;float local[16],pm[16],world[16];memcpy(local,p,64);memcpy(pm,parent+0x80,64);for(unsigned col=0;col<4;col++)for(unsigned row=0;row<4;row++){float n=mul(pm[row],local[col*4]);n=add(n,mul(pm[4+row],local[col*4+1]));n=add(n,mul(pm[8+row],local[col*4+2]));world[col*4+row]=add(n,mul(pm[12+row],local[col*4+3]));}for(unsigned k=0;k<16;k++)if(!isfinite(world[k])){fesetenv(&saved);return fail(s,0x1501d0,i,GH_ACTOR_MODEL_UNSUPPORTED);}memcpy(p+0x80,world,64);}}
 if(fesetenv(&saved))return fail(s,0x149eb8,0,GH_ACTOR_MODEL_UNSUPPORTED);return GH_ACTOR_MODEL_OK;
}
