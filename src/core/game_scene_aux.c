#include "game_scene_aux.h"
#include <string.h>
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static gh_scene_aux_status stop(gh_game_scene_aux*s,uint32_t f){s->frontier=f;return GH_AUX_UNSUPPORTED;}
gh_scene_aux_status gh_scene_door_step_001ffe10(gh_game_scene_aux*s,uint16_t*wait,const gh_boot_work*w){
 if(!s||!wait||!w)return GH_AUX_ARGUMENT;s->frontier=0;
 if(!s->door_stage){if(*wait)*wait=(uint16_t)(*wait-1u);s->door_stage=1;}
 if(word((const uint8_t*)w+0x5e4)&0x02000000u)return stop(s,0x2000f8);
 s->door_stage=0;return GH_AUX_OK;
}
void gh_scene_background_mode_0030e5c0(gh_boot_work*w,int mode,uint32_t a,uint32_t b,uint32_t c){
 if(!w||(mode!=0&&mode!=1))return;
 uint8_t*p=(uint8_t*)w;uint64_t x,y;memcpy(&x,p+0x3e0,8);memcpy(&y,p+0x270,8);
 uint64_t low=mode?0x16:0x10;x=(x&UINT64_C(0xffffffffffff8000))|low;y=(y&UINT64_C(0xffffffffffff8000))|low;
 memcpy(p+0x3e0,&x,8);memcpy(p+0x270,&y,8);
 if(mode){uint32_t config=(a&255)|((b&255)<<8)|((c&255)<<16);memcpy(p+0x60c,&config,4);}
}
gh_scene_aux_status gh_scene_cursor_step_002bf2d0(gh_game_scene_aux*s,gh_boot_work*w,const uint8_t*kind){
 static const uint32_t targets[]={0x2bf388,0x2bf478,0x2bf540,0x2bf568};
 if(!s||!w)return GH_AUX_ARGUMENT;s->frontier=0;
 /* Predecessor2BF380 is proven JRRA/NOP. Original inactive branch never reads
  * the cursor pointer, so an absent native cursor is valid on that branch. */
 if(!(word((const uint8_t*)w+0x5e8)&0x40000000u))return GH_AUX_OK;
 if(!kind)return GH_AUX_ARGUMENT;
 gh_scene_background_mode_0030e5c0(w,1,0,0,0);
 return *kind>=2&&*kind<=5?stop(s,targets[*kind-2]):GH_AUX_OK;
}
gh_scene_aux_status gh_scene_effects_step_0030f228(gh_game_scene_aux*s,const gh_boot_work*w,gh_scene_aux_clock clock,void*context){
 if(!s||!w||!clock)return GH_AUX_ARGUMENT;s->frontier=0;
 if(!s->effect_stage){s->effect_profile_752d00=clock(context);s->effect_count_3c3bb8=0;s->effect_stage=1;}
 if(!(word((const uint8_t*)w+0x5d8)&0x10000000u))return stop(s,0x300628);
 s->effect_profile_752d00-=clock(context);s->effect_stage=0;return GH_AUX_OK;
}
