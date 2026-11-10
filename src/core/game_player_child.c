#include "game_player_child.h"
#include <string.h>
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static void wh(uint8_t*p,uint16_t v){memcpy(p,&v,2);}
static void vf0(uint8_t*p){memset(p,0,12);ww(p+12,0x3f800000);}
static gh_title_player_status fail(gh_game_player_child*s,uint32_t pc,gh_title_player_status e){s->frontier=pc;return e;}
gh_title_player_status gh_game_player_child_construct_00312f70(gh_game_title_player*s,gh_room_pools*pools,const gh_boot_work*work,int*original){
 if(!s||!pools||!work||!original)return GH_PLAYER_ARGUMENT;*original=0;
 if(s->allocation.pointer||s->arena_allocation.pointer)return GH_PLAYER_BUSY;
 uint32_t flags=w((const uint8_t*)work+0x594);if(flags&0x10000)return GH_PLAYER_OK;
 gh_room_arena_allocation a;gh_room_pool_status e=gh_room_arena_allocate_002a9538(pools,0x754c10,flags,GH_PLAYER_CHILD_BYTES,16,&a);
 if(e){s->frontier=0x2a9538;return GH_PLAYER_MEMORY;}if(!a.pointer)return GH_PLAYER_OK;
 s->arena_allocation=a;s->allocation=(gh_r207_allocation){a.pointer,&pools->arenas[2]};s->allocation_domain=GH_PLAYER_ALLOCATION_ROOM_ARENA;s->heap=pools->heap;
 uint8_t*p=a.pointer;memset(p,0,GH_PLAYER_CHILD_BYTES);gh_title_player_status r=gh_game_actor_construct_0014e990(p,GH_PLAYER_CHILD_BYTES,&s->view);if(r)return r;
 ww(p+0x214,0x41b6a0);s->view.original_vtable214=0x41b6a0;
 const unsigned offsets[]={0,0x10,0x30,0x40,0x50,0x80};for(unsigned i=0;i<44;i++)for(unsigned j=0;j<6;j++)vf0(p+0x4d0+i*0xa0+offsets[j]);vf0(p+0x20b0);
 s->view.initial_memory=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;s->ready=1;s->frontier=0;*original=1;return GH_PLAYER_OK;
}
static int same(const gh_player_child_bindings*a,const gh_player_child_bindings*b){return a->pools==b->pools&&a->global_objects==b->global_objects&&a->room_objects==b->room_objects&&a->work==b->work&&a->factory_serial==b->factory_serial&&a->resources.dvd==b->resources.dvd&&a->resources.heap==b->resources.heap&&a->resources.threads==b->resources.threads&&a->resources.sound_check==b->resources.sound_check&&a->resources.sound_context==b->resources.sound_context;}
gh_title_player_status gh_game_player_child_create_0030fa98(gh_game_player_child*s,const gh_player_child_bindings*b,int*original){
 if(!s||!b||!original||!b->pools||!b->global_objects||!b->room_objects||!b->work||!b->factory_serial)return GH_PLAYER_ARGUMENT;*original=0;
 if(s->stage&&!same(&s->b,b))return GH_PLAYER_ARGUMENT;if(s->complete){*original=1;return GH_PLAYER_OK;}
 if(!s->stage){const gh_room_vector*all,*objects;if(gh_room_registry_view(b->pools,0x754c30,&all)||gh_room_registry_view(b->pools,0x754c58,&objects))return fail(s,0x30fb70,GH_PLAYER_ARGUMENT);
  if(all->count==all->capacity||objects->count==objects->capacity)return GH_PLAYER_OK;int present=0;gh_title_player_status e=gh_game_player_child_construct_00312f70(&s->object,b->pools,b->work,&present);if(e)return fail(s,s->object.frontier,e);if(!present)return GH_PLAYER_OK;s->b=*b;s->stage=1;}
 uint8_t*p=s->object.view.bytes;
 if(s->stage==1){p[0x2fc]=*b->factory_serial;*b->factory_serial=(uint8_t)(*b->factory_serial+1);wh(p+0x2fe,0x101);ww(p+0x2d0,0x80000000);wh(p+0x300,0x101);s->stage=2;}
 if(s->stage==2){gh_object_view v;gh_object_status e=gh_game_objects_acquire_001fea48(b->room_objects,0x101,&v);if(e!=GH_OBJECT_OK&&e!=GH_OBJECT_MISSING)return fail(s,0x1fea48,GH_PLAYER_UNSUPPORTED);
  if(e==GH_OBJECT_OK){s->room_reference=1;if(v.bytes){s->resource=v;s->resource_from_room=1;}}s->stage=s->resource_from_room?4:3;}
 if(s->stage==3){uint32_t f=0;gh_object_status e=gh_game_objects_load_wait_001fe8a0(&s->load,b->global_objects,0x101,&b->resources,1,&s->resource,&f);if(e)return fail(s,f?f:0x1fe8a0,GH_PLAYER_UNSUPPORTED);s->global_reference=1;s->stage=4;}
 if(s->stage==4){if(!s->resource.file_extent_known){gh_object_view v;if(gh_game_objects_find(s->resource_from_room?b->room_objects:b->global_objects,0x101,&v)==GH_OBJECT_OK&&v.bytes==s->resource.bytes)s->resource=v;}if(!s->resource.bytes||!s->resource.file_extent_known||!s->resource.file_extent||s->resource.file_extent>s->resource.size)return fail(s,0x30fe68,GH_PLAYER_BOUNDS);
  s->file_bytes=s->resource.file_extent;ww(p+0x304,0);ww(p+0x2c8,0);if(!s->resource_from_room)ww(p+0x250,w(p+0x250)|0x2000000);s->stage=5;}
 if(s->stage==5){if(gh_room_object_register(b->pools,&s->object,&s->object,GH_ROOM_OBJECT_PLAYER_CHILD,9,&s->handle))return fail(s,0x30fcc0,GH_PLAYER_UNSUPPORTED);s->stage=6;}
 s->complete=1;s->frontier=0;*original=1;return GH_PLAYER_OK;
}
gh_title_player_status gh_game_player_child_initialize_00100378(gh_game_player_child*s,int*original){
 if(!s||!original||!s->complete)return GH_PLAYER_ARGUMENT;*original=0;
 if(!s->init_stage){gh_bundle b;if(gh_bundle_open(s->resource.bytes,s->file_bytes,&b)||gh_bundle_at(&b,0,&s->geometry)||gh_bundle_at(&b,1,&s->texture))return fail(s,0x100378,GH_PLAYER_BOUNDS);s->init_stage=1;}
 if(s->init_stage==1){gh_actor_model_status e=gh_actor_model_initialize_00149738(&s->model,&s->object.view,s->b.work,s->b.pools,&s->geometry,&s->texture,0,0,GH_STATIC_BINARY32_REFERENCE,&s->model_original_return);
  if(e)return fail(s,s->model.frontier?s->model.frontier:0x149738,e==GH_ACTOR_MODEL_UNSUPPORTED?GH_PLAYER_UNSUPPORTED:GH_PLAYER_BOUNDS);
  s->init_stage=2;}
 uint8_t*p=s->object.view.bytes;
 if(s->model_original_return){ww(p+0x2f0,w(p+0x2f0)|0x80);ww(p+0x250,w(p+0x250)|0x10000);ww(p+0x20d0,0);
  /* Original nextwrites use mutable global3BD880 then hierarchy149EB8.
   * Their canonical producer/binding is not yet recovered. */
  return fail(s,0x100420,GH_PLAYER_UNSUPPORTED);
 }
 ww(p+0x254,w(p+0x254)|0x8000000);ww(p+0x2f0,w(p+0x2f0)|1);s->init_stage=3;s->frontier=0;return GH_PLAYER_OK;
}
gh_title_player_status gh_game_player_child_release_prefix(gh_game_player_child*s){
 if(!s)return GH_PLAYER_ARGUMENT;if(gh_actor_model_release(&s->model))return GH_PLAYER_UNSUPPORTED;
 if(s->handle&&gh_room_object_unregister(s->b.pools,s->handle,&s->object))return GH_PLAYER_UNSUPPORTED;s->handle=0;
 if(s->room_reference){if(gh_game_objects_release_reference_001ff0b0(s->b.room_objects,0x101))return GH_PLAYER_UNSUPPORTED;s->room_reference=0;}
 if(s->global_reference){if(gh_game_objects_release_reference_001ff0b0(s->b.global_objects,0x101))return GH_PLAYER_UNSUPPORTED;s->global_reference=0;}
 gh_title_player_status e=gh_game_title_player_release(&s->object);if(e)return e;memset(s,0,sizeof(*s));return GH_PLAYER_OK;
}


