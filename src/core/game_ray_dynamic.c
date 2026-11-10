#include "game_ray_dynamic.h"
#include "game_scene_dynamic_internal.h"
#include "game_dynamic_corners.h"
#include "game_ray_static.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static float val(uint32_t n){float f;memcpy(&f,&n,4);return f;}
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float dot(const float*a,const float*b){return add(add(mul(a[0],b[0]),mul(a[1],b[1])),mul(a[2],b[2]));}
static void cross(const float*a,const float*b,float*c){c[0]=sub(mul(a[1],b[2]),mul(a[2],b[1]));c[1]=sub(mul(a[2],b[0]),mul(a[0],b[2]));c[2]=sub(mul(a[0],b[1]),mul(a[1],b[0]));}
static int unit(float*v){float d=dot(v,v);if(!isfinite(d)||d<=0)return 0;float n=sqrtf(d);volatile float inverse=1.0f/n;for(unsigned k=0;k<3;k++){float x=mul(v[k],inverse);v[k]=x;if(!isfinite(x))return 0;}return 1;}
static gh_scene_ray_status run(gh_scene_dynamic_owner*s,const gh_scene_ray_request*r,uint32_t*f){
 gh_scene_ray_state*q=&s->tables->ray;gh_ray_dynamic_reports*d=r->dynamic_reports;if(d&&d->count)*d->count=0;
 if(s->update_active){*f=0x12e840;return GH_RAY_UNSUPPORTED;}
 for(struct gh_scene_dynamic_box*b=s->tables->pending_actor_head;b;b=b->next){
  const uint8_t*p=b->allocation.pointer;if(!(word(p+8)&1))continue;
  if(!b->world_ready){*f=0x12dcb8;return GH_RAY_UNSUPPORTED;}
  int overlap=1;for(unsigned k=0;k<3;k++){float lo=val(q->bounds_min90[k]),hi=val(q->bounds_maxa0[k]),a=val(word(p+0x10+k*4)),z=val(word(p+0x20+k*4));if(!isfinite(lo)||!isfinite(hi)||!isfinite(a)||!isfinite(z))return GH_RAY_NUMERIC;if(hi<a||z<lo)overlap=0;}if(!overlap)continue;
  if(word(p+4)!=2||word(p+0x5c)!=0x41d558){*f=0x130700;return GH_RAY_UNSUPPORTED;}
  const gh_dynamic_corner_tables*t=s->world_tables;if(!t||t->identity!=t||!t->initialized){*f=0x133670;return GH_RAY_UNSUPPORTED;}
  for(unsigned face=0;face<6;face++){
   const uint8_t*attr=p+0xb0+face*16;uint16_t mask;memcpy(&mask,attr+2,2);if(!(mask&q->mask174)||(word(attr+4)&q->flags178))continue;
   float v[4][3],a[3],z[3],n[3],direction[3],from[3],to[3],hit[3];for(unsigned i=0;i<4;i++){unsigned index=t->box_faces[face][i];if(index>=8)return GH_RAY_BOUNDS;memcpy(v[i],p+0x110+index*16,12);for(unsigned k=0;k<3;k++)if(!isfinite(v[i][k]))return GH_RAY_NUMERIC;}
   for(unsigned k=0;k<3;k++){a[k]=sub(v[1][k],v[0][k]);z[k]=sub(v[3][k],v[0][k]);direction[k]=val(q->directiond0[k]);from[k]=val(q->frome0[k]);to[k]=val(q->tof0[k]);if(!isfinite(direction[k])||!isfinite(from[k])||!isfinite(to[k]))return GH_RAY_NUMERIC;}
   if(!unit(a)||!unit(z))return GH_RAY_NUMERIC;cross(a,z,n);float facing=dot(direction,n);if(!isfinite(facing))return GH_RAY_NUMERIC;if(!(facing<0))continue;
   int intersects=gh_scene_ray_quad_intersection_001365b8(from,to,v,n,hit);if(intersects<0)return GH_RAY_NUMERIC;if(!intersects)continue;
   if(q->reports18c&&(q->mode181||q->report_limit140)){
    if(!r->native_reports){*f=0x130700;return GH_RAY_UNSUPPORTED;}uint32_t index=q->mode181?0:q->triangle17c;uint32_t capacity=q->mode181?1:(r->report_limit>0?(uint32_t)r->report_limit:0);if(index>=capacity)return GH_RAY_BOUNDS;
    r->native_reports[index]=(gh_scene_ray_report){attr,16,UINT16_MAX,(uint16_t)face};if(q->mode181)q->triangle17c=1;else{q->triangle17c++;q->report_limit140--;}
   }
   if(d&&d->object_identities&&d->count&&d->capacity&&b->actor&&b->actor->bytes){const void*identity=b->actor->bytes;if(q->mode181){d->object_identities[0]=identity;*d->count=1;}else if(*d->count<d->capacity){int duplicate=0;for(uint32_t i=0;i<*d->count;i++)if(d->object_identities[i]==identity)duplicate=1;if(!duplicate)d->object_identities[(*d->count)++]=identity;}}
   float delta[3];for(unsigned k=0;k<3;k++)delta[k]=sub(from[k],hit[k]);float distance=dot(delta,delta);if(!isfinite(distance)||!isfinite(val(q->distance188)))return GH_RAY_NUMERIC;q->distance184=bits(distance);
   if(distance<val(q->distance188)){for(unsigned k=0;k<3;k++){a[k]=sub(v[1][k],v[0][k]);z[k]=sub(v[3][k],v[0][k]);}cross(a,z,n);if(!unit(n))return GH_RAY_NUMERIC;q->distance188=bits(distance);for(unsigned k=0;k<3;k++){q->hit_position120[k]=bits(hit[k]);q->hit_normal130[k]=bits(n[k]);if(q->mode181)q->tof0[k]=bits(hit[k]);}}
   q->hit_kind180=1;
  }
 }
 return GH_RAY_OK;
}
gh_scene_ray_status gh_scene_ray_dynamic_00130700(gh_scene_dynamic_owner*s,const gh_scene_ray_request*r,uint32_t*f){
 uint32_t local=0;if(!f)f=&local;*f=0;if(!s||!r)return GH_RAY_ARGUMENT;if(gh_scene_dynamic_validate_owner(s)){*f=0x130700;return GH_RAY_UNSUPPORTED;}
 fenv_t env;if(feholdexcept(&env))return GH_RAY_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_RAY_NUMERIC;}gh_scene_ray_status e=run(s,r,f);if(fesetenv(&env))return GH_RAY_NUMERIC;return e;
}
