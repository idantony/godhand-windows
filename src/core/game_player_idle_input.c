#include "game_action_button_set.h"
#include "game_player_idle_input.h"
#include "game_player_idle_target.h"
#include "game_player_square_input.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t w(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static float num(const void*p){float n;memcpy(&n,p,4);return n;}
static gh_player_frame_status fail(gh_player_idle_input*s,uint32_t pc,gh_player_frame_status e){s->frontier=pc;return e;}
static gh_player_frame_status done(gh_player_idle_input*s,int*out,int value){s->phase=100;s->frontier=0;*out=value;return GH_PLAYER_FRAME_OK;}
static gh_player_frame_status transition(gh_player_idle_input*s,int*out,unsigned sub,unsigned select){uint8_t*p=s->b.frame.player->view.bytes;p[0x2f4]=0;p[0x2f5]=(uint8_t)sub;p[0x2f6]=0;p[0x2f7]=(uint8_t)select;return done(s,out,1);}
gh_player_frame_status gh_game_player_input_cleanup_0012bfb8(const gh_player_frame_bindings *b,
 uint32_t *frontier)
{
 if(!b||!b->player||!b->player->view.bytes||!b->init||!frontier)return GH_PLAYER_FRAME_ARGUMENT;
 uint8_t *p=b->player->view.bytes;
 *frontier=0;
 if(!b->auxiliary_bindings_known){*frontier=0x12bfb8;return GH_PLAYER_FRAME_UNSUPPORTED;}
 if(b->aux_6a0||w(p+0x6a0))return GH_PLAYER_FRAME_OK;
 /* 2DC920 and 2DCAD8 use the same live-player and battle gates. */
 if(!b->current_player||b->current_player->bytes!=p){
  *frontier=0x2db6b0;return GH_PLAYER_FRAME_UNSUPPORTED;
 }
 int16_t hp;
 const uint16_t bits=(uint16_t)h(p+0x54a);
 memcpy(&hp,&bits,2);
 if(hp>0&&!(w(p+0x15f4)&0x400000)&&(int32_t)w(b->init->enemies+0x514)<1&&!b->init->enemies[0x53c]){
  if(!b->door_probe){*frontier=0x2c2b58;return GH_PLAYER_FRAME_UNSUPPORTED;}
  const gh_player_frame_status status=b->door_probe(b->door_context,&b->player->view,frontier);
  if(status){if(!*frontier)*frontier=0x2c2b58;return status;}
 }
 put(p+0x15f4,w(p+0x15f4)|0x100);
 return GH_PLAYER_FRAME_OK;
}

/* Original 2DC108 checks battle eligibility before reading the target point.
 * 2DB7E0 picks the short-angle prompt using XZ squared distance. */
gh_player_frame_status gh_game_player_interact_prompt_002dc108(
 const gh_player_idle_input_bindings *bindings,const gh_player_target *target,uint32_t *frontier)
{
 if(!bindings||!target||!frontier)return GH_PLAYER_FRAME_ARGUMENT;
 *frontier=0;
 if(!target->handle)return GH_PLAYER_FRAME_OK;
 const gh_player_frame_bindings *b=&bindings->frame;
 if(!b->work||(b->work->room!=0x101&&b->work->room!=0x102&&b->work->room!=0x103)){
  *frontier=0x2dc108;return GH_PLAYER_FRAME_UNSUPPORTED;
 }
 const gh_title_player_view *player=b->current_player;
 if(!player||!b->player||player->bytes!=b->player->view.bytes||
    !player->bytes||player->size<0x15f8||!b->init){
  *frontier=0x2db6b0;return GH_PLAYER_FRAME_ARGUMENT;
 }
 if((int16_t)h(player->bytes+0x54a)<=0||(w(player->bytes+0x15f4)&0x400000u)||
    (int32_t)w(b->init->enemies+0x514)>0||b->init->enemies[0x53c])
  return GH_PLAYER_FRAME_OK;
 if(!player->alias_f0||!target->view.alias_f0||!bindings->action_buttons){
  *frontier=0x2dc108;return GH_PLAYER_FRAME_ARGUMENT;
 }
 float from[3],to[3];
 memcpy(from,player->alias_f0,12);memcpy(to,target->view.alias_f0,12);
 fenv_t env;
 if(feholdexcept(&env)){*frontier=0x1f7eb0;return GH_PLAYER_FRAME_NUMERIC;}
 if(fesetround(FE_TONEAREST)){
  (void)fesetenv(&env);*frontier=0x1f7eb0;return GH_PLAYER_FRAME_NUMERIC;
 }
 volatile float dx=to[0]-from[0],dz=to[2]-from[2];
 volatile float xx=dx*dx,zz=dz*dz;
 const float squared=xx+zz;
 if(fesetenv(&env)||!isfinite(squared)){
  *frontier=0x1f7eb0;return GH_PLAYER_FRAME_NUMERIC;
 }
 int near=squared<=2;
 if(near&&squared>=0.5f){
  float turn;
  const gh_player_target_status status=gh_player_target_angle_002da038(b->elf,b->elf_bytes,
     from,to,num(player->bytes+0x104),3.1415927f,&turn,frontier);
  if(status)return status==GH_PLAYER_TARGET_NUMERIC?GH_PLAYER_FRAME_NUMERIC:GH_PLAYER_FRAME_UNSUPPORTED;
  near=fabsf(turn)<0.2617994f;
 }
 const gh_action_set_request request={.priority=near?8:7,.life=7,.callback_id=0x2dc1d0};
 int emitted;
 unsigned index;
 const gh_action_set_status status=gh_action_button_set_001f7858(
    bindings->action_buttons,&request,&emitted,&index,frontier);
 return status==GH_ACTION_SET_OK?GH_PLAYER_FRAME_OK:status==GH_ACTION_SET_UNSUPPORTED?
        GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
}

static gh_player_frame_status cleanup(gh_player_idle_input *s)
{
 uint32_t frontier=0;
 const gh_player_frame_status status=gh_game_player_input_cleanup_0012bfb8(&s->b.frame,&frontier);
 return status?fail(s,frontier,status):GH_PLAYER_FRAME_OK;
}
static gh_player_frame_status target_status(gh_player_idle_input*s,gh_player_target_status e,uint32_t pc){return fail(s,pc,e==GH_PLAYER_TARGET_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:e==GH_PLAYER_TARGET_NUMERIC?GH_PLAYER_FRAME_NUMERIC:GH_PLAYER_FRAME_CALLEE);}
enum player_input_mode { IDLE_INPUT, REACTION_INPUT, DODGE_INPUT };
static gh_player_frame_status body(gh_player_idle_input*s,int*out,enum player_input_mode mode){gh_player_frame_bindings*b=&s->b.frame;uint8_t*p=b->player->view.bytes,*heat=b->init->heat;const uint8_t*work=(const uint8_t*)b->work;uint32_t pc=0,flags;
 if(mode==DODGE_INPUT&&!s->phase){
  const unsigned seq=h(p+0x3ac);
  const unsigned pressed=w(work+8);
  if(w(p+0x15f4)&0x10u)return fail(s,0x2a9ad0,GH_PLAYER_FRAME_UNSUPPORTED);
  if(seq&0x40u){
   if(b->work->room==0x700)return fail(s,0x1169d8,GH_PLAYER_FRAME_UNSUPPORTED);
   if(w(work+0x5d8)&0x2000000u){
    if(!p[0x2f4]&&!p[0x2f5])return done(s,out,0);
    gh_player_frame_status e=cleanup(s);
    if(e)return e;
    return transition(s,out,0,0);
   }
   if(pressed&3u||p[0x621]||p[0x622]||p[0x623]||p[0x624])
    return fail(s,0x123acc,GH_PLAYER_FRAME_UNSUPPORTED);
   if(!b->auxiliary_bindings_known||b->aux_698||b->aux_6a0||w(p+0x698)||w(p+0x6a0))
    return fail(s,0x1240f8,GH_PLAYER_FRAME_UNSUPPORTED);
   put(p+0x15f4,w(p+0x15f4)|0x400u);
   if(p[0x627])return transition(s,out,6,0);
   if(p[0x61e]){
    if(!b->camera||!b->camera->constructed||!b->camera->identity)
     return fail(s,0x138f88,GH_PLAYER_FRAME_UNSUPPORTED);
    if(((uint32_t)b->camera->identity->mode&1u)&&w(b->camera->fields+0x520))return done(s,out,0);
    const float angle=num(p+0x614),signed_angle=num(p+0x610);
    if(!isfinite(angle)||!isfinite(signed_angle))return fail(s,0x1240f8,GH_PLAYER_FRAME_NUMERIC);
    if(angle<.7853982f){p[0x1603]=2;return transition(s,out,7,0);}
    put(p+0x54c,0x40a00000);
    return transition(s,out,6,angle>2.3561945f?1:signed_angle>0?2:3);
   }
  }
  if(seq&0x10u){
   put(p+0x15f4,w(p+0x15f4)|0x400u);
   if(pressed&4u)return transition(s,out,4,0);
  }
  if((seq&0x40u)&&!w(p+0x698)&&!w(p+0x6a0))put(p+0x15f4,w(p+0x15f4)|0x100u);
  if(!(seq&0x20u))return done(s,out,0);
  put(p+0x15f4,w(p+0x15f4)|0x400u);
  s->phase=8;
 }
 if(mode==REACTION_INPUT&&!s->phase){
  if(!(h(p+0x3ac)&0x10u))return fail(s,0x123acc,GH_PLAYER_FRAME_ARGUMENT);
  if((w(work+8)&7u)||p[0x621]||p[0x622]||p[0x623]||p[0x624])
   return fail(s,0x123acc,GH_PLAYER_FRAME_UNSUPPORTED);
  s->phase=4;
 }
 if(s->phase==0){flags=w(p+0x15f4);if(flags&0x10){float current=num(heat+4),minimum=num(heat+8),maximum=num(heat);if(!isfinite(current)||!isfinite(minimum)||!isfinite(maximum))return fail(s,0x2a9ad0,GH_PLAYER_FRAME_NUMERIC);if(current<minimum)current=minimum;float ratio=maximum!=0?current/maximum:0;if(!isfinite(ratio))return fail(s,0x2a9b50,GH_PLAYER_FRAME_NUMERIC);memcpy(heat+4,&current,4);put(heat+0x14,ratio<.5f?0:ratio<=.75f?1:2);if(current<=minimum&&!(flags&0x80200)&&!(p[0x2f4]==0&&p[0x2f5]==9)&&!(h(p+0x3ac)&0xc)){float threshold=num(heat+12);if(!isfinite(threshold))return fail(s,0x2a9cc8,GH_PLAYER_FRAME_NUMERIC);volatile float next_minimum=current-threshold;float minimum_value=next_minimum<0?0:next_minimum;heat[0x10]=0;memcpy(heat+8,&minimum_value,4);put(p+0x54c,0x40400000);return transition(s,out,9,0);}}s->phase=1;}
 if(s->phase==1){flags=w(p+0x15f4);if(!(flags&0x10)&&!heat[0x10]&&!(flags&0x20)){put(p+0x15f4,flags|0x20);s->effect=(gh_player_frame_effect_request){&b->player->view,0x2e,0,0,0xffffffffu,-1,0};s->phase=2;}else s->phase=3;}
 if(s->phase==2){if(!b->effect_create)return fail(s,0x306528,GH_PLAYER_FRAME_UNSUPPORTED);gh_player_frame_status e=b->effect_create(b->effect_context,&s->effect,&pc);if(e)return fail(s,pc?pc:0x306528,e);s->phase=3;}
 if(s->phase==3){if(h(work+0x5b0)==0x700)return fail(s,0x2c2638,GH_PLAYER_FRAME_UNSUPPORTED);s->phase=4;}
 if(s->phase==4){
  if(w(work+0x5d8)&0x2000000){
   if(!p[0x2f4]&&!p[0x2f5])return done(s,out,0);
   gh_player_frame_status e=cleanup(s);if(e)return e;
   return transition(s,out,0,0);
  }
  put(p+0x15f4,w(p+0x15f4)|0x400);
  if((w(work+8)&2)&&!heat[0x10]){
   float current=num(heat+4),threshold=num(heat+12);
   if(!isfinite(current)||!isfinite(threshold))return fail(s,0x2a9bc0,GH_PLAYER_FRAME_NUMERIC);
   unsigned ready=!(current<threshold);put(heat+0x14,ready);
   if(ready)return transition(s,out,8,0);
  }
  if(!b->auxiliary_bindings_known)return fail(s,0x123b14,GH_PLAYER_FRAME_UNSUPPORTED);
  if(p[0x624]&&!b->aux_6a0&&!w(p+0x6a0)){
   if(!b->save.bytes||b->save.size<=0xa8)return fail(s,0x1fac58,GH_PLAYER_FRAME_UNSUPPORTED);
   if(b->save.bytes[0xa8])return transition(s,out,0x2b,0);
  }
  if(b->aux_698||w(p+0x698)||w(p+0x6a0))return fail(s,0x123b14,GH_PLAYER_FRAME_UNSUPPORTED);
  if(b->aux_6a0&&(p[0x621]||p[0x622]||p[0x623]))return transition(s,out,0x16,0);
  s->phase=13;
 }
 if(s->phase==13){
  if(!h(p+0x5e0)){
   gh_player_target nearby;
   gh_player_target_status e=gh_player_enemy_target_00291010(&s->b.targets,b->init,2.5f,1.57079637f,&nearby,&pc);
   if(e)return target_status(s,e,pc);
   gh_player_idle_target_result result;
   e=gh_player_idle_target_00123bf0(&b->player->view,&nearby,b->init,s->b.action_buttons,0,0,&result,&pc);
   if(e)return target_status(s,e,pc);
   if(result.transitioned){
    put(p+0x640,result.target_640.handle);
    return done(s,out,1);
   }
  }
  s->phase=14;
 }
 if(s->phase==14){
  if(p[0x622]){
   /* 123CE4 first selects the real current attack target. The analog
    * target-height transition remains outside this neutral Triangle route. */
   if(p[0x61f])return fail(s,0x123d80,GH_PLAYER_FRAME_UNSUPPORTED);
   gh_player_target selected={.handle=w(p+0x640)};
   if(selected.handle&&(int8_t)p[0x648]>0){
    if(!s->b.targets.resolve)return fail(s,0x124540,GH_PLAYER_FRAME_UNSUPPORTED);
    gh_player_target_status e=s->b.targets.resolve(s->b.targets.context,selected.handle,&selected.view,&pc);
    if(e)return target_status(s,e,pc?pc:0x124540);
   }
   gh_player_target_status e=gh_player_attack_target_00124540(&s->b.targets,b->init,&selected,&pc);
   if(e)return target_status(s,e,pc?pc:0x124540);
   if(selected.handle){
    /* Original +6C is 275978 for cEm00. It also writes actor+1680. */
    float xyz[3];
    e=gh_enemy_target_position_00275978(&selected.view,xyz);
    if(e)return target_status(s,e,0x123d04);
   }
  }
  s->phase=5;
 }
 if(s->phase==5){if(s->b.targets.player!=&b->player->view)return fail(s,0x129cc8,GH_PLAYER_FRAME_ARGUMENT);gh_player_target_status e=gh_player_small_object_gate_00129cc8(&s->b.targets,&pc);if(e)return target_status(s,e,pc);s->phase=6;}
 if(s->phase==6){/*2948C8's TWO original fields, notenemy_kept16. */if(w(b->init->enemies+0x5ac)||w(b->init->enemies+0x5b0))return fail(s,0x2948c8,GH_PLAYER_FRAME_UNSUPPORTED);s->phase=7;}
 if(s->phase==7&&mode==REACTION_INPUT)return done(s,out,0);
 if(s->phase==7&&!p[0x623]&&p[0x622]){
  int selected;
  gh_player_frame_status e=gh_player_triangle_input_00123f44(&b->player->view,b->save,0,&selected,&pc);
  if(e)return fail(s,pc,e);
  if(selected)return done(s,out,1);
 }
 if(s->phase==7&&!p[0x623]&&!p[0x622]&&p[0x621]){
  int selected;
  gh_player_frame_status e=gh_player_square_input_00123fc8(&b->player->view,b->save,b->init,0,&selected,&pc);
  if(e)return fail(s,pc,e);
  if(selected)return done(s,out,1);
 }
 if(s->phase==7){if(p[0x623]||p[0x621]){if(p[0x61f]&&num(p+0x608)>2.3561945f)return fail(s,0x12c000,GH_PLAYER_FRAME_UNSUPPORTED);if(w(p+0x15f4)&0x8000)return transition(s,out,12,p[0x623]?2:0);return fail(s,0x12c000,GH_PLAYER_FRAME_UNSUPPORTED);}if(w(work+8)&1){if(!p[0x2f4]&&p[0x2f5]==10)return done(s,out,0);return transition(s,out,10,0);}put(p+0x15f4,w(p+0x15f4)|0x400);if(p[0x627])return transition(s,out,6,0);if(p[0x61e]){if(!b->camera||!b->camera->constructed||!b->camera->identity)return fail(s,0x138f88,GH_PLAYER_FRAME_UNSUPPORTED);if(((uint32_t)b->camera->identity->mode&1u)&&w(b->camera->fields+0x520))return done(s,out,0);float angle=num(p+0x614),signed_angle=num(p+0x610);if(!isfinite(angle)||!isfinite(signed_angle))return fail(s,0x1240f8,GH_PLAYER_FRAME_NUMERIC);if(angle<.7853982f){p[0x1603]=2;return transition(s,out,7,0);}put(p+0x54c,0x40a00000);return transition(s,out,6,angle>2.3561945f?1:signed_angle>0?2:3);}if(w(work+8)&4){if(!p[0x2f4]&&p[0x2f5]==4)return done(s,out,0);return transition(s,out,4,0);}put(p+0x15f4,w(p+0x15f4)|0x500);s->phase=8;}
 if(s->phase==8){if(w(p+0x15f4)&0x20){gh_player_target_status e=gh_player_pickup_target_0012a610(&s->b.targets,&s->pickup,&pc);if(e)return target_status(s,e,pc);/* Typed companion is original69C; nohostpointer truncation. */put(p+0x69c,0);s->phase=9;}else s->phase=12;}
 if(s->phase==9){if(s->pickup.handle)return fail(s,s->pickup.view.bytes[0x665]?0x2dbfb0:0x2dbd40,GH_PLAYER_FRAME_UNSUPPORTED);gh_player_target_status e=gh_player_interact_target_0012b328(&s->b.targets,&s->interaction,&pc);if(e)return target_status(s,e,pc);put(p+0x6a4,0);s->phase=10;}
 if(s->phase==10){
  gh_player_frame_status e=gh_game_player_interact_prompt_002dc108(&s->b,&s->interaction,&pc);
  if(e)return fail(s,pc?pc:0x2dc108,e);
  s->phase=11;
 }
 if(s->phase==11){if(h(work+0x5b0)==0x608)return fail(s,0x2c2568,GH_PLAYER_FRAME_UNSUPPORTED);s->phase=12;}
 if(s->phase==12){if(!p[0x61f]){if(mode==DODGE_INPUT)return done(s,out,0);if((int8_t)p[0x1601]>0||(int8_t)p[0x1602]>0)return done(s,out,0);if(!p[0x2f4]&&!p[0x2f5])return done(s,out,0);gh_player_frame_status e=cleanup(s);if(e)return e;return transition(s,out,0,0);}float angle=num(p+0x608),magnitude=num(p+0x60c);if(!isfinite(angle)||!isfinite(magnitude))return fail(s,0x1243a0,GH_PLAYER_FRAME_NUMERIC);unsigned next=angle<1.2217305f?(magnitude>=10000?2:1):angle>2.0943952f?3:0;if(!p[0x2f4]&&p[0x2f5]==next)return done(s,out,0);gh_player_frame_status e=cleanup(s);if(e)return e;return transition(s,out,next,next==2&&(w(p+0x15f4)&0x4000)?1:0);}
 return fail(s,0x123938,GH_PLAYER_FRAME_ARGUMENT);
}
static gh_player_frame_status player_input(gh_player_idle_input*s,const gh_player_idle_input_bindings*b,int*out,enum player_input_mode mode){if(!s||!b||!out||!b->frame.player||!b->frame.player->view.bytes||b->frame.player->view.size<GH_TITLE_PLAYER_BYTES||!b->frame.work||!b->frame.init)return GH_PLAYER_FRAME_ARGUMENT;const uint8_t binding=(uint8_t)(mode+1);if(s->bound&&(s->source_bindings!=b||s->bound!=binding))return GH_PLAYER_FRAME_BUSY;if(!s->bound){s->b=*b;s->source_bindings=b;s->bound=binding;}if(s->phase==100)return fail(s,0x123938,GH_PLAYER_FRAME_BUSY);fenv_t env;if(feholdexcept(&env))return GH_PLAYER_FRAME_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_PLAYER_FRAME_NUMERIC;}gh_player_frame_status e=body(s,out,mode);if(fesetenv(&env))return GH_PLAYER_FRAME_NUMERIC;return e;}

gh_player_frame_status gh_game_player_idle_input_00123938(gh_player_idle_input *s,
 const gh_player_idle_input_bindings *b,int *out)
{
 return player_input(s,b,out,IDLE_INPUT);
}

gh_player_frame_status gh_game_player_reaction_input_active_00123acc(gh_player_idle_input *s,
 const gh_player_idle_input_bindings *b,int *out)
{
 return player_input(s,b,out,REACTION_INPUT);
}

gh_player_frame_status gh_player_idle_input_begin(gh_player_idle_input*s){if(!s)return GH_PLAYER_FRAME_ARGUMENT;if(s->bound&&s->phase!=100)return GH_PLAYER_FRAME_BUSY;s->phase=0;s->frontier=0;s->bound=0;s->source_bindings=NULL;return GH_PLAYER_FRAME_OK;}

gh_player_frame_status gh_game_player_idle_cleanup_0012bfb8(gh_player_idle_input*s){if(!s||!s->bound)return GH_PLAYER_FRAME_ARGUMENT;return cleanup(s);}

gh_player_frame_status gh_game_player_dodge_input_00123938(gh_player_idle_input *s,
 const gh_player_idle_input_bindings *b,int *out)
{
 return player_input(s,b,out,DODGE_INPUT);
}