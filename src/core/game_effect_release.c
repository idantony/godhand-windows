#include "game_effect_create.h"
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
  if(!match)continue;
  const uint32_t profile=rd(p+(emitter?0x18c:0x23a));
  const int attached=kind==4||(kind==5&&profile==0x004e0000u)||(kind==3&&(profile==0x00040000u||profile==0x00210000u||profile==0x00250000u))||
      (kind==0&&(profile==0x00590058u||
      profile==0x005d0058u||(emitter&&profile==0x00420000u)));
  /* These authored mode-0 strikes can remain on the boss after the motion ends. */
  const int boss_strike=kind==0&&(profile==0x002c0058u||profile==0x00320058u||
      profile==0x00340058u||profile==0x004f0058u)&&
      actor&&actor->size>0x1650&&actor->original_vtable214==0x4479c8&&
      actor->bytes[0x1650]==15&&
      (rd(actor->bytes+0x564)==0x214||rd(actor->bytes+0x564)==0x215);
  if(emitter){
   gh_effect_emitter_view view;
   if(!gh_effect_create_emitter_at(b->effects,i,&view)&&view.payload==p&&
      (gh_effect_stamp19_attack_part_is_bound(&view)||gh_effect_stamp19_death_part_is_bound(&view)||gh_effect_knockdown_emitter_is_bound(&view)||gh_effect_em00_attack_emitter_is_bound(&view)||gh_effect_devil_emitter_is_bound(b->effects,&view)||
       (boss_strike&&gh_effect_em00_attack_emitter_is_bound(&view))||
       (attached&&(gh_effect_player_attachment_emitter_is_bound(&view)||
        gh_effect_player_heat_root_emitter_is_bound(&view))))&&
      actor&&view.actor->bytes==actor->bytes){
    /* 302918 only sets the stop request. The Effects step owns child
     * links, pending masks, and final destruction. */
    const uint32_t stopped=rd(p+0x114)|0x40000000u;
    memcpy(view.payload+0x114,&stopped,sizeof(stopped));
    continue;
   }
  }
  if(!emitter){
   gh_effect_esp_view view;
   if(!gh_effect_create_esp_at(b->effects,i,&view)&&view.payload==p&&
      (gh_effect_devil_esp_is_bound(b->effects,&view)||
       (((kind==0&&(profile==0x002b0058u||profile==0x002c0058u||profile==0x001d0058u||profile==0x00500058u))||boss_strike)&&
        gh_effect_em00_attack_esp_is_bound(b->effects,&view))||
       /* 306200 stops the original trail on EM03's interrupted attack. */
       (kind==0&&profile==0x002d0058u&&gh_effect_attack_esp_is_bound(b->effects,&view))||
       (attached&&gh_effect_player_attachment_esp_is_bound(b->effects,&view)))&&actor&&view.actor->bytes==actor->bytes){
    e=gh_effect_create_esp_pause_002fbe50(b->effects,i,view.serial,f);
    if(e)return e;
    continue;
   }
  }
  *f=emitter?0x302918:0x2fbe50;
  return GH_EFFECT_UNSUPPORTED;
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
