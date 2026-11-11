#include "game_enemy_solids.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
gh_actor_motion_status gh_enemy_solids_initialize_00204b4c(gh_enemy_solids*s,gh_enemy_shadow*shadow,gh_game_collision_solids*manager,gh_heap*heap){
 if(!s||!shadow||shadow->stage!=1||!shadow->pose||!shadow->pose->motion||!shadow->pose->motion->hp)return GH_ACTOR_MOTION_ARGUMENT;
 gh_enemy_entry*e=shadow->pose->motion->hp->tail->entry;
 if(s->stage&&(s->shadow!=shadow||s->manager!=manager||s->heap!=heap))return GH_ACTOR_MOTION_BUSY;
 if(!manager||!heap){s->frontier=0x1346f8;return GH_ACTOR_MOTION_UNSUPPORTED;}
 uint8_t*p=e->object.view.bytes;
 if(!s->stage){if(manager->unit_head!=manager->native_units)return GH_ACTOR_MOTION_ARGUMENT;unsigned count=0;for(gh_collision_solid_unit*u=manager->native_units;u;u=u->next){if(++count>manager->unit_count||u->manager!=manager)return GH_ACTOR_MOTION_ARGUMENT;if(u->object&&u->object->bytes==p)return GH_ACTOR_MOTION_BUSY;}
  s->shadow=shadow;s->manager=manager;s->heap=heap;put(p,0x16cc,0x3ecccccd);s->stage=1;
 }
 if(s->stage==1){gh_solid_unit_status status=gh_collision_solid_create_unit_001346f8(manager,heap,&e->object.view,5,0x3e4ccccd,&s->unit,&s->frontier);if(status)return GH_ACTOR_MOTION_UNSUPPORTED;s->stage=2;}
 if(s->stage==2){const uint32_t xyz[]={0,0x3f000000,0};gh_solid_unit_status status=gh_collision_solid_create_sphere_00134880(manager,&e->object.view,p+0x80,xyz,e->request.dependency_kind==0x20a?0x3f333333:0x3f000000,&s->sphere,&s->frontier);if(status)return GH_ACTOR_MOTION_UNSUPPORTED;s->stage=3;}
 if(s->stage==3){p[0x1868]=(word(p,0x1644)&0x20000)?0:1;/* Original273D28 simpleSB. */put(p,0x16f0,0x3f800000);
  if(gh_enemy_manager_random_check_002949e8(e->b.init,32,&s->random_result)){s->frontier=0x2949e8;return GH_ACTOR_MOTION_UNSUPPORTED;}
  uint32_t flags=word(p,0x1644),v=word(p,0x16d0);if(s->random_result||(flags&0x8000000))v|=0x200;if(flags&0x4000000)v&=~0x200u;put(p,0x16d0,v);s->stage=4;
 }
 s->frontier=0x14b730;e->frontier=s->frontier;return GH_ACTOR_MOTION_UNSUPPORTED;
}
gh_actor_motion_status gh_enemy_solids_abort_prefix(gh_enemy_solids*s){
 if(!s)return GH_ACTOR_MOTION_ARGUMENT;if(!s->stage)return GH_ACTOR_MOTION_OK;
 if(s->unit){gh_enemy_entry*e=s->shadow->pose->motion->hp->tail->entry;unsigned count=0;int found=0;for(gh_collision_solid_unit*u=s->manager->native_units;u;u=u->next){if(++count>s->manager->unit_count)return GH_ACTOR_MOTION_BUSY;if(u==s->unit){if(u->object!=&e->object.view)return GH_ACTOR_MOTION_BUSY;found=1;break;}}
  if(!found)return GH_ACTOR_MOTION_BUSY;if(gh_collision_solid_release_unit_001347e8(s->manager,&e->object.view,&s->frontier))return GH_ACTOR_MOTION_UNSUPPORTED;
 }
 *s=(gh_enemy_solids){0};return GH_ACTOR_MOTION_OK;
}
