#include "sdk_rotation_trig.h"
#include "game_player_frame.h"
#include "game_player_attack_hitarea.h"
#include "game_action_button_set.h"
#include "game_player_idle_input.h"
#include "game_player_walk.h"
#include "game_player_run.h"
#include "id_layout_rotation.h"
#include "game_scene_at_geometry.h"
#include "game_actor_finish_model.h"
#include "game_hitarea_frame.h"
#include "game_player_root_delta.h"
#include "game_player_post_init.h"
#include "game_enemy_ems.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t w(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static float number(uint32_t n){float f;memcpy(&f,&n,4);return f;}
static void sf(void*p,float f){memcpy(p,&f,4);}
static float sub(float a,float b){volatile float f=a-b;return f;}
static float add(float a,float b){volatile float f=a+b;return f;}
static gh_player_frame_status fail(gh_game_player_frame*s,uint32_t pc,gh_player_frame_status e){s->frontier=pc;return e;}
static gh_player_frame_status enemy_focus(gh_game_player_frame *s,float range,float cone,gh_player_target *out)
{
 const gh_player_frame_bindings *b=&s->b;
 if(!b->idle_input_bindings){
  if(b->init->enemy_top||w(b->init->enemies+0x500))return fail(s,0x291010,GH_PLAYER_FRAME_UNSUPPORTED);
  *out=(gh_player_target){0};
  return GH_PLAYER_FRAME_OK;
 }
 const gh_player_target_bindings *targets=&b->idle_input_bindings->targets;
 if(targets->player!=&b->player->view)return fail(s,0x291010,GH_PLAYER_FRAME_ARGUMENT);
 uint32_t pc=0;
 gh_player_target_status status=gh_player_enemy_target_00291010(targets,b->init,range,cone,out,&pc);
 if(!status)return GH_PLAYER_FRAME_OK;
 return fail(s,pc?pc:0x291010,status==GH_PLAYER_TARGET_NUMERIC?GH_PLAYER_FRAME_NUMERIC:
  status==GH_PLAYER_TARGET_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
}
static void decrement_byte(uint8_t*p){if(*p)*p=(uint8_t)(*p-1u);}
static void decrement_half(uint8_t*p){uint16_t n;memcpy(&n,p,2);if(n){n=(uint16_t)(n-1u);memcpy(p,&n,2);}}
static void history_clear(uint8_t*p){memset(p+0x1644,0,20);}
static void history_push(uint8_t*p,uint32_t value){for(unsigned i=0;i<5;i++)if(!w(p+0x1644+i*4)){put(p+0x1644+i*4,value);put(p+0x1658,15);return;}}
gh_player_frame_status gh_game_player_button_history_00126820(gh_title_player_view*v,const gh_boot_work*work){if(!v||!v->bytes||v->size<0x165c||!work)return GH_PLAYER_FRAME_ARGUMENT;uint8_t*p=v->bytes;uint32_t pressed=w((const uint8_t*)work+8);if(!(pressed&0x90)){uint32_t n=w(p+0x1658);if(!n)history_clear(p);else put(p+0x1658,n-1);}else if(pressed&0x10){history_clear(p);history_push(p,2);}else history_push(p,1);return GH_PLAYER_FRAME_OK;}
static int same(const gh_player_frame_bindings*a,const gh_player_frame_bindings*b){return a->player==b->player&&a->work==b->work&&a->init==b->init&&a->camera==b->camera&&a->opacity_camera==b->opacity_camera&&a->save.bytes==b->save.bytes&&a->save.size==b->save.size&&a->scenario.bytes==b->scenario.bytes&&a->scenario.size==b->scenario.size&&a->effects==b->effects&&a->current_player==b->current_player&&a->elf==b->elf&&a->elf_bytes==b->elf_bytes&&a->numeric_policy==b->numeric_policy&&a->effect_create==b->effect_create&&a->effect_context==b->effect_context&&a->scene_at==b->scene_at&&a->take_object==b->take_object&&a->give_objects==b->give_objects&&a->solids==b->solids&&a->collision_pairs==b->collision_pairs&&a->take_binding_known==b->take_binding_known&&a->give_binding_known==b->give_binding_known&&a->give_present_mask==b->give_present_mask&&a->model==b->model&&a->resource==b->resource&&a->resource_bytes==b->resource_bytes&&a->rate==b->rate&&a->initialize_motion==b->initialize_motion&&a->advance_motion==b->advance_motion&&a->motion_context==b->motion_context&&a->death_fade==b->death_fade&&a->grab_camera==b->grab_camera&&a->grab_attacker==b->grab_attacker&&a->grab_attacker_retired==b->grab_attacker_retired&&a->grab_resource.bytes==b->grab_resource.bytes&&a->grab_resource.size==b->grab_resource.size&&a->aux_698==b->aux_698&&a->aux_6a0==b->aux_6a0&&a->auxiliary_bindings_known==b->auxiliary_bindings_known&&a->idle_input==b->idle_input&&a->idle_input_bindings==b->idle_input_bindings&&a->node_cache==b->node_cache&&a->door_probe==b->door_probe&&a->door_context==b->door_context&&a->actors==b->actors&&a->ground_globals==b->ground_globals&&a->ik_globals==b->ik_globals&&a->atan_coeff==b->atan_coeff&&a->opacity_globals==b->opacity_globals&&a->aux_68c==b->aux_68c&&a->auxiliary_68c_binding_known==b->auxiliary_68c_binding_known&&a->target_644==b->target_644&&a->cockpit_flags_1ca0==b->cockpit_flags_1ca0&&a->square_attack==b->square_attack&&a->square_context==b->square_context&&a->damage_sound==b->damage_sound&&a->damage_sound_context==b->damage_sound_context&&a->damage_effect==b->damage_effect&&a->damage_abort==b->damage_abort&&a->damage_camera_exit==b->damage_camera_exit&&a->damage_context==b->damage_context&&a->damage_vibration_rows==b->damage_vibration_rows&&a->paper_release==b->paper_release&&a->paper_context==b->paper_context&&a->interact_prop==b->interact_prop&&a->throw_prop==b->throw_prop&&a->roulette_target_owner==b->roulette_target_owner&&a->roulette_time_scale==b->roulette_time_scale&&a->roulette_projectile==b->roulette_projectile;}
static int elf_word(const gh_player_frame_bindings*b,uint32_t address,uint32_t*out){const uint8_t*p=b->elf;size_t n=b->elf_bytes;if(!p||n<52||memcmp(p,"\177ELF\1\1\1",7))return 0;uint32_t ph=w(p+28);uint16_t size,count;memcpy(&size,p+42,2);memcpy(&count,p+44,2);if(size!=32||count>128||ph>n||(size_t)count*32>n-ph)return 0;for(unsigned i=0;i<count;i++){const uint8_t*q=p+ph+i*32;uint32_t base=w(q+8),off=w(q+4),files=w(q+16);if(w(q)!=1||address<base||(uint64_t)address+4>(uint64_t)base+files)continue;uint64_t o=(uint64_t)off+address-base;if(o>n||4>n-o)return 0;*out=w(p+(size_t)o);return 1;}return 0;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void sh(uint8_t*p,uint16_t n){memcpy(p,&n,2);}
static int heat_rank(const uint8_t*h,float current,uint32_t*out){float maximum=number(w(h));if(!isfinite(current)||!isfinite(maximum))return 0;volatile float ratio=maximum!=0?current/maximum:0;if(!isfinite(ratio))return 0;*out=ratio<.5f?0:(ratio<=.75f?1:2);return 1;}
static gh_player_frame_status heat_frame(gh_game_player_frame*s){const gh_player_frame_bindings*b=&s->b;uint8_t*p=b->player->view.bytes,*h=b->init->heat;uint32_t flags=w(p+0x15f4),pc=0;const uint8_t*work=(const uint8_t*)b->work;
 if(!s->heat_stage){
  if(w(p+0x6a8)&&!(flags&0x20)){
   if(!b->paper_release)return fail(s,0x1cf6e0,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_frame_status status=b->paper_release(b->paper_context,&b->player->view,b->save,&pc);
   if(status)return fail(s,pc?pc:0x1cf6e0,status);
   if(w(p+0x6a8))return fail(s,0x1234a8,GH_PLAYER_FRAME_CALLEE);
  }
  if(p[0x2f4]==5||p[0x2f4]==6){s->heat_stage=7;return GH_PLAYER_FRAME_OK;}s->heat_stage=1;
 }
 if(s->heat_stage==1){if(w(work+0x58c)&0x80000){if(!b->save.bytes||b->save.size<0x158)return fail(s,0x1fabb0,GH_PLAYER_FRAME_ARGUMENT);float maximum=number(w(h));uint32_t rank;if(!heat_rank(h,maximum,&rank))return fail(s,0x2a9b50,GH_PLAYER_FRAME_NUMERIC);sf(h+4,maximum);put(h+0x14,rank);unsigned count=0,slots=b->save.bytes[0x156];if(slots>6)slots=6;for(unsigned i=0;i<slots;i++)if(b->save.bytes[0xa8+i])count++;if(count<3)for(unsigned i=0;i<slots;i++)if(!b->save.bytes[0xa8+i]){b->save.bytes[0xa8+i]=1;break;}}s->heat_stage=2;}
 if(s->heat_stage==2){if(h[0x10]&&!(flags&0x200)){uint32_t constant;if(!elf_word(b,0x3bd4a0,&constant))return fail(s,0x123460,GH_PLAYER_FRAME_ARGUMENT);float rate=number(constant),factor=number(w(p+0x5a8)),current=number(w(h+4)),minimum=number(w(h+8));if(!isfinite(rate)||!isfinite(factor)||!isfinite(current)||!isfinite(minimum))return fail(s,0x2a9ad0,GH_PLAYER_FRAME_NUMERIC);if(flags&0x80000){volatile float quarter=rate*.25f;rate=quarter;}volatile float drain=rate*factor,result=current-drain;float next=result;if(next<minimum)next=minimum;uint32_t rank;if(!heat_rank(h,next,&rank))return fail(s,0x2a9b50,GH_PLAYER_FRAME_NUMERIC);sf(h+4,next);put(h+0x14,rank);}s->heat_stage=3;}
 if(s->heat_stage==3){if(!(flags&0x20)){if(half(p+0x5f2)==3){s->heat_stage=6;}else{sh(p+0x5f2,3);unsigned mode=b->save.bytes&&b->save.size>0xae?b->save.bytes[0xae]:0;int special=mode==1||mode==3||mode==5||mode==7;s->pending_effect=(gh_player_frame_effect_request){special?b->current_player:&b->player->view,special?0x72:0x21,0,0,3,-1,0};s->heat_stage=5;}}else{float current=number(w(h+4)),threshold=number(w(h+12));if(!isfinite(current)||!isfinite(threshold))return fail(s,0x2a9bc0,GH_PLAYER_FRAME_NUMERIC);s->heat_ready=(uint8_t)(!(current<threshold));put(h+0x14,s->heat_ready);unsigned phase=half(p+0x5f2);if((!s->heat_ready&&phase==0)||(s->heat_ready&&phase==1))s->heat_stage=6;else s->heat_stage=4;}}
 if(s->heat_stage==4){if(!b->effects)return fail(s,0x306200,GH_PLAYER_FRAME_UNSUPPORTED);unsigned slot;gh_effect_status e=gh_game_effect_release_00306200(b->effects,&b->player->view,3,2,&pc,&slot);if(e)return fail(s,pc?pc:0x306200,GH_PLAYER_FRAME_CALLEE);sh(p+0x5f2,s->heat_ready?1:0);if(!s->heat_ready)s->heat_stage=6;else{unsigned mode=b->save.bytes&&b->save.size>0xae?b->save.bytes[0xae]:0;int special=mode==1||mode==3||mode==5||mode==7;s->pending_effect=(gh_player_frame_effect_request){b->current_player,special?0x73:0x25,0,0,3,-1,0};s->heat_stage=5;}}
 if(s->heat_stage==5){if(!s->pending_effect.actor)return fail(s,0x2db6b0,GH_PLAYER_FRAME_UNSUPPORTED);if(!b->effect_create)return fail(s,0x306528,GH_PLAYER_FRAME_UNSUPPORTED);gh_player_frame_status e=b->effect_create(b->effect_context,&s->pending_effect,&pc);if(e)return fail(s,pc?pc:0x306528,e);s->heat_stage=6;}
 if(s->heat_stage==6){put(p+0x5dc,0x3f800000);put(p+0x5a8,0x3f800000);put(p+0x444,(half(p+0x434)&4)?0xbf800000:0x3f800000);s->heat_stage=7;}return GH_PLAYER_FRAME_OK;
}

static unsigned damage_rank_001fa010(int points,const int32_t thresholds[5])
{
 unsigned index=0;
 while(index<5&&points>=thresholds[index])++index;
 return index<3?index+1:5;
}

static int damage_rank_thresholds(const gh_player_frame_bindings *b,int32_t thresholds[5])
{
 for(unsigned i=0;i<5;++i){
  uint32_t raw;
  if(!elf_word(b,0x3bf160+4*i,&raw))return 0;
  memcpy(thresholds+i,&raw,4);
 }
 return 1;
}

gh_player_frame_status gh_core_save_game_level_001fa010(gh_core_save_view save,
    const void *elf,size_t elf_bytes,unsigned *level)
{
 if(!level)return GH_PLAYER_FRAME_ARGUMENT;
 *level=0;
 if(!save.bytes)return GH_PLAYER_FRAME_OK;
 const gh_player_frame_bindings asset={.elf=elf,.elf_bytes=elf_bytes};
 int32_t thresholds[5];
 if(save.size<0x1e||!damage_rank_thresholds(&asset,thresholds))return GH_PLAYER_FRAME_ARGUMENT;
 *level=damage_rank_001fa010((int16_t)half(save.bytes+0x1c),thresholds);
 return GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status damage_hp_0012c0f8(gh_game_player_frame *s,int32_t power)
{
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes,*heat=b->init->heat;
 const gh_core_save_view save=b->save;
 unsigned rank;
 if((save.bytes&&save.size<0x20)||gh_core_save_game_level_001fa010(save,b->elf,b->elf_bytes,&rank))
  return fail(s,0x1fa010,GH_PLAYER_FRAME_ARGUMENT);
 const unsigned difficulty=save.bytes?save.bytes[0x1f]:1;
 const int easy=save.bytes&&!difficulty&&!(w(save.bytes+0x14)&0x4000000u);
 const uint16_t continues=(uint16_t)b->init->continue_count;
 const int16_t old_hp=(int16_t)half(p+0x54a);
 fenv_t env;
 if(feholdexcept(&env))return fail(s,0x12c0f8,GH_PLAYER_FRAME_NUMERIC);
 if(fesetround(FE_TOWARDZERO)){
  (void)fesetenv(&env);
  return fail(s,0x12c0f8,GH_PLAYER_FRAME_NUMERIC);
 }
 const float factor=rank==5?1.2f:rank>=3?1.0f:rank==2?.8f:.6f;
 volatile float scale=factor*(easy?.5f:.75f);
 if(difficulty!=2&&continues>1)scale=scale*(continues==2?.9f:continues==3?.8f:.7f);
 volatile float input=(float)power;
 volatile float product=input*scale;
 volatile float rounded=product+.5f;
 if(!isfinite(rounded)||rounded<-2147483648.0f||rounded>=2147483648.0f){
  (void)fesetenv(&env);
  return fail(s,0x12c26c,GH_PLAYER_FRAME_NUMERIC);
 }
 int32_t damage=(int32_t)rounded;
 if(damage>32000)damage=32000;
 if(power>0&&damage<1)damage=1;
 uint16_t hp=(uint16_t)((uint32_t)(uint16_t)old_hp-(uint32_t)damage);
 if(!hp||hp>=0x8000)hp=old_hp>10?1:0;
 float next=0,next_peak=0;
 uint32_t heat_rank_next=0;
 const int add_heat=damage>0&&!heat[0x10];
 if(add_heat){
  const int16_t maximum_hp=(int16_t)half(p+0x548);
  const float maximum=number(w(heat)),current=number(w(heat+4)),peak=number(w(heat+8));
  if(!save.bytes||maximum_hp<=0||!isfinite(maximum)||maximum<=0||
     !isfinite(current)||!isfinite(peak)){
   (void)fesetenv(&env);
   return fail(s,0x2a9978,GH_PLAYER_FRAME_UNSUPPORTED);
  }
  volatile float inverse=1.0f/(float)maximum_hp;
  volatile float fraction=(float)damage*inverse;
  volatile float amount=fraction*10.0f;
  if(amount>10.0f)amount=10.0f;
  if(easy)amount=amount*1.25f;
  if(b->work->room==0x801)amount=amount*1.25f;
  volatile float sum=current+amount;
  next=maximum<sum?maximum:sum;
  volatile float added=next-current;
  volatile float peak_sum=peak+added;
  next_peak=maximum<peak_sum?maximum:peak_sum;
  if(!heat_rank(heat,next,&heat_rank_next)||!isfinite(next_peak)){
   (void)fesetenv(&env);
   return fail(s,0x2a9b50,GH_PLAYER_FRAME_NUMERIC);
  }
 }
 if(fesetenv(&env))return fail(s,0x12c0f8,GH_PLAYER_FRAME_NUMERIC);
 sh(p+0x54a,hp);
 if(add_heat){
  sf(heat+4,next);
  sf(heat+8,next_peak);
  put(heat+0x14,heat_rank_next);
 }
 return GH_PLAYER_FRAME_OK;
}

/* 109A64 deducts 160 for kind 37 and 320 for kind 3E. Both calls occur in R100. */
static gh_player_frame_status damage_score_001fa280(gh_game_player_frame *s,int penalty)
{
 const gh_player_frame_bindings *b=&s->b;
 gh_core_save_view save=b->save;
 if(!save.bytes)return GH_PLAYER_FRAME_OK;
 if(save.size<0x20)return fail(s,0x1fa280,GH_PLAYER_FRAME_ARGUMENT);
 if((w((const uint8_t *)b->work+0x5e4)&0x400000u)||save.bytes[0x1f]==2)
  return GH_PLAYER_FRAME_OK;
 if(w(save.bytes+0x14)&0x4000000u)return fail(s,0x1fa2ec,GH_PLAYER_FRAME_UNSUPPORTED);
 int32_t thresholds[5];
 if(!damage_rank_thresholds(b,thresholds))return fail(s,0x1fa010,GH_PLAYER_FRAME_ARGUMENT);
 const int maximum=thresholds[save.bytes[0x1f]?4:1];
 if(maximum<1||maximum>INT16_MAX)return fail(s,0x1fa4bc,GH_PLAYER_FRAME_UNSUPPORTED);
 const int old=(int16_t)half(save.bytes+0x1c);
 const unsigned old_rank=damage_rank_001fa010(old,thresholds);
 int change=-penalty;
 if(change>0){
  static const float gain[]={1.5f,1.2f,1.0f,0.7f,0.5f};
  fenv_t env;
  if(feholdexcept(&env))return fail(s,0x1fa3fc,GH_PLAYER_FRAME_NUMERIC);
  if(fesetround(FE_TOWARDZERO)){
   (void)fesetenv(&env);
   return fail(s,0x1fa3fc,GH_PLAYER_FRAME_NUMERIC);
  }
  volatile float multiplier=gain[old_rank-1];
  if((uint16_t)b->init->continue_count>1)multiplier=multiplier*0.8f;
  volatile float scaled=multiplier*(float)change;
  if(fesetenv(&env))return fail(s,0x1fa3fc,GH_PLAYER_FRAME_NUMERIC);
  change=(int)scaled;
 }
 int points=(int16_t)(uint16_t)(old+change);
 if(points<0)points=0;
 if(points>=maximum)points=maximum-1;
 unsigned rank=damage_rank_001fa010(points,thresholds);
 if(rank+1<old_rank)rank=old_rank-1;
 if(rank==4)rank=3;
 if(rank>old_rank)save.bytes[0x1e]=1;
 if(rank<old_rank&&save.bytes[0x1e]){
  --save.bytes[0x1e];
  points=(int16_t)thresholds[rank-1];
 }
 sh(save.bytes+0x1c,(uint16_t)points);
 return GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status hurt_sound_with_bank(gh_game_player_frame *s,int16_t program,
 int16_t bank,const gh_title_player_view *bank_actor)
{
 const gh_player_frame_bindings *b=&s->b;
 if(!b->damage_sound||!b->model)return fail(s,0x2cba48,GH_PLAYER_FRAME_UNSUPPORTED);
 s->damage_sound_request=(gh_motion_sound_request){.model=b->model,.actor=&b->player->view,
    .bank=bank,.program=program,.other_actor=bank_actor};
 uint32_t handle=0,pc=0;
 gh_actor_motion_status status=b->damage_sound(b->damage_sound_context,&s->damage_sound_request,&handle,&pc);
 return !status?GH_PLAYER_FRAME_OK:fail(s,pc?pc:0x2cba48,
     status==GH_ACTOR_MOTION_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
}

static gh_player_frame_status hurt_sound(gh_game_player_frame *s,int16_t program)
{
 return hurt_sound_with_bank(s,program,0,NULL);
}

static gh_player_frame_status hurt_vibration_0012c348(gh_game_player_frame *s,unsigned duration_frames)
{
 const gh_player_frame_bindings *b=&s->b;
 const uint8_t *work=(const uint8_t *)b->work;
 if(w(work+0x58c)&0x08000000u)return GH_PLAYER_FRAME_OK;
 if(!b->damage_vibration_rows)return fail(s,0x31bb08,GH_PLAYER_FRAME_UNSUPPORTED);
 const uint32_t duration=(duration_frames<<16)<<(w(work+0x564)&31u);
 for(unsigned i=0;i<32;++i){
  uint8_t *row=b->damage_vibration_rows[i];
  if(row[0])continue;
  memcpy(row,(const uint8_t[4]){3,255,0,0},4);
  sh(row+4,(uint16_t)(duration+1));
  sh(row+6,(uint16_t)(duration>>16));
  memset(row+10,0,4);
  break;
 }
 return GH_PLAYER_FRAME_OK;
}

/* Neutral animation-gated caller. Original mode 1 returns zero at 1242EC
 * when there is no enabled input, preserving the current animation owner. */
static gh_player_frame_status hurt_input_00123938(gh_game_player_frame *s)
{
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes;
 const uint8_t *work=(const uint8_t *)b->work;
 const unsigned animation=half(p+0x3ac);
 const uint32_t flags=w(p+0x15f4);
 if(flags&0x10u){
  int expired;
  uint32_t frontier=0;
  const gh_player_frame_status status=gh_game_player_input_heat_00123938(b,1,&expired,&frontier);
  if(status)return fail(s,frontier,status);
  if(expired){
   s->input_result=1;
   return GH_PLAYER_FRAME_OK;
  }
 }
 if(animation&0x10u){
  if(!b->idle_input||!b->idle_input_bindings||
     (!(animation&0x40u)&&(w(work+0x5d8)&0x02000000u)))
   return fail(s,0x123acc,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_bindings *source=&b->idle_input_bindings->frame;
  if(source->player!=b->player||source->init!=b->init||source->work!=b->work||
     source->current_player!=b->current_player)return fail(s,0x123acc,GH_PLAYER_FRAME_ARGUMENT);
  if(!s->input_started){
   gh_player_frame_status status=gh_player_idle_input_begin(b->idle_input);
   if(status)return fail(s,0x123acc,status);
   s->input_started=1;
  }
  if(s->input_started==1){
   gh_player_frame_status status=gh_game_player_reaction_input_active_00123acc(
    b->idle_input,b->idle_input_bindings,&s->input_result);
   if(status)return fail(s,b->idle_input->frontier?b->idle_input->frontier:0x123acc,status);
   s->input_started=2;
  }
  if(s->input_result)return GH_PLAYER_FRAME_OK;
 }
 if(animation&0x40u){
  if(b->work->room==0x700||(w(work+0x5d8)&0x02000000u)||p[0x627]||
     !b->auxiliary_bindings_known||b->aux_698||b->aux_6a0||w(p+0x698)||w(p+0x6a0))
   return fail(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);
  if(p[0x61e]){
   /* 1240DC admits the original directional dodge during SEQ bit 40. */
   put(p+0x15f4,w(p+0x15f4)|0x400u);
   if(!b->camera||!b->camera->constructed||!b->camera->identity)
    return fail(s,0x138f88,GH_PLAYER_FRAME_UNSUPPORTED);
   if(((uint32_t)b->camera->identity->mode&1u)&&w(b->camera->fields+0x520))
    return GH_PLAYER_FRAME_OK;
   uint32_t pc=0;
   const gh_player_frame_status status=gh_game_player_directional_dodge_00124154(&b->player->view,&pc);
   if(status)return fail(s,pc?pc:0x124154,status);
   s->input_result=1;
   return GH_PLAYER_FRAME_OK;
  }
  put(p+0x15f4,flags|0x500u);
 }
 if(animation&0x20u){
  put(p+0x15f4,w(p+0x15f4)|0x400u);
  if(flags&0x20u){
   if(!b->auxiliary_bindings_known||b->aux_698||b->aux_6a0||w(p+0x698)||w(p+0x6a0))
    return fail(s,0x1242f8,GH_PLAYER_FRAME_UNSUPPORTED);
   if(!b->idle_input_bindings||!b->idle_input_bindings->targets.player||
      b->idle_input_bindings->targets.player->bytes!=p)
    return fail(s,0x12a610,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_target_bindings *targets=&b->idle_input_bindings->targets;
   gh_player_target found={0};
   uint32_t pc=0;
   gh_player_target_status status=gh_player_pickup_target_0012a610(targets,&found,&pc);
   if(status)return fail(s,pc?pc:0x12a610,GH_PLAYER_FRAME_UNSUPPORTED);
   if(found.handle){
    const gh_player_frame_status pickup=gh_game_player_pickup_prompt_002dbd40(b->idle_input_bindings,&found,&pc);
    if(pickup)return fail(s,pc?pc:0x2dbd40,pickup);
   }
   put(p+0x69c,0);
   status=gh_player_interact_target_0012b328(targets,&found,&pc);
   if(status)return fail(s,pc?pc:0x12b328,GH_PLAYER_FRAME_UNSUPPORTED);
   gh_player_frame_status prompt=gh_game_player_interact_prompt_002dc108(b->idle_input_bindings,&found,&pc);
   if(prompt)return fail(s,pc?pc:0x2dc108,prompt);
   put(p+0x6a4,0);
   if(half(work+0x5b0)==0x608)return fail(s,0x2c2568,GH_PLAYER_FRAME_UNSUPPORTED);
  }
  if(p[0x61f]){
   /* Original 124378 dispatches movement directly after the hurt
    * animation interval opens. The held direction uses current stick bits. */
   const float angle=number(w(p+0x608)),magnitude=number(w(p+0x60c));
   if(!isfinite(angle)||!isfinite(magnitude))
    return fail(s,0x1243ac,GH_PLAYER_FRAME_NUMERIC);
   const unsigned next=angle<number(0x3f9c61aa)?(magnitude>=10000.0f?2u:1u):
       angle>2.0943952f?3u:0u;
   if(p[0x2f4]||p[0x2f5]!=next){
    uint32_t pc=0;
    const gh_player_frame_status status=gh_game_player_input_cleanup_0012bfb8(b,&pc);
    if(status)return fail(s,pc?pc:0x12bfb8,status);
    p[0x2f4]=0;
    p[0x2f5]=(uint8_t)next;
    p[0x2f6]=0;
    p[0x2f7]=(uint8_t)(next==2&&(w(p+0x15f4)&0x4000u));
   }
  }
 }
 return GH_PLAYER_FRAME_OK;
}


/* 11A8F8 selects one of three authored short reactions, then plays CORE CE. */
static gh_player_frame_status hurt_small_0011a8f8(gh_game_player_frame *s)
{
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes;
 uint32_t pc=0;
 enum { INIT=1, VOICE, VIBRATE, CAMERA, CAMERA_PENDING, RELEASE, STEP,
        ENDED_INPUT, ROTATE, TRANSLATE, INPUT };
 if(!s->hurt_phase){
  if(!p[0x2f6]){
   static const unsigned rows[]={0x140,0x4dc,0x4e4};
   if(!b->model||!b->resource||b->resource_bytes<0x4ec||!b->initialize_motion)
    return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
   uint32_t draw;
   if(gh_game_mt_next_0030f348(&b->init->random,&draw))
    return fail(s,0x31cfe0,GH_PLAYER_FRAME_UNSUPPORTED);
   const unsigned row=rows[draw%3u];
   s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,
    .work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,
    .motion_offset=w(b->resource+row),.sequence_candidate_offset=w(b->resource+row+4),
    .blend_frames=1,.release_previous_sequence=1,.rate=b->rate};
   s->hurt_phase=INIT;
  }else s->hurt_phase=p[0x2f6]==1?STEP:INPUT;
 }
 if(s->hurt_phase==INIT){
  gh_player_init_status status=b->initialize_motion(b->motion_context,&s->pending_motion,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  s->hurt_phase=VOICE;
 }
 if(s->hurt_phase==VOICE){
  gh_player_frame_status status=hurt_sound(s,0xce);
  if(status)return status;
  s->hurt_phase=VIBRATE;
 }
 if(s->hurt_phase==VIBRATE){
  gh_player_frame_status status=hurt_vibration_0012c348(s,0);
  if(status)return status;
  s->hurt_phase=CAMERA;
 }
 if(s->hurt_phase==CAMERA){
  if(w(p+0x15f4)&2u){
   if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
   put(p+0x15f4,w(p+0x15f4)&~2u);
   s->hurt_phase=CAMERA_PENDING;
  }else s->hurt_phase=RELEASE;
 }
 if(s->hurt_phase==CAMERA_PENDING){
  gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
  if(status)return fail(s,pc?pc:0x124f60,status);
  s->hurt_phase=RELEASE;
 }
 if(s->hurt_phase==RELEASE){
  if(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698))
   return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
  if(b->aux_6a0||w(p+0x6a0))return fail(s,0x12b928,GH_PLAYER_FRAME_UNSUPPORTED);
  ++p[0x2f6];
  s->hurt_phase=STEP;
 }
 if(s->hurt_phase==STEP){
  if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  int ended;
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
    .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->hurt_phase=ended?ENDED_INPUT:ROTATE;
 }
 if(s->hurt_phase==ENDED_INPUT){
  gh_player_frame_status status=hurt_input_00123938(s);
  if(status||s->input_result)return status;
  memset(p+0x2f4,0,4);
  s->input_started=0;
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=INPUT;
 }
 if(number(w(p+0x54c))<=1)sh(p+0x3ac,half(p+0x3ac)|0x40u);
 return hurt_input_00123938(s);
}

static gh_player_frame_status hurt_reaction_0011aaa0(gh_game_player_frame *s)
{
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes;
 uint32_t pc=0;
 if(!s->hurt_phase){
  if(p[0x2f6]>1||p[0x2f7])return fail(s,0x11aaa0,GH_PLAYER_FRAME_UNSUPPORTED);
  if(p[0x2f6])s->hurt_phase=7;
  else{
   static const unsigned rows[]={0x148,0x150,0x4ec,0x4f4};
   if(!b->model||!b->resource||b->resource_bytes<0x4fc||!b->initialize_motion)
    return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
   uint32_t draw;
   if(gh_game_mt_next_0030f348(&b->init->random,&draw))
    return fail(s,0x31cfe0,GH_PLAYER_FRAME_UNSUPPORTED);
   const unsigned row=rows[draw&3u];
   s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,
    .work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,
    .motion_offset=w(b->resource+row),.sequence_candidate_offset=w(b->resource+row+4),
    .blend_frames=1,.release_previous_sequence=1,.rate=b->rate};
   s->hurt_phase=1;
  }
 }
 if(s->hurt_phase==1){
  gh_player_init_status status=b->initialize_motion(b->motion_context,&s->pending_motion,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  s->hurt_phase=2;
 }
 if(s->hurt_phase==2){
  uint32_t draw;
  if(gh_game_mt_next_0030f348(&b->init->random,&draw))
   return fail(s,0x31cfe0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_sound_program=(draw&1u)?0xcf:0xd0;
  s->hurt_phase=3;
 }
 if(s->hurt_phase==3){
  gh_player_frame_status status=hurt_sound(s,s->hurt_sound_program);
  if(status)return status;
  s->hurt_phase=4;
 }
 if(s->hurt_phase==4){
  gh_player_frame_status status=hurt_vibration_0012c348(s,6);
  if(status)return status;
  s->hurt_phase=5;
 }
 if(s->hurt_phase==5){
  if(w(p+0x15f4)&2u)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
  if(b->aux_698||w(p+0x698))return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
  p[0x2f6]=1;
  s->hurt_phase=7;
 }
 if(s->hurt_phase==7){
  if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  int ended;
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
    .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  if(ended)memset(p+0x2f4,0,4);
  s->hurt_phase=8;
 }
 if(s->hurt_phase==8){
  if(gh_player_root_rotation_0014f9f0(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=9;
 }
 if(s->hurt_phase==9){
  if(gh_player_root_translation_0014f928(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  if(number(w(p+0x54c))<=1)sh(p+0x3ac,half(p+0x3ac)|0x40u);
  s->hurt_phase=10;
 }
 return hurt_input_00123938(s);
}

/* Original PL00 01/02 reaction after the boss's zero-power deflection. */
static gh_player_frame_status hurt_deflect_0011ac80(gh_game_player_frame *s)
{
 enum { SOUND=1, MOTION, CAMERA, VIBRATE, RELEASE, SAMPLE, ROTATE, TRANSLATE, INPUT };
 const gh_player_frame_bindings *b=&s->b;
 gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 if(!s->hurt_phase){
  if(b->work->room!=0x103||p[0x2f7]||p[0x2f6]>1)
   return fail(s,0x11ac80,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=p[0x2f6]?SAMPLE:SOUND;
 }
 if(s->hurt_phase==SOUND){
  gh_player_frame_status status=hurt_sound(s,0xcf);
  if(status)return status;
  put(p+0x54c,0x41200000u);
  s->hurt_phase=MOTION;
 }
 if(s->hurt_phase==MOTION){
  if(!b->initialize_motion||!b->model||!b->resource||b->resource_bytes<0x880||!b->rate)
   return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate,
   .motion_offset=w(b->resource+0x878),.sequence_candidate_offset=w(b->resource+0x87c),
   .blend_frames=1,.release_previous_sequence=1};
  gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
    GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  s->hurt_phase=CAMERA;
 }
 if(s->hurt_phase==CAMERA){
  if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
  if(status)return fail(s,pc?pc:0x124f60,status);
  put(p+0x15f4,w(p+0x15f4)&~2u);
  s->hurt_phase=VIBRATE;
 }
 if(s->hurt_phase==VIBRATE){
  gh_player_frame_status status=hurt_vibration_0012c348(s,0);
  if(status)return status;
  s->hurt_phase=RELEASE;
 }
 if(s->hurt_phase==RELEASE){
  gh_game_mt random=b->init->random;
  uint32_t draw;
  if(gh_game_mt_next_0030f348(&random,&draw))
   return fail(s,0x31cfe0,GH_PLAYER_FRAME_UNSUPPORTED);
  if(draw&1u){
   if(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698))
    return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
   if(b->aux_6a0||w(p+0x6a0))return fail(s,0x12b928,GH_PLAYER_FRAME_UNSUPPORTED);
  }
  b->init->random=random;
  p[0x2f6]=1;
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  if(s->motion_ended)memset(p+0x2f4,0,4);
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=INPUT;
 }
 return hurt_input_00123938(s);
}

static gh_player_frame_status hurt_fall_0011add8(gh_game_player_frame *s)
{
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes;
 const uint8_t *work=(const uint8_t *)b->work;
 uint32_t pc=0;
 if(!s->hurt_phase){
  put(p+0x54c,0x41700000);
  const int projectile_fall=(b->work->room==0x100||b->work->room==0x104)&&p[0x2f7]==1;
  const int charge_fall=b->work->room==0x104&&p[0x2f7]==2;
  const int sweep_fall=b->work->room==0x104&&p[0x2f7]==5;
  if(p[0x2f6]>1||(p[0x2f7]&&!projectile_fall&&!charge_fall&&!sweep_fall))
   return fail(s,0x11add8,GH_PLAYER_FRAME_UNSUPPORTED);
  if(p[0x2f6])s->hurt_phase=8;
  else{
   uint32_t draw;
   static const uint16_t rows[]={0x158,0x4fc,0x164,0x504,0x50c};
   if(!b->model||!b->resource||b->resource_bytes<0x514||!b->initialize_motion)
    return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
   /* Original 41C5B0 variants 1, 2 and 5 select fixed PL00 clips. */
   unsigned row=0x158;
   if(charge_fall)row=0x4fc;
   if(sweep_fall){
    uint32_t target;
    if(!elf_word(b,0x41c5c4,&target)||target!=0x11af2cu)
     return fail(s,0x11af2c,GH_PLAYER_FRAME_UNSUPPORTED);
    row=0x504;
   }
   if(!p[0x2f7]){
    if(gh_game_mt_next_0030f348(&b->init->random,&draw))
     return fail(s,0x31cfe0,GH_PLAYER_FRAME_UNSUPPORTED);
    row=rows[draw%5u];
   }
   s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,
    .work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,
    .motion_offset=w(b->resource+row),.sequence_candidate_offset=w(b->resource+row+4),
    .blend_frames=2,.release_previous_sequence=1,.rate=b->rate};
   s->hurt_phase=1;
  }
 }
 if(s->hurt_phase==1){
  gh_player_frame_status status=hurt_sound(s,0xd7);
  if(status)return status;
  s->hurt_phase=2;
 }
 if(s->hurt_phase==2){
  gh_player_frame_status status=hurt_sound(s,0x123);
  if(status)return status;
  s->hurt_phase=3;
 }
 if(s->hurt_phase==3){
  if(w(p+0x15f4)&2u){
   if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
   put(p+0x15f4,w(p+0x15f4)&~2u);
   s->hurt_phase=4;
  }else s->hurt_phase=5;
 }
 if(s->hurt_phase==4){
  gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
  if(status)return fail(s,pc?pc:0x124f60,status);
  s->hurt_phase=5;
 }
 if(s->hurt_phase==5){
  gh_player_frame_status status=hurt_vibration_0012c348(s,10);
  if(status)return status;
  s->hurt_phase=6;
 }
 if(s->hurt_phase==6){
  if(!b->take_binding_known)return fail(s,0x11af9c,GH_PLAYER_FRAME_UNSUPPORTED);
  if(b->take_object){
   gh_title_player_view *actor=NULL;
   if(gh_hitarea_object_actor(b->take_object,&actor)||!actor||actor->bytes!=p||
      b->take_object->size<0x20)return fail(s,0x11af9c,GH_PLAYER_FRAME_ARGUMENT);
   const uint8_t *hit=b->take_object->bytes;
   float angle;
   if(gh_game_angle_001f8180(number(w(hit+0x10)),number(w(hit+0x18)),&angle,b->numeric_policy,&pc))
    return fail(s,pc?pc:0x1f8180,GH_PLAYER_FRAME_UNSUPPORTED);
   volatile float turn=angle+number(0x40490fdb);
   if(gh_id_angle_reference_001f7d48(turn,&angle))
    return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);
   sf(p+0x104,angle);
  }
  if(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698))
   return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=7;
 }
 if(s->hurt_phase==7){
  gh_player_init_status status=b->initialize_motion(b->motion_context,&s->pending_motion,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  put(p+0x15b0,0);put(p+0x15b4,0);p[0x2f6]=1;
  s->hurt_phase=8;
 }
 if(s->hurt_phase==8){
  uint32_t buffered=w(p+0x15b4);
  if(buffered)--buffered;
  if(w(work+8)&0xf0u)buffered=10;
  put(p+0x15b4,buffered);
  s->hurt_phase=9;
 }
 if(s->hurt_phase==9){
  if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
    .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  if(s->motion_ended){
   p[0x2f4]=1;p[0x2f5]=4;p[0x2f6]=0;p[0x2f7]=(uint8_t)((int32_t)w(p+0x15b4)>0);
   s->hurt_phase=13;
  }else s->hurt_phase=10;
 }
 if(s->hurt_phase==10){
  if(gh_player_root_rotation_0014f9f0(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=11;
 }
 if(s->hurt_phase==11){
  if(gh_player_root_translation_0014f928(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=12;
 }
 if(s->hurt_phase==12){
  if(half(p+0x3ac)&0x20u){
   put(p+0x15b0,1);
   if(p[0x61e]||(w(work+8)&0xf0u)){
    put(p+0x54c,0x40a00000);
    p[0x2f4]=0;p[0x2f5]=6;p[0x2f6]=0;p[0x2f7]=1;
   }
  }else if(w(p+0x15b0)){
   const int32_t residual=(int32_t)w(p+0x160c);
   const int arena_residual=b->work->room==0x103&&
     ((p[0x2f7]<=1&&(residual==10||residual==11))||(!p[0x2f7]&&(residual==3||residual==5)));
   const int grenade_residual=b->work->room==0x101&&!p[0x2f7]&&residual==12;
   const int combo_residual=(b->work->room==0x100||b->work->room==0x104)&&residual==10;
   const int r104_anger_residual=b->work->room==0x104&&!p[0x2f7]&&residual==22;
   if(residual!=0&&residual!=15&&residual!=7&&!arena_residual&&!grenade_residual&&!r104_anger_residual&&
      !combo_residual&&!(b->work->room==0x104&&p[0x2f7]==5&&residual>0)&&
      !(b->work->room==0x100&&!p[0x2f7]&&residual==5)&&!(b->work->room==0x102&&residual==3))
    return fail(s,0x12c0f8,GH_PLAYER_FRAME_UNSUPPORTED);
   gh_player_frame_status status=damage_hp_0012c0f8(s,residual);
   if(status)return status;
   if((int16_t)half(p+0x54a)<1)sh(p+0x54a,1);
   put(p+0x15b0,0);put(p+0x160c,0);
  }
  s->hurt_phase=13;
 }
 return GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status hurt_stagger_0011b5f8(gh_game_player_frame *s)
{
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes;
 uint32_t pc=0;
 const uint32_t buttons=w((const uint8_t *)b->work+8);
 if(!s->hurt_phase){
  const unsigned phase=p[0x2f6];
  if(phase>3)return GH_PLAYER_FRAME_OK;
  if(phase==1||phase==3)s->hurt_phase=9;
  else{
   if(!b->initialize_motion||!b->model||!b->resource||!b->rate||
      b->resource_bytes<(phase==2?0x4a4u:0x158u))
    return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
   unsigned row=0x49c;
   if(!phase){
    uint32_t draw;
    if(gh_game_mt_next_0030f348(&b->init->random,&draw))
     return fail(s,0x31cfe0,GH_PLAYER_FRAME_UNSUPPORTED);
    row=(draw&1)?0x148:0x150;
    s->hurt_sound_program=(draw&1)?0xcf:0xd0;
   }
   s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,
    .work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,
    .motion_offset=w(b->resource+row),.sequence_candidate_offset=w(b->resource+row+4),
    .blend_frames=phase==2?10:1,.rate=b->rate};
   s->hurt_phase=phase==2?8:1;
  }
 }
 if(s->hurt_phase==1){
  gh_player_frame_status status=hurt_sound(s,(int16_t)s->hurt_sound_program);
  if(status)return status;
  s->hurt_phase=2;
 }
 if(s->hurt_phase==2){
  if(w(p+0x15f4)&2u){
   if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
   put(p+0x15f4,w(p+0x15f4)&~2u);
   s->hurt_phase=3;
  }else s->hurt_phase=4;
 }
 if(s->hurt_phase==3){
  gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
  if(status)return fail(s,pc?pc:0x124f60,status);
  s->hurt_phase=4;
 }
 if(s->hurt_phase==4||s->hurt_phase==8){
  gh_player_init_status status=b->initialize_motion(b->motion_context,&s->pending_motion,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
    GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  if(s->hurt_phase==8){
   p[0x2f6]=3;
   s->hurt_phase=9;
  }else s->hurt_phase=5;
 }
 if(s->hurt_phase==5){
  gh_player_frame_status status=hurt_sound(s,0x8c);
  if(status)return status;
  s->hurt_phase=6;
 }
 if(s->hurt_phase==6){
  gh_player_frame_status status=hurt_vibration_0012c348(s,4);
  if(status)return status;
  s->hurt_phase=7;
 }
 if(s->hurt_phase==7){
  if(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698))
   return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
  if(b->aux_6a0||w(p+0x6a0))return fail(s,0x12b928,GH_PLAYER_FRAME_UNSUPPORTED);
  sh(p+0x568,15);
  put(p+0x15b0,150);
  p[0x2f6]=1;
  s->hurt_phase=9;
 }
 if(s->hurt_phase==9){
  if(!b->idle_input_bindings||!b->idle_input_bindings->action_buttons)
   return fail(s,0x2db770,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_action_set_request prompt={.priority=11,.life=19,.unknown20=16};
  unsigned index;
  int emitted;
  if(gh_action_button_set_001f7858(b->idle_input_bindings->action_buttons,&prompt,&emitted,&index,&pc))
   return fail(s,pc?pc:0x1f7858,GH_PLAYER_FRAME_CALLEE);
  if(p[0x2f6]==1){
   if(buttons&0xf0u)put(p+0x15b0,w(p+0x15b0)-5);
   if(buttons&0xf00000u)put(p+0x15b0,w(p+0x15b0)-20);
  }
  s->hurt_phase=10;
 }
 if(s->hurt_phase==10){
  if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
    .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  if(p[0x2f6]==1){
   const uint16_t timer=half(p+0x568);
   if(s->motion_ended||!timer)p[0x2f6]=2;
   if(timer)sh(p+0x568,(uint16_t)(timer-1));
  }else{
   const uint32_t countdown=w(p+0x15b0)-1-((buttons&0xf0u)?5u:0u)-((buttons&0xf00000u)?20u:0u);
   put(p+0x15b0,countdown);
   if((int32_t)countdown<1)put(p+0x2f4,0);
  }
  s->hurt_phase=11;
 }
 if(s->hurt_phase==11){
  if(gh_player_root_rotation_0014f9f0(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=12;
 }
 if(s->hurt_phase==12){
  if(gh_player_root_translation_0014f928(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=13;
 }
 return GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status hurt_rise_0011b280(gh_game_player_frame *s)
{
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes;
 uint32_t pc=0;
 if(!s->hurt_phase){
  put(p+0x54c,0x41700000);
  if(p[0x2f6]>1)return GH_PLAYER_FRAME_OK;
  if(p[0x2f6])s->hurt_phase=2;
  else{
   if(p[0x2f7]||(w((const uint8_t *)b->work)&0xf0u))
    return fail(s,0x10a408,GH_PLAYER_FRAME_UNSUPPORTED);
   if(!b->model||!b->resource||b->resource_bytes<0x17c||!b->initialize_motion)
    return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
   s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,
    .work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,
    .motion_offset=w(b->resource+0x174),.sequence_candidate_offset=w(b->resource+0x178),
    .blend_frames=10,.release_previous_sequence=1,.rate=b->rate};
   s->hurt_phase=1;
  }
 }
 if(s->hurt_phase==1){
  gh_player_init_status status=b->initialize_motion(b->motion_context,&s->pending_motion,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  p[0x2f6]=1;
  s->hurt_phase=2;
 }
 if(s->hurt_phase==2){
  if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
    .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  if(s->motion_ended)memset(p+0x2f4,0,4);
  s->hurt_phase=3;
 }
 if(s->hurt_phase==3){
  if(gh_player_root_rotation_0014f9f0(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=4;
 }
 if(s->hurt_phase==4){
  if(gh_player_root_translation_0014f928(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=5;
 }
 if(s->hurt_phase==5){
  gh_player_frame_status status=hurt_input_00123938(s);
  if(status)return status;
  s->hurt_phase=6;
 }
 if(s->input_result)return GH_PLAYER_FRAME_OK;
 const gh_hitarea_status hit=gh_player_attack_hitarea_0010a438(&b->player->view,b->give_objects,&pc);
 return hit?fail(s,pc?pc:0x10a438,hit==GH_HITAREA_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE):GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status damage_frame(gh_game_player_frame*s){const gh_player_frame_bindings*b=&s->b;uint8_t*p=b->player->view.bytes;uint32_t pc=0;
 if(!s->damage_stage){if(w((const uint8_t*)b->work+0x58c)&8){s->damage_stage=7;return GH_PLAYER_FRAME_OK;}float timer=number(w(p+0x15d0)),cooldown=number(w(p+0x54c));if(!isfinite(timer)||!isfinite(cooldown))return fail(s,0x12cae0,GH_PLAYER_FRAME_NUMERIC);s->damage_stage=(uint8_t)(timer<=0&&cooldown<=0?1:3);}
 if(s->damage_stage==1){if(!b->scene_at)return fail(s,0x2c2490,GH_PLAYER_FRAME_UNSUPPORTED);if(!b->player->view.alias_f0)return fail(s,0x2db6b0,GH_PLAYER_FRAME_ARGUMENT);float point[3];memcpy(point,b->player->view.alias_f0,12);uint32_t values[3];int hit;gh_scene_at_status e=gh_scene_at_damage_region_002c2490(b->scene_at,point,values,&hit,&pc);if(e)return fail(s,pc?pc:0x2c2490,e==GH_AT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);if(hit){memcpy(p+0x15e0,values,12);put(p+0x15d0,0x42b40000);s->damage_region_hit=1;s->pending_effect=(gh_player_frame_effect_request){&b->player->view,0x4b,0,0,UINT32_MAX,-1,0};s->damage_stage=2;}else s->damage_stage=3;}
 if(s->damage_stage==2){if(((const uint8_t*)b->work)[0x5b1]==5){if(!b->effect_create)return fail(s,0x306528,GH_PLAYER_FRAME_UNSUPPORTED);gh_player_frame_status e=b->effect_create(b->effect_context,&s->pending_effect,&pc);if(e)return fail(s,pc?pc:0x306528,e);}int16_t hp;uint16_t raw=(uint16_t)half(p+0x54a);memcpy(&hp,&raw,2);if(hp<11)sh(p+0x54a,0);else{raw=(uint16_t)(raw-25u);memcpy(&hp,&raw,2);sh(p+0x54a,hp<1?1:raw);}p[0x2f4]=1;p[0x2f5]=7;p[0x2f7]=p[0x2f6]=0;s->damage_stage=7;return GH_PLAYER_FRAME_OK;}
 if(s->damage_stage==3){if(!b->take_binding_known)return fail(s,0x109928,GH_PLAYER_FRAME_UNSUPPORTED);if(!b->take_object){s->damage_stage=7;return GH_PLAYER_FRAME_OK;}gh_title_player_view*actor=NULL;if(gh_hitarea_object_actor(b->take_object,&actor)||!actor||actor->bytes!=p||b->take_object->size<0x64)return fail(s,0x109928,GH_PLAYER_FRAME_ARGUMENT);const uint8_t*t=b->take_object->bytes;float cooldown=number(w(p+0x54c));if(!(w(t+0x60)&1)||cooldown>0){s->damage_stage=7;return GH_PLAYER_FRAME_OK;}uint32_t raw=w(t+0x4c);int32_t amount;memcpy(&amount,&raw,4);sh(p+0x1616,0);put(p+0x160c,0);unsigned kind=half(t+0x46);switch(kind){case 0x3e:case 0x3f:case 0x41:case 0x42:case 0x43:case 0x44:case 0x45:case 0x46:case 0x48:case 0x52:case 0x55:{int32_t part=amount/4;put(p+0x160c,(uint32_t)part);raw=(uint32_t)amount-(uint32_t)part;memcpy(&amount,&raw,4);break;}case 0x54:amount=100;break;default:break;}s->pending_damage=amount;s->pending_damage_kind=(uint16_t)kind;s->damage_stage=4;}
 if(s->damage_stage==4){
  const gh_title_player_view *source=NULL;
  if(gh_hitarea_object_last_hit_actor(b->take_object,&source)||!source||!source->bytes)
   return fail(s,0x12c0f8,GH_PLAYER_FRAME_UNSUPPORTED);
  const uint8_t *take=b->take_object->bytes;
  const int projectile=source->size==0x630&&
    source->original_vtable214==0x4290c8&&w(source->bytes+0x214)==0x4290c8&&
    half(source->bytes+0x2fe)==0x3be;
  const int falling_plank=b->work->room==0x100&&source->size==0xda0&&
    source->original_vtable214==0x426e78&&w(source->bytes+0x214)==0x426e78&&
    half(source->bytes+0x2fe)==0x346&&source->bytes[0x2f4]==2&&
    source->bytes[0x2f5]>=1&&source->bytes[0x2f5]<=3;
  const int grenade=b->work->room==0x101&&source->size==0x680&&
    source->original_vtable214==0x4282f0&&w(source->bytes+0x214)==0x4282f0&&
    half(source->bytes+0x2fe)==0x364&&source->bytes[0x2f4]==3;
  if(grenade){
   /* 1B1070 publishes Give48 50. 109928 applies 38 and queues the other 12. */
   if(s->pending_damage_kind!=0x48||s->pending_damage!=38||w(p+0x160c)!=12||
      w(take+0x4c)!=50||w(take+0x40)!=UINT32_MAX||half(take+0x44))
    return fail(s,0x12c0f8,GH_PLAYER_FRAME_UNSUPPORTED);
  }else if(falling_plank){
   /* 1A0E98 publishes the original blunt Give53 with power 10. */
   if(s->pending_damage_kind!=0x53||s->pending_damage!=10||w(p+0x160c)||
      w(take+0x4c)!=10||w(take+0x40)!=UINT32_MAX||half(take+0x44))
    return fail(s,0x12c0f8,GH_PLAYER_FRAME_UNSUPPORTED);
  }else if(projectile){
   const uint32_t launch_type=w(source->bytes+0x610);
   const int32_t power=b->work->room==0x100&&launch_type==3?30:
       b->work->room==0x104&&launch_type==0?60:0;
   const int32_t residual=power/4;
   /* 109928 derives the immediate damage and later falls from this Give. */
   if(!power||s->pending_damage_kind!=0x41||s->pending_damage!=power-residual||
      w(p+0x160c)!=(uint32_t)residual||w(take+0x4c)!=(uint32_t)power||
      w(take+0x40)!=UINT32_MAX||half(take+0x44))
    return fail(s,0x12c0f8,GH_PLAYER_FRAME_UNSUPPORTED);
  }else{
   if(source->size<0x642||source->original_vtable214!=0x4479c8||
      w(source->bytes+0x214)!=0x4479c8)
    return fail(s,0x12c0f8,GH_PLAYER_FRAME_UNSUPPORTED);
   const unsigned source_kind=half(source->bytes+0x2fe);
   const uint32_t dependency=w(source->bytes+0x564);
   const unsigned stamp=half(source->bytes+0x640);
   const uint32_t override_bank=w(take+0x40);
   /* Original R104 EM03 close strikes share the source bank and cue. */
   const uint32_t residual=w(p+0x160c);
   const int r104_em03=b->work->room==0x104&&source_kind==0x203&&dependency==0x250&&!stamp&&
     source->size>0x1651&&w(source->bytes+0x1644)==0x80000u&&!source->bytes[0x1650]&&
     !source->bytes[0x1651]&&!source->bytes[0x2f7];
   const int r104_close=r104_em03&&source->bytes[0x2f6]==1&&override_bank==1&&half(take+0x44)==0x28;
   const unsigned source_move=half(source->bytes+0x2f4);
   /* 21FD50 pairs the light hit with the later fall. 21A538 can emit the
    * stronger fall directly from the low-rank 19D0 sequence. */
   const int r104_combo=r104_close&&source_move==0x1600&&
     ((s->pending_damage_kind==0x3d&&w(take+0x4c)==30&&s->pending_damage==30&&!residual)||
      (s->pending_damage_kind==0x3e&&w(take+0x4c)==40&&s->pending_damage==30&&residual==10));
   /* The published Give keeps its effective power after the anger timer changes. */
   const int r104_amplified_melee=source->size>=0x1882&&
     !w(source->bytes+0x1870)&&w(source->bytes+0x1874)==60&&
     w(source->bytes+0x1878)==0x3e&&w(source->bytes+0x187c)==1&&
     half(source->bytes+0x1880)==0x28&&w(take+0x4c)==90&&
     s->pending_damage==68&&residual==22;
   const int r104_melee=r104_close&&source_move==0x1500&&s->pending_damage_kind==0x3e&&
     ((w(take+0x4c)==60&&s->pending_damage==45&&residual==15)||r104_amplified_melee);
   /* 224090 sets the charge shape and both Give rows at 2609A8. */
   const int r104_charge=r104_em03&&source_move==0x1a00&&source->bytes[0x2f6]>=3&&
     source->bytes[0x2f6]<=5&&source->size>=0x1882&&w(source->bytes+0x1870)==10&&
     w(source->bytes+0x1874)==40&&w(source->bytes+0x1878)==0x42&&!w(source->bytes+0x187c)&&
     half(source->bytes+0x1880)==0xf5&&!override_bank&&half(take+0x44)==0xf5&&
     s->pending_damage_kind==0x42&&w(take+0x4c)==40&&s->pending_damage==30&&residual==10;
   /* 21A538's 924 motion writes the original unsplit Give3C. */
   const int r104_punch=r104_close&&source_move==0x1500&&source->size>=0x1882&&
     !w(source->bytes+0x1870)&&w(source->bytes+0x1874)==60&&w(source->bytes+0x1878)==0x3c&&
     w(source->bytes+0x187c)==1&&half(source->bytes+0x1880)==0x28&&
     s->pending_damage_kind==0x3c&&w(take+0x4c)==60&&s->pending_damage==60&&!residual;
   /* 232500 publishes the R104 EM03 sweep's shape 1B, Take45 and default sound. */
   const int r104_sweep=r104_em03&&source_move==0x4500&&source->bytes[0x2f6]==1&&
     source->size>=0x1882&&w(source->bytes+0x1870)==0x1b&&w(source->bytes+0x1874)==50&&
     w(source->bytes+0x1878)==0x45&&w(source->bytes+0x187c)==UINT32_MAX&&
     !half(source->bytes+0x1880)&&override_bank==UINT32_MAX&&!half(take+0x44)&&
     s->pending_damage_kind==0x45&&s->pending_damage>0;
   /* 226450 authors the zero-power brace contact with Take4B. */
   const int r104_brace=r104_em03&&source_move==0x2100&&source->bytes[0x2f6]==1&&
     source->size>=0x1882&&w(source->bytes+0x1870)==0x10&&!w(source->bytes+0x1874)&&
     w(source->bytes+0x1878)==0x4b&&w(source->bytes+0x187c)==UINT32_MAX&&
     !half(source->bytes+0x1880)&&override_bank==UINT32_MAX&&!half(take+0x44)&&
     s->pending_damage_kind==0x4b&&!w(take+0x4c)&&!s->pending_damage&&!residual;
   const int r104_strike=r104_combo||r104_melee||r104_charge||r104_punch||r104_sweep||r104_brace;
   /* The long EM20 Give's original kind3E splits power 40 into 30 and 10. */
   const int devil_melee=b->work->room==0x100&&source_kind==0x220&&dependency==0x220&&stamp==0xff&&
     source->size>0x1651&&w(source->bytes+0x1644)==0x400000u&&source->bytes[0x1651]==0xff&&
     half(source->bytes+0x2f4)==0x1500&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     override_bank==UINT32_MAX&&!half(take+0x44)&&s->pending_damage_kind==0x3e&&
     w(take+0x4c)==40&&s->pending_damage==30&&w(p+0x160c)==10;
   /* EM20's selector 16 uses the two Give rows recovered at 221C4C. */
   const unsigned combo_phase=source->bytes[0x2f6];
   const int devil_combo=b->work->room==0x100&&source_kind==0x220&&dependency==0x220&&
     stamp==0xff&&source->size>0x1651&&w(source->bytes+0x1644)==0x400000u&&
     source->bytes[0x1651]==0xff&&half(source->bytes+0x2f4)==0x1600&&
     override_bank==UINT32_MAX&&!half(take+0x44)&&
     (((combo_phase==3||combo_phase==5)&&s->pending_damage_kind==0x3d&&
       w(take+0x4c)==5&&s->pending_damage==5&&!w(p+0x160c))||
      (combo_phase==7&&s->pending_damage_kind==0x3e&&
       w(take+0x4c)==10&&s->pending_damage==8&&w(p+0x160c)==2));
   const int arena_actor=b->work->room==0x103&&source_kind==0x204&&
     source->size>0x1650&&source->bytes[0x1650]==24;
   const int arena_selector15=arena_actor&&half(source->bytes+0x2f4)==0x1500&&
     source->bytes[0x2f6]==1;
   const int arena_attack=arena_selector15&&override_bank==UINT32_MAX&&!half(take+0x44);
   const int arena_short=s->pending_damage_kind==0x37&&s->pending_damage==20&&!w(p+0x160c);
   /* Original EM03 selector15 uses power30 far and power60 close. */
   const int arena_fall=s->pending_damage_kind==0x3e&&
     ((w(take+0x4c)==30&&s->pending_damage==23&&w(p+0x160c)==7)||
      (w(take+0x4c)==60&&s->pending_damage==45&&w(p+0x160c)==15));
   const int arena_em00=dependency==0x200&&(stamp==2||stamp==3)&&arena_short;
   const int arena_em03=dependency==0x203&&(stamp==6||stamp==7)&&(arena_short||arena_fall);
   const int arena_slide=b->work->room==0x103&&source_kind==0x204&&dependency==0x20a&&
     (stamp==4||stamp==5)&&source->size>0x1650&&source->bytes[0x1650]==24&&
     half(source->bytes+0x2f4)==0x1900&&source->bytes[0x2f6]==3&&!source->bytes[0x2f7]&&
     s->pending_damage_kind==0x41&&w(take+0x4c)==40&&s->pending_damage==30&&w(p+0x160c)==10&&
     !override_bank&&half(take+0x44)==0xf5;
   const int arena_em0a=dependency==0x20a&&(stamp==4||stamp==5)&&!source->bytes[0x2f7];
   /* 21D0A0 selects distinct close and long variants of this original short Take. */
   const int arena_em0a_short=arena_em0a&&(arena_short||
     (s->pending_damage_kind==0x37&&w(take+0x4c)==15&&s->pending_damage==15&&!w(p+0x160c)));
   const int arena_em0a_fall=arena_em0a&&s->pending_damage_kind==0x3e&&w(take+0x4c)==15&&
     s->pending_damage==12&&w(p+0x160c)==3;
   const int arena_em0a_f6=arena_selector15&&arena_em0a&&arena_short&&
     !override_bank&&half(take+0x44)==0xf6;
   /* 231AC0's reverse sweep publishes the original fall with no HP damage. */
   const int arena_reverse=arena_actor&&arena_em0a&&half(source->bytes+0x2f4)==0x4400&&
     source->bytes[0x2f6]==1&&s->pending_damage_kind==0x3e&&!w(take+0x4c)&&
     !s->pending_damage&&!w(p+0x160c)&&override_bank==UINT32_MAX&&!half(take+0x44);
   /* Original EM14 selector 16 publishes a light strike and a short finisher. */
   const int boss_melee=b->work->room==0x103&&source_kind==0x204&&dependency==0x214&&
     !stamp&&source->size>=0x1896&&source->bytes[0x1650]==15&&half(source->bytes+0x2f4)==0x1600&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&!w(p+0x160c)&&
     ((s->pending_damage_kind==0x3d&&w(source->bytes+0x1874)==5&&
       override_bank==UINT32_MAX&&!half(take+0x44))||
      (s->pending_damage_kind==0x37&&w(source->bytes+0x1888)==10&&
       !override_bank&&half(take+0x44)==0xf6));
   /* EM15's original 21A538 short Give becomes Take37 after the paired throw. */
   const int em15_short=b->work->room==0x103&&source_kind==0x204&&dependency==0x215&&stamp==1&&
     source->size>0x1650&&source->bytes[0x1650]==15&&half(source->bytes+0x2f4)==0x1500&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&arena_short&&w(take+0x4c)==20&&
     override_bank==UINT32_MAX&&!half(take+0x44);
   /* EM14/15's 23D7D0 deflection passes a zero-power Give49 to Gene. */
   const int boss_deflect=b->work->room==0x103&&source_kind==0x204&&stamp<2&&
     dependency==0x214u+stamp&&source->size>0x1650&&source->bytes[0x1650]==15&&
     half(source->bytes+0x2f4)==0x6f00&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     s->pending_damage_kind==0x49&&!w(take+0x4c)&&!s->pending_damage&&!w(p+0x160c)&&
     override_bank==UINT32_MAX&&!half(take+0x44);
   /* EM14's default 21A654 row transmits the original boosted short Take37. */
   const int em14_short=b->work->room==0x103&&source_kind==0x204&&dependency==0x214&&!stamp&&
     source->size>=0x1896&&source->bytes[0x1650]==15&&half(source->bytes+0x2f4)==0x1500&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&s->pending_damage_kind==0x37&&
     w(source->bytes+0x1870)<=1&&w(source->bytes+0x1874)==20&&w(source->bytes+0x1878)==0x37&&
     w(take+0x4c)==30&&s->pending_damage==30&&!w(p+0x160c)&&
     override_bank==UINT32_MAX&&!half(take+0x44);
   /* 22FAE0's two original paired kick rows both deliver the shared fall Take. */
   const int boss_kick=b->work->room==0x103&&source_kind==0x204&&stamp<2&&
     dependency==0x214u+stamp&&source->size>=0x1896&&source->bytes[0x1650]==15&&
     half(source->bytes+0x2f4)==0x1700&&source->bytes[0x2f6]==1&&source->bytes[0x2f7]==1&&
     (w(source->bytes+0x1870)==2||w(source->bytes+0x1870)==12)&&w(source->bytes+0x1874)==20&&
     s->pending_damage_kind==0x3e&&override_bank==UINT32_MAX&&!half(take+0x44)&&
     ((w(take+0x4c)==20&&s->pending_damage==15&&w(p+0x160c)==5)||
      (w(take+0x4c)==30&&s->pending_damage==23&&w(p+0x160c)==7));
   /* 22D2F0 emits Give3E power30, including the frame that enters phase10.
    * The later flank emits power40. 260C20 can amplify either by half. */
   const unsigned flank_phase=source->bytes[0x2f6];
   const uint32_t flank_take_power=w(take+0x4c), flank_residual=w(p+0x160c);
   const int boss_flank=b->work->room==0x103&&source_kind==0x204&&stamp<2&&
     dependency==0x214u+stamp&&source->size>=0x187c&&source->bytes[0x1650]==15&&
     half(source->bytes+0x2f4)==0x3c00&&!source->bytes[0x2f7]&&
     s->pending_damage_kind==0x3e&&override_bank==UINT32_MAX&&!half(take+0x44)&&
     (((flank_phase==9||flank_phase==10)&&!w(source->bytes+0x1870)&&
       w(source->bytes+0x1874)==30&&w(source->bytes+0x1878)==0x3e&&
       ((flank_take_power==30&&s->pending_damage==23&&flank_residual==7)||
        (flank_take_power==45&&s->pending_damage==34&&flank_residual==11)))||
      (((flank_phase==11&&!w(source->bytes+0x1870)&&w(source->bytes+0x1874)==40&&
         w(source->bytes+0x1878)==0x3e)||(flank_phase>=13&&flank_phase<=16))&&
       ((flank_take_power==40&&s->pending_damage==30&&flank_residual==10)||
        (flank_take_power==60&&s->pending_damage==45&&flank_residual==15&&
         w(source->bytes+0x1874)==40&&w(source->bytes+0x1878)==0x3e))));
   const int arena_hit=(arena_attack&&(arena_em00||arena_em03||arena_em0a_short||arena_em0a_fall))||
     arena_slide||arena_em0a_f6||arena_reverse||boss_melee||em15_short||em14_short||boss_deflect||boss_kick||boss_flank;
   const int em14_sphere=b->work->room==0x103&&source_kind==0x204&&dependency==0x214&&!stamp&&
     source->size>=0x1896&&source->bytes[0x1650]==15&&half(source->bytes+0x2f4)==0x1500&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&s->pending_damage_kind==0x4e&&
     w(source->bytes+0x1870)==0x11&&w(take+0x4c)==30&&s->pending_damage==30&&
     !w(p+0x160c)&&override_bank==UINT32_MAX&&!half(take+0x44);
   /* The timed 21FB34 strike publishes Give3D/15 or Give37/20.
    * 260C20 applies the live scale before Take reads power. */
   const int r200_em1a=dependency==0x21a&&!stamp&&w(source->bytes+0x5f4)==1;
   const int r200_wave=dependency==0x201&&(stamp==6||stamp==7)&&!w(source->bytes+0x5f4)&&
     s->pending_damage_kind==0x37;
   const int r200_short=b->work->room==0x200&&source_kind==0x201&&(r200_em1a||r200_wave)&&
     source->size>=0x1884&&!source->bytes[0x1650]&&w(source->bytes+0x2f4)==0x00011500u&&
     (!w(source->bytes+0x1870)||(r200_wave&&stamp==7&&w(source->bytes+0x1870)==1))&&
     w(source->bytes+0x187c)==UINT32_MAX&&
     !half(source->bytes+0x1880)&&override_bank==UINT32_MAX&&!half(take+0x44)&&!residual&&
     s->pending_damage==(int32_t)w(take+0x4c)&&s->pending_damage_kind==w(source->bytes+0x1878)&&
     ((s->pending_damage_kind==0x3d&&w(source->bytes+0x1874)==15&&s->pending_damage==18)||
      (s->pending_damage_kind==0x37&&w(source->bytes+0x1874)==20&&s->pending_damage==24));
   if(source_kind!=0x202&&!arena_hit&&!em14_sphere&&!devil_melee&&!devil_combo&&!r104_strike&&!r200_short)
    return fail(s,0x12c0f8,GH_PLAYER_FRAME_UNSUPPORTED);
   const int em0a_story=source->size>=0x16c8&&b->work->room==0x102&&stamp==2&&
     source->bytes[0x1650]==5&&!w(source->bytes+0x16c4)&&!source->bytes[0x2f7];
   const int em0a_opening=source->size>0x1650&&b->work->room==0x100&&!stamp&&source->bytes[0x1650]==3;
   const int em0a_override=dependency==0x20a&&(em0a_opening||em0a_story)&&source->bytes[0x2f4]==0&&
     source->bytes[0x2f5]==0x15&&source->bytes[0x2f6]==1&&
     (s->pending_damage_kind==0x37||(em0a_story&&s->pending_damage_kind==0x3c))&&
     s->pending_damage==20&&!override_bank&&half(take+0x44)==0xf6;
   const unsigned cue=half(take+0x44);
   /* Original 21D1C0 face-kick Give is shape0/power20/kind3E with CORE F6. */
   const int west_em0a_face_kick=em0a_opening&&dependency==0x20a&&source->size>=0x1884&&
     half(source->bytes+0x2f4)==0x1500&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     !w(source->bytes+0x1870)&&w(source->bytes+0x1874)==20&&w(source->bytes+0x1878)==0x3e&&
     !override_bank&&cue==0xf6&&w(take+0x4c)==20&&s->pending_damage_kind==0x3e&&
     s->pending_damage==15&&residual==5;
   const int em0a_wide=em0a_story&&dependency==0x20a&&half(source->bytes+0x2f4)==0x4500&&
     source->bytes[0x2f6]==1&&!override_bank&&cue==0xf6&&s->pending_damage_kind==0x42&&
     w(take+0x4c)==20&&s->pending_damage==15&&residual==5;
   const int r101_slash=cue==0x140&&
     ((s->pending_damage_kind==0x3b&&s->pending_damage==60)||
      (s->pending_damage_kind==0x3a&&s->pending_damage==40));
   const int r101_combo=cue==0x111&&
     ((s->pending_damage_kind==0x3b&&s->pending_damage==40)||
      (s->pending_damage_kind==0x3a&&s->pending_damage==20));
   const int r101_weapon=b->work->room==0x101&&dependency==0x203&&stamp==13&&
     source->size>0x1650&&!source->bytes[0x1650]&&!override_bank&&(r101_slash||r101_combo);
   /* 233738 publishes the same two OM363 Take rows for these armed cEm00 actors. */
   const int single_om363=b->work->room==0x100&&dependency==0x202&&stamp==18?9:
     b->work->room==0x102&&dependency==0x200&&stamp==3?1:0;
   const int armed47=single_om363&&source->size>0x1650&&
     source->bytes[0x1650]==single_om363&&half(source->bytes+0x2f4)==0x4700&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&source->original_kind4ac==0x42c7f0&&
     w(source->bytes+0x4ac)==0x42c7f0&&
     ((s->pending_damage_kind==0x3b&&s->pending_damage==60)||
      (s->pending_damage_kind==0x3a&&s->pending_damage==40))&&
     !override_bank&&cue==0x142;
   /* 221E10 publishes a short hit and a fall on the two OM363 Give rows. */
   const int r102_armed16=b->work->room==0x102&&dependency==0x200&&stamp==3&&
     source->size>=0x1896&&source->bytes[0x1650]==1&&half(source->bytes+0x2f4)==0x1600&&
     !source->bytes[0x2f7]&&source->original_kind4ac==0x42c7f0&&
     w(source->bytes+0x4ac)==0x42c7f0&&!override_bank&&cue==0x11d&&
     ((source->bytes[0x2f6]==5&&w(source->bytes+0x1878)==0x3a&&half(source->bytes+0x1880)==0x11d&&
       s->pending_damage_kind==0x3a&&s->pending_damage==20)||
      (source->bytes[0x2f6]==7&&w(source->bytes+0x188c)==0x3b&&half(source->bytes+0x1894)==0x11d&&
       s->pending_damage_kind==0x3b&&s->pending_damage==40));
   if(override_bank!=UINT32_MAX&&!boss_melee&&!em0a_override&&!arena_slide&&!arena_em0a_f6&&
      !em0a_wide&&!west_em0a_face_kick&&!r101_weapon&&!armed47&&!r102_armed16&&!r104_strike)
    return fail(s,0x12c0f8,GH_PLAYER_FRAME_UNSUPPORTED);
   const int stamp8_attack=b->work->room==0x100&&dependency==0x200&&stamp==8&&
     source->size>0x1650&&source->bytes[0x1650]==5&&source->bytes[0x2f4]==0&&
     source->bytes[0x2f5]==0x15&&source->bytes[0x2f6]==1;
   /* The original AT2 EM00 and EM03 close strikes share this short Take. */
   const int formation_strike=b->work->room==0x100&&
     ((dependency==0x200&&stamp==3)||(dependency==0x203&&stamp==5))&&
     source->size>=0x16c8&&source->bytes[0x1650]==2&&w(source->bytes+0x16c4)==6&&
     half(source->bytes+0x2f4)==0x1500&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     override_bank==UINT32_MAX&&!cue&&w(take+0x4c)==20&&!w(p+0x160c);
   /* The unarmed AT02 and AT03 actors use the original selector-15 Give table. */
   const uint32_t home_mode=source->size>=0x16c8?w(source->bytes+0x16c4):UINT32_MAX;
   const int at2_strike=home_mode==2&&
     ((dependency==0x202&&stamp==29)||(dependency==0x203&&stamp==30));
   const int at3_strike=!home_mode&&dependency==0x202&&stamp==32;
   const int wave_strike=b->work->room==0x100&&(at2_strike||at3_strike)&&
     !source->bytes[0x1650]&&
     half(source->bytes+0x2f4)==0x1500&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     override_bank==UINT32_MAX&&!cue;
   /* West's EM03 patrol uses the same 21A538 Give rows as the AT02 actor. */
   const int patrol_strike=b->work->room==0x100&&dependency==0x203&&stamp==11&&
     source->size>=0x16c8&&source->bytes[0x1650]==5&&w(source->bytes+0x16c4)==6&&
     half(source->bytes+0x2f4)==0x1500&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     override_bank==UINT32_MAX&&!cue;
   const int at3_em00_short=b->work->room==0x100&&dependency==0x200&&stamp==31&&
     !home_mode&&!source->bytes[0x1650]&&(source_move==0x7000||source_move==0x1500)&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&override_bank==UINT32_MAX&&!cue&&
     w(take+0x4c)==20&&!w(p+0x160c);
   const int close_enemy=(dependency==0x203&&stamp==19)||(dependency==0x200&&stamp==20)||
     formation_strike||at3_em00_short||
     ((wave_strike||patrol_strike)&&w(take+0x4c)==20&&!w(p+0x160c));
   const int r105=b->work->room==0x105&&source->size>0x1650&&!source->bytes[0x1650];
   /* R105's original EM00 and EM27 selector15 rows use the shared short hit. */
   const int original_em27_short_hit=dependency==0x227&&source->size>0x1650&&
     ((b->work->room==0x102&&stamp==4&&!source->bytes[0x1650])||
      (b->work->room==0x101&&(stamp==0x1e||stamp==0x26)&&source->bytes[0x1650]==6))&&
     half(source->bytes+0x2f4)==0x1500&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&override_bank==UINT32_MAX&&!cue&&
     s->pending_damage_kind==0x37&&s->pending_damage==30&&!residual;
   const int r105_short_hit=r105&&s->pending_damage_kind==0x37&&
     ((dependency==0x200&&stamp==0&&s->pending_damage==20)||
      (dependency==0x227&&stamp==2&&s->pending_damage==30));
   /* EM27 selector16 publishes two Give rows with different Take reactions. */
   const int em27_selector16=r105&&dependency==0x227&&stamp==2&&
     source->bytes[0x2f4]==0&&source->bytes[0x2f5]==0x16&&source->bytes[0x2f6]==1;
   const int r105_combo_hit=em27_selector16&&
     ((s->pending_damage_kind==0x3d&&s->pending_damage==7)||
      (s->pending_damage_kind==0x37&&s->pending_damage==15));
   const int original_em27_combo_hit=dependency==0x227&&source->size>0x1650&&
     ((b->work->room==0x102&&stamp==4&&!source->bytes[0x1650])||
      (b->work->room==0x101&&(stamp==0x1e||stamp==0x26)&&source->bytes[0x1650]==6))&&
     half(source->bytes+0x2f4)==0x1600&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&override_bank==UINT32_MAX&&!cue&&
     !residual&&((s->pending_damage_kind==0x3d&&s->pending_damage==7)||
                 (s->pending_damage_kind==0x37&&s->pending_damage==15));
   const int opening_em03_combo=dependency==0x203&&stamp==19&&source->size>0x1650;
   /* The original 21FD50 light Give row is shared by EM02 and EM03. */
   const int west_combo=b->work->room==0x100&&(at3_strike||opening_em03_combo)&&
     !source->bytes[0x1650]&&half(source->bytes+0x2f4)==0x1600&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&override_bank==UINT32_MAX&&!cue&&
     ((s->pending_damage_kind==0x3d&&w(take+0x4c)==5&&s->pending_damage==5&&!residual)||
      (at3_strike&&s->pending_damage_kind==0x3e&&w(take+0x4c)==10&&
       s->pending_damage==8&&residual==2));
   /* The AT2 EM00's original 230F20 Give splits power 30 into 23 and 7. */
   const int west_formation_fall=b->work->room==0x100&&dependency==0x200&&stamp==3&&
     source->size>=0x16c8&&source->bytes[0x1650]==2&&w(source->bytes+0x16c4)==6&&
     half(source->bytes+0x2f4)==0x4200&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     override_bank==UINT32_MAX&&!cue&&s->pending_damage_kind==0x3e&&
     w(take+0x4c)==30&&s->pending_damage==23&&residual==7;
   const int west_em02_capture=b->work->room==0x100&&source->size>=0x16c8&&
     ((dependency==0x202&&
       ((stamp==4&&source->bytes[0x1650]==2&&w(source->bytes+0x16c4)==6)||
        (!source->bytes[0x1650]&&((stamp==29&&w(source->bytes+0x16c4)==2)||at3_strike))))||
      (dependency==0x203&&stamp==19&&!source->bytes[0x1650]))&&
     half(source->bytes+0x2f4)==0x1800&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     override_bank==UINT32_MAX&&!cue&&s->pending_damage_kind==0x4a&&
     !w(take+0x4c)&&!s->pending_damage&&!residual;
   /* The two original EM03 kind3E rows split power30 and power60. */
   const int em03_fall=r105&&dependency==0x203&&stamp==1&&s->pending_damage_kind==0x3e&&
     ((s->pending_damage==23&&residual==7)||(s->pending_damage==45&&residual==15));
   const int at3_em00_timed=b->work->room==0x100&&dependency==0x200&&stamp==31&&
     !home_mode&&!source->bytes[0x1650]&&source_move==0x1500&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&override_bank==UINT32_MAX&&!cue&&
     w(take+0x4c)==15&&!residual;
   const int r100_em00_strike=b->work->room==0x100&&source_kind==0x202&&dependency==0x200&&
     source->size>=0x1884&&source_move==0x1500&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&override_bank==UINT32_MAX&&!cue&&
     !w(source->bytes+0x1870)&&!residual;
   const int north_em00=r100_em00_strike&&stamp==10&&home_mode==6&&source->bytes[0x1650]==5;
   const int opening_em00=r100_em00_strike&&stamp==20&&!home_mode&&!source->bytes[0x1650]&&
     source->size>=0x1896&&w(source->bytes+0x187c)==UINT32_MAX&&!half(source->bytes+0x1880)&&
     !memcmp(source->bytes+0x1870,source->bytes+0x1884,18);
   const int r100_em00_timed=(north_em00||opening_em00)&&w(source->bytes+0x5f4)==1&&
     w(source->bytes+0x1874)==15&&w(source->bytes+0x1878)==0x3d&&
     s->pending_damage_kind==0x3d&&w(take+0x4c)==15&&s->pending_damage==15;
   /* The source timer can be one while both published Give rows
    * still describe the same short Take. */
   const int north_em00_matching_rows=north_em00&&source->size>=0x1896&&
     w(source->bytes+0x5f4)==1&&w(source->bytes+0x187c)==UINT32_MAX&&
     !half(source->bytes+0x1880)&&!w(source->bytes+0x1884)&&
     w(source->bytes+0x1888)==20&&w(source->bytes+0x188c)==0x37&&
     w(source->bytes+0x1890)==UINT32_MAX&&!half(source->bytes+0x1894);
   const int north_em00_short=north_em00&&(!w(source->bytes+0x5f4)||north_em00_matching_rows)&&
     w(source->bytes+0x1874)==20&&w(source->bytes+0x1878)==0x37&&
     s->pending_damage_kind==0x37&&w(take+0x4c)==20&&s->pending_damage==20;
   const int short_hit=s->pending_damage_kind==0x37&&
     ((s->pending_damage==15&&(dependency==0x20a||at3_em00_timed))||
      (s->pending_damage==20&&(close_enemy||stamp8_attack)));
   const int stamp19_fall=s->pending_damage_kind==0x3e&&(s->pending_damage==45||s->pending_damage==23)&&
     dependency==0x203&&stamp==19;
   /* Both AT0B attacks publish the same original unarmed short Take. */
   const int area11_short_hit_source=((dependency==0x200&&stamp==7)||(dependency==0x202&&stamp==0x17))&&
     source->size>=0x16c8&&!source->bytes[0x1650]&&w(source->bytes+0x16c4)==2&&
     override_bank==UINT32_MAX&&!cue&&residual==0;
   const int r102_short_hit=b->work->room==0x102&&((dependency==0x200&&stamp==11)||area11_short_hit_source)&&
     source->bytes[0x2f4]==0&&source->bytes[0x2f5]==0x15&&source->bytes[0x2f6]==1&&
     s->pending_damage_kind==0x37&&s->pending_damage==20;
   const int wave_fall=(wave_strike||patrol_strike)&&s->pending_damage_kind==0x3e&&
     ((w(take+0x4c)==30&&s->pending_damage==23&&residual==7)||
      (w(take+0x4c)==60&&s->pending_damage==45&&residual==15));
   const int area11_fall=b->work->room==0x102&&dependency==0x202&&stamp==0x17&&
     source->size>0x1650&&!source->bytes[0x1650]&&half(source->bytes+0x2f4)==0x1500&&
     source->bytes[0x2f6]==1&&s->pending_damage_kind==0x3e&&
     ((w(take+0x4c)==60&&s->pending_damage==45&&residual==15)||
      (source->size>=0x16c8&&w(source->bytes+0x16c4)==2&&override_bank==UINT32_MAX&&!cue&&
       w(take+0x4c)==30&&s->pending_damage==23&&residual==7));
   const uint32_t em0a_power=w(take+0x4c);
   /* 21FB34 publishes the rank-five timed West kick's weak Give. */
   const int west_em0a_timed=em0a_opening&&dependency==0x20a&&source->size>=0x1884&&
     half(source->bytes+0x2f4)==0x1500&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     w(source->bytes+0x5f4)==1&&!w(source->bytes+0x1870)&&w(source->bytes+0x1874)==15&&
     w(source->bytes+0x1878)==0x3d&&override_bank==UINT32_MAX&&!cue&&
     em0a_power==15&&s->pending_damage_kind==0x3d&&s->pending_damage==15&&!residual;
   const int em0a_kick=em0a_story&&dependency==0x20a&&half(source->bytes+0x2f4)==0x1500&&
     source->bytes[0x2f6]==1&&override_bank==UINT32_MAX&&!cue&&
     ((s->pending_damage_kind==0x37&&!residual&&
       ((em0a_power==15&&s->pending_damage==15)||(em0a_power==20&&s->pending_damage==20)))||
      (em0a_power==15&&s->pending_damage_kind==0x3e&&s->pending_damage==12&&residual==3));
   /* EM0A's original reverse kick falls through to Take 3E with zero power. */
   const int em0a_reverse=em0a_story&&dependency==0x20a&&half(source->bytes+0x2f4)==0x4400&&
     source->bytes[0x2f6]==1&&override_bank==UINT32_MAX&&!cue&&
     s->pending_damage_kind==0x3e&&!em0a_power&&!s->pending_damage&&!residual;
   const int west_em0a_reverse=em0a_opening&&dependency==0x20a&&source->size>=0x1884&&
     half(source->bytes+0x2f4)==0x4400&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     !w(source->bytes+0x1870)&&w(source->bytes+0x1874)==20&&w(source->bytes+0x1878)==0x3e&&
     override_bank==UINT32_MAX&&!cue&&em0a_power==20&&s->pending_damage_kind==0x3e&&
     s->pending_damage==15&&residual==5;
   const int r101_west_em00=source->size>0x1650&&dependency==0x200&&stamp==12&&
     source->bytes[0x1650]==4;
   /* REL0D68's first EM03 uses 21A538 close shape 2 with Give37/power20. */
   const int r101_side_em03=source->size>=0x187c&&dependency==0x203&&stamp==0x20&&
     source->bytes[0x1650]==6&&w(source->bytes+0x1870)==2&&w(source->bytes+0x1874)==20&&
     w(source->bytes+0x1878)==0x37;
   const int r101_side_em00=source->size>=0x187c&&dependency==0x200&&stamp==0x18&&
     source->bytes[0x1650]==6&&w(source->bytes+0x1870)<=1&&w(source->bytes+0x1874)==20&&
     w(source->bytes+0x1878)==0x37;
   const int r101_side_em02=source->size>=0x187c&&dependency==0x202&&stamp==0x27&&
     source->bytes[0x1650]==6&&w(source->bytes+0x1870)==2&&w(source->bytes+0x1874)==20&&
     w(source->bytes+0x1878)==0x37;
   const int r101_short=b->work->room==0x101&&(r101_west_em00||r101_side_em03||r101_side_em00||r101_side_em02)&&
     half(source->bytes+0x2f4)==0x1500&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&override_bank==UINT32_MAX&&!cue&&
     s->pending_damage_kind==0x37&&s->pending_damage==20&&w(take+0x4c)==20&&!residual;
   const int r101_em0f_short=b->work->room==0x101&&source->size>=0x187c&&dependency==0x20f&&
     (stamp==0x1a||stamp==0x1b||stamp==0x22||stamp==0x23)&&source->bytes[0x1650]==6&&
     half(source->bytes+0x2f4)==0x1500&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     w(source->bytes+0x1870)==2&&w(source->bytes+0x1874)==2&&w(source->bytes+0x1878)==0x37&&
     s->pending_damage_kind==0x37&&s->pending_damage==2&&w(take+0x4c)==2&&!residual&&
     override_bank==UINT32_MAX&&!cue;
   const int r101_wide_strike=b->work->room==0x101&&source->size>=0x187c&&
     dependency==0x200&&stamp==0x18&&source->bytes[0x1650]==6&&
     half(source->bytes+0x2f4)==0x1500&&source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&
     !w(source->bytes+0x1870)&&w(source->bytes+0x1874)==15&&w(source->bytes+0x1878)==0x3d&&
     s->pending_damage_kind==0x3d&&s->pending_damage==15&&w(take+0x4c)==15&&!residual&&
     override_bank==UINT32_MAX&&!cue;
   /* Arena wave 2 EM02 uses the original close 21A538 fall row. */
   const int r101_side_em02_fall=b->work->room==0x101&&dependency==0x202&&stamp==0x27&&
     source->size>=0x187c&&source->bytes[0x1650]==6&&half(source->bytes+0x2f4)==0x1500&&
     source->bytes[0x2f6]==1&&!source->bytes[0x2f7]&&override_bank==UINT32_MAX&&!cue&&
     w(source->bytes+0x1870)==2&&w(source->bytes+0x1874)==60&&w(source->bytes+0x1878)==0x3e&&
     s->pending_damage_kind==0x3e&&w(take+0x4c)==60&&s->pending_damage==45&&residual==15;
   if(!r101_em0f_short&&!r101_wide_strike&&!r101_side_em02_fall&&!r200_short&&!r101_short&&!r101_weapon&&!armed47&&!r102_armed16&&!r102_short_hit&&!original_em27_short_hit&&
      !area11_fall&&!r105_short_hit&&!r105_combo_hit&&!original_em27_combo_hit&&!r104_strike&&
      !em03_fall&&!arena_hit&&!em14_sphere&&!em0a_override&&!em0a_kick&&!em0a_reverse&&!em0a_wide&&
      !west_em0a_timed&&!west_em0a_reverse&&!west_em0a_face_kick&&
      (b->work->room!=0x100||(!short_hit&&!stamp19_fall&&!wave_fall&&!west_formation_fall&&
       !west_em02_capture&&!west_combo&&!devil_melee&&!devil_combo&&!r100_em00_timed&&!north_em00_short)))
    return fail(s,0x12c0f8,GH_PLAYER_FRAME_UNSUPPORTED);
  }
  gh_player_frame_status status=damage_hp_0012c0f8(s,s->pending_damage);
  if(status)return status;
  /* 109A10 bypasses the damage score when raw Take power is nonpositive. */
  s->damage_stage=s->pending_damage>0?5:6;
 }
 if(s->damage_stage==5){
  const unsigned kind=s->pending_damage_kind;
  gh_player_frame_status status=damage_score_001fa280(s,kind==0x37||kind==0x3d?160:320);
  if(status)return status;
  s->damage_stage=6;
 }
 if(s->damage_stage==6){
  /* 2CBA48 uses the ledger's source actor only for logical bank 1. */
  const uint8_t *take=b->take_object->bytes;
  const uint32_t override_bank=w(take+0x40);
  const uint16_t program=override_bank==UINT32_MAX?0x6e:half(take+0x44);
  const gh_title_player_view *bank_actor=NULL;
  if(override_bank==1&&(gh_hitarea_object_last_hit_actor(b->take_object,&bank_actor)||!bank_actor))
   return fail(s,0x2cba48,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status status=hurt_sound_with_bank(s,(int16_t)program,override_bank==1?1:0,bank_actor);
  if(status)return status;
  const int sword=(b->work->room==0x100||b->work->room==0x102)&&
    (s->pending_damage_kind==0x3a||s->pending_damage_kind==0x3b);
  s->damage_stage=sword?15:8;
 }
 if(s->damage_stage==15){
  /* The admitted cEm00 strikes with OM363 select a second original CORE cue. */
  gh_player_frame_status status=hurt_sound(s,0x137);
  if(status)return status;
  s->damage_stage=8;
 }
 if(s->damage_stage==8){
  if(w(p+0x1620))return fail(s,0x2cbba0,GH_PLAYER_FRAME_UNSUPPORTED);
  put(p+0x1620,0);
  if(p[0x2b4]>=5){
   const gh_actor_part *part;
   if(!b->model||gh_actor_model_part_at(b->model,4,&part)||!part->alias_f0)
    return fail(s,0x14bfe0,GH_PLAYER_FRAME_UNSUPPORTED);
   memcpy(s->damage_position,part->alias_f0,12);
   s->damage_stage=9;
  }else s->damage_stage=10;
 }
 if(s->damage_stage==9){
  if(!b->damage_effect)return fail(s,0x306838,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status status=b->damage_effect(b->damage_context,s->damage_position,&pc);
  if(status)return fail(s,pc?pc:0x306838,status);
  s->damage_stage=10;
 }
 if(s->damage_stage==10){
  put(p+0x54c,0x40400000);
  s->damage_stage=11;
 }
 if(s->damage_stage==11){
  if(!b->damage_abort)return fail(s,0x126770,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status status=b->damage_abort(b->damage_context,b,&pc);
  if(status)return fail(s,pc?pc:0x126770,status);
  s->damage_stage=12;
 }
 if(s->damage_stage==12){
  if(w(p+0x15f4)&2u){
   if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
   put(p+0x15f4,w(p+0x15f4)&~2u);
   s->damage_stage=14;
  }else s->damage_stage=13;
 }
 if(s->damage_stage==14){
  gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
  if(status)return fail(s,pc?pc:0x124f60,status);
  s->damage_stage=13;
 }
 if(s->damage_stage==13){
  const unsigned kind=s->pending_damage_kind;
  if(s->pending_damage_kind==0x48){
   /* Original 109DCC notifies the enemy director before the Take selector. */
   b->init->enemies[0x53b]=60;
   if((int16_t)half(p+0x54a)<=0){
    uint32_t target;
    if(!elf_word(b,0x41bab4,&target)||target!=0x109e10)
     return fail(s,0x109de8,GH_PLAYER_FRAME_UNSUPPORTED);
    p[0x2f4]=2;p[0x2f5]=1;p[0x2f6]=p[0x2f7]=0;
   }else if(!(w(p+0x15f4)&0x800u)){
    uint32_t target;
    if(!elf_word(b,0x41bb34,&target)||target!=0x10a064)
     return fail(s,0x109f80,GH_PLAYER_FRAME_UNSUPPORTED);
    put(p+0x5f8,w(p+0x5f8)+150u);
    put(p+0x5f4,w(p+0x5f4)+100u);
    p[0x2f4]=1;p[0x2f5]=3;p[0x2f6]=p[0x2f7]=0;
   }
  }else if(kind==0x41||kind==0x42||kind==0x45){
   static const uint32_t lethal_entries[]={0,0x109e28,0x109e40,0,0,0x109e90};
   static const uint32_t living_entries[]={0,0x10a094,0x10a0c4,0,0,0x10a15c};
   const uint8_t variant=(uint8_t)(kind-0x40);
   const unsigned slot=4*(kind-0x37);
   uint32_t entry;
   if((int16_t)half(p+0x54a)<=0){
    if(!elf_word(b,0x41ba70+slot,&entry)||entry!=lethal_entries[variant])
     return fail(s,0x109de8,GH_PLAYER_FRAME_UNSUPPORTED);
    p[0x2f4]=2;p[0x2f5]=1;p[0x2f6]=0;p[0x2f7]=variant;
   }else if(!(w(p+0x15f4)&0x800u)){
    if(!elf_word(b,0x41baf0+slot,&entry)||entry!=living_entries[variant])
     return fail(s,0x109f80,GH_PLAYER_FRAME_UNSUPPORTED);
    put(p+0x5f8,w(p+0x5f8)+150u);
    put(p+0x5f4,w(p+0x5f4)+100u);
    p[0x2f4]=1;p[0x2f5]=3;p[0x2f6]=0;p[0x2f7]=variant;
   }
  }else if((int16_t)half(p+0x54a)<=0){
   const int fall=(kind==0x3e&&(b->work->room==0x100||b->work->room==0x102||
       b->work->room==0x103||b->work->room==0x104||b->work->room==0x105))||
       (kind==0x3b&&(b->work->room==0x100||b->work->room==0x101||b->work->room==0x102));
   const int short_hit=(kind==0x37&&(b->work->room==0x100||b->work->room==0x102||
                       b->work->room==0x103||b->work->room==0x105))||
                       (kind==0x3d&&(b->work->room==0x100||b->work->room==0x103||
                        b->work->room==0x104||b->work->room==0x105))||
                       (kind==0x3a&&(b->work->room==0x100||b->work->room==0x101||b->work->room==0x102))||
                       (kind==0x3c&&(b->work->room==0x102||b->work->room==0x104))||
                       ((kind==0x49||kind==0x4e)&&b->work->room==0x103)||
                       (kind==0x53&&b->work->room==0x100);
   uint32_t target;
   if((!fall&&!short_hit)||!elf_word(b,0x41ba70+4*(kind-0x37),&target)||
      target!=(fall?0x109e10u:0x109f04u))
    return fail(s,0x109de8,GH_PLAYER_FRAME_UNSUPPORTED);
   p[0x2f4]=2;p[0x2f5]=(uint8_t)fall;p[0x2f6]=p[0x2f7]=0;
  }else if(s->pending_damage_kind==0x49){
   if(!(w(p+0x15f4)&0x800u)){
    uint32_t target;
    if(!elf_word(b,0x41bb38,&target)||target!=0x10a04cu)
     return fail(s,0x109f80,GH_PLAYER_FRAME_UNSUPPORTED);
    p[0x2f4]=1;p[0x2f5]=2;p[0x2f6]=p[0x2f7]=0;
   }
  }else if(s->pending_damage_kind==0x4e){
   uint32_t target;
   if(!elf_word(b,0x41bb4c,&target)||target!=0x10a224u)
    return fail(s,0x109f80,GH_PLAYER_FRAME_UNSUPPORTED);
   put(p+0x5f4,0);
   p[0x2f4]=1;p[0x2f5]=5;p[0x2f6]=p[0x2f7]=0;
  }else if(s->pending_damage_kind==0x53){
   if(!(w(p+0x15f4)&0x800u)){
    uint32_t target;
    if(!elf_word(b,0x41bb60,&target)||target!=0x10a240u)
     return fail(s,0x109f80,GH_PLAYER_FRAME_UNSUPPORTED);
    put(p+0x5f8,w(p+0x5f8)+150u);
    put(p+0x5f4,w(p+0x5f4)+100u);
    p[0x2f4]=1;p[0x2f5]=1;p[0x2f6]=p[0x2f7]=0;
   }
  }else if(!(w(p+0x15f4)&0x800u)){
   if(kind==0x4a||kind==0x4b){
    uint32_t target;
    if(!elf_word(b,0x41baf0+4*(kind-0x37),&target)||target!=0x10a210u)
     return fail(s,0x109f80,GH_PLAYER_FRAME_UNSUPPORTED);
    p[0x2f4]=1;p[0x2f5]=1;p[0x2f6]=p[0x2f7]=0;
    s->damage_stage=7;
    return GH_PLAYER_FRAME_OK;
   }
   const int fall=s->pending_damage_kind==0x3e||s->pending_damage_kind==0x3b;
   if((b->work->room==0x102&&fall)||
      (b->work->room==0x100&&s->pending_damage_kind==0x3b)){
    uint32_t target;
    if(!elf_word(b,0x41baf0+4*(s->pending_damage_kind-0x37),&target)||target!=0x10a064u)
     return fail(s,0x109f80,GH_PLAYER_FRAME_UNSUPPORTED);
   }
   if(((b->work->room==0x100||b->work->room==0x102)&&s->pending_damage_kind==0x3a)||
      ((b->work->room==0x102||b->work->room==0x104)&&s->pending_damage_kind==0x3c)){
    uint32_t target;
    if(!elf_word(b,0x41baf0+4*(s->pending_damage_kind-0x37),&target)||target!=0x109facu)
     return fail(s,0x109f80,GH_PLAYER_FRAME_UNSUPPORTED);
   }
   const int small_hit=s->pending_damage_kind==0x3d;
   if(small_hit){
    uint32_t target;
    if(!elf_word(b,0x41bb08,&target)||target!=0x109ffcu)
     return fail(s,0x109f80,GH_PLAYER_FRAME_UNSUPPORTED);
   }
   const uint32_t stagger=w(p+0x5f4)+(fall?100u:small_hit?20u:40u);
   put(p+0x5f8,w(p+0x5f8)+150u);
   const int knockdown=!fall&&(int32_t)stagger>=130;
   put(p+0x5f4,knockdown?0:stagger);
   p[0x2f4]=1;p[0x2f5]=fall?3:knockdown?6:small_hit?0:1;p[0x2f6]=p[0x2f7]=0;
  }
  s->damage_stage=7;
 }
 return GH_PLAYER_FRAME_OK;
}
static gh_player_frame_status reset_give(gh_game_player_frame*s){const gh_player_frame_bindings*b=&s->b;uint8_t*p=b->player->view.bytes;if(!b->give_binding_known)return fail(s,0x10a2c8,GH_PLAYER_FRAME_UNSUPPORTED);if((b->give_present_mask&31)!=31)return GH_PLAYER_FRAME_OK;if(!b->give_objects)return fail(s,0x10a2c8,GH_PLAYER_FRAME_ARGUMENT);while(s->give_index<5){gh_hitarea_object*o=&b->give_objects[s->give_index];gh_title_player_view*actor=NULL;if(gh_hitarea_object_actor(o,&actor)||!actor||actor->bytes!=p||o->size<0x50)return fail(s,0x10a2c8,GH_PLAYER_FRAME_ARGUMENT);uint32_t pc=0;if(!s->give_phase){sh(o->bytes+0x46,0);s->give_phase=1;}if(s->give_phase==1){gh_hitarea_status e=gh_hitarea_object_set_active_001fd668(o,0,&pc);if(e)return fail(s,pc?pc:0x1fd668,GH_PLAYER_FRAME_CALLEE);s->give_phase=2;}if(s->give_phase==2){gh_hitarea_status e=gh_hitarea_object_set_node_scalar_001fd7b0(o,(int16_t)half(p+0x5b4+s->give_index*8),0,&pc);if(e)return fail(s,pc?pc:0x1fd7b0,GH_PLAYER_FRAME_CALLEE);s->give_phase=3;}put(o->bytes+0x4c,0);s->give_index++;s->give_phase=0;}return GH_PLAYER_FRAME_OK;}static gh_player_frame_status idle_random(gh_game_player_frame*s,uint32_t*out){if(gh_game_mt_next_0030f348(&s->b.init->random,out))return fail(s,0x31cfe0,GH_PLAYER_FRAME_ARGUMENT);return GH_PLAYER_FRAME_OK;}
static gh_player_frame_status idle_select(gh_game_player_frame*s){const gh_player_frame_bindings*b=&s->b;uint8_t*p=b->player->view.bytes;sh(p+0x5e0,0);sh(p+0x5e2,0);float current=number(w(b->init->heat+4)),threshold=number(w(b->init->heat+12));if(!isfinite(current)||!isfinite(threshold))return fail(s,0x2a9bc0,GH_PLAYER_FRAME_NUMERIC);unsigned mode=current<threshold?0:1;put(b->init->heat+0x14,mode);if(b->init->heat[0x10])mode=2;put(p+0x15b4,0);if(!b->resource||b->resource_bytes<0x2c0)return fail(s,0x10be18,GH_PLAYER_FRAME_ARGUMENT);unsigned selector=p[0x2f7];uint32_t random=0;int alternate=selector==2;gh_player_frame_status e;
 if(selector==0){e=idle_random(s,&random);if(e)return e;alternate=(random&1)!=0;}
 size_t motion=mode==0?0x28:mode==1?0x2a0:0x2a8,sequence=motion+4;
 if(alternate){if(mode==0){motion=0x2b8;sequence=0x2bc;}else if(mode==1){e=idle_random(s,&random);if(e)return e;unsigned n=random%3;if(n==1&&w(p+0x690)){motion=0x2b0;sequence=0x2b4;}else if(n==2){motion=0x30;sequence=0x34;}else{motion=0x2a8;sequence=0x2ac;}}else{e=idle_random(s,&random);if(e)return e;unsigned n=random&1;if(n==1&&!w(p+0x690))n=0;motion=n?0x2b0:0x30;sequence=motion+4;}put(p+0x15b4,2);p[0x2f7]=2;}
 s->idle_motion_word=motion;s->idle_sequence_word=sequence;s->idle_phase=1;return GH_PLAYER_FRAME_OK;}
static gh_player_frame_status idle_frame(gh_game_player_frame*s){const gh_player_frame_bindings*b=&s->b;uint8_t*p=b->player->view.bytes;uint32_t pc=0;
 if(!s->idle_phase){int16_t hp;uint16_t raw=(uint16_t)half(p+0x54a);memcpy(&hp,&raw,2);if(hp<1)sh(p+0x54a,1);if(p[0x2f6]==0){gh_player_frame_status e=idle_select(s);if(e)return e;}else s->idle_phase=p[0x2f6]==1?3:8;}
 if(s->idle_phase==1){if(!b->auxiliary_bindings_known)return fail(s,0x10c030,GH_PLAYER_FRAME_UNSUPPORTED);if(b->aux_6a0){
  if(!b->interact_prop)return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status status=b->interact_prop(b->motion_context,0,b->aux_6a0,b,0x36c,0x388,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status);
  s->idle_motion_word=0x344;s->idle_sequence_word=0x348;p[0x2f7]=3;
 }if(w(p+0x6a0))return fail(s,0x10c030,GH_PLAYER_FRAME_UNSUPPORTED);if(b->aux_698||w(p+0x698))return fail(s,0x10c080,GH_PLAYER_FRAME_UNSUPPORTED);size_t mo=s->idle_motion_word,seq=s->idle_sequence_word;if(mo>b->resource_bytes||4>b->resource_bytes-mo||seq>b->resource_bytes||4>b->resource_bytes-seq)return fail(s,0x10be18,GH_PLAYER_FRAME_ARGUMENT);size_t motion=w(b->resource+mo),candidate=w(b->resource+seq);if(motion>=b->resource_bytes||candidate>=b->resource_bytes)return fail(s,0x2a8578,GH_PLAYER_FRAME_ARGUMENT);s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,.work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,.motion_offset=motion,.sequence_candidate_offset=candidate,.start_frame_bits=0,.blend_frames=10,.flags=0,.release_previous_sequence=!b->aux_6a0,.rate=b->rate};s->idle_phase=2;}
 if(s->idle_phase==2){if(!b->model||b->model->root.bytes!=p||!b->initialize_motion)return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);gh_player_init_status e=b->initialize_motion(b->motion_context,&s->pending_motion,&pc);if(e)return fail(s,pc?pc:0x2a8578,e==GH_PLAYER_INIT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);put(p+0x15b0,90);p[0x2f6]=(uint8_t)(p[0x2f6]+1u);s->idle_phase=3;}
 if(s->idle_phase==3){if(!(w((const uint8_t*)b->work+0x5d8)&0x2000000)&&w(p+0x5d8)!=4&&p[0x61f]){uint32_t speed;if(!b->ground_globals||!b->ground_globals->tuning_loaded)return fail(s,0x10c120,GH_PLAYER_FRAME_UNSUPPORTED);memcpy(&speed,&b->ground_globals->idle_turn_speed,4);float absolute=number(w(p+0x608)),heading=number(w(p+0x104)),maximum=b->ground_globals->idle_turn_cap;if(!isfinite(absolute)||!isfinite(heading)||!isfinite(maximum))return fail(s,0x10c120,GH_PLAYER_FRAME_NUMERIC);float delta=sub(absolute,number(0x3f060a92));if(delta<0)delta=0;volatile float product=delta*.2f;float turn=product;if(maximum<turn)turn=maximum;if(number(w(p+0x604))<0)turn=-turn;volatile float sum=heading+turn;float wrapped;if(gh_id_angle_reference_001f7d48(sum,&wrapped))return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);put(p+0x15f0,speed);if((int8_t)p[0x648]>10)p[0x648]=10;sf(p+0x104,wrapped);}s->idle_phase=4;}
 if(s->idle_phase==4){if(!s->motion_step_complete){if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);if(!s->pending_motion.player)s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,.work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};gh_player_frame_status e=b->advance_motion(b->motion_context,&s->pending_motion,&s->motion_ended,&pc);if(e)return fail(s,pc?pc:0x157c18,e);s->motion_step_complete=1;}if(s->motion_ended&&!s->idle_end_prefix_done){uint32_t n=w(p+0x15b4);if(n)put(p+0x15b4,n-1);if(p[0x2f7]==2){p[0x2f6]=0;p[0x2f7]=1;}else if(p[0x2f7]<3)s->idle_end_needs_random=1;s->idle_end_prefix_done=1;}if(s->idle_end_needs_random){uint32_t random;gh_player_frame_status e=idle_random(s,&random);if(e)return e;int32_t remaining;uint32_t bits=w(p+0x15b4);memcpy(&remaining,&bits,4);if(!(random&3)&&remaining<1){p[0x2f7]=2;p[0x2f6]=0;}s->idle_end_needs_random=0;}s->idle_phase=5;} if(s->idle_phase==5){if(!(w((const uint8_t*)b->work+0x5e4)&0x20000000)){float in[4],out[4];memcpy(in,p+0x100,16);for(unsigned i=0;i<3;i++){float delta=number(w(p+0x360+i*4));if(!isfinite(in[i])||!isfinite(delta))return fail(s,0x14f9f0,GH_PLAYER_FRAME_NUMERIC);volatile float sum=in[i]+delta;in[i]=sum;}if(gh_actor_euler_adjust_001f7da0(in,out,GH_STATIC_BINARY32_REFERENCE,&pc))return fail(s,pc?pc:0x1f7da0,GH_PLAYER_FRAME_UNSUPPORTED);memcpy(p+0x100,out,12);}s->idle_phase=6;}
 if(s->idle_phase==6){if(!(w((const uint8_t*)b->work+0x5e4)&0x20000000)){if(!b->player->view.alias_f0)return fail(s,0x14f928,GH_PLAYER_FRAME_ARGUMENT);float local[3],angles[3],matrix[16],world[4],next[3];for(unsigned i=0;i<3;i++){float delta=number(w(p+0x330+i*4)),scale=number(w(p+0x110+i*4));if(!isfinite(delta)||!isfinite(scale))return fail(s,0x14f928,GH_PLAYER_FRAME_NUMERIC);volatile float product=delta*scale;local[i]=product;}memcpy(angles,p+0x100,12);if(gh_actor_matrix_rotation_00147728(angles,0,matrix,GH_STATIC_BINARY32_REFERENCE,&pc))return fail(s,pc?pc:0x147728,GH_PLAYER_FRAME_UNSUPPORTED);if(gh_scene_at_apply_world_001508d8(matrix,local,world))return fail(s,0x150830,GH_PLAYER_FRAME_NUMERIC);for(unsigned i=0;i<3;i++){volatile float sum=number(w(b->player->view.alias_f0+i*4))+world[i];next[i]=sum;if(!isfinite(next[i]))return fail(s,0x14f928,GH_PLAYER_FRAME_NUMERIC);}memcpy(b->player->view.alias_f0,next,12);}s->idle_phase=7;}
 if(s->idle_phase==7){
  gh_player_target target;
  gh_player_frame_status e=enemy_focus(s,3,number(0x40490fdb),&target);
  if(e)return e;
  uint32_t timer=w(p+0x15b0);
  put(p+0x15b0,target.handle?(timer?timer-1:0):90);
  s->idle_phase=8;
 }
 if(s->idle_phase==8){if(!p[0x61f])p[0x1600]=2;s->idle_phase=9;}
 if(s->idle_phase==9){if(!b->idle_input||!b->idle_input_bindings)return fail(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);const gh_player_frame_bindings*q=&b->idle_input_bindings->frame;if(q->player!=b->player||q->init!=b->init||q->work!=b->work||q->current_player!=b->current_player)return fail(s,0x123938,GH_PLAYER_FRAME_ARGUMENT);if(!s->input_started){gh_player_frame_status e=gh_player_idle_input_begin(b->idle_input);if(e)return fail(s,0x123938,e);s->input_started=1;}gh_player_frame_status e=gh_game_player_idle_input_00123938(b->idle_input,b->idle_input_bindings,&s->input_result);if(e)return fail(s,b->idle_input->frontier?b->idle_input->frontier:0x123938,e);s->idle_phase=10;}
 if(s->idle_phase==10){if(!s->input_result){gh_player_frame_status e=gh_game_player_idle_cleanup_0012bfb8(b->idle_input);if(e)return fail(s,b->idle_input->frontier?b->idle_input->frontier:0x12bfb8,e);}s->idle_phase=11;}
 if(s->idle_phase==11){if(b->init->heat[0x10])put(p+0x54c,0x40a00000);if(!b->take_binding_known)return fail(s,0x1fd610,GH_PLAYER_FRAME_UNSUPPORTED);if(b->take_object){while(s->post_take_index<3){int16_t index=(int16_t)half(p+0x67c+s->post_take_index*4);gh_hitarea_status e=gh_hitarea_object_set_node_active_001fd610(b->take_object,index,p[0x1603]==0,&pc);if(e)return fail(s,pc?pc:0x1fd610,GH_PLAYER_FRAME_CALLEE);s->post_take_index++;}}s->idle_phase=12;}
 return GH_PLAYER_FRAME_OK;
}/* 1299F0 angle0/mode0 for the original grab approach. Both actors retain
 * their canonical position stores; the grab changes player X/Z only. */
static gh_player_frame_status place_grab_001299f0(gh_game_player_frame *s,
    const gh_title_player_view *target,uint32_t local_x,uint32_t local_z)
{
 const gh_player_frame_bindings*b=&s->b;
 const gh_title_player_view *player=&b->player->view;
 uint8_t *p=player->bytes,*e=target->bytes;
 const uintptr_t pb=(uintptr_t)p,pp=(uintptr_t)player->alias_f0;
 const uintptr_t eb=(uintptr_t)e,ep=(uintptr_t)target->alias_f0;
 if(pp<pb||pp-pb>player->size||12>player->size-(pp-pb)||
    ep<eb||ep-eb>target->size||12>target->size-(ep-eb))
  return fail(s,0x1299f0,GH_PLAYER_FRAME_ARGUMENT);
 float from[3],to[3],turn,player_yaw,enemy_yaw,sn,cs;
 memcpy(from,player->alias_f0,12);
 memcpy(to,target->alias_f0,12);
 const float yaw=number(w(p+0x104));
 uint32_t pc=0;
 if(gh_player_target_angle_002da038(b->elf,b->elf_bytes,from,to,yaw,number(0x40490fdb),&turn,&pc))
  return fail(s,pc?pc:0x2da038,GH_PLAYER_FRAME_UNSUPPORTED);
 volatile float aimed=yaw+turn;
 if(gh_id_angle_reference_001f7d48(aimed,&player_yaw))
  return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);
 volatile float opposite=player_yaw+number(0x40490fdb);
 if(gh_id_angle_reference_001f7d48(opposite,&enemy_yaw)||
    gh_id_angle_reference_001f7d48(player_yaw,&player_yaw))
  return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);
 if(gh_sdk_rotation_sincos_reference_0030a5d0(enemy_yaw,&sn,&cs,GH_STATIC_BINARY32_REFERENCE,&pc))
  return fail(s,pc?pc:0x147988,GH_PLAYER_FRAME_UNSUPPORTED);
 const float basis[16]={cs,0,-sn,0, 0,1,0,0, sn,0,cs,0, 0,0,0,1};
 volatile float x=number(local_x)*number(w(e+0x110));
 volatile float z=number(local_z)*number(w(e+0x118));
 const float local[3]={x,0,z};
 float offset[4];
 if(gh_scene_at_apply_world_001508d8(basis,local,offset))
  return fail(s,0x30a2b0,GH_PLAYER_FRAME_UNSUPPORTED);
 volatile float new_x=to[0]+offset[0],new_z=to[2]+offset[2];
 if(!isfinite(new_x)||!isfinite(new_z))
  return fail(s,0x1299f0,GH_PLAYER_FRAME_NUMERIC);
 sf(p+0x104,player_yaw);
 sf(e+0x104,enemy_yaw);
 sf(player->alias_f0,new_x);
 sf(player->alias_f0+8,new_z);
 memset(p+0x550,0,12);
 memset(e+0x550,0,12);
 return GH_PLAYER_FRAME_OK;
}

static int r103_grab_attacker(const gh_boot_work *work,const gh_title_player_view *attacker,unsigned dependency)
{
 if(work->room!=0x103||!attacker||!attacker->bytes||attacker->size<=0x1650)return 0;
 const uint8_t *p=attacker->bytes;
 const unsigned stamp=half(p+0x640);
 const int em00=dependency==0x200&&(stamp==2||stamp==3);
 const int em03=dependency==0x203&&(stamp==6||stamp==7);
 const int em0a=dependency==0x20a&&(stamp==4||stamp==5);
 const int em14=dependency==0x214&&stamp==0;
 return (em00||em03||em0a||em14)&&w(p+0x564)==dependency&&p[0x1650]==(em14?15:24)&&
    attacker->original_vtable214==0x4479c8&&half(p+0x2fe)==0x204;
}

/* Gene's short grab response to original 129718 selector 2. */
static gh_player_frame_status hurt_grab_0011c348(gh_game_player_frame *s)
{
 enum { PLACE=1, BIND, AUX, FOLLOW, CAMERA, ADVANCE, END, ROTATE, TRANSLATE, DAMAGE, RANK, VOICE, VIBRATE, DEATH_SE };
 const gh_player_frame_bindings *b=&s->b;
 const gh_title_player_view *attacker=b->grab_attacker;
 uint8_t *p=b->player->view.bytes;
 uint32_t pc=0;
 if(!b->player->view.alias_f0||!attacker||!attacker->bytes||attacker->size<0x770)
  return fail(s,0x11c348,GH_PLAYER_FRAME_UNSUPPORTED);
 const uint32_t dependency=w(attacker->bytes+0x564);
 const unsigned stamp=half(attacker->bytes+0x640);
 const int em27=dependency==0x227&&attacker->original_vtable214==0x4479c8&&
    ((b->work->room==0x105&&stamp==2)||(b->work->room==0x101&&(stamp==0x1e||stamp==0x26)&&
      attacker->size>0x1650&&attacker->bytes[0x1650]==6));
 const int r102_em00=b->work->room==0x102&&dependency==0x200&&
    attacker->size>0x1650&&attacker->original_vtable214==0x4479c8&&
    ((stamp==11&&attacker->bytes[0x1650]==5)||(stamp==3&&attacker->bytes[0x1650]==1)||
     (stamp==7&&attacker->size>=0x16c8&&!attacker->bytes[0x1650]&&w(attacker->bytes+0x16c4)==2));
 const int arena=r103_grab_attacker(b->work,attacker,0x200);
 if(!em27&&!r102_em00&&!arena&&(b->work->room!=0x100||stamp!=20||dependency!=0x200))
  return fail(s,0x11c348,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->hurt_phase){
  put(p+0x54c,0x40a00000);
  put(p+0x250,w(p+0x250)|0x10000u);
  if(!b->solids||gh_collision_solid_set_enabled_001351d8(b->solids,&b->player->view,0,&pc))
   return fail(s,pc?pc:0x1351d8,GH_PLAYER_FRAME_UNSUPPORTED);
  b->init->enemies[0x53c]=2;
  if(p[0x2f6]>1)return GH_PLAYER_FRAME_OK;
  s->hurt_phase=p[0x2f6]?FOLLOW:PLACE;
 }
 if(s->hurt_phase==PLACE){
  gh_player_frame_status status=place_grab_001299f0(s,attacker,0xbf01bda5u,0x3e0ed917u);
  if(status)return status;
  s->hurt_phase=BIND;
 }
 if(s->hurt_phase==BIND){
  const gh_object_view *resource=&b->grab_resource;
  if(!b->initialize_motion||!resource->bytes||resource->size<0x528)
   return fail(s,0x23a300,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
    .resource=resource->bytes,.resource_bytes=resource->size,.motion_offset=w(resource->bytes+0x520),
    .sequence_candidate_offset=w(resource->bytes+0x524),.release_previous_sequence=!em27&&!r102_em00&&!arena,.rate=b->rate};
  gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
      GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  put(p+0x15b0,1);
  s->hurt_phase=AUX;
 }
 if(s->hurt_phase==AUX){
  if(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698))
   return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
  sh(p+0x56e,15);
  p[0x2f6]=1;
  s->hurt_phase=FOLLOW;
 }
 if(s->hurt_phase==FOLLOW){
  const uint16_t remaining=half(p+0x56e);
  if(remaining){
   float position[3],offset[3];
   memcpy(position,b->player->view.alias_f0,12);
   memcpy(offset,attacker->bytes+0x550,12);
   for(unsigned i=0;i<3;i++){
    volatile float next=position[i]+offset[i];
    if(!isfinite(next))return fail(s,0x11c478,GH_PLAYER_FRAME_NUMERIC);
    position[i]=next;
   }
   sh(p+0x56e,(uint16_t)(remaining-1));
   memcpy(b->player->view.alias_f0,position,12);
  }
  s->hurt_phase=CAMERA;
 }
 if(s->hurt_phase==CAMERA){
  if(!b->grab_camera)return fail(s,0x124ec0,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status status=b->grab_camera(b->motion_context,b->model,&pc);
  if(status)return fail(s,pc?pc:0x124ec0,status);
  put(p+0x15f4,w(p+0x15f4)|2u);
  s->hurt_phase=ADVANCE;
 }
 if(s->hurt_phase==ADVANCE){
  if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.work=b->work,.model=b->model,
    .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->hurt_phase=s->motion_ended?END:ROTATE;
 }
 if(s->hurt_phase==END){
  if((int16_t)half(p+0x54a)<=0)return fail(s,0x11c4f0,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
  if(status)return fail(s,pc?pc:0x124f60,status);
  put(p+0x15f4,w(p+0x15f4)&~2u);
  p[0x2f4]=1;p[0x2f5]=4;p[0x2f6]=p[0x2f7]=0;
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!(half(p+0x3ac)&1)||!w(p+0x15b0))return GH_PLAYER_FRAME_OK;
  put(p+0x15b0,0);
  s->hurt_phase=DAMAGE;
 }
 if(s->hurt_phase==DAMAGE){
  volatile float power=number(w(attacker->bytes+0x76c))*50.0f;
  if(!isfinite(power)||power<-2147483648.0f||power>=2147483648.0f)
   return fail(s,0x12c0f8,GH_PLAYER_FRAME_NUMERIC);
  gh_player_frame_status status=damage_hp_0012c0f8(s,(int32_t)power);
  if(status)return status;
  s->hurt_phase=RANK;
 }
 if(s->hurt_phase==RANK){
  gh_player_frame_status status=damage_score_001fa280(s,320);
  if(status)return status;
  s->hurt_phase=VOICE;
 }
 if(s->hurt_phase==VOICE){
  const int dead=(int16_t)half(p+0x54a)<=0;
  gh_player_frame_status status=hurt_sound(s,dead?0xd6:0xd7);
  if(status)return status;
  s->hurt_phase=dead?DEATH_SE:VIBRATE;
 }
 if(s->hurt_phase==DEATH_SE)return fail(s,0x2ca718,GH_PLAYER_FRAME_UNSUPPORTED);
 return hurt_vibration_0012c348(s,2);
}

static gh_player_frame_status em0a_grab_camera_exit(gh_game_player_frame *s,int escape)
{
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes;
 uint32_t pc=0;
 if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
 gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
 if(status)return fail(s,pc?pc:0x124f60,status);
 put(p+0x15f4,w(p+0x15f4)&~2u);
 p[0x2f4]=escape?0:1;
 p[0x2f5]=escape?0:4;
 p[0x2f6]=p[0x2f7]=0;
 return GH_PLAYER_FRAME_OK;
}

/* Original 11BAF8 paired catch, damage and escape animation phases. */
static gh_player_frame_status hurt_r103_em14_sphere_0011b3e8(gh_game_player_frame *s)
{
 enum { VOICE=1,CUE_FIRST,CUE_SECOND,VIBRATION,BIND,FACE,CAMERA,SAMPLE,EXIT,ROTATE,TRANSLATE };
 const gh_player_frame_bindings *b=&s->b;
 gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 if(b->work->room!=0x103||p[0x2f7]||p[0x2f6]>1)
  return fail(s,0x11b3e8,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->hurt_phase)s->hurt_phase=p[0x2f6]?CAMERA:VOICE;
 while(s->hurt_phase>=VOICE&&s->hurt_phase<=CUE_SECOND){
  static const uint16_t programs[]={0xcf,0x2d,0x32};
  const gh_player_frame_status status=hurt_sound(s,(int16_t)programs[s->hurt_phase-VOICE]);
  if(status)return status;
  s->hurt_phase++;
 }
 if(s->hurt_phase==VIBRATION){
  put(p+0x54c,0x40a00000u);
  const gh_player_frame_status status=hurt_vibration_0012c348(s,2);
  if(status)return status;
  s->hurt_phase=BIND;
 }
 if(s->hurt_phase==BIND){
  if(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698))
   return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
  if(b->aux_6a0||w(p+0x6a0))return fail(s,0x12b928,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!b->initialize_motion||!b->resource||b->resource_bytes<0x278||!b->rate)
   return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.work=b->work,.model=b->model,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate,
   .motion_offset=w(b->resource+0x270),.sequence_candidate_offset=w(b->resource+0x274),
   .start_frame_bits=0x41700000u,.blend_frames=2,.release_previous_sequence=1};
  const gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
    GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  s->hurt_phase=FACE;
 }
 if(s->hurt_phase==FACE){
  if(b->take_object){
   const gh_title_player_view *source=NULL;
   if(gh_hitarea_object_last_hit_actor(b->take_object,&source))
    return fail(s,0x11b4f8,GH_PLAYER_FRAME_UNSUPPORTED);
   if(source){
    if(!source->alias_f0||!actor->alias_f0)return fail(s,0x11b510,GH_PLAYER_FRAME_UNSUPPORTED);
    const float x=sub(number(w(source->alias_f0)),number(w(actor->alias_f0)));
    const float z=sub(number(w(source->alias_f0+8)),number(w(actor->alias_f0+8)));
    float yaw;
    if(gh_game_angle_001f8180(x,z,&yaw,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
     return fail(s,pc?pc:0x1f8180,GH_PLAYER_FRAME_UNSUPPORTED);
    sf(p+0x104,yaw);
   }
  }
  put(p+0x15b0,30);
  p[0x2f6]=1;
  s->hurt_phase=CAMERA;
 }
 if(s->hurt_phase==CAMERA){
  if(!b->grab_camera)return fail(s,0x124ec0,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_status status=b->grab_camera(b->motion_context,b->model,&pc);
  if(status)return fail(s,pc?pc:0x124ec0,status);
  put(p+0x15f4,w(p+0x15f4)|2u);
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.work=b->work,.model=b->model,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  const gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->hurt_phase=s->motion_ended?EXIT:ROTATE;
 }
 if(s->hurt_phase==EXIT){
  const gh_player_frame_status status=em0a_grab_camera_exit(s,1);
  if(status)return status;
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
  return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
 return GH_PLAYER_FRAME_OK;
}

/* Original paired receivers at 11BAF8 and 11BF20. */
static gh_player_frame_status hurt_paired_grab(gh_game_player_frame *s)
{
 enum { CUE=1, PLACE, BIND, AUX, FOLLOW, ESCAPE_SCORE, CAMERA, SAMPLE,
   EXIT_CAMERA, ROTATE, TRANSLATE, TIMER, DAMAGE_SCORE, DAMAGE, DAMAGE_VOICE,
   DEATH_SE, DEATH_SCORE, VIBRATE, ESCAPE_INPUT, ESCAPE_CAMERA };
 const gh_player_frame_bindings *b=&s->b;
 const gh_title_player_view *actor=&b->player->view,*attacker=b->grab_attacker;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 const int r104=s->state_entry==0x11bf20;
 if(!actor->alias_f0||!attacker||!attacker->bytes||attacker->size<=0x1650||
    attacker->original_vtable214!=0x4479c8||w(attacker->bytes+0x564)!=(r104?0x250u:0x20au))
  return fail(s,s->state_entry,GH_PLAYER_FRAME_UNSUPPORTED);
 const uint8_t *source=attacker->bytes;
 const int r100=b->work->room==0x100&&!half(source+0x640)&&source[0x1650]==3;
 const int r102=b->work->room==0x102&&half(source+0x640)==2&&source[0x1650]==5&&
    attacker->size>=0x16c8&&!w(source+0x16c4);
 const int em03=b->work->room==0x104&&attacker->size>0x1651&&
    half(source+0x2fe)==0x203&&!half(source+0x640)&&w(source+0x1644)==0x80000u&&
    !source[0x1650]&&!source[0x1651];
 if(r104?!em03:(!r100&&!r102&&!r103_grab_attacker(b->work,attacker,0x20a)))
  return fail(s,s->state_entry,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->hurt_phase){
  put(p+0x54c,0x40a00000u);
  put(p+0x250,w(p+0x250)|0x10000u);
  if(!b->solids||gh_collision_solid_set_enabled_001351d8(b->solids,&b->player->view,0,&pc))
   return fail(s,pc?pc:0x1351d8,GH_PLAYER_FRAME_UNSUPPORTED);
  b->init->enemies[0x53c]=2;
  s->hurt_sample_phase=p[0x2f6];
  switch(s->hurt_sample_phase){
   case 0:s->hurt_phase=CUE;break;
   case 1:s->hurt_phase=FOLLOW;break;
   case 2:s->hurt_phase=BIND;break;
   case 4:s->hurt_phase=ESCAPE_SCORE;break;
   case 3:case 5:s->hurt_phase=CAMERA;break;
   default:return GH_PLAYER_FRAME_OK;
  }
 }
 if(s->hurt_phase==CUE){
  gh_player_frame_status status=hurt_sound(s,0xcf);
  if(status)return status;
  s->hurt_phase=PLACE;
 }
 if(s->hurt_phase==PLACE){
  const uint32_t x_offset=r104?0xbd32fec5u:0xbe2b020cu;
  const uint32_t z_offset=r104?0x3f9d0e56u:0x3f8240b8u;
  gh_player_frame_status status=place_grab_001299f0(s,attacker,x_offset,z_offset);
  if(status)return status;
  s->hurt_phase=BIND;
 }
 if(s->hurt_phase==ESCAPE_SCORE){
  gh_player_frame_status status=damage_score_001fa280(s,-100);
  if(status)return status;
  s->hurt_phase=BIND;
 }
 if(s->hurt_phase==BIND){
  const size_t row=(r104?0x1a7cu:0xc80u)+4u*s->hurt_sample_phase;
  const gh_object_view *resource=&b->grab_resource;
  if(!b->initialize_motion||!resource->bytes||!resource->file_extent_known||
     resource->file_extent<row+8||resource->file_extent>resource->size)
   return fail(s,0x23a300,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=resource->bytes,.resource_bytes=resource->size,.motion_offset=w(resource->bytes+row),
   .sequence_candidate_offset=w(resource->bytes+row+4),.release_previous_sequence=1,.rate=b->rate,
   .blend_frames=s->hurt_sample_phase?3:0};
  gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
    GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  if(s->hurt_sample_phase){
   if(s->hurt_sample_phase==2)put(p+0x15b0,r104?5:25);
   ++p[0x2f6];
   s->hurt_phase=CAMERA;
  }else s->hurt_phase=AUX;
 }
 if(s->hurt_phase==AUX){
  if(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698))
   return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
  if(b->aux_6a0||w(p+0x6a0))return fail(s,0x12b928,GH_PLAYER_FRAME_UNSUPPORTED);
  put(p+0x15b0,30);
  sh(p+0x56e,15);
  p[0x2f6]=1;
  s->hurt_phase=FOLLOW;
 }
 if(s->hurt_phase==FOLLOW){
  const uint16_t frames=half(p+0x56e);
  if(frames){
   float xyz[3],delta[3];
   memcpy(xyz,actor->alias_f0,12);
   memcpy(delta,attacker->bytes+0x550,12);
   for(unsigned i=0;i<3;i++){
    volatile float next=xyz[i]+delta[i];
    if(!isfinite(next))return fail(s,0x11bca0,GH_PLAYER_FRAME_NUMERIC);
    xyz[i]=next;
   }
   sh(p+0x56e,(uint16_t)(frames-1));
   memcpy(actor->alias_f0,xyz,12);
  }
  s->hurt_phase=CAMERA;
 }
 if(s->hurt_phase==CAMERA){
  if(!b->grab_camera)return fail(s,0x124ec0,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status status=b->grab_camera(b->motion_context,b->model,&pc);
  if(status)return fail(s,pc?pc:0x124ec0,status);
  put(p+0x15f4,w(p+0x15f4)|2u);
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.work=b->work,.model=b->model,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->hurt_phase=ROTATE;
  if(s->motion_ended){
   if(p[0x2f6]==1)p[0x2f6]=2;
   else if(p[0x2f6]==3&&(int16_t)half(p+0x54a)<1)b->work->flags|=8;
   else s->hurt_phase=EXIT_CAMERA;
  }
 }
 if(s->hurt_phase==EXIT_CAMERA){
  gh_player_frame_status status=em0a_grab_camera_exit(s,s->hurt_sample_phase>=4);
  if(status)return status;
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  if(s->hurt_sample_phase<2)return GH_PLAYER_FRAME_OK;
  s->hurt_phase=s->hurt_sample_phase<4?TIMER:ESCAPE_INPUT;
 }
 if(s->hurt_phase==ESCAPE_INPUT){
  gh_player_frame_status status=hurt_input_00123938(s);
  if(status)return status;
  if(!(s->input_result||(half(p+0x3ac)&0x20))||(!r104&&!(w(p+0x15f4)&2u)))
   return GH_PLAYER_FRAME_OK;
  s->hurt_phase=ESCAPE_CAMERA;
 }
 if(s->hurt_phase==ESCAPE_CAMERA)return em0a_grab_camera_exit(s,1);
 if(s->hurt_phase==TIMER){
  const uint32_t remaining=w(p+0x15b0);
  if(remaining)put(p+0x15b0,remaining-1);
  if(remaining!=1)return GH_PLAYER_FRAME_OK;
  s->hurt_phase=DAMAGE_SCORE;
 }
 if(s->hurt_phase==DAMAGE_SCORE){
  gh_player_frame_status status=damage_score_001fa280(s,320);
  if(status)return status;
  s->hurt_phase=DAMAGE;
 }
 if(s->hurt_phase==DAMAGE){
  volatile float power=number(w(attacker->bytes+0x76c))*(r104?50.0f:30.0f);
  if(!isfinite(power)||power<-2147483648.0f||power>=2147483648.0f)
   return fail(s,0x12c0f8,GH_PLAYER_FRAME_NUMERIC);
  gh_player_frame_status status=damage_hp_0012c0f8(s,(int32_t)power);
  if(status)return status;
  s->hurt_phase=DAMAGE_VOICE;
 }
 if(s->hurt_phase==DAMAGE_VOICE){
  const int dead=(int16_t)half(p+0x54a)<1;
  gh_player_frame_status status=hurt_sound(s,dead?0xd6:0xd7);
  if(status)return status;
  s->hurt_phase=dead?DEATH_SE:VIBRATE;
 }
 if(s->hurt_phase==DEATH_SE){
  if(!r104||!b->death_fade)return fail(s,0x2ca718,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status status=b->death_fade(b->damage_context,&pc);
  if(status)return fail(s,pc?pc:0x2ca718,status);
  s->hurt_phase=DEATH_SCORE;
 }
 if(s->hurt_phase==DEATH_SCORE){
  gh_player_frame_status status=damage_score_001fa280(s,1000);
  if(status)return status;
  sh(p+0x434,half(p+0x434)|8u);
  s->hurt_phase=VIBRATE;
 }
 if(s->hurt_phase==VIBRATE)return hurt_vibration_0012c348(s,2);
 return fail(s,s->state_entry,GH_PLAYER_FRAME_UNSUPPORTED);
}

/* Original R103 placement for Gene, the EM15 owner and its live EM14 partner. */
static gh_player_frame_status place_r103_boss_throw(gh_game_player_frame *s)
{
 const gh_player_frame_bindings *b=&s->b;
 const gh_title_player_view *actor=&b->player->view,*attacker=b->grab_attacker;
 gh_game_title_player *em14=b->init->enemy_r103_boss[0];
 gh_title_player_view *partner=em14?&em14->view:NULL;
 uint8_t *p=actor->bytes;
 if(!actor->alias_f0||!attacker->alias_f0||
    (partner&&(!partner->bytes||partner->size<0x642||!partner->alias_f0||
     half(partner->bytes+0x2fe)!=0x204||w(partner->bytes+0x564)!=0x214||
     half(partner->bytes+0x640)||partner->original_vtable214!=0x4479c8)))
  return fail(s,0x11d4b0,GH_PLAYER_FRAME_UNSUPPORTED);
 if(partner&&(int16_t)half(partner->bytes+0x54a)<1)partner=NULL;
 const float yaw=number(w(p+0x104));
 float sn,cs,player_xyz[4],attacker_xyz[4],partner_xyz[4],opposite;
 uint32_t pc=0;
 if(gh_sdk_rotation_sincos_reference_0030a5d0(yaw,&sn,&cs,GH_STATIC_BINARY32_REFERENCE,&pc))
  return fail(s,pc?pc:0x147988,GH_PLAYER_FRAME_UNSUPPORTED);
 volatile float reversed=yaw+number(0x40490fdb);
 if(gh_id_angle_reference_001f7d48(reversed,&opposite))
  return fail(s,0x1f7d48,GH_PLAYER_FRAME_NUMERIC);
 const float stage_basis[16]={cs,0,-sn,0,0,1,0,0,sn,0,cs,0,5.5f,0,-94.5f,1};
 const float stage_offset[3]={0,0,4};
 if(gh_scene_at_apply_world_001508d8(stage_basis,stage_offset,player_xyz))
  return fail(s,0x30a2b0,GH_PLAYER_FRAME_NUMERIC);
 const float pair_basis[16]={cs,0,-sn,0,0,1,0,0,sn,0,cs,0,
    player_xyz[0],player_xyz[1],player_xyz[2],1};
 const float attacker_offset[3]={number(0xbf1ae148),0,number(0xbd95b574)};
 const float partner_offset[3]={number(0xc009b08a),0,number(0xc0833eab)};
 if(gh_scene_at_apply_world_001508d8(pair_basis,attacker_offset,attacker_xyz)||
    (partner&&gh_scene_at_apply_world_001508d8(pair_basis,partner_offset,partner_xyz)))
  return fail(s,0x30a2b0,GH_PLAYER_FRAME_NUMERIC);
 memcpy(actor->alias_f0,player_xyz,12);
 memcpy(attacker->alias_f0,attacker_xyz,12);
 sf(attacker->bytes+0x104,opposite);
 if(partner){
  volatile float partner_yaw=yaw+number(0x3fc90fdb);
  memcpy(partner->alias_f0,partner_xyz,12);
  sf(partner->bytes+0x104,partner_yaw);
  memcpy(partner->bytes+0x2f4,(const uint8_t[4]){0,0x52,2,0},4);
 }
 p[0x2f7]=partner!=NULL;
 return GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status hurt_r103_boss_throw_0011d4b0(gh_game_player_frame *s)
{
 enum { PREPARE=1, PLACE, BIND, SCORE, CAMERA, SAMPLE, EXIT_CAMERA, ROTATE,
    TRANSLATE, IMPACT, DAMAGE, SOUND, DEATH, DEATH_SCORE, VIBRATE };
 const gh_player_frame_bindings *b=&s->b;
 gh_title_player_view *actor=&b->player->view;
 const gh_title_player_view *attacker=b->grab_attacker;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 if(b->work->room!=0x103||!attacker||!attacker->bytes||attacker->size<0x16c8||
    attacker->original_vtable214!=0x4479c8||half(attacker->bytes+0x2fe)!=0x204||
    w(attacker->bytes+0x564)!=0x215||half(attacker->bytes+0x640)!=1||attacker->bytes[0x1650]!=15||
    p[0x2f6]>1||p[0x2f7]>1)
  return fail(s,0x11d4b0,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->hurt_phase)s->hurt_phase=PREPARE;
 if(s->hurt_phase==PREPARE){
  if(!b->solids||gh_collision_solid_set_enabled_001351d8(b->solids,actor,0,&pc))
   return fail(s,pc?pc:0x1351d8,GH_PLAYER_FRAME_UNSUPPORTED);
  put(p+0x15f4,w(p+0x15f4)|0x2000u);
  put(p+0x250,w(p+0x250)|0x10000u);
  put(p+0x54c,0x40a00000u);
  b->init->enemies[0x53c]=2;
  s->hurt_phase=p[0x2f6]?CAMERA:PLACE;
 }
 if(s->hurt_phase==PLACE){
  if(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698))
   return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
  if(b->aux_6a0||w(p+0x6a0))return fail(s,0x12b928,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_status status=place_r103_boss_throw(s);
  if(status)return status;
  s->hurt_phase=BIND;
 }
 if(s->hurt_phase==BIND){
  const size_t row=p[0x2f7]?0x1bb0:0x1bb8;
  const gh_object_view *resource=&b->grab_resource;
  if(!b->initialize_motion||!resource->bytes||!resource->file_extent_known||
     resource->file_extent<row+8||resource->file_extent>resource->size)
   return fail(s,0x23a300,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.work=b->work,.model=b->model,
   .resource=resource->bytes,.resource_bytes=resource->size,.rate=b->rate,
   .motion_offset=w(resource->bytes+row),.sequence_candidate_offset=w(resource->bytes+row+4)};
  const gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
    GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  s->hurt_phase=SCORE;
 }
 if(s->hurt_phase==SCORE){
  const gh_player_frame_status status=damage_score_001fa280(s,320);
  if(status)return status;
  put(p+0x15b0,1);
  p[0x2f6]=1;
  s->hurt_phase=CAMERA;
 }
 if(s->hurt_phase==CAMERA){
  if(!b->grab_camera)return fail(s,0x124ec0,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_status status=b->grab_camera(b->motion_context,b->model,&pc);
  if(status)return fail(s,pc?pc:0x124ec0,status);
  put(p+0x15f4,w(p+0x15f4)|2u);
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.work=b->work,.model=b->model,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  const gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->hurt_phase=ROTATE;
  if(s->motion_ended){
   if((int16_t)half(p+0x54a)<1)b->work->flags|=8;
   else s->hurt_phase=EXIT_CAMERA;
  }
 }
 if(s->hurt_phase==EXIT_CAMERA){
  const gh_player_frame_status status=em0a_grab_camera_exit(s,0);
  if(status)return status;
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!(half(p+0x3ac)&1u)||!w(p+0x15b0))return GH_PLAYER_FRAME_OK;
  put(p+0x15b0,0);
  s->hurt_phase=DAMAGE;
 }
 if(s->hurt_phase==DAMAGE){
  volatile float power=number(w(attacker->bytes+0x76c))*(p[0x2f7]?40.0f:25.0f);
  if(!isfinite(power)||power<0||power>=2147483648.0f)
   return fail(s,0x12c0f8,GH_PLAYER_FRAME_NUMERIC);
  const gh_player_frame_status status=damage_hp_0012c0f8(s,(int32_t)power);
  if(status)return status;
  s->hurt_phase=SOUND;
 }
 if(s->hurt_phase==SOUND){
  const int dead=(int16_t)half(p+0x54a)<1;
  const gh_player_frame_status status=hurt_sound(s,dead?0xd6:0xd7);
  if(status)return status;
  s->hurt_phase=dead?DEATH:VIBRATE;
 }
 if(s->hurt_phase==DEATH){
  if(!b->death_fade)return fail(s,0x2ca718,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_status status=b->death_fade(b->damage_context,&pc);
  if(status)return fail(s,pc?pc:0x2ca718,status);
  s->hurt_phase=DEATH_SCORE;
 }
 if(s->hurt_phase==DEATH_SCORE){
  const gh_player_frame_status status=damage_score_001fa280(s,1000);
  if(status)return status;
  sh(p+0x434,half(p+0x434)|8u);
  s->hurt_phase=VIBRATE;
 }
 if(s->hurt_phase==VIBRATE)return hurt_vibration_0012c348(s,2);
 return fail(s,0x11d4b0,GH_PLAYER_FRAME_UNSUPPORTED);
}

static gh_player_frame_status player_turn_0010c930(gh_game_player_frame *s)
{
 enum { BIND=1, TURN, SAMPLE, ROTATE, TRANSLATE, INPUT, DOOR };
 const gh_player_frame_bindings *b=&s->b;
 const gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 if(!s->hurt_phase){
  if((int16_t)half(p+0x54a)<1)sh(p+0x54a,1);
  put(p+0x5a8,0x3fc00000u);
  put(p+0x444,(half(p+0x434)&4u)?0xbfc00000u:0x3fc00000u);
  if(p[0x2f6]==0){
   sh(p+0x5e0,0);
   sh(p+0x5e2,0);
   if((int8_t)p[0x648]>10)p[0x648]=10;
   s->hurt_phase=BIND;
  }else s->hurt_phase=p[0x2f6]==1?TURN:INPUT;
 }
 if(s->hurt_phase==BIND){
  if(!b->auxiliary_bindings_known||b->aux_6a0||w(p+0x6a0)||
     !b->initialize_motion||!b->resource||b->resource_bytes<0x68)
   return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.motion_offset=w(b->resource+0x60),
   .sequence_candidate_offset=w(b->resource+0x64),.blend_frames=10,.release_previous_sequence=1,.rate=b->rate};
  gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
      GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  ++p[0x2f6];
  s->hurt_phase=TURN;
 }
 if(s->hurt_phase==TURN){
  if(p[0x61f]){
   const float stick=number(w(p+0x608)),direction=number(w(p+0x604)),yaw=number(w(p+0x104));
   uint32_t maximum_bits;
   if(!elf_word(b,0x3bd478,&maximum_bits)||!isfinite(stick)||!isfinite(direction)||!isfinite(yaw))
    return fail(s,0x10ca40,GH_PLAYER_FRAME_NUMERIC);
   const float maximum=number(maximum_bits);
   if(!isfinite(maximum)||maximum<0)return fail(s,0x10ca40,GH_PLAYER_FRAME_NUMERIC);
   const float remaining=sub(sub(number(0x40490fdb),stick),number(0x3f060a92));
   volatile float candidate=(remaining>0?remaining:0)*0.4f;
   const float turn=maximum<candidate?maximum:candidate;
   float result;
   volatile float sum=yaw+(direction<0?-turn:turn);
   if(gh_id_angle_reference_001f7d48(sum,&result))
    return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);
   sf(p+0x104,result);
  }
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=INPUT;
 }
 if(s->hurt_phase==INPUT){
  if(!b->idle_input||!b->idle_input_bindings)return fail(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_bindings *source=&b->idle_input_bindings->frame;
  if(source->player!=b->player||source->init!=b->init||source->work!=b->work||
     source->current_player!=b->current_player)return fail(s,0x123938,GH_PLAYER_FRAME_ARGUMENT);
  if(!s->input_started){
   gh_player_frame_status status=gh_player_idle_input_begin(b->idle_input);
   if(status)return fail(s,0x123938,status);
   s->input_started=1;
  }
  gh_player_frame_status status=gh_game_player_idle_input_00123938(b->idle_input,
    b->idle_input_bindings,&s->input_result);
  if(status)return fail(s,b->idle_input->frontier?b->idle_input->frontier:0x123938,status);
  s->hurt_phase=DOOR;
 }
 if(s->input_result)return GH_PLAYER_FRAME_OK;
 gh_player_frame_status status=gh_game_player_idle_cleanup_0012bfb8(b->idle_input);
 return status?fail(s,b->idle_input->frontier?b->idle_input->frontier:0x12bfb8,status):GH_PLAYER_FRAME_OK;
}

/* Original PL00 00/04 quick turn at 10CB10. */
static gh_player_frame_status quick_turn_0010cb10(gh_game_player_frame *s)
{
 enum { BIND=1, SAMPLE, EXIT, ROTATE, TRANSLATE, INPUT };
 const gh_player_frame_bindings *b=&s->b;
 gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 if(!s->hurt_phase){
  uint32_t rate;
  if(b->work->room!=0x101||p[0x2f7]||!elf_word(b,0x3bd488,&rate))
   return fail(s,0x10cb10,GH_PLAYER_FRAME_UNSUPPORTED);
  if((int16_t)half(p+0x54a)<1)sh(p+0x54a,1);
  if(p[0x2f6]>1)return GH_PLAYER_FRAME_OK;
  put(p+0x15f0,rate);
  s->hurt_phase=p[0x2f6]?SAMPLE:BIND;
 }
 if(s->hurt_phase==BIND){
  if(!b->auxiliary_bindings_known||b->aux_6a0||w(p+0x6a0)||!b->initialize_motion||
     !b->model||!b->resource||b->resource_bytes<0x84||!b->rate)
   return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  sh(p+0x5e0,0);
  sh(p+0x5e2,0);
  p[0x648]=0;
  put(p+0x640,0);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate,
   .motion_offset=w(b->resource+0x7c),.sequence_candidate_offset=w(b->resource+0x80),
   .blend_frames=5,.release_previous_sequence=1};
  const gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  p[0x2f6]=1;
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  const gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->hurt_phase=s->motion_ended?EXIT:ROTATE;
 }
 if(s->hurt_phase==EXIT){
  if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
  if(status)return fail(s,pc?pc:0x124f60,status);
  memset(p+0x2f4,0,4);
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=INPUT;
 }
 return hurt_input_00123938(s);
}

static gh_player_frame_status lateral_dodge_focus(gh_game_player_frame *s,int turn)
{
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes;
 const uint32_t handle=w(p+0x640);
 const gh_player_idle_input_bindings *input=b->idle_input_bindings;
 if(!input||input->targets.player!=&b->player->view)
  return fail(s,0x124540,GH_PLAYER_FRAME_UNSUPPORTED);
 uint32_t pc=0;
 gh_player_target target={.handle=handle};
 gh_player_target_status status;
 if(handle){
  if(!input->targets.resolve)return fail(s,0x124e68,GH_PLAYER_FRAME_UNSUPPORTED);
  status=input->targets.resolve(input->targets.context,handle,&target.view,&pc);
  if(status)return fail(s,pc?pc:0x124e68,GH_PLAYER_FRAME_UNSUPPORTED);
 }
 if(!turn||!handle){
  status=gh_player_dodge_target_00124540(&input->targets,b->init,&target,&pc);
  if(status)return fail(s,pc?pc:0x124540,GH_PLAYER_FRAME_UNSUPPORTED);
 }
 if(turn&&target.handle){
  if(!target.view.bytes||target.view.original_vtable214!=0x4479c8||
     b->player->view.alias_f0!=p+0xb0)
   return fail(s,0x124e68,GH_PLAYER_FRAME_UNSUPPORTED);
  float origin[3],position[3],delta;
  memcpy(origin,b->player->view.alias_f0,sizeof origin);
  status=gh_enemy_target_position_00275978(&target.view,position);
  if(status)return fail(s,0x275978,GH_PLAYER_FRAME_UNSUPPORTED);
  const float yaw=number(w(p+0x104));
  volatile float cap=number(0x3e490fdb)*number(w((const uint8_t*)b->work+0x55c));
  status=gh_player_target_angle_002da038(b->elf,b->elf_bytes,origin,position,yaw,cap,&delta,&pc);
  if(status)return fail(s,pc?pc:0x2da038,GH_PLAYER_FRAME_UNSUPPORTED);
  volatile float sum=yaw+delta;
  float wrapped;
  if(gh_id_angle_reference_001f7d48(sum,&wrapped))
   return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);
  sf(p+0x104,wrapped);
 }
 return GH_PLAYER_FRAME_OK;
}

/* Original Roulette accepts unlocked save slots plus the unconditional row 0B.
 * The stored icon is the original zero-based TIM2 entry in the language bundle. */
static int roulette_unlocked(gh_core_save_view save,unsigned id)
{
 if(id==0xb)return 1;
 for(unsigned i=0;i<10;i++)if(save.bytes[0x98+i]==id)return 1;
 return 0;
}

static gh_player_frame_status roulette_open_00125048(gh_game_player_frame *s)
{
 static const uint32_t source_address[]={0x3bd4a8,0x3bd4f8,0x3bd518};
 static const unsigned source_count[]={19,8,2};
 static const uint8_t icon[]={0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,17,16,18,20,21,22,
     23,24,25,26,30,27,19,28,29,1};
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes;
 const gh_player_target_bindings *targets=&b->idle_input_bindings->targets;
 gh_game_action_buttons *manager=b->idle_input_bindings->action_buttons;
 if(!b->save.bytes||b->save.size<GH_CORE_SAVE_BYTES||!manager)
  return fail(s,0x125048,GH_PLAYER_FRAME_ARGUMENT);
 const uint8_t *work=(const uint8_t *)b->work;
 unsigned rank;
 if(gh_core_save_game_level_001fa010(b->save,b->elf,b->elf_bytes,&rank))
  return fail(s,0x1fa010,GH_PLAYER_FRAME_UNSUPPORTED);
 /* This original ordinary-rank encounter has no bonus, alternate costume,
  * special unlock or debug-injected commands. */
 if((w(work+0x58c)&0x80000u)||(w(work+0x594)&2u)||rank>2||(b->save.bytes[0xae]&1u))
  return fail(s,0x125048,GH_PLAYER_FRAME_UNSUPPORTED);
 gh_player_target target={.handle=w(p+0x640)};
 uint32_t pc=0;
 float squared=999800.06f;
 int target_alive=0;
 if(target.handle){
  if(!targets->resolve||targets->resolve(targets->context,target.handle,&target.view,&pc)||
     !target.view.bytes||target.view.size<0x568)
   return fail(s,pc?pc:0x124e68,GH_PLAYER_FRAME_UNSUPPORTED);
  float destination[3],origin[3];
  if(gh_player_throw_target_position(targets,&target,destination,&pc))
   return fail(s,pc?pc:0x275978,GH_PLAYER_FRAME_UNSUPPORTED);
  memcpy(origin,b->player->view.alias_f0,sizeof origin);
  const float x=sub(destination[0],origin[0]),z=sub(destination[2],origin[2]);
  const volatile float xx=x*x,zz=z*z;
  squared=add(xx,zz);
  if(!isfinite(squared))return fail(s,0x1f7eb0,GH_PLAYER_FRAME_NUMERIC);
  target_alive=(int16_t)half(target.view.bytes+0x54a)>0;
 }
 unsigned stock=0;
 const unsigned slots=b->save.bytes[0x156]<6?b->save.bytes[0x156]:6;
 for(unsigned i=0;i<slots;i++)stock+=b->save.bytes[0xa8+i]!=0;
 gh_roulette_option rows[GH_ACTION_ROULETTE_LIMIT];
 unsigned used=0;
 gh_game_mt random=b->init->random;
 for(unsigned group=0;group<3;group++){
  uint8_t commands[19];
  unsigned count=0;
  for(unsigned i=0;i<source_count[group];i++){
   uint32_t id;
   if(!elf_word(b,source_address[group]+i*4,&id)||id>=sizeof(icon))
    return fail(s,0x125048,GH_PLAYER_FRAME_ARGUMENT);
   if(roulette_unlocked(b->save,id))commands[count++]=(uint8_t)id;
  }
  /* The original performs eleven passes over group0, ten over groups1/2. */
  for(unsigned pass=0;pass<(group?10u:11u);pass++){
   for(unsigned i=0;i<count;i++){
    uint32_t draw;
    if(gh_game_mt_next_0030f348(&random,&draw))
     return fail(s,0x31cfe0,GH_PLAYER_FRAME_ARGUMENT);
    const unsigned j=draw%count;
    const uint8_t saved=commands[i];
    commands[i]=commands[j];commands[j]=saved;
   }
  }
  for(unsigned i=0;i<count;i++){
   const unsigned id=commands[i];
   const uint32_t free_range=(1u<<0)|(1u<<6)|(1u<<8)|(1u<<9)|(1u<<11)|
       (1u<<13)|(1u<<14)|(1u<<23)|(1u<<24)|(1u<<29)|(1u<<30);
   const uint32_t cost2=(1u<<5)|(1u<<8)|(1u<<10)|(1u<<12)|(1u<<17)|(1u<<23)|(1u<<28)|(1u<<30);
   const uint32_t cost3=(1u<<6)|(1u<<9)|(1u<<24);
   if(id==25)return fail(s,0x125b84,GH_PLAYER_FRAME_UNSUPPORTED); /* Original two-way line checks. */
   const uint32_t bit=1u<<id;
   const int needs_target=!(free_range&bit);
   const int no_target=needs_target&&(!target_alive||squared>25.0f);
   const int no_stock=((cost2&bit)&&stock<2)||((cost3&bit)&&stock<3);
   const int full_hp=(id==8||id==9)&&half(p+0x54a)>=half(p+0x548);
   const int full_heat=(id==0||id==30)&&number(w(b->init->heat+4))>=number(w(b->init->heat));
   if(used>=GH_ACTION_ROULETTE_LIMIT)return fail(s,0x125048,GH_PLAYER_FRAME_ARGUMENT);
   rows[used++]=(gh_roulette_option){icon[id],(uint16_t)id,(uint8_t)(no_target||no_stock||full_hp||full_heat)};
  }
 }
 uint32_t handle;
 const gh_action_set_status status=gh_action_roulette_open_001f7900(manager,rows,used,
    (const float *)(const void *)(p+0x80),&handle,&pc);
 if(status)return fail(s,pc?pc:0x1f7900,GH_PLAYER_FRAME_UNSUPPORTED);
 b->init->random=random;
 put(p+0x165c,handle);
 return GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status roulette_restore_camera(gh_game_player_frame *s)
{
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes;
 if(!s->idle_end_prefix_done&&(w(p+0x15f4)&2u)){
  put(p+0x15f4,w(p+0x15f4)&~2u);
  s->idle_end_prefix_done=1;
 }
 if(!s->idle_end_prefix_done)return GH_PLAYER_FRAME_OK;
 if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
 uint32_t pc=0;
 const gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
 return status?fail(s,pc?pc:0x124f60,status):GH_PLAYER_FRAME_OK;
}

/* PL00 00/2B calls 114608 while the original Roulette animation is active. */
static gh_player_frame_status roulette_00114608(gh_game_player_frame *s)
{
 enum { CAMERA=1, BIND, TRACK, MARK, LIGHT, CHECK_WHEEL, WHEEL_SOUND, WHEEL_RATE, WHEEL_SPARK, SAMPLE, ROTATE, TRANSLATE, INPUT, END_CAMERA, END_ABORT, INPUT_ABORT, INPUT_CAMERA };
 const gh_player_frame_bindings *b=&s->b;
 gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes;
 const gh_player_idle_input_bindings *input=b->idle_input_bindings;
 uint32_t pc=0;
 if((b->work->room!=0x103&&b->work->room!=0x104)||p[0x2f7]||p[0x2f6]>1)
  return fail(s,0x114608,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!input||input->targets.player!=actor||!actor->alias_f0)
  return fail(s,0x124540,GH_PLAYER_FRAME_ARGUMENT);
 const gh_player_target_bindings *targets=&input->targets;
 if(!s->hurt_phase){
  if(!p[0x2f6]){
   sh(p+0x5e0,0);sh(p+0x5e2,0);
   put(p+0x640,0);p[0x648]=0;
   gh_player_target target={0};
   const gh_player_target_status status=gh_player_roulette_target_00124540(targets,b->init,&target,&pc);
   if(status)return fail(s,pc?pc:0x124540,GH_PLAYER_FRAME_UNSUPPORTED);
   s->hurt_phase=CAMERA;
  }else s->hurt_phase=TRACK;
 }
 gh_player_target target={.handle=w(p+0x640)};
 float origin[3],destination[3];
 if(target.handle&&s->hurt_phase<=MARK){
  if(!targets->resolve)return fail(s,0x124e68,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_target_status status=targets->resolve(targets->context,target.handle,&target.view,&pc);
  if(status)return fail(s,pc?pc:0x124e68,GH_PLAYER_FRAME_UNSUPPORTED);
  if(gh_player_throw_target_position(targets,&target,destination,&pc))
   return fail(s,pc?pc:0x275978,GH_PLAYER_FRAME_UNSUPPORTED);
  memcpy(origin,actor->alias_f0,sizeof origin);
 }
 if(s->hurt_phase==CAMERA){
  if(target.handle){
   if(!b->grab_camera)return fail(s,0x124ec0,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_frame_status status=b->grab_camera(b->motion_context,b->model,&pc);
   if(status)return fail(s,pc?pc:0x124ec0,status);
   put(p+0x15f4,w(p+0x15f4)|2u);
  }
  s->hurt_phase=BIND;
 }
 if(s->hurt_phase==BIND){
  size_t row=0x220;
  if(target.handle){
   const float x=sub(destination[0],origin[0]),z=sub(destination[2],origin[2]);
   volatile float xx=x*x,zz=z*z;
   const float squared=add(xx,zz);
   if(!isfinite(squared))return fail(s,0x1f7eb0,GH_PLAYER_FRAME_NUMERIC);
   if(squared>64.0f)row=0x228;
  }
  if(!b->initialize_motion||!b->resource||b->resource_bytes<row+8||!b->rate)
   return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate,
   .motion_offset=w(b->resource+row),.sequence_candidate_offset=w(b->resource+row+4),
   .blend_frames=3,.release_previous_sequence=1};
  const gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
    GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  put(p+0x15b0,102);put(p+0x15bc,1);sh(p+0x568,5);++p[0x2f6];
  put(p+0x15b4,1);put(p+0x15b8,0);put(p+0x165c,0);put(p+0x161c,0);
  s->hurt_phase=TRACK;
 }
 if(s->hurt_phase==TRACK){
  if(target.handle){
   const float yaw=number(w(p+0x104));
   volatile float cap=number(0x3e490fdb)*number(w((const uint8_t *)b->work+0x55c));
   float turn;
   if(gh_player_target_angle_002da038(b->elf,b->elf_bytes,origin,destination,yaw,cap,&turn,&pc))
    return fail(s,pc?pc:0x2da038,GH_PLAYER_FRAME_UNSUPPORTED);
   float next;
   if(gh_id_angle_reference_001f7d48(add(yaw,turn),&next))
    return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);
   sf(p+0x104,next);
   p[0x648]=20;
   put(p+0x15f4,w(p+0x15f4)|0x800000u);
  }
  s->hurt_phase=MARK;
 }
 if(s->hurt_phase==MARK){
  if(target.handle&&!w(p+0x644)){
   gh_player_tail_target_slot *slot=b->target_644;
   if(!slot||!slot->known||slot->view||slot->expected_guest_token)
    return fail(s,0x12cc70,GH_PLAYER_FRAME_ARGUMENT);
   const gh_title_player_view *owner=NULL;
   for(unsigned i=0;i<2;i++){
    const gh_game_title_player *boss=b->init->enemy_r103_boss[i];
    if(boss&&boss->view.bytes==target.view.bytes)owner=&boss->view;
   }
   if(!owner&&b->work->room==0x104&&b->roulette_target_owner)
    owner=b->roulette_target_owner(b->effect_context,target.handle);
   if(!owner||owner->bytes!=target.view.bytes)return fail(s,0x12cc70,GH_PLAYER_FRAME_UNSUPPORTED);
   put(p+0x644,target.handle);
   slot->view=owner;
   slot->expected_guest_token=target.handle;
   s->pending_effect=(gh_player_frame_effect_request){.actor=owner,.kind=0x61,.mode=6,.selector=-1};
   s->hurt_phase=LIGHT;
  }else s->hurt_phase=CHECK_WHEEL;
 }
 if(s->hurt_phase==LIGHT){
  if(!b->effect_create)return fail(s,0x306528,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_status status=b->effect_create(b->effect_context,&s->pending_effect,&pc);
  if(status)return fail(s,pc?pc:0x306528,status);
  s->hurt_phase=CHECK_WHEEL;
 }
 if(s->hurt_phase==CHECK_WHEEL){
  if(half(p+0x3ac)&0x100u){
   if(w(p+0x15b4)&&!w(p+0x165c)){
    const gh_player_frame_status status=roulette_open_00125048(s);
    if(status)return status;
   }
   const uint32_t timer=w(p+0x15b0);
   if(timer){
    put(p+0x15b0,timer-1);
    s->hurt_phase=WHEEL_SOUND;
   }
   else s->hurt_phase=SAMPLE;
  }else s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==WHEEL_SOUND){
  if(!w(p+0x161c)){
   if(!b->damage_sound||!b->model)return fail(s,0x2cba48,GH_PLAYER_FRAME_UNSUPPORTED);
   s->damage_sound_request=(gh_motion_sound_request){.model=b->model,.actor=actor,.bank=0,.program=4};
   uint32_t voice=0;
   const gh_actor_motion_status status=b->damage_sound(b->damage_sound_context,
       &s->damage_sound_request,&voice,&pc);
   if(status)return fail(s,pc?pc:0x2cba48,status==GH_ACTOR_MOTION_UNSUPPORTED?
       GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
   put(p+0x161c,voice);
  }
  s->hurt_phase=WHEEL_RATE;
 }
 if(s->hurt_phase==WHEEL_RATE){
  if(!b->roulette_time_scale)return fail(s,0x292f38,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_status status=b->roulette_time_scale(b->effect_context,0x3cf5c28fu,&pc);
  if(status)return fail(s,pc?pc:0x292f38,status);
  put(p+0x5a8,0x3cf5c28fu);
  put(p+0x444,(half(p+0x434)&4u)?0xbcf5c28fu:0x3cf5c28fu);
  if(w(p+0x15bc)){
   put(p+0x15bc,0);
   if(w(p+0x15f4)&0x20u)s->hurt_phase=WHEEL_SPARK;
   else s->hurt_phase=SAMPLE;
  }else s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==WHEEL_SPARK){
  if(!b->effect_create)return fail(s,0x306528,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_effect_request request={.actor=actor,.kind=14,.mode=UINT32_MAX,.selector=-1};
  const gh_player_frame_status status=b->effect_create(b->effect_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x306528,status);
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.work=b->work,.model=b->model,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  int ended;
  const gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->hurt_phase=ended?END_CAMERA:ROTATE;
 }
 if(s->hurt_phase==END_CAMERA){
  const gh_player_frame_status status=roulette_restore_camera(s);
  if(status)return status;
  s->hurt_phase=END_ABORT;
 }
 if(s->hurt_phase==END_ABORT){
  if(!b->damage_abort)return fail(s,0x126770,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_status status=b->damage_abort(b->damage_context,b,&pc);
  if(status)return fail(s,pc?pc:0x126770,status);
  memset(p+0x2f4,0,4);
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  if(half(p+0x3ac)&0x10u)put(p+0x15f4,w(p+0x15f4)|0x400000u);
  s->hurt_phase=INPUT;
 }
 if(s->hurt_phase==INPUT){
  if(!b->idle_input)return fail(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!s->input_started){
   gh_player_frame_status status=gh_player_idle_input_begin(b->idle_input);
   if(status)return fail(s,0x123938,status);
   s->input_started=1;
  }
  const gh_player_frame_status status=gh_game_player_dodge_input_00123938(b->idle_input,input,&s->input_result);
  if(status)return fail(s,b->idle_input->frontier?b->idle_input->frontier:0x123938,status);
  if(s->input_result)s->hurt_phase=INPUT_ABORT;
 }
 if(s->hurt_phase==INPUT_ABORT){
  if(!b->damage_abort)return fail(s,0x126770,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_status status=b->damage_abort(b->damage_context,b,&pc);
  if(status)return fail(s,pc?pc:0x126770,status);
  s->hurt_phase=INPUT_CAMERA;
 }
 if(s->hurt_phase==INPUT_CAMERA)return roulette_restore_camera(s);
 return GH_PLAYER_FRAME_OK;
}

/* Original 112A48, ordinary command14 variant. */
static gh_player_frame_status roulette_shot_00112a48(gh_game_player_frame *s)
{
 enum { START_EFFECT=1, STOCK, BIND, SAMPLE, CAMERA_EXIT, ROTATE, TRANSLATE,
        PROJECTILE, RATE, INPUT, END_EFFECT, END_CAMERA };
 const gh_player_frame_bindings *b=&s->b;
 gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 if((b->work->room!=0x103&&b->work->room!=0x104)||p[0x2f7]||actor->size<0x1685)
  return fail(s,0x112a48,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->hurt_phase){
  put(p+0x54c,0x41f00000);
  uint8_t *enemies=b->init->enemies;
  if((int32_t)w(enemies+0x514)<2)put(enemies+0x514,2);
  enemies[0x53c]=2;
  if((int8_t)enemies[0x538]<2)enemies[0x538]=2;
  put(p+0x15f4,w(p+0x15f4)|0x200u);
  if(!p[0x2f6]){
   p[0x648]=0;put(p+0x640,0);
   sh(p+0x5e0,0);sh(p+0x5e2,0);
   p[0x1684]=1;
   s->hurt_phase=START_EFFECT;
  }else s->hurt_phase=p[0x2f6]==1?SAMPLE:PROJECTILE;
 }
 if(s->hurt_phase==START_EFFECT){
  if(w(p+0x15f4)&0x20u){
   if(!b->effect_create)return fail(s,0x306528,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_frame_effect_request request={.actor=actor,.kind=0x19,.mode=UINT32_MAX,.selector=-1};
   const gh_player_frame_status status=b->effect_create(b->effect_context,&request,&pc);
   if(status)return fail(s,pc?pc:0x306528,status);
   put(p+0x15f4,w(p+0x15f4)&~0x20u);
  }
  p[0x649]=0;s->hurt_phase=STOCK;
 }
 if(s->hurt_phase==STOCK){
  if(!b->save.bytes||b->save.size<0xae)return fail(s,0x1fac00,GH_PLAYER_FRAME_ARGUMENT);
  if(!(w((const uint8_t *)b->work+0x58c)&0x80000u)){
   memmove(b->save.bytes+0xa8,b->save.bytes+0xa9,5);
   b->save.bytes[0xad]=0;
  }
  s->hurt_phase=BIND;
 }
 if(s->hurt_phase==BIND){
  if(!b->initialize_motion||!b->resource||b->resource_bytes<0x458||!b->rate||
     !b->roulette_time_scale)return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request motion={.player=b->player,.model=b->model,.work=b->work,
      .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate,
      .motion_offset=w(b->resource+0x450),.sequence_candidate_offset=w(b->resource+0x454),
      .blend_frames=3,.release_previous_sequence=1};
  const gh_player_init_status result=b->initialize_motion(b->motion_context,&motion,&pc);
  if(result)return fail(s,pc?pc:0x2a8578,result==GH_PLAYER_INIT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  const gh_player_frame_status status=b->roulette_time_scale(b->effect_context,0x3dcccccdu,&pc);
  if(status)return fail(s,pc?pc:0x292f38,status);
  put(p+0x15b0,1);p[0x2f6]=1;
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(w(p+0x640))return fail(s,0x124ec0,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request motion={.player=b->player,.work=b->work,.model=b->model,
      .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  int ended;
  const gh_player_frame_status status=b->advance_motion(b->motion_context,&motion,&ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->hurt_phase=ended?END_EFFECT:ROTATE;
 }
 if(s->hurt_phase==END_EFFECT){
  if(!(w(p+0x15f4)&0x10u)){
   if(!b->effect_create)return fail(s,0x129630,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_frame_effect_request request={.actor=actor,.kind=0x2e,.mode=UINT32_MAX,.selector=-1};
   const gh_player_frame_status status=b->effect_create(b->effect_context,&request,&pc);
   if(status)return fail(s,pc?pc:0x306528,status);
   put(p+0x15f4,w(p+0x15f4)|0x20u);
  }
  s->hurt_phase=END_CAMERA;
 }
 if(s->hurt_phase==END_CAMERA){
  const gh_player_frame_status status=roulette_restore_camera(s);
  if(status)return status;
  if(!s->input_result){memset(p+0x2f4,0,4);s->hurt_phase=ROTATE;}
  else return GH_PLAYER_FRAME_OK;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=PROJECTILE;
 }
 if(s->hurt_phase==PROJECTILE){
  if((half(p+0x3ac)&1u)&&w(p+0x15b0)){
   if(!b->roulette_projectile)return fail(s,0x12bc00,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_frame_status status=b->roulette_projectile(b->effect_context,actor,&pc);
   if(status)return fail(s,pc?pc:0x12bc00,status);
   put(p+0x15b0,0);
  }
  s->hurt_phase=RATE;
 }
 if(s->hurt_phase==RATE){
  if(half(p+0x3ac)&0x100u){
   if(!b->roulette_time_scale)return fail(s,0x292f38,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_frame_status status=b->roulette_time_scale(b->effect_context,0x3dcccccdu,&pc);
   if(status)return fail(s,pc?pc:0x292f38,status);
  }
  s->hurt_phase=INPUT;
 }
 if(s->hurt_phase==INPUT){
  if(!b->idle_input||!b->idle_input_bindings)return fail(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!s->input_started){
   const gh_player_frame_status status=gh_player_idle_input_begin(b->idle_input);
   if(status)return fail(s,0x123938,status);
   s->input_started=1;
  }
  const gh_player_frame_status status=gh_game_player_dodge_input_00123938(b->idle_input,b->idle_input_bindings,&s->input_result);
  if(status)return fail(s,b->idle_input->frontier?b->idle_input->frontier:0x123938,status);
  if(s->input_result){
   s->hurt_phase=END_EFFECT;
   return roulette_shot_00112a48(s);
  }
  put(p+0x674,0x3d4ccccdu);
  sh(p+0x3ac,half(p+0x3ac)|0x800u);
 }
 return GH_PLAYER_FRAME_OK;
}

/* Original 10CD38 dodge. The lateral entries use a retained target. */
static gh_player_frame_status dodge_0010cd38(gh_game_player_frame *s)
{
 enum { PREPARE=1, EFFECT, SOUND, BIND, UPDATE, SAMPLE, APPLY, ROTATE, TRANSLATE, INPUT, FOCUS };
 const gh_player_frame_bindings *b=&s->b;
 gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes;
 const unsigned variant=p[0x2f7];
 uint32_t pc=0;
 if(variant>3)return fail(s,0x10cd38,GH_PLAYER_FRAME_UNSUPPORTED);
 const int lateral=variant>=2;
 if(!s->hurt_phase)s->hurt_phase=p[0x2f6]==0?PREPARE:p[0x2f6]==1?UPDATE:INPUT;
 if(s->hurt_phase==PREPARE){
  const uint32_t target=w(p+0x640);
  if(w(p+0x1620))return fail(s,0x2cbba0,GH_PLAYER_FRAME_UNSUPPORTED);
  if(variant&&target){
   const gh_player_idle_input_bindings *input=b->idle_input_bindings;
   gh_title_player_view selected={0};
   if(!input||!input->targets.resolve||
      input->targets.resolve(input->targets.context,target,&selected,&pc)||
      !selected.bytes||selected.size<0x300)
    return fail(s,pc?pc:0x10cd38,GH_PLAYER_FRAME_UNSUPPORTED);
   const unsigned kind=half(selected.bytes+0x2fe);
   if(kind>=0x200&&kind<0x300){
    const gh_player_frame_status status=damage_score_001fa280(s,-8);
    if(status)return status;
   }
  }
  sh(p+0x5e0,0);
  sh(p+0x5e2,0);
  memset(p+0x1644,0,20);
  put(p+0x15c0,0x3f800000);
  put(p+0x1620,0);
  sh(p+0x568,0);
  if(lateral){
   put(p+0x54c,0x40a00000);
   s->hurt_phase=FOCUS;
  }else{
   if(variant)put(p+0x54c,0x41400000);
   put(p+0x640,0);
   p[0x648]=0;
   s->hurt_phase=EFFECT;
  }
 }
 if(s->hurt_phase==FOCUS){
  const gh_player_frame_status status=lateral_dodge_focus(s,0);
  if(status)return status;
  s->hurt_phase=EFFECT;
 }
 if(s->hurt_phase==EFFECT){
  static const unsigned effects[]={0x17,0x18,0x16,0x15};
  s->pending_effect=(gh_player_frame_effect_request){actor,effects[variant],0,0,UINT32_MAX,-1,0};
  if(!b->effect_create)return fail(s,0x306528,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status status=b->effect_create(b->effect_context,&s->pending_effect,&pc);
  if(status)return fail(s,pc?pc:0x306528,status);
  s->hurt_phase=variant==1?BIND:SOUND;
 }
 if(s->hurt_phase==SOUND){
  gh_player_frame_status status=hurt_sound(s,0x2a);
  if(status)return status;
  if(lateral)put(p+0x15c0,0x3f99999a);
  else sh(p+0x568,12);
  s->hurt_phase=BIND;
 }
 if(s->hurt_phase==BIND){
  static const unsigned rows[]={0x48,0x50,0x40,0x38};
  const unsigned row=rows[variant];
  if(!b->initialize_motion||!b->resource||b->resource_bytes<row+8)
   return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.motion_offset=w(b->resource+row),
   .sequence_candidate_offset=w(b->resource+row+4),.blend_frames=3,
   .release_previous_sequence=1,.rate=b->rate};
  gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
   GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  p[0x2f6]=1;
  if(w(p+0x640))p[0x648]=20;
  s->hurt_phase=UPDATE;
 }
 if(s->hurt_phase==UPDATE){
  if(lateral){
   const gh_player_frame_status status=lateral_dodge_focus(s,1);
   if(status)return status;
  }else if(w(p+0x640))return fail(s,0x124e68,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!variant)put(p+0x15f4,w(p+0x15f4)|0x4000u);
  const int16_t timer=(int16_t)half(p+0x568);
  if(timer>0){
   sh(p+0x568,(uint16_t)(timer-1));
   if(p[0x61f])put(p+0x15f4,w(p+0x15f4)|0x8000u);
  }
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.work=b->work,.model=b->model,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  if(s->motion_ended)put(p+0x2f4,0);
  s->hurt_phase=APPLY;
 }
 if(s->hurt_phase==APPLY){
  const float factor=number(w(p+0x15c0));
  volatile float x=number(w(p+0x330))*factor,z=number(w(p+0x338))*factor;
  if(!isfinite(x)||!isfinite(z))return fail(s,0x10d084,GH_PLAYER_FRAME_NUMERIC);
  sf(p+0x330,x);
  sf(p+0x338,z);
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=INPUT;
 }
 if(!b->idle_input||!b->idle_input_bindings)return fail(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->input_started){
  gh_player_frame_status status=gh_player_idle_input_begin(b->idle_input);
  if(status)return fail(s,0x123938,status);
  s->input_started=1;
 }
 const gh_player_frame_status status=gh_game_player_dodge_input_00123938(
  b->idle_input,b->idle_input_bindings,&s->input_result);
 return status?fail(s,b->idle_input->frontier?b->idle_input->frontier:0x123938,status):GH_PLAYER_FRAME_OK;
}
/* Original 10D0E0 forward evade. Select the clip before drawing its successor. */
static gh_player_frame_status forward_evade_0010d0e0(gh_game_player_frame *s)
{
 enum { RESET=1, TARGET, CHOOSE, SOUND, BIND, TRACK, SAMPLE, ROTATE, TRANSLATE,
        WINDOW, CHECK_PAIR, AWARD_BEGIN, AWARD_STREAK_VOICE, AWARD_STREAK_RANK,
        AWARD_HEAT, AWARD_VOICE, AWARD_RANK, INPUT, DONE };
 const gh_player_frame_bindings *b=&s->b;
 gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 if(!s->hurt_phase)s->hurt_phase=!p[0x2f6]?RESET:p[0x2f6]==1?TRACK:INPUT;
 if(s->hurt_phase==RESET){
  sh(p+0x5e0,0);
  sh(p+0x5e2,0);
  memset(p+0x1644,0,20);
  s->hurt_phase=TARGET;
 }
 if(s->hurt_phase==TARGET){
  gh_player_frame_status status=lateral_dodge_focus(s,0);
  if(status)return status;
  if(w(p+0x1620))return fail(s,0x2cbba0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=CHOOSE;
 }
 if(s->hurt_phase==CHOOSE){
  static const unsigned rows[]={0x284,0x27c,0x28c};
  static const uint8_t next[3][2]={{2,1},{2,0},{1,0}};
  unsigned clip=p[0x1614];
  if(clip>2)clip=0;
  const unsigned row=rows[clip];
  if(!b->initialize_motion||!b->resource||b->resource_bytes<row+8)
   return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.motion_offset=w(b->resource+row),
   .sequence_candidate_offset=w(b->resource+row+4),.start_frame_bits=0x40000000,
   .blend_frames=3,.release_previous_sequence=1,.rate=b->rate};
  uint32_t random;
  gh_player_frame_status status=idle_random(s,&random);
  if(status)return status;
  p[0x1614]=next[clip][random&1u];
  s->hurt_phase=SOUND;
 }
 if(s->hurt_phase==SOUND){
  gh_player_frame_status status=hurt_sound(s,0x41);
  if(status)return status;
  s->hurt_phase=BIND;
 }
 if(s->hurt_phase==BIND){
  gh_player_init_status status=b->initialize_motion(b->motion_context,&s->pending_motion,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
                        GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  put(p+0x15b4,8);
  put(p+0x15b0,0);
  p[0x2f6]=1;
  s->hurt_phase=TRACK;
 }
 if(s->hurt_phase==TRACK){
  if(w(p+0x640)){
   p[0x648]=20;
   gh_player_frame_status status=lateral_dodge_focus(s,1);
   if(status)return status;
  }
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.work=b->work,.model=b->model,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  if(s->motion_ended)put(p+0x2f4,0);
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  sh(p+0x5e2,0);
  s->hurt_phase=WINDOW;
 }
 if(s->hurt_phase==WINDOW){
  const uint32_t remaining=w(p+0x15b4);
  s->hurt_phase=INPUT;
  if(remaining){
   put(p+0x15b4,remaining-1);
   put(p+0x15f4,w(p+0x15f4)|0x10000u);
   p[0x1603]=2;
   s->hurt_phase=CHECK_PAIR;
  }
 }
 if(s->hurt_phase==CHECK_PAIR){
  if(!w(p+0x15b0)){
   if(!actor->alias_f0)return fail(s,0x2b8e10,GH_PLAYER_FRAME_ARGUMENT);
   float origin[3];
   memcpy(origin,actor->alias_f0,sizeof origin);
   origin[1]=add(origin[1],1.0f);
   int near;
   if(gh_collision_pair_near_002b8e10(b->collision_pairs,2,origin,1.5f,&near,&pc))
    return fail(s,pc?pc:0x2b8e10,GH_PLAYER_FRAME_UNSUPPORTED);
   if(near)put(p+0x15b0,1);
  }
  s->hurt_phase=!w(p+0x15b4)&&w(p+0x15b0)==1?AWARD_BEGIN:INPUT;
 }
 /* 129430 can run before input when the window closes, or after an input
  * cancels the move. Resume the original call without repeating a reward. */
 while(s->hurt_phase!=DONE){
  if(s->hurt_phase==AWARD_BEGIN){
   put(p+0x15b0,2);
   if(!half(p+0x1616))p[0x1615]=0;
   ++p[0x1615];
   sh(p+0x1616,45);
   s->hurt_phase=p[0x1615]==5?AWARD_STREAK_VOICE:AWARD_HEAT;
  }
  if(s->hurt_phase==AWARD_STREAK_VOICE){
   const gh_player_frame_status status=hurt_sound(s,0x36);
   if(status)return status;
   s->hurt_phase=AWARD_STREAK_RANK;
  }
  if(s->hurt_phase==AWARD_STREAK_RANK){
   const gh_player_frame_status status=damage_score_001fa280(s,-100);
   if(status)return status;
   s->hurt_phase=AWARD_HEAT;
  }
  if(s->hurt_phase==AWARD_HEAT){
   unsigned rank;
   if(gh_core_save_game_level_001fa010(b->save,b->elf,b->elf_bytes,&rank))
    return fail(s,0x1fa010,GH_PLAYER_FRAME_ARGUMENT);
   const int count=p[0x1615]<128?p[0x1615]:p[0x1615]-256;
   float amount=count<2?1.5f:0.5f;
   if(rank==5)amount*=0.5f;
   if(gh_game_heat_gain_002a9978(b->init,b->work,b->save,amount,&pc))
    return fail(s,pc,GH_PLAYER_FRAME_NUMERIC);
   s->hurt_phase=count%3==1?AWARD_VOICE:AWARD_RANK;
  }
  if(s->hurt_phase==AWARD_VOICE){
   const gh_player_frame_status status=hurt_sound(s,0xee);
   if(status)return status;
   s->hurt_phase=AWARD_RANK;
  }
  if(s->hurt_phase==AWARD_RANK){
   const gh_player_frame_status status=damage_score_001fa280(s,-50);
   if(status)return status;
   s->hurt_phase=s->input_started?DONE:INPUT;
  }
  if(s->hurt_phase==INPUT){
   if(!b->idle_input||!b->idle_input_bindings)return fail(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);
   if(!s->input_started){
    gh_player_frame_status status=gh_player_idle_input_begin(b->idle_input);
    if(status)return fail(s,0x123938,status);
    s->input_started=1;
   }
   const gh_player_frame_status status=gh_game_player_dodge_input_00123938(
    b->idle_input,b->idle_input_bindings,&s->input_result);
   if(status)return fail(s,b->idle_input->frontier?b->idle_input->frontier:0x123938,status);
   s->hurt_phase=s->input_result&&w(p+0x15b0)==1?AWARD_BEGIN:DONE;
  }
 }
 return GH_PLAYER_FRAME_OK;
}

/* 129718 argument1D maps to state00/39, the original EM00 escape. */
static gh_player_frame_status escape_grab_00117600(gh_game_player_frame *s)
{
 enum { PLACE=1, BIND, RANK, CAMERA, FOLLOW, SAMPLE, END, RESTORE, ROTATE, TRANSLATE };
 const gh_player_frame_bindings *b=&s->b;
 const gh_title_player_view *actor=&b->player->view,*attacker=b->grab_attacker;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 /* Original phase one skips the follow add when +694 has been cleared. */
 const int retired=b->grab_attacker_retired&&!attacker&&p[0x2f6]==1;
 const int borrowed=attacker&&attacker->bytes&&attacker->size>=0x642&&w(attacker->bytes+0x564)==0x200;
 const int opening=b->work->room==0x100&&borrowed&&half(attacker->bytes+0x640)==20;
 const int story=b->work->room==0x102&&borrowed&&attacker->size>0x1650&&
    ((half(attacker->bytes+0x640)==11&&attacker->bytes[0x1650]==5)||
     (half(attacker->bytes+0x640)==3&&attacker->bytes[0x1650]==1)||
     (half(attacker->bytes+0x640)==7&&attacker->size>=0x16c8&&!attacker->bytes[0x1650]&&
      w(attacker->bytes+0x16c4)==2));
 const int em27=b->work->room==0x101&&attacker&&attacker->bytes&&attacker->size>0x1650&&
    attacker->original_vtable214==0x4479c8&&w(attacker->bytes+0x564)==0x227&&
    (half(attacker->bytes+0x640)==0x1e||half(attacker->bytes+0x640)==0x26)&&
    attacker->bytes[0x1650]==6;
 const int retired_pair=retired&&(b->work->room==0x100||b->work->room==0x102||b->work->room==0x103);
 if(!actor->alias_f0||(!retired_pair&&!opening&&!story&&!em27&&!r103_grab_attacker(b->work,attacker,0x200)))
  return fail(s,0x117600,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->hurt_phase){
  put(p+0x54c,0x40a00000);
  put(p+0x250,w(p+0x250)|0x10000u);
  if(!b->solids||gh_collision_solid_set_enabled_001351d8(b->solids,&b->player->view,0,&pc))
   return fail(s,pc?pc:0x1351d8,GH_PLAYER_FRAME_UNSUPPORTED);
  uint8_t *manager=b->init->enemies;
  manager[0x53c]=2;
  if((int32_t)w(manager+0x514)<2)put(manager+0x514,2);
  put(p+0x15f4,w(p+0x15f4)|0x80000u);
  if(p[0x2f6]>1)return GH_PLAYER_FRAME_OK;
  s->hurt_phase=p[0x2f6]?CAMERA:PLACE;
 }
 if(s->hurt_phase==PLACE){
  if(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698))
   return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
  if(b->aux_6a0||w(p+0x6a0))return fail(s,0x12b928,GH_PLAYER_FRAME_UNSUPPORTED);
  const float scale_x=number(w(attacker->bytes+0x110)),scale_z=number(w(attacker->bytes+0x118));
  if(!isfinite(scale_x)||!isfinite(scale_z)||scale_x==0||scale_z==0)
   return fail(s,0x1176d4,GH_PLAYER_FRAME_NUMERIC);
  fenv_t env;
  if(feholdexcept(&env))return fail(s,0x1176d4,GH_PLAYER_FRAME_NUMERIC);
  if(fesetround(FE_TOWARDZERO)){
   (void)fesetenv(&env);
   return fail(s,0x1176d4,GH_PLAYER_FRAME_NUMERIC);
  }
  volatile float inverse_x=1.0f/scale_x,inverse_z=1.0f/scale_z;
  const float x=inverse_x*number(0xbf158e22u),z=inverse_z*number(0x3e0ce704u);
  if(fesetenv(&env)||!isfinite(x)||!isfinite(z))
   return fail(s,0x1176d4,GH_PLAYER_FRAME_NUMERIC);
  uint32_t xb,zb;
  memcpy(&xb,&x,4);memcpy(&zb,&z,4);
  gh_player_frame_status status=place_grab_001299f0(s,attacker,xb,zb);
  if(status)return status;
  s->hurt_phase=BIND;
 }
 if(s->hurt_phase==BIND){
  const gh_object_view *resource=&b->grab_resource;
  if(!b->initialize_motion||!resource->bytes||!resource->file_extent_known||
     resource->file_extent<0x540||resource->file_extent>resource->size)
   return fail(s,0x23a300,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,
   .work=b->work,.resource=resource->bytes,.resource_bytes=resource->size,
   .motion_offset=w(resource->bytes+0x538),.sequence_candidate_offset=w(resource->bytes+0x53c),
   .release_previous_sequence=1,.rate=b->rate};
  gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
   GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  s->hurt_phase=RANK;
 }
 if(s->hurt_phase==RANK){
  gh_player_frame_status status=damage_score_001fa280(s,-100);
  if(status)return status;
  if((int16_t)half(p+0x54a)<1)sh(p+0x54a,1);
  sh(p+0x56e,30);
  p[0x2f6]=1;
  s->hurt_phase=CAMERA;
 }
 if(s->hurt_phase==CAMERA){
  if(!b->grab_camera)return fail(s,0x124ec0,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status status=b->grab_camera(b->motion_context,b->model,&pc);
  if(status)return fail(s,pc?pc:0x124ec0,status);
  put(p+0x15f4,w(p+0x15f4)|2u);
  s->hurt_phase=FOLLOW;
 }
 if(s->hurt_phase==FOLLOW){
  const unsigned frames=half(p+0x56e);
  if(frames&&attacker){
   float position[3],offset[3];
   memcpy(position,actor->alias_f0,12);
   memcpy(offset,attacker->bytes+0x550,12);
   for(unsigned i=0;i<3;i++){
    volatile float next=position[i]+offset[i];
    if(!isfinite(next))return fail(s,0x117810,GH_PLAYER_FRAME_NUMERIC);
    position[i]=next;
   }
   sh(p+0x56e,(uint16_t)(frames-1));
   memcpy(actor->alias_f0,position,12);
  }
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.work=b->work,.model=b->model,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->hurt_phase=s->motion_ended?END:ROTATE;
 }
 if(s->hurt_phase==END){
  if(w(p+0x15f4)&2u){
   if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
   put(p+0x15f4,w(p+0x15f4)&~2u);
   s->hurt_phase=RESTORE;
  }else s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==RESTORE){
  gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
  if(status)return fail(s,pc?pc:0x124f60,status);
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(p[0x2f6]&&s->motion_ended)p[0x2f4]=p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
  return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
 return GH_PLAYER_FRAME_OK;
}

/* Original 11A530 EventWait, unarmed/nonthrow context after R105 JMP1. */
static gh_player_frame_status event_wait_0011a530(gh_game_player_frame *s)
{
 const gh_player_frame_bindings *b=&s->b;
 uint8_t *p=b->player->view.bytes;
 uint32_t pc=0;
 put(p+0x15f4,w(p+0x15f4)|0x80u);
 if(!s->script_wait_phase){
  if(p[0x2f6]>1)return GH_PLAYER_FRAME_OK;
  if(!p[0x2f6]){
   sh(p+0x5e0,0);
   sh(p+0x5e2,0);
   if(!b->auxiliary_bindings_known||b->aux_6a0||b->aux_698||w(p+0x6a0)||w(p+0x698))
    return fail(s,0x11a530,GH_PLAYER_FRAME_UNSUPPORTED);
   if(!b->resource||b->resource_bytes<0x30||!b->model||!b->initialize_motion)
    return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
   s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,
    .work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,
    .motion_offset=w(b->resource+0x28),.sequence_candidate_offset=w(b->resource+0x2c),
    .blend_frames=3,.rate=b->rate};
   s->script_wait_phase=1;
  }else s->script_wait_phase=2;
 }
 if(s->script_wait_phase==1){
  gh_player_init_status status=b->initialize_motion(b->motion_context,&s->pending_motion,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
    GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  p[0x2f6]=1;
  s->script_wait_phase=2;
 }
 if(s->script_wait_phase==2){
  if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  int ended;
  const gh_player_motion_request request={.player=b->player,.model=b->model,
   .work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->script_wait_phase=3;
 }
 if(s->script_wait_phase==3){
  if(gh_player_root_rotation_0014f9f0(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->script_wait_phase=4;
 }
 if(gh_player_root_translation_0014f928(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
  return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
 return GH_PLAYER_FRAME_OK;
}

/* Original action30 initialization and unbound-prop motion. The positive
 * prop path continues through 1C8AB0 before either motion is opened. */
static gh_player_frame_status throw_box_00110e98(gh_game_player_frame *s)
{
 enum { TARGET=1, BOX_CLIP, PLAYER_CLIP, TURN, SAMPLE, ROTATE, TRANSLATE, RELEASE, INPUT };
 const gh_player_frame_bindings *b=&s->b;
 gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 if(!b->auxiliary_bindings_known||w(p+0x6a0)||!b->idle_input_bindings)
  return fail(s,0x110e98,GH_PLAYER_FRAME_UNSUPPORTED);
 const gh_player_target_bindings *targets=&b->idle_input_bindings->targets;
 if(targets->player!=actor)return fail(s,0x124540,GH_PLAYER_FRAME_ARGUMENT);
 if(!s->idle_phase){
  put(p+0x15f4,w(p+0x15f4)|0x100000);
  if(!p[0x2f6]){
   sh(p+0x5e0,0);
   sh(p+0x5e2,0);
   s->idle_phase=TARGET;
  }else s->idle_phase=p[0x2f6]==1?TURN:RELEASE;
 }
continue_phase:
 if(s->idle_phase==TARGET||s->idle_phase==TURN){
  gh_player_target target={0};
  if(s->idle_phase==TURN||(int8_t)p[0x648]>0)target.handle=w(p+0x640);
  if(target.handle){
   if(!targets->resolve)return fail(s,0x124e68,GH_PLAYER_FRAME_UNSUPPORTED);
   if(targets->resolve(targets->context,target.handle,&target.view,&pc))
    return fail(s,pc?pc:0x124e68,GH_PLAYER_FRAME_UNSUPPORTED);
  }
  if(s->idle_phase==TARGET){
   if(gh_player_throw_target_00124540(targets,b->init,&target,&pc))
    return fail(s,pc?pc:0x124540,GH_PLAYER_FRAME_UNSUPPORTED);
   s->idle_phase=BOX_CLIP;
  }else{
   if(target.handle){
    p[0x648]=20;
    float origin[3],position[3],delta,yaw;
    if(!actor->alias_f0)return fail(s,0x124e68,GH_PLAYER_FRAME_ARGUMENT);
    memcpy(origin,actor->alias_f0,12);
    if(gh_player_throw_target_position(targets,&target,position,&pc))
     return fail(s,pc?pc:0x124e68,GH_PLAYER_FRAME_UNSUPPORTED);
    const float previous=number(w(p+0x104));
    volatile float cap=number(0x3e490fdb)*number(w((const uint8_t*)b->work+0x55c));
    if(gh_player_target_angle_002da038(b->elf,b->elf_bytes,origin,position,previous,cap,&delta,&pc))
     return fail(s,pc?pc:0x2da038,GH_PLAYER_FRAME_UNSUPPORTED);
    if(gh_id_angle_reference_001f7d48(add(previous,delta),&yaw))
     return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);
    sf(p+0x104,yaw);
   }
   s->idle_phase=SAMPLE;
  }
 }
 if(s->idle_phase==BOX_CLIP){
  if(b->aux_6a0){
   if(!b->interact_prop)return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
   gh_player_frame_status status=b->interact_prop(b->motion_context,0,b->aux_6a0,b,0x37c,0x398,&pc);
   if(status)return fail(s,pc?pc:0x2a8578,status);
  }
  s->idle_phase=PLAYER_CLIP;
 }
 if(s->idle_phase==PLAYER_CLIP){
  if(!b->initialize_motion||!b->resource||b->resource_bytes<0x36c||!b->rate)
   return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate,
   .motion_offset=w(b->resource+0x364),.sequence_candidate_offset=w(b->resource+0x368),
   .blend_frames=3};
  const gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
   GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  put(p+0x15b0,1);
  ++p[0x2f6];
  s->idle_phase=TURN;
  goto continue_phase;
 }
 if(s->idle_phase==SAMPLE){
  if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  if(s->motion_ended){
   put(p+0x640,0);
   p[0x648]=0;
   put(p+0x2f4,0);
  }
  s->idle_phase=ROTATE;
 }
 if(s->idle_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->idle_phase=TRANSLATE;
 }
 if(s->idle_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->idle_phase=RELEASE;
 }
 if(s->idle_phase==RELEASE){
  if((half(p+0x3ac)&1)&&b->aux_6a0){
   if(!b->throw_prop)return fail(s,0x12b618,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_frame_status status=b->throw_prop(b->motion_context,b,w(p+0x640),&pc);
   if(status)return fail(s,pc?pc:0x12b618,status);
  }
  s->idle_phase=INPUT;
 }
 if(!b->idle_input)return fail(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->input_started){
  gh_player_frame_status status=gh_player_idle_input_begin(b->idle_input);
  if(status)return fail(s,0x123938,status);
  s->input_started=1;
 }
 gh_player_frame_status status=gh_game_player_dodge_input_00123938(
  b->idle_input,b->idle_input_bindings,&s->input_result);
 if(status)return fail(s,b->idle_input->frontier?b->idle_input->frontier:0x123938,status);
 if(s->input_result){put(p+0x640,0);p[0x648]=0;}
 return GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status interact_00115310(gh_game_player_frame *s)
{
 const gh_player_frame_bindings *b=&s->b;
 gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes,*manager=b->init->enemies;
 uint32_t pc=0;
 if(b->work->room!=0x101||!b->auxiliary_bindings_known||!b->idle_input_bindings||
    !actor->alias_f0)return fail(s,0x115310,GH_PLAYER_FRAME_UNSUPPORTED);
 const gh_player_target_bindings *targets=&b->idle_input_bindings->targets;
 if(!s->idle_phase){
  put(p+0x54c,0x40a00000);
  if((int32_t)w(manager+0x514)<2)put(manager+0x514,2);
  manager[0x53c]=2;
  put(p+0x5dc,0x3fa00000);put(p+0x5a8,0x3fa00000);
  put(p+0x444,(half(p+0x434)&4u)?0xbfa00000u:0x3fa00000u);
  if(p[0x2f6]>1)return GH_PLAYER_FRAME_OK;
  s->idle_phase=p[0x2f6]?4:1;
 }
 if(s->idle_phase==1){
  sh(p+0x5e0,0);sh(p+0x5e2,0);memset(p+0x1644,0,20);
  p[0x648]=0;put(p+0x640,0);
  float world[16],destination[4];
  const float forward[3]={0,0,1};
  memcpy(world,p+0x80,sizeof(world));
  if(gh_scene_at_apply_world_001508d8(world,forward,destination))
   return fail(s,0x30a2b0,GH_PLAYER_FRAME_NUMERIC);
  memcpy(p+0x580,destination,12);
  s->idle_phase=2;
 }
 if(s->idle_phase==2){
  gh_player_target target;
  const gh_player_target_status status=gh_player_enemy_target_00291010(
    targets,b->init,40.0f,3.1415927f,&target,&pc);
  if(status)return fail(s,pc?pc:0x291010,status==GH_PLAYER_TARGET_NUMERIC?
     GH_PLAYER_FRAME_NUMERIC:GH_PLAYER_FRAME_UNSUPPORTED);
  size_t row=0x6e0;
  const float yaw=number(w(p+0x104));
  if(target.handle){
   float from[3],to[3],angle;
   memcpy(from,actor->alias_f0,12);
   if(gh_enemy_target_position_00275978(&target.view,to))
    return fail(s,0x275978,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_target_status angle_status=gh_player_target_angle_002da038(
     b->elf,b->elf_bytes,from,to,yaw,3.1415927f,&angle,&pc);
   if(angle_status)return fail(s,pc?pc:0x2da038,GH_PLAYER_FRAME_UNSUPPORTED);
   memcpy(p+0x580,to,12);
   if(fabsf(angle)>2.3561945f)row=0x35c;
  }
  put(p+0x640,target.handle);
  put(p+0x15c0,w(p+0x104));
  s->idle_motion_word=row;
  s->idle_phase=3;
 }
 if(s->idle_phase==3){
  gh_player_target prop={0};
  if(b->idle_input)prop=b->idle_input->interaction;
  if(!prop.handle&&!b->aux_6a0&&!w(p+0x6a0)&&!w(p+0x6a4)){
   const gh_player_target_status status=gh_player_interact_target_0012b328(targets,&prop,&pc);
   if(status)return fail(s,pc?pc:0x12b328,GH_PLAYER_FRAME_UNSUPPORTED);
  }
  if(prop.handle||b->aux_6a0||w(p+0x6a0)||w(p+0x6a4)){
   if(!b->interact_prop||!prop.handle||w(p+0x6a0)||w(p+0x6a4))
    return fail(s,0x1c8ab0,GH_PLAYER_FRAME_UNSUPPORTED);
   const int reverse=s->idle_motion_word==0x35c;
   const gh_player_frame_status status=b->interact_prop(b->motion_context,prop.handle,
       &prop.view,b,reverse?0x378:0x6e8,reverse?0x394:0x6ec,&pc);
   if(status)return fail(s,pc?pc:0x1c8ab0,status);
  }
  s->idle_phase=10;
 }
 if(s->idle_phase==10){
  const size_t row=s->idle_motion_word;
  if(!b->initialize_motion||!b->resource||b->resource_bytes<row+8||!b->rate)
   return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate,
   .motion_offset=w(b->resource+row),.sequence_candidate_offset=w(b->resource+row+4)};
  const gh_player_init_status status=b->initialize_motion(b->motion_context,&s->pending_motion,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  if(row==0x35c){
   float next;
   if(gh_id_angle_reference_001f7d48(add(number(w(p+0x104)),3.1415927f),&next))
    return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);
   sf(p+0x15c0,next);
  }
  put(p+0x640,0);p[0x648]=0;++p[0x2f6];
  s->idle_phase=4;
 }
 if(s->idle_phase==4){
  if(!b->grab_camera)return fail(s,0x124ec0,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_frame_status status=b->grab_camera(b->motion_context,b->model,&pc);
  if(status)return fail(s,pc?pc:0x124ec0,status);
  put(p+0x15f4,w(p+0x15f4)|2u);
  s->idle_phase=5;
 }
 if(s->idle_phase==5){
  if(half(p+0x3ac)&0x10u){
   float from[3],to[3],delta,target_yaw,actor_yaw;
   memcpy(from,actor->alias_f0,12);memcpy(to,p+0x580,12);
   const gh_player_target_status status=gh_player_target_angle_002da038(b->elf,b->elf_bytes,
      from,to,number(w(p+0x15c0)),number(0x3e65c8fa),&delta,&pc);
   if(status)return fail(s,pc?pc:0x2da038,GH_PLAYER_FRAME_UNSUPPORTED);
   if(gh_id_angle_reference_001f7d48(add(number(w(p+0x15c0)),delta),&target_yaw)||
      gh_id_angle_reference_001f7d48(add(number(w(p+0x104)),delta),&actor_yaw))
    return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);
   sf(p+0x15c0,target_yaw);sf(p+0x104,actor_yaw);
  }
  s->idle_phase=6;
 }
 if(s->idle_phase==6){
  if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
     .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  const gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  s->idle_phase=7;
 }
 if(s->idle_phase==7){
  if(s->motion_ended){
   if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
   if(status)return fail(s,pc?pc:0x124f60,status);
   put(p+0x15f4,w(p+0x15f4)&~2u);memset(p+0x2f4,0,4);
  }
  s->idle_phase=8;
 }
 if(s->idle_phase==8){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->idle_phase=9;
 }
 if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
  return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
 return GH_PLAYER_FRAME_OK;
}

/* PL00 uses separate authored Heat transition clips. Activation enables Heat
 * on SEQ bit0. Exhaustion publishes Give2A once during initialization and
 * restores ordinary flags from the finish clip's own SEQ bit0. */

/* 291B90 only sees the single authored stamp-zero enemy in R104.
 * 447A8C dispatches 26F200 before the caller awards Heat and rank input. */
static gh_player_frame_status r104_taunt_nearby_00291b90(gh_game_player_frame *s, int *accepted)
{
    const gh_player_frame_bindings *b = &s->b;
    gh_game_title_player *enemy = NULL;
    *accepted = 0;
    if (gh_enemy_manager_get_00290868(b->init, 0, &enemy))
        return fail(s, 0x291bd8, GH_PLAYER_FRAME_ARGUMENT);
    if (!enemy) return GH_PLAYER_FRAME_OK;

    const gh_title_player_view *view = &enemy->view;
    uint8_t *p = view->bytes;
    const gh_title_player_view *player = &b->player->view;
    if (!p || view->size < 0x16f0 || !view->alias_f0 || !player->alias_f0 ||
        view->original_vtable214 != 0x4479c8 || half(p + 0x2fe) != 0x203 ||
        w(p + 0x564) != 0x250)
        return fail(s, 0x291cd8, GH_PLAYER_FRAME_UNSUPPORTED);
    if ((int16_t)half(p + 0x54a) <= 0) return GH_PLAYER_FRAME_OK;

    float origin[3], target[3];
    memcpy(origin, player->alias_f0, sizeof(origin));
    memcpy(target, view->alias_f0, sizeof(target));
    fenv_t env;
    if (feholdexcept(&env)) return fail(s, 0x1f7eb0, GH_PLAYER_FRAME_NUMERIC);
    if (fesetround(FE_TOWARDZERO)) {
        (void)fesetenv(&env);
        return fail(s, 0x1f7eb0, GH_PLAYER_FRAME_NUMERIC);
    }
    const float dx = sub(target[0], origin[0]), dz = sub(target[2], origin[2]);
    const float dy = sub(target[1], origin[1]);
    volatile float xx = dx * dx, zz = dz * dz;
    const float distance_squared = add(xx, zz);
    gh_scene_ray_request ray = {.mask = 1, .flags = 0x10, .segmented = 1};
    memcpy(ray.from, target, sizeof(target));
    memcpy(ray.to, origin, sizeof(origin));
    ray.from[1] = add(target[1], 1.0f);
    ray.to[1] = add(origin[1], 1.0f);
    ray.from[3] = ray.to[3] = 1.0f;
    if (fesetenv(&env) || !isfinite(distance_squared) || !isfinite(dy) ||
        !isfinite(ray.from[1]) || !isfinite(ray.to[1]))
        return fail(s, 0x1f7eb0, GH_PLAYER_FRAME_NUMERIC);
    if (fabsf(dy) > 2.0f || distance_squared > 225.0f) return GH_PLAYER_FRAME_OK;

    uint32_t pc = 0;
    float angle;
    if (gh_player_target_angle_002da038(b->elf, b->elf_bytes, origin, target,
        number(w(player->bytes + 0x104)), number(0x40490fdb), &angle, &pc))
        return fail(s, pc ? pc : 0x2da038, GH_PLAYER_FRAME_NUMERIC);
    if (fabsf(angle) > number(0x3f060a92) || number(w(p + 0x16c0)) > 0.0f)
        return GH_PLAYER_FRAME_OK;

    /* 26F328 checks enemy-to-player visibility and writes the visibility bit. */
    int hit = 0;
    if (!b->actors || gh_scene_ray_query_00131e10(b->actors, &ray, &hit, &pc))
        return fail(s, pc ? pc : 0x131e10, GH_PLAYER_FRAME_UNSUPPORTED);
    const uint32_t flags = w(p + 0x16d0);
    put(p + 0x16d0, hit == 1 ? flags & ~0x200000u : flags | 0x200000u);
    if (hit == 1 || (flags & 0x40000u)) return GH_PLAYER_FRAME_OK;

    unsigned rank;
    if (gh_core_save_game_level_001fa010(b->save, b->elf, b->elf_bytes, &rank))
        return fail(s, 0x1fa010, GH_PLAYER_FRAME_ARGUMENT);
    /* 445B50 selects threshold150. The forced callback clears the pressure
     * and sets the rank-selected timer. Dep0250 skips both pressure sounds. */
    put(p + 0x16ec, rank >= 3 ? 300 : 150);
    put(p + 0x16e8, 0);
    put(p + 0x16dc, 0);
    *accepted = (int)rank;
    return GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status taunt_00114460(gh_game_player_frame *s)
{
 enum {BIND=1,SAMPLE,ROTATE,TRANSLATE,MARKER,HEAT,RANK,INPUT};
 const gh_player_frame_bindings *b=&s->b;
 gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 if(b->work->room!=0x104)return fail(s,0x114460,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->hurt_phase){
  if(p[0x2f6]){
   s->hurt_phase=p[0x2f6]==1?SAMPLE:INPUT;
  }else{
   if(!b->model||!b->resource||b->resource_bytes<0x8a8||!b->initialize_motion||!b->rate)
    return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
   sh(p+0x5e0,0);
   sh(p+0x5e2,0);
   uint32_t random;
   gh_player_frame_status status=idle_random(s,&random);
   if(status)return status;
   const size_t row=(random&1u)?0x230:0x8a0;
   s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,.work=b->work,
      .resource=b->resource,.resource_bytes=b->resource_bytes,.motion_offset=w(b->resource+row),
      .sequence_candidate_offset=w(b->resource+row+4),.blend_frames=3,
      .release_previous_sequence=1,.rate=b->rate};
   s->hurt_phase=BIND;
  }
 }
 if(s->hurt_phase==BIND){
  const gh_player_init_status status=b->initialize_motion(b->motion_context,&s->pending_motion,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
      GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  sh(p+0x568,1);
  p[0x2f6]=1;
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  if(s->motion_ended)memset(p+0x2f4,0,4);
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=MARKER;
 }
 if(s->hurt_phase==MARKER){
  s->hurt_phase=INPUT;
  if((half(p+0x3ac)&1u)&&half(p+0x568)){
   int rank;
   const gh_player_frame_status status=r104_taunt_nearby_00291b90(s,&rank);
   if(status){s->hurt_phase=MARKER;return status;}
   sh(p+0x568,0);
   s->pending_damage=rank;
   if(rank)s->hurt_phase=HEAT;
  }
 }
 if(s->hurt_phase==HEAT){
  if(gh_game_heat_gain_002a9978(b->init,b->work,b->save,s->pending_damage==5?6.0f:12.0f,&pc))
   return fail(s,pc,GH_PLAYER_FRAME_NUMERIC);
  s->hurt_phase=s->pending_damage==5?INPUT:RANK;
 }
 if(s->hurt_phase==RANK){
  const gh_player_frame_status status=damage_score_001fa280(s,-250);
  if(status)return status;
  s->hurt_phase=INPUT;
 }
 if(!b->idle_input||!b->idle_input_bindings)return fail(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->input_started){
  const gh_player_frame_status status=gh_player_idle_input_begin(b->idle_input);
  if(status)return fail(s,0x123938,status);
  s->input_started=1;
 }
 const gh_player_frame_status status=gh_game_player_dodge_input_00123938(
     b->idle_input,b->idle_input_bindings,&s->input_result);
 return status?fail(s,b->idle_input->frontier?b->idle_input->frontier:0x123938,status):GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status heat_transition_00114120_001142e0(gh_game_player_frame *s,int finishing)
{
 enum { BIND=1, FINISH_GIVE, SAMPLE, ROTATE, TRANSLATE, ACTIVATE, RELEASE, STRIKE, INPUT, DONE };
 const gh_player_frame_bindings *b=&s->b;
 const gh_title_player_view *actor=&b->player->view;
 uint8_t *p=actor->bytes;
 uint32_t pc=0;
 const unsigned motion_row=finishing?0x18:0x10;
 if(b->work->room<0x100||b->work->room>0x103||p[0x2f7])
  return fail(s,finishing?0x1142e0:0x114120,GH_PLAYER_FRAME_UNSUPPORTED);
 if(!s->hurt_phase){
  put(p+0x54c,0x41700000u);
  if(p[0x2f6])s->hurt_phase=p[0x2f6]==1?SAMPLE:finishing?INPUT:ACTIVATE;
  else{
   if(!finishing&&(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698)))
    return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
   if(!finishing&&(b->aux_6a0||w(p+0x6a0)))return fail(s,0x12b928,GH_PLAYER_FRAME_UNSUPPORTED);
   if(!b->model||!b->resource||b->resource_bytes<motion_row+8||!b->initialize_motion)
    return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
   sh(p+0x5e0,0);sh(p+0x5e2,0);
   p[0x648]=0;
   put(p+0x640,0);
   if(!finishing){
    for(unsigned side=0;side<2;++side){
    put(p+0x1660+4*side,0x17);
    put(p+0x1668+4*side,5);
    put(p+0x1670+4*side,0x2a);
    put(p+0x1678+4*side,0);
    sh(p+0x1680+2*side,10);
   }
    p[0x1684]=1;
   }
   s->hurt_phase=BIND;
  }
 }
 if(s->hurt_phase==BIND){
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.motion_offset=w(b->resource+motion_row),
   .sequence_candidate_offset=w(b->resource+motion_row+4),.blend_frames=3,.release_previous_sequence=1,.rate=b->rate};
  const gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
  if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
      GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  s->hurt_phase=finishing?FINISH_GIVE:SAMPLE;
  if(!finishing)p[0x2f6]=1;
 }
 if(s->hurt_phase==FINISH_GIVE){
  for(unsigned side=0;side<2;++side){
   put(p+0x1660+4*side,0x17);
   put(p+0x1668+4*side,5);
   put(p+0x1670+4*side,0x2a);
   put(p+0x1678+4*side,0);
   sh(p+0x1680+2*side,10);
  }
  p[0x1684]=1;
  sh(p+0x3ac,(uint16_t)(half(p+0x3ac)|1u));
  const gh_hitarea_status status=gh_player_attack_hitarea_0010a438(actor,b->give_objects,&pc);
  if(status)return fail(s,pc?pc:0x10a438,status==GH_HITAREA_UNSUPPORTED?
      GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  p[0x2f6]=1;
  s->hurt_phase=SAMPLE;
 }
 if(s->hurt_phase==SAMPLE){
  if(!b->advance_motion||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
  const gh_player_motion_request request={.player=b->player,.model=b->model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
  const gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&s->motion_ended,&pc);
  if(status)return fail(s,pc?pc:0x157c18,status);
  if(s->motion_ended)memset(p+0x2f4,0,4);
  s->hurt_phase=ROTATE;
 }
 if(s->hurt_phase==ROTATE){
  if(gh_player_root_rotation_0014f9f0(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=TRANSLATE;
 }
 if(s->hurt_phase==TRANSLATE){
  if(gh_player_root_translation_0014f928(actor,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->hurt_phase=finishing?INPUT:ACTIVATE;
 }
 if(s->hurt_phase==ACTIVATE){
  if((half(p+0x3ac)&1u)&&!b->init->heat[0x10]){
   if(!b->effects||!b->current_player||b->current_player->bytes!=actor->bytes)
    return fail(s,0x306200,GH_PLAYER_FRAME_UNSUPPORTED);
   if(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698))
    return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
   if(b->aux_6a0||w(p+0x6a0))return fail(s,0x12b928,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_init_status status=gh_game_heat_set_mode_002a9cc8(b->init,GH_STATIC_BINARY32_REFERENCE,1,&pc);
   if(status)return fail(s,pc?pc:0x2a9cc8,GH_PLAYER_FRAME_CALLEE);
   s->hurt_phase=RELEASE;
  }else s->hurt_phase=STRIKE;
 }
 if(s->hurt_phase==RELEASE){
  unsigned slot;
  const gh_effect_status status=gh_game_effect_release_00306200(b->effects,b->current_player,3,2,&pc,&slot);
  if(status)return fail(s,pc?pc:0x306200,GH_PLAYER_FRAME_CALLEE);
  put(p+0x15f4,(w(p+0x15f4)|0x10u)&~0x20u);
  s->hurt_phase=STRIKE;
 }
 if(s->hurt_phase==STRIKE){
  const gh_hitarea_status status=gh_player_attack_hitarea_0010a438(actor,b->give_objects,&pc);
  if(status)return fail(s,pc?pc:0x10a438,status==GH_HITAREA_UNSUPPORTED?
      GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  s->hurt_phase=INPUT;
 }
 if(s->hurt_phase==INPUT){
  if(!b->idle_input||!b->idle_input_bindings)return fail(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!s->input_started){
   const gh_player_frame_status status=gh_player_idle_input_begin(b->idle_input);
   if(status)return fail(s,0x123938,status);
   s->input_started=1;
  }
  const gh_player_frame_status status=gh_game_player_dodge_input_00123938(
      b->idle_input,b->idle_input_bindings,&s->input_result);
  if(status)return fail(s,b->idle_input->frontier?b->idle_input->frontier:0x123938,status);
  if(!s->input_result){
   if(finishing&&(half(p+0x3ac)&1u))
    put(p+0x15f4,(w(p+0x15f4)&~0x10u)|0x20u);
   sf(p+0x674,0.05f);
   sh(p+0x3ac,(uint16_t)(half(p+0x3ac)|0x800u));
  }
  s->hurt_phase=DONE;
 }
 return GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status body(gh_game_player_frame*s){const gh_player_frame_bindings*b=&s->b;uint8_t*p=b->player->view.bytes;const uint8_t*work=(const uint8_t*)b->work;uint32_t pc=0;float angle;
 if(s->stage==0){if(w(work+0x5d8)&0x80000000u||w(b->init->object_pool+0xacd0)){s->bound=0;s->frontier=0;return GH_PLAYER_FRAME_OK;}s->previous_left=number(w(p+0x60c));if(!isfinite(s->previous_left))return fail(s,0x108b0c,GH_PLAYER_FRAME_NUMERIC);int x=(int)(int8_t)work[0x1f0],y=(int)(int8_t)work[0x1f1];s->left_x=(int16_t)x;s->left_y=(int16_t)y;sf(p+0x60c,(float)(x*x+y*y));s->stage=1;}
 if(s->stage==1){int x=s->left_x,y=s->left_y;if(gh_game_angle_001f8180((float)-x,(float)y,&angle,b->numeric_policy,&pc))return fail(s,pc?pc:0x1f8180,GH_PLAYER_FRAME_UNSUPPORTED);sf(p+0x604,angle);sf(p+0x608,angle<0?-angle:angle);float current=number(w(p+0x60c));p[0x61c]=(uint8_t)(s->previous_left<=100&&current>100);p[0x61d]=(uint8_t)(s->previous_left<=10000&&current>10000);p[0x61f]=(uint8_t)(current>100);s->previous_right=number(w(p+0x618));if(!isfinite(s->previous_right))return fail(s,0x108c40,GH_PLAYER_FRAME_NUMERIC);int x2=(int)(int8_t)work[0x1f2],y2=(int)(int8_t)work[0x1f3];s->right_x=(int16_t)x2;s->right_y=(int16_t)y2;sf(p+0x618,(float)(x2*x2+y2*y2));s->stage=2;}
 if(s->stage==2){float timer=number(w(p+0x15d0));if(!isfinite(timer))return fail(s,0x108dc0,GH_PLAYER_FRAME_NUMERIC);int x=s->right_x,y=s->right_y;if(gh_game_angle_001f8180((float)-x,(float)y,&angle,b->numeric_policy,&pc))return fail(s,pc?pc:0x1f8180,GH_PLAYER_FRAME_UNSUPPORTED);sf(p+0x610,angle);sf(p+0x614,angle<0?-angle:angle);float current=number(w(p+0x618));p[0x61e]=(uint8_t)(s->previous_right<=100&&current>100);p[0x620]=(uint8_t)(current>100);uint32_t n=w(p+0x5f8);if(!n)put(p+0x5f4,0);else put(p+0x5f8,n-1);static const unsigned offsets[]={0x626,0x621,0x622,0x623,0x624,0x625,0x627,0x628};for(unsigned i=0;i<8;i++)decrement_byte(p+offsets[i]);if(timer!=0)sf(p+0x15d0,sub(timer,1));put(p+0x674,0);n=w(p+0x5fc);if(n){uint32_t next=n-1;put(p+0x5fc,next);int32_t signed_next;memcpy(&signed_next,&next,4);s->effect_pending=(uint8_t)(signed_next<1);}s->stage=3;}
 if(s->stage==3){if(s->effect_pending){if(!b->current_player||!b->effects)return fail(s,0x306200,GH_PLAYER_FRAME_UNSUPPORTED);unsigned slot;gh_effect_status e=gh_game_effect_release_00306200(b->effects,b->current_player,5,2,&pc,&slot);if(e)return fail(s,pc?pc:0x306200,GH_PLAYER_FRAME_CALLEE);s->effect_pending=0;}if(!b->save.bytes||b->save.size<GH_CORE_SAVE_BYTES||!b->scenario.bytes||b->scenario.size<0x15)return fail(s,0x108db8,GH_PLAYER_FRAME_ARGUMENT);unsigned stage=work[0x5b1];if((w(b->save.bytes+0x14)&0x4000000)||b->scenario.bytes[0x14])stage=b->init->enemies[0x5b4];static const uint32_t multiplier[]={0x3f800000,0x3f800000,0x3f99999a,0x3fb33333,0x3fcccccd,0x3fe66666,0x40000000,0x400ccccd,0x4019999a};put(p+0x600,stage<=8?multiplier[stage]:0x3f800000);uint32_t pressed=w(work+8);if(pressed&0x80)p[0x621]=3;if(pressed&0x10)p[0x622]=3;if(pressed&0x20)p[0x625]=3;if(pressed&0x40)p[0x623]=3;if(pressed&8)p[0x624]=3;if(pressed&(w(work+0x56c)?0x10:0x40))p[0x626]=3;if(p[0x61d]&&number(w(p+0x608))<number(0x3f860a92)){if(!p[0x628])p[0x628]=10;else {p[0x628]=0;p[0x627]=2;}}gh_player_frame_status e=gh_game_player_button_history_00126820(&b->player->view,b->work);if(e)return e;s->stage=4;}
 if(s->stage==4){if(!b->camera||!b->camera->constructed||b->init->active_camera!=b->camera->identity)return fail(s,0x108f04,GH_PLAYER_FRAME_UNSUPPORTED);float x=sub(number(w(b->camera->fields+0x200)),number(w(b->camera->fields+0x210))),z=sub(number(w(b->camera->fields+0x208)),number(w(b->camera->fields+0x218)));if(gh_game_angle_001f8180(x,z,&angle,b->numeric_policy,&pc))return fail(s,pc?pc:0x1f8180,GH_PLAYER_FRAME_UNSUPPORTED);sf(p+0x15f8,angle);s->stage=5;}
 if(s->stage==5){if(p[0x61f]){volatile float sum=number(w(p+0x15f8))+number(w(p+0x604));float wrapped;if(gh_id_angle_reference_001f7d48(sum,&wrapped))return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);sf(p+0x15fc,wrapped);}/*2E0D60→2E0B78→2E0AD8 returns1 unconditionally: both diagnosticprints truly skipped. */decrement_byte(p+0x648);decrement_byte(p+0x1600);decrement_byte(p+0x1601);decrement_byte(p+0x1602);decrement_half(p+0x1616);decrement_half(p+0x1618);decrement_byte(p+0x1603);s->stage=6;}
 if(s->stage==6){
  gh_player_target_bindings local={.player=&b->player->view};
  const gh_player_target_bindings *targets=&local;
  if((int8_t)p[0x648]>0&&w(p+0x640)&&b->idle_input_bindings)
   targets=&b->idle_input_bindings->targets;
  if(targets->player!=&b->player->view)return fail(s,0x124c30,GH_PLAYER_FRAME_ARGUMENT);
  gh_player_target_status status=gh_player_check_attack_target_00124c30(targets,b->init,&pc);
  if(status)return fail(s,pc?pc:0x124c30,status==GH_PLAYER_TARGET_UNSUPPORTED?
   GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  s->stage=7;
 }
 if(s->stage==7){
  gh_player_target target;
  if(!s->enemy_focus_fallback){
   s->enemy_focus_previous=(uint8_t)(w(p+0x690)!=0);
   gh_player_frame_status e=enemy_focus(s,s->enemy_focus_previous?26:24,
    number(s->enemy_focus_previous?0x3f1c61aa:0x3f060a92),&target);
   if(e)return e;
   /* Canonical handle stands in for the original actor pointer word. */
   put(p+0x690,target.handle);
   s->enemy_focus_fallback=(uint8_t)(!target.handle);
  }
  if(s->enemy_focus_fallback){
   gh_player_frame_status e=enemy_focus(s,s->enemy_focus_previous?10:8,number(0x40490fdb),&target);
   if(e)return e;
   put(p+0x690,target.handle);
   s->enemy_focus_fallback=0;
  }
  s->stage=8;
 }
 if(s->stage==8){uint32_t scalar;if(!elf_word(b,w(p+0x5d8)==4?0x3bd46c:0x3bd468,&scalar))return fail(s,0x109280,GH_PLAYER_FRAME_ARGUMENT);put(p+0x15f0,scalar);s->stage=9;}
 if(s->stage==9){gh_player_frame_status e=heat_frame(s);if(e)return e;s->stage=10;}
 if(s->stage==10){gh_player_frame_status e=damage_frame(s);if(e)return e;put(p+0x15f4,w(p+0x15f4)&0xfe000076u);s->stage=11;}
 if(s->stage==11){if(!b->solids)return fail(s,0x1351d8,GH_PLAYER_FRAME_UNSUPPORTED);gh_solid_unit_status e=gh_collision_solid_set_enabled_001351d8(b->solids,&b->player->view,1,&pc);if(e)return fail(s,pc?pc:0x1351d8,GH_PLAYER_FRAME_CALLEE);put(p+0x250,w(p+0x250)&~0x10000u);s->stage=12;}
 if(s->stage==12){gh_player_frame_status e=reset_give(s);if(e)return e;put(p+0x1610,0x3f800000);s->stage=13;}
 if(s->stage==13){uint32_t selector,delta;if(!elf_word(b,0x3bd0b8+(uint32_t)p[0x2f4]*8,&delta)||!elf_word(b,0x3bd0bc+(uint32_t)p[0x2f4]*8,&selector))return fail(s,0x10931c,GH_PLAYER_FRAME_ARGUMENT);if((int16_t)(delta>>16)>=0)return fail(s,0x10931c,GH_PLAYER_FRAME_UNSUPPORTED);if((int16_t)delta!=0)return fail(s,0x10931c,GH_PLAYER_FRAME_UNSUPPORTED);s->state_entry=selector;s->stage=14;}
 if(s->stage==14&&s->state_entry==0x10b6b0){if(p[0x2f5]>0x46)p[0x2f5]=0;uint32_t method,descriptor;if(!elf_word(b,0x3bd0f0+(uint32_t)p[0x2f5]*8,&descriptor)||!elf_word(b,0x3bd0f4+(uint32_t)p[0x2f5]*8,&method))return fail(s,0x10b6b0,GH_PLAYER_FRAME_ARGUMENT);if((int16_t)(descriptor>>16)>=0||(int16_t)descriptor)return fail(s,0x10b6b0,GH_PLAYER_FRAME_UNSUPPORTED);s->state_entry=method;s->stage=15;}
 if(s->stage==14&&s->state_entry==0x122c48){
  const unsigned selector=p[0x2f5];
  if((b->work->room!=0x100&&b->work->room!=0x102&&b->work->room!=0x103&&
      b->work->room!=0x104&&b->work->room!=0x105)||
     selector>1||(int16_t)half(p+0x54a)>0)
   return fail(s,0x122c48,GH_PLAYER_FRAME_UNSUPPORTED);
  p[0x648]=0;put(p+0x640,0);
  if(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698))
   return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
  if(b->aux_6a0||w(p+0x6a0))return fail(s,0x12b928,GH_PLAYER_FRAME_UNSUPPORTED);
  uint32_t descriptor,method;
  const uint32_t slot=0x3bd458+8*selector;
  if(!elf_word(b,slot,&descriptor)||!elf_word(b,slot+4,&method)||
     descriptor!=0xffff0000u||method!=(selector?0x122e90u:0x122d18u))
   return fail(s,0x122c48,GH_PLAYER_FRAME_UNSUPPORTED);
  s->state_entry=method;s->stage=15;
 }
 if(s->stage==14&&s->state_entry==0x11a820){
  uint32_t method,descriptor;
  const unsigned selector=p[0x2f5];
  const int r104_grab=selector==9&&b->work->room==0x104;
  if(selector!=0&&selector!=1&&(selector!=2||b->work->room!=0x103)&&selector!=3&&selector!=4&&
     selector!=5&&selector!=6&&selector!=8&&!r104_grab&&selector!=0xa&&selector!=0xb&&selector!=0xf)
   return fail(s,0x11a820,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!b->auxiliary_bindings_known||b->aux_6a0||w(p+0x6a0))
   return fail(s,0x12b928,GH_PLAYER_FRAME_UNSUPPORTED);
  p[0x648]=0;put(p+0x640,0);
  sh(p+0x5e0,0);sh(p+0x5e2,0);p[0x1603]=0;sh(p+0x1618,0);
  const uint32_t slot=0x3bd328+8*selector;
  const uint32_t expected=selector==0?0x11a8f8:selector==1?0x11aaa0:selector==2?0x11ac80:selector==3?0x11add8:selector==4?0x11b280:
     selector==5?0x11b3e8:selector==6?0x11b5f8:selector==8?0x11baf8:r104_grab?0x11bf20:
     selector==0xa?0x11c348:selector==0xb?0x11c608:0x11d4b0;
  if(!elf_word(b,slot,&descriptor)||!elf_word(b,slot+4,&method)||
     descriptor!=0xffff0000u||method!=expected)
   return fail(s,0x11a820,GH_PLAYER_FRAME_UNSUPPORTED);
  s->state_entry=method;s->stage=15;
 }
 if(s->stage==15&&s->state_entry==0x117600){
  gh_player_frame_status status=escape_grab_00117600(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x11b3e8){
  const gh_player_frame_status status=hurt_r103_em14_sphere_0011b3e8(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x11d4b0){
  const gh_player_frame_status status=hurt_r103_boss_throw_0011d4b0(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&(s->state_entry==0x11baf8||s->state_entry==0x11bf20)){
  gh_player_frame_status status=hurt_paired_grab(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x11c348){
  gh_player_frame_status status=hurt_grab_0011c348(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&(s->state_entry==0x122d18||s->state_entry==0x122e90)){
  const int fall_pose=s->state_entry==0x122e90;
  put(p+0x54c,0x41f00000);
  if(p[0x2f6]>=2)s->stage=16;
  else if(s->hurt_phase==0&&p[0x2f6]==1)s->hurt_phase=8;
  if(s->stage==15&&s->hurt_phase==0){
   gh_player_frame_status status=hurt_sound(s,0xd6);
   if(status)return status;
   s->hurt_phase=fall_pose?1:3;
  }
  if(s->hurt_phase==1){
   gh_player_frame_status status=hurt_sound(s,0x123);
   if(status)return status;
   s->hurt_phase=2;
  }
  if(s->hurt_phase==2){
   gh_player_frame_status status=hurt_vibration_0012c348(s,3);
   if(status)return status;
   s->hurt_phase=3;
  }
  if(s->hurt_phase==3){
   if(!b->death_fade)return fail(s,0x2ca718,GH_PLAYER_FRAME_UNSUPPORTED);
   gh_player_frame_status status=b->death_fade(b->damage_context,&pc);
   if(status)return fail(s,pc?pc:0x2ca718,status);
   s->hurt_phase=4;
  }
  if(s->hurt_phase==4){
   gh_player_frame_status status=damage_score_001fa280(s,1000);
   if(status)return status;
   s->hurt_phase=fall_pose?5:12;
  }
  if(s->hurt_phase==12){
   gh_player_frame_status status=hurt_vibration_0012c348(s,3);
   if(status)return status;
   s->hurt_phase=5;
  }
  if(s->hurt_phase==5){
   const size_t rows[]={0x158,0x4fc,0x164,0x504,0x50c};
   const int devil_fall=fall_pose&&b->work->room==0x100&&p[0x2f7]==1;
   const int wide_fall=fall_pose&&b->work->room==0x102&&p[0x2f7]==2;
   const int sweep_fall=fall_pose&&b->work->room==0x104&&p[0x2f7]==5;
   if(!b->model||!b->resource||b->resource_bytes<(fall_pose?0x514u:0x174u)||
      !b->initialize_motion||(p[0x2f7]&&!devil_fall&&!wide_fall&&!sweep_fall))
    return fail(s,fall_pose?0x123040:0x122d64,GH_PLAYER_FRAME_UNSUPPORTED);
   size_t row=0x16c;
   if(devil_fall)row=0x158; /* 122E90 variant 1 uses the fixed fall clip. */
   else if(wide_fall){
    uint32_t target;
    if(!elf_word(b,0x41c798,&target)||target!=0x123000u)
     return fail(s,0x123000,GH_PLAYER_FRAME_UNSUPPORTED);
    row=0x4fc;
   }else if(sweep_fall){
    uint32_t target;
    if(!elf_word(b,0x41c7a4,&target)||target!=0x123030u)
     return fail(s,0x123030,GH_PLAYER_FRAME_UNSUPPORTED);
    row=0x504;
   }else if(fall_pose){
    uint32_t value;
    if(gh_game_mt_next_0030f348(&b->init->random,&value))
     return fail(s,0x31cfe0,GH_PLAYER_FRAME_ARGUMENT);
    row=rows[value%5u];
   }
   s->pending_motion=(gh_player_motion_request){.player=b->player,.model=b->model,
      .work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,
      .motion_offset=w(b->resource+row),.sequence_candidate_offset=w(b->resource+row+4),
      .blend_frames=2,.release_previous_sequence=fall_pose,.rate=b->rate};
   s->hurt_phase=fall_pose?6:7;
  }
  if(s->hurt_phase==6){
   if(!b->take_binding_known)return fail(s,0x123070,GH_PLAYER_FRAME_UNSUPPORTED);
   if(b->take_object){
    if(!b->take_object->bytes||b->take_object->size<0x1c)
     return fail(s,0x123070,GH_PLAYER_FRAME_ARGUMENT);
    const uint8_t *hit=b->take_object->bytes;
    if(gh_game_angle_001f8180(number(w(hit+0x10)),number(w(hit+0x18)),&angle,b->numeric_policy,&pc))
     return fail(s,pc?pc:0x1f8180,GH_PLAYER_FRAME_UNSUPPORTED);
    volatile float rotated=angle+number(0x40490fdb);
    if(gh_id_angle_reference_001f7d48(rotated,&angle))
     return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);
    sf(p+0x104,angle);
   }
   s->hurt_phase=7;
  }
  if(s->hurt_phase==7){
   gh_player_init_status status=b->initialize_motion(b->motion_context,&s->pending_motion,&pc);
   if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
       GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
   put(p+0x15b0,fall_pose?40:130);
   p[0x2f6]=1;
   s->hurt_phase=8;
  }
  if(s->hurt_phase==8){
   if(!b->grab_camera)return fail(s,0x124ec0,GH_PLAYER_FRAME_UNSUPPORTED);
   gh_player_frame_status status=b->grab_camera(b->motion_context,b->model,&pc);
   if(status)return fail(s,pc?pc:0x124ec0,status);
   if(fall_pose)put(p+0x15f4,w(p+0x15f4)|2u);
   const uint32_t timer=w(p+0x15b0);
   if(timer)put(p+0x15b0,timer-1);
   else b->work->flags|=8u;
   s->hurt_phase=9;
  }
  if(s->hurt_phase==9){
   if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_motion_request request={.player=b->player,.work=b->work,
       .model=b->model,.resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
   int ended;
   gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&ended,&pc);
   if(status)return fail(s,pc?pc:0x157c18,status);
   if(ended){b->work->flags|=8u;p[0x2f6]=2;}
   s->hurt_phase=10;
  }
  if(s->hurt_phase==10){
   if(gh_player_root_rotation_0014f9f0(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
    return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
   s->hurt_phase=11;
  }
  if(s->hurt_phase==11){
   if(gh_player_root_translation_0014f928(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
    return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
   s->stage=16;
  }
 }
 if(s->stage==15&&s->state_entry==0x11c608){
  const gh_title_player_view *attacker=b->grab_attacker;
  if(!attacker||!attacker->bytes||attacker->size<0x642||p[0x2f6]>7)
   return fail(s,0x11c608,GH_PLAYER_FRAME_UNSUPPORTED);
  const uint8_t *source=attacker->bytes;
  const unsigned stamp=half(source+0x640);
  const uint32_t dependency=w(source+0x564);
  const int area11=b->work->room==0x102&&dependency==0x202&&stamp==0x17&&
     attacker->size>=0x16c8&&!source[0x1650]&&w(source+0x16c4)==2;
  const int side=b->work->room==0x101&&attacker->size>0x1650&&
     attacker->original_vtable214==0x4479c8&&source[0x1650]==6&&
     ((dependency==0x202&&stamp==0x27)||(dependency==0x203&&stamp==0x28));
  const int arena=r103_grab_attacker(b->work,attacker,0x203);
  const int arena_boss=r103_grab_attacker(b->work,attacker,0x214);
  const int wave_em02=b->work->room==0x100&&dependency==0x202&&
     attacker->original_vtable214==0x4479c8&&w(source+0x214)==0x4479c8&&
     attacker->size>=0x16c8&&!source[0x1650]&&
     ((stamp==29&&w(source+0x16c4)==2)||(stamp==32&&!w(source+0x16c4)));
  if((!arena_boss&&!wave_em02&&(p[0x2f6]==4||p[0x2f6]==5))||
     (!area11&&!side&&!arena&&!arena_boss&&!wave_em02&&(b->work->room!=0x100||dependency!=0x203||stamp!=19)))
   return fail(s,0x11c608,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!s->hurt_phase){
   put(p+0x54c,0x40a00000);
   put(p+0x250,w(p+0x250)|0x10000u);
   if(!b->solids||gh_collision_solid_set_enabled_001351d8(b->solids,&b->player->view,0,&pc))
    return fail(s,pc?pc:0x1351d8,GH_PLAYER_FRAME_UNSUPPORTED);
   b->init->enemies[0x53c]=2;
   s->hurt_phase=1;
  }
  if(s->hurt_phase==1){
   if(p[0x2f6]==0||p[0x2f6]==2){
    gh_player_frame_status status=place_grab_001299f0(s,attacker,0xbd98c7e3u,0x3f231f8au);
    if(status)return status;
   }
   s->hurt_phase=2;
  }
  if(s->hurt_phase==2&&!(p[0x2f6]&1u)){
   if(!p[0x2f6]&&(!b->auxiliary_bindings_known||b->aux_698||w(p+0x698)))
    return fail(s,0x12ac28,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_object_view *resource=&b->grab_resource;
   const unsigned phase=p[0x2f6];
   const unsigned row=phase==4&&(int16_t)half(p+0x54a)<1?0x810:
      phase>=4?0x800:phase==2?0x7f8:0x808;
   if(!b->initialize_motion||!resource->bytes||resource->size<row+8)
    return fail(s,0x23a300,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_motion_request request={.player=b->player,.model=b->model,
     .work=b->work,.resource=resource->bytes,.resource_bytes=resource->size,
     .motion_offset=w(resource->bytes+row),
     .sequence_candidate_offset=w(resource->bytes+row+4),
     .blend_frames=phase>=4?3:0,.release_previous_sequence=1,.rate=b->rate};
   gh_player_init_status status=b->initialize_motion(b->motion_context,&request,&pc);
   if(status)return fail(s,pc?pc:0x2a8578,status==GH_PLAYER_INIT_UNSUPPORTED?
      GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
   if(phase==4){
    put(p+0x15b0,30);
    s->hurt_phase=14;
   }else if(phase==6){
    p[0x2f6]=7;
    s->hurt_phase=4;
   }else if(p[0x2f6]==2){
    put(p+0x15b0,5);
    p[0x2f6]=3;
    s->hurt_phase=4;
   }else{
    sh(p+0x56e,15);
    s->hurt_phase=3;
   }
  }
  if(s->hurt_phase==3){
   gh_player_frame_status status=damage_score_001fa280(s,320);
   if(status)return status;
   p[0x2f6]=1;
   s->hurt_phase=4;
  }
  if(s->hurt_phase==14){
   if(!b->death_fade)return fail(s,0x2ca718,GH_PLAYER_FRAME_UNSUPPORTED);
   gh_player_frame_status status=b->death_fade(b->damage_context,&pc);
   if(status)return fail(s,pc?pc:0x2ca718,status);
   s->hurt_phase=15;
  }
  if(s->hurt_phase==15){
   gh_player_frame_status status=damage_score_001fa280(s,1000);
   if(status)return status;
   p[0x2f6]=5;
   s->hurt_phase=4;
  }
  if(p[0x2f6]==1&&half(p+0x56e)&&s->hurt_phase<=4){
   float position[3],offset[3];
   memcpy(position,b->player->view.alias_f0,12);
   memcpy(offset,attacker->bytes+0x550,12);
   for(unsigned i=0;i<3;i++){
    volatile float next=position[i]+offset[i];
    if(!isfinite(next))return fail(s,0x11c738,GH_PLAYER_FRAME_NUMERIC);
    position[i]=next;
   }
   sh(p+0x56e,(uint16_t)(half(p+0x56e)-1));
   memcpy(b->player->view.alias_f0,position,12);
  }
  if(s->hurt_phase<=4)s->hurt_phase=5;
  if(s->hurt_phase==5){
   if(!b->grab_camera)return fail(s,0x124ec0,GH_PLAYER_FRAME_UNSUPPORTED);
   gh_player_frame_status status=b->grab_camera(b->motion_context,b->model,&pc);
   if(status)return fail(s,pc?pc:0x124ec0,status);
   put(p+0x15f4,w(p+0x15f4)|2u);
   if(p[0x2f6]==5)sh(p+0x434,half(p+0x434)|8u);
   s->hurt_phase=6;
  }
  if(s->hurt_phase==6){
   if(!b->advance_motion)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);
   const gh_player_motion_request request={.player=b->player,.work=b->work,
    .model=b->model,.resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};
   int ended=0;
   gh_player_frame_status status=b->advance_motion(b->motion_context,&request,&ended,&pc);
   if(status)return fail(s,pc?pc:0x157c18,status);
   if(p[0x2f6]==1){if(ended)p[0x2f6]=2;}
   else if(p[0x2f6]==5){if(ended)b->work->flags|=8u;}
   else if(p[0x2f6]==7){if(ended)s->hurt_phase=11;}
   else{
    if(ended)put(p+0x15b0,5);
    const uint32_t remaining=w(p+0x15b0);
    if(remaining)put(p+0x15b0,remaining-1);
    if(remaining==1)s->hurt_phase=9;
   }
   if(s->hurt_phase==6)s->hurt_phase=7;
  }
  if(s->hurt_phase==9){
   /* The paired throw applies damage directly from the live grabber's
    * +76C scalar. This original call has no Take entry. */
   if(attacker->size<0x770)return fail(s,0x11c828,GH_PLAYER_FRAME_ARGUMENT);
   fenv_t env;
   if(feholdexcept(&env))return fail(s,0x11c82c,GH_PLAYER_FRAME_NUMERIC);
   if(fesetround(FE_TOWARDZERO)){
    (void)fesetenv(&env);
    return fail(s,0x11c82c,GH_PLAYER_FRAME_NUMERIC);
   }
   volatile float scaled=number(w(attacker->bytes+0x76c))*10.0f;
   if(fesetenv(&env)||!isfinite(scaled)||scaled<0||scaled>=2147483648.0f)
    return fail(s,0x11c830,GH_PLAYER_FRAME_NUMERIC);
   gh_player_frame_status status=damage_hp_0012c0f8(s,(int32_t)scaled);
   if(status)return status;
   s->hurt_phase=10;
  }
  if(s->hurt_phase==10){
   gh_player_frame_status status=hurt_vibration_0012c348(s,0);
   if(status)return status;
   s->hurt_phase=7;
  }
  if(s->hurt_phase==11){
   if(w(p+0x15f4)&2u){
    if(!b->damage_camera_exit)return fail(s,0x124f60,GH_PLAYER_FRAME_UNSUPPORTED);
    put(p+0x15f4,w(p+0x15f4)&~2u);
    s->hurt_phase=12;
   }else s->hurt_phase=13;
  }
  if(s->hurt_phase==12){
   gh_player_frame_status status=b->damage_camera_exit(b->damage_context,b,&pc);
   if(status)return fail(s,pc?pc:0x124f60,status);
   s->hurt_phase=13;
  }
  if(s->hurt_phase==13){
   p[0x2f4]=p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
   s->hurt_phase=7;
  }
  if(s->hurt_phase==7){
   if(gh_player_root_rotation_0014f9f0(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
    return fail(s,pc?pc:0x14f9f0,GH_PLAYER_FRAME_UNSUPPORTED);
   s->hurt_phase=8;
  }
  if(gh_player_root_translation_0014f928(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc))
   return fail(s,pc?pc:0x14f928,GH_PLAYER_FRAME_UNSUPPORTED);
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x11a8f8){
  gh_player_frame_status status=hurt_small_0011a8f8(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x11aaa0){gh_player_frame_status e=hurt_reaction_0011aaa0(s);if(e)return e;s->stage=16;}
 if(s->stage==15&&s->state_entry==0x11ac80){gh_player_frame_status e=hurt_deflect_0011ac80(s);if(e)return e;s->stage=16;}
 if(s->stage==15&&s->state_entry==0x11add8){gh_player_frame_status e=hurt_fall_0011add8(s);if(e)return e;s->stage=16;}
 if(s->stage==15&&s->state_entry==0x11b280){gh_player_frame_status e=hurt_rise_0011b280(s);if(e)return e;s->stage=16;}
 if(s->stage==15&&s->state_entry==0x11b5f8){gh_player_frame_status e=hurt_stagger_0011b5f8(s);if(e)return e;s->stage=16;}
 if(s->stage==15&&s->state_entry==0x11a530){
  gh_player_frame_status status=event_wait_0011a530(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x114460){
  const gh_player_frame_status status=taunt_00114460(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&(s->state_entry==0x114120||s->state_entry==0x1142e0)){
  const gh_player_frame_status status=heat_transition_00114120_001142e0(s,s->state_entry==0x1142e0);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x110e98){gh_player_frame_status e=throw_box_00110e98(s);if(e)return e;s->stage=16;}
 if(s->stage==15&&s->state_entry==0x115310){gh_player_frame_status e=interact_00115310(s);if(e)return e;s->stage=16;}
 if(s->stage==15&&s->state_entry==0x112a48){
  const gh_player_frame_status status=roulette_shot_00112a48(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x114608){
  const gh_player_frame_status status=roulette_00114608(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x10be18){gh_player_frame_status e=idle_frame(s);if(e)return e;s->stage=16;}
 if(s->stage==15&&s->state_entry==0x10cd38){
  gh_player_frame_status status=dodge_0010cd38(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x10d0e0){
  const gh_player_frame_status status=forward_evade_0010d0e0(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x10c300){gh_player_frame_status e=gh_game_player_walk_0010c300(&s->walk,b);if(e)return fail(s,s->walk.frontier?s->walk.frontier:0x10c300,e);s->stage=16;}
 if(s->stage==15&&s->state_entry==0x10d3a8){
  if(!b->square_attack)return fail(s,0x10d3a8,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_player_frame_status e=b->square_attack(b->square_context,b,&pc);
  if(e)return fail(s,pc?pc:0x10d3a8,e);
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x10cb10){
  const gh_player_frame_status status=quick_turn_0010cb10(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x10c930){
  gh_player_frame_status status=player_turn_0010c930(s);
  if(status)return status;
  s->stage=16;
 }
 if(s->stage==15&&s->state_entry==0x10c5a8){gh_player_frame_status e=gh_game_player_run_0010c5a8(&s->run,b);if(e)return fail(s,s->run.frontier?s->run.frontier:0x10c5a8,e);s->stage=16;}
 if(s->stage==15&&s->state_entry==0x10c930){
  gh_player_frame_status e=gh_game_player_run_backwards_0010c930(&s->run,b);
  if(e)return fail(s,s->run.frontier?s->run.frontier:0x10c930,e);
  s->stage=16;
 }
 if(s->stage==16){
  const gh_effect_create_bindings effect={.work=b->work,.random=&b->init->random,.model=b->model,
     .storage=b->effects?*b->effects:(gh_effect_release_bindings){0}};
  gh_player_post_status e=gh_player_state_effect_with_create_0012bb38(&s->post_state,&b->player->view,
     b->effects,b->effects?&effect:NULL);if(e)return fail(s,s->post_state.frontier?s->post_state.frontier:0x12bb38,e==GH_PLAYER_POST_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);s->stage=17;}
 if(s->stage==17){gh_player_post_status e=gh_player_mesh_visibility_resource_001236e8(&s->post_state,b->node_cache,b->save,b->resource,b->resource_bytes);if(e)return fail(s,s->post_state.frontier?s->post_state.frontier:0x1236e8,e==GH_PLAYER_POST_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);s->stage=18;}
 if(s->stage==18){uint32_t ignored;gh_player_post_status e=gh_player_camera_mode_00123908(b->init,&ignored);if(e)return fail(s,0x138f88,GH_PLAYER_FRAME_CALLEE);s->stage=19;}
 if(s->stage==19){if(!b->player->view.alias_f0)return fail(s,0x1348e8,GH_PLAYER_FRAME_ARGUMENT);memcpy(s->previous_world_position,b->player->view.alias_f0,12);s->stage=20;}
 if(s->stage==20){if(!b->solids)return fail(s,0x1348e8,GH_PLAYER_FRAME_UNSUPPORTED);int original_hit=0;gh_solid_unit_status e=gh_collision_solid_step_resume_001348e8(b->solids,&b->player->view,GH_STATIC_BINARY32_REFERENCE,&s->solid_frame,&original_hit,&pc);if(e)return fail(s,pc?pc:0x1348e8,e==GH_SOLID_UNIT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);s->stage=21;}
 if(s->stage==21){
  if(!(w(p+0x15f4)&0x3000)){
   if(!b->actors)return fail(s,0x132da0,GH_PLAYER_FRAME_UNSUPPORTED);
   s->sphere_call.work=b->work;
   gh_player_terrain_bindings tb={b->actors,&b->player->view,gh_terrain_sphere_query_001321d8,&s->sphere_call};int hit=0;
   /*1094B0 passes one attribute-output identity, t0=0, t1=NULL.*/
   gh_scene_ray_status e=gh_player_terrain_wall_00132da0(&s->wall_call,&tb,.4f,(w(p+0x15f4)&0x80)?0x224:0x200,s->terrain_position,&s->wall_report,0,NULL,&hit,&pc);
   if(e)return fail(s,pc?pc:0x132da0,e==GH_RAY_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
   if(hit){memcpy(b->player->view.alias_f0,s->terrain_position,4);memcpy(b->player->view.alias_f0+8,s->terrain_position+2,4);}
  }s->stage=22;
 }
 if(s->stage==22){put(p+0x474,0);put(p+0x480,0);put(p+0x484,0x3f800000);put(p+0x488,0);s->stage=23;}
 if(s->stage==23){
  if(!(w(p+0x15f4)&0x80)){
   if(!b->actors||!b->player->view.alias_f0)return fail(s,0x1329c0,GH_PLAYER_FRAME_UNSUPPORTED);
   if(!s->floor_call.active)s->floor_height=number(w(b->player->view.alias_f0+4));
   gh_player_terrain_bindings tb={.tables=b->actors,.actor=&b->player->view};int hit=0;
   gh_scene_ray_status e=gh_player_terrain_floor_001329c0(&s->floor_call,&tb,20,s->terrain_position,s->terrain_normal,&s->floor_report,&hit,&pc);
   if(e)return fail(s,pc?pc:0x1329c0,e==GH_RAY_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
   float floor_y=hit?s->terrain_position[1]:s->floor_height,current_y=number(w(b->player->view.alias_f0+4)),velocity=number(w(p+0x1608));
   if(!isfinite(floor_y)||!isfinite(current_y)||!isfinite(velocity))return fail(s,0x10954c,GH_PLAYER_FRAME_NUMERIC);
   if(hit&&(!s->floor_report.attributes||s->floor_report.attribute_bytes<2))return fail(s,0x109534,GH_PLAYER_FRAME_ARGUMENT);
   volatile float threshold=floor_y+.4f;
   if(velocity==0&&current_y<threshold){current_y=floor_y;velocity=0;}
   else {float gravity=number(w(p+0x1604));if(!isfinite(gravity))return fail(s,0x10959c,GH_PLAYER_FRAME_NUMERIC);velocity=sub(velocity,gravity);volatile float next=current_y+velocity;current_y=next;}
   if(current_y<floor_y){current_y=floor_y;velocity=0;}
   if(!isfinite(current_y)||!isfinite(velocity))return fail(s,0x1095cc,GH_PLAYER_FRAME_NUMERIC);
   /*109534 is LHU, zero extended into the word474; not a pointer/SW load.*/
   if(hit){put(p+0x474,half(s->floor_report.attributes));memcpy(p+0x480,s->terrain_normal,12);}
   else {put(p+0x480,0);put(p+0x484,0x3f800000);put(p+0x488,0);}
   sf(b->player->view.alias_f0+4,current_y);sf(p+0x1608,velocity);
  }s->stage=24;
 }
 if(s->stage==24){
  /*14FB48 consumes the SAME ordered eight SEQ effect slots. Keep unknown
   * positive producers precise; skip and release branches are original work.*/
  while(s->sequence_effect_index<half(p+0x3ae)){
   if(s->sequence_effect_index>=8)return fail(s,0x14fb48,GH_PLAYER_FRAME_ARGUMENT);
   const uint8_t*entry=p+0x3b0+s->sequence_effect_index*12;uint8_t flags=entry[9];
   if(!((flags&4)&&p[0x436])){
    if(flags&0x80){
     if(!b->effects)return fail(s,0x306200,GH_PLAYER_FRAME_UNSUPPORTED);uint32_t raw=w(entry+4);int32_t kind;memcpy(&kind,&raw,4);unsigned slot=0;
     gh_effect_status e=gh_game_effect_release_00306200(b->effects,(flags&1)?&b->player->view:NULL,kind,2,&pc,&slot);
     if(e)return fail(s,pc?pc:0x306200,GH_PLAYER_FRAME_CALLEE);
    }else{
     const uint32_t mode=w(entry+4);
     const int rise=entry[1]==0x3e&&mode==UINT32_MAX;
     const int grab=entry[1]==0x42&&mode==0&&s->state_entry==0x11c348;
     const int heat_finish=entry[1]==0x28&&mode==0&&s->state_entry==0x1142e0;
     const int em0a_grab=b->work->room==0x103&&s->state_entry==0x11baf8&&p[0x2f6]==3&&
       entry[1]==0x35&&mode==UINT32_MAX&&flags==1&&r103_grab_attacker(b->work,b->grab_attacker,0x20a);
     const int boss_throw=b->work->room==0x103&&s->state_entry==0x11d4b0&&
       p[0x2f6]==1&&p[0x2f7]==1&&entry[0]==0x6c&&entry[1]==0x3b&&mode==UINT32_MAX&&flags==1;
     const int roulette_launch=(b->work->room==0x103||b->work->room==0x104)&&s->state_entry==0x112a48&&
       p[0x2f6]==1&&!p[0x2f7]&&entry[1]==0x39&&mode==0;
     const int heat=entry[1]==0x22&&mode==UINT32_MAX&&s->state_entry==0x114120&&flags==1&&
       (b->work->room>=0x100&&b->work->room<=0x103);
     const int forward_evade=b->work->room==0x103&&s->state_entry==0x10d0e0&&
       p[0x2f6]==1&&!p[0x2f7]&&entry[1]>=0x29&&entry[1]<=0x2b&&mode==0;
     const int held_impact=b->work->room==0x104&&s->state_entry==0x11bf20&&p[0x2f6]==3&&!p[0x2f7]&&
       entry[0]==0xa8&&entry[1]==0x19&&!entry[8]&&mode==UINT32_MAX&&flags==1;
     if(!b->effect_create||(!held_impact&&(entry[8]!=0xff||
        (!boss_throw&&(entry[0]!=0||(!(flags==7&&(rise||grab||heat_finish||roulette_launch||forward_evade))&&!em0a_grab&&!heat)))||
        (half(p+0x434)&2u))))
      return fail(s,entry[8]!=0xff?0x306900:(flags&8)?0x306838:0x306528,GH_PLAYER_FRAME_UNSUPPORTED);
     const gh_player_frame_effect_request request={.actor=&b->player->view,.kind=entry[1],
       .parameter_bits=entry[0],.mode=mode,.selector=-1,.sequence_scale=held_impact?0x3f800000u:w(p+0x114)};
     if(!request.sequence_scale)return fail(s,0x306528,GH_PLAYER_FRAME_UNSUPPORTED);
     gh_player_frame_status status=b->effect_create(b->effect_context,&request,&pc);
     if(status)return fail(s,pc?pc:0x306528,status);
    }
   }s->sequence_effect_index++;
  }s->stage=25;
 }
 if(s->stage==25){gh_player_ground_status e=gh_player_ground_pose_00108ab0(&s->ground_pose,&b->player->view,b->model,b->ground_globals);if(e)return fail(s,s->ground_pose.frontier?s->ground_pose.frontier:0x1095f4,e==GH_GROUND_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);s->stage=26;}
 if(s->stage==26){
  if(s->model_failure_latched)return s->model_failure_status;
  if(!b->model||b->model->root.bytes!=p)return fail(s,0x149eb8,GH_PLAYER_FRAME_UNSUPPORTED);
  gh_actor_model_status e=gh_actor_model_update_hierarchy_00149eb8(b->model,GH_STATIC_BINARY32_REFERENCE);
  if(e){s->model_failure_latched=1;s->model_failure_status=e==GH_ACTOR_MODEL_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;return fail(s,b->model->frontier?b->model->frontier:0x149eb8,s->model_failure_status);}s->stage=27;
 }
 if(s->stage==27){
  gh_motion_solve_bindings mb={.model=b->model,.work=b->work,.actors=b->actors,.ik_globals=b->ik_globals,.atan_coeff=b->atan_coeff};
  gh_actor_motion_status e=gh_actor_motion_solve_00152a20(&s->pose_solve,&mb,GH_STATIC_BINARY32_REFERENCE,&pc);if(e)return fail(s,pc?pc:0x152a20,e==GH_ACTOR_MOTION_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);s->stage=28;
 }
 if(s->stage==28){
  gh_actor_model_status e=gh_actor_finish_model_resume_0014a038(&s->finish_model,b->model,GH_STATIC_BINARY32_REFERENCE);
  if(e)return fail(s,b->model->frontier?b->model->frontier:0x14a038,e==GH_ACTOR_MODEL_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);s->stage=29;
 }
 if(s->stage==29){
  gh_player_opacity_bindings ob={.init=b->init,.camera=b->opacity_camera?b->opacity_camera:b->camera,.globals=b->opacity_globals,.aux_68c=b->aux_68c,.auxiliary_binding_known=b->auxiliary_68c_binding_known,.policy=b->numeric_policy};
  gh_player_opacity_status e=gh_player_opacity_00123160(&s->opacity_call,&b->player->view,&ob);if(e)return fail(s,s->opacity_call.frontier?s->opacity_call.frontier:0x123160,e==GH_OPACITY_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);s->stage=30;
 }
 if(s->stage==30){
  gh_player_frame_tail_bindings tb={.player=b->player,.work=b->work,.init=b->init,.save=b->save,.save_binding_known=1,.previous_world_position=s->previous_world_position,
   .target_644=b->target_644,.aux_698=b->aux_698,.aux_6a0=b->aux_6a0,.auxiliary_bindings_known=b->auxiliary_bindings_known,.cockpit_flags_1ca0=b->cockpit_flags_1ca0,.effects=b->effects,.policy=GH_STATIC_BINARY32_REFERENCE};
  gh_player_frame_tail_status e=gh_game_player_frame_tail_001097e0(&s->tail,&tb);if(e)return fail(s,s->tail.frontier?s->tail.frontier:0x1097e0,e==GH_PLAYER_TAIL_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  if(!s->tail.complete)return fail(s,0x1097e0,GH_PLAYER_FRAME_CALLEE);s->stage=31;s->bound=0;s->frontier=0;return GH_PLAYER_FRAME_OK;
 }
 return fail(s,s->state_entry?s->state_entry:0x10931c,GH_PLAYER_FRAME_UNSUPPORTED);
}
gh_player_frame_status gh_game_player_frame_00108ab0(gh_game_player_frame*s,const gh_player_frame_bindings*b){if(!s||!b||!b->player||!b->player->view.bytes||b->player->view.size<GH_TITLE_PLAYER_BYTES||!b->work||!b->init)return GH_PLAYER_FRAME_ARGUMENT;if(s->bound&&!same(&s->b,b))return GH_PLAYER_FRAME_BUSY;if(!s->bound){s->b=*b;s->bound=1;s->stage=0;s->effect_pending=0;s->heat_stage=0;s->damage_stage=0;s->damage_region_hit=0;s->hurt_phase=s->script_wait_phase=0;s->give_index=s->give_phase=0;s->state_entry=0;s->idle_phase=0;s->pending_motion=(gh_player_motion_request){0};s->motion_step_complete=s->idle_end_prefix_done=s->idle_end_needs_random=0;s->input_started=s->post_take_index=0;s->input_result=0;s->post_state=(gh_game_player_post_state){0};s->wall_call=(gh_player_terrain_call){0};s->floor_call=(gh_player_terrain_call){0};s->sphere_call=(gh_terrain_sphere_call){.work=b->work};s->wall_report=(gh_scene_ray_report){0};s->floor_report=(gh_scene_ray_report){0};s->ground_pose=(gh_player_ground_pose_call){0};s->pose_solve=(gh_motion_solve){0};s->model_failure_latched=0;s->sequence_effect_index=0;s->opacity_call=(gh_player_opacity_call){0};s->tail=(gh_game_player_frame_tail){0};s->finish_model=(gh_actor_finish_model_call){0};s->walk=(gh_player_walk_call){0};s->run=(gh_player_run_call){0};}fenv_t env;if(feholdexcept(&env))return GH_PLAYER_FRAME_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_PLAYER_FRAME_NUMERIC;}gh_player_frame_status e=body(s);if(fesetenv(&env))return GH_PLAYER_FRAME_NUMERIC;return e;}
