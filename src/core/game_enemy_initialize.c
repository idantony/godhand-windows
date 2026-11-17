#include "game_enemy_initialize.h"
#include "game_enemy_row2.h"
#include "game_effect_create.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
gh_actor_model_status gh_enemy202_initialize_prefix_002022d0(gh_enemy_initialize*s,const gh_title_player_view*root,
 const gh_enemy_request*r,const gh_object_view*resource,gh_boot_work*work,gh_core_save_view save,const uint8_t*scenario,
 const uint8_t*override,gh_room_pools*pools,gh_heap*heap,const void*fog,gh_static_transform_policy policy){
 if(!s||!root||!root->bytes||root->size<GH_EM202_BYTES||!r||!resource||!work||!save.bytes||save.size<GH_CORE_SAVE_BYTES||!scenario)return GH_ACTOR_MODEL_ARGUMENT;
 uint16_t constructed_kind;
 memcpy(&constructed_kind,root->bytes+0x2fe,2);
 const int devil=constructed_kind==0x220&&root->original_vtable214==0x4479c8&&r->object_kind==0x220&&r->dependency_kind==0x220&&work->room==0x100&&
    r->flags==0x400000&&r->entry_number==0xff&&!r->appearance&&!r->dependency_flags;
 const int r103=constructed_kind==0x204&&root->original_vtable214==0x4479c8&&gh_enemy204_is_r103_entry(r,work->room);
 const int r104=constructed_kind==0x203&&root->original_vtable214==0x4479c8&&
    gh_enemy203_is_r104_entry(r,work->room);
 const int r200=constructed_kind==0x201&&root->original_vtable214==0x4479c8&&
    gh_enemy201_is_r200_entry(r,work->room);
 if(!devil&&!r103&&!r104&&!r200&&(r->object_kind!=0x202||(r->dependency_kind!=0x20a&&!gh_enemy202_is_room_supported_profile(r,work->room)))){s->frontier=0x2022d0;return GH_ACTOR_MODEL_UNSUPPORTED;}
 if(s->stage&&(s->root!=root||(s->request.object_kind!=r->object_kind||memcmp(s->request.position_xyz,r->position_xyz,12)||s->request.yaw_bits!=r->yaw_bits||s->request.flags!=r->flags||s->request.dependency_kind!=r->dependency_kind||s->request.dependency_flags!=r->dependency_flags||s->request.appearance!=r->appearance||s->request.entry_number!=r->entry_number)))return GH_ACTOR_MODEL_BUSY;
 uint8_t*p=root->bytes;
 if(!s->stage){s->root=root;s->request=*r;put(p,0x1620,r->object_kind);memcpy(p+0x1630,r->position_xyz,12);
  put(p,0x1640,r->yaw_bits);put(p,0x1644,r->flags);put(p,0x1648,r->dependency_kind);put(p,0x164c,r->dependency_flags);
  p[0x1650]=r->appearance;p[0x1651]=r->entry_number;put(p,0x564,r->dependency_kind);p[0x186b]=(uint8_t)((uint16_t)work->room>>8);s->stage=1;
 }
 if(s->stage==1){if((word(save.bytes,0x14)&0x4000000)||*scenario){if(!override){s->frontier=0x202434;return GH_ACTOR_MODEL_UNSUPPORTED;}p[0x186b]=*override;}
  static const uint16_t zero[]={0x6ec,0x6f0,0x6f8,0x6fc,0x700,0x704,0x708,0x734,0x6f4,0x70c,0x1964,0x196c,0x1970,0x1974,0x740,0x744,0x748,0x74c,0x750};
  for(unsigned i=0;i<sizeof(zero)/sizeof(zero[0]);i++)put(p,zero[i],0);s->stage=2;
 }
 if(s->stage==2&&(r104||(r103&&r->entry_number<2))&&!s->effect_bank.bytes){
  /* 2022D0 registers the dependency-specific embedded bundle before the model. */
  const size_t directory=r104?0x18ec:0x1a94;
  if(!s->effects){s->frontier=0x305758;return GH_ACTOR_MODEL_UNSUPPORTED;}
  if(!resource->bytes||resource->kind!=constructed_kind||!resource->file_extent_known||
     resource->file_extent>resource->size||resource->file_extent<directory+4){
   s->frontier=0x2024f8;return GH_ACTOR_MODEL_BOUNDS;
  }
  const uint32_t offset=word(resource->bytes,directory);
  if(offset<directory+4||offset>=resource->file_extent){s->frontier=0x2024f8;return GH_ACTOR_MODEL_BOUNDS;}
  const gh_effect_view bank={resource->bytes+offset,resource->file_extent-offset};
  const gh_effect_status error=r104?gh_game_effects_register_250_00305758(s->effects,bank):
      gh_game_effects_register_204_00305758(s->effects,bank);
  if(error){
   s->frontier=s->effects->frontier?s->effects->frontier:0x305758;
   return error==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MODEL_UNSUPPORTED:GH_ACTOR_MODEL_BOUNDS;
  }
  s->effect_bank=bank;
 }
 if(s->stage==2){int original=0;gh_actor_model_status e=gh_enemy_model_initialize_00208268(&s->model,root,resource,work,pools,heap,fog,policy,&original);if(e){s->frontier=s->model.frontier;return e;}if(!original){s->frontier=0x290770;return GH_ACTOR_MODEL_UNSUPPORTED;}s->stage=3;}
 /* Next original call after successful208268. No initialized enemy ack. */
 s->frontier=0x28ebf0;return GH_ACTOR_MODEL_UNSUPPORTED;
}
gh_actor_model_status gh_enemy204_release_effect_bank_00305778(gh_enemy_initialize*s)
{
 if(!s)return GH_ACTOR_MODEL_ARGUMENT;
 if(!s->effect_bank.bytes)return GH_ACTOR_MODEL_OK;
 int borrowed;
 if(gh_effect_create_source_has_live_refs(s->effects,s->effect_bank.bytes,s->effect_bank.size,&borrowed)||
    borrowed||(s->request.dependency_kind==0x250?
       gh_game_effects_release_250_00305778(s->effects,s->effect_bank):
       gh_game_effects_release_204_00305778(s->effects,s->effect_bank))){
  s->frontier=0x305778;return GH_ACTOR_MODEL_BUSY;
 }
 s->effect_bank=(gh_effect_view){0};
 return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_enemy202_initialize_abort_prefix(gh_enemy_initialize*s)
{
 if(!s)return GH_ACTOR_MODEL_ARGUMENT;
 gh_actor_model_status error=gh_enemy_model_release_prefix(&s->model);
 if(error)return error;
 error=gh_enemy204_release_effect_bank_00305778(s);
 if(error)return error;
 *s=(gh_enemy_initialize){0};
 return GH_ACTOR_MODEL_OK;
}
