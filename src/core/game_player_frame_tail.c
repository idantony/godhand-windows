#include "game_player_frame_tail.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static float number(const void*p){float n;memcpy(&n,p,4);return n;}
static void put_float(void*p,float n){memcpy(p,&n,4);}
static gh_player_frame_tail_status fail(gh_game_player_frame_tail*s,uint32_t pc,gh_player_frame_tail_status e){s->frontier=pc;return e;}
static int same(const gh_player_frame_tail_bindings*a,const gh_player_frame_tail_bindings*b){return a->player==b->player&&a->work==b->work&&a->init==b->init&&a->save.bytes==b->save.bytes&&a->save.size==b->save.size&&a->save_binding_known==b->save_binding_known&&a->previous_world_position==b->previous_world_position&&a->target_644==b->target_644&&a->aux_698==b->aux_698&&a->aux_6a0==b->aux_6a0&&a->auxiliary_bindings_known==b->auxiliary_bindings_known&&a->cockpit_flags_1ca0==b->cockpit_flags_1ca0&&a->effects==b->effects&&a->policy==b->policy;}
gh_player_frame_tail_status gh_player_frame_tail_begin(gh_game_player_frame_tail*s){if(!s)return GH_PLAYER_TAIL_ARGUMENT;if(s->bound&&!s->complete)return GH_PLAYER_TAIL_BUSY;memset(s,0,sizeof(*s));return GH_PLAYER_TAIL_OK;}
/* Actual306168(0,kind,2): group+23A/18C and kind+23C/18E,
 * unlike actor-matching306200. Never substitute that different overload. */
static gh_player_frame_tail_status release_effects(gh_game_player_frame_tail*s,unsigned kind){
 const gh_effect_release_bindings*b=s->b.effects;
 if(!b||!b->effects||!b->effects->ready||!b->esp||b->esp_bytes<0x100180||!b->emitters||b->emitter_bytes<0x26060)return fail(s,0x306168,GH_PLAYER_TAIL_ARGUMENT);
 for(;s->effect_bank<2;s->effect_bank++,s->effect_index=0){int emitter=s->effect_bank!=0;uint8_t*base=emitter?b->emitters:b->esp;unsigned count=emitter?256:1024,stride=emitter?0x260:0x400,bits=emitter?0x26000:0x100000,bank=emitter?0x20:0x80,group=emitter?0x18c:0x23a;
  for(;s->effect_index<count;s->effect_index++){unsigned i=s->effect_index,off=(i>>5)*4;uint32_t mask=UINT32_C(0x80000000)>>(i&31);
   if(!(word(base+bits+off)&mask)||(word(base+bits+bank+off)&mask)||(word(base+bits+bank*2+off)&mask))continue;
   const uint8_t*p=base+(size_t)i*stride;if(half(p+group)||half(p+group+2)!=kind)continue;
   return fail(s,emitter?0x302918:0x2fbe50,GH_PLAYER_TAIL_UNSUPPORTED);
  }
 }
 return GH_PLAYER_TAIL_OK;
}
static gh_player_frame_tail_status target_cleanup(gh_game_player_frame_tail*s){
 uint8_t*p=s->b.player->view.bytes;gh_player_tail_target_slot*slot=s->b.target_644;
 if(!s->target_phase){
  /* Masked branch has identical behavior for all target identities. */
  if(word(p+0x15f4)&0x800000u){s->target_phase=4;return GH_PLAYER_TAIL_OK;}
  if(!slot||!slot->known)return fail(s,0x12cc00,GH_PLAYER_TAIL_UNSUPPORTED);
  if(word(p+0x644)!=slot->expected_guest_token)return fail(s,0x12cc00,GH_PLAYER_TAIL_UNSUPPORTED);
  if(!slot->view){if(slot->expected_guest_token)return fail(s,0x12cc00,GH_PLAYER_TAIL_UNSUPPORTED);s->target_phase=4;return GH_PLAYER_TAIL_OK;}
  if(!slot->view->bytes||slot->view->size<0x568)return fail(s,0x12cc2c,GH_PLAYER_TAIL_ARGUMENT);
  s->target_view=slot->view;s->target_token=slot->expected_guest_token;uint32_t type=word(slot->view->bytes+0x564);memcpy(&s->target_kind,&type,4);s->target_phase=s->target_kind==0x26a?1:2;
 }
 if(s->target_phase==4)return GH_PLAYER_TAIL_OK;
 if(!slot||!slot->known||slot->view!=s->target_view||slot->expected_guest_token!=s->target_token||word(p+0x644)!=s->target_token)return fail(s,0x12cc00,GH_PLAYER_TAIL_BUSY);
 if(s->target_phase==1){gh_player_frame_tail_status e=release_effects(s,0x79);if(e)return e;s->effect_bank=0;s->effect_index=0;s->target_phase=2;}
 if(s->target_phase==2){gh_player_frame_tail_status e=release_effects(s,0x61);if(e)return e;s->target_phase=3;}
 if(s->target_phase==3){put(p+0x644,0);slot->view=NULL;slot->expected_guest_token=0;s->target_phase=4;}
 return GH_PLAYER_TAIL_OK;
}
static gh_player_frame_tail_status body(gh_game_player_frame_tail*s,const gh_player_frame_tail_bindings*b){
 if(!s||!b||!b->player||!b->work||!b->init||!b->previous_world_position||!b->player->view.bytes||b->player->view.size<GH_TITLE_PLAYER_BYTES||!b->player->view.alias_f0)return GH_PLAYER_TAIL_ARGUMENT;
 const gh_title_player_view*v=&b->player->view;uintptr_t base=(uintptr_t)v->bytes,alias=(uintptr_t)v->alias_f0;if(alias<base||alias-base>v->size||12>v->size-(alias-base))return GH_PLAYER_TAIL_ARGUMENT;
 if(b->policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x1097f0,GH_PLAYER_TAIL_UNSUPPORTED);
 if(s->bound){if(!same(&s->b,b))return fail(s,0x1097e0,GH_PLAYER_TAIL_BUSY);}else{s->b=*b;s->bound=1;memcpy(s->previous,b->previous_world_position,12);}
 s->frontier=0;if(s->complete)return GH_PLAYER_TAIL_OK;uint8_t*p=v->bytes;
 if(!s->stage){float delta[3];for(unsigned i=0;i<3;i++){float now=number(v->alias_f0+i*4),previous=s->previous[i];if(!isfinite(now)||!isfinite(previous))return fail(s,0x1097f0,GH_PLAYER_TAIL_NUMERIC);volatile float d=now-previous;if(!isfinite(d))return fail(s,0x1097f0,GH_PLAYER_TAIL_NUMERIC);delta[i]=d;}memcpy(p+0x550,delta,12);memcpy(p+0x630,p+0x490,12);memcpy(p+0x490,v->alias_f0,12);s->stage=1;}
 if(s->stage==1){gh_player_frame_tail_status e=target_cleanup(s);if(e)return e;s->stage=2;}
 if(s->stage==2){if(!b->save_binding_known)return fail(s,0x1facb0,GH_PLAYER_TAIL_UNSUPPORTED);if(b->save.bytes&&b->save.size<GH_CORE_SAVE_BYTES)return fail(s,0x1facb0,GH_PLAYER_TAIL_ARGUMENT);if(!b->save.bytes&&b->save.size)return fail(s,0x1facb0,GH_PLAYER_TAIL_ARGUMENT);if(b->save.bytes){if(b->work->flags598&0x8000000u)b->save.bytes[0x154]=13;put(b->save.bytes+0x80,100u+20u*b->save.bytes[0x154]);}s->stage=3;}
 if(s->stage==3){uint32_t maximum=b->save.bytes?word(b->save.bytes+0x80):100;p[0x548]=(uint8_t)maximum;p[0x549]=(uint8_t)(maximum>>8);uint16_t bits=(uint16_t)half(p+0x54a);int16_t hp;memcpy(&hp,&bits,2);if(b->save.bytes)put(b->save.bytes+0x84,(uint32_t)(int32_t)hp);s->stage=4;}
 if(s->stage==4){s->heat_level=0;if(b->save.bytes){if(b->work->flags598&0x8000000u)b->save.bytes[0x155]=5;s->heat_level=b->save.bytes[0x155];}s->heat_maximum=(float)(120u+30u*s->heat_level);s->stage=5;}
 if(s->stage==5){float current=number(b->init->heat+4);if(!s->heat_started){put_float(b->init->heat,s->heat_maximum);s->heat_started=1;}if(!isfinite(current))return fail(s,0x2a9c28,GH_PLAYER_TAIL_NUMERIC);float maximum=s->heat_maximum;if(maximum<current){current=maximum;put_float(b->init->heat+4,current);}volatile float ratio=current/maximum;put(b->init->heat+0x14,ratio<.5f?0:ratio<=.75f?1:2);s->stage=6;}
 if(s->stage==6){if(!b->auxiliary_bindings_known)return fail(s,0x1098a4,GH_PLAYER_TAIL_UNSUPPORTED);if((word(p+0x698)&&!b->aux_698)||(word(p+0x6a0)&&!b->aux_6a0))return fail(s,0x1098a4,GH_PLAYER_TAIL_UNSUPPORTED);if(b->aux_698||b->aux_6a0){if(!b->cockpit_flags_1ca0)return fail(s,0x1098bc,GH_PLAYER_TAIL_UNSUPPORTED);*b->cockpit_flags_1ca0|=1;}s->stage=7;}
 if(s->stage==7){unsigned costume=b->save.bytes?b->save.bytes[0xae]:0;if(costume>=4&&costume<6)put(p+0x674,0);s->stage=8;}
 s->complete=1;return GH_PLAYER_TAIL_OK;
}
gh_player_frame_tail_status gh_game_player_frame_tail_001097e0(gh_game_player_frame_tail*s,const gh_player_frame_tail_bindings*b){fenv_t env;if(feholdexcept(&env))return GH_PLAYER_TAIL_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_PLAYER_TAIL_NUMERIC;}gh_player_frame_tail_status e=body(s,b);if(fesetenv(&env))return GH_PLAYER_TAIL_NUMERIC;return e;}
