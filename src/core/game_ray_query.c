#include "game_ray_query.h"
#include "game_ray_static.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
static float number(uint32_t n){float f;memcpy(&f,&n,4);return f;}
static float add(float a,float b){volatile float r=a+b;return r;}
static float sub(float a,float b){volatile float r=a-b;return r;}
static float mul(float a,float b){volatile float r=a*b;return r;}
static float divide(float a,float b){volatile float r=a/b;return r;}
static gh_scene_ray_status stop(uint32_t*f,uint32_t pc,gh_scene_ray_status e){if(f)*f=pc;return e;}
static gh_scene_ray_status straight(gh_game_scene_tables*s,const gh_scene_ray_request*r,int*hit,uint32_t*f,uint8_t shorten){
 float delta[3],minimum[3],maximum[3],length_squared,length;fenv_t env;gh_scene_ray_state q;
 if(f)*f=0;if(!s||!r||!hit)return GH_RAY_ARGUMENT;*hit=0;
 /* 12ED60's actual first gate is manager+0 (AKG), before reading any ray or
  * writing persistent query scratch. Do not confuse actor list size with it. */
 if(!s->akg.bytes)return GH_RAY_OK;
 if(!s->actors_ready)return GH_RAY_ARGUMENT;
 for(unsigned i=0;i<3;++i){
  if(!isfinite(r->from[i])||!isfinite(r->to[i])||!isfinite(number(s->actor_vec30[i]))||!isfinite(number(s->actor_vec40[i])))return GH_RAY_NUMERIC;
 }
 if(feholdexcept(&env))return GH_RAY_NUMERIC;
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_RAY_NUMERIC;}
 for(unsigned i=0;i<3;++i){delta[i]=sub(r->to[i],r->from[i]);minimum[i]=r->from[i];maximum[i]=r->to[i];if(minimum[i]>maximum[i]){float t=minimum[i];minimum[i]=maximum[i];maximum[i]=t;}}
 length_squared=add(add(mul(delta[0],delta[0]),mul(delta[1],delta[1])),mul(delta[2],delta[2]));length=sqrtf(length_squared);
 q=s->ray;
 for(unsigned i=0;i<3;++i){
  q.fromb0[i]=q.frome0[i]=bits(r->from[i]);q.toc0[i]=q.tof0[i]=bits(r->to[i]);
  q.directiond0[i]=bits(length>0?mul(delta[i],divide(1,length)):0);
  q.bounds_min90[i]=bits(minimum[i]);q.bounds_maxa0[i]=bits(maximum[i]);
 }
 q.mask174=r->mask;q.flags178=r->flags;q.triangle17c=0;q.hit_kind180=0;q.mode181=shorten;
 q.report_limit140=(uint32_t)r->report_limit;q.reports18c=r->native_reports?(void*)r->native_reports:r->reports;q.distance184=q.distance188=bits(length_squared);++q.native_queries;
 if(fesetenv(&env))return GH_RAY_NUMERIC;
 if(!isfinite(length_squared)||!isfinite(length))return GH_RAY_NUMERIC;
 s->ray=q;
 /* 133408 tests inclusive overlap on all axes. Original returns false before
  * grid division or triangle/report traversal when any interval is disjoint. */
 for(unsigned i=0;i<3;++i)if(maximum[i]<number(s->actor_vec30[i])||minimum[i]>number(s->actor_vec40[i]))return GH_RAY_OK;
 return gh_scene_ray_static_overlap(s,r,hit,f);
}
gh_scene_ray_status gh_scene_ray_query_direct_0012ed60(gh_game_scene_tables*s,const gh_scene_ray_request*r,uint8_t mode,int*hit,uint32_t*f){
 if(mode>1)return GH_RAY_ARGUMENT;return straight(s,r,hit,f,mode);
}
gh_scene_ray_status gh_scene_ray_query_00131e10(gh_game_scene_tables*s,const gh_scene_ray_request*r,int*hit,uint32_t*f){
 if(f)*f=0;if(!s||!r||!hit)return GH_RAY_ARGUMENT;*hit=0;
 if(!r->segmented)return straight(s,r,hit,f,0);
 fenv_t env;if(feholdexcept(&env))return GH_RAY_NUMERIC;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_RAY_NUMERIC;}
 gh_scene_ray_status e=GH_RAY_OK;float d[3],unit[3],square=0;
 for(unsigned k=0;k<3;k++){if(!isfinite(r->from[k])||!isfinite(r->to[k])){e=GH_RAY_NUMERIC;goto out;}d[k]=sub(r->to[k],r->from[k]);}
 square=add(add(mul(d[0],d[0]),mul(d[1],d[1])),mul(d[2],d[2]));float length=sqrtf(square);
 if(!isfinite(length)){e=GH_RAY_NUMERIC;goto out;}if(length==0)goto out;
 float inverse=divide(1,length);for(unsigned k=0;k<3;k++)unit[k]=mul(d[k],inverse);
 float step=divide(8,length);if(step>1)step=1;if(!(step>0)){e=stop(f,0x131fd8,GH_RAY_NUMERIC);goto out;}
 float begin=0,end=step;
 for(unsigned iteration=0;iteration<1000000;iteration++){
  gh_scene_ray_request part=*r;part.segmented=0;float from_distance=mul(length,begin),to_distance=mul(length,end);
  for(unsigned k=0;k<3;k++){part.from[k]=add(r->from[k],mul(unit[k],from_distance));part.to[k]=add(r->from[k],mul(unit[k],to_distance));}part.from[3]=part.to[3]=1;
  e=straight(s,&part,hit,f,1);if(e||*hit||end>=1)goto out;
  float next=sub(add(begin,step),divide(step,100));if(!(next>begin)){e=stop(f,0x131fd8,GH_RAY_NUMERIC);goto out;}begin=next;end=add(begin,step);if(end>1)end=1;
 }
 e=stop(f,0x131fd8,GH_RAY_UNSUPPORTED);
out:if(fesetenv(&env))return GH_RAY_NUMERIC;return e;
}
