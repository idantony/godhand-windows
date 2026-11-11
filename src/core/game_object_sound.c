#include "game_object_sound.h"
#include "enemy_se_banks.h"
gh_voice_status gh_sound_recent_push_002cbdc0(gh_sound_control*s,int16_t bank,int16_t program,uint32_t key){
 if(!s||!s->initialized||!s->recent_initialized)return GH_VOICE_ARGUMENT;
 if(s->recent_pos < -1||s->recent_pos>=16)return GH_VOICE_UNSUPPORTED;
 int32_t pos=s->recent_pos+1;if(pos==16)pos=0;s->recent_pos=pos;s->recent[pos]=(gh_sound_recent){bank,program,key};return GH_VOICE_OK;
}
gh_object_status gh_sound_object_slot_release_002cb340(void*p,int32_t slot,uint32_t*f){
 if(f)*f=0;gh_object_sound_binding*b=p;if(!b||!b->control||!b->control->initialized||!b->control->recent_initialized||!b->voices)return GH_OBJECT_ARGUMENT;
 if(slot!=-1){
  gh_enemy_se_banks *banks=b->control->enemy_se_banks;
  gh_enemy_se_slot_view owned;
  if(slot<0||!banks||gh_enemy_se_slot_at(banks,slot,&owned)!=GH_ENEMY_SE_OK){
   if(f)*f=0x2cb340;
   return GH_OBJECT_UNSUPPORTED;
  }
  gh_enemy_se_status status=gh_enemy_se_mark_failed_002cb7e8(banks,slot);
  if(status){
   if(f)*f=gh_enemy_se_frontier(banks);
   return status==GH_ENEMY_SE_BUSY?GH_OBJECT_BUSY:GH_OBJECT_UNSUPPORTED;
  }
  return GH_OBJECT_OK;
 }
 /* 2CE510 compares the signed bank ID before doing anything to a voice.
  * Slot -1 aliases the recent entry, and unrelated voices stay live. */
 if(gh_se_voice_bank_nodes(b->voices,-1)){if(f)*f=0x2cbe68;return GH_OBJECT_UNSUPPORTED;}
 gh_sound_recent*r=b->control->recent+10;
 if((uint16_t)r->bank|| (uint16_t)r->program){r->bank=3;r->program=0;} /*write32bit3, preservekey*/
 return GH_OBJECT_OK;
}
