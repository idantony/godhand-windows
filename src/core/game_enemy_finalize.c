#include "game_enemy_finalize.h"
#include "game_enemy_row2.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
gh_actor_motion_status gh_enemy_finalize_00207d40(gh_enemy_finalize*s,gh_enemy_hitareas*hitareas,int*original){
 if(!s||!hitareas||hitareas->stage!=4||!hitareas->behavior||!original)return GH_ACTOR_MOTION_ARGUMENT;*original=0;
 if(s->stage&&s->hitareas!=hitareas)return GH_ACTOR_MOTION_BUSY;
 gh_enemy_entry*e=hitareas->behavior->timers->nodes->solids->shadow->pose->motion->hp->tail->entry;uint8_t*p=e->object.view.bytes;
 if(e->request.object_kind!=0x202||word(p,0x564)!=e->request.dependency_kind||(e->request.dependency_kind!=0x20a&&!gh_enemy202_is_r100_supported_profile(&e->request))||!e->published||!e->object.view.alias_f0)return GH_ACTOR_MOTION_ARGUMENT;
 if(!s->stage){/* Actual26B9E8 only211 creates3B8;20Aguardreturnswithoutwrites. */
  s->hitareas=hitareas;put(p,0x560,0xffff);put(p,0x1728,0);put(p,0x17a4,0);put(p,0x17a8,0);p[0x17be]=0;put(p,0x624,0);
  /* Actual42FA10[20A-204] skipsconditionalrandomloot560 setter. */
  memcpy(p+0x1660,e->object.view.alias_f0,12);put(p,0x1670,word(p,0x104));p[0x617]=0;put(p,0x1730,0);p[0x17c2]=0;put(p,0x1750,0);put(p,0x1754,0);put(p,0x1758,0);
  put(p,0x16c8,0x41f00000);put(p,0x172c,0x43e10000);put(p,0x174c,0x43960000);s->stage=1;
 }
 if(s->stage==1){if(gh_game_mt_next_0030f348(&e->b.init->random,&s->final_random)){s->frontier=0x31cfe0;return GH_ACTOR_MOTION_ARGUMENT;}
  put(p,0x770,0);put(p,0x1718,0);p[0x17c0]=0;put(p,0x1764,0);put(p,0x1768,0);put(p,0x16e0,0);p[0x17bf]=(uint8_t)(s->final_random%3u);s->stage=2;s->complete=1;
 }
 s->frontier=0;e->frontier=0;*original=1;return GH_ACTOR_MOTION_OK;
}
