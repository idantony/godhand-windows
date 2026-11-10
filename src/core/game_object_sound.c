#include "game_object_sound.h"
gh_voice_status gh_sound_recent_push_002cbdc0(gh_sound_control*s,int16_t bank,int16_t program,uint32_t key){
 if(!s||!s->initialized||!s->recent_initialized)return GH_VOICE_ARGUMENT;
 if(s->recent_pos < -1||s->recent_pos>=16)return GH_VOICE_UNSUPPORTED;
 int32_t pos=s->recent_pos+1;if(pos==16)pos=0;s->recent_pos=pos;s->recent[pos]=(gh_sound_recent){bank,program,key};return GH_VOICE_OK;
}
gh_object_status gh_sound_object_slot_release_002cb340(void*p,int32_t slot,uint32_t*f){
 if(f)*f=0;gh_object_sound_binding*b=p;if(!b||!b->control||!b->control->initialized||!b->control->recent_initialized||!b->voices)return GH_OBJECT_ARGUMENT;
 if(slot!=-1){if(f)*f=0x2cb340;return GH_OBJECT_UNSUPPORTED;}
 /*2CBE68 walks the SAMEactualEEvoice list before2CFC98. No silentstop of an
  * active list or substitution of a native/driver handle for the bank index. */
 if(gh_se_voice_active_nodes(b->voices)){if(f)*f=0x2cbe68;return GH_OBJECT_UNSUPPORTED;}
 gh_sound_recent*r=b->control->recent+10;
 if((uint16_t)r->bank|| (uint16_t)r->program){r->bank=3;r->program=0;} /*write32bit3, preservekey*/
 return GH_OBJECT_OK;
}
