/* Actual313A60->186738->1BBEB8->1B6E90->shared2A7338.
 * Shared room arena, serial, source registries and lists are borrowed owners. */
#include "game_om_factory.h"
#include <stdlib.h>
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}
static void half(uint8_t*p,size_t o,uint16_t n){memcpy(p+o,&n,2);}
static void vf0(uint8_t*p,size_t o){memset(p+o,0,12);put(p,o+12,0x3f800000);}
static gh_scene_placement_status fail(gh_om_factory*f,uint32_t pc,uint32_t detail,gh_scene_placement_status e){f->frontier=pc;f->detail=detail;return e;}
static uint32_t creator(uint32_t type){switch(type){case 0x300:return 0x313a60;case 0x303:return 0x313bb0;case 0x304:return 0x313c20;case 0x305:return 0x313c90;case 0x307:return 0x313d70;case 0x309:return 0x313e50;case 0x30a:return 0x313ec0;case 0x30e:return 0x314080;case 0x310:return 0x314160;case 0x320:return 0x3147f0;case 0x322:return 0x314860;case 0x323:return 0x3148d0;case 0x324:return 0x314940;case 0x33f:return 0x315510;default:return 0x30fe68;}}
static gh_scene_placement_status construct(uint8_t*p,size_t n,gh_title_player_view*out){
 if(gh_game_object_construct_002a7338(p,n,out)!=GH_PLAYER_OK)return GH_PLACE_BOUNDS;
 /* 1B6E90 */put(p,0x214,0x428a20);put(p,0x5b0,0);put(p,0x5b4,0);put(p,0x5b8,0);
 vf0(p,0x5c0);vf0(p,0x5d0);put(p,0x560,0xffff);put(p,0x4ac,0x4289f0);
 const unsigned zero[]={0x5e0,0x5e4,0x5e8,0x5ec,0x5f0,0x490,0x494,0x498,0x5d0,0x5d4,0x5d8};
 for(unsigned i=0;i<sizeof(zero)/sizeof(*zero);i++)put(p,zero[i],0);half(p,0x5f4,0);half(p,0x5f6,0);put(p,0x5b8,0x10);
 /* 1BBEB8 */put(p,0x214,0x4292f0);vf0(p,0x610);put(p,0x620,0);put(p,0x634,0);half(p,0x630,0);put(p,0x644,0);
 /* 186738 seven0x60 records; only first5VF0 andbyte54 written. */
 put(p,0x214,0x422f90);for(unsigned i=0;i<7;i++){size_t o=0x650+(size_t)i*0x60;for(unsigned j=0;j<5;j++)vf0(p,o+j*16);p[o+0x54]=0;}
 out->original_vtable214=0x422f90;out->original_kind4ac=0x4289f0;out->initial_memory=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_factory_bind(gh_om_factory*f,const gh_om_factory_bindings*b,size_t capacity){
 if(!f||!b||!b->pools||!b->pools->initialized||!b->work||!b->room_state||!b->room_state->active||!b->objects||!b->room_objects||!capacity||capacity>65535)return GH_PLACE_ARGUMENT;
 if(f->bound||f->objects)return GH_PLACE_BUSY;gh_om_object*o=calloc(capacity,sizeof(*o));if(!o)return GH_PLACE_MEMORY;
 f->objects=o;f->capacity=capacity;f->bindings=*b;f->bound=1;return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_factory_create_record(gh_om_factory*f,const gh_om_placement_record*r,int*out){
 if(!f||!f->bound||!r||!r->bytes||r->size<64||!out)return GH_PLACE_ARGUMENT;*out=0;f->frontier=0;f->detail=0;
 /* This slice retains the current partialobject atcount; virtualInit hasnot
  * returned, so the originalouterloop mustnot advance pastit. */
 if(f->count>=f->capacity)return GH_PLACE_MEMORY;gh_om_object*o=f->objects+f->count;
 if(o->stage&&o->source.bytes!=r->bytes)return GH_PLACE_BUSY;
 if(!o->stage){
  const gh_room_vector *all,*om;if(gh_room_registry_view(f->bindings.pools,0x754c30,&all)||gh_room_registry_view(f->bindings.pools,0x754c58,&om))return fail(f,0x30fb70,0,GH_PLACE_ARGUMENT);
  if(all->count>=all->capacity||om->count>=om->capacity)return GH_PLACE_OK;
  if(r->type!=0x300)return fail(f,creator(r->type),r->type,GH_PLACE_UNSUPPORTED);
  uint32_t flags=word((const uint8_t*)f->bindings.work+0x594);if(flags&0x10000)return GH_PLACE_OK;
  gh_room_pool_status pe=gh_room_arena_allocate_002a9538(f->bindings.pools,0x754c10,flags,0x8f0,16,&o->allocation);
  if(pe)return fail(f,0x2a9538,pe,GH_PLACE_MEMORY);if(!o->allocation.pointer)return GH_PLACE_OK;
  memset(o->allocation.pointer,0,0x8f0);gh_scene_placement_status e=construct(o->allocation.pointer,0x8f0,&o->view);if(e)return e;o->source=*r;o->stage=1;
 }
 uint8_t*p=o->view.bytes;
 if(o->stage==1){if(!f->bindings.factory_serial)return fail(f,0x30fe68,0,GH_PLACE_UNSUPPORTED);
  p[0x2fc]=*f->bindings.factory_serial;*f->bindings.factory_serial=(uint8_t)(*f->bindings.factory_serial+1u);
  half(p,0x2fe,(uint16_t)r->type);put(p,0x2d0,0x20000000);half(p,0x300,(uint16_t)r->type);o->stage=2;
 }
 if(o->stage==2){
  gh_object_view existing;gh_object_status e=gh_game_objects_find(f->bindings.room_objects,r->type,&existing);
  if(e==GH_OBJECT_MISSING){
   if(!f->bindings.resources)return fail(f,0x1fe8a0,r->type,GH_PLACE_UNSUPPORTED);
   uint32_t frontier=0;e=gh_game_objects_load_wait_001fe8a0(&o->resource_load,f->bindings.objects,r->type,f->bindings.resources,1,&o->resource,&frontier);
   if(e)return fail(f,frontier?frontier:0x1fe8a0,e,GH_PLACE_UNSUPPORTED);
   if(!o->resource.bytes)return fail(f,0x1fe8a0,r->type,GH_PLACE_MEMORY);
   o->resource_manager=f->bindings.objects;
  }else{
   if(e!=GH_OBJECT_OK)return fail(f,0x1fea48,e,GH_PLACE_BOUNDS);
   e=gh_game_objects_acquire_001fea48(f->bindings.room_objects,r->type,&o->resource);if(e)return fail(f,0x1fea48,e,GH_PLACE_BOUNDS);
   o->resource_manager=f->bindings.room_objects;
  }
  o->acquired=1;/* raw304/2C8 guestpointers stayzero; typedresource retained. */o->stage=3;
 }
 if(o->stage==3){gh_room_pool_status e=gh_room_object_register(f->bindings.pools,o,f,GH_ROOM_OBJECT_OM,9,&o->handle);if(e)return fail(f,0x30fcc0,e,GH_PLACE_MEMORY);o->registered=1;o->stage=4;}
 /* Actualdoorvtable422F90+9C resolves1B7118 with thisadjust0. Required
  * OMSInitialize is not implied by constructor/resource/list readiness. */
 return fail(f,0x1b7118,r->index,GH_PLACE_UNSUPPORTED);
}
gh_scene_placement_status gh_om_factory_object_view(const gh_om_factory*f,uint32_t handle,gh_title_player_view*out){
 if(!f||!f->bound||!out||!handle)return GH_PLACE_ARGUMENT;void*object=NULL;
 if(gh_room_object_lookup(f->bindings.pools,handle,GH_ROOM_OBJECT_OM,f,&object)!=GH_POOL_OK)return GH_PLACE_BOUNDS;
 const gh_om_object*o=object;*out=o->view;return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_factory_release(gh_om_factory*f){
 if(!f)return GH_PLACE_ARGUMENT;
 for(size_t i=0;i<f->capacity;i++){gh_om_object*o=f->objects+i;
  if(o->registered){if(gh_room_object_unregister(f->bindings.pools,o->handle,f)!=GH_POOL_OK)return GH_PLACE_BUSY;o->registered=0;}
  if(o->acquired){if(gh_game_objects_release_reference_001ff0b0(o->resource_manager,o->source.type)!=GH_OBJECT_OK)return GH_PLACE_BUSY;o->acquired=0;}
  if(o->allocation.pointer){if(gh_room_arena_free_002a9680(f->bindings.pools,o->allocation)!=GH_POOL_OK)return GH_PLACE_MEMORY;o->allocation=(gh_room_arena_allocation){0};}
 }
 free(f->objects);*f=(gh_om_factory){0};return GH_PLACE_OK;
}
