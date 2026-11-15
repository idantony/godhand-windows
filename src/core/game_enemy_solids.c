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
 if(e->request.object_kind==0x220&&e->request.dependency_kind==0x220&&
    e->b.work->room==0x100&&e->request.flags==0x400000&&e->request.entry_number==0xff){
  if(s->stage==3){
   p[0x1868]=1; /* 273D28(1) precedes the positive spawn effect. */
   p[0x614]=1;p[0x531]=5;
   s->stage=5;
  }
  s->frontier=s->stage==4?0x14b730:0x306528;e->frontier=s->frontier;
  return GH_ACTOR_MOTION_UNSUPPORTED;
 }
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

gh_actor_motion_status gh_enemy_devil_pose_00152a20(gh_enemy_devil_pose *s,
 gh_enemy_devil_initialize *init,const gh_motion_solve_bindings *b,gh_heap *heap)
{
 if(!s||!init||!init->owner||init->stage!=5||init->motion.stage!=4||!b||!heap)
  return GH_ACTOR_MOTION_ARGUMENT;
 gh_enemy_entry *entry=&init->owner->entry;
 if(s->init&&(s->init!=init||s->heap!=heap||s->bindings.model!=b->model||
    s->bindings.work!=b->work||s->bindings.actors!=b->actors||s->bindings.ray!=b->ray||
    s->bindings.context!=b->context||s->bindings.ik_globals!=b->ik_globals||
    s->bindings.atan_coeff!=b->atan_coeff))return GH_ACTOR_MOTION_BUSY;
 if(!s->init){s->init=init;s->heap=heap;s->bindings=*b;}
 if(!s->stage){
  gh_actor_motion_status status=gh_enemy_pose_initialize_00204af8(
      &s->pose,&init->motion,b,entry->b.policy);
  if(status!=GH_ACTOR_MOTION_UNSUPPORTED||s->pose.stage!=3||s->pose.frontier!=0x14ef50){
   s->frontier=entry->frontier=s->pose.frontier?s->pose.frontier:0x152a20;
   return status;
  }
  s->stage=1;
 }
 if(s->stage==1){
  gh_actor_motion_status status=gh_enemy_shadow_initialize_00204b10(&s->shadow,&s->pose);
  if(status!=GH_ACTOR_MOTION_UNSUPPORTED||s->shadow.stage!=1||s->shadow.frontier!=0x204b4c){
   s->frontier=entry->frontier=s->shadow.frontier?s->shadow.frontier:0x14ef50;
   return status;
  }
  s->stage=2;
 }
 gh_actor_motion_status status=gh_enemy_solids_initialize_00204b4c(
     &s->solids,&s->shadow,init->bindings.solids,heap);
 s->frontier=entry->frontier=s->solids.frontier?s->solids.frontier:0x1346f8;
 return status;
}

static gh_actor_motion_status devil_effect_stop(gh_enemy_devil_pose *s,gh_enemy_entry *entry,
 uint32_t pc,gh_actor_motion_status status)
{
 s->frontier=entry->frontier=pc;
 return status;
}

gh_actor_motion_status gh_enemy_devil_spawn_effect_00306528(gh_enemy_devil_pose *s,
 const gh_effect_create_bindings *b)
{
 if(!s||!s->init||!s->init->owner||!b||!b->storage.effects||
    s->stage!=2||(s->solids.stage!=5&&s->solids.stage!=4))
  return GH_ACTOR_MOTION_ARGUMENT;
 gh_enemy_entry *entry=&s->init->owner->entry;
 gh_actor_model *model=&entry->initialize.model.model;
 if(b->work!=entry->b.work||b->random!=&entry->b.init->random||
    (b->model&&b->model!=model))return GH_ACTOR_MOTION_ARGUMENT;
 gh_effect_create_call *call=&s->spawn_effect;
 if(s->solids.stage==4){
  if(call->owner!=b->storage.effects)return GH_ACTOR_MOTION_BUSY;
  return devil_effect_stop(s,entry,0x14b730,GH_ACTOR_MOTION_UNSUPPORTED);
 }
 if(!call->active){
  /* 2022D0 supplies unit scales, zero local XYZ/Euler, and part override FF:
   * the same effective configuration as 306528's default in this call. */
  if(!(word((const uint8_t *)b->work,0x5d8)&0x10000000u)){
   gh_effect_view est;
   const gh_effect_status lookup=gh_effect_est_at_00306030(b->storage.effects,0x58,3,&est);
   if(lookup)return devil_effect_stop(s,entry,0x306030,GH_ACTOR_MOTION_UNSUPPORTED);
   if(est.bytes){
    static const uint8_t parts[]={4,4,10,10,8,16,16,14};
    static const uint8_t textures[]={8,15,20,8,10,20,8,10};
    if(est.size<16+8*0x2f0||word(est.bytes,0)!=0x545345||word(est.bytes,4)!=8)
     return devil_effect_stop(s,entry,0x3066d0,GH_ACTOR_MOTION_UNSUPPORTED);
    for(unsigned i=0;i<8;i++){
     const uint8_t *row=est.bytes+16+i*0x2f0;
     if(row[0x128]||row[0x12a]||row[0x136]||row[0xbe]!=parts[i]||
        row[0xbc]!=1||row[0xbd]!=textures[i])
      return devil_effect_stop(s,entry,0x3066d0,GH_ACTOR_MOTION_UNSUPPORTED);
    }
   }
  }
  const gh_effect_create_request request={.bank=0x58,.index=3,
      .actor=&entry->object.view,.mode=1,.selector=-1};
  if(gh_effect_create_begin(call,&request))
   return devil_effect_stop(s,entry,0x306528,GH_ACTOR_MOTION_ARGUMENT);
 }
 gh_effect_create_bindings binding=*b;
 binding.model=model;
 gh_effect_status status=gh_game_effect_create_00306528(call,&binding);
 if(status)return devil_effect_stop(s,entry,call->frontier?call->frontier:0x306528,
     status==GH_EFFECT_BOUNDS?GH_ACTOR_MOTION_BOUNDS:GH_ACTOR_MOTION_UNSUPPORTED);
 /* The original ignores the effect result, disables 1868, and skips
  * 2949E8 for dependency 0220. The ordinary node cache is next. */
 uint8_t *p=entry->object.view.bytes;
 p[0x1868]=0;
 put(p,0x16f0,0x3f800000);
 s->solids.stage=4;
 return devil_effect_stop(s,entry,0x14b730,GH_ACTOR_MOTION_UNSUPPORTED);
}
