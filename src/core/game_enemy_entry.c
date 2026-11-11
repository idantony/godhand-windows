#include "game_enemy_entry.h"
#include "game_enemy_row2.h"
#include "game_enemy_2a2.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
static void half(uint8_t*p,size_t o,uint16_t v){memcpy(p+o,&v,2);}
static gh_actor_model_status fail(gh_enemy_entry*s,uint32_t pc){s->frontier=pc;return GH_ACTOR_MODEL_UNSUPPORTED;}
static int same_request(const gh_enemy_request*a,const gh_enemy_request*b){return a->object_kind==b->object_kind&&!memcmp(a->position_xyz,b->position_xyz,12)&&a->yaw_bits==b->yaw_bits&&a->flags==b->flags&&a->dependency_kind==b->dependency_kind&&a->dependency_flags==b->dependency_flags&&a->appearance==b->appearance&&a->entry_number==b->entry_number;}
static gh_actor_model_status prepare_entry(gh_enemy_entry*s,const gh_enemy_entry_bindings*b,const gh_enemy_request*r,int32_t kind,uint32_t argument,int*ready,int heavy){
 if(!s||!b||!ready||!b->init||!b->pools||!b->room_objects||!b->global_objects||!b->work||!b->room_heap||!b->packet_heap||!b->fog_rgb||!b->factory_serial||!b->scenario14||!b->save.bytes)return GH_ACTOR_MODEL_ARGUMENT;
 *ready=0;if(!r||kind< -1)return GH_ACTOR_MODEL_OK;
 if(heavy){
  if(r->object_kind!=0x2a2||r->appearance||r->flags||r->dependency_flags||
     !((r->dependency_kind==0x2a2&&r->entry_number==6)||
       (r->dependency_kind==0x2a3&&r->entry_number==14)))return fail(s,0x2905d0);
 }else if(r->object_kind!=0x202||(r->dependency_kind!=0x20a&&!gh_enemy202_is_r100_supported_profile(r)))return fail(s,0x2905d0);
 if(s->stage&&(s->b.init!=b->init||s->b.pools!=b->pools||s->b.work!=b->work||s->b.room_objects!=b->room_objects||s->b.global_objects!=b->global_objects||s->manager_kind!=kind||s->argument!=argument||s->b.room_heap!=b->room_heap||s->b.packet_heap!=b->packet_heap||s->b.factory_serial!=b->factory_serial||!same_request(&s->request,r)))return GH_ACTOR_MODEL_BUSY;
 if(!s->stage){int slot;if(gh_enemy_manager_find_free_00290958(b->init,&slot))return GH_ACTOR_MODEL_ARGUMENT;if(slot<0)return GH_ACTOR_MODEL_OK;s->slot=slot;s->b=*b;s->request=*r;s->manager_kind=kind;s->argument=argument;s->stage=1;}
 if(s->stage==1){int ready=0;gh_object_status e=gh_game_objects_check_ready_001ff238(s->b.room_objects,r->object_kind,&s->b.resources,&ready,&s->frontier);if(e==GH_OBJECT_MISSING)ready=0;else if(e)return fail(s,s->frontier?s->frontier:0x1fe9b8);
  if(!ready){e=gh_game_objects_load_wait_001fe8a0(&s->load,s->b.room_objects,r->object_kind,&s->b.resources,1,&s->resource,&s->frontier);if(e)return fail(s,s->frontier?s->frontier:0x1fe8a0);s->preload_reference=1;}s->stage=2;
 }
 if(s->stage==2){gh_object_status e=gh_game_object_sounds_001feaa0(&s->sound,s->b.room_objects,r->object_kind,r->dependency_kind,r->dependency_flags,s->b.work,s->b.banks,s->b.resources.threads,&s->frontier);if(e)return fail(s,s->frontier?s->frontier:0x1feaa0);s->stage=3;}
 if(s->stage==3){const gh_room_vector*a,*v;if(gh_room_registry_view(s->b.pools,0x754c30,&a)||gh_room_registry_view(s->b.pools,0x754c58,&v))return GH_ACTOR_MODEL_ARGUMENT;if(a->count==a->capacity||v->count==v->capacity)return GH_ACTOR_MODEL_OK;
  int present;gh_title_player_status e=heavy?
      gh_enemy2a2_allocate_00313820(&s->object,s->b.room_heap,s->b.work,&present):
      gh_enemy202_allocate_003130e0(&s->object,s->b.room_heap,s->b.work,&present);
  if(e)return fail(s,heavy?0x313820:0x3130e0);if(!present)return GH_ACTOR_MODEL_OK;
  uint8_t*p=s->object.view.bytes;p[0x2fc]=*s->b.factory_serial;*s->b.factory_serial=(uint8_t)(*s->b.factory_serial+1);half(p,0x2fe,(uint16_t)r->object_kind);put(p,0x2d0,0x40000000);half(p,0x300,(uint16_t)r->object_kind);s->stage=4;
 }
 if(s->stage==4){gh_object_status e=gh_game_objects_acquire_001fea48(s->b.room_objects,r->object_kind,&s->resource);if(e==GH_OBJECT_OK){s->room_reference=1;if(s->resource.bytes)s->resource_from_room=1;}else if(e!=GH_OBJECT_MISSING)return fail(s,0x1fea48);s->stage=5;}
 if(s->stage==5){if(!s->resource_from_room){gh_object_status e=gh_game_objects_load_wait_001fe8a0(&s->global_load,s->b.global_objects,r->object_kind,&s->b.resources,1,&s->resource,&s->frontier);if(e)return fail(s,s->frontier?s->frontier:0x1fe8a0);s->global_reference=1;put(s->object.view.bytes,0x250,word(s->object.view.bytes,0x250)|0x2000000);}
  if(!s->resource.bytes||!s->resource.file_extent_known||s->resource.file_extent>s->resource.size)return fail(s,0x30fe68);put(s->object.view.bytes,0x304,0);put(s->object.view.bytes,0x2c8,0);s->stage=6;}
 if(s->stage==6){if(gh_room_object_register(s->b.pools,&s->object,s,GH_ROOM_OBJECT_ENEMY,9,&s->handle))return fail(s,0x30fcc0);s->stage=7;}
 if(s->stage==7){if(gh_enemy_manager_insert_00290048(s->b.init,(unsigned)s->slot,&s->object,s->manager_kind,&s->frontier))return fail(s,s->frontier?s->frontier:0x290048);s->published=1;half(s->object.view.bytes,0x640,r->entry_number);s->stage=8;}
 *ready=1;
 s->frontier=0;
 return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_enemy_entry_prepare_2a2_002905d0(gh_enemy_entry*s,
 const gh_enemy_entry_bindings*b,const gh_enemy_request*r,int32_t kind,uint32_t argument,int*ready)
{
 return prepare_entry(s,b,r,kind,argument,ready,1);
}

gh_actor_model_status gh_enemy_entry_002905d0(gh_enemy_entry*s,const gh_enemy_entry_bindings*b,
 const gh_enemy_request*r,int32_t kind,uint32_t argument,int*original)
{
 if(!original)return GH_ACTOR_MODEL_ARGUMENT;
 *original=0;
 int ready;
 gh_actor_model_status status=prepare_entry(s,b,r,kind,argument,&ready,0);
 if(status||!ready)return status;
 gh_actor_model_status e=gh_enemy202_initialize_prefix_002022d0(&s->initialize,&s->object.view,&s->request,&s->resource,s->b.work,s->b.save,s->b.scenario14,s->b.borrowed586aa4,s->b.pools,s->b.packet_heap,s->b.fog_rgb,s->b.policy);s->frontier=s->initialize.frontier;return e;
}
gh_actor_model_status gh_enemy_entry_abort_prefix(gh_enemy_entry*s){
 if(!s)return GH_ACTOR_MODEL_ARGUMENT;if(!s->stage)return GH_ACTOR_MODEL_OK;
 if(s->published){unsigned k=(unsigned)s->slot;gh_game_init_state*i=s->b.init;if(k>=64||i->enemy_actors[k]!=&s->object||i->enemies[k*20+16]!=1)return GH_ACTOR_MODEL_BUSY;
 uint8_t prev=i->enemy_prev[k],next=i->enemy_next[k];if((prev!=255&&(prev>=64||i->enemy_next[prev]!=k))||(next!=255&&(next>=64||i->enemy_prev[next]!=k)))return GH_ACTOR_MODEL_BUSY;}
 gh_actor_model_status e=gh_enemy202_initialize_abort_prefix(&s->initialize);if(e)return e;
 if(s->published){gh_game_init_state*i=s->b.init;unsigned k=(unsigned)s->slot;if(k>=64||i->enemy_actors[k]!=&s->object||!i->enemies[k*20+16])return GH_ACTOR_MODEL_BUSY;
  uint8_t prev=i->enemy_prev[k],next=i->enemy_next[k];if((prev!=255&&(prev>=64||i->enemy_next[prev]!=k))||(next!=255&&(next>=64||i->enemy_prev[next]!=k)))return GH_ACTOR_MODEL_BUSY;
  if(prev==255)i->enemy_top=next==255?NULL:i->enemies+next*20;else i->enemy_next[prev]=next;
  if(next==255)i->enemy_last=prev==255?NULL:i->enemies+prev*20;else i->enemy_prev[next]=prev;
  i->enemy_actors[k]=NULL;i->enemy_prev[k]=i->enemy_next[k]=255;put(i->enemies,k*20+12,UINT32_MAX);i->enemies[k*20+16]=0;
  put(i->enemies,0x508,word(i->enemies,0x508)-1);int remains=0;for(unsigned j=0;j<64;j++)if(i->enemies[j*20+16]==1&&word(i->enemies,j*20+12)==(uint32_t)s->manager_kind)remains=1;
  if(!remains)put(i->enemies,0x50c,word(i->enemies,0x50c)-1);s->published=0;
 }
 if(s->handle){if(gh_room_object_unregister(s->b.pools,s->handle,s))return GH_ACTOR_MODEL_BUSY;s->handle=0;}
 if(s->room_reference){if(gh_game_objects_release_reference_001ff0b0(s->b.room_objects,s->request.object_kind))return GH_ACTOR_MODEL_BUSY;s->room_reference=0;}
 if(s->global_reference){if(gh_game_objects_release_reference_001ff0b0(s->b.global_objects,s->request.object_kind))return GH_ACTOR_MODEL_BUSY;s->global_reference=0;}
 if(s->preload_reference){if(gh_game_objects_release_reference_001ff0b0(s->b.room_objects,s->request.object_kind))return GH_ACTOR_MODEL_BUSY;s->preload_reference=0;}
 if(gh_game_title_player_release(&s->object))return GH_ACTOR_MODEL_MEMORY;*s=(gh_enemy_entry){0};return GH_ACTOR_MODEL_OK;
}
