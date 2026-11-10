#include "game_cloth_finish.h"
#include "game_cloth_point.h"
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
static float length(const float*v){volatile float f=sqrtf(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2])));return f;}
static void normalize(float*v){float n=length(v);if(n>0){float inverse=divide(1,n);for(unsigned k=0;k<3;k++)v[k]=mul(v[k],inverse);}else v[0]=v[1]=v[2]=0;}
static void direction(const float*m,const float*v,float*out){for(unsigned k=0;k<3;k++)out[k]=add(add(mul(m[k],v[0]),mul(m[4+k],v[1])),mul(m[8+k],v[2]));}
static gh_cloth_status fail(gh_cloth_finish*s,uint32_t f,gh_cloth_status e){s->frontier=f;return e;}
static gh_cloth_status body(gh_cloth_finish*s,gh_cloth_distances*distances){
 gh_cloth_angles*angles=distances->angles;gh_cloth_step_prefix*prefix=angles->forces->prefix;gh_game_player_springs*springs=prefix->bindings.springs;gh_actor_model*model=&springs->child->model;uint8_t*p=model->root.bytes;uint32_t flags=u32(p+0x20ac);if(!(flags&0x100)||u32(p+0x20a4))return fail(s,0x105a94,GH_CLOTH_UNSUPPORTED);if(flags&0x80)return fail(s,0x106bf8,GH_CLOTH_UNSUPPORTED);if(u32(p+0x2074))return fail(s,0x105ea0,GH_CLOTH_UNSUPPORTED);
 uint8_t records[44*0xa0];memcpy(records,p+0x4d0,sizeof(records));float matrices[255][16];for(unsigned i=0;i<model->parts_allocated;i++){if(model->parts[i].alias_f0!=model->parts[i].bytes+0xb0)return fail(s,0x105ab8,GH_CLOTH_UNSUPPORTED);memcpy(matrices[i],model->parts[i].bytes+0x80,64);}unsigned rotated=0,hits=0;
 for(unsigned row=0;row<springs->count;row++){unsigned id=springs->indices[0][row],target=springs->indices[5][row];if(id>=44||id>=model->parts_allocated||(target!=255&&target>=44))return fail(s,0x105b34,GH_CLOTH_BOUNDS);uint8_t*r=records+id*0xa0;float*m=matrices[id];if(target!=255)memcpy(m+12,records+target*0xa0+0x40,12);
  if(!(flags&8)){float difference[3];for(unsigned k=0;k<3;k++)difference[k]=add(rf(r+0x40+4*k),-m[12+k]);if(difference[0]==0&&difference[1]==0&&difference[2]==0)memcpy(difference,r+0x10,12);else normalize(difference);float rest=mul(rf(r+0x60),prefix->radius_scale);for(unsigned k=0;k<3;k++){float point=add(m[12+k],mul(difference[k],rest));memcpy(r+0x40+4*k,&point,4);}}
  float point[3],anchor[3];memcpy(point,r+0x40,12);if(target==255)memcpy(anchor,m+12,12);else memcpy(anchor,records+target*0xa0+0x40,12);int hit;uint32_t pc;gh_cloth_status e=gh_cloth_point_body_001067b8(prefix->geometry,point,anchor,angles->shared_pivot,prefix->bindings.matrix_policy,&hit,&pc);if(e)return fail(s,pc,e);memcpy(r+0x40,point,12);if(hit){r[0x90]|=1;hits++;}if(r[0x91]&1)memcpy(r+0x40,r+0x80,12);
  float damping=rf(p+0x2090);if(r[0x90]&1)damping=mul(damping,0.3f);for(unsigned k=0;k<3;k++){float velocity=mul(add(rf(r+0x40+4*k),-rf(r+0x50+4*k)),damping);memcpy(r+0x30+4*k,&velocity,4);}
  float local[3],reference[3],current[3],position[3];memcpy(local,r+0x10,12);direction(m,local,reference);normalize(reference);for(unsigned k=0;k<3;k++)current[k]=add(rf(r+0x40+4*k),-m[12+k]);if(current[0]==0&&current[1]==0&&current[2]==0)memcpy(current,local,12);else normalize(current);memcpy(position,m+12,12);
  float dot=add(add(mul(reference[0],current[0]),mul(reference[1],current[1])),mul(reference[2],current[2])),radical=add(1,-mul(dot,dot));if(!isfinite(radical)||radical<0)return fail(s,0x1f8140,GH_CLOTH_UNSUPPORTED);float angle;if(gh_game_angle_001f8180(sqrtf(radical),dot,&angle,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))return fail(s,pc,GH_CLOTH_UNSUPPORTED);
  if(angle>0.01f&&angle<3.1315928f){float axis[3]={add(mul(reference[1],current[2]),-mul(reference[2],current[1])),add(mul(reference[2],current[0]),-mul(reference[0],current[2])),add(mul(reference[0],current[1]),-mul(reference[1],current[0]))},turn[16],result[16];if(gh_axis_angle_matrix_0015af30(angle,axis,angles->shared_pivot->bytes,turn,prefix->bindings.matrix_policy,&pc)||gh_actor_matrix_product_00147548(turn,m,result,prefix->bindings.matrix_policy,&pc))return fail(s,pc,GH_CLOTH_UNSUPPORTED);memcpy(m,result,64);rotated++;}memcpy(m+12,position,12);
  for(unsigned k=0;k<3;k++)if(!isfinite(rf(r+0x30+4*k))||!isfinite(rf(r+0x40+4*k)))return fail(s,0x1061c0,GH_CLOTH_NUMERIC);for(unsigned k=0;k<16;k++)if(!isfinite(m[k]))return fail(s,0x1061c0,GH_CLOTH_NUMERIC);
 }
 memcpy(p+0x4d0,records,sizeof(records));for(unsigned i=0;i<model->parts_allocated;i++)memcpy(model->parts[i].bytes+0x80,matrices[i],64);s->rotated_parts=rotated;s->point_hits=hits;s->complete=1;s->frontier=0;return GH_CLOTH_OK;
}
gh_cloth_status gh_cloth_finish_00105a94(gh_cloth_finish*s,gh_cloth_distances*distances){
 if(!s||!distances||!distances->complete||distances->frontier!=0x105a94||!distances->angles)return GH_CLOTH_ARGUMENT;if(s->distances&&s->distances!=distances)return GH_CLOTH_ARGUMENT;if(s->complete)return GH_CLOTH_OK;s->distances=distances;fenv_t env;if(feholdexcept(&env))return GH_CLOTH_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CLOTH_NUMERIC;}gh_cloth_status e=body(s,distances);if(fesetenv(&env))return GH_CLOTH_NUMERIC;return e;
}
