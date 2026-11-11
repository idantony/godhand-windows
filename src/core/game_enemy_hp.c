#include "game_enemy_hp.h"
#include "game_enemy_row2.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
gh_actor_model_status gh_enemy_hp_initialize_00202b88(gh_enemy_hp*s,gh_enemy_entry_tail*t,gh_em_numeric_policy policy){
 if(!s||!t||t->stage!=3||!t->entry||!t->entry->published)return GH_ACTOR_MODEL_ARGUMENT;
 if(policy!=GH_EM_NUMERIC_EXACT&&policy!=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE)return GH_ACTOR_MODEL_ARGUMENT;
 if(s->stage&&s->tail!=t)return GH_ACTOR_MODEL_BUSY;
 gh_enemy_entry*e=t->entry;uint8_t*p=e->object.view.bytes;gh_core_save_view save=e->b.save;
 if(!save.bytes||save.size<GH_CORE_SAVE_BYTES||e->request.object_kind!=0x202||(e->request.dependency_kind!=0x20a&&!gh_enemy202_is_r100_supported_profile(&e->request)))return GH_ACTOR_MODEL_ARGUMENT;
 if(!s->stage){p[0x622]=0;unsigned level=p[0x186b];static const uint16_t hp20a[]={148,192,236,296,340,399,444,488};
  static const uint16_t hp200[]={74,96,118,148,170,192,222,244};
  const uint16_t *hp=e->request.dependency_kind==0x20a?hp20a:hp200;
  uint32_t value=level>=2&&level<=8?hp[level-1]:hp[0];
  /* Actual1FABB0 visits atmostsix equippedA8 bytes whenroomhigh!=0.
   * The 20A and 200 bonus-switch default is 0; no globalplayerHP read occurs. */
  uint8_t equipped=0;if(((uint16_t)e->b.work->room>>8)!=0){unsigned n=save.bytes[0x156];if(n>6)n=6;for(unsigned i=0;i<n;i++)if(save.bytes[0xa8+i])equipped++;}
  if(!(word(save.bytes,0x14)&0x4000000)){unsigned difficulty=save.bytes[0x1f];if(difficulty==0){if(policy!=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE){s->frontier=0x20367c;return GH_ACTOR_MODEL_UNSUPPORTED;}
    /* value<=488 exactly converts tobinary32; factor3F4CCCCD is
     * 13421773/2^24. ChopZero product andCVT preserve integerfloor:
     * floor(exactproduct) is itself exactly representable. No hostFEnv. */
    value=(uint32_t)(((uint64_t)value*UINT64_C(13421773))>>24);
   }else if(difficulty==2)value+=20;
  }
  p[0x622]=0;uint16_t small=(uint16_t)value;memcpy(p+0x548,&small,2);memcpy(p+0x54a,&small,2);
  s->tail=t;s->hp=value;s->equipped_count=equipped;s->stage=1;
 }
 /* Next original2036B0 begins per-dependency data selection. */
 s->frontier=0x2036b0;e->frontier=s->frontier;return GH_ACTOR_MODEL_UNSUPPORTED;
}
