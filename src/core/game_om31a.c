#include "game_om31a.h"
#include "game_om_base.h"
#include "game_om_fragments.h"
#include <string.h>

static void put32(uint8_t *p,uint32_t value){memcpy(p,&value,4);}
static void put16(uint8_t *p,uint16_t value){memcpy(p,&value,2);}

gh_scene_placement_status gh_om31a_construct_001931f0(void *data,size_t size,
 gh_title_player_view *out)
{
 if(!data||!out)return GH_PLACE_ARGUMENT;
 if(size<GH_OM31A_BYTES)return GH_PLACE_BOUNDS;
 gh_scene_placement_status status=gh_om_base_construct_001b6e90(data,size,out);
 if(status)return status;
 uint8_t *actor=data;
 /* Final vtable after 1C27E8, then four fragment records. */
 put32(actor+0x214,0x4246a8);
 out->original_vtable214=0x4246a8;
 const uint32_t vf0[4]={0,0,0,0x3f800000};
 for(unsigned i=0;i<4;i++){
  uint8_t *fragment=actor+0x620+i*0x60;
  for(unsigned j=0;j<5;j++)memcpy(fragment+j*16,vf0,sizeof(vf0));
  fragment[0x54]=0;
 }
 return GH_PLACE_OK;
}

gh_scene_placement_status gh_om_r101_door_construct(unsigned kind,void *data,size_t size,gh_title_player_view *out)
{
 if(!data||!out)return GH_PLACE_ARGUMENT;
 if(kind!=0x313&&kind!=0x330)return GH_PLACE_ARGUMENT;
 if(size<0x8f0)return GH_PLACE_BOUNDS;
 gh_scene_placement_status e=gh_om_door_construct_001bbeb8(data,size,out);
 if(e)return e;
 uint8_t *actor=data;
 out->original_vtable214=kind==0x313?0x424128:0x425b90;
 put32(actor+0x214,out->original_vtable214);
 const uint32_t vf0[4]={0,0,0,0x3f800000};
 for(unsigned i=0;i<7;++i){
  uint8_t *fragment=actor+0x650+i*0x60;
  for(unsigned j=0;j<5;++j)memcpy(fragment+j*16,vf0,sizeof(vf0));
  fragment[0x54]=0;
 }
 return GH_PLACE_OK;
}

gh_scene_placement_status gh_om_r101_fragment_construct(unsigned kind,void *data,size_t size,gh_title_player_view *out)
{
 if((kind!=0x317&&kind!=0x318)||!data||!out)return GH_PLACE_ARGUMENT;
 const int corner=kind==0x317;
 if(size<(corner?0x860u:0x800u))return GH_PLACE_BOUNDS;
 gh_scene_placement_status e=gh_om_base_construct_001b6e90(data,size,out);
 if(e)return e;
 uint8_t *actor=data;
 out->original_vtable214=corner?0x4244d8:0x4245c0;
 put32(actor+0x214,out->original_vtable214);
 const uint32_t vf0[4]={0,0,0,0x3f800000};
 for(unsigned i=0;i<(corner?6u:5u);++i){
  uint8_t *fragment=actor+0x620+i*0x60;
  for(unsigned j=0;j<5;++j)memcpy(fragment+j*16,vf0,sizeof(vf0));
  fragment[0x54]=0;
 }
 return GH_PLACE_OK;
}

gh_scene_placement_status gh_om351_construct_001a5fd8(void *data,size_t size,gh_title_player_view *out)
{
 if(!data||!out)return GH_PLACE_ARGUMENT;
 if(size<0x690)return GH_PLACE_BOUNDS;
 gh_scene_placement_status result=gh_om_base_construct_001b6e90(data,size,out);
 if(result)return result;
 uint8_t *actor=data;
 out->original_vtable214=0x427400;
 put32(actor+0x214,out->original_vtable214);
 const uint32_t vf0[4]={0,0,0,0x3f800000};
 for(size_t offset=0x610;offset<=0x670;offset+=0x10)memcpy(actor+offset,vf0,sizeof(vf0));
 actor[0x684]=0;
 return GH_PLACE_OK;
}

gh_scene_placement_status gh_om351_initialize_001a6098(gh_om_init_state *state,gh_om_object *object,
 const gh_om_initializer_bindings *bindings,int *original_return)
{
 if(!state||!object||!bindings||!original_return||object->source.type!=0x351)return GH_PLACE_ARGUMENT;
 *original_return=0;
 int common_return=0;
 gh_scene_placement_status status=gh_om_common_initialize_001b6fb8(state,object,bindings,&common_return);
 if(status)return status;
 if(!common_return){state->frontier=0x1b6fb8;return GH_PLACE_UNSUPPORTED;}
 uint8_t *actor=object->view.bytes;
 if(state->stage==3){
  if(!bindings->init){state->frontier=0x1fe218;return GH_PLACE_UNSUPPORTED;}
  uint32_t frontier=0;int created=0;
  gh_hitarea_status hit=gh_hitarea_object_create_001fe218(bindings->init,&object->view,1,&state->hitarea,&created,&frontier);
  if(hit){state->frontier=frontier?frontier:0x1fe218;return GH_PLACE_UNSUPPORTED;}
  state->hitarea_created=(uint8_t)created;
  put32(actor+0x608,0);
  state->stage=4;
 }
 if(state->stage==4){
  if(state->hitarea_created){
   if(!bindings->shapes){state->frontier=0x1fdc00;return GH_PLACE_UNSUPPORTED;}
   const uint32_t zero[3]={0};uint32_t frontier=0;int index;
   gh_hitarea_status hit=gh_hitarea_add_cylinder_001fdc00(&state->hitarea,bindings->shapes,
       actor+0x80,zero,zero,0x400f5c29,0x3f4ccccd,&index,&frontier);
   if(hit){state->frontier=frontier?frontier:0x1fdc00;return GH_PLACE_UNSUPPORTED;}
  }
  actor[0x604]=0;
  put16(actor+0x548,0x28);put16(actor+0x54a,0x28);
  state->stage=5;
 }
 state->frontier=0;
 *original_return=1;
 return GH_PLACE_OK;
}

static gh_scene_placement_status fail(gh_om303_state *state,uint32_t frontier,
 gh_scene_placement_status status)
{
 state->frontier=frontier;
 return status;
}

gh_scene_placement_status gh_om31a_initialize_001932d8(gh_om303_state *state,
 gh_om_object *object,const gh_om_initializer_bindings *bindings,int *original_return)
{
 if(!state||!object||!bindings||!original_return||
    (object->source.type!=0x31a&&object->source.type!=0x317&&object->source.type!=0x318))
  return GH_PLACE_ARGUMENT;
 *original_return=0;
 state->frontier=0;
 int common_return=0;
 gh_scene_placement_status status=gh_om_common_initialize_001b6fb8(
  &state->common,object,bindings,&common_return);
 if(status)return fail(state,state->common.frontier,status);
 if(!common_return)return fail(state,0x1b6fb8,GH_PLACE_UNSUPPORTED);
 uint8_t *actor=object->view.bytes;
 if(!state->stage){
  if(!bindings->init)return fail(state,0x1fe218,GH_PLACE_UNSUPPORTED);
  uint32_t frontier=0;
  int created=0;
  gh_hitarea_status hit=gh_hitarea_object_create_001fe218(
   bindings->init,&object->view,2,&state->common.hitarea,&created,&frontier);
  if(hit)return fail(state,frontier?frontier:0x1fe218,GH_PLACE_UNSUPPORTED);
  state->common.hitarea_created=(uint8_t)created;
  put32(actor+0x600,0); /* Native HitArea view replaces the guest pointer. */
  state->stage=1;
 }
 if(state->stage==1){
  if(state->common.hitarea_created){
   if(!bindings->shapes)return fail(state,0x1fd9d8,GH_PLACE_UNSUPPORTED);
   const uint32_t zero[3]={0};
   const uint32_t *extent=(const uint32_t[3]){0x3f4ccccd,0x3fd9999a,0x3f4ccccd};
   const uint32_t corner_extent[3]={0x3ecccccd,0x3f99999a,0x40133333};
   const uint32_t corner_offset[3]={0,0,0x3f800000};
   const uint32_t short_extent[3]={0x3fa66666,0x3ff33333,0x3f4ccccd};
   const uint32_t *offset=zero;
   if(object->source.type==0x317){
    extent=corner_extent;
    offset=corner_offset;
   }else if(object->source.type==0x318)extent=short_extent;
   uint32_t frontier=0;
   int index;
   gh_hitarea_status hit=gh_hitarea_add_box_001fd9d8(&state->common.hitarea,
    bindings->shapes,actor+0x80,offset,zero,extent,&index,&frontier);
   if(hit)return fail(state,frontier?frontier:0x1fd9d8,GH_PLACE_UNSUPPORTED);
  }
  state->stage=2;
 }
 if(state->stage==2){
  if(state->common.hitarea_created&&object->source.type==0x317){
   const uint32_t zero[3]={0},offset[3]={0xbf800000,0,0},extent[3]={0x40133333,0x3f99999a,0x3ecccccd};
   uint32_t frontier=0;int index;
   gh_hitarea_status hit=gh_hitarea_add_box_001fd9d8(&state->common.hitarea,bindings->shapes,
    actor+0x80,offset,zero,extent,&index,&frontier);
   if(hit)return fail(state,frontier?frontier:0x1fd9d8,GH_PLACE_UNSUPPORTED);
  }
  state->stage=3;
 }
 if(state->stage==3){
  put32(actor+0x5e8,0);
  put32(actor+0x5e4,0);
  put32(actor+0x5f0,0);
  put16(actor+0x5f6,0);
  put16(actor+0x54a,1);
  put32(actor+0x608,0); /* Native segments borrow replaces the guest pointer. */
  state->segments=actor+0x620;
  actor[0x60c]=object->source.type==0x317?6:object->source.type==0x318?5:4;
  put32(actor+0x5ec,7);
  put16(actor+0x548,1);
  put16(actor+0x5f4,0xffff);
  status=gh_om_fragments_initialize_001c28a0(&state->common);
  if(status)return fail(state,state->common.frontier,status);
  state->stage=5;
 }
 *original_return=1;
 return GH_PLACE_OK;
}
