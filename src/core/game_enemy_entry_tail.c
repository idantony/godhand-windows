#include "game_enemy_entry_tail.h"
#include "game_enemy_row2.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
gh_actor_model_status gh_enemy_entry_continue_002022d0(gh_enemy_entry_tail*s,gh_enemy_entry*e,gh_game_cockpit*c){
 if(!s||!e||e->stage!=8||!e->published||e->initialize.stage!=3||!e->initialize.model.model.ready||!e->b.init)return GH_ACTOR_MODEL_ARGUMENT;
 if(s->stage&&s->entry!=e)return GH_ACTOR_MODEL_BUSY;
 if(e->request.object_kind!=0x202||(e->request.dependency_kind!=0x20a&&!gh_enemy202_is_r100_supported_profile(&e->request))){s->frontier=0x2022d0;return GH_ACTOR_MODEL_UNSUPPORTED;}
 if(!s->stage){s->entry=e;s->stage=1;}
 uint8_t*p=e->object.view.bytes;
 if(s->stage==1){int original=0;gh_actor_model_status status=gh_enemy_base_initialize_0028ebf0(&s->base,&e->object.view,&e->request,e->argument,c,&e->b.init->random,&original);if(status){s->frontier=s->base.frontier;return status;}if(original!=1)return GH_ACTOR_MODEL_ARGUMENT;s->stage=2;}
 if(s->stage==2){/* Actual150128(self,4), notplayer's twoextraflagwrites. */p[0x140]=4;
  /* Actual207EE0 dependency20A hits switchdefault and returns. */
  s->original_counter=word(e->b.init->enemies,0x55c);put(e->b.init->enemies,0x55c,s->original_counter+UINT32_C(1));put(p,0x17d0,s->original_counter);
  /* 200/202/203 keep the spawn-counter scale from 42CCE0. 20A overwrites it. */
  static const uint32_t scales[]={0x3f851eb8,0x3f88f5c3,0x3f87ae14,0x3f8ccccd,
      0x3f8a3d71,0x3f8ccccd,0x3f866666,0x3f8b851f};
  uint32_t scale=e->request.dependency_kind==0x20a?0x3f800000:scales[s->original_counter&7];
  put(p,0x110,scale);put(p,0x118,scale);put(p,0x114,scale);s->stage=3;
 }
 /* Actual271410 dependency20A is another switchdefault with noallocations.
  * Next actualinstruction begins HPlookup; wholevirtualInit remainspending. */
 s->frontier=0x202b88;e->frontier=s->frontier;return GH_ACTOR_MODEL_UNSUPPORTED;
}
