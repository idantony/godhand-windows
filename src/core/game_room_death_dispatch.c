#include "game_room_death_dispatch.h"
#include <string.h>

static uint32_t word(const void *bytes)
{
 uint32_t result;
 memcpy(&result,bytes,4);
 return result;
}

static gh_room_pool_status fail(gh_room_death_dispatch *state,uint32_t handle,uint32_t entry)
{
 state->blocked_handle=handle;
 state->frontier=entry;
 return GH_POOL_UNSUPPORTED;
}

static int enemy_kind(uint32_t kind)
{
 return kind==0x202||kind==0x2a2;
}

static gh_room_pool_status retire(gh_room_death_dispatch *state,const gh_room_death_bindings *b)
{
 uint32_t handle=state->retiring_handle,entry=0;
 gh_room_pool_status status;
 if(enemy_kind(state->retiring_kind)||state->retiring_kind==0x363){
  gh_actor_motion_status enemy=state->retiring_kind==0x363?
   gh_enemy_ems_retire_weapon_0030f8b0(b->enemies,&state->enemy,handle,&b->enemy,&entry):
   gh_enemy_ems_retire_0030f8b0(b->enemies,&state->enemy,handle,&b->enemy,&entry);
  if(enemy)return fail(state,handle,entry?entry:0x290770);
  state->enemy=(gh_enemy_full_retirement){0};
  state->retiring_kind=state->retiring_handle=0;
  return GH_POOL_OK;
 }
 switch(state->retiring_kind){
 case 0x100:
  status=gh_room_death_retire_player_0030f8b0(&state->player,&b->player,b->hitarea,b->pairs);
  entry=state->player.frontier?state->player.frontier:0x107ec8;
  break;
 case 0x101:
  status=gh_room_death_retire_child_0030f8b0(&state->child,b->child,b->child_tail,b->hitarea,b->pairs);
  entry=state->child.frontier?state->child.frontier:0x100508;
  break;
 case 0x300:case 0x303:case 0x304:case 0x305:case 0x307:case 0x30a:case 0x30e:case 0x310:
 case 0x320:case 0x322:case 0x323:case 0x324:
 case 0x33f:case 0x346:case 0x354:case 0x35d:case 0x309:case 0x3eb:
  status=gh_om_retire_0030f8b0(&state->door,&b->door,handle);
  entry=state->door.frontier?state->door.frontier:gh_om_retire_entry(state->retiring_kind);
  if(!status)state->door=(gh_om300_full_retirement){0};
  break;
 default:return fail(state,handle,0x30fa00);
 }
 if(status)return fail(state,handle,entry);
 state->retiring_kind=state->retiring_handle=0;
 return GH_POOL_OK;
}

gh_room_pool_status gh_room_death_dispatch_0030f8b0(gh_room_death_dispatch *state,
 const gh_room_death_bindings *b)
{
 if(!state||!b||!b->scroll_meshes||!b->scroll_meshes->pools||
    !b->hitarea||!b->pairs||!b->dynamic||!b->effects||
    (state->bound&&state->bound!=b))return GH_POOL_ARGUMENT;
 gh_room_pools *pools=b->scroll_meshes->pools;
 state->bound=b;
 state->frontier=state->blocked_handle=0;
 if(state->retiring_kind){
  gh_room_pool_status status=retire(state,b);
  if(status)return status;
 }
 for(;;){
  gh_room_pool_status status=gh_room_death_scroll_0030f8b0(&state->scan,b->scroll_meshes);
  if(!status)return GH_POOL_OK;
  if(status!=GH_POOL_UNSUPPORTED||state->scan.frontier!=0x312708)
   return fail(state,state->scan.blocked_handle,state->scan.frontier?state->scan.frontier:0x30f8b0);
  const uint32_t handle=state->scan.blocked_handle;
  uint32_t kind=0,frontier=0;
  const gh_title_player_view *actor=NULL;
  gh_title_player_view om_view;
  if(b->player.factory&&handle==b->player.factory->handle){
   if(gh_game_player_factory_scalar_view(b->player.factory,handle,&actor))
    return fail(state,handle,0x312708);
   kind=0x100;
  }else if(b->child&&handle==b->child->handle){
   void *object;
   if(gh_room_object_lookup(pools,handle,GH_ROOM_OBJECT_PLAYER_CHILD,&b->child->object,&object)||
      object!=&b->child->object)return fail(state,handle,0x312708);
   actor=&b->child->object.view;
   kind=0x101;
  }else if(b->door.factory&&!gh_om_factory_object_view(b->door.factory,handle,&om_view)){
   actor=&om_view;
   if(!actor->bytes||actor->size<0x300)return fail(state,handle,0x312708);
   kind=(uint32_t)actor->bytes[0x2fe]|(uint32_t)actor->bytes[0x2ff]<<8;
  }
  if(!actor&&b->enemies){
   gh_enemy_ems_live live;
   if(gh_enemy_ems_live_at(b->enemies,handle,&live))return fail(state,handle,0x312708);
   if(live.actor){
    actor=&live.actor->view;
    kind=live.held_weapon?0x363:live.enemy202?0x202:live.enemy2a2?0x2a2:0;
   }
  }
  if(!actor||!actor->bytes||actor->size<0x254)return fail(state,handle,0x312708);
  uint32_t flags=word(actor->bytes+0x250);
  if(kind!=0x100&&kind!=0x101&&kind!=0x363&&!enemy_kind(kind)&&!gh_om_retire_entry(kind))
   return fail(state,handle,0x312708);
  if(!(flags&0x80000000u)){
   if(kind==0x363)return fail(state,handle,0x312708);
   if(enemy_kind(kind)){
    gh_actor_motion_status enemy=gh_enemy_death_mark_00312708(&state->enemy_mark,
     b->enemies,handle,b->voices,&frontier);
    if(enemy)return fail(state,handle,frontier?frontier:0x312708);
    state->enemy_mark=(gh_enemy_death_mark){0};
   }else{
   switch(kind){
   case 0x100:status=gh_room_death_mark_player_00312708(b->player.factory,b->player.model,b->voices,&frontier);break;
   case 0x101:status=gh_room_death_mark_child_00312708(b->child,b->voices,&frontier);break;
   default:status=gh_om_retire_mark_00312708(b->door.factory,handle,b->voices,&frontier);break;
   }
   if(status)return fail(state,handle,frontier?frontier:0x312708);
   }
   flags=word(actor->bytes+0x250);
  }
  if(!(flags&0x20000000u)){
   flags|=flags&0x40000000u?0x20000000u:0x40000000u;
   memcpy(actor->bytes+0x250,&flags,4);
   ++state->scan.cursor;
   continue;
  }
  int borrowed;
  if(gh_effect_create_actor_has_live_refs(b->effects,actor,&borrowed)||borrowed)
   return fail(state,handle,0x305b90);
  if(!enemy_kind(kind)&&!gh_om_retire_entry(kind))gh_scene_dynamic_forget_sphere_actor(b->dynamic,actor);
  state->retiring_kind=kind;
  state->retiring_handle=handle;
  status=retire(state,b);
  if(status)return status;
  /* Original 30FA08 compacts the primary vector. Re-read the same index. */
 }
}
