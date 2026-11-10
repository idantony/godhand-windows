/* Actual12F288/12F4C8/12F600/12F750 and polygon helpers135EC8/136030/
 *1361A0/136458/1365B8. Explicit finite binary32 reference, not EE/VU bits.
 * Native visited bytes represent ONLY original per-triangle flag mask4;
 * all other flags are read from the canonical, possibly modified link4 view. */
#include "game_ray_static.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
struct gh_ray_triangle_state {const uint8_t *akg;size_t count;uint8_t *visited;};
static uint16_t h(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t w(const uint8_t*p){return (uint32_t)h(p)|(uint32_t)h(p+2)<<16;}
static float rf(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static float from_bits(uint32_t n){float x;memcpy(&x,&n,4);return x;}
static uint32_t bits(float x){uint32_t n;memcpy(&n,&x,4);return n;}
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float divf(float a,float b){volatile float x=a/b;return x;}
static float dot(const float*a,const float*b){return add(add(mul(a[0],b[0]),mul(a[1],b[1])),mul(a[2],b[2]));}
static void delta(float*out,const float*a,const float*b){for(unsigned k=0;k<3;k++)out[k]=sub(a[k],b[k]);}
static void cross(float*out,const float*a,const float*b){float t[3]={sub(mul(a[1],b[2]),mul(a[2],b[1])),sub(mul(a[2],b[0]),mul(a[0],b[2])),sub(mul(a[0],b[1]),mul(a[1],b[0]))};memcpy(out,t,sizeof(t));}
static int normalize(float*out,const float*in){float d=dot(in,in);if(!isfinite(d)||d<0)return 0;float len=sqrtf(d);if(!isfinite(len))return 0;float inv=len>0?divf(1,len):0;for(unsigned k=0;k<3;k++)out[k]=mul(in[k],inv);return 1;}
static int vector(gh_scene_table_view v,uint16_t index,float*out){size_t o=(size_t)index*12;if(!v.bytes||o>v.size||12>v.size-o)return 0;for(unsigned k=0;k<3;k++){out[k]=rf(v.bytes+o+k*4);if(!isfinite(out[k]))return 0;}return 1;}
static gh_scene_ray_status stop(uint32_t*f,uint32_t pc,gh_scene_ray_status e){if(f)*f=pc;return e;}
void gh_scene_ray_triangle_state_release(gh_game_scene_tables*s){if(s&&s->ray.triangle_state){free(s->ray.triangle_state->visited);free(s->ray.triangle_state);s->ray.triangle_state=NULL;}}
gh_scene_ray_status gh_scene_ray_triangle_flags(const gh_game_scene_tables*s,uint16_t index,uint32_t*out){
 if(!s||!out||!s->akg.bytes||s->akg.size<0x2a)return GH_RAY_ARGUMENT;
 size_t count=h(s->akg.bytes+0x28);gh_scene_table_view tri=s->actor_links[4].view;if(index>=count||!tri.bytes||count>tri.size/20)return GH_RAY_BOUNDS;
 uint32_t flags=w(tri.bytes+(size_t)index*20+16);const struct gh_ray_triangle_state*t=s->ray.triangle_state;
 if(t){if(t->akg!=s->akg.bytes||t->count!=count)return GH_RAY_ARGUMENT;flags=(flags&~4u)|(t->visited[index]?4u:0);}
 *out=flags;return GH_RAY_OK;
}
static gh_scene_ray_status marks(gh_game_scene_tables*s){
 if(!s->akg.bytes||s->akg.size<0x2a)return GH_RAY_BOUNDS;size_t n=h(s->akg.bytes+0x28);gh_scene_table_view tri=s->actor_links[4].view;
 if(n&&(!tri.bytes||n>tri.size/20))return GH_RAY_BOUNDS;
 if(s->ray.triangle_state)return s->ray.triangle_state->akg==s->akg.bytes&&s->ray.triangle_state->count==n?GH_RAY_OK:GH_RAY_ARGUMENT;
 struct gh_ray_triangle_state*t=calloc(1,sizeof(*t));if(!t)return GH_RAY_MEMORY;t->visited=calloc(n?n:1,1);if(!t->visited){free(t);return GH_RAY_MEMORY;}
 t->akg=s->akg.bytes;t->count=n;for(size_t i=0;i<n;i++)t->visited[i]=(uint8_t)((w(tri.bytes+i*20+16)&4)!=0);s->ray.triangle_state=t;return GH_RAY_OK;
}
/* Return -1 only for a native numeric limitation; original false/true remain
 *0/1. Epsilon fallback checks the LAST failed dot, exactly as original. */
static int inside(const float v[4][3],unsigned n,const float*p){
 float radial[4][3],reference[3],a[3],b[3],c[3];for(unsigned i=0;i<n;i++){delta(a,v[i],p);if(!normalize(radial[i],a))return -1;}
 if(n==3){cross(reference,radial[1],radial[2]);cross(a,radial[2],radial[0]);float f=dot(reference,a);if(!isfinite(f))return -1;if(f>=0){cross(a,radial[0],radial[1]);f=dot(reference,a);if(!isfinite(f))return -1;if(f>=0)return 1;}return fabsf(f)<0.000001f;}
 delta(a,v[1],v[0]);delta(b,v[3],v[0]);if(!normalize(a,a)||!normalize(b,b))return -1;cross(reference,a,b);
 for(unsigned i=0;i<4;i++){cross(c,radial[i],radial[(i+1)&3]);float f=dot(reference,c);if(!isfinite(f))return -1;if(f<0)return fabsf(f)<0.000001f;}return 1;
}
static int intersect(const float*from,const float*to,const float v[4][3],unsigned n,const float*normal,float*out){
 float plane[3],direction[3];if(n==3){float a[3],b[3];delta(a,v[1],v[0]);delta(b,v[2],v[0]);cross(plane,a,b);delta(direction,to,from);if(!(dot(plane,direction)<0))return 0;}else memcpy(plane,normal,12);
 float unit[3];if(!normalize(unit,plane))return -1;delta(direction,to,from);float denominator=dot(unit,direction);if(!isfinite(denominator))return -1;if(denominator==0)return 0;
 float t=divf(sub(dot(unit,v[0]),dot(unit,from)),denominator);if(!isfinite(t))return -1;if(t<0||t>1)return 0;
 for(unsigned k=0;k<3;k++){out[k]=add(from[k],mul(direction[k],t));if(!isfinite(out[k]))return -1;}return inside(v,n,out);
}
static gh_scene_ray_status triangle(gh_game_scene_tables*s,const gh_scene_ray_request*r,uint16_t id,uint32_t*f){
 uint32_t flags;gh_scene_ray_status e=gh_scene_ray_triangle_flags(s,id,&flags);if(e)return stop(f,0x12f750,e);if(flags&6)return GH_RAY_OK;
 s->ray.triangle_state->visited[id]=1;const uint8_t*t=s->actor_links[4].view.bytes+(size_t)id*20;uint16_t attr=h(t+12),normal_id=h(t+8);
 gh_scene_table_view attributes=s->actor_links[1].view;size_t off=(size_t)attr*16;
 if(!attributes.bytes||off>attributes.size||16>attributes.size-off)return stop(f,0x12f750,GH_RAY_BOUNDS);
 const uint8_t*a=attributes.bytes+off;if(w(a+4)&r->flags)return GH_RAY_OK;
 float normal[3],dir[3];if(!vector(s->actor_links[3].view,normal_id,normal))return stop(f,0x12f750,GH_RAY_BOUNDS);
 for(unsigned k=0;k<3;k++)dir[k]=from_bits(s->ray.directiond0[k]);if(!(dot(dir,normal)<0))return GH_RAY_OK;
 unsigned n=(flags&1)?4:3;uint16_t selectors=h(t+10);float v[4][3]={{0}};
 for(unsigned i=0;i<4;i++){int needed=i<n;for(unsigned k=0;k<3;k++)needed|=((selectors>>(k*4))&3)==i||((selectors>>(k*4+2))&3)==i;
  if(needed&&!vector(s->actor_links[2].view,h(t+2*i),v[i]))return stop(f,0x12f750,GH_RAY_BOUNDS);}
 float minimum[3],maximum[3];
 float from[3],to[3];for(unsigned k=0;k<3;k++){from[k]=from_bits(s->ray.frome0[k]);to[k]=from_bits(s->ray.tof0[k]);float lo=from[k],hi=to[k];if(lo>hi){float x=lo;lo=hi;hi=x;}s->ray.bounds_min90[k]=bits(lo);s->ray.bounds_maxa0[k]=bits(hi);}
 for(unsigned k=0;k<3;k++){unsigned lo=(selectors>>(k*4))&3,hi=(selectors>>(k*4+2))&3;minimum[k]=v[lo][k];maximum[k]=v[hi][k];if(from[k]>maximum[k]&&to[k]>maximum[k])return GH_RAY_OK;if(from[k]<minimum[k]&&to[k]<minimum[k])return GH_RAY_OK;}
 float p[3];int yes=intersect(from,to,v,n,normal,p);if(yes<0)return stop(f,n==3?0x136458:0x1365b8,GH_RAY_NUMERIC);if(!yes)return GH_RAY_OK;
 if(r->reports&&!r->native_reports&&(s->ray.report_limit140||s->ray.mode181))return stop(f,0x12f750,GH_RAY_UNSUPPORTED);
 if(r->native_reports&&(s->ray.report_limit140||s->ray.mode181)){size_t i=s->ray.mode181?0:s->ray.triangle17c;r->native_reports[i]=(gh_scene_ray_report){a,16,id,attr};if(s->ray.mode181)s->ray.triangle17c=1;else {s->ray.triangle17c++;s->ray.report_limit140--;}}
 float d[3];delta(d,from,p);float distance=dot(d,d);if(!isfinite(distance))return stop(f,0x12f750,GH_RAY_NUMERIC);s->ray.distance184=bits(distance);
 if(distance<=from_bits(s->ray.distance188)){s->ray.distance188=bits(distance);for(unsigned k=0;k<3;k++){s->ray.hit_position120[k]=bits(p[k]);s->ray.hit_normal130[k]=bits(normal[k]);if(s->ray.mode181)s->ray.tof0[k]=bits(p[k]);}}
 s->ray.hit_kind180=1;return GH_RAY_OK;
}
static gh_scene_ray_status cell(gh_game_scene_tables*s,const gh_scene_ray_request*r,uint32_t x,uint32_t y,uint32_t z,int clear,uint32_t*f){
 uint64_t index=((uint64_t)x*s->actor_count64+y)*s->actor_count68+z;gh_scene_table_view grid=s->actor_links[5].view,list=s->actor_links[6].view;
 if(!grid.bytes||grid.size<8||index>(grid.size-8)/8)return stop(f,0x12f4c8,GH_RAY_BOUNDS);const uint8_t*c=grid.bytes+(size_t)index*8;uint32_t base=w(c);if(base==UINT32_MAX)return GH_RAY_OK;
 uint32_t start=0;for(unsigned group=0;group<3;group++){unsigned count=c[4+group];if(r->mask&(1u<<group)){
  uint64_t begin=(uint64_t)base+start;if(!list.bytes||begin>list.size/2||count>list.size/2-begin)return stop(f,clear?0x12f600:0x12ed60,GH_RAY_BOUNDS);
  for(unsigned i=0;i<count;i++){uint16_t id=h(list.bytes+((size_t)begin+i)*2);if(id>=s->ray.triangle_state->count)return stop(f,0x12f600,GH_RAY_BOUNDS);
   if(clear)s->ray.triangle_state->visited[id]=0;else {gh_scene_ray_status e=triangle(s,r,id,f);if(e)return e;}}
 }start+=count;}return GH_RAY_OK;
}
gh_scene_ray_status gh_scene_ray_static_overlap(gh_game_scene_tables*s,const gh_scene_ray_request*r,int*hit,uint32_t*f){
 if(!s||!r||!hit)return GH_RAY_ARGUMENT;fenv_t env;if(feholdexcept(&env))return GH_RAY_NUMERIC;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_RAY_NUMERIC;}gh_scene_ray_status e=GH_RAY_OK;
 gh_scene_ray_request query=*r;gh_scene_ray_report *reports=NULL;
 uint32_t counts[3]={s->actor_count60,s->actor_count64,s->actor_count68};
 for(unsigned k=0;k<3;k++){float size=from_bits(s->actor_vec50[k]);float n=divf(sub(from_bits(s->ray.bounds_min90[k]),from_bits(s->actor_vec30[k])),size);
  if(!isfinite(size)||size<=0||!isfinite(n)||n<=-2147483648.0f||n>=2147483648.0f){e=stop(f,0x12f318,GH_RAY_NUMERIC);goto out;}int32_t value=(int32_t)n;s->ray.grid_min6c[k]=value<0?0:(uint32_t)value;}
 for(unsigned k=0;k<3;k++)if(s->ray.grid_min6c[k]>=counts[k])goto out;
 for(unsigned k=0;k<3;k++){float n=divf(sub(from_bits(s->ray.bounds_maxa0[k]),from_bits(s->actor_vec30[k])),from_bits(s->actor_vec50[k]));if(!isfinite(n)||n<=-2147483648.0f||n>=2147483648.0f){e=stop(f,0x12f318,GH_RAY_NUMERIC);goto out;}uint32_t value=(uint32_t)(int32_t)n;s->ray.grid_max78[k]=value>counts[k]-1?counts[k]-1:value;}
 if((r->native_reports||r->reports)&&r->report_limit<0){e=GH_RAY_ARGUMENT;goto out;}e=marks(s);if(e)goto out;
 if(r->native_reports&&(r->report_limit||s->ray.mode181)){size_t cap=s->ray.mode181?1:(uint32_t)r->report_limit;if(cap>s->ray.triangle_state->count)cap=s->ray.triangle_state->count;if(cap){reports=calloc(cap,sizeof(*reports));if(!reports){e=GH_RAY_MEMORY;goto out;}}query.native_reports=reports;}
 /* Native-error adaptation: caller outputs commit only after the complete
  * query succeeds; original successful report order/count remain unchanged. */
 for(unsigned pass=0;pass<2;pass++)for(uint32_t x=s->ray.grid_min6c[0];x<=s->ray.grid_max78[0];x++)for(uint32_t y=s->ray.grid_min6c[1];y<=s->ray.grid_max78[1];y++)for(uint32_t z=s->ray.grid_min6c[2];z<=s->ray.grid_max78[2];z++){e=cell(s,&query,x,y,z,pass==0,f);if(e)goto out;}
 /* Legacyhost names denote the original LIVE dynamic shape190/194 list;
  * 12ECF0 destroys it on reset, while130700 traverses it for every grid hit. */
 if(s->pending_actor_head||s->pending_actor_count){e=stop(f,0x130700,GH_RAY_UNSUPPORTED);goto out;}
 if(s->ray.hit_kind180){float unit[3];if(r->hit_normal){float n[3];for(unsigned k=0;k<3;k++)n[k]=from_bits(s->ray.hit_normal130[k]);if(!normalize(unit,n)){e=GH_RAY_NUMERIC;goto out;}}
  if(r->hit_position)for(unsigned k=0;k<3;k++)r->hit_position[k]=from_bits(s->ray.hit_position120[k]);if(r->hit_normal)memcpy(r->hit_normal,unit,12);
  if(reports&&s->ray.triangle17c)memcpy(r->native_reports,reports,(size_t)s->ray.triangle17c*sizeof(*reports));*hit=1;}
 if(r->dynamic_reports&&r->dynamic_reports->count)*r->dynamic_reports->count=0;
out:free(reports);if(fesetenv(&env))return GH_RAY_NUMERIC;return e;
}
