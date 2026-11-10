#include "game_scene_finish.h"
#include <string.h>
static uint32_t u32(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
gh_light_status gh_scene_world_light_select_002d8408(gh_game_world_light*s,unsigned index){
 if(!s||!s->constructed||!s->ready)return GH_LIGHT_ARGUMENT;
 if(index>=8)return GH_LIGHT_BOUNDS;uint8_t*p=s->scalar;const uint8_t*q=p+0x15d00+index*0x80;
 if(u32(q+0x70)||u32(q+0x74)||u32(p+0x78)){s->frontier=0x2d8408;return GH_LIGHT_UNSUPPORTED;}
 uint32_t selected=index;memcpy(p+0x16288,&selected,4);
 memcpy(p+0x30,q,12);memcpy(p+0x40,q+0x10,12);
 memcpy(p,q+0x30,32);memcpy(p+0x20,q+0x50,8);memcpy(p+0x60,q+0x58,24);
 s->frontier=0;return GH_LIGHT_OK;
}
void gh_scene_system_menu_enable_002b65f0(uint8_t*field){if(field)*field=1;}
