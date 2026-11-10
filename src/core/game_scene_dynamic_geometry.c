#include "game_scene_dynamic_geometry.h"
#include "game_scene_dynamic_internal.h"
#include <string.h>
#include <math.h>
#include <float.h>
#include <fenv.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float value(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static float val(uint32_t n){float f;memcpy(&f,&n,4);return f;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static int normal(float n){return isfinite(n)&&(n==0||fabsf(n)>=FLT_MIN);}
static void translation(float*m,const uint8_t*p){memset(m,0,64);for(unsigned k=0;k<4;k++)m[k*5]=1;memcpy(m+12,p,12);}
static gh_scene_dynamic_status fail(gh_scene_dynamic_owner*s,uint32_t*f,uint32_t pc,gh_scene_dynamic_status e){if(f)*f=pc;if(s)s->frontier=pc;return e;}
gh_scene_dynamic_status gh_scene_dynamic_box_update_0012dcb8(gh_scene_dynamic_owner*s,struct gh_scene_dynamic_box*b,const gh_dynamic_corner_tables*t,gh_static_transform_policy policy,uint32_t*f){
 if(f)*f=0;gh_scene_dynamic_box_view v;if(!s||!b||!t||gh_scene_dynamic_box_view_at(s,b,&v))return GH_DYNAMIC_ARGUMENT;if(s->world_tables&&s->world_tables!=t)return GH_DYNAMIC_BUSY;if(!t->loaded||!t->initialized||t->identity!=t)return fail(s,f,0x133670,GH_DYNAMIC_UNSUPPORTED);if(policy!=GH_STATIC_BINARY32_REFERENCE||w(v.bytes+4)!=2||w(v.bytes+0x5c)!=0x41d558)return fail(s,f,0x12dcb8,GH_DYNAMIC_UNSUPPORTED);
 float base[16],rotation[16],offset[16],intermediate[16],world[16],angles[3];uint32_t pc=0;if(w(v.bytes+8)&4){if(!v.matrix)return fail(s,f,0x12dcb8,GH_DYNAMIC_UNSUPPORTED);memcpy(base,v.matrix,64);memcpy(angles,v.bytes+0x90,12);}else{translation(base,v.bytes+0x60);memcpy(angles,v.bytes+0x80,12);}translation(offset,v.bytes+0x70);
 if(gh_actor_matrix_rotation_00147728(angles,0,rotation,policy,&pc)||gh_actor_matrix_product_00147548(base,rotation,intermediate,policy,&pc)||gh_actor_matrix_product_00147548(intermediate,offset,world,policy,&pc))return fail(s,f,pc?pc:0x12dcb8,GH_DYNAMIC_UNSUPPORTED);
 fenv_t env;if(feholdexcept(&env))return fail(s,f,0x12dcb8,GH_DYNAMIC_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(s,f,0x12dcb8,GH_DYNAMIC_UNSUPPORTED);}float corners[8][4],minimum[3],maximum[3];int valid=1;
 for(unsigned i=0;i<8;i++){float local[4];for(unsigned k=0;k<3;k++)local[k]=mul(val(t->box[i][k]),value(v.bytes+0xa0+k*4));local[3]=value(v.bytes+0x110+i*16+12);for(unsigned k=0;k<4;k++){float n=mul(world[k],local[0]);n=add(n,mul(world[4+k],local[1]));n=add(n,mul(world[8+k],local[2]));corners[i][k]=add(n,mul(world[12+k],local[3]));valid=valid&&normal(local[k])&&normal(corners[i][k]);}}
 memcpy(minimum,corners[0],12);memcpy(maximum,corners[0],12);for(unsigned i=1;i<8;i++)for(unsigned k=0;k<3;k++){if(corners[i][k]<minimum[k])minimum[k]=corners[i][k];if(maximum[k]<corners[i][k])maximum[k]=corners[i][k];}
 if(fesetenv(&env))return fail(s,f,0x12dcb8,GH_DYNAMIC_UNSUPPORTED);if(!valid)return fail(s,f,0x30a2b0,GH_DYNAMIC_UNSUPPORTED);memcpy(v.bytes+0x110,corners,sizeof(corners));memcpy(v.bytes+0x10,minimum,12);memcpy(v.bytes+0x20,maximum,12);b->world_ready=1;s->world_tables=t;s->frontier=0;return GH_DYNAMIC_OK;
}
gh_scene_dynamic_status gh_scene_dynamic_step_0012e840(gh_scene_dynamic_owner*s,const gh_dynamic_corner_tables*t,const gh_boot_work*work,gh_static_transform_policy policy,uint32_t*f){
 if(f)*f=0;if(!s||!t||!work)return GH_DYNAMIC_ARGUMENT;gh_scene_dynamic_status e=gh_scene_dynamic_validate_owner(s);if(e)return fail(s,f,0x12e840,e);if(!s->update_active){s->update_cursor=s->tables->pending_actor_head;s->update_completed=0;s->update_allocation_count=s->allocation_count;s->update_tables=t;s->update_work=work;s->update_active=1;}else if(s->update_tables!=t||s->update_work!=work||s->update_allocation_count!=s->allocation_count)return GH_DYNAMIC_ARGUMENT;
 while(s->update_cursor){if(s->update_completed>=s->allocation_count)return fail(s,f,0x12e840,GH_DYNAMIC_UNSUPPORTED);e=gh_scene_dynamic_box_update_0012dcb8(s,s->update_cursor,t,policy,f);if(e)return e;s->update_cursor=s->update_cursor->next;s->update_completed++;}
 if(w((const uint8_t*)work+0x594)&0x08000000)return fail(s,f,0x2db6b0,GH_DYNAMIC_UNSUPPORTED);gh_scene_dynamic_cancel_update(s);s->frontier=0;return GH_DYNAMIC_OK;
}
