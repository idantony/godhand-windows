/* Readable original REL leaves; source identity is checked by room_module.
 * No imported stub addresses from the offline Ghidra wrapper are executable. */
#include "r100.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
gh_scene_status gh_r100_preinit_000001a0(const gh_boot_work*w,const gh_room_state*s,uint8_t*stop){
 if(!w||!s||!s->active||s->size<8||!stop)return GH_SCENE_ARGUMENT;
 const uint8_t*p=(const uint8_t*)w;
 if(!(word(p+0x590)&0x200000u)&&!(word(s->active+4)&0x1000000u)&&!(word(p+0x594)&0x800u))*stop=1;
 return GH_SCENE_OK;
}
gh_scene_status gh_r100_move_00000740(void){return GH_SCENE_OK;} /*actualJRRA/NOP*/
