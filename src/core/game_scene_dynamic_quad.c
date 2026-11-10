#include "game_scene_dynamic_geometry.h"
#include "game_scene_dynamic_internal.h"
#include <math.h>
#include <float.h>
#include <string.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float value(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static float from_bits(uint32_t n){float x;memcpy(&x,&n,4);return x;}
static float add(float a,float b){volatile float x=a+b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static int normal(float f){return isfinite(f)&&(f==0||fabsf(f)>=FLT_MIN);}
static gh_scene_dynamic_status fail(gh_scene_dynamic_owner*s,uint32_t*f,uint32_t pc){if(f)*f=pc;s->frontier=pc;return GH_DYNAMIC_UNSUPPORTED;}
gh_scene_dynamic_status gh_scene_dynamic_quad_update_0012d1a8(gh_scene_dynamic_owner*s,struct gh_scene_dynamic_box*b,const gh_dynamic_corner_tables*t,gh_static_transform_policy policy,uint32_t*f){
 if(f)*f=0;gh_scene_dynamic_box_view v;if(!s||!b||!t||gh_scene_dynamic_box_view_at(s,b,&v))return GH_DYNAMIC_ARGUMENT;
 if(s->world_tables&&s->world_tables!=t)return GH_DYNAMIC_BUSY;if(!t->loaded||!t->initialized||t->identity!=t)return fail(s,f,0x133670);
 if(policy!=GH_STATIC_BINARY32_REFERENCE||v.size!=0x100||word(v.bytes+4)!=1||word(v.bytes+0x5c)!=0x41d580)return fail(s,f,0x12d1a8);
 float base[16]={0},rotation[16],offset[16]={0},intermediate[16],world[16],angles[3];uint32_t pc=0;
 for(unsigned k=0;k<4;k++){base[k*5]=1;offset[k*5]=1;}memcpy(offset+12,v.bytes+0x70,12);
 if(word(v.bytes+8)&4){if(!v.matrix)return fail(s,f,0x12d1a8);memcpy(base,v.matrix,64);memcpy(angles,v.bytes+0x90,12);}else{memcpy(base+12,v.bytes+0x60,12);memcpy(angles,v.bytes+0x80,12);}
 if(gh_actor_matrix_rotation_00147728(angles,0,rotation,policy,&pc)||gh_actor_matrix_product_00147548(base,rotation,intermediate,policy,&pc)||gh_actor_matrix_product_00147548(intermediate,offset,world,policy,&pc))return fail(s,f,pc?pc:0x12d1a8);
 fenv_t env;if(feholdexcept(&env))return fail(s,f,0x12d1a8);if(fesetround(FE_TONEAREST)){fesetenv(&env);return fail(s,f,0x12d1a8);}float corners[4][4],minimum[3],maximum[3];int valid=1;
 for(unsigned i=0;i<4;i++){float local[4]={mul(from_bits(t->quad[i][0]),value(v.bytes+0xa0)),mul(from_bits(t->quad[i][1]),value(v.bytes+0xa4)),0,value(v.bytes+0xc0+i*16+12)};
  for(unsigned k=0;k<4;k++){float n=mul(world[k],local[0]);n=add(n,mul(world[4+k],local[1]));n=add(n,mul(world[8+k],local[2]));corners[i][k]=add(n,mul(world[12+k],local[3]));valid=valid&&normal(local[k])&&normal(corners[i][k]);}
 }
 memcpy(minimum,corners[0],12);memcpy(maximum,corners[0],12);for(unsigned i=1;i<4;i++)for(unsigned k=0;k<3;k++){if(corners[i][k]<minimum[k])minimum[k]=corners[i][k];if(maximum[k]<corners[i][k])maximum[k]=corners[i][k];}
 if(fesetenv(&env))return fail(s,f,0x12d1a8);if(!valid)return fail(s,f,0x30a2b0);memcpy(v.bytes+0xc0,corners,sizeof(corners));memcpy(v.bytes+0x10,minimum,12);memcpy(v.bytes+0x20,maximum,12);b->world_ready=1;s->world_tables=t;s->frontier=0;return GH_DYNAMIC_OK;
}
