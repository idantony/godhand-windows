/* Original1F92C0; six face winding records from actualreadonly42C380.
 * Not a generic OBB-volume overlap: only segment crossings INTO a face count. */
#include "game_scene_box_visibility.h"
#include "game_ray_static.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static float value(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static float add(float a,float b){volatile float n=a+b;return n;}static float sub(float a,float b){volatile float n=a-b;return n;}static float mul(float a,float b){volatile float n=a*b;return n;}
static void transform(float*out,const float*m,const float*p){for(unsigned k=0;k<3;k++)out[k]=add(add(add(mul(m[k],p[0]),mul(m[k+4],p[1])),mul(m[k+8],p[2])),m[k+12]);}
static int normalize(float*v){float squared=add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]));if(!isfinite(squared)||squared<0)return 0;float length=sqrtf(squared);if(length>0){volatile float inverse=1.0f/length;for(unsigned k=0;k<3;k++)v[k]=mul(v[k],inverse);}else memset(v,0,12);return isfinite(v[0])&&isfinite(v[1])&&isfinite(v[2]);}
static gh_scene_post_status body(const uint8_t*b,const uint8_t*raw,const gh_scene_model_ray*r,size_t n,int*hit,uint32_t*f){
 float box[6],m[16],center[3],vertices[8][3];for(unsigned i=0;i<6;i++){box[i]=value(b+4*i);if(!isfinite(box[i])){*f=0x1f92c0;return GH_POST_NUMERIC;}}for(unsigned i=0;i<16;i++){m[i]=value(raw+4*i);if(!isfinite(m[i])){*f=0x30a2b0;return GH_POST_NUMERIC;}}
 transform(center,m,box);for(unsigned k=0;k<3;k++){m[k+12]=center[k];if(!isfinite(center[k])){*f=0x30a2b0;return GH_POST_NUMERIC;}}
 for(unsigned i=0;i<8;i++){float p[3]={(i&1)?box[3]:-box[3],(i&4)?-box[4]:box[4],(i&2)?box[5]:-box[5]};transform(vertices[i],m,p);for(unsigned k=0;k<3;k++)if(!isfinite(vertices[i][k])){*f=0x30a2b0;return GH_POST_NUMERIC;}}
 static const uint8_t faces[6][4]={{4,5,1,0},{2,3,7,6},{0,2,6,4},{5,7,3,1},{0,1,3,2},{6,7,5,4}};
 for(unsigned face=0;face<6;face++){float quad[4][3],a[3],z[3],normal[3];for(unsigned i=0;i<4;i++)memcpy(quad[i],vertices[faces[face][i]],12);for(unsigned k=0;k<3;k++){a[k]=sub(quad[0][k],quad[1][k]);z[k]=sub(quad[2][k],quad[1][k]);}if(!normalize(a)||!normalize(z)){*f=0x101e28;return GH_POST_NUMERIC;}normal[0]=sub(mul(a[1],z[2]),mul(a[2],z[1]));normal[1]=sub(mul(a[2],z[0]),mul(a[0],z[2]));normal[2]=sub(mul(a[0],z[1]),mul(a[1],z[0]));if(!normalize(normal)){*f=0x101e28;return GH_POST_NUMERIC;}
  for(size_t ray=0;ray<n;ray++){for(unsigned k=0;k<3;k++)if(!isfinite(r[ray].from[k])||!isfinite(r[ray].to[k])){*f=0x1365b8;return GH_POST_NUMERIC;}float point[3];int result=gh_scene_ray_quad_intersection_001365b8(r[ray].from,r[ray].to,quad,normal,point);if(result<0){*f=0x1365b8;return GH_POST_UNSUPPORTED;}if(result){float d[3];for(unsigned k=0;k<3;k++)d[k]=sub(r[ray].to[k],r[ray].from[k]);float dot=add(add(mul(d[0],normal[0]),mul(d[1],normal[1])),mul(d[2],normal[2]));if(dot<0){*hit=1;return GH_POST_OK;}}}
 }*hit=0;return GH_POST_OK;
}
gh_scene_post_status gh_scene_box_rays_001f92c0(const void*box,const void*matrix,const gh_scene_model_ray*r,size_t count,gh_static_transform_policy policy,int*out,uint32_t*f){
 if(f)*f=0;if(!box||!matrix||!out||!f||(!r&&count))return GH_POST_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE||count>5){*f=0x1f92c0;return GH_POST_UNSUPPORTED;}fenv_t env;if(feholdexcept(&env))return GH_POST_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_POST_NUMERIC;}int hit=0;gh_scene_post_status e=body(box,matrix,r,count,&hit,f);if(fesetenv(&env))return GH_POST_NUMERIC;if(!e)*out=hit;return e;
}
