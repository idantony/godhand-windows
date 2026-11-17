#include "game_enemy_hp.h"
#include "game_enemy_row2.h"
#include <string.h>
#include <math.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
gh_actor_model_status gh_enemy_hp_initialize_00202b88(gh_enemy_hp*s,gh_enemy_entry_tail*t,gh_em_numeric_policy policy){
 if(!s||!t||t->stage!=3||!t->entry||!t->entry->published)return GH_ACTOR_MODEL_ARGUMENT;
 if(policy!=GH_EM_NUMERIC_EXACT&&policy!=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE)return GH_ACTOR_MODEL_ARGUMENT;
 if(s->stage&&s->tail!=t)return GH_ACTOR_MODEL_BUSY;
 gh_enemy_entry*e=t->entry;uint8_t*p=e->object.view.bytes;gh_core_save_view save=e->b.save;
 const int devil=e->request.object_kind==0x220&&e->request.dependency_kind==0x220&&
     e->b.work->room==0x100&&p[0x186b]==1&&e->request.flags==0x400000;
 const int r103=gh_enemy204_is_r103_entry(&e->request,e->b.work->room);
 const int r103_boss=r103&&e->request.entry_number<2;
 const int r104_boss=gh_enemy203_is_r104_entry(&e->request,e->b.work->room);
 const int r200=gh_enemy201_is_r200_entry(&e->request,e->b.work->room);
 const int r200_heavy=r200&&e->request.dependency_kind==0x20c;
 if(!save.bytes||save.size<GH_CORE_SAVE_BYTES||(!devil&&!r103&&!r104_boss&&!r200&&(e->request.object_kind!=0x202||
    (e->request.dependency_kind!=0x20a&&!gh_enemy202_is_room_supported_profile(&e->request,e->b.work->room)))))
  return GH_ACTOR_MODEL_ARGUMENT;
 if(!s->stage){p[0x622]=0;unsigned level=p[0x186b];static const uint16_t hp20a[]={148,192,236,296,340,399,444,488};
  static const uint16_t hp200[]={74,96,118,148,170,192,222,244};
  /* Runtime 227 uses jump table 42D110 and sets the elite flag. */
  const int exchange=word(p,0x564)==0x227;
  static const uint16_t hp227[]={322,388,455,544,610,677,766,832};
  /* 42CEF0[0x0F] -> 202D9C -> the 42D190 level table. */
  static const uint16_t hp20f[]={37,51,66,81,96,111,125,140};
  const uint16_t *hp=hp200;
  if(exchange)hp=hp227;
  else if(e->request.dependency_kind==0x20a||r200_heavy)hp=hp20a;
  else if(e->request.dependency_kind==0x20f)hp=hp20f;
  uint32_t value=r104_boss?1000u:r103_boss?580u:devil?330u:level>=2&&level<=8?hp[level-1]:hp[0];
  /* Actual1FABB0 visits atmostsix equippedA8 bytes whenroomhigh!=0.
   * The 20A and 200 bonus-switch default is 0; no globalplayerHP read occurs. */
  uint8_t equipped=0;if(((uint16_t)e->b.work->room>>8)!=0){unsigned n=save.bytes[0x156];if(n>6)n=6;for(unsigned i=0;i<n;i++)if(save.bytes[0xa8+i])equipped++;}
  if((r103_boss||r104_boss)&&((const uint8_t *)e->b.work)[0x5b1]){
   /* 203604 applies the full equipped-slot and Heat bonus to 250. */
   static const unsigned per_move[]={35,45,50,60,67,75,87,100};
   static const uint32_t heat_factors[]={0x3f4ccccd,0x3f800000,0x3f99999a,0x3fb33333,
       0x3fcccccd,0x3fe66666,0x40000000,0x400ccccd};
   const unsigned rank=level>=1&&level<=8?level-1:0;
   float heat,factor;
   memcpy(&heat,e->b.init->heat+4,4);memcpy(&factor,&heat_factors[rank],4);
   if(policy!=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE||!isfinite(heat)||heat<0||heat>1024){
    s->frontier=0x203604;return GH_ACTOR_MODEL_UNSUPPORTED;
   }
   /* The product of two binary32 significands is exact in binary64.
    * Truncating here gives the same integer as EE ChopZero in this range. */
   const unsigned heat_bonus=(unsigned)((double)heat*(double)factor);
   const unsigned bonus=per_move[rank]*(equipped>1?equipped:0)+heat_bonus;
   value+=r104_boss?bonus:bonus/4;
  }
  if(!(word(save.bytes,0x14)&0x4000000)){unsigned difficulty=save.bytes[0x1f];if(difficulty==0){if(policy!=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE){s->frontier=0x20367c;return GH_ACTOR_MODEL_UNSUPPORTED;}
    /* value<=488 exactly converts tobinary32; factor3F4CCCCD is
     * 13421773/2^24. ChopZero product andCVT preserve integerfloor:
     * floor(exactproduct) is itself exactly representable. No hostFEnv. */
    value=(uint32_t)(((uint64_t)value*UINT64_C(13421773))>>24);
   }else if(difficulty==2)value+=20;
  }
  if(devil||exchange||r103_boss||r104_boss||r200_heavy)p[0x17c1]=1;
  if(r104_boss){uint32_t timer=0x40a00000;memcpy(p+0x54c,&timer,4);}
  p[0x622]=(uint8_t)(devil||r103_boss||r104_boss);uint16_t small=(uint16_t)value;memcpy(p+0x548,&small,2);memcpy(p+0x54a,&small,2);
  s->tail=t;s->hp=value;s->equipped_count=equipped;s->stage=1;
 }
 /* Next original2036B0 begins per-dependency data selection. */
 s->frontier=0x2036b0;e->frontier=s->frontier;return GH_ACTOR_MODEL_UNSUPPORTED;
}
