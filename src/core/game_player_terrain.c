#include "game_player_terrain.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float divide(float a,float b){volatile float x=a/b;return x;}
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float value(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static gh_scene_ray_status stop(uint32_t*f,uint32_t pc,gh_scene_ray_status e){if(f)*f=pc;return e;}
static int same(const gh_player_terrain_call*c,const gh_player_terrain_bindings*b){
 return c->bindings.tables==b->tables&&c->bindings.actor==b->actor&&
 c->bindings.sphere_query==b->sphere_query&&c->bindings.sphere_context==b->sphere_context&&
 c->actor_bytes==b->actor->bytes&&c->alias_identity==b->actor->alias_f0&&
 c->akg_identity==b->tables->akg.bytes;
}
static gh_scene_ray_status begin(gh_player_terrain_call*c,const gh_player_terrain_bindings*b,
 float size,uint8_t kind,uint32_t flags,gh_scene_ray_report*r,uint32_t cap,uint32_t*count){
 if(!c||!b||!b->tables||!b->actor||!b->actor->bytes||b->actor->size<0x5a4)return GH_RAY_ARGUMENT;
 if(c->active){if(!same(c,b)||c->kind!=kind||memcmp(&c->radius_or_depth,&size,4)||c->flags!=flags||c->reports!=r||c->report_capacity!=cap||c->report_count!=count)return GH_RAY_ARGUMENT;return GH_RAY_OK;}
 memset(c,0,sizeof(*c));c->bindings=*b;c->actor_bytes=b->actor->bytes;
 c->alias_identity=b->actor->alias_f0;c->akg_identity=b->tables->akg.bytes;
 c->kind=kind;c->flags=flags;c->radius_or_depth=size;c->reports=r;c->report_capacity=cap;c->report_count=count;
 /*132DA0 checks actor5A0 bit4 before reading position aliases or radius.*/
 if(kind==1&&(word(b->actor->bytes+0x5a0)&4)){c->active=1;c->complete=1;return GH_RAY_OK;}
 if(!b->actor->alias_f0)return GH_RAY_ARGUMENT;
 if(!isfinite(size)||size<0)return GH_RAY_NUMERIC;
 for(unsigned k=0;k<3;k++){c->old[k]=value(b->actor->bytes+0x490+4*k);c->current[k]=value(b->actor->alias_f0+4*k);if(!isfinite(c->old[k])||!isfinite(c->current[k]))return GH_RAY_NUMERIC;}
 c->active=1;c->phase=1;return GH_RAY_OK;
}
static int normalize(float*v){float d=add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]));float n=sqrtf(d);if(!isfinite(n))return 0;float inv=n>0?divide(1,n):0;for(unsigned k=0;k<3;k++)v[k]=mul(v[k],inv);return 1;}
static gh_scene_ray_status wall(gh_player_terrain_call*c,float*out,uint32_t*f){
 gh_scene_ray_status e;int hit=0;float half=mul(c->radius_or_depth,.5f);
 if(c->phase==1){
  for(unsigned k=0;k<3;k++)c->direction[k]=sub(c->current[k],c->old[k]);
  if(!normalize(c->direction))return GH_RAY_NUMERIC;
  c->phase=2;
 }
 if(c->phase==2){gh_scene_ray_request q={0};q.mask=1;q.flags=c->flags;q.segmented=1;q.hit_position=c->result;
  for(unsigned k=0;k<3;k++){float y=k==1?.8f:0;q.from[k]=sub(add(c->old[k],y),mul(c->direction[k],half));q.to[k]=add(add(c->current[k],y),mul(c->direction[k],half));}q.from[3]=q.to[3]=1;
  e=gh_scene_ray_query_00131e10(c->bindings.tables,&q,&hit,f);if(e)return e;
  if(hit){float d[3];for(unsigned k=0;k<3;k++)d[k]=sub(q.from[k],c->result[k]);if(!normalize(d))return GH_RAY_NUMERIC;for(unsigned k=0;k<3;k++)c->candidate[k]=add(c->result[k],mul(d[k],half));}
  else for(unsigned k=0;k<3;k++)c->candidate[k]=add(c->current[k],k==1?.8f:0);
  c->phase=3;
 }
 if(c->phase==3){
  /*1321D8 has an actual AKG-null earlyout. A ray miss cannot replace its sphere test.*/
  if(!c->bindings.tables->akg.bytes){c->hit=0;c->complete=1;return GH_RAY_OK;}
  if(!c->bindings.sphere_query)return stop(f,0x1321d8,GH_RAY_UNSUPPORTED);
  gh_terrain_sphere_request q={0};q.actor=c->bindings.actor;memcpy(q.center,c->candidate,12);q.radius=c->radius_or_depth;q.mask=1;q.flags=c->flags;q.reports=c->reports;q.report_capacity=c->report_capacity;q.report_count=c->report_count;
  e=c->bindings.sphere_query(c->bindings.sphere_context,c->bindings.tables,&q,c->result,&hit,f);if(e)return e;
  c->hit=(uint8_t)(hit!=0);c->complete=1;c->phase=4;
 }
 if(c->hit&&out)memcpy(out,c->result,12);
 return GH_RAY_OK;
}
static gh_scene_ray_status floor_query(gh_player_terrain_call*c,float*out,float*normal,uint32_t*f){
 gh_scene_ray_status e;int hit=0;
 if(c->phase==1){
  c->direction[0]=sub(c->current[0],c->old[0]);c->direction[1]=0;c->direction[2]=sub(c->current[2],c->old[2]);
  c->distance=sqrtf(add(mul(c->direction[0],c->direction[0]),mul(c->direction[2],c->direction[2])));
  if(!isfinite(c->distance)||!normalize(c->direction))return GH_RAY_NUMERIC;
  c->height=add(c->old[1],1);c->next_step=1.5f;c->phase=c->distance>=1.5f?2:3;
 }
 while(c->phase==2){
  gh_scene_ray_request q={0};q.mask=2;q.flags=0x400;q.hit_position=c->result;
  for(unsigned k=0;k<3;k++)q.from[k]=q.to[k]=add(c->old[k],mul(c->direction[k],c->next_step));
  q.from[1]=c->height;q.to[1]=sub(c->height,c->radius_or_depth);q.from[3]=q.to[3]=1;
  e=gh_scene_ray_query_direct_0012ed60(c->bindings.tables,&q,1,&hit,f);if(e)return e;
  if(hit)c->height=add(c->result[1],1);
  float next=add(c->next_step,1.5f);if(!isfinite(next)||!(next>c->next_step))return stop(f,0x132c40,GH_RAY_NUMERIC);c->next_step=next;
  if(c->next_step>=c->distance)c->phase=3;
 }
 if(c->phase==3){gh_scene_ray_request q={0};q.mask=2;q.flags=0x400;q.hit_position=c->result;q.hit_normal=c->normal;q.native_reports=c->reports;q.report_limit=c->reports?1:0;
  for(unsigned k=0;k<3;k++)q.from[k]=q.to[k]=c->current[k];
  q.from[1]=c->distance>=1.5f?c->height:add(c->current[1],1);q.to[1]=sub(c->current[1],c->radius_or_depth);q.from[3]=q.to[3]=1;
  e=gh_scene_ray_query_00131e10(c->bindings.tables,&q,&hit,f);if(e)return e;
  c->hit=(uint8_t)(hit!=0);c->complete=1;c->phase=4;
 }
 if(c->hit){if(out)memcpy(out,c->result,12);if(normal)memcpy(normal,c->normal,12);}
 return GH_RAY_OK;
}
static gh_scene_ray_status run(gh_player_terrain_call*c,const gh_player_terrain_bindings*b,float size,uint8_t kind,uint32_t flags,float*out,float*normal,gh_scene_ray_report*r,uint32_t cap,uint32_t*count,int*hit,uint32_t*f){
 if(f)*f=0;if(!hit)return GH_RAY_ARGUMENT;*hit=0;
 gh_scene_ray_status e=begin(c,b,size,kind,flags,r,cap,count);if(e)return e;if(c->complete){*hit=c->hit;if(c->hit){if(out)memcpy(out,c->result,12);if(normal)memcpy(normal,c->normal,12);}return GH_RAY_OK;}
 fenv_t env;if(feholdexcept(&env))return GH_RAY_NUMERIC;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_RAY_NUMERIC;}
 e=kind==1?wall(c,out,f):floor_query(c,out,normal,f);
 if(fesetenv(&env))return GH_RAY_NUMERIC;if(!e)*hit=c->hit;return e;
}
gh_scene_ray_status gh_player_terrain_wall_00132da0(gh_player_terrain_call*c,const gh_player_terrain_bindings*b,float r,uint32_t flags,float*out,gh_scene_ray_report*reports,uint32_t cap,uint32_t*count,int*hit,uint32_t*f){return run(c,b,r,1,flags,out,NULL,reports,cap,count,hit,f);}
gh_scene_ray_status gh_player_terrain_floor_001329c0(gh_player_terrain_call*c,const gh_player_terrain_bindings*b,float depth,float*out,float*normal,gh_scene_ray_report*report,int*hit,uint32_t*f){return run(c,b,depth,2,0x400,out,normal,report,report?1:0,NULL,hit,f);}
