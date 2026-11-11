/* Actual2BF8F8/2C05B0/full0 snapshot path; MIT LICENSE.upstream. */
#include "game_checkpoint.h"
#include <stddef.h>
#include <string.h>
_Static_assert(sizeof(gh_checkpoint_tail32)==0x20,"originalrestarttail");
_Static_assert(offsetof(gh_checkpoint_tail32,heat)==0x18,"originalcurrentheat");
_Static_assert(offsetof(gh_checkpoint_tail32,heat_max)==0x1c,"originalmaxheat");
static uint32_t u32(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void w(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void h(uint8_t*p,uint16_t n){memcpy(p,&n,2);}
gh_checkpoint_status gh_game_checkpoint_002c05b0(gh_game_checkpoint*s,const gh_checkpoint_bindings*b){
 if(!s||!b||!b->work)return GH_CHECKPOINT_ARGUMENT;s->frontier=0;s->last_skipped=0;
 if(!b->save.bytes||b->save.size<GH_CORE_SAVE_BYTES||!b->prepare||!b->init||!b->player||!b->destination)return GH_CHECKPOINT_ARGUMENT;
 if(b->destination_size<GH_CHECKPOINT_BYTES)return GH_CHECKPOINT_BOUNDS;
 uint8_t*save=b->save.bytes;uint16_t stage_b;memcpy(&stage_b,(const uint8_t*)b->work+0x5b6,2);
 h(save+0xc,(uint16_t)b->work->room);h(save+0xe,stage_b); /*1F9C88*/
 if(b->work->flags598&0x08000000u)save[0x154]=13; /*1FAE60*/
 w(save+0x80,(uint32_t)save[0x154]*20+100); /*1FACB0*/
 int16_t hp;uint32_t frontier;gh_title_player_status r=gh_title_player_read_vitality(b->player,&hp,&frontier);
 if(r!=GH_PLAYER_OK){s->frontier=frontier?frontier:0x1f9bec;return r==GH_PLAYER_UNSUPPORTED?GH_CHECKPOINT_UNSUPPORTED:GH_CHECKPOINT_ARGUMENT;}
 w(save+0x84,hp>0?(uint32_t)hp:1); /*1FAC98, no clamp to vitalMax*/
 uintptr_t base=(uintptr_t)b->player->bytes,pos=(uintptr_t)b->player->alias_f0;
 if(pos<base||pos-base>b->player->size||b->player->size-(size_t)(pos-base)<12){s->frontier=0x31c350;return GH_CHECKPOINT_BOUNDS;}
 memcpy(s->tail.position,b->player->alias_f0,12);memcpy(s->tail.rotation,b->player->bytes+0x100,12);
 s->tail.heat=u32(b->init->heat+4);s->tail.heat_max=u32(b->init->heat);
 memcpy(b->destination,save,GH_CORE_SAVE_BYTES);
 memcpy(b->destination+0x14a0,b->prepare->fields+0x1660,0x9c30);
 memcpy(b->destination+0xb0d0,&s->tail,0x20);
 s->captured=1;return GH_CHECKPOINT_OK;
}
gh_checkpoint_status gh_game_checkpoint_002bf8f8(gh_game_checkpoint*s,const gh_checkpoint_bindings*b){
 if(!s||!b||!b->work)return GH_CHECKPOINT_ARGUMENT;s->frontier=0;s->last_skipped=0;
 if((b->work->flags598&0x800u)||(b->work->flags&0x08000000u)){s->last_skipped=1;return GH_CHECKPOINT_SKIPPED;}
 return gh_game_checkpoint_002c05b0(s,b);
}
