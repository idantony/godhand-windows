#include "game_terrain_sphere.h"
#include "game_ray_static.h"
#include "game_terrain_closest.h"
#include "game_scene_dynamic_internal.h"
#include "game_dynamic_corners.h"
#include <math.h>
#include <string.h>
#include <fenv.h>
static uint16_t half(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float value(uint32_t n){float x;memcpy(&x,&n,4);return x;}
static uint32_t bits(float x){uint32_t n;memcpy(&n,&x,4);return n;}
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float divf(float a,float b){volatile float x=a/b;return x;}
static float dot(const float*a,const float*b){return add(add(mul(a[0],b[0]),mul(a[1],b[1])),mul(a[2],b[2]));}
static void cross(const float*a,const float*b,float*out){out[0]=sub(mul(a[1],b[2]),mul(a[2],b[1]));out[1]=sub(mul(a[2],b[0]),mul(a[0],b[2]));out[2]=sub(mul(a[0],b[1]),mul(a[1],b[0]));}
static int normalize(float*v){float n=sqrtf(dot(v,v));if(!isfinite(n)||n<=0)return 0;float inv=divf(1,n);for(unsigned k=0;k<3;k++)v[k]=mul(v[k],inv);return 1;}
static gh_scene_ray_status stop(uint32_t*f,uint32_t pc,gh_scene_ray_status e){if(f)*f=pc;return e;}
static int vertex(gh_scene_table_view v,unsigned index,float*out){size_t offset=(size_t)index*12;if(!v.bytes||offset>v.size||12>v.size-offset)return 0;for(unsigned k=0;k<3;k++){out[k]=value(word(v.bytes+offset+4*k));if(!isfinite(out[k]))return 0;}return 1;}
static int same(const gh_terrain_sphere_request*a,const gh_terrain_sphere_request*b){return a->actor==b->actor&&!memcmp(a->center,b->center,12)&&!memcmp(&a->radius,&b->radius,4)&&a->mask==b->mask&&a->flags==b->flags&&a->reports==b->reports&&a->report_capacity==b->report_capacity&&a->report_count==b->report_count;}
/* Original12FC20 filter, closestpoint and XZ response order. */
static gh_scene_ray_status polygon(gh_terrain_sphere_call*c,uint32_t*f){
 gh_game_scene_tables*s=c->tables;uint32_t flags=c->triangle_flags;gh_scene_table_view triangles=s->actor_links[4].view;
 size_t off=(size_t)c->triangle*20;if(!triangles.bytes||off>triangles.size||20>triangles.size-off)return stop(f,0x12fc20,GH_RAY_BOUNDS);const uint8_t*t=triangles.bytes+off;
 unsigned attr=half(t+12);gh_scene_table_view attrs=s->actor_links[1].view;if(!attrs.bytes||(size_t)attr*16>attrs.size||16>attrs.size-(size_t)attr*16)return stop(f,0x12fc20,GH_RAY_BOUNDS);
 if(word(attrs.bytes+(size_t)attr*16+4)&c->request.flags)return GH_RAY_OK;
 unsigned selectors=half(t+10),n=(flags&1)?4:3;float v[4][3]={{0}},minimum[3],maximum[3];
 for(unsigned i=0;i<4;i++){int needed=i<n;for(unsigned k=0;k<3;k++)needed|=((selectors>>(4*k))&3)==i||((selectors>>(4*k+2))&3)==i;if(needed&&!vertex(s->actor_links[2].view,half(t+i*2),v[i]))return stop(f,0x12fc20,GH_RAY_BOUNDS);}
 float radius=c->request.radius;for(unsigned k=0;k<3;k++){minimum[k]=sub(value(s->ray.hit_position120[k]),radius);maximum[k]=add(value(s->ray.hit_position120[k]),radius);}
 uint32_t oldmin[3],oldmax[3];memcpy(oldmin,s->ray.grid_min6c,12);memcpy(oldmax,s->ray.grid_max78,12);int overlap=0;
 gh_scene_ray_status e=gh_scene_query_grid_0012f288(s,minimum,maximum,&overlap,f);if(e||!overlap)return e;
 if(memcmp(oldmin,s->ray.grid_min6c,12)||memcmp(oldmax,s->ray.grid_max78,12)){e=gh_scene_query_clear_0012f4c8(s,c->request.mask,f);if(e)return e;}
 for(unsigned k=0;k<3;k++){float lo=v[(selectors>>(4*k))&3][k],hi=v[(selectors>>(4*k+2))&3][k];if(maximum[k]<lo||hi<minimum[k])return GH_RAY_OK;}
 float a[3],b[3],normal[3];for(unsigned k=0;k<3;k++){a[k]=sub(v[1][k],v[0][k]);b[k]=sub(v[n==4?3:2][k],v[0][k]);}cross(a,b,normal);
 if(!normalize(normal))return stop(f,0x30a370,GH_RAY_UNSUPPORTED);
 int back=1;for(unsigned i=0;i<n;i++){for(unsigned k=0;k<3;k++)a[k]=sub(v[i][k],value(s->ray.hit_position120[k]));
  float len=sqrtf(dot(a,a));if(!isfinite(len))return GH_RAY_NUMERIC;if(len>0){float inv=divf(1,len);for(unsigned k=0;k<3;k++)a[k]=mul(a[k],inv);}else memset(a,0,12);
  if(!(dot(normal,a)>=0))back=0;
 }if(back)return GH_RAY_OK;
 float center[3],closest[3];for(unsigned k=0;k<3;k++)center[k]=value(s->ray.hit_position120[k]);
 if(n==3){
  float average[3],maximum_distance=0;for(unsigned k=0;k<3;k++)average[k]=mul(add(add(v[0][k],v[1][k]),v[2][k]),0x1.555556p-2f);
  for(unsigned i=0;i<3;i++){for(unsigned k=0;k<3;k++)a[k]=sub(average[k],v[i][k]);float d=dot(a,a);if(maximum_distance<=d)maximum_distance=d;}
  for(unsigned k=0;k<3;k++)a[k]=sub(center[k],average[k]);if(!(dot(a,a)<=add(maximum_distance,mul(radius,radius))))return GH_RAY_OK;
  if(!gh_terrain_closest_00135348(center,v[0],v[1],v[2],closest))return stop(f,0x135348,GH_RAY_UNSUPPORTED);
 }else{
  float other[3];if(!gh_terrain_closest_00135348(center,v[0],v[1],v[3],closest)||!gh_terrain_closest_00135348(center,v[1],v[2],v[3],other))return stop(f,0x135348,GH_RAY_UNSUPPORTED);
  for(unsigned k=0;k<3;k++){a[k]=sub(center[k],closest[k]);b[k]=sub(center[k],other[k]);}if(!(dot(a,a)<=dot(b,b)))memcpy(closest,other,12);
 }
 for(unsigned k=0;k<3;k++)a[k]=sub(center[k],closest[k]);if(!(dot(a,a)<=mul(radius,radius)))return GH_RAY_OK;
 float dx=sub(center[0],closest[0]),dz=sub(center[2],closest[2]);float horizontal=sqrtf(add(mul(dx,dx),mul(dz,dz)));
 /* Original VU divide-zero/special domains remain explicit until profiled. */
 if(!isfinite(horizontal)||!(horizontal>0))return stop(f,0x30a468,GH_RAY_UNSUPPORTED);
 closest[1]=value(s->ray.sphere_center100[1]);float corrected[3];
 for(unsigned k=0;k<3;k++){float delta=sub(closest[k],center[k]);float scaled=mul(delta,radius);corrected[k]=sub(closest[k],mul(scaled,divf(1,horizontal)));if(!isfinite(corrected[k]))return GH_RAY_NUMERIC;}
 if(c->request.reports&&s->ray.triangle17c<c->request.report_capacity){unsigned index=s->ray.triangle17c;c->request.reports[index]=(gh_scene_ray_report){attrs.bytes+(size_t)attr*16,16,c->triangle,(uint16_t)attr};s->ray.triangle17c++;}
 for(unsigned k=0;k<3;k++)s->ray.hit_position120[k]=bits(corrected[k]);s->ray.hit_kind180=1;return GH_RAY_OK;
}
static gh_scene_ray_status dynamic(gh_terrain_sphere_call*c,uint32_t*f){
 gh_game_scene_tables*s=c->tables;if(!s->pending_actor_head&&!s->pending_actor_count)return GH_RAY_OK;
 gh_scene_dynamic_owner*owner=s->dynamic_native_owner;if(!owner||gh_scene_dynamic_validate_owner(owner))return stop(f,0x1311a0,GH_RAY_UNSUPPORTED);
 if(owner->update_active)return stop(f,0x12e840,GH_RAY_UNSUPPORTED);
 if(c->phase==3){c->dynamic_cursor=s->pending_actor_head;c->dynamic_face=0;c->phase=4;}
 while(c->dynamic_cursor){gh_scene_dynamic_box_view view;if(gh_scene_dynamic_box_view_at(owner,c->dynamic_cursor,&view))return stop(f,0x1311a0,GH_RAY_UNSUPPORTED);
  const uint8_t*p=view.bytes;if(view.size<0x30)return GH_RAY_BOUNDS;
  if(c->dynamic_face==0){
   if(!(word(p+8)&1))goto next_box;
   int overlap=1;for(unsigned k=0;k<3;k++){float center=value(s->ray.hit_position120[k]),lo=sub(center,c->request.radius),hi=add(center,c->request.radius),a=value(word(p+0x10+k*4)),b=value(word(p+0x20+k*4));if(!isfinite(a)||!isfinite(b))return GH_RAY_NUMERIC;s->ray.bounds_min90[k]=bits(lo);s->ray.bounds_maxa0[k]=bits(hi);if(hi<a||b<lo)overlap=0;}if(!overlap)goto next_box;
  }
  unsigned kind=word(p+4);if(kind!=1&&kind!=2)goto next_box;
  unsigned faces=kind==1?1:6;
  for(;c->dynamic_face<faces;c->dynamic_face++){
   unsigned face=c->dynamic_face;size_t offset=kind==1?0xa8:0xb0+face*16;if(offset>view.size||16>view.size-offset)return GH_RAY_BOUNDS;const uint8_t*attr=p+offset;if(!(half(attr+2)&c->request.mask)||(word(attr+4)&c->request.flags))continue;
   if((kind==2&&(view.size!=0x190||word(p+0x5c)!=0x41d558))||(kind==1&&(view.size!=0x100||word(p+0x5c)!=0x41d580)))return stop(f,0x1311a0,GH_RAY_UNSUPPORTED);
   if(!view.world_ready)return stop(f,kind==1?0x12d1a8:0x12dcb8,GH_RAY_UNSUPPORTED);
   const gh_dynamic_corner_tables*t=owner->world_tables;if(!t||!t->initialized||t->identity!=t)return stop(f,0x133670,GH_RAY_UNSUPPORTED);
   float v[4][3],a[3],b[3],normal[3],center[3];for(unsigned i=0;i<4;i++){unsigned index=kind==1?t->quad_indices[i]:t->box_faces[face][i];if(index>=(kind==1?4u:8u))return GH_RAY_BOUNDS;for(unsigned k=0;k<3;k++){v[i][k]=value(word(p+(kind==1?0xc0:0x110)+index*16+k*4));if(!isfinite(v[i][k]))return GH_RAY_NUMERIC;}}
   for(unsigned k=0;k<3;k++){a[k]=sub(v[1][k],v[0][k]);b[k]=sub(v[3][k],v[0][k]);center[k]=value(s->ray.hit_position120[k]);}cross(a,b,normal);if(!normalize(normal))return stop(f,0x30a370,GH_RAY_UNSUPPORTED);
   int back=1;for(unsigned i=0;i<4;i++){for(unsigned k=0;k<3;k++)a[k]=sub(v[i][k],center[k]);float len=sqrtf(dot(a,a));if(!isfinite(len))return GH_RAY_NUMERIC;if(len>0){float inv=divf(1,len);for(unsigned k=0;k<3;k++)a[k]=mul(a[k],inv);}else memset(a,0,12);if(!(dot(normal,a)>=0))back=0;}if(back)continue;
   float closest[3],other[3];if(!gh_terrain_closest_00135348(center,v[0],v[1],v[3],closest)||!gh_terrain_closest_00135348(center,v[1],v[2],v[3],other))return stop(f,0x135348,GH_RAY_UNSUPPORTED);
   for(unsigned k=0;k<3;k++){a[k]=sub(center[k],closest[k]);b[k]=sub(center[k],other[k]);}if(!(dot(a,a)<=dot(b,b)))memcpy(closest,other,12);for(unsigned k=0;k<3;k++)a[k]=sub(center[k],closest[k]);if(!(dot(a,a)<=mul(c->request.radius,c->request.radius)))continue;
   float dx=sub(center[0],closest[0]),dz=sub(center[2],closest[2]),horizontal=sqrtf(add(mul(dx,dx),mul(dz,dz)));if(!isfinite(horizontal)||!(horizontal>0))return stop(f,0x30a468,GH_RAY_UNSUPPORTED);
   closest[1]=value(s->ray.sphere_center100[1]);float corrected[3];for(unsigned k=0;k<3;k++){float delta=sub(closest[k],center[k]);corrected[k]=sub(closest[k],mul(mul(delta,c->request.radius),divf(1,horizontal)));if(!isfinite(corrected[k]))return GH_RAY_NUMERIC;}
   /* Fixed eightentry native companion shares original+54 count. Preflight/
    * publication cannot fail after the geometric scalar outputs are committed. */
   int inserted=0;gh_scene_dynamic_status e=gh_scene_dynamic_sphere_contact_001311a0(owner,(struct gh_scene_dynamic_box*)c->dynamic_cursor,c->request.actor,&inserted);if(e)return stop(f,0x1311a0,GH_RAY_UNSUPPORTED);
   for(unsigned k=0;k<3;k++)s->ray.hit_position120[k]=bits(corrected[k]);
   if(c->request.reports&&s->ray.triangle17c<c->request.report_capacity){unsigned index=s->ray.triangle17c;c->request.reports[index]=(gh_scene_ray_report){attr,16,UINT16_MAX,(uint16_t)face};s->ray.triangle17c++;}s->ray.hit_kind180=1;
  }
next_box:c->dynamic_cursor=c->dynamic_cursor->next;c->dynamic_face=0;
 }
 return GH_RAY_OK;
}
static gh_scene_ray_status run(gh_terrain_sphere_call*c,uint32_t*f){
 gh_game_scene_tables*s=c->tables;gh_scene_ray_status e;
 if(c->phase==1){float lo[3],hi[3];s->ray.mask174=c->request.mask;s->ray.flags178=c->request.flags;s->ray.triangle17c=0;s->ray.hit_kind180=0;s->ray.report_limit140=c->request.report_capacity;s->ray.reports18c=c->request.reports;
  for(unsigned k=0;k<3;k++){float x=c->request.center[k];s->ray.sphere_center100[k]=s->ray.hit_position120[k]=bits(x);lo[k]=sub(x,c->request.radius);hi[k]=add(x,c->request.radius);}s->ray.sphere_radius110=bits(c->request.radius);int overlap=0;
  e=gh_scene_query_grid_0012f288(s,lo,hi,&overlap,f);if(e)return e;if(!overlap){c->complete=1;return GH_RAY_OK;}
  e=gh_scene_query_clear_0012f4c8(s,c->request.mask,f);if(e)return e;
  c->x=s->ray.grid_min6c[0];c->y=s->ray.grid_min6c[1];c->z=s->ray.grid_min6c[2];c->phase=2;
 }
 while(c->phase==2){
  uint64_t index=((uint64_t)c->x*s->actor_count64+c->y)*s->actor_count68+c->z;gh_scene_table_view grid=s->actor_links[5].view,list=s->actor_links[6].view;
  if(!grid.bytes||index>grid.size/8||grid.size/8-index<1)return stop(f,0x1321d8,GH_RAY_BOUNDS);const uint8_t*cell=grid.bytes+(size_t)index*8;uint32_t base=word(cell);
  if(base!=UINT32_MAX){for(;c->group<3;c->group++,c->item=0){unsigned count=cell[4+c->group];if(!(c->request.mask&(1u<<c->group)))continue;
   unsigned prefix=0;for(unsigned g=0;g<c->group;g++)prefix+=cell[4+g];uint64_t start=(uint64_t)base+prefix;
   if(!list.bytes||start>list.size/2||count>list.size/2-start)return stop(f,0x1321d8,GH_RAY_BOUNDS);
   for(;c->item<count;c->item++){
    if(!c->pending_triangle){c->triangle=half(list.bytes+((size_t)start+c->item)*2);int skip=0;e=gh_scene_query_visit_0012fc20(s,c->triangle,&skip,&c->triangle_flags,f);if(e)return e;if(skip)continue;c->pending_triangle=1;}
    e=polygon(c,f);if(e)return e;c->pending_triangle=0;
   }
  }}c->group=0;c->item=0;
  if(c->z<s->ray.grid_max78[2])c->z++;else{c->z=s->ray.grid_min6c[2];if(c->y<s->ray.grid_max78[1])c->y++;else{c->y=s->ray.grid_min6c[1];if(c->x<s->ray.grid_max78[0])c->x++;else c->phase=3;}}
 }
 e=dynamic(c,f);if(e)return e;
 if(!c->work)return stop(f,0x1321d8,GH_RAY_UNSUPPORTED);
 if(word((const uint8_t*)c->work+0x594)&0x8000000)return stop(f,0x15f2a8,GH_RAY_UNSUPPORTED);
 c->hit=s->ray.hit_kind180!=0;c->result_count=s->ray.triangle17c;for(unsigned k=0;k<3;k++)c->result[k]=value(s->ray.hit_position120[k]);c->complete=1;return GH_RAY_OK;
}
gh_scene_ray_status gh_terrain_sphere_query_001321d8(void*state,gh_game_scene_tables*s,const gh_terrain_sphere_request*r,float*out,int*hit,uint32_t*f){
 if(f)*f=0;if(!state||!s||!r||!hit)return GH_RAY_ARGUMENT;*hit=0;gh_terrain_sphere_call*c=state;
 if(c->active){if(c->tables!=s||c->akg!=s->akg.bytes||c->dynamic_identity!=s->dynamic_native_owner||!same(&c->request,r))return GH_RAY_ARGUMENT;}
 else {if(!isfinite(r->radius)||r->radius<0)return GH_RAY_NUMERIC;for(unsigned k=0;k<3;k++)if(!isfinite(r->center[k]))return GH_RAY_NUMERIC;const gh_boot_work*work=c->work;memset(c,0,sizeof(*c));c->work=work;c->tables=s;c->akg=s->akg.bytes;c->request=*r;c->dynamic_identity=s->dynamic_native_owner;c->active=1;c->phase=1;if(!s->akg.bytes)c->complete=1;}
 if(!c->complete){fenv_t env;if(feholdexcept(&env))return GH_RAY_NUMERIC;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_RAY_NUMERIC;}gh_scene_ray_status e=run(c,f);if(fesetenv(&env))return GH_RAY_NUMERIC;if(e)return e;}
 *hit=c->hit;if(*hit){if(out)memcpy(out,c->result,12);if(r->report_count)*r->report_count=c->result_count;}return GH_RAY_OK;
}
