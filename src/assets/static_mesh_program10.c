/* Program10 MSCAL0 non-clipped algorithm, independently traced at VU0000..
 * 0360. This is readable geometry math, not an instruction interpreter and
 * not an alias to program14. Clipping/fan branches retain an exact frontier.
 * Evidence: analysis/static-mesh-program10-evidence.json. */
#include "static_mesh_program10.h"
#include "static_mesh_vu_numeric.h"
#include <fenv.h>
#include <math.h>
#include <string.h>

/* Force each original arithmetic stage to a binary32 reference result. */
static float mul(float a,float b){volatile float r=a*b;return r;}
static float add(float a,float b){volatile float r=a+b;return r;}
static float sub(float a,float b){volatile float r=a-b;return r;}
static float divide(float a,float b){volatile float r=a/b;return r;}
static gh_static_mesh_status unsupported(gh_static_transform_frontier*f,uint32_t pc){
 if(f){f->ee_entry=0x155800;f->vu_byte_pc=pc;}return GH_STATIC_UNSUPPORTED;
}
static int finite_vector(const float*p,size_t n){for(size_t i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
static int transform(const float matrix[16],const float p[3],float out[4]){
 for(unsigned row=0;row<4;row++){
  float v=mul(matrix[row],p[0]);
  v=add(v,mul(matrix[4+row],p[1]));
  v=add(v,mul(matrix[8+row],p[2]));
  out[row]=add(v,matrix[12+row]);
 }
 return finite_vector(out,4);
}
static int sign_bit(float x){uint32_t bits;memcpy(&bits,&x,4);return (int)(bits>>31);}

static gh_static_mesh_status calculate(const gh_scene_mesh_stream*s,uint32_t flags,
 const gh_static_mesh_batch*b,const gh_static_transform_inputs*in,
 gh_static_gs_vertex*out,uint32_t*indices,gh_static_geometry_result*result,
 gh_static_transform_frontier*f){
 float screen_history[38][3];size_t triangles=0;
 for(size_t i=0;i<b->logical_vertices;i++){
  gh_static_mesh_vertex raw;uint32_t ee;
  gh_static_mesh_status status=gh_static_mesh_vertex_at(s,flags,b->first_vertex+i,&raw,&ee);
  if(status){if(f)f->ee_entry=ee;return status;}
  float p[3],screen[4],clip[4];
  for(unsigned j=0;j<3;j++)p[j]=mul((float)raw.position[j],0.01f); /* VU0138/0158 */
  if(!transform(in->screen_matrix,p,screen)||!transform(in->clip_matrix,p,clip))return unsupported(f,0x0190);
  float q;if(!gh_static_vu_reciprocal(screen[3],&q))return unsupported(f,0x01b0);
  gh_static_gs_vertex v={0};v.raw_control=raw.raw_w;v.clip_flags=gh_static_vu_clip_flags(clip);
  /* Program10's 0308 IBGTZ enters polygon processing0368 when accumulated
   * clip flags are positive; unlike14 it must not discard those triangles.
   * This first bounded domain requires every input vertex inside allplanes.
   * Conservative preflight rejects even a rawADC vertex whose ownbranch
   * might bypass clipping; no successful geometry is falsely reported. */
  if(v.clip_flags)return unsupported(f,0x0308);
  for(unsigned j=0;j<3;j++){
   screen_history[i][j]=mul(screen[j],q);
   if(!gh_static_vu_ftoi4(screen_history[i][j],v.xyz_fixed4+j))return unsupported(f,0x0290);
  }
  for(unsigned j=0;j<2;j++){
   float uv=add(divide((float)raw.uv[j],4096.0f),in->uv_offset[j]);
   v.stq[j]=mul(uv,q);
   if(!isfinite(v.stq[j]))return unsupported(f,0x0260);
  }
  v.stq[2]=q;
  for(unsigned j=0;j<4;j++){
   float color=mul((float)raw.color[j],in->color_scale[j]);
   if(!isfinite(color)||color<0.0f)return unsupported(f,0x0218);
   if(color>255.0f)color=255.0f;
   v.rgba[j]=(uint32_t)color;
  }
  float fog=add(in->fog[0],mul(in->fog[1],screen[3]));
  if(!isfinite(fog))return unsupported(f,0x0228);
  if(fog>255.0f)fog=255.0f;if(fog<0.0f)fog=0.0f;
  int32_t fog_fixed; if(!gh_static_vu_ftoi4(fog,&fog_fixed))return unsupported(f,0x0290);
  v.fog_fixed4=(uint16_t)fog_fixed;
  v.adc=(raw.raw_w&0x8000u)!=0;
  if(i>=2){
   v.triangle_tested=1;
   /* Program10 0298..02C0 OPMULA/OPMSUB and02D8 FMAND0x20:
    * negative projected cross sets ADC at0430. Lowcontrol bits reverse
    * operands. Clip tests are zero by the separately checked domain. */
   if(!v.adc){
   float a[2],c[2];for(unsigned j=0;j<2;j++){
    a[j]=sub(screen_history[i-2][j],screen_history[i-1][j]);
    c[j]=sub(screen_history[i][j],screen_history[i-1][j]);
   }
   float area=(raw.raw_w&0x7fffu)?sub(mul(c[0],a[1]),mul(c[1],a[0])):sub(mul(a[0],c[1]),mul(a[1],c[0]));
   if(!isfinite(area))return unsupported(f,0x02b8);
   if(sign_bit(area))v.adc=1;
   }
   if(!v.adc){
    uint32_t first=(uint32_t)(i-2),second=(uint32_t)(i-1);
    if(i&1u){uint32_t swap=first;first=second;second=swap;}
    indices[triangles*3]=first;indices[triangles*3+1]=second;indices[triangles*3+2]=(uint32_t)i;triangles++;
   }
  }
  out[i]=v;
 }
 *result=(gh_static_geometry_result){b->logical_vertices,triangles,b->material,(uint16_t)((b->gif_strip[0]>>47)&0x7ff)};
 return GH_STATIC_OK;
}

gh_static_mesh_status gh_static_mesh_transform_program10_batch(const gh_scene_mesh_stream*s,uint32_t flags,size_t batch,
 const gh_static_transform_inputs*in,gh_static_transform_policy policy,
 gh_static_gs_vertex*vertices,size_t capacity,uint32_t*indices,size_t index_capacity,
 gh_static_geometry_result*result,gh_static_transform_frontier*f){
 if(f)*f=(gh_static_transform_frontier){0};
 if(!in||!vertices||!indices||!result)return GH_STATIC_ARGUMENT;
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return unsupported(f,0x0138);
 if(!finite_vector(in->screen_matrix,16)||!finite_vector(in->clip_matrix,16)||
    !finite_vector(in->uv_offset,2)||!finite_vector(in->color_scale,4)||!finite_vector(in->fog,2))return GH_STATIC_ARGUMENT;
 gh_static_mesh_batch b;uint32_t ee;gh_static_mesh_status status=gh_static_mesh_batch_at(s,flags,batch,&b,&ee);
 if(status){if(f)f->ee_entry=ee;return status;}
 if(capacity<b.logical_vertices||index_capacity<(b.logical_vertices>2?(b.logical_vertices-2)*3:0))return GH_STATIC_BOUNDS;
 gh_static_gs_vertex temporary[38];uint32_t temporary_indices[108];gh_static_geometry_result temporary_result;
 fenv_t saved;if(feholdexcept(&saved)||fesetround(FE_TONEAREST))return unsupported(f,0x0138);
 status=calculate(s,flags,&b,in,temporary,temporary_indices,&temporary_result,f);
 int restored=fesetenv(&saved);if(restored)return unsupported(f,0x0138);if(status)return status;
 memcpy(vertices,temporary,b.logical_vertices*sizeof(*vertices));memcpy(indices,temporary_indices,temporary_result.triangles*3*sizeof(*indices));*result=temporary_result;
 return GH_STATIC_OK;
}
