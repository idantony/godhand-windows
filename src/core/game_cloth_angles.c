#include "game_cloth_angles.h"
#include "game_axis_angle.h"
#include "game_camera.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t u32(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static float rf(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static float add(float a,float b){volatile float f=a+b;return f;}
static float mul(float a,float b){volatile float f=a*b;return f;}
static float divide(float a,float b){volatile float f=a/b;return f;}
static float norm(const float*v){volatile float f=sqrtf(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2])));return f;}
static void normalize(float*v){float length=norm(v);if(length>0){float inverse=divide(1,length);for(unsigned k=0;k<3;k++)v[k]=mul(v[k],inverse);}else v[0]=v[1]=v[2]=0;}
static void direction(const uint8_t*m,const float*v,float*out){for(unsigned k=0;k<3;k++)out[k]=add(add(mul(rf(m+4*k),v[0]),mul(rf(m+16+4*k),v[1])),mul(rf(m+32+4*k),v[2]));}
static gh_cloth_status fail(gh_cloth_angles*s,uint32_t f,gh_cloth_status e){s->frontier=f;return e;}
static gh_cloth_status body(gh_cloth_angles*s,gh_cloth_forces*forces,gh_player_shared_vector*pivot){
 gh_cloth_step_prefix*prefix=forces->prefix;gh_game_player_springs*springs=prefix->bindings.springs;gh_actor_model*model=&springs->child->model;uint8_t*p=model->root.bytes;
 if((u32(p+0x20ac)&0x10)||!springs->table_addresses[7]){s->complete=1;s->frontier=0x104ec8;return GH_CLOTH_OK;}
 if(u32(p+0x20a4))return fail(s,0x1049b0,GH_CLOTH_UNSUPPORTED);
 uint8_t records[44*0xa0];memcpy(records,p+0x4d0,sizeof(records));float points[255][3];for(unsigned i=0;i<model->parts_allocated;i++)memcpy(points[i],model->parts[i].alias_f0,12);unsigned corrections=0;
 for(unsigned row=0;row<springs->count;row++){unsigned id=springs->indices[0][row];if(id>=44||id>=model->parts_allocated)return fail(s,0x1049d0,GH_CLOTH_BOUNDS);uint8_t*r=records+id*0xa0;if(r[0x91]&1)continue;
  unsigned target=springs->indices[5][row];float reference[3],local[3];memcpy(local,r+0x10,12);
  if(target==255){direction(model->parts[id].bytes+0x80,local,reference);normalize(reference);}else{if(target>=44||target>=model->parts_allocated)return fail(s,0x104ac0,GH_CLOTH_BOUNDS);memcpy(points[id],records+target*0xa0+0x40,12);for(unsigned k=0;k<3;k++)reference[k]=add(rf(records+target*0xa0+0x40+k*4),-points[target][k]);if(reference[0]==0&&reference[1]==0&&reference[2]==0)memcpy(reference,local,12);else normalize(reference);}
  float current[3];for(unsigned k=0;k<3;k++)current[k]=add(rf(r+0x40+4*k),-points[id][k]);float length=norm(current);
  if(current[0]==0&&current[1]==0&&current[2]==0)memcpy(current,local,12);else normalize(current);
  float dot=add(add(mul(reference[0],current[0]),mul(reference[1],current[1])),mul(reference[2],current[2]));float radical=add(1,-mul(dot,dot));if(!isfinite(radical)||radical<0)return fail(s,0x1f8140,GH_CLOTH_UNSUPPORTED);float angle;uint32_t pc;if(gh_game_angle_001f8180(sqrtf(radical),dot,&angle,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))return fail(s,pc,GH_CLOTH_UNSUPPORTED);float limit;memcpy(&limit,springs->coefficients[0]+row,4);if(!isfinite(limit)||!isfinite(length))return fail(s,0x104d94,GH_CLOTH_NUMERIC);
  if(limit<angle&&angle<3.1315928f){float axis[3]={add(mul(reference[1],current[2]),-mul(reference[2],current[1])),add(mul(reference[2],current[0]),-mul(reference[0],current[2])),add(mul(reference[0],current[1]),-mul(reference[1],current[0]))};float target_angle=add(mul(angle,0.2f),mul(limit,0.8f)),matrix[16],rotated[3];if(gh_axis_angle_matrix_0015af30(target_angle,axis,pivot->bytes,matrix,prefix->bindings.matrix_policy,&pc))return fail(s,pc,GH_CLOTH_UNSUPPORTED);direction((const uint8_t*)matrix,reference,rotated);for(unsigned k=0;k<3;k++){float value=add(mul(rotated[k],length),points[id][k]);memcpy(r+0x40+4*k,&value,4);}corrections++;}
  for(unsigned k=0;k<3;k++)if(!isfinite(points[id][k])||!isfinite(rf(r+0x40+4*k)))return fail(s,0x104ec4,GH_CLOTH_NUMERIC);
 }
 memcpy(p+0x4d0,records,sizeof(records));for(unsigned i=0;i<model->parts_allocated;i++)memcpy(model->parts[i].alias_f0,points[i],12);s->corrections=corrections;s->complete=1;s->frontier=0x104ec8;return GH_CLOTH_OK;
}
gh_cloth_status gh_cloth_constrain_angles_0010497c(gh_cloth_angles*s,gh_cloth_forces*forces,gh_player_shared_vector*pivot){
 if(!s||!forces||!forces->complete||forces->frontier!=0x10497c||!forces->prefix||!pivot||!pivot->initialized)return GH_CLOTH_ARGUMENT;if(s->forces&&(s->forces!=forces||s->shared_pivot!=pivot))return GH_CLOTH_ARGUMENT;if(s->complete)return GH_CLOTH_OK;s->forces=forces;s->shared_pivot=pivot;fenv_t env;if(feholdexcept(&env))return GH_CLOTH_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CLOTH_NUMERIC;}gh_cloth_status e=body(s,forces,pivot);if(fesetenv(&env))return GH_CLOTH_NUMERIC;return e;
}
