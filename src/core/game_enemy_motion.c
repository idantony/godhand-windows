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
   case 0:
    selection_pc=0x20393c;status=gh_enemy202_appearance0_select_0020393c(&e->request,r,&selected);break;
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
