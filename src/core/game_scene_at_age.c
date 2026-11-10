/* Actual patchedELF2C1540: NOW->PREV, clear NOW/TRG/UNK26/HANDLED.
 * Native first_unit combines original getUnitList sentinel + first getNext. */
#include "game_scene_at_age.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void put(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static gh_scene_at_status fail(uint32_t*f,gh_scene_at_status e){if(f)*f=0x2c30c8;return e;}
gh_scene_at_status gh_scene_at_age_002c1540(gh_scene_at_age*s,gh_scene_at_manager*m,uint32_t*f){
 if(f)*f=0;if(!s||!m||!m->ready)return GH_AT_ARGUMENT;
 if(!s->active){s->manager=m;s->native_owner=m->native_units;s->limit=gh_scene_at_native_unit_count(m);s->active=1;}
 else if(s->manager!=m||s->native_owner!=m->native_units||s->limit!=gh_scene_at_native_unit_count(m))return GH_AT_ARGUMENT;
 gh_scene_at_status e;
 if(!s->stage){e=gh_scene_at_first_unit_002c30c0(m,&s->current);if(e)return fail(f,e);s->stage=1;}
 for(;;){
  if(s->stage==1){
   if(!s->current.bytes){memset(s,0,sizeof(*s));return GH_AT_OK;}
   if(s->completed>=s->limit||s->current.size!=0x9c)return fail(f,GH_AT_BOUNDS);
   uint32_t old=word(s->current.bytes+0x50),next=old&UINT32_C(0xbfffffff);
   if(old&UINT32_C(0x80000000))next|=UINT32_C(0x40000000);
   put(s->current.bytes+0x50,next&UINT32_C(0x71ffffff));s->completed++;s->stage=2;
  }
  gh_at_native_unit next;e=gh_scene_at_next_unit_002c30c8(m,s->current,&next);if(e)return fail(f,e);
  s->current=next;s->stage=1;
 }
}
