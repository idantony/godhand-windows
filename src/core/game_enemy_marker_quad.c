#include "game_enemy_marker_quad.h"
#include "game_actor_inverse.h"
#include "game_scene_at_geometry.h"
#include "../assets/static_mesh_vu_numeric.h"
#include "../assets/tim2.h"
#include <fenv.h>
#include <math.h>
#include <string.h>

static float number(const void *p){float value;memcpy(&value,p,4);return value;}
static uint32_t word(const void *p){uint32_t value;memcpy(&value,p,4);return value;}
static float add(float a,float b){volatile float value=a+b;return value;}
static float mul(float a,float b){volatile float value=a*b;return value;}
static float div32(float a,float b){volatile float value=a/b;return value;}
static float dot(const float *a,const float *b)
{
 return add(add(mul(a[0],b[0]),mul(a[2],b[2])),mul(a[1],b[1]));
}
static gh_actor_model_status stop(uint32_t *f,uint32_t pc)
{
 *f=pc;
 return GH_ACTOR_MODEL_UNSUPPORTED;
}

static gh_actor_model_status texture(const gh_id_state *manager,const gh_id_entry *entry,
 gh_id_texture *out,uint32_t *f)
{
 const unsigned item=(unsigned)entry->b8b+1;
 if(entry->texture.bytes||entry->b8a!=1||(item!=6&&item!=7&&item!=9&&item!=38))
  return stop(f,0x2acdc8);
 const gh_bundle_entry *table=&manager->pair[1].texture_data;
 if(!table->bytes||table->size<(item+1)*4)return stop(f,0x2acdc8);
 uint32_t count=word(table->bytes);
 if(count<item||count>(table->size-4)/4)return stop(f,0x2acdc8);
 size_t offset=word(table->bytes+item*4),end=table->size;
 if(offset<(count+1u)*4u||offset>=end)return stop(f,0x2acdc8);
 for(uint32_t i=1;i<=count;i++){
  size_t other=word(table->bytes+i*4u);
  if(other&&(other<(count+1u)*4u||other>=table->size))return stop(f,0x2acdc8);
  if(other>offset&&other<end)end=other;
 }
 const uint8_t *source=table->bytes+offset;
 gh_tim2_picture picture;
 if(gh_tim2_first(source,end-offset,&picture)||picture.width!=64||(picture.height!=32&&picture.height!=64)||
    picture.image_type!=5||picture.clut_type!=3||picture.colors!=256||
    picture.tex0!=(picture.height==64?UINT64_C(0x199308000):UINT64_C(0x159308000)))return stop(f,0x15e0a0);
 *out=(gh_id_texture){.source={source,end-offset,offset},.width=64,.height=picture.height,
                     .tex0=picture.tex0};
 return GH_ACTOR_MODEL_OK;
}

static gh_actor_model_status project(const gh_game_pl_camera *camera,
 const gh_player_shared_vector *pivot,const gh_id_state *manager,const gh_id_base *base,
 gh_id_entry *entry,gh_enemy_marker_quad *out,uint32_t *f)
{
 gh_enemy_marker_quad result={0};
 gh_actor_model_status status=texture(manager,entry,&result.texture,f);
 if(status)return status;
 float screen[16],zero[3],probe[4];
 memcpy(screen,camera->fields,64);
 memcpy(zero,pivot->bytes,12);
 if(gh_scene_at_apply_world_001508d8(screen,zero,probe)||
    !isfinite(probe[3])||probe[3]==0)return stop(f,0x30a2b0);
 if(div32(probe[2],probe[3])<0){*out=result;return GH_ACTOR_MODEL_OK;}

 float forward[3],to_anchor[3];
 for(unsigned k=0;k<3;k++){
  float eye=number(camera->fields+0x210+k*4);
  forward[k]=add(number(camera->fields+0x200+k*4),-eye);
  to_anchor[k]=add(base->vector_a[k],-eye);
 }
 float length=sqrtf(dot(forward,forward));
 if(!isfinite(length)||length<=0)return stop(f,0x1f7e28);
 float axial=div32(dot(forward,to_anchor),length);
 if(!isfinite(axial))return stop(f,0x30a348);
 uint32_t color=entry->color;
 if(axial<length){
  uint32_t alpha=axial<0?0:(uint32_t)mul(mul((float)(color>>24),div32(axial,length)),.5f);
  color=(color&0xffffffu)|(alpha<<24);
 }

 static const float corner[5][4]={
  {-.5f,.5f,-.5f,.5f},{-1,0,-1,0},{1,0,-1,0},{-1,0,0,1},{0,1,0,1}};
 const float *shape=corner[entry->b89<5?entry->b89:0];
 float width=mul(entry->f68,entry->draw_scale_x),height=mul(entry->f6c,entry->draw_scale_y);
 float x=add(256,-entry->draw_x),y=add(224,-entry->draw_y);
 float local_x[2],local_y[2];
 for(unsigned i=0;i<2;i++){
  local_x[i]=add(mul(add(x,mul(width,shape[i])),base->f40),base->vector_b[0]);
  local_y[i]=add(mul(add(y,mul(height,shape[2+i])),base->f40),base->vector_b[1]);
 }
 float view[16],inverse[16];
 memcpy(view,camera->fields+0x140,64);
 memset(view+12,0,12);
 if(gh_actor_matrix_inverse_00147568(view,inverse,GH_STATIC_BINARY32_REFERENCE,f))
  return GH_ACTOR_MODEL_UNSUPPORTED;

 result.quad.vertex_count=4;
 for(unsigned i=0;i<4;i++){
  const float local[3]={local_x[i&1],local_y[i>>1],0};
  float world[4],clip[4],q;
  if(gh_scene_at_apply_world_001508d8(inverse,local,world))return stop(f,0x30a2b0);
  for(unsigned k=0;k<3;k++)world[k]=add(world[k],base->vector_a[k]);
  if(gh_scene_at_apply_world_001508d8(screen,world,clip)||
     !gh_static_vu_reciprocal(clip[3],&q))return stop(f,0x30ab48);
  int32_t fixed[3];
  for(unsigned k=0;k<3;k++){
   if(!gh_static_vu_ftoi4(mul(clip[k],q),fixed+k))return stop(f,0x30ab48);
  }
  if(fixed[0]<0||fixed[0]>65535||fixed[1]<0||fixed[1]>65535||
     fixed[2]<0||fixed[2]>0xfffff)return stop(f,0x15bcf0);
  gh_effect_trans_vertex *vertex=result.quad.vertices+i;
  vertex->x12_4=fixed[0];
  vertex->y12_4=fixed[1];
  vertex->z=(uint32_t)fixed[2]>>4;
  vertex->rgba=color;
  vertex->q=1.0f;
  vertex->s=entry->f70[(i&1)?0:2];
  vertex->t=entry->f70[(i&2)?1:3];
  if(!isfinite(vertex->s)||!isfinite(vertex->t))return stop(f,0x15bcf0);
 }
 if(base->h44==0)return stop(f,0x31a650);
 result.order=(gh_ui_order_group){.bank=6,.depth=(uint32_t)(int32_t)base->h44};
 result.test=0x5000d;
 result.alpha=UINT64_C(0x8000000044);
 result.tex1=0x60;
 result.clamp=0;
 result.prim=0x54;
 result.visible=1;
 entry->color=color;
 *out=result;
 return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_enemy_marker_quad_002ab360(const gh_game_pl_camera *camera,
 const gh_player_shared_vector *pivot,const gh_id_state *manager,const gh_id_base *base,
 gh_id_entry *entry,gh_enemy_marker_quad *out,uint32_t *frontier)
{
 if(!frontier)return GH_ACTOR_MODEL_ARGUMENT;
 *frontier=0;
 if(!camera||!camera->constructed||!pivot||!pivot->initialized||!manager||!base||!entry||!out)
  return GH_ACTOR_MODEL_ARGUMENT;
 if(entry->kind!=3||base->packed.bytes)return stop(frontier,0x2ab360);
 fenv_t env;
 if(feholdexcept(&env))return stop(frontier,0x2ab360);
 if(fesetround(FE_TONEAREST)){
  (void)fesetenv(&env);
  return stop(frontier,0x2ab360);
 }
 gh_actor_model_status status=project(camera,pivot,manager,base,entry,out,frontier);
 if(fesetenv(&env))return stop(frontier,0x2ab360);
 return status;
}
