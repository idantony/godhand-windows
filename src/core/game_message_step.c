/* Actualpatched2AEFA8 readable empty/gated domain; MIT LICENSE.upstream. */
#include "game_message_step.h"
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
gh_scene_ui_status gh_scene_message_update_002aefa8(const gh_game_scene_ui*s,const gh_boot_work*w,uint32_t*f){
 if(f)*f=0;if(!s||!w)return GH_SCENE_UI_ARGUMENT;
 if(word((const uint8_t*)w+0x5d8)&0x80000u)return GH_SCENE_UI_OK;
 if(!s->ready||!s->root)return GH_SCENE_UI_ARGUMENT;
 if(s->active_head==-1)return GH_SCENE_UI_OK;
 if(s->active_head<0||s->active_head>=16)return GH_SCENE_UI_BOUNDS;
 if(f)*f=0x2b2438;return GH_SCENE_UI_UNSUPPORTED;
}
