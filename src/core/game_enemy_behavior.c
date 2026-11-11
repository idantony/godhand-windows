#include "game_enemy_behavior.h"
#include <string.h>
gh_actor_motion_status gh_enemy_behavior_initialize_002064fc(gh_enemy_behavior*s,gh_enemy_timers*timers){
 if(!s||!timers||timers->stage!=2||!timers->nodes||!timers->nodes->solids)return GH_ACTOR_MOTION_ARGUMENT;if(s->stage&&s->timers!=timers)return GH_ACTOR_MOTION_BUSY;
 gh_enemy_entry*e=timers->nodes->solids->shadow->pose->motion->hp->tail->entry;uint8_t*p=e->object.view.bytes;
 if(!s->stage){s->timers=timers;
  /* Actual42E8C0[20A-202]→20654C: SBzero17B9. Ghidra'scase3iswrong. */
  uint8_t ordinary=e->request.dependency_kind!=0x20a;
  p[0x17b9]=e->request.dependency_kind==0x200;p[0x17b8]=1;p[0x17ba]=ordinary;p[0x17bd]=1;p[0x17bb]=1;p[0x17bc]=ordinary;s->stage=1;
 }
 if(s->stage==1){unsigned room=(uint16_t)e->b.work->room;if(room>=0x803&&room<=0x805){s->frontier=0x274140;return GH_ACTOR_MOTION_UNSUPPORTED;}
  static const uint32_t factors[]={0x3f800000,0x3f99999a,0x3fb33333,0x3fcccccd,0x3ff33333,0x40066666,0x40133333,0x40200000};unsigned level=p[0x186b];uint32_t speed=level>=2&&level<=8?factors[level-1]:factors[0];if(room==0x801)speed=0x3f800000;memcpy(p+0x76c,&speed,4);s->stage=2;
 }
 /* 20A skips theoriginalspecialboss Cockpitregistration switch. */
 s->frontier=0x1fe218;e->frontier=s->frontier;return GH_ACTOR_MOTION_UNSUPPORTED;
}
