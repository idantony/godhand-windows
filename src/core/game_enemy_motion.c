#include "game_enemy_motion.h"
#include "game_enemy_row2.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
gh_actor_motion_status gh_enemy_motion_initialize_002036b0(gh_enemy_motion*s,gh_enemy_hp*hp,const struct gh_motion_ik_globals*ik,gh_motion_effect_consumer effects,void*context){
 if(!s||!hp||hp->stage!=1||!hp->tail||!hp->tail->entry)return GH_ACTOR_MOTION_ARGUMENT;
 if(s->stage&&s->hp!=hp)return GH_ACTOR_MOTION_BUSY;
 gh_enemy_entry*e=hp->tail->entry;uint8_t*p=e->object.view.bytes;if(e->request.object_kind!=0x202||(e->request.dependency_kind!=0x20a&&!gh_enemy202_is_r100_supported_profile(&e->request)))return GH_ACTOR_MOTION_ARGUMENT;
 if(!s->stage){const gh_object_view*r=&e->resource;if(!r->bytes||!r->file_extent_known||r->file_extent>r->size||r->file_extent<0xc10)return GH_ACTOR_MOTION_BOUNDS;
  size_t a,b;
  uint8_t substate,detail=0;
  if(gh_enemy202_is_r100_supported_profile(&e->request)){
   gh_enemy_initial_motion_selection selected;
   gh_actor_model_status status;
   uint32_t selection_pc;
   switch(e->request.appearance){
   case 1:
    selection_pc=0x203a9c;status=gh_enemy202_appearance1_select_00203a9c(&e->request,r,&selected);break;
   case 0:
    selection_pc=0x20393c;status=gh_enemy202_appearance0_select_0020393c(&e->request,r,&selected);break;
   case 4:
    selection_pc=0x203a18;status=gh_enemy202_appearance4_select_00203a18(&e->request,r,&selected);break;
   case 5:
    selection_pc=0x203b3c;status=gh_enemy202_appearance5_select_00203b3c(&e->request,r,&selected);break;
   case 9:
    selection_pc=0x20393c;status=gh_enemy202_appearance9_select_0020393c(&e->request,r,&selected);break;
   default:
    selection_pc=0x203c38;status=gh_enemy202_appearance2_select_00203c38(&e->request,r,&selected);break;
   }
   if(status){s->frontier=selection_pc;return status==GH_ACTOR_MODEL_BOUNDS?GH_ACTOR_MOTION_BOUNDS:GH_ACTOR_MOTION_UNSUPPORTED;}
   a=selected.motion_offset;b=selected.sequence_offset;substate=selected.substate;detail=selected.phase_detail;
  }else{
   unsigned app=e->request.appearance;
   if(app!=0&&app!=3){s->frontier=0x203918;return GH_ACTOR_MOTION_UNSUPPORTED;}
   size_t first=app==3?0xc08:0x828;
   a=word(r->bytes,first);b=word(r->bytes,first+4);substate=app==3?0x67:1;
   if(a>=r->file_extent||b>=r->file_extent)return GH_ACTOR_MOTION_BOUNDS;
  }
  put(p,0x54c,0x40a00000);p[0x2f4]=0;p[0x2f5]=substate;p[0x2f6]=0;p[0x2f7]=detail;
  s->hp=hp;s->motion_offset=a;s->sequence_offset=b;s->stage=1;
 }
 if(s->stage==1){gh_actor_motion_bindings b={.model=&e->initialize.model.model,.resource=e->resource.bytes,.resource_bytes=e->resource.file_extent,.work=e->b.work,.effect=effects,.context=context,.ik_globals=ik};
  gh_actor_motion_status status=gh_actor_motion_begin_002a8578(&s->motion,&b,s->motion_offset,s->sequence_offset,0);if(status){s->frontier=s->motion.frontier;return status;}s->stage=2;
 }
 if((word(e->b.save.bytes,0x14)&0x4000000)||*e->b.scenario14)put(p,0x16d4,word(p,0x16d4)|2);
 s->frontier=0x157c18;e->frontier=s->frontier;return GH_ACTOR_MOTION_UNSUPPORTED;
}

gh_actor_motion_status gh_enemy_motion_initial_step_00204abc(gh_enemy_motion*s,float rate,gh_static_transform_policy policy,int*ended){
 if(!s||!ended||s->stage<2||!s->hp||!s->hp->tail||!s->hp->tail->entry)return GH_ACTOR_MOTION_ARGUMENT;
 gh_enemy_entry*e=s->hp->tail->entry;uint8_t*p=e->object.view.bytes;*ended=0;
 if(s->stage==2){gh_actor_motion_status status=gh_actor_motion_step_00157c18(&s->motion,rate,policy,ended,&s->frontier);if(status)return status;s->initial_ended=*ended;s->stage=3;}
 *ended=s->initial_ended;
 if(s->stage==3){if(word(p,0x1644)&0x80000){s->frontier=0x204ae8;return GH_ACTOR_MOTION_UNSUPPORTED;}
  gh_actor_model_status status=gh_actor_model_update_hierarchy_00149eb8(&e->initialize.model.model,policy);if(status){s->frontier=e->initialize.model.model.frontier;return GH_ACTOR_MOTION_UNSUPPORTED;}s->stage=4;
 }
 s->frontier=0x152a20;e->frontier=s->frontier;return GH_ACTOR_MOTION_UNSUPPORTED;
}

static gh_actor_motion_status devil_stop(gh_enemy_devil_initialize *s,uint32_t pc,
 gh_actor_motion_status status)
{
 s->owner->entry.frontier=s->frontier=pc;
 return status;
}

gh_actor_motion_status gh_enemy_devil_initialize_0028ebf0(gh_enemy_devil_initialize *s,
 gh_enemy_devil_entry *owner,const gh_enemy_devil_bindings *b)
{
 if(!s||!owner||!b||!b->cockpit||!b->solids||!owner->parent)
  return GH_ACTOR_MOTION_ARGUMENT;
 if(s->owner&&(s->owner!=owner||s->bindings.cockpit!=b->cockpit||
    s->bindings.solids!=b->solids||s->bindings.ik!=b->ik||
    s->bindings.sound!=b->sound||s->bindings.sound_context!=b->sound_context||
    s->bindings.rate!=b->rate))return GH_ACTOR_MOTION_BUSY;
 if(!s->owner){s->owner=owner;s->bindings=*b;}
 gh_enemy_entry *e=&owner->entry;
 if(!s->stage){
  gh_actor_model_status status=gh_enemy_devil_continue_002022d0(&s->tail,owner,b->cockpit);
  if(status!=GH_ACTOR_MODEL_UNSUPPORTED||s->tail.stage!=3||s->tail.frontier!=0x202b88)
   return devil_stop(s,s->tail.frontier?s->tail.frontier:0x28ebf0,GH_ACTOR_MOTION_UNSUPPORTED);
  s->stage=1;
 }
 uint8_t *p=e->object.view.bytes;
 if(s->stage==1){
  gh_actor_model_status status=gh_enemy_hp_initialize_00202b88(
      &s->hp,&s->tail,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE);
  if(status!=GH_ACTOR_MODEL_UNSUPPORTED||s->hp.stage!=1||s->hp.frontier!=0x2036b0)
   return devil_stop(s,s->hp.frontier?s->hp.frontier:0x202b88,GH_ACTOR_MOTION_UNSUPPORTED);
  put(p,0x54c,0x40a00000);
  s->cue=(gh_motion_sound_request){.model=&e->initialize.model.model,
      .actor=&e->object.view,.bank=0,.program=5};
  s->stage=2;
 }
 if(s->stage==2){
  if(!b->sound)return devil_stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
  uint32_t handle,pc=0;
  gh_actor_motion_status status=b->sound(b->sound_context,&s->cue,&handle,&pc);
  if(status)return devil_stop(s,pc?pc:0x2cba48,status);
  s->stage=3;
 }
 if(s->stage==3){
  uint32_t pc=0;
  if(gh_collision_solid_set_enabled_001351d8(b->solids,&e->object.view,0,&pc))
   return devil_stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
  /* 26EB00 starts the original opacity ramp. */
  put(p,0x54c,0x41700000);put(p,0x24c,0);put(p,0x1704,0x3f800000);
  put(p,0x250,word(p,0x250)|0x10u);
  p[0x2f4]=0;p[0x2f5]=4;p[0x2f6]=2;p[0x2f7]=0;
  s->stage=4;
 }
 if(s->stage==4){
  const gh_object_view *r=&e->resource;
  if(!r->bytes||!r->file_extent_known||r->file_extent>r->size||r->file_extent<0x1d80)
   return devil_stop(s,0x2036b0,GH_ACTOR_MOTION_BOUNDS);
  gh_enemy_motion *m=&s->motion;
  gh_actor_motion_bindings bind={.model=&e->initialize.model.model,.resource=r->bytes,
      .resource_bytes=r->file_extent,.work=e->b.work,.ik_globals=b->ik,
      .sound=b->sound,.sound_context=b->sound_context};
  m->motion_offset=word(r->bytes,0x1d78);m->sequence_offset=word(r->bytes,0x1d7c);
  gh_actor_motion_status status=gh_actor_motion_begin_002a8578(
      &m->motion,&bind,m->motion_offset,m->sequence_offset,0);
  if(status)return devil_stop(s,m->motion.frontier?m->motion.frontier:0x2a8578,status);
  m->hp=&s->hp;m->stage=2;
  if((word(e->b.save.bytes,0x14)&0x4000000)||*e->b.scenario14)
   put(p,0x16d4,word(p,0x16d4)|2u);
  s->stage=5;
 }
 int ended;
 gh_actor_motion_status status=gh_enemy_motion_initial_step_00204abc(
     &s->motion,b->rate,e->b.policy,&ended);
 return devil_stop(s,s->motion.frontier?s->motion.frontier:0x157c18,status);
}
