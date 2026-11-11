/* Original ordinary skill0 state 10D3A8, resumed by exact callee boundary. */
#include "game_player_square_frame.h"
#include "game_player_attack_recent.h"
#include "game_player_root_delta.h"
#include "id_layout_rotation.h"
#include <string.h>
#include <fenv.h>
#include <math.h>

static uint32_t word(const void *p){uint32_t n;memcpy(&n,p,4);return n;}
static uint16_t half(const void *p){uint16_t n;memcpy(&n,p,2);return n;}
static float value(const void *p){float n;memcpy(&n,p,4);return n;}
static void put(void *p,uint32_t n){memcpy(p,&n,4);}
static void sh(void *p,uint16_t n){memcpy(p,&n,2);}
static void scalar(void *p,float n){memcpy(p,&n,4);}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static gh_player_frame_status stop(gh_player_square_frame *s,uint32_t pc,gh_player_frame_status e)
{
 s->frontier=pc;
 return e;
}
static gh_player_frame_status target_error(gh_player_square_frame *s,uint32_t pc,gh_player_target_status e)
{
 return stop(s,pc,e==GH_PLAYER_TARGET_NUMERIC?GH_PLAYER_FRAME_NUMERIC:
    e==GH_PLAYER_TARGET_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:
    e==GH_PLAYER_TARGET_ARGUMENT?GH_PLAYER_FRAME_ARGUMENT:GH_PLAYER_FRAME_CALLEE);
}
static gh_player_frame_status target_position(gh_player_square_frame *s,
 const gh_player_square_frame_bindings *b,float xyz[3])
{
 const gh_player_target *target=b->target_640;
 uint16_t kind=target->view.bytes&&target->view.size>=0x300?half(target->view.bytes+0x2fe):0;
 if(kind<0x200||kind>=0x300)return stop(s,0x124e68,GH_PLAYER_FRAME_UNSUPPORTED);
 gh_player_target_status status=gh_enemy_target_position_00275978(&target->view,xyz);
 return status?target_error(s,0x275978,status):GH_PLAYER_FRAME_OK;
}
static gh_player_frame_status camera_exit(gh_player_square_frame *s,const gh_player_square_frame_bindings *b)
{
 uint8_t *p=b->frame->player->view.bytes;
 if(!s->camera_pending&&!(word(p+0x15f4)&2u))return GH_PLAYER_FRAME_OK;
 if(!b->camera_exit_00124f60)return stop(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->camera_pending){put(p+0x15f4,word(p+0x15f4)&~2u);s->camera_pending=1;}
 uint32_t pc=0;
 gh_player_frame_status e=b->camera_exit_00124f60(b->context,b->frame,&pc);
 if(e)return stop(s,pc?pc:0x124f60,e);
 s->camera_pending=0;
 return GH_PLAYER_FRAME_OK;
}
/* Narrow original 123938(player,1) route for this attack's SEQ. */
static gh_player_frame_status attack_input(gh_player_square_frame *s,
 const gh_player_square_frame_bindings *b)
{
 const gh_player_frame_bindings *f=b->frame;
 uint8_t *p=f->player->view.bytes;
 const uint8_t *work=(const uint8_t*)f->work;
 const unsigned sequence=half(p+0x3ac);
 const uint32_t pressed=word(work+8),flags=word(p+0x15f4);
 if(flags&0x10)return stop(s,0x2a9ad0,GH_PLAYER_FRAME_UNSUPPORTED);
 if(sequence&0x40){
  if(half(work+0x5b0)==0x700)return stop(s,0x1169d8,GH_PLAYER_FRAME_UNSUPPORTED);
  if(word(work+0x5d8)&0x2000000)return stop(s,0x12bfb8,GH_PLAYER_FRAME_UNSUPPORTED);
 }
 if(!f->auxiliary_bindings_known||f->aux_698||f->aux_6a0||word(p+0x698)||word(p+0x6a0))
  return stop(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);
 if(sequence&0x10){
  put(p+0x15f4,flags|0x400u);
  if((pressed&2)||p[0x622]||p[0x623])return stop(s,0x123ae0,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!b->target_bindings)return stop(s,0x129cc8,GH_PLAYER_FRAME_UNSUPPORTED);
  uint32_t pc=0;
  gh_player_target_status e=gh_player_small_object_gate_00129cc8(b->target_bindings,&pc);
  if(e)return target_error(s,pc?pc:0x129cc8,e);
  if(word(f->init->enemies+0x5ac)||word(f->init->enemies+0x5b0))
   return stop(s,0x12a108,GH_PLAYER_FRAME_UNSUPPORTED);
  if(p[0x621]){
   if((flags&0x8000u)||(p[0x61f]&&value(p+0x608)>2.3561945f))
    return stop(s,0x12c000,GH_PLAYER_FRAME_UNSUPPORTED);
   if(!f->save.bytes||f->save.size<GH_CORE_SAVE_BYTES)return stop(s,0x1fb830,GH_PLAYER_FRAME_UNSUPPORTED);
   int exhausted=!f->init->heat[0x10]&&!half(p+0x5e2)&&
                 half(p+0x5e0)>=word(f->save.bytes+0x1d0);
   if(exhausted&&!(sequence&0x20)){s->input_result=0;return GH_PLAYER_FRAME_OK;}
   if(half(p+0x5e2)){sh(p+0x5e2,0);sh(p+0x5e0,0);}
   p[0x2f4]=p[0x2f6]=p[0x2f7]=0;p[0x2f5]=0xb;
   s->input_result=1;
   return GH_PLAYER_FRAME_OK;
  }
 }
 if(sequence&0x40){
  put(p+0x15f4,word(p+0x15f4)|0x400u);
  if(p[0x627]||p[0x61e])return stop(s,0x1240f8,GH_PLAYER_FRAME_UNSUPPORTED);
 }
 if((sequence&0x10)&&(pressed&4))return stop(s,0x124244,GH_PLAYER_FRAME_UNSUPPORTED);
 if(sequence&0x40)put(p+0x15f4,word(p+0x15f4)|0x100u);
 if(sequence&0x20)return stop(s,0x1242c4,GH_PLAYER_FRAME_UNSUPPORTED);
 s->input_result=0;
 return GH_PLAYER_FRAME_OK;
}
static gh_player_frame_status body(gh_player_square_frame *s,const gh_player_square_frame_bindings *b)
{
 const gh_player_frame_bindings *f=b->frame;
 uint8_t *p=f->player->view.bytes;
 uint32_t pc=0;
 if(!s->stage){
  uint16_t hp=half(p+0x54a);
  if(!hp||hp>=0x8000)sh(p+0x54a,1);
  s->stage=1;
 }
 if(s->stage==1){
  if(!f->give_binding_known||!f->give_objects||(f->give_present_mask&31u)!=31)
   return stop(s,0x10b2e8,GH_PLAYER_FRAME_UNSUPPORTED);
  int recent=0;
  gh_hitarea_status e=gh_player_attack_recent_0010b2e8(&f->player->view,f->give_objects,&recent,&pc);
  if(e)return stop(s,pc?pc:0x10b2e8,GH_PLAYER_FRAME_CALLEE);
  if(recent&&word(p+0x5e8)==0x5d)return stop(s,0x2cba48,GH_PLAYER_FRAME_UNSUPPORTED);
  if(p[0x2f6]>1||(p[0x2f6]&&word(p+0x5e8)!=0))return stop(s,0x10d3a8,GH_PLAYER_FRAME_UNSUPPORTED);
  s->stage=p[0x2f6]?5:2;
 }
 if(s->stage==2){
  if(!b->target_bindings||b->target_bindings->player!=&f->player->view)
   return stop(s,0x124540,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_target_status e=gh_player_attack_target_00124540(b->target_bindings,f->init,b->target_640,&pc);
  if(e)return target_error(s,pc?pc:0x124540,e);
  put(p+0x1588,0);
  s->stage=3;
 }
 if(s->stage==3){
  if(b->target_640->handle){
   float to[3];gh_player_frame_status e=target_position(s,b,to);if(e)return e;
   float dx=add(value(f->player->view.alias_f0),-to[0]);
   float dz=add(value(f->player->view.alias_f0+8),-to[2]);
   float distance=sqrtf(add(mul(dx,dx),mul(dz,dz)));
   if(!isfinite(distance))return stop(s,0x1f7e80,GH_PLAYER_FRAME_NUMERIC);
   float cap=(!half(p+0x5e0)&&p[0x61f])?1.0f:0.5f;
   float approach=add(distance,-1.2f);
   if(approach>0)scalar(p+0x1588,mul(approach<cap?approach:cap,.15f));
  }
  put(p+0x5ec,half(p+0x5e0)?word(p+0x5e8):0x72u);
  s->stage=4;
 }
 if(s->stage==4){
  gh_player_frame_status e=gh_player_square_attack_begin_00126918(&s->begin,f);
  if(e)return stop(s,s->begin.frontier,e);
  p[0x2f7]=0;put(p+0x5e8,0);put(p+0x15c0,0x3f800000);
  put(p+0x15b8,1);sh(p+0x5f0,10);put(p+0x15c4,0);
  put(p+0x15b0,1);put(p+0x15b4,0);sh(p+0x568,999);sh(p+0x56a,0);
  put(p+0x1598,0x3f000000);
  p[0x2f6]++;
  s->stage=5;
 }
 if(s->stage==5){
  if(half(p+0x3ac)&1)sh(p+0x56a,0);
  if(half(p+0x56a)){sh(p+0x56a,(uint16_t)(half(p+0x56a)-1));put(p+0x15f4,word(p+0x15f4)|0x800);}
  s->turn_pending=(uint8_t)(half(p+0x568)!=0);
  if(s->turn_pending)sh(p+0x568,(uint16_t)(half(p+0x568)-1));
  s->stage=6;
 }
 if(s->stage==6){
  if(b->target_640->handle&&s->turn_pending){
   float to[3],from[3],turn,heading=value(p+0x104);
   gh_player_frame_status e=target_position(s,b,to);if(e)return e;
   memcpy(from,f->player->view.alias_f0,12);
   float cap=mul(0.19634955f,value((const uint8_t*)f->work+0x55c));
   gh_player_target_status t=gh_player_target_angle_002da038(f->elf,f->elf_bytes,from,to,heading,cap,&turn,&pc);
   if(t)return target_error(s,pc?pc:0x2da038,t);
   if(gh_id_angle_reference_001f7d48(add(heading,turn),&heading))return stop(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);
   scalar(p+0x104,heading);
  }
  if(b->target_640->handle)p[0x648]=20;
  put(p+0x5dc,0x3f800000);put(p+0x5a8,0x3f800000);
  if(value(p+0x15c4)>0||word(p+0x15b4))return stop(s,0x10d77c,GH_PLAYER_FRAME_UNSUPPORTED);
  s->stage=7;
 }
 if(s->stage==7){
  if(half(p+0x3ac)&3){
   if(!f->effects||!f->current_player)return stop(s,0x306200,GH_PLAYER_FRAME_UNSUPPORTED);
   unsigned slot;
   gh_effect_status e=gh_game_effect_release_00306200(f->effects,f->current_player,2,2,&pc,&slot);
   if(e)return stop(s,pc?pc:0x306200,GH_PLAYER_FRAME_CALLEE);
  }
  s->stage=8;
 }
 if(s->stage==8){
  if(!f->advance_motion)return stop(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status e=f->advance_motion(f->motion_context,&s->motion,&s->ended,&pc);
  if(e)return stop(s,pc?pc:0x157c18,e);
  s->stage=9;
 }
 if(s->stage==9){
  if(s->ended){
   gh_player_frame_status e=camera_exit(s,b);if(e)return e;
   memset(p+0x2f4,0,4);
  }
  scalar(p+0x338,add(value(p+0x338),value(p+0x1588)));
  scalar(p+0x1588,mul(value(p+0x1588),.9f));
  s->stage=10;
 }
 if(s->stage==10){
  if(b->target_640->handle&&!p[0x2f7]){
   float to[3];gh_player_frame_status e=target_position(s,b,to);if(e)return e;
   float dx=add(value(f->player->view.alias_f0),-to[0]),dz=add(value(f->player->view.alias_f0+8),-to[2]);
   float squared=add(mul(dx,dx),mul(dz,dz));
   if(!isfinite(squared))return stop(s,0x1f7eb0,GH_PLAYER_FRAME_NUMERIC);
   if(squared<1.21f){put(p+0x330,0);put(p+0x338,0);}
  }
  s->stage=11;
 }
 if(s->stage==11){
  gh_player_delta_status e=gh_player_root_rotation_0014f9f0(&f->player->view,f->work,GH_STATIC_BINARY32_REFERENCE,&pc);
  if(e)return stop(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->stage=12;
 }
 if(s->stage==12){
  gh_player_delta_status e=gh_player_root_translation_0014f928(&f->player->view,f->work,GH_STATIC_BINARY32_REFERENCE,&pc);
  if(e)return stop(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->stage=13;
 }
 if(s->stage==13){
  unsigned flags=half(p+0x3ac);
  if(flags&0x10){
   unsigned counter=half(p+0x5f0);
   if(counter)sh(p+0x5f0,(uint16_t)(counter-1));
   else{sh(p+0x5e0,0);sh(p+0x5e2,0);}
  }
  put(p+0x1610,word(p+0x15c0));
  s->stage=14;
 }
 if(s->stage==14){
  if(half(p+0x3ac)&3){
   if(!b->hit_area_0010a438)return stop(s,0x10a438,GH_PLAYER_FRAME_UNSUPPORTED);
   gh_player_frame_status e=b->hit_area_0010a438(b->context,&f->player->view,f->give_objects,&pc);
   if(e)return stop(s,pc?pc:0x10a438,e);
   sh(p+0x5e6,1);
  }else if(half(p+0x5e6)){
   /* The decoded skill0 table's remaining four entries are zero. */
   unsigned i=half(p+0x5e4)+1u;if(i>4)i=4;
   sh(p+0x5e4,(uint16_t)i);put(p+0x1668,0);put(p+0x166c,0);sh(p+0x5e6,0);
  }
  s->stage=15;
 }
 if(s->stage==15){
  gh_player_frame_status e=b->input_mode1_00123938?
    b->input_mode1_00123938(b->context,f,&s->input_result,&pc):attack_input(s,b);
  if(e)return s->frontier?e:stop(s,pc?pc:0x123938,e);
  if(!s->input_result&&half(p+0x1618))p[0x1603]=2;
  s->stage=16;
 }
 if(s->stage==16){
  if(s->input_result){gh_player_frame_status e=camera_exit(s,b);if(e)return e;}
  s->stage=17;
 }
 s->frontier=0;
 return GH_PLAYER_FRAME_OK;
}

gh_player_frame_status gh_game_player_square_frame_0010d3a8(
 gh_player_square_frame *s,const gh_player_square_frame_bindings *b)
{
 if(!s||!b||!b->frame||!b->target_640)return GH_PLAYER_FRAME_ARGUMENT;
 const gh_player_frame_bindings *f=b->frame;
 if(!f->player||!f->player->view.bytes||f->player->view.size<GH_TITLE_PLAYER_BYTES||
    !f->player->view.alias_f0||!f->work||!f->init||!f->model||
    f->model->root.bytes!=f->player->view.bytes||!f->rate)return GH_PLAYER_FRAME_ARGUMENT;
 if(s->bindings&&s->bindings!=b)return GH_PLAYER_FRAME_BUSY;
 if(s->stage==17)return GH_PLAYER_FRAME_BUSY;
 if(f->numeric_policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return stop(s,0x10d3a8,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->bindings){
  if(word(f->player->view.bytes+0x640)!=b->target_640->handle)
   return stop(s,0x124540,GH_PLAYER_FRAME_ARGUMENT);
  s->bindings=b;
  s->motion=(gh_player_motion_request){.player=f->player,.model=f->model,.work=f->work,
   .resource=f->resource,.resource_bytes=f->resource_bytes,.rate=f->rate};
 }
 fenv_t env;
 if(feholdexcept(&env))return GH_PLAYER_FRAME_NUMERIC;
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_PLAYER_FRAME_NUMERIC;}
 s->frontier=0;
 gh_player_frame_status e=body(s,b);
 if(fesetenv(&env))return GH_PLAYER_FRAME_NUMERIC;
 return e;
}

gh_player_frame_status gh_player_square_frame_reset(gh_player_square_frame *s)
{
 if(!s)return GH_PLAYER_FRAME_ARGUMENT;
 if(s->bindings&&s->stage!=17)return GH_PLAYER_FRAME_BUSY;
 memset(s,0,sizeof(*s));
 return GH_PLAYER_FRAME_OK;
}
