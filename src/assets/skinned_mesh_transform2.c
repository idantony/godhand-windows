#include "skinned_mesh_transform2.h"
#include "skinned_mesh_blend.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static float mul(float a,float b){volatile float n=a*b;return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float sub(float a,float b){volatile float n=a-b;return n;}
static float divide(float a,float b){volatile float n=a/b;return n;}
static int finite(const float*p,size_t n){for(size_t i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
static gh_static_mesh_status bad(gh_static_transform_frontier*f,uint32_t pc){if(f){f->ee_entry=0x154e98;f->vu_byte_pc=pc;}return GH_STATIC_UNSUPPORTED;}
static void transform(const float*m,const float*p,float*out){for(unsigned r=0;r<4;r++){float n=mul(m[r],p[0]);n=add(n,mul(m[4+r],p[1]));n=add(n,mul(m[8+r],p[2]));out[r]=add(n,m[12+r]);}}
static int fixed(float n,int32_t*out){n=mul(n,16);if(!isfinite(n)||n< -2147483648.0f||n>=2147483648.0f)return 0;*out=(int32_t)n;return 1;}
static int negative(float n){uint32_t bits;memcpy(&bits,&n,4);return (int)(bits>>31);}
static uint8_t clip_flags(const float*p){uint8_t bits=0;float w=fabsf(p[3]);for(unsigned k=0;k<3;k++){if(p[k]>w)bits|=(uint8_t)(1u<<(k*2));if(p[k]< -w)bits|=(uint8_t)(2u<<(k*2));}return bits;}
static gh_static_mesh_status calculate(const gh_skinned_mesh_stream*s,const gh_skinned_mesh_batch*b,const gh_skinned_program2_inputs*in,gh_static_gs_vertex*out,uint32_t*indices,gh_static_geometry_result*result,gh_static_transform_frontier*f){
 float history[26][3],palette[42][16];size_t triangles=0;memcpy(palette,in->palette,in->part_count*64);
 for(size_t i=0;i<b->logical_vertices;i++){
  gh_skinned_mesh_vertex raw;gh_static_mesh_status e=gh_skinned_mesh_vertex_at(s,b->first_vertex+i,&raw);if(e)return e;
  gh_skinned_blend_result blended;uint32_t pc=0;e=gh_skinned_mesh_blend_program2(&raw,palette,in->part_count,GH_STATIC_BINARY32_REFERENCE,&blended,&pc);if(e){if(f){f->ee_entry=0x154e98;f->vu_byte_pc=pc;}return e;}
  /* ERLENG writes P at238, but this actual program never reads P. The
   * shared blend helper preserves original unnormalized signed8 normals. */
  const float *position=blended.position,*skin_normal=blended.normal;
  float screen[4],clip[4];transform(in->screen,position,screen);transform(in->clip,position,clip);
  if(!finite(screen,4)||!finite(clip,4)||!finite(skin_normal,3))return bad(f,0x290);
  if(!(screen[3]>0))return bad(f,0x290);float q=divide(1,screen[3]);if(!isfinite(q))return bad(f,0x290);
  gh_static_gs_vertex vertex={0};vertex.raw_control=(uint16_t)raw.raw_control;vertex.clip_flags=clip_flags(clip);vertex.adc=(vertex.raw_control&0x8000u)!=0;
  for(unsigned k=0;k<3;k++){history[i][k]=mul(screen[k],q);if(!fixed(history[i][k],vertex.xyz_fixed4+k))return bad(f,0x418);}
  for(unsigned k=0;k<2;k++){vertex.stq[k]=mul(add(divide((float)raw.uv[k],4096),in->uv[k]),q);if(!isfinite(vertex.stq[k]))return bad(f,0x460);}vertex.stq[2]=q;
  float response[4]={0,0,0,1};for(unsigned r=0;r<3;r++){float n=mul(in->normal_light[r],skin_normal[0]);n=add(n,mul(in->normal_light[4+r],skin_normal[1]));response[r]=add(n,mul(in->normal_light[8+r],skin_normal[2]));if(response[r]<0)response[r]=0;}
  float light[4];transform(in->light_color,response,light);
  for(unsigned k=0;k<4;k++){
   float color=mul((float)raw.color[k],in->normal_light[12+k]);
   color=mul(color,k==3?in->light_color[15]:light[k]);
   if(!isfinite(color)||color<0)return bad(f,0x458);if(color>255)color=255;vertex.rgba[k]=(uint32_t)color;
  }
  int32_t fog;if(!fixed(in->fog,&fog)||fog<0||fog>4080)return bad(f,0x418);vertex.fog_fixed4=(uint16_t)fog;
  if(i>=2){vertex.triangle_tested=1;float a[2],c[2];for(unsigned k=0;k<2;k++){a[k]=sub(history[i-2][k],history[i-1][k]);c[k]=sub(history[i][k],history[i-1][k]);}float area=(vertex.raw_control&0x7fffu)?sub(mul(c[0],a[1]),mul(c[1],a[0])):sub(mul(a[0],c[1]),mul(a[1],c[0]));if(!isfinite(area))return bad(f,0x490);if(negative(area)||vertex.clip_flags||out[i-1].clip_flags||out[i-2].clip_flags)vertex.adc=1;
   if(!vertex.adc){uint32_t first=(uint32_t)i-2,second=(uint32_t)i-1;if(i&1u){uint32_t swap=first;first=second;second=swap;}indices[triangles*3]=first;indices[triangles*3+1]=second;indices[triangles*3+2]=(uint32_t)i;triangles++;}
  }
  out[i]=vertex;
 }
 *result=(gh_static_geometry_result){b->logical_vertices,triangles,s->material,(uint16_t)((b->gif_strip[0]>>47)&0x7ff)};return GH_STATIC_OK;
}
gh_static_mesh_status gh_skinned_transform_program2_batch(const gh_skinned_mesh_stream*s,size_t batch,const gh_skinned_program2_inputs*in,gh_static_transform_policy policy,gh_static_gs_vertex*vertices,size_t capacity,uint32_t*indices,size_t index_capacity,gh_static_geometry_result*result,gh_static_transform_frontier*f){
 if(f)*f=(gh_static_transform_frontier){0};if(!s||!in||!vertices||!indices||!result||!in->palette||!in->part_count||in->part_count>42||policy!=GH_STATIC_BINARY32_REFERENCE)return GH_STATIC_ARGUMENT;
 if(in->palette_bytes<in->part_count*64||!finite(in->screen,16)||!finite(in->clip,16)||!finite(in->normal_light,16)||!finite(in->light_color,16)||!finite(in->uv,2)||!isfinite(in->fog))return GH_STATIC_ARGUMENT;
 gh_skinned_mesh_batch b;gh_static_mesh_status e=gh_skinned_mesh_batch_at(s,batch,&b);if(e)return e;if(b.logical_vertices>26)return GH_STATIC_BOUNDS;if(capacity<b.logical_vertices||index_capacity<(b.logical_vertices>2?(b.logical_vertices-2)*3:0))return GH_STATIC_BOUNDS;
 gh_static_gs_vertex output[26];uint32_t triangles[72];gh_static_geometry_result done;fenv_t env;if(feholdexcept(&env))return bad(f,0);if(fesetround(FE_TONEAREST)){fesetenv(&env);return bad(f,0);}
 e=calculate(s,&b,in,output,triangles,&done,f);if(fesetenv(&env))return bad(f,0);if(!e){memcpy(vertices,output,done.vertices*sizeof(*output));memcpy(indices,triangles,done.triangles*3*sizeof(*triangles));*result=done;}return e;
}
