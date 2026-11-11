#include "game_cloth_distances.h"
#include "game_cloth_collision.h"
#include "game_cloth_point.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t u32(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static float rf(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static float add(float a,float b){volatile float f=a+b;return f;}
static float mul(float a,float b){volatile float f=a*b;return f;}
static float divide(float a,float b){volatile float f=a/b;return f;}
static float length(const float*v){volatile float f=sqrtf(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2])));return f;}
static void lock(uint8_t*r){if(r[0x91]&1)memcpy(r+0x40,r+0x80,12);}
static gh_cloth_status fail(gh_cloth_distances*s,uint32_t f,gh_cloth_status e){s->frontier=f;return e;}
static gh_cloth_status contact(gh_cloth_distances*s,gh_cloth_step_prefix*prefix,uint8_t*a,uint8_t*b){float p[3],q[3];memcpy(p,a+0x40,12);memcpy(q,b+0x40,12);uint32_t f;gh_cloth_status e=gh_cloth_segment_body_00107360(prefix->geometry,p,q,prefix->bindings.matrix_policy,&f);if(e)return fail(s,f,e);memcpy(a+0x40,p,12);memcpy(b+0x40,q,12);return GH_CLOTH_OK;}
static gh_cloth_status correct(gh_cloth_distances*s,uint8_t*a,uint8_t*b,const float*anchor,float rest,float scale,float stiffness,int half_rest){
 float delta[3];for(unsigned k=0;k<3;k++)delta[k]=add(b?rf(b+0x40+4*k):anchor[k],-rf(a+0x40+4*k));float distance=length(delta);if(!(distance>0)||!isfinite(distance))return fail(s,0x1059cc,GH_CLOTH_NUMERIC);float desired=mul(rest,scale),factor;
 if(half_rest)factor=divide(mul(add(mul(distance,0.5f),-desired),stiffness),distance);
 else factor=divide(mul(mul(add(distance,-desired),0.5f),stiffness),distance);
 for(unsigned k=0;k<3;k++){float shift=mul(delta[k],factor),p=add(rf(a+0x40+4*k),shift);memcpy(a+0x40+4*k,&p,4);if(b){float q=add(rf(b+0x40+4*k),-shift);memcpy(b+0x40+4*k,&q,4);}}
 lock(a);if(b)lock(b);return GH_CLOTH_OK;
}
static gh_cloth_status body(gh_cloth_distances*s,gh_cloth_angles*angles){
 gh_cloth_step_prefix*prefix=angles->forces->prefix;const gh_cloth_operands*springs=&prefix->operands;gh_actor_model*model=springs->model;uint8_t*p=springs->config;uint32_t flags=u32(p+0x5c),iterations=u32(p+0x44);if((flags&2)||!iterations){s->complete=1;s->frontier=0x105a94;return GH_CLOTH_OK;}if(!(flags&0x100)||u32(p+0x54))return fail(s,0x104ec8,GH_CLOTH_UNSUPPORTED);if(iterations>64)return fail(s,0x104ec8,GH_CLOTH_BOUNDS);
 for(unsigned row=0;row<springs->count;row++){unsigned id=springs->indices[0][row];if(id>=44||id>=model->parts_allocated)return fail(s,0x104f10,GH_CLOTH_BOUNDS);for(unsigned j=1;j<=5;j++)if(springs->indices[j][row]!=255&&springs->indices[j][row]>=44)return fail(s,0x104f10,GH_CLOTH_BOUNDS);if((flags&0x80)&&springs->indices[5][row]==255)return fail(s,0x106bf8,GH_CLOTH_UNSUPPORTED);}
 uint8_t records[44*0xa0];memcpy(records,springs->records,sizeof(records));unsigned queries=0,hits=0;float scale=prefix->radius_scale,stiffness=rf(p+0x4c);
 for(uint32_t pass=0;pass<iterations;pass++)for(unsigned row=0;row<springs->count;row++){
  unsigned id=springs->indices[0][row];uint8_t*r=records+id*0xa0;
  if(!(flags&4)){const unsigned order[4]={3,4,1,2};for(unsigned item=0;item<4;item++){unsigned table=order[item],target=springs->indices[table][row];if(!(springs->tables&(1u<<table))||target==255)continue;uint8_t*q=records+target*0xa0;gh_cloth_status e=contact(s,prefix,r,q);if(e)return e;queries++;if(table<3){e=correct(s,r,q,NULL,rf(r+0x20+(table-1)*4),scale,stiffness,1);if(e)return e;}else{lock(r);lock(q);}}}
  unsigned target=springs->indices[5][row];gh_cloth_status e;
  if(target==255){float point[3],anchor[3];memcpy(point,r+0x40,12);memcpy(anchor,model->parts[id].alias_f0,12);int hit;uint32_t f;e=gh_cloth_point_body_001067b8(prefix->geometry,point,anchor,angles->shared_pivot,prefix->bindings.matrix_policy,&hit,&f);if(e)return fail(s,f,e);memcpy(r+0x40,point,12);if(hit){r[0x90]|=1;hits++;}e=correct(s,r,NULL,anchor,rf(r+0x60),scale,stiffness,0);if(e)return e;}
  else{uint8_t*q=records+target*0xa0;e=contact(s,prefix,r,q);if(e)return e;queries++;e=correct(s,r,q,NULL,rf(r+0x60),scale,stiffness,0);if(e)return e;}
 }
 for(unsigned i=0;i<44;i++)for(unsigned k=0;k<3;k++)if(!isfinite(rf(records+i*0xa0+0x40+4*k)))return fail(s,0x105a90,GH_CLOTH_NUMERIC);
 memcpy(springs->records,records,sizeof(records));s->iterations=iterations;s->pair_queries=queries;s->point_hits=hits;s->complete=1;s->frontier=0x105a94;return GH_CLOTH_OK;
}
gh_cloth_status gh_cloth_constrain_distances_00104ec8(gh_cloth_distances*s,gh_cloth_angles*angles){
 if(!s||!angles||!angles->complete||angles->frontier!=0x104ec8||!angles->forces||!angles->forces->prefix||!angles->shared_pivot)return GH_CLOTH_ARGUMENT;if(s->angles&&s->angles!=angles)return GH_CLOTH_ARGUMENT;if(s->complete)return GH_CLOTH_OK;s->angles=angles;fenv_t env;if(feholdexcept(&env))return GH_CLOTH_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CLOTH_NUMERIC;}gh_cloth_status e=body(s,angles);if(fesetenv(&env))return GH_CLOTH_NUMERIC;return e;
}
