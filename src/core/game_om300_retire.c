#include "game_collision_pair_shapes.h"
#include "game_hitarea_objects.h"
#include "game_om300_retire.h"
#include "game_om303.h"
#include "game_om307.h"
#include "game_om322.h"
#include "game_om323.h"
#include "game_om346.h"
#include "game_om35d.h"
#include "game_hitarea_internal.h"
#include <string.h>
#include <stdlib.h>

static uint32_t word(const uint8_t *p)
{
 uint32_t value;
 memcpy(&value,p,4);
 return value;
}

static void put(uint8_t *p,uint32_t value)
{
 memcpy(p,&value,4);
}

static uint32_t om_kind(const gh_om_object *actor)
{
 return actor->runtime_created?actor->runtime_kind:actor->source.type;
}

static uint32_t class_vtable(const gh_om_object *actor)
{
 if(om_kind(actor)==0x300)return 0x422f90;
 if(om_kind(actor)==0x303)return 0x423230;
 if(om_kind(actor)==0x304)return 0x423338;
 if(om_kind(actor)==0x305)return 0x423400;
 if(om_kind(actor)==0x307)return 0x423608;
 if(om_kind(actor)==0x30a)return 0x4238d8;
 if(om_kind(actor)==0x30e)return 0x423c70;
 if(om_kind(actor)==0x310)return 0x423e48;
 if(om_kind(actor)==0x320)return 0x424c30;
 if(om_kind(actor)==0x322)return 0x424e18;
 if(om_kind(actor)==0x323)return 0x424f28;
 if(om_kind(actor)==0x324)return 0x425000;
 if(om_kind(actor)==0x33f)return 0x4267f8;
 if(om_kind(actor)==0x346)return 0x426e78;
 if(om_kind(actor)==0x354)return 0x427690;
 if(om_kind(actor)==0x35d)return 0x428090;
 if(om_kind(actor)==0x309)return 0x4237e8;
 if(actor->runtime_created&&om_kind(actor)==0x3eb)return 0x4296f0;
 return 0;
}

static gh_om_object *registered_om(gh_om_factory *factory,uint32_t handle)
{
 void *object=NULL;
 if(!factory||!factory->bound||
    gh_room_object_lookup(factory->bindings.pools,handle,GH_ROOM_OBJECT_OM,factory,&object))return NULL;
 gh_om_object *actor=object;
 if(!actor||actor->handle!=handle||!actor->registered||!actor->view.bytes||!actor->initialization)
  return NULL;
 const uint32_t vtable=class_vtable(actor);
 if(!vtable||actor->view.original_vtable214!=vtable)return NULL;
 if(actor->runtime_created)
  return om_kind(actor)==0x3eb&&actor->view.size>=0x610&&
         actor->initialization->stage==3?actor:NULL;
 if(om_kind(actor)==0x300)
  return actor->view.size>=0x648&&actor->initialization->stage==7?actor:NULL;
 if(om_kind(actor)==0x307)
  return actor->view.size>=0xce0&&((gh_om307_state *)actor->initialization)->stage==8?actor:NULL;
 if(om_kind(actor)==0x322)
  return actor->view.size>=0x9e0&&((gh_om322_state *)actor->initialization)->stage==6?actor:NULL;
 if(om_kind(actor)==0x323)
  return actor->view.size>=0x9e0&&((gh_om323_state *)actor->initialization)->stage==6?actor:NULL;
 if(om_kind(actor)==0x346)
  return actor->view.size>=0xda0&&((gh_om346_state *)actor->initialization)->stage==8?actor:NULL;
 if(om_kind(actor)==0x354)
  return actor->view.size>=0x6b0&&actor->initialization->stage==6?actor:NULL;
 if(om_kind(actor)==0x35d)
  return actor->view.size>=0xb00&&((gh_om35d_state *)actor->initialization)->stage==6?actor:NULL;
 size_t extent;
 switch(om_kind(actor)){
 case 0x303:extent=0x9e0;break;
 case 0x304:extent=0x850;break;
 case 0x305:extent=0x860;break;
 case 0x30a:extent=0x9c0;break;
 case 0x30e:extent=0x610;break;
 case 0x310:extent=0x920;break;
 case 0x320:extent=0x800;break;
 case 0x324:extent=0x610;break;
 case 0x33f:extent=0xb40;break;
 case 0x309:extent=0x9c0;break;
 default:return NULL;
 }
 unsigned stage=om_kind(actor)==0x310?6:5;
 return actor->view.size>=extent&&((gh_om303_state *)actor->initialization)->stage==stage?actor:NULL;
}

static gh_room_pool_status fail(uint32_t *frontier,uint32_t entry)
{
 if(frontier)*frontier=entry;
 return GH_POOL_UNSUPPORTED;
}

uint32_t gh_om_retire_entry(uint32_t kind)
{
 switch(kind){
 case 0x300:return 0x1867c0;
 case 0x303:return 0x187a50;
 case 0x304:return 0x187d10;
 case 0x305:return 0x188af0;
 case 0x307:return 0x189008;
 case 0x30a:return 0x18c120;
 case 0x30e:return 0x18ecd0;
 case 0x310:return 0x18f1a0;
 case 0x320:return 0x195ca8;
 case 0x322:return 0x1960e0;
 case 0x323:return 0x197708;
 case 0x324:return 0x198ce0;
 case 0x33f:return 0x19f130;
 case 0x346:return 0x1a0df0;
 case 0x354:return 0x1a81d8;
 case 0x35d:return 0x1ae8b0;
 case 0x309:return 0x18b318;
 case 0x3eb:return 0x1bf4b8;
 default:return 0;
 }
}

gh_room_pool_status gh_om_retire_mark_00312708(gh_om_factory *factory,uint32_t handle,
 gh_se_voice_engine *voices,uint32_t *frontier)
{
 if(frontier)*frontier=0;
 gh_om_object *object=registered_om(factory,handle);
 if(!object)return GH_POOL_ARGUMENT;
 uint8_t *actor=object->view.bytes;
 if(word(actor+0x250)&0x80000000u)return GH_POOL_OK;
 if(word(actor+0x214)!=class_vtable(object))return fail(frontier,0x312748);
 gh_actor_model *model=&object->initialization->model;
 if(!voices||!model->ready||model->native_identity!=model||model->root.bytes!=actor)
  return fail(frontier,0x2cbe18);
 if(gh_se_voice_detach_actor_002ce488(voices,model)){
  uint32_t child=gh_se_voice_frontier(voices);
  return fail(frontier,child?child:0x2ce488);
 }
 /* Each admitted class dispatches (0,14E928). */
 put(actor+0x250,word(actor+0x250)|0x80000002u);
 return GH_POOL_OK;
}

static gh_hitarea_status release_hitarea(gh_hitarea_object *object,uint32_t *frontier)
{
 int original;
 gh_hitarea_status status=gh_hitarea_object_clear_shapes_001fd580(object,&original,frontier);
 if(status)return status;
 return gh_hitarea_object_release_empty_001fe278(object,frontier);
}

static gh_room_pool_status release_remaining_hitareas(gh_om300_retirement *state,
 gh_game_init_state *hitarea,gh_title_player_view *actor)
{
 if(!hitarea||!hitarea->hitarea_native||hitarea->hitarea_native->owner!=hitarea)
  return fail(&state->frontier,0x1fe370);
 while(state->hitarea_cursor<416){
  unsigned i=state->hitarea_cursor, group=i<160?1:2;
  if(hitarea->hitarea_native->actors[i]==actor){
   uint8_t *bytes=group==1?hitarea->object_pool+0x60*i:hitarea->object_pool+0x3c00+0x70*(i-160);
   if(bytes[0x4a]){
    gh_hitarea_object unit={hitarea,bytes,group==1?0x60:0x70,hitarea->hitarea_generation,
     (uint16_t)i,(uint8_t)group,0,hitarea->hitarea_native->serials[i]};
    if(release_hitarea(&unit,&state->frontier))
     return fail(&state->frontier,state->frontier?state->frontier:0x1fe370);
   }
  }
  ++state->hitarea_cursor;
 }
 return GH_POOL_OK;
}

static gh_room_pool_status release_resource(gh_om300_retirement *state)
{
 const gh_om_factory_bindings *factory=&state->bindings.factory->bindings;
 gh_om_object *object=state->object;
 const uint32_t kind=om_kind(object);
 gh_game_objects *owner=factory->room_objects;
 int ready=0;
 gh_object_status result=gh_game_objects_check_ready_001ff238(owner,kind,factory->resources,&ready,&state->frontier);
 if(result!=GH_OBJECT_OK&&result!=GH_OBJECT_MISSING)
  return fail(&state->frontier,state->frontier?state->frontier:0x1fe9b8);
 if(!ready){
  owner=factory->objects;
  result=gh_game_objects_check_ready_001ff238(owner,kind,factory->resources,&ready,&state->frontier);
  if(result!=GH_OBJECT_OK&&result!=GH_OBJECT_MISSING)
   return fail(&state->frontier,state->frontier?state->frontier:0x1fe9b8);
 }
 if(!ready)return fail(&state->frontier,0x1fe9b8);
 if(owner!=object->resource_manager||!object->acquired)
  return fail(&state->frontier,0x1fed78);
 if(gh_game_objects_release_event_001fed78(owner,kind,object->resource.bytes,
    state->bindings.release_sound_slot,state->bindings.sound_context,&state->frontier))
  return fail(&state->frontier,state->frontier?state->frontier:0x1fed78);
 object->acquired=0;
 return GH_POOL_OK;
}

static gh_room_pool_status retire_prefix(gh_om300_retirement *state,
 const gh_om300_retire_bindings *bindings,uint32_t handle)
{
 if(!state||!bindings||!bindings->factory||!bindings->tables||!bindings->scroll||!bindings->solids)
  return GH_POOL_ARGUMENT;
 gh_om_object *object=registered_om(bindings->factory,handle);
 if(!object)return GH_POOL_ARGUMENT;
 if(state->stage){
  const gh_om300_retire_bindings *old=&state->bindings;
  if(state->object!=object||state->handle!=handle||old->factory!=bindings->factory||
     old->tables!=bindings->tables||old->scroll!=bindings->scroll||old->solids!=bindings->solids||
     old->release_sound_slot!=bindings->release_sound_slot||old->sound_context!=bindings->sound_context)
   return GH_POOL_ARGUMENT;
 }else{
  uint8_t *actor=object->view.bytes;
  if(word(actor+0x214)!=class_vtable(object)||(word(actor+0x250)&0xa0000000u)!=0xa0000000u||
     actor[0x2fe]!=(om_kind(object)&255)||actor[0x2ff]!=3||!bindings->factory->bindings.resources)
   return GH_POOL_ARGUMENT;
  state->object=object;state->handle=handle;state->bindings=*bindings;state->stage=1;
 }
 uint8_t *actor=object->view.bytes;
 gh_om_init_state *init=object->initialization;
 state->frontier=0;
 if(state->stage==1){
  switch(om_kind(object)){
  case 0x300:case 0x354:put(actor+0x214,0x4292f0);break;
  case 0x303:put(actor+0x214,0x429c80);break;
  default:put(actor+0x214,class_vtable(object));break;
  }
  size_t box_field=0x600;
  switch(om_kind(object)){
  case 0x304:box_field=0x604;break;
  case 0x30a:case 0x33f:case 0x309:box_field=0x650;break;
  case 0x310:box_field=0x654;break;
  default:break;
  }
  if(om_kind(object)!=0x3eb&&word(actor+box_field))return fail(&state->frontier,0x1fe278);
  if(om_kind(object)==0x3eb&&init->hitarea_created)return fail(&state->frontier,0x1b6f38);
  if(init->hitarea_created){
   if(release_hitarea(&init->hitarea,&state->frontier))
    return fail(&state->frontier,state->frontier?state->frontier:0x1fe278);
   init->hitarea_created=0;
  }
  state->stage=2;
 }
 if(state->stage==2){
  if(om_kind(object)==0x300||om_kind(object)==0x354){
   if(gh_om_door_release_collision_001bd3f0(&init->geometry_state))
    return fail(&state->frontier,0x1bd3f0);
  }else if(om_kind(object)==0x307||om_kind(object)==0x346){
   gh_hitarea_object *box_b2,*box_a;
   uint8_t *b2_created,*a_created;
   if(om_kind(object)==0x307){
    gh_om307_state *three=(gh_om307_state *)init;
    box_b2=&three->box_b2;box_a=&three->box_a;
    b2_created=&three->b2_created;a_created=&three->a_created;
   }else{
    gh_om346_state *three=(gh_om346_state *)init;
    box_b2=&three->box_b2;box_a=&three->box_a;
    b2_created=&three->b2_created;a_created=&three->a_created;
   }
   if(word(actor+0x604))return fail(&state->frontier,0x1fe278);
   if(*b2_created){
    if(release_hitarea(box_b2,&state->frontier))
     return fail(&state->frontier,state->frontier?state->frontier:0x1fe278);
    *b2_created=0;
   }
   if(word(actor+0x608))return fail(&state->frontier,0x1fe148);
   if(*a_created){
    if(release_hitarea(box_a,&state->frontier))
     return fail(&state->frontier,state->frontier?state->frontier:0x1fe148);
    *a_created=0;
   }
  }else if(om_kind(object)==0x3eb){
   /* 1BF4B8 enters 1B6F38 directly; +600 holds the tutorial ordinal. */
  }else if(om_kind(object)==0x35d){
   if(word(actor+0x604))return fail(&state->frontier,0x1fe148);
  }else if(om_kind(object)==0x304||om_kind(object)==0x310){
   size_t sphere_field=om_kind(object)==0x310?0x658:0x608;
   if(word(actor+sphere_field))return fail(&state->frontier,0x1fe148);
   gh_om303_state *fragment=(gh_om303_state *)init;
   if(fragment->sphere_created){
    if(release_hitarea(&fragment->sphere_unit,&state->frontier))
     return fail(&state->frontier,state->frontier?state->frontier:0x1fe148);
    fragment->sphere_created=0;
   }
   if(om_kind(object)==0x310)put(actor+0x214,0x429dd8);
  }else if(om_kind(object)==0x322||om_kind(object)==0x323){
   if(word(actor+0x604))return fail(&state->frontier,0x1fe148);
   gh_hitarea_object *unit;
   uint8_t *created;
   if(om_kind(object)==0x322){
    gh_om322_state *actor_state=(gh_om322_state *)init;
    unit=&actor_state->box_a;created=&actor_state->a_created;
   }else{
    gh_om323_state *actor_state=(gh_om323_state *)init;
    unit=&actor_state->box_a;created=&actor_state->a_created;
   }
   if(*created){
    if(release_hitarea(unit,&state->frontier))
     return fail(&state->frontier,state->frontier?state->frontier:0x1fe148);
    *created=0;
   }
  }else if(om_kind(object)==0x30a||om_kind(object)==0x33f||om_kind(object)==0x309){
   put(actor+0x214,0x429dd8); /* 1C7DE0 */
  }else if(om_kind(object)==0x30e||om_kind(object)==0x324){
   put(actor+0x214,0x4297d8); /* 1BFB00 */
  }else {
   if(om_kind(object)==0x305||om_kind(object)==0x320)put(actor+0x214,0x429c80);
   /* 1C286C reloads +600, just cleared, as the argument for the
    * +604-positive branch. 1FE278(NULL) returns. The sphere remains
    * associated until base 1FE370, which uses the correct A/B class. */
   if(word(actor+0x604))return fail(&state->frontier,0x1c286c);
  }
  state->stage=3;
 }
 if(state->stage==3){
  put(actor+0x214,0x428a20);
  uint64_t key;
  memcpy(&key,actor+0x540,8);
  uint32_t changed;
  if(gh_collision_scroll_set_key_enabled_001331b8(bindings->tables,bindings->scroll,key,0,&changed))
   return fail(&state->frontier,0x1331b8);
  state->stage=4;
 }
 if(state->stage==4){
  if(gh_collision_solid_release_unit_001347e8(bindings->solids,&object->view,&state->frontier))
   return fail(&state->frontier,state->frontier?state->frontier:0x1347e8);
  if(om_kind(object)==0x35d){
   gh_om35d_state *solid=(gh_om35d_state *)init;
   solid->solid_created=0;solid->solid_unit=NULL;solid->sphere=NULL;
  }
  /* The base destructor calls 312708 again here; signed +250 returns. */
  state->stage=5;
 }
 if(state->stage==5){
  if(release_remaining_hitareas(state,init->bindings->init,&object->view))return GH_POOL_UNSUPPORTED;
  if(om_kind(object)==0x303)((gh_om303_state *)init)->sphere_created=0;
  put(actor+0x214,0x44ae28);
  put(actor+0x214,0x41ebe8);
  put(actor+0x214,0x41eaf0);
  state->stage=6;
 }
 if(state->stage==6){
  if(release_resource(state))return GH_POOL_UNSUPPORTED;
  state->stage=7;
 }
 return fail(&state->frontier,0x1496c0);
}

static gh_room_pool_status retire_mode3(gh_om300_full_retirement *state,
 const gh_om300_retire_bindings *bindings)
{
 gh_om_factory *factory=state->factory;
 gh_room_pools *pools=factory->bindings.pools;
 gh_om_object *object=state->object;
 const uint32_t handle=state->handle;
 state->frontier=0;
 if(state->stage==1){
  gh_room_pool_status status=retire_prefix(&state->prefix,bindings,handle);
  if(status!=GH_POOL_UNSUPPORTED||state->prefix.stage!=7||state->prefix.frontier!=0x1496c0)
   return fail(&state->frontier,state->prefix.frontier?state->prefix.frontier:gh_om_retire_entry(om_kind(object)));
  state->stage=2;
 }
 if(state->stage==2){
  gh_om_init_state *init=object->initialization;
  if(!init->bindings->shapes||!init->bindings->dynamic||
     gh_collision_pair_shapes_retire_model_native(init->bindings->shapes->pairs,&init->model)||
     gh_hitarea_forget_actor_native(init->bindings->init,&object->view))
   return fail(&state->frontier,0x1496c0);
  gh_scene_dynamic_forget_sphere_actor(init->bindings->dynamic,&object->view);
  state->stage=3;
 }
 if(state->stage==3){
  uint32_t frontier;
  if(gh_actor_model_retire_001496c0(&object->initialization->model,&state->model,&frontier))
   return fail(&state->frontier,frontier?frontier:0x1496c0);
  if(gh_room_arena_free_002a9680(pools,object->allocation))return fail(&state->frontier,0x30f5a0);
  state->stage=4;
 }
 return GH_POOL_OK;
}

gh_room_pool_status gh_om_retire_0030f8b0(gh_om300_full_retirement *state,
 const gh_om300_retire_bindings *bindings,uint32_t handle)
{
 if(!state||!bindings||!bindings->factory||!bindings->tables||!bindings->scroll||
    !bindings->solids||!handle)return GH_POOL_ARGUMENT;
 if(state->stage&&(state->factory!=bindings->factory||state->handle!=handle))
  return GH_POOL_ARGUMENT;
 if(state->stage==5)return GH_POOL_OK;
 gh_om_factory *factory=bindings->factory;
 gh_room_pools *pools=factory->bindings.pools;
 if(!state->stage){
  gh_om_object *object=registered_om(factory,handle);
  const gh_room_vector *primary,*dynamic;
  if(!object||word(object->view.bytes+0x214)!=class_vtable(object)||
     (word(object->view.bytes+0x250)&0xa0000000u)!=0xa0000000u||
     gh_room_registry_view(pools,0x754c30,&primary)||!primary->count||
     gh_room_registry_view(pools,0x754c58,&dynamic))
   return GH_POOL_ARGUMENT;
  const uint8_t *primary_entries=primary->allocation.pointer;
  uint32_t primary_index=0;
  while(primary_index<primary->count&&word(primary_entries+4*(size_t)primary_index)!=handle)
   ++primary_index;
  if(primary_index==primary->count)return GH_POOL_ARGUMENT;
  gh_actor_model *model=&object->initialization->model;
  if(model->external_owner||model->external_parts||model->external_borrowers||model->sound_borrowers)
   return fail(&state->frontier,0x1496c0);
  uint8_t *entries=dynamic->allocation.pointer;
  uint32_t i=0;
  while(i<dynamic->count&&word(entries+4*(size_t)i)!=handle)++i;
  if(i==dynamic->count)return fail(&state->frontier,0x30f9a4);
  state->factory=factory;state->object=object;state->handle=handle;
  --pools->vectors[3].count;
  memmove(entries+4*(size_t)i,entries+4*(size_t)(i+1),4*(size_t)(dynamic->count-i));
  state->stage=1;
 }
 state->frontier=0;
 gh_om_object *object=state->object;
 gh_room_pool_status status=retire_mode3(state,bindings);
 if(status)return status;
 if(gh_room_object_unregister(pools,handle,factory))return fail(&state->frontier,0x30fa08);
 free(object->initialization);
 memset(object,0,sizeof(*object));
 state->stage=5;
 return GH_POOL_OK;
}

gh_room_pool_status gh_om324_destroy_detached_00198ce0(gh_om300_full_retirement *state,
 const gh_om300_retire_bindings *bindings,uint32_t handle,const gh_title_player_view *actor)
{
 if(!state||!bindings||!bindings->factory||!actor||!handle||
    (state->stage&&(state->factory!=bindings->factory||state->handle!=handle)))
  return GH_POOL_ARGUMENT;
 if(state->stage==5)return GH_POOL_OK;
 gh_om_factory *factory=bindings->factory;
 if(!state->stage){
  gh_om_object *object=registered_om(factory,handle);
  if(!object||om_kind(object)!=0x324||object->view.bytes!=actor->bytes||
     object->view.size!=actor->size||word(actor->bytes+0x214)!=0x425000||
     (word(actor->bytes+0x250)&0xa0000000u)!=0xa0000000u)
   return GH_POOL_ARGUMENT;
  gh_actor_model *model=&object->initialization->model;
  if(model->external_owner||model->external_parts||model->external_borrowers||model->sound_borrowers)
   return fail(&state->frontier,0x1496c0);
  const gh_room_vector *primary,*secondary;
  if(gh_room_registry_view(factory->bindings.pools,0x754c30,&primary)||
     gh_room_registry_view(factory->bindings.pools,0x754c58,&secondary))
   return GH_POOL_ARGUMENT;
  unsigned found=0;
  for(unsigned i=0;i<primary->count;i++)
   found+=word((uint8_t *)primary->allocation.pointer+4*i)==handle;
  if(found!=1)return GH_POOL_ARGUMENT;
  for(unsigned i=0;i<secondary->count;i++)
   if(word((uint8_t *)secondary->allocation.pointer+4*i)==handle)
    return fail(&state->frontier,0x30f784);
  state->object=object;state->factory=factory;state->handle=handle;state->stage=1;
 }
 if(state->object->view.bytes!=actor->bytes||state->object->view.size!=actor->size)
  return GH_POOL_ARGUMENT;
 gh_room_pool_status status=retire_mode3(state,bindings);
 if(status)return status;
 /* Original Trans retains the registered handle until the destructor returns. */
 free(state->object->initialization);
 memset(state->object,0,sizeof(*state->object));
 state->stage=5;
 return GH_POOL_OK;
}

gh_room_pool_status gh_om300_mark_00312708(gh_om_factory *factory,uint32_t handle,
 gh_se_voice_engine *voices,uint32_t *frontier)
{
 gh_om_object *actor=registered_om(factory,handle);
 if(!actor||om_kind(actor)!=0x300)return GH_POOL_ARGUMENT;
 return gh_om_retire_mark_00312708(factory,handle,voices,frontier);
}

gh_room_pool_status gh_om300_retire_prefix_001867c0(gh_om300_retirement *state,
 const gh_om300_retire_bindings *bindings,uint32_t handle)
{
 gh_om_object *actor=bindings?registered_om(bindings->factory,handle):NULL;
 if(!actor||om_kind(actor)!=0x300)return GH_POOL_ARGUMENT;
 return retire_prefix(state,bindings,handle);
}

gh_room_pool_status gh_om300_retire_0030f8b0(gh_om300_full_retirement *state,
 const gh_om300_retire_bindings *bindings,uint32_t handle)
{
 gh_om_object *actor=bindings?registered_om(bindings->factory,handle):NULL;
 if((!state||!state->stage)&&(!actor||om_kind(actor)!=0x300))return GH_POOL_ARGUMENT;
 return gh_om_retire_0030f8b0(state,bindings,handle);
}
