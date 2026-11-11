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

static gh_scene_placement_status fail(gh_om303_state *state,uint32_t frontier,
 gh_scene_placement_status status)
{
 state->frontier=frontier;
 return status;
}

gh_scene_placement_status gh_om31a_initialize_001932d8(gh_om303_state *state,
 gh_om_object *object,const gh_om_initializer_bindings *bindings,int *original_return)
{
 if(!state||!object||!bindings||!original_return||object->source.type!=0x31a)
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
   const uint32_t zero[3]={0},extent[3]={0x3f4ccccd,0x3fd9999a,0x3f4ccccd};
   uint32_t frontier=0;
   int index;
   gh_hitarea_status hit=gh_hitarea_add_box_001fd9d8(&state->common.hitarea,
    bindings->shapes,actor+0x80,zero,zero,extent,&index,&frontier);
   if(hit)return fail(state,frontier?frontier:0x1fd9d8,GH_PLACE_UNSUPPORTED);
  }
  state->stage=2;
 }
 if(state->stage==2){
  put32(actor+0x5e8,0);
  put32(actor+0x5e4,0);
  put32(actor+0x5f0,0);
  put16(actor+0x5f6,0);
  put16(actor+0x54a,1);
  put32(actor+0x608,0); /* Native segments borrow replaces the guest pointer. */
  state->segments=actor+0x620;
  actor[0x60c]=4;
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
