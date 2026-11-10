/* Readable actual program10 polygon math; no VU instruction execution.
 * VU0448..0CA8 sixplanes/intersections,0CB8..0FC8 projectedfan emission. */
#include "static_mesh_clip10.h"
#include <math.h>
#include <fenv.h>
#include <string.h>
static float mul(float a,float b){volatile float r=a*b;return r;}
static float add(float a,float b){volatile float r=a+b;return r;}
static float sub(float a,float b){volatile float r=a-b;return r;}
static float divide(float a,float b){volatile float r=a/b;return r;}
static int finite(const float*p,size_t n){for(size_t i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
static gh_static_mesh_status unsupported(gh_static_transform_frontier*f,uint32_t pc){if(f)*f=(gh_static_transform_frontier){0x155800,pc};return GH_STATIC_UNSUPPORTED;}
typedef struct {float clip[4],rgba[4],uv[2];} polygon_vertex;
static void project(const float m[16],const float p[4],float out[4]){
 for(unsigned j=0;j<4;j++){float x=mul(m[j],p[0]);x=add(x,mul(m[4+j],p[1]));x=add(x,mul(m[8+j],p[2]));out[j]=add(x,mul(m[12+j],p[3]));}
}
static int sign(float x){uint32_t b;memcpy(&b,&x,4);return (int)(b>>31);}
static int fixed(float x,int32_t*out){float y=mul(x,16);if(!isfinite(y)||y< -2147483648.0f||y>=2147483648.0f)return 0;*out=(int32_t)y;return 1;}
static gh_static_mesh_status calculate(const gh_static_clip10_vertex tri[3],const gh_static_clip10_inputs*in,
 gh_static_gs_vertex*out,uint32_t*idx,gh_static_geometry_result*r,gh_static_transform_frontier*f){
 polygon_vertex a[12],b[12];size_t count=3;
 for(unsigned i=0;i<3;i++){memcpy(a[i].clip,tri[i].clip,16);memcpy(a[i].uv,tri[i].uv,8);for(unsigned j=0;j<4;j++)a[i].rgba[j]=(float)tri[i].rgba[j];}
 /* Original plane order and signed distances after MR32 axis selection.
  * Zero lies inside on either plane (CLIP uses strict comparisons). */
 const unsigned axis[6]={2,2,0,0,1,1};const float side[6]={-1,1,-1,1,-1,1};
 for(unsigned plane=0;plane<6&&count;plane++){
  size_t n=0;
  for(size_t i=0;i<count;i++){
   const polygon_vertex*p=a+i,*q=a+(i+1)%count;
   float dp=sub(p->clip[axis[plane]],mul(p->clip[3],side[plane]));
   float dq=sub(q->clip[axis[plane]],mul(q->clip[3],side[plane]));
   int pin=side[plane]<0?dp>=0:dp<=0,qin=side[plane]<0?dq>=0:dq<=0;
   if(!isfinite(dp)||!isfinite(dq))return unsupported(f,0x0bc8);
   if(pin){if(n>=12)return GH_STATIC_BOUNDS;b[n++]=*p;}
   if(pin!=qin){
    float t=fabsf(divide(dp,sub(dq,dp)));if(!isfinite(t))return unsupported(f,0x0c40);
    if(n>=12)return GH_STATIC_BOUNDS;polygon_vertex*v=b+n++;
    for(unsigned j=0;j<4;j++){v->clip[j]=add(mul(sub(q->clip[j],p->clip[j]),t),p->clip[j]);v->rgba[j]=add(mul(sub(q->rgba[j],p->rgba[j]),t),p->rgba[j]);}
    for(unsigned j=0;j<2;j++)v->uv[j]=add(mul(sub(q->uv[j],p->uv[j]),t),p->uv[j]);
   }
  }
  memcpy(a,b,n*sizeof(*a));count=n;
 }
 if(count<3){r->vertices=r->triangles=0;return GH_STATIC_OK;}
 if(count>9)return GH_STATIC_BOUNDS;
 float screen[9][4],xyz[9][3],reciprocal[9];
 for(size_t i=0;i<count;i++){
  project(in->projection,a[i].clip,screen[i]);if(!finite(screen[i],4))return unsupported(f,0x0d28);
  if(!(screen[i][3]>0))return unsupported(f,0x0d48);reciprocal[i]=divide(1,screen[i][3]);
  for(unsigned j=0;j<3;j++)xyz[i][j]=mul(screen[i][j],reciprocal[i]);
 }
 float x0=sub(xyz[0][0],xyz[1][0]),y0=sub(xyz[0][1],xyz[1][1]);
 float x2=sub(xyz[2][0],xyz[1][0]),y2=sub(xyz[2][1],xyz[1][1]);
 float area=sub(mul(x2,y0),mul(y2,x0));if(!isfinite(area))return unsupported(f,0x0e38);
 if(sign(area)){r->vertices=r->triangles=0;return GH_STATIC_OK;}
 for(size_t i=0;i<count;i++){
  gh_static_gs_vertex v={0};for(unsigned j=0;j<3;j++)if(!fixed(xyz[i][j],v.xyz_fixed4+j))return unsupported(f,0x0f50);
  for(unsigned j=0;j<2;j++){v.stq[j]=mul(a[i].uv[j],reciprocal[i]);if(!isfinite(v.stq[j]))return unsupported(f,0x0f00);}v.stq[2]=reciprocal[i];
  for(unsigned j=0;j<4;j++){if(!isfinite(a[i].rgba[j])||a[i].rgba[j]<0||a[i].rgba[j]>255)return unsupported(f,0x0ef8);v.rgba[j]=(uint32_t)a[i].rgba[j];}
  float fog=add(in->fog[0],mul(in->fog[1],screen[i][3]));if(!isfinite(fog))return unsupported(f,0x0ef0);if(fog>255)fog=255;if(fog<0)fog=0;
  int32_t fi;if(!fixed(fog,&fi))return unsupported(f,0x0f50);v.fog_fixed4=(uint16_t)fi;v.triangle_tested=i>=2;out[i]=v;
 }
 for(size_t i=2;i<count;i++){idx[(i-2)*3]=0;idx[(i-2)*3+1]=(uint32_t)i-1;idx[(i-2)*3+2]=(uint32_t)i;}
 r->vertices=count;r->triangles=count-2;return GH_STATIC_OK;
}
gh_static_mesh_status gh_static_clip10_triangle(const gh_static_clip10_vertex tri[3],const gh_static_clip10_inputs*in,uint16_t primitive,gh_static_transform_policy policy,
 gh_static_gs_vertex*out,size_t capacity,uint32_t*indices,size_t index_capacity,gh_static_geometry_result*r,gh_static_transform_frontier*f){
 if(f)*f=(gh_static_transform_frontier){0};if(!tri||!in||!out||!indices||!r)return GH_STATIC_ARGUMENT;
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return unsupported(f,0x0448);
 if(primitive!=0x7d&&primitive!=0x5d)return GH_STATIC_ARGUMENT;
 if(!finite(in->projection,16)||!finite(in->fog,2))return GH_STATIC_ARGUMENT;
 for(unsigned i=0;i<3;i++){if(!finite(tri[i].clip,4)||!finite(tri[i].uv,2))return GH_STATIC_ARGUMENT;for(unsigned j=0;j<4;j++)if(tri[i].rgba[j]>255)return GH_STATIC_ARGUMENT;}
 gh_static_gs_vertex tmp[9];uint32_t ix[21];gh_static_geometry_result result={.primitive=primitive};
 fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return unsupported(f,0x0448);
 gh_static_mesh_status status=calculate(tri,in,tmp,ix,&result,f);int restored=fesetenv(&saved);if(restored)return unsupported(f,0x0448);if(status)return status;
 if(capacity<result.vertices||index_capacity<result.triangles*3)return GH_STATIC_BOUNDS;
 memcpy(out,tmp,result.vertices*sizeof(*out));memcpy(indices,ix,result.triangles*3*sizeof(*indices));*r=result;return GH_STATIC_OK;
}
