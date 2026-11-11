#include "game_player_attack_recent.h"
#include <string.h>

gh_hitarea_status gh_player_attack_recent_0010b2e8(const gh_title_player_view *player,
 const gh_hitarea_object give[5],int *original_hit,uint32_t *frontier)
{
 if(frontier)*frontier=0;
 if(!player||!player->bytes||!give||!original_hit||!frontier)return GH_HITAREA_ARGUMENT;
 *original_hit=0;
 for(unsigned i=0;i<5;i++){
  gh_title_player_view *owner;
  const gh_title_player_view *victim;
  if(gh_hitarea_object_actor(give+i,&owner)||!owner||owner->bytes!=player->bytes||
     gh_hitarea_object_last_hit_actor(give+i,&victim)){
   *frontier=0x10b2e8;
   return GH_HITAREA_ARGUMENT;
  }
  if(!victim)continue;
  if(!victim->bytes||victim->size<0x568){
   *frontier=0x10b2e8;
   return GH_HITAREA_ARGUMENT;
  }
  uint32_t dependency;
  uint16_t kind;
  memcpy(&dependency,victim->bytes+0x564,4);
  memcpy(&kind,victim->bytes+0x2fe,2);
  if(dependency==0x2a2||dependency==0x2a5)return GH_HITAREA_OK;
  if((kind>=0x200&&kind<0x300)||kind==0x31f||kind==0x359||kind==0x35a||
     kind==0x35d||kind==0x3b3||kind==0x440){
   *original_hit=1;
   return GH_HITAREA_OK;
  }
 }
 return GH_HITAREA_OK;
}
