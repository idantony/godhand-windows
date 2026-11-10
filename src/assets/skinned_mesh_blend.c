#include "skinned_mesh_blend.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static float mul(float a,float b){volatile float r=a*b;return r;}
static float add(float a,float b){volatile float r=a+b;return r;}
static float bits(uint32_t word){float f;memcpy(&f,&word,4);return f;}
static gh_static_mesh_status fail(uint32_t*f,uint32_t pc,gh_static_mesh_status e){if(f)*f=pc;return e;}
gh_static_mesh_status gh_skinned_mesh_blend_program2(const gh_skinned_mesh_vertex*v,
 const float(*palette)[16],size_t count,gh_static_transform_policy policy,
 gh_skinned_blend_result*out,uint32_t*frontier){
 if(frontier)*frontier=0;if(!v||!palette||!out||!count||count>42)return GH_STATIC_ARGUMENT;
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(frontier,0x140,GH_STATIC_UNSUPPORTED);
 size_t refs[3];for(unsigned k=0;k<3;k++){
  unsigned address=v->bone_indices[k+1];if((address&3)||address/4>=count)return fail(frontier,0x148,GH_STATIC_BOUNDS);
  refs[k]=address/4;for(unsigned j=0;j<16;j++)if(!isfinite(palette[refs[k]][j]))return fail(frontier,0x148,GH_STATIC_UNSUPPORTED);
  if(!isfinite(v->position[k]))return fail(frontier,0x178,GH_STATIC_UNSUPPORTED);
 }
 fenv_t saved;if(feholdexcept(&saved))return fail(frontier,0x140,GH_STATIC_UNSUPPORTED);
 if(fesetround(FE_TONEAREST)){fesetenv(&saved);return fail(frontier,0x140,GH_STATIC_UNSUPPORTED);}
 float weight[3],normal[3],matrix[16];
 for(unsigned k=0;k<3;k++){
  weight[k]=mul((float)v->weights[k],bits(0x3c23d70a));
  int signed_normal=v->normal[k]<128?(int)v->normal[k]:(int)v->normal[k]-256;
  normal[k]=mul((float)signed_normal,bits(0x3c010204));
 }
 for(unsigned j=0;j<16;j++){
  float r=mul(palette[refs[0]][j],weight[0]);r=add(r,mul(palette[refs[1]][j],weight[1]));
  matrix[j]=add(r,mul(palette[refs[2]][j],weight[2]));
 }
 gh_skinned_blend_result candidate={.raw_control=v->raw_control};int finite=1;
 for(unsigned row=0;row<3;row++){
  float p=mul(matrix[row],v->position[0]);p=add(p,mul(matrix[4+row],v->position[1]));p=add(p,mul(matrix[8+row],v->position[2]));
  candidate.position[row]=add(p,matrix[12+row]);
  float n=mul(matrix[row],normal[0]);n=add(n,mul(matrix[4+row],normal[1]));n=add(n,mul(matrix[8+row],normal[2]));
  candidate.normal[row]=add(n,0.0f);
  if(!isfinite(candidate.position[row])||!isfinite(candidate.normal[row]))finite=0;
 }
 int restored=fesetenv(&saved);if(!finite||restored)return fail(frontier,0x230,GH_STATIC_UNSUPPORTED);
 *out=candidate;return GH_STATIC_OK;
}
