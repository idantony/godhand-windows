#include "sound_scene_tick.h"
#include <string.h>
#include <fenv.h>
#include <math.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float value(uint32_t bits){float n;memcpy(&n,&bits,4);return n;}
static uint32_t bits(float n){uint32_t b;memcpy(&b,&n,4);return b;}
gh_sound_scene_status gh_sound_scene_tick_002caf28(gh_sound_scene*s,const gh_boot_work*w,const gh_game_children*children){
 if(!s||!s->startup_bound||!w)return GH_SOUND_SCENE_ARGUMENT;
 if(u32((const uint8_t*)w+0x5e4)&0x8000000){
  s->ratios88[2]=0x3dcccccd;s->ratios74[2]=0x3ee66666;s->ratios74[1]=0;s->ratios88[1]=0x3dcccccd;
  const uint8_t*p;size_t n;if(!children||gh_game_children_manager_view(children,&p,&n)||n<0xeec){s->frontier=0x2caf6c;return GH_SOUND_SCENE_UNSUPPORTED;}
  s->ratios74[3]=u32(p+0xee8)?0x3f800000:0x3ee66666;s->ratios88[3]=0x3dcccccd;
 }else for(unsigned i=1;i<=3;i++){s->ratios74[i]=0x3f800000;s->ratios88[i]=0x3dcccccd;}
 for(unsigned i=0;i<5;i++)if(!isfinite(value(s->ratios60[i]))||!isfinite(value(s->ratios74[i]))||!isfinite(value(s->ratios88[i]))){s->frontier=0x2caff8;return GH_SOUND_SCENE_UNSUPPORTED;}
 fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST)){s->frontier=0x2caff8;return GH_SOUND_SCENE_UNSUPPORTED;}
 for(unsigned i=0;i<5;i++){
  float current=value(s->ratios60[i]),target=value(s->ratios74[i]),rate=value(s->ratios88[i]);
  if(current!=target){if(target<current){volatile float next=current-rate;current=next;if(current<target)current=target;}
   else {volatile float next=current+rate;current=next;if(target<current)current=target;}s->ratios60[i]=bits(current);}
 }
 if(fesetenv(&saved)){s->frontier=0x2caff8;return GH_SOUND_SCENE_UNSUPPORTED;}s->frontier=0;return GH_SOUND_SCENE_OK;
}
