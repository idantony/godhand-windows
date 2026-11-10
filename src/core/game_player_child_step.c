/* actualpatchedELF100530..100590; upstreamsectionname not used as address. */
#include "game_player_child_step.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
gh_title_player_status gh_game_player_child_move_gate_00100530(const gh_title_player_view*child,const gh_boot_work*work,int*out,uint32_t*f){
 if(!child||!child->bytes||child->size<0x300||!work||!out||!f)return GH_PLAYER_ARGUMENT;*out=0;*f=0;
 const uint8_t*p=child->bytes,*w=(const uint8_t*)work;uint16_t kind;memcpy(&kind,p+0x2fe,2);uint32_t system=word(w+0x5d8);
 if(kind==0x101){if((system&0x80000000u)&&!(word(w+0x5e4)&0x20000000u)){*out=1;return GH_PLAYER_OK;}*f=0x100590;}
 else{if(system&0x40000000u){*out=1;return GH_PLAYER_OK;}*f=0x100594;}
 return GH_PLAYER_UNSUPPORTED;
}
