#include "game_effect_release.h"
#include <string.h>
static uint32_t rd(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
static gh_effect_status scan(const gh_effect_release_bindings*b,const gh_title_player_view*actor,int32_t kind,int emitter,uint32_t*f,unsigned*slot){
 uint8_t*base=emitter?b->emitters:b->esp;unsigned count=emitter?256:1024,stride=emitter?0x260:0x400,bits=emitter?0x26000:0x100000,bank=emitter?0x20:0x80;
 for(unsigned i=0;i<count;i++){
  unsigned off=(i>>5)*4;uint32_t mask=0x80000000u>>(i&31);
  if(!(rd(base+bits+off)&mask)||(rd(base+bits+bank+off)&mask)||(rd(base+bits+bank*2+off)&mask))continue;
  const uint8_t*p=base+(size_t)i*stride;unsigned kindoff=emitter?0x110:0x10c;
  if(rd(p+kindoff)!=(uint32_t)kind)continue;*slot=i;int match=0;
  if(!b->actor_match){*f=emitter?0x3039e8:0x2feb80;return GH_EFFECT_UNSUPPORTED;}
  gh_effect_status e=b->actor_match(b->context,emitter,i,actor,&match,f);if(e)return e;
  if(match){*f=emitter?0x302918:0x2fbe50;return GH_EFFECT_UNSUPPORTED;}
 }
 return GH_EFFECT_OK;
}
gh_effect_status gh_game_effect_release_00306200(const gh_effect_release_bindings*b,const gh_title_player_view*actor,int32_t kind,uint32_t mode,uint32_t*f,unsigned*slot){
 if(!f||!slot)return GH_EFFECT_ARGUMENT;*f=0;*slot=0;
 /* Actual306200 accepts non-0/1/2 mode as genuineempty switchdefault. */
 if((!actor&&kind==-1)||mode>2)return GH_EFFECT_OK;
 if(!b||!b->effects||!b->effects->ready||!b->esp||b->esp_bytes<0x100180||!b->emitters||b->emitter_bytes<0x26060)return GH_EFFECT_ARGUMENT;
 if(actor&&(!actor->bytes||actor->size<0x214))return GH_EFFECT_ARGUMENT;
 gh_effect_status e;if(mode==1||mode==2){e=scan(b,actor,kind,0,f,slot);if(e)return e;}
 if(mode==0||mode==2){e=scan(b,actor,kind,1,f,slot);if(e)return e;}
 return GH_EFFECT_OK;
}
