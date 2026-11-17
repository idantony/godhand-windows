#include "game_enemy_behavior.h"
#include "game_enemy_row2.h"
#include <string.h>
static uint32_t word(const uint8_t *p,size_t offset){uint32_t value;memcpy(&value,p+offset,4);return value;}
gh_actor_motion_status gh_enemy_behavior_initialize_002064fc(gh_enemy_behavior*s,gh_enemy_timers*timers){
 if(!s||!timers||timers->stage!=2||!timers->nodes||!timers->nodes->solids)return GH_ACTOR_MOTION_ARGUMENT;if(s->stage&&s->timers!=timers)return GH_ACTOR_MOTION_BUSY;
 gh_enemy_entry*e=timers->nodes->solids->shadow->pose->motion->hp->tail->entry;uint8_t*p=e->object.view.bytes;
 const int r104_boss=gh_enemy203_is_r104_entry(&e->request,e->b.work->room);
 const int r103_boss=gh_enemy204_is_r103_entry(&e->request,e->b.work->room)&&e->request.entry_number<2;
 const int r200=gh_enemy201_is_r200_entry(&e->request,e->b.work->room);
 const uint32_t dependency=e->request.dependency_kind;
 if(!s->stage){s->timers=timers;
  /* Actual42E8C0[20A-202]→20654C: SBzero17B9. Ghidra'scase3iswrong. */
  const int devil=e->request.object_kind==0x220&&e->request.dependency_kind==0x220&&
      e->b.work->room==0x100&&e->request.flags==0x400000;
  const int exchange=word(p,0x564)==0x227;
  uint8_t ordinary=e->request.dependency_kind!=0x20a&&!devil;
  p[0x17b9]=exchange?2:devil?3:e->request.dependency_kind==0x200;
  p[0x17b8]=p[0x17bd]=exchange?2:1;p[0x17ba]=p[0x17bc]=exchange?2:ordinary;p[0x17bb]=1;
  if(e->request.dependency_kind==0x20f){
   /* Original 2065A4/206608/20666C/2066C8 clear approach options
    * and leave retreat mode 2 on this dependency. */
   p[0x17b8]=p[0x17ba]=p[0x17bb]=p[0x17bc]=0;
   p[0x17bd]=2;
  }
  if(r104_boss){p[0x17b8]=p[0x17b9]=p[0x17ba]=p[0x17bc]=p[0x17bd]=2;}
  if(r103_boss){p[0x17b9]=1;p[0x17ba]=2;p[0x17bc]=0;}
  if(r200){
   /* 2064FC uses six independent tables for these behavior selectors. */
   p[0x17b9]=dependency==0x202?0:dependency==0x20c?2:1;
   if(dependency==0x21a||dependency==0x20c)p[0x17ba]=p[0x17bc]=0;
   if(dependency==0x20c)p[0x17b8]=p[0x17bd]=2;
  }
  s->stage=1;
 }
 if(s->stage==1){unsigned room=(uint16_t)e->b.work->room;if(room>=0x803&&room<=0x805){s->frontier=0x274140;return GH_ACTOR_MOTION_UNSUPPORTED;}
  static const uint32_t factors[]={0x3f800000,0x3f99999a,0x3fb33333,0x3fcccccd,0x3ff33333,0x40066666,0x40133333,0x40200000};unsigned level=p[0x186b];
  uint32_t speed=level>=2&&level<=8?factors[level-1]:factors[0];
  if(word(p,0x564)==0x227){
   /* The only admitted room uses level 1. Keep other numerical paths strict. */
   if(level!=1){s->frontier=0x206cc8;return GH_ACTOR_MOTION_UNSUPPORTED;}
   speed=0x3fc00000;
  }
  if(r103_boss||r104_boss){
   uint32_t flags=word(p,0x16d4)|0x1000u; /* 274140 */
   memcpy(p+0x16d4,&flags,4);
   if(e->request.dependency_kind==0x214){
    /* R103 cold setup uses the first rank. Other rank products remain explicit. */
    if(level!=1){s->frontier=0x2068a8;return GH_ACTOR_MOTION_UNSUPPORTED;}
    speed=0x3fc00000;
   }
  }
  if(r200&&dependency==0x20c){
   /* 2068A8 applies a 1.5 base to the rank multiplier at 206D8C. */
   uint64_t product=(uint64_t)((speed&0x7fffff)|0x800000)*UINT64_C(0xc00000);
   unsigned normalized=(unsigned)(product>>47);
   speed=((((speed>>23)&0xffu)+normalized)<<23)|
       ((uint32_t)(product>>(23+normalized))&0x7fffff);
  }
  if(room==0x801||r104_boss)speed=0x3f800000;memcpy(p+0x76c,&speed,4);s->stage=2;
 }
 /* 20A skips theoriginalspecialboss Cockpitregistration switch. */
 s->frontier=0x1fe218;e->frontier=s->frontier;return GH_ACTOR_MOTION_UNSUPPORTED;
}
