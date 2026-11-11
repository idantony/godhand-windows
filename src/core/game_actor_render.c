#include "game_actor_render.h"
#include "game_enemy_marker_quad.h"
#include <string.h>
#include <math.h>
#include "id_animation.h"

static int same(const gh_actor_render_bindings *a,const gh_actor_render_bindings *b)
{
 return a->work==b->work&&a->light==b->light&&
  a->depth_shift==b->depth_shift&&a->elf==b->elf&&a->elf_bytes==b->elf_bytes&&
  a->shadow==b->shadow&&a->actors==b->actors&&a->up_owner==b->up_owner&&
  a->pivot_owner==b->pivot_owner&&a->templates==b->templates&&a->textures==b->textures&&
  a->emit_shadow==b->emit_shadow&&a->emit_context==b->emit_context&&
  a->ray==b->ray&&a->ray_context==b->ray_context&&
  a->draw_callee==b->draw_callee&&a->draw_context==b->draw_context&&
  a->other_callee==b->other_callee&&a->other_context==b->other_context&&a->policy==b->policy&&
  a->actor_emit==b->actor_emit&&a->actor_emit_context==b->actor_emit_context&&a->light_globals==b->light_globals&&
  a->marker_player==b->marker_player&&a->marker_save.bytes==b->marker_save.bytes&&
  a->marker_save.size==b->marker_save.size&&a->marker_cockpit==b->marker_cockpit&&
  a->marker_id==b->marker_id&&a->marker_random==b->marker_random&&
  a->marker_runtime==b->marker_runtime&&a->marker_manager==b->marker_manager&&
  a->marker_emit==b->marker_emit&&a->marker_context==b->marker_context;
}

static uint16_t marker_half(const uint8_t *p)
{
 uint16_t value;
 memcpy(&value,p,sizeof(value));
 return value;
}

static uint32_t marker_word(const uint8_t *p)
{
 uint32_t value;
 memcpy(&value,p,sizeof(value));
 return value;
}

static float marker_float(const uint8_t *p)
{
 float value;
 memcpy(&value,p,sizeof(value));
 return value;
}

static float marker_add(float a,float b){volatile float value=a+b;return value;}
static float marker_mul(float a,float b){volatile float value=a*b;return value;}

static float marker_dot(const float *a,const float *b)
{
 return marker_add(marker_add(marker_mul(a[0],b[0]),marker_mul(a[1],b[1])),
                   marker_mul(a[2],b[2]));
}

static gh_actor_model_status marker_stop(uint32_t *frontier,uint32_t address)
{
 *frontier=address;
 return GH_ACTOR_MODEL_UNSUPPORTED;
}

static uint32_t marker_random(void *context)
{
 uint32_t word=0;
 (void)gh_game_mt_next_0030f348(context,&word);
 return word;
}

static void marker_store(uint8_t *p,float value){memcpy(p,&value,4);}
static float marker_div(float a,float b){volatile float value=a/b;return value;}

static int marker_color(float alpha,uint32_t *color)
{
 if(!isfinite(alpha)||alpha< -2147483648.0f||alpha>=2147483648.0f)return 0;
 *color=0x808080u|((uint32_t)(int32_t)alpha<<24);
 return 1;
}

static gh_actor_model_status marker_draw(gh_actor_render_call *c,uint32_t leaf,uint32_t *f)
{
 const gh_actor_render_bindings *b=&c->bindings;
 uint8_t *actor=c->model->root.bytes;
 gh_id_base *base=*b->marker_id;
 if(!b->marker_runtime||!b->marker_manager||!b->marker_emit||!b->pivot_owner||!b->textures)
  return marker_stop(f,leaf);
 if(c->marker_stage==2){
  gh_id_entry *entries[4];
  int boss=leaf==0x28f5b0;
  for(int id=0;id<3+boss;id++){
   int fallback;
   if(gh_id_base_get_work_002aa410(b->marker_runtime,base,id,entries+id,&fallback))
    return marker_stop(f,0x2aa410);
  }
  float x_scale=1,alpha=marker_float(actor+0x644);
  if(c->marker_facing<0){
   x_scale=marker_div(marker_add(c->marker_facing,.7f),.7f);
   alpha=marker_mul(marker_mul(x_scale,128),alpha);
  }else{
   if(c->marker_distance>4)alpha=marker_mul(marker_div(marker_add(10,-c->marker_distance),6),alpha);
   alpha=marker_mul(alpha,128);
  }
  uint32_t color;
  if(!marker_color(alpha,&color))return marker_stop(f,leaf);
  int16_t maximum,current,depth;
  memcpy(&maximum,actor+0x548,2);
  memcpy(&current,actor+0x54a,2);
  memcpy(&depth,actor+0x2ac,2);
  if(!maximum)return marker_stop(f,leaf);
  float ratio=marker_div((float)current,(float)maximum);
  float previous=marker_float(actor+0x630);
  float fade=0,lag=0;
  uint32_t lag_color=0;
  if(boss){
   fade=marker_mul(marker_float(actor+0x63c),.7f);
   lag=marker_float(actor+0x638);
   if(marker_float(actor+0x634)!=(float)current){fade=1;lag=previous;}
   else if(fade<.001f)fade=0;
   if(!isfinite(lag)||!marker_color(marker_mul(fade,128),&lag_color))return marker_stop(f,leaf);
  }

  entries[0]->scale_x=x_scale;
  entries[0]->scale_y=1;
  entries[0]->color_base=color;
  entries[1]->scale_x=ratio;
  entries[2]->scale_x=boss?ratio:previous;
  if(boss){
   entries[2]->color_base=lag_color;
   entries[3]->color_base=lag_color;
   entries[3]->scale_x=lag;
   marker_store(actor+0x634,(float)current);
   marker_store(actor+0x638,lag);
   marker_store(actor+0x63c,fade);
  }
  marker_store(actor+0x630,marker_add(previous,-marker_mul(marker_add(previous,-ratio),.07f)));
  memcpy(base->vector_a,actor+0x650,12);
  base->h44=depth;
  uint32_t kind=marker_word(actor+0x564);
  const float offset[3]={kind==0x209&&!boss?-.35f:kind==0x223&&boss?.6f:-.3f,
   kind==0x209&&boss?-.3f:kind==0x223&&boss?.55f:.2f,0};
  memcpy(base->vector_b,offset,sizeof(offset));
  uint32_t scale=boss?(kind==0x223?0x3c1eda4b:0x3c6d72ca):0x3bed72ca;
  memcpy(&base->f40,&scale,4);

  const gh_id_animation_profile profile={1,0};
  gh_id_frontier id_frontier={0};
  if(gh_id_jobs_move_reference(base,NULL,NULL,&profile,&id_frontier)){
   c->marker_stage=6;
   c->marker_frontier=id_frontier.address?id_frontier.address:0x2aaa40;
   return marker_stop(f,c->marker_frontier);
  }
  c->marker_stage=4;
 }
 if(c->marker_stage==6)return marker_stop(f,c->marker_frontier);
 gh_id_draw_job jobs[8];
 size_t count;
 gh_id_frontier id_frontier={0};
 if(gh_id_jobs_collect_visible(base,jobs,8,&count,&id_frontier))
  return marker_stop(f,id_frontier.address?id_frontier.address:0x2aac28);
 for(;c->marker_cursor<count;c->marker_cursor++){
  gh_id_draw_job *job=jobs+c->marker_cursor;
  if(job->consumer_address!=0x2ab360)return marker_stop(f,job->consumer_address);
  gh_id_entry pending=*job->entry;
  gh_enemy_marker_quad draw;
  gh_actor_model_status status=gh_enemy_marker_quad_002ab360(b->camera,b->pivot_owner,
     b->marker_manager,base,&pending,&draw,f);
  if(status)return status;
  if(!draw.visible)continue;
  const gh_stage_material *material;
  if(gh_stage_tim2_marker_resolve_0015e0a0(b->textures,draw.texture.source.bytes,
     draw.texture.source.size,&material))return marker_stop(f,0x15e0a0);
  gh_effect_trans_packet packet={.order=draw.order,.original_callee=0x15bcf0,
   .kind=GH_EFFECT_PASS_SPRITE,.source=GH_EFFECT_SURFACE_MATERIAL,
   .destination=GH_EFFECT_SURFACE_SCENE_COLOR,.prim=draw.prim,
   .alpha=draw.alpha,.test=draw.test,.tex0=material->source_tex0,
   .writes_tex0=1,.quad_count=1,.quads={draw.quad},.writes_clamp=1,.clamp=draw.clamp,
   .writes_tex1=1,.tex1=draw.tex1,.inherits_scene_target=1,.material=material};
  if(b->marker_emit(b->marker_context,&packet))return marker_stop(f,0x31a650);
  job->entry->color=pending.color;
 }
 c->marker_stage=3;
 return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_enemy_marker_after_shadow_0028edd0(gh_actor_render_call *c,uint32_t *f)
{
 if(!c||!f||!c->model||!c->model->root.bytes||!c->bindings.work)return GH_ACTOR_MODEL_ARGUMENT;
 *f=0;
 const gh_actor_render_bindings *b=&c->bindings;
 const uint8_t *work=(const uint8_t *)b->work;
 uint8_t *actor=c->model->root.bytes;
 if(c->marker_stage==3)return GH_ACTOR_MODEL_OK;
 if(c->model->root.size<0x65c||!c->model->root.alias_f0)return marker_stop(f,0x28edd0);
 const uint32_t leaf=actor[0x614]?0x28f5b0:0x28f108;
 if(c->marker_stage>=2)return marker_draw(c,c->marker_frontier,f);
 if(!c->marker_stage){
  if((marker_word(work+0x584)&8)||(marker_word(work+0x5e0)&0x40000)){
   c->marker_stage=3;
   return GH_ACTOR_MODEL_OK;
  }
  if(!b->marker_save.bytes||b->marker_save.size<0xbbc)return marker_stop(f,0x1fc708);
  if((marker_half(b->marker_save.bytes+0xbba)&1)||actor[0x615]||actor[0x616]||
     !marker_half(actor+0x548)){
   c->marker_stage=3;
   return GH_ACTOR_MODEL_OK;
  }
  const gh_actor_part *part;
  if(gh_actor_model_part_at(c->model,4,&part))return marker_stop(f,0x28ee70);
  const float point[3]={marker_float(actor+0xb0),marker_float(part->bytes+0xb4),
                        marker_float(actor+0xb8)};
  uint16_t flags=marker_half(actor+0x62c);
  float anchor[3],delta[3];
  for(unsigned i=0;i<3;i++){
   anchor[i]=flags&1?marker_float(actor+0x650+i*4):point[i];
   delta[i]=marker_add(point[i],-anchor[i]);
  }
  float distance=sqrtf(marker_dot(delta,delta));
  if(!isfinite(distance))return marker_stop(f,0x1f7df8);
  if(!b->marker_id||!*b->marker_id)return marker_stop(f,0x2abbe8);
  if(!(flags&1)){
   if(!b->marker_random)return marker_stop(f,0x2abbe8);
   uint32_t kind=marker_word(actor+0x564);
   unsigned offset=kind==0x209?0xdb0:kind==0x223?0xd60:kind>=0x220&&kind<=0x222?0xcc0:0;
   gh_id_base *id=*b->marker_id;
   if(offset&&gh_game_cockpit_base(b->marker_cockpit,offset,&id))return marker_stop(f,0x2abbe8);
   if(gh_id_base_restart_anim(id,marker_random,b->marker_random))
    return marker_stop(f,id->first_unported?id->first_unported:0x2aba78);
   id->playing=1;
   *b->marker_id=id;
   flags|=1;
  }
  if(distance>.1f)flags|=2;
  if(flags&2){
   for(unsigned i=0;i<3;i++)anchor[i]=marker_add(anchor[i],marker_mul(delta[i],.3f));
   if(distance<.0001f)flags&=(uint16_t)~2u;
  }
  memcpy(actor+0x650,anchor,sizeof(anchor));
  memcpy(actor+0x62c,&flags,sizeof(flags));
  c->marker_stage=1;
 }
 if(!b->marker_player||!b->marker_player->bytes||b->marker_player->size<0xbc)
  return marker_stop(f,0x2db6b0);
 if(!b->camera||!b->camera->constructed)return marker_stop(f,0x30a370);
 float eye_forward[3],player_to_enemy[3];
 for(unsigned i=0;i<3;i++){
  eye_forward[i]=marker_add(marker_float(b->camera->fields+0x200+i*4),
                            -marker_float(b->camera->fields+0x210+i*4));
  player_to_enemy[i]=marker_add(marker_float(c->model->root.alias_f0+i*4),
                               -marker_float(b->marker_player->bytes+0xb0+i*4));
 }
 float length=sqrtf(marker_dot(eye_forward,eye_forward));
 float distance=sqrtf(marker_dot(player_to_enemy,player_to_enemy));
 if(!isfinite(length)||!isfinite(distance)||length<=0||distance<=0)
  return marker_stop(f,0x30a370);
 volatile float inverse_length=1.0f/length,inverse_distance=1.0f/distance;
 for(unsigned i=0;i<3;i++){
  eye_forward[i]=marker_mul(eye_forward[i],inverse_length);
  player_to_enemy[i]=marker_mul(player_to_enemy[i],inverse_distance);
 }
 float facing=marker_dot(eye_forward,player_to_enemy);
 float ratio=marker_float(actor+0x630),alpha=marker_float(actor+0x644);
 if(!isfinite(facing)||!isfinite(ratio)||!isfinite(alpha))return marker_stop(f,leaf);
 gh_id_base *id=*b->marker_id;
 if(!actor[0x614]){
  const float offset[3]={-.3f,.2f,0};
  memcpy(id->vector_b,offset,sizeof(offset));
 }
 if(marker_half(actor+0x62c)&4){
  alpha=alpha>=.0001f?marker_mul(alpha,.8f):0;
  memcpy(actor+0x644,&alpha,sizeof(alpha));
 }
 if(distance>=10.0f||facing<=-.7f||(!actor[0x614]&&ratio<=.001f)){
  c->marker_stage=3;
  return GH_ACTOR_MODEL_OK;
 }
 c->marker_stage=2;
 c->marker_frontier=leaf;
 c->marker_distance=distance;
 c->marker_facing=facing;
 return marker_draw(c,leaf,f);
}
static gh_actor_model_status invoke(void *context,const gh_actor_trans_request *r,uint32_t *f)
{
 gh_actor_render_call *c=context;const gh_actor_render_bindings *b=&c->bindings;
 if(!r||r->model!=c->model||r->shadow!=b->shadow)return GH_ACTOR_MODEL_ARGUMENT;
 if(r->entry==0x14efa8||r->entry==0x28edd0){
  gh_shadow_bindings shadow={.model=c->model,.shadow=b->shadow,.work=b->work,
   .camera=b->camera,.world_light=b->light,.actors=b->actors,.up_owner=b->up_owner,
   .pivot_owner=b->pivot_owner,.templates=b->templates,.emit=b->emit_shadow,
   .emit_context=b->emit_context,.ray=b->ray,.ray_context=b->ray_context,.policy=b->policy};
  gh_actor_model_status status=gh_actor_shadow_draw_0014efa8(&c->shadow,&shadow,f);
  if(status||r->entry==0x14efa8)return status;
  return gh_enemy_marker_after_shadow_0028edd0(c,f);
 }
 if(r->entry==0x14ac90){
  if(r->depth<0||r->depth>UINT16_MAX){if(f)*f=r->entry;return GH_ACTOR_MODEL_UNSUPPORTED;}
  gh_actor_draw_bindings draw={.camera=b->camera,.light=b->light,
   .callee=b->draw_callee,.context=b->draw_context,.policy=b->policy,.textures=b->textures,
   .emit=b->actor_emit,.emit_context=b->actor_emit_context,
   .light_globals=b->light_globals,.pivot_owner=b->pivot_owner};
  return gh_actor_model_draw_0014ac90(&c->draw,c->model,(uint16_t)r->depth,r->far_mode,&draw,f);
 }
 if(!b->other_callee){if(f)*f=r->entry;return GH_ACTOR_MODEL_UNSUPPORTED;}
 return b->other_callee(b->other_context,r,f);
}

gh_actor_model_status gh_actor_render_0014a3e8(gh_actor_render_call *c,gh_actor_model *m,
 const gh_actor_render_bindings *b,uint32_t *f)
{
 if(f)*f=0;
 if(!c||!m||!b)return GH_ACTOR_MODEL_ARGUMENT;
 if(c->bound){
  if(c->identity!=c||c->model!=m||!same(&c->bindings,b))return GH_ACTOR_MODEL_BUSY;
  if(c->bindings.camera!=b->camera){
   /* The original current camera can change between invocations. */
   if(c->trans.active||!b->camera||!b->camera->constructed)return GH_ACTOR_MODEL_BUSY;
   c->bindings.camera=b->camera;
  }
 }
 else{memset(c,0,sizeof(*c));c->bindings=*b;c->model=m;c->identity=c;c->bound=1;}
 gh_actor_trans_bindings trans={.work=b->work,.camera=b->camera,.world_light=b->light,
  .depth_shift=b->depth_shift,.elf=b->elf,.elf_bytes=b->elf_bytes,.shadow=b->shadow,
  .callee=invoke,.context=c,.numeric_policy=b->policy};
 return gh_actor_model_trans_0014a3e8(&c->trans,m,&trans,f);
}

gh_actor_model_status gh_actor_render_next_call(gh_actor_render_call *c)
{
 if(!c||!c->bound||c->identity!=c||!c->trans.complete)return GH_ACTOR_MODEL_BUSY;
 memset(&c->trans,0,sizeof(c->trans));memset(&c->shadow,0,sizeof(c->shadow));
 memset(&c->draw,0,sizeof(c->draw));c->marker_stage=0;c->marker_frontier=0;c->marker_cursor=0;return GH_ACTOR_MODEL_OK;
}
