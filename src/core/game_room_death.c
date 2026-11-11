#include "game_room_death.h"
#include "game_collision_pair_shapes.h"
#include <string.h>

static uint32_t read_u32(const uint8_t *p)
{
 return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}

static gh_room_pool_status block(gh_room_death_pass *pass,uint32_t handle,uint32_t entry)
{
 pass->blocked_handle=handle;
 pass->frontier=entry;
 return GH_POOL_UNSUPPORTED;
}

static uint32_t position(const gh_room_vector *vector,uint32_t handle)
{
 const uint8_t *bytes=vector->allocation.pointer;
 for(uint32_t i=0;i<vector->count;++i)
  if(read_u32(bytes+4*(size_t)i)==handle)return i;
 return vector->count;
}

static void erase(gh_room_vector *vector,uint32_t i)
{
 uint8_t *entry=(uint8_t *)vector->allocation.pointer+4*(size_t)i;
 --vector->count;
 memmove(entry,entry+4,4*(size_t)(vector->count-i));
}

static gh_room_pool_status retire(gh_room_death_pass *pass,gh_scene_meshes *meshes,
                                 gh_scene_mesh_object *mesh)
{
 gh_room_pools *pools=meshes->pools;
 uint32_t handle=mesh->handle;
 uint8_t *actor=mesh->allocation.pointer;
 if(!pass->retire_stage){
  const gh_room_vector *scroll,*other,*dynamic;
  if(gh_room_registry_view(pools,0x754c38,&scroll)||
     gh_room_registry_view(pools,0x754c50,&other)||
     gh_room_registry_view(pools,0x754c58,&dynamic))return GH_POOL_BOUNDS;
  uint32_t index=position(scroll,handle),optional=position(other,handle);
  if(index==scroll->count||position(dynamic,handle)!=dynamic->count||
     !actor||actor[0x2b1]!=1||!mesh->node_handle||!mesh->node)
   return block(pass,handle,0x31a018);
  erase(&pools->vectors[1],index);
  if(optional<other->count)erase(&pools->vectors[2],optional);
  pass->retiring_handle=handle;
  pass->retire_stage=1;
 }
 if(pass->retiring_handle!=handle)return GH_POOL_ARGUMENT;
 if(pass->retire_stage==1){
  /* 1496C0: object +238 native packet precedes the node list. */
  if(gh_heap_release(meshes->heap,mesh->object_packet_allocation))
   return block(pass,handle,0x2a9680);
  mesh->object_packet_allocation=(gh_r207_allocation){0};
  /* 14B798 decrements the model node count. 1494F8 increments the
   * allocator count before the 1483C8 node packet destructor. */
  actor[0x2b1]=0;
  ++pools->blocks[1].remaining1c;
  pass->retire_stage=2;
 }
 if(pass->retire_stage==2){
  if(gh_heap_release(meshes->heap,mesh->packet_allocation))
   return block(pass,handle,0x2a9680);
  mesh->packet_allocation=(gh_r207_allocation){0};
  pass->retire_stage=3;
 }
 if(pass->retire_stage==3){
  gh_room_block_pool *nodes=&pools->blocks[1];
  if(gh_room_block_put_002a9800(nodes,mesh->node_handle))return block(pass,handle,0x2a9800);
  mesh->node_handle=0;
  mesh->node=NULL;
  memset(actor+0x260,0,24);
  pass->retire_stage=4;
 }
 if(pass->retire_stage==4){
  /* 149E60 has no part list in type0, 150048(3) frees the actor. */
  if(gh_heap_release(meshes->heap,mesh->allocation))return block(pass,handle,0x2a9680);
  mesh->allocation=(gh_r207_allocation){0};
  pass->retire_stage=5;
 }
 if(gh_room_object_unregister(pools,handle,meshes))return block(pass,handle,0x30fa08);
 memset(mesh,0,sizeof(*mesh));
 pass->retire_stage=0;
 pass->retiring_handle=0;
 return GH_POOL_OK;
}

static gh_room_pool_status traverse(gh_room_death_pass *pass,gh_scene_meshes *meshes,int destroy)
{
 if(!pass||!meshes||!meshes->pools||(pass->owner&&pass->owner!=meshes))
  return GH_POOL_ARGUMENT;
 const gh_room_vector *primary;
 if(gh_room_registry_view(meshes->pools,0x754c30,&primary)||pass->cursor>primary->count)
  return GH_POOL_ARGUMENT;
 pass->owner=meshes;
 pass->frontier=pass->blocked_handle=0;
 const uint8_t *handles=primary->allocation.pointer;
 while(pass->cursor<primary->count){
  uint32_t handle=read_u32(handles+4*(size_t)pass->cursor);
  void *registered;
  if(gh_room_object_lookup(meshes->pools,handle,GH_ROOM_OBJECT_SCROLL_MESH,meshes,&registered))
   return block(pass,handle,0x312708);
  gh_scene_mesh_object *mesh=registered;
  if(pass->retire_stage){
   if(!destroy)return block(pass,handle,0x31a018);
   gh_room_pool_status status=retire(pass,meshes,mesh);
   if(status)return status;
   continue;
  }
  if(mesh->handle!=handle||!mesh->constructor_complete||!mesh->allocation.pointer)
   return block(pass,handle,0x312708);
  uint8_t *bytes=mesh->allocation.pointer;
  if(bytes[0x2fe]||bytes[0x2ff])return block(pass,handle,0x312708);
  uint32_t flags=read_u32(bytes+0x250);
  if(!(flags&0x80000000u)){
   /* 2CBE18 has no attachment to this static object in the native voice
    * namespace. Static type0 VT 450CE8+4C is the exact empty 14E928. */
   flags|=0x80000002u;
  }
  if(flags&0x20000000u){
   if(!destroy)return block(pass,handle,0x31a018);
   gh_room_pool_status status=retire(pass,meshes,mesh);
   if(status)return status;
   continue;
  }
  flags|=(flags&0x40000000u)?0x20000000u:0x40000000u;
  memcpy(bytes+0x250,&flags,sizeof(flags));
  ++pass->cursor;
 }
 pass->cursor=0;
 return GH_POOL_OK;
}


gh_room_pool_status gh_room_death_mark_scroll_0030f8b0(gh_room_death_pass *pass,gh_scene_meshes *meshes)
{
 return traverse(pass,meshes,0);
}

gh_room_pool_status gh_room_death_scroll_0030f8b0(gh_room_death_pass *pass,gh_scene_meshes *meshes)
{
 return traverse(pass,meshes,1);
}


gh_room_pool_status gh_room_death_mark_player_00312708(gh_game_player_factory *factory,
 gh_actor_model *model,gh_se_voice_engine *voices,uint32_t *frontier)
{
 if(frontier)*frontier=0;
 if(!factory)return GH_POOL_ARGUMENT;
 const gh_title_player_view *player;
 if(gh_game_player_factory_scalar_view(factory,factory->handle,&player)||!player||
    !player->bytes||player->size<0x254)return GH_POOL_ARGUMENT;
 uint32_t flags=read_u32(player->bytes+0x250);
 if(flags&0x80000000u)return GH_POOL_OK;
 if(player->original_vtable214!=0x41d220||read_u32(player->bytes+0x214)!=0x41d220){
  if(frontier)*frontier=0x312748;
  return GH_POOL_UNSUPPORTED;
 }
 if(!voices||!model||model->native_identity!=model||!model->ready||
    model->root.bytes!=player->bytes||model->pools!=factory->b.pools){
  if(frontier)*frontier=0x2cbe18;
  return GH_POOL_UNSUPPORTED;
 }
 if(gh_se_voice_detach_actor_002ce488(voices,model)){
  uint32_t detail=gh_se_voice_frontier(voices);
  if(frontier)*frontier=detail?detail:0x2ce488;
  return GH_POOL_UNSUPPORTED;
 }
 /* 41D220 +48/+4C resolve to (adjustment 0, 14E928); JR RA/NOP. */
 flags=read_u32(player->bytes+0x250)|0x80000002u;
 memcpy(player->bytes+0x250,&flags,sizeof(flags));
 return GH_POOL_OK;
}


static gh_room_pool_status player_block(gh_player_death_prefix *state,uint32_t entry)
{
 state->frontier=entry;
 return GH_POOL_UNSUPPORTED;
}

static void set_player_vtable(gh_title_player_view *player,uint32_t value)
{
 memcpy(player->bytes+0x214,&value,sizeof(value));
}

static gh_room_pool_status release_pinned_resource(uint32_t kind,gh_game_objects *room,
 gh_game_objects *global,const gh_object_view *expected,const gh_object_resource_bindings *resources,
 uint8_t *room_reference,uint8_t *global_reference,uint32_t *frontier)
{
 int ready=0;
 gh_game_objects *owner=room;
 uint8_t *reference=room_reference;
 gh_object_status found=gh_game_objects_check_ready_001ff238(owner,kind,resources,&ready,frontier);
 if(found!=GH_OBJECT_OK&&found!=GH_OBJECT_MISSING){
  if(!*frontier)*frontier=0x1fe9b8;
  return GH_POOL_UNSUPPORTED;
 }
 if(!ready){
  owner=global;
  reference=global_reference;
  found=gh_game_objects_check_ready_001ff238(owner,kind,resources,&ready,frontier);
  if(found!=GH_OBJECT_OK&&found!=GH_OBJECT_MISSING){
   if(!*frontier)*frontier=0x1fe9b8;
   return GH_POOL_UNSUPPORTED;
  }
 }
 if(!ready)return GH_POOL_OK;
 gh_object_view resource;
 if(gh_game_objects_find(owner,kind,&resource)||resource.bytes!=expected->bytes||
    resource.use_count>=0||!*reference){
  *frontier=0x1fed78;
  return GH_POOL_UNSUPPORTED;
 }
 if(gh_game_objects_release_event_001fed78(owner,kind,resource.bytes,NULL,NULL,frontier)){
  if(!*frontier)*frontier=0x1fed78;
  return GH_POOL_UNSUPPORTED;
 }
 *reference=0;
 return GH_POOL_OK;
}

gh_room_pool_status gh_room_death_player_prefix_00107ec8(gh_player_death_prefix *state,
 const gh_player_death_bindings *bindings)
{
 if(!state||!bindings||!bindings->factory||!bindings->model||!bindings->solids||
    !bindings->attachment||!bindings->resources)return GH_POOL_ARGUMENT;
 gh_game_player_factory *factory=bindings->factory;
 gh_actor_model *model=bindings->model;
 gh_player_child_attachment *attachment=bindings->attachment;
 const gh_title_player_view *view;
 if(gh_game_player_factory_scalar_view(factory,factory->handle,&view)||!view||
    view->original_vtable214!=0x41d220||view->size<0x300||
    (read_u32(view->bytes+0x250)&0xa0000000u)!=0xa0000000u||model->native_identity!=model||
    model->root.bytes!=view->bytes||model->sound_borrowers)return GH_POOL_ARGUMENT;
 if(state->stage){
  const gh_player_death_bindings *saved=&state->bound;
  if(state->handle!=factory->handle||saved->factory!=factory||saved->model!=model||
     saved->solids!=bindings->solids||saved->attachment!=attachment||
     saved->resources!=bindings->resources)return GH_POOL_ARGUMENT;
 }else {
  if(read_u32(view->bytes+0x214)!=0x41d220)return GH_POOL_ARGUMENT;
  if(attachment->attached&&(attachment->parent!=factory||attachment->parent_model!=model||
     !attachment->child||model->external_borrowers!=1))return player_block(state,0x100ed8);
  if(!attachment->attached&&model->external_borrowers)return player_block(state,0x100ed8);
  state->bound=*bindings;
  state->handle=factory->handle;
  state->stage=1;
 }
 gh_title_player_view *player=&factory->b.player->view;
 uint32_t frontier=0;
 if(state->stage==1){
  if(gh_collision_solid_release_unit_001347e8(bindings->solids,player,&frontier))
   return player_block(state,frontier?frontier:0x1347e8);
  state->stage=2;
 }
 if(state->stage==2){
  set_player_vtable(player,0x41b598);
  if(gh_collision_solid_release_unit_001347e8(bindings->solids,player,&frontier))
   return player_block(state,frontier?frontier:0x1347e8);
  set_player_vtable(player,0x44ae28); /* 2A73C8 */
  set_player_vtable(player,0x41ebe8); /* 14EA28 */
  set_player_vtable(player,0x41eaf0); /* 14E5D0 */
  state->stage=3;
 }
 if(state->stage==3){
  uint32_t kind=(uint32_t)player->bytes[0x2fe]|(uint32_t)player->bytes[0x2ff]<<8;
  if(kind!=0x100)return player_block(state,0x1fe9b8);
  if(release_pinned_resource(kind,factory->b.room_objects,factory->b.global_objects,
     &factory->resource,bindings->resources,&factory->room_reference,&factory->global_reference,&frontier))
   return player_block(state,frontier);
  state->stage=4;
 }
 if(state->stage==4){
  /* The original destructor has no child call. End only the native borrow
   * of the parent part before the parent model can be reclaimed. */
  if(attachment->attached)gh_game_player_child_attachment_clear(attachment);
  if(attachment->attached||model->external_borrowers)return player_block(state,0x100ed8);
  state->stage=5;
 }
 return player_block(state,0x1496c0);
}

static gh_room_pool_status player_retirement_error(gh_player_retirement *state,uint32_t entry)
{
 state->frontier=entry;
 return GH_POOL_UNSUPPORTED;
}

gh_room_pool_status gh_room_death_retire_player_0030f8b0(gh_player_retirement *state,
 const gh_player_death_bindings *bindings,gh_game_init_state *hitarea,gh_collision_pairs *pairs)
{
 if(!state||!bindings||!bindings->factory||!bindings->model||!bindings->solids||
    !bindings->attachment||!bindings->resources||!hitarea||!pairs)return GH_POOL_ARGUMENT;
 if(state->stage){
  const gh_player_death_bindings *saved=&state->prefix.bound;
  if(state->hitarea!=hitarea||state->pairs!=pairs||saved->factory!=bindings->factory||
     saved->model!=bindings->model||saved->solids!=bindings->solids||
     saved->attachment!=bindings->attachment||saved->resources!=bindings->resources)
   return GH_POOL_ARGUMENT;
 }
 if(state->stage==5)return GH_POOL_OK;
 gh_game_player_factory *factory=bindings->factory;
 gh_room_pools *pools=factory->b.pools;
 gh_game_title_player *player=factory->b.player;
 if(!pools||!player||!factory->handle||(state->stage&&state->handle!=factory->handle))return GH_POOL_ARGUMENT;
 state->frontier=0;
 if(!state->stage){
  const gh_room_vector *primary,*dynamic;
  const gh_title_player_view *view;
  if(gh_game_player_factory_scalar_view(factory,factory->handle,&view)||!view||
     !view->bytes||view->size<0x300||
     (read_u32(view->bytes+0x250)&0xa0000000u)!=0xa0000000u||
     gh_room_registry_view(pools,0x754c30,&primary)||!primary->count||
     position(primary,factory->handle)==primary->count||
     gh_room_registry_view(pools,0x754c58,&dynamic))return GH_POOL_ARGUMENT;
  uint32_t index=position(dynamic,factory->handle);
  if(index==dynamic->count)return player_retirement_error(state,0x30f9a4);
  state->hitarea=hitarea;state->pairs=pairs;state->handle=factory->handle;
  state->prefix.bound=*bindings;
  erase(&pools->vectors[3],index);
  state->stage=1;
 }
 if(state->stage==1){
  gh_room_pool_status status=gh_room_death_player_prefix_00107ec8(&state->prefix,bindings);
  if(status!=GH_POOL_UNSUPPORTED||state->prefix.stage!=5||state->prefix.frontier!=0x1496c0)
   return player_retirement_error(state,state->prefix.frontier?state->prefix.frontier:0x107ec8);
  state->stage=2;
 }
 if(state->stage==2){
  if(gh_collision_pair_shapes_retire_model_native(pairs,bindings->model)||
     gh_hitarea_forget_actor_native(hitarea,&player->view))
   return player_retirement_error(state,0x1496c0);
  state->stage=3;
 }
 if(state->stage==3){
  uint32_t frontier=0;
  if(gh_actor_model_retire_pl00_001496c0(bindings->model,&state->model,&frontier))
   return player_retirement_error(state,frontier?frontier:0x1496c0);
  if(gh_game_title_player_release(player))return player_retirement_error(state,0x30f5a0);
  state->stage=4;
 }
 if(gh_room_object_unregister(pools,state->handle,player))
  return player_retirement_error(state,0x30fa08);
 memset(factory,0,sizeof(*factory));
 state->stage=5;
 return GH_POOL_OK;
}

static int child_registered(const gh_game_player_child *child)
{
 void *object=NULL;
 return child&&child->complete&&child->object.view.bytes&&child->object.view.size>=0x300&&
  child->object.view.original_vtable214==0x41b6a0&&
  !gh_room_object_lookup(child->b.pools,child->handle,GH_ROOM_OBJECT_PLAYER_CHILD,&child->object,&object)&&
  object==&child->object;
}

gh_room_pool_status gh_room_death_mark_child_00312708(gh_game_player_child *child,
 gh_se_voice_engine *voices,uint32_t *frontier)
{
 if(frontier)*frontier=0;
 if(!child_registered(child))return GH_POOL_ARGUMENT;
 uint8_t *bytes=child->object.view.bytes;
 uint32_t flags=read_u32(bytes+0x250);
 if(flags&0x80000000u)return GH_POOL_OK;
 if(read_u32(bytes+0x214)!=0x41b6a0){
  if(frontier)*frontier=0x312748;
  return GH_POOL_UNSUPPORTED;
 }
 if(!voices||!child->model.ready||child->model.root.bytes!=bytes||
    child->model.native_identity!=&child->model){
  if(frontier)*frontier=0x2cbe18;
  return GH_POOL_UNSUPPORTED;
 }
 if(gh_se_voice_detach_actor_002ce488(voices,&child->model)){
  uint32_t detail=gh_se_voice_frontier(voices);
  if(frontier)*frontier=detail?detail:0x2ce488;
  return GH_POOL_UNSUPPORTED;
 }
 /* Child 41B6A0 +48/+4C is (0,14E928), an actual return. */
 flags=read_u32(bytes+0x250)|0x80000002u;
 memcpy(bytes+0x250,&flags,4);
 return GH_POOL_OK;
}

static gh_room_pool_status child_retirement_error(gh_child_retirement *state,uint32_t entry)
{
 state->frontier=entry;
 return GH_POOL_UNSUPPORTED;
}

gh_room_pool_status gh_room_death_retire_child_0030f8b0(gh_child_retirement *state,
 gh_game_player_child *child,gh_player_child_tail *tail,gh_game_init_state *hitarea,gh_collision_pairs *pairs)
{
 if(!state||!child||!tail||!hitarea||!pairs)return GH_POOL_ARGUMENT;
 if(state->stage&&(state->child!=child||state->tail!=tail||
    state->hitarea!=hitarea||state->pairs!=pairs))return GH_POOL_ARGUMENT;
 if(state->stage==5)return GH_POOL_OK;
 if(!state->stage){
  if(!child_registered(child)||tail->child!=child||!tail->complete||
     !tail->work5f8||*tail->work5f8!=child)return GH_POOL_ARGUMENT;
  if(child->model.external_owner||child->model.external_parts||
     child->model.external_borrowers||child->model.sound_borrowers)
   return child_retirement_error(state,0x1496c0);
  gh_title_player_view *actor=&child->object.view;
  gh_room_pools *pools=child->b.pools;
  const gh_room_vector *primary,*dynamic;
  if(read_u32(actor->bytes+0x214)!=0x41b6a0||
     (read_u32(actor->bytes+0x250)&0xa0000000u)!=0xa0000000u||
     gh_room_registry_view(pools,0x754c30,&primary)||!primary->count||
     position(primary,child->handle)==primary->count||
     gh_room_registry_view(pools,0x754c58,&dynamic))return GH_POOL_ARGUMENT;
  uint32_t index=position(dynamic,child->handle);
  if(index==dynamic->count)return child_retirement_error(state,0x30f9a4);
  state->child=child;state->tail=tail;state->hitarea=hitarea;state->pairs=pairs;
  state->pools=pools;state->handle=child->handle;
  erase(&pools->vectors[3],index);
  state->stage=1;
 }
 state->frontier=0;
 if(state->stage==1){
  gh_title_player_view *actor=&child->object.view;
  set_player_vtable(actor,0x41b6a0); /* 100508 */
  set_player_vtable(actor,0x41ebe8); /* 14EA28 */
  set_player_vtable(actor,0x41eaf0); /* 14E5D0 */
  if(actor->bytes[0x2fe]!=1||actor->bytes[0x2ff]!=1)return child_retirement_error(state,0x1fe9b8);
  uint32_t frontier=0;
  if(release_pinned_resource(0x101,child->b.room_objects,child->b.global_objects,
     &child->resource,&child->b.resources,&child->room_reference,&child->global_reference,&frontier))
   return child_retirement_error(state,frontier);
  state->stage=2;
 }
 if(state->stage==2){
  if(gh_collision_pair_shapes_retire_model_native(pairs,&child->model)||
     gh_hitarea_forget_actor_native(hitarea,&child->object.view))
   return child_retirement_error(state,0x1496c0);
  state->stage=3;
 }
 if(state->stage==3){
  uint32_t frontier=0;
  if(gh_actor_model_retire_001496c0(&child->model,&state->model,&frontier))
   return child_retirement_error(state,frontier?frontier:0x1496c0);
  if(gh_game_title_player_release(&child->object))return child_retirement_error(state,0x30f5a0);
  *tail->work5f8=NULL;
  memset(tail,0,sizeof(*tail));
  state->stage=4;
 }
 if(gh_room_object_unregister(state->pools,state->handle,&child->object))
  return child_retirement_error(state,0x30fa08);
 memset(child,0,sizeof(*child));
 state->stage=5;
 return GH_POOL_OK;
}
