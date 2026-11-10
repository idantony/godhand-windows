#include "skinned_mesh_transform30.h"
#include "skinned_mesh_blend.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static float mul(float a,float b){volatile float n=a*b;return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static gh_static_mesh_status bad(gh_static_transform_frontier *f,uint32_t pc){if(f){f->ee_entry=0x154e98;f->vu_byte_pc=pc;}return GH_STATIC_UNSUPPORTED;}
/* Same verified arithmetic subgraph, different actual instruction addresses. */
static void program30_frontier(gh_static_transform_frontier *f){if(!f)return;switch(f->vu_byte_pc){case 0x230:f->vu_byte_pc=0x228;break;case 0x290:f->vu_byte_pc=0x280;break;case 0x418:f->vu_byte_pc=0x638;break;case 0x460:f->vu_byte_pc=0x608;break;case 0x458:f->vu_byte_pc=0x368;break;case 0x490:f->vu_byte_pc=0x668;break;case 0x4a8:f->vu_byte_pc=0x680;break;default:break;}}
gh_static_mesh_status gh_skinned_transform_program30_batch(const gh_skinned_mesh_stream *stream,size_t batch,const gh_skinned_program30_inputs *in,gh_static_transform_policy policy,gh_static_gs_vertex *vertices,size_t capacity,uint32_t *indices,size_t index_capacity,gh_static_geometry_result *result,gh_static_transform_frontier *frontier){
 if(frontier)*frontier=(gh_static_transform_frontier){0};if(!stream||!in||!vertices||!indices||!result)return GH_STATIC_ARGUMENT;
 if(!in->squarings||in->squarings>255)return bad(frontier,0x3a8);
 for(unsigned k=0;k<3;k++)if(!isfinite(in->half_vector[k]))return bad(frontier,0x378);
 gh_skinned_mesh_batch source;gh_static_mesh_status status=gh_skinned_mesh_batch_at(stream,batch,&source);if(status)return status;if(source.logical_vertices>26||capacity<source.logical_vertices||index_capacity<(source.logical_vertices>2?(source.logical_vertices-2)*3:0))return GH_STATIC_BOUNDS;
 gh_static_gs_vertex prepared[26];uint32_t prepared_indices[72];gh_static_geometry_result geometry;
 status=gh_skinned_transform_program2_batch(stream,batch,&in->base,policy,prepared,26,prepared_indices,72,&geometry,frontier);
 if(status){program30_frontier(frontier);return status;}
 /* Program2's quantized alpha is deliberately NOT reused. The original30
  * multiplies the still-float raw alpha by its highlight before FTOI0. */
 float palette[42][16];if(in->base.part_count>42||in->base.palette_bytes<in->base.part_count*64)return GH_STATIC_BOUNDS;memcpy(palette,in->base.palette,in->base.part_count*64);
 fenv_t env;if(feholdexcept(&env))return bad(frontier,0x378);if(fesetround(FE_TONEAREST)){fesetenv(&env);return bad(frontier,0x378);}
 for(size_t i=0;i<geometry.vertices;i++){
  gh_skinned_mesh_vertex raw;status=gh_skinned_mesh_vertex_at(stream,source.first_vertex+i,&raw);if(status)break;
  gh_skinned_blend_result blended;uint32_t pc=0;status=gh_skinned_mesh_blend_program2(&raw,palette,in->base.part_count,policy,&blended,&pc);if(status){if(frontier){frontier->ee_entry=0x154e98;frontier->vu_byte_pc=pc==0x230?0x228:pc;}break;}
  float dot=mul(blended.normal[0],in->half_vector[0]);dot=add(dot,mul(blended.normal[1],in->half_vector[1]));dot=add(dot,mul(blended.normal[2],in->half_vector[2]));
  if(!isfinite(dot)){status=bad(frontier,0x388);break;}if(dot<0)dot=0;if(dot>1)dot=1;
  for(unsigned square=0;square<in->squarings;square++)dot=mul(dot,dot);
  float alpha=mul((float)raw.color[3],in->base.normal_light[15]);alpha=mul(alpha,in->base.light_color[15]);alpha=mul(alpha,dot);
  if(!isfinite(alpha)||alpha<0){status=bad(frontier,0x3c8);break;}if(alpha>255)alpha=255;prepared[i].rgba[3]=(uint32_t)alpha;
 }
 if(fesetenv(&env))return bad(frontier,0x3c8);if(status)return status;
 memcpy(vertices,prepared,geometry.vertices*sizeof(*vertices));memcpy(indices,prepared_indices,geometry.triangles*3*sizeof(*indices));*result=geometry;return GH_STATIC_OK;
}
