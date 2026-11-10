/* Actual313A60->186738->1BBEB8->1B6E90->shared2A7338.
 * Shared room arena, serial, source registries and lists are borrowed owners. */
#include "game_om_factory.h"
#include "game_om_initialize.h"
#include "game_om303.h"
#include "game_om304.h"
#include "game_om305.h"
#include "game_om307.h"
#include "game_om30a.h"
#include "game_om30e.h"
#include "game_om310.h"
#include "game_om320.h"
#include "game_om322.h"
#include "game_om323.h"
#include "game_om324.h"
#include "game_om33f.h"
#include "game_om346.h"
#include "game_om_base.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}
static void half(uint8_t*p,size_t o,uint16_t n){memcpy(p+o,&n,2);}
static void vf0(uint8_t*p,size_t o){memset(p+o,0,12);put(p,o+12,0x3f800000);}
static gh_scene_placement_status fail(gh_om_factory*f,uint32_t pc,uint32_t detail,gh_scene_placement_status e){f->frontier=pc;f->detail=detail;return e;}
static uint32_t creator(uint32_t type){switch(type){case 0x300:return 0x313a60;case 0x303:return 0x313bb0;case 0x304:return 0x313c20;case 0x305:return 0x313c90;case 0x307:return 0x313d70;case 0x309:return 0x313e50;case 0x30a:return 0x313ec0;case 0x30e:return 0x314080;case 0x310:return 0x314160;case 0x320:return 0x3147f0;case 0x322:return 0x314860;case 0x323:return 0x3148d0;case 0x324:return 0x314940;case 0x33f:return 0x315510;case 0x346:return 0x315820;case 0x354:return 0x315c10;default:return 0x30fe68;}}
static gh_scene_placement_status construct(uint8_t*p,size_t n,gh_title_player_view*out){
 if(gh_om_base_construct_001b6e90(p,n,out)!=GH_PLACE_OK)return GH_PLACE_BOUNDS;
 /* 1BBEB8 */put(p,0x214,0x4292f0);vf0(p,0x610);put(p,0x620,0);put(p,0x634,0);half(p,0x630,0);put(p,0x644,0);
 /* 186738 seven0x60 records; only first5VF0 andbyte54 written. */
 put(p,0x214,0x422f90);for(unsigned i=0;i<7;i++){size_t o=0x650+(size_t)i*0x60;for(unsigned j=0;j<5;j++)vf0(p,o+j*16);p[o+0x54]=0;}
 out->original_vtable214=0x422f90;out->original_kind4ac=0x4289f0;out->initial_memory=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;return GH_PLACE_OK;
}
/* Native bindings for the original creator/vtable targets. Unknown classes
 * have no allocation/empty virtual substitute. */
static size_t class_bytes(uint32_t type){switch(type){case 0x300:return 0x8f0;case 0x303:return 0x9e0;case 0x304:return 0x850;case 0x305:return 0x860;case 0x307:return 0xce0;case 0x30a:return 0x9c0;case 0x30e:return 0x610;case 0x310:return 0x920;case 0x320:return 0x800;case 0x322:return 0x9e0;case 0x323:return 0x9e0;case 0x324:return 0x610;case 0x33f:return 0xb40;case 0x346:return 0xda0;default:return 0;}}
static gh_scene_placement_status construct_class(uint32_t type,void*p,size_t bytes,gh_title_player_view*out){switch(type){case 0x300:return construct(p,bytes,out);case 0x303:return gh_om303_construct_001879c8(p,bytes,out);case 0x304:return gh_om304_construct_00187c88(p,bytes,out);case 0x305:return gh_om305_construct_00188a68(p,bytes,out);case 0x307:return gh_om307_construct_00188f80(p,bytes,out);case 0x30a:return gh_om30a_construct_0018c098(p,bytes,out);case 0x30e:return gh_om30e_construct_0018ec98(p,bytes,out);case 0x310:return gh_om310_construct_0018f110(p,bytes,out);case 0x320:return gh_om320_construct_00195c20(p,bytes,out);case 0x322:return gh_om322_construct_00196058(p,bytes,out);case 0x323:return gh_om323_construct_00197680(p,bytes,out);case 0x324:return gh_om324_construct_00198ca8(p,bytes,out);case 0x33f:return gh_om33f_construct_0019f0a8(p,bytes,out);case 0x346:return gh_om346_construct_001a0d68(p,bytes,out);default:return GH_PLACE_UNSUPPORTED;}}
static size_t state_bytes(uint32_t type){return type==0x346?sizeof(gh_om346_state):type==0x323?sizeof(gh_om323_state):type==0x322?sizeof(gh_om322_state):type==0x310?sizeof(gh_om310_state):type==0x300?sizeof(gh_om_init_state):type==0x307?sizeof(gh_om307_state):sizeof(gh_om303_state);}
static uint32_t state_frontier(const gh_om_object*o){return o->source.type==0x346?((gh_om346_state*)o->initialization)->frontier:o->source.type==0x323?((gh_om323_state*)o->initialization)->frontier:o->source.type==0x322?((gh_om322_state*)o->initialization)->frontier:o->source.type==0x300?o->initialization->frontier:o->source.type==0x307?((gh_om307_state*)o->initialization)->frontier:((gh_om303_state*)o->initialization)->frontier;}
static gh_scene_placement_status initialize_class(gh_om_object*o,const gh_om_initializer_bindings*b,int*out){switch(o->source.type){case 0x300:return gh_om_door_initialize_001b7118(o->initialization,o,b,out);case 0x303:return gh_om303_initialize_00187a78((gh_om303_state*)o->initialization,o,b,out);case 0x304:return gh_om304_initialize_00187d88((gh_om304_state*)o->initialization,o,b,out);case 0x305:return gh_om305_initialize_00188b50((gh_om305_state*)o->initialization,o,b,out);case 0x307:return gh_om307_initialize_001890b0((gh_om307_state*)o->initialization,o,b,out);case 0x30a:return gh_om30a_initialize_0018c180((gh_om30a_state*)o->initialization,o,b,out);case 0x30e:return gh_om30e_initialize_0018ed30((gh_om30e_state*)o->initialization,o,b,out);case 0x310:return gh_om310_initialize_0018f218((gh_om310_state*)o->initialization,o,b,out);case 0x320:return gh_om320_initialize_00195d08((gh_om320_state*)o->initialization,o,b,out);case 0x322:return gh_om322_initialize_00196188((gh_om322_state*)o->initialization,o,b,out);case 0x323:return gh_om323_initialize_001977b0((gh_om323_state*)o->initialization,o,b,out);case 0x324:return gh_om324_initialize_00198d40((gh_om324_state*)o->initialization,o,b,out);case 0x33f:return gh_om33f_initialize_0019f190((gh_om33f_state*)o->initialization,o,b,out);case 0x346:return gh_om346_initialize_001a0e98((gh_om346_state*)o->initialization,o,b,out);default:return GH_PLACE_UNSUPPORTED;}}
static gh_scene_placement_status abort_class(gh_om_object*o){switch(o->source.type){case 0x300:return gh_om_initialize_abort(o->initialization);case 0x303:return gh_om303_abort((gh_om303_state*)o->initialization);case 0x304:return gh_om304_abort((gh_om304_state*)o->initialization);case 0x305:return gh_om305_abort((gh_om305_state*)o->initialization);case 0x307:return gh_om307_abort((gh_om307_state*)o->initialization);case 0x30a:return gh_om30a_abort((gh_om30a_state*)o->initialization);case 0x30e:return gh_om30e_abort((gh_om30e_state*)o->initialization);case 0x310:return gh_om310_abort((gh_om310_state*)o->initialization);case 0x320:return gh_om320_abort((gh_om320_state*)o->initialization);case 0x322:return gh_om322_abort((gh_om322_state*)o->initialization);case 0x323:return gh_om323_abort((gh_om323_state*)o->initialization);case 0x324:return gh_om324_abort((gh_om324_state*)o->initialization);case 0x33f:return gh_om33f_abort((gh_om33f_state*)o->initialization);case 0x346:return gh_om346_abort((gh_om346_state*)o->initialization);default:return GH_PLACE_UNSUPPORTED;}}
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
  size_t object_bytes=class_bytes(r->type);if(!object_bytes)return fail(f,creator(r->type),r->type,GH_PLACE_UNSUPPORTED);
  uint32_t flags=word((const uint8_t*)f->bindings.work+0x594);if(flags&0x10000)return GH_PLACE_OK;
  gh_room_pool_status pe=gh_room_arena_allocate_002a9538(f->bindings.pools,0x754c10,flags,object_bytes,16,&o->allocation);
  if(pe)return fail(f,0x2a9538,pe,GH_PLACE_MEMORY);if(!o->allocation.pointer)return GH_PLACE_OK;
  memset(o->allocation.pointer,0,object_bytes);gh_scene_placement_status e=construct_class(r->type,o->allocation.pointer,object_bytes,&o->view);if(e)return e;o->view.initial_memory=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;o->source=*r;o->stage=1;
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
 if(!f->bindings.initializer)return fail(f,0x1b7118,r->index,GH_PLACE_UNSUPPORTED);
 if(f->bindings.initializer->work!=(const void*)f->bindings.work)return GH_PLACE_ARGUMENT;
 if(!o->initialization){o->initialization=calloc(1,state_bytes(r->type));if(!o->initialization)return GH_PLACE_MEMORY;}
 int initialized=0;gh_scene_placement_status status=initialize_class(o,f->bindings.initializer,&initialized);if(status)return fail(f,state_frontier(o),r->index,status);

 if(!initialized)return GH_PLACE_OK;
 if(o->stage==4)o->stage=5;
 if(o->stage==5){/*Actual1C3CB8 doorvirtual+74/+7C, after1B7118.*/if(r->type==0x303||r->type==0x305||r->type==0x320){gh_om303_state*state=(gh_om303_state*)o->initialization;gh_scene_placement_status e=gh_om303_set_position_001c3778(state,r->bytes);if(e)return fail(f,state->frontier,r->index,e);e=gh_om303_set_euler_001c3988(state,r->bytes+0x10);if(e)return fail(f,state->frontier,r->index,e);}else{memcpy(p+0x490,r->bytes,12);memcpy(o->view.alias_f0,p+0x490,12);put(p,0x5b8,word(p+0x5b8)|0x10u);memcpy(p+0x100,r->bytes+0x10,12);put(p,0x5b8,word(p+0x5b8)|0x10u);}o->stage=6;}
 if(o->stage==6){if(word(r->bytes+0x20)==0x2c3)return fail(f,0x292af0,r->index,GH_PLACE_UNSUPPORTED);
  if(r->version){float field2ae;memcpy(&field2ae,r->bytes+0x28,4);if(!isfinite(field2ae)||field2ae>=2147483648.0f)return fail(f,0x1c3cb8,r->index,GH_PLACE_UNSUPPORTED);if(field2ae>.001f)half(p,0x2ae,(uint16_t)(int32_t)field2ae);if(r->version>=2){memcpy(p+0x538,r->bytes+0x30,8);memcpy(p+0x540,r->bytes+0x38,8);p[0x2fd]=r->bytes[0x2f];}}o->stage=7;}
 f->count++;*out=1;return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_factory_object_view(const gh_om_factory*f,uint32_t handle,gh_title_player_view*out){
 if(!f||!f->bound||!out||!handle)return GH_PLACE_ARGUMENT;void*object=NULL;
 if(gh_room_object_lookup(f->bindings.pools,handle,GH_ROOM_OBJECT_OM,f,&object)!=GH_POOL_OK)return GH_PLACE_BOUNDS;
 const gh_om_object*o=object;*out=o->view;return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_factory_release(gh_om_factory*f){
 if(!f)return GH_PLACE_ARGUMENT;
 for(size_t i=0;i<f->capacity;i++){gh_om_object*o=f->objects+i;
  if(o->initialization){gh_scene_placement_status status=abort_class(o);if(status)return status;free(o->initialization);o->initialization=NULL;}
  if(o->registered){if(gh_room_object_unregister(f->bindings.pools,o->handle,f)!=GH_POOL_OK)return GH_PLACE_BUSY;o->registered=0;}
  if(o->acquired){if(gh_game_objects_release_reference_001ff0b0(o->resource_manager,o->source.type)!=GH_OBJECT_OK)return GH_PLACE_BUSY;o->acquired=0;}
  if(o->allocation.pointer){if(gh_room_arena_free_002a9680(f->bindings.pools,o->allocation)!=GH_POOL_OK)return GH_PLACE_MEMORY;o->allocation=(gh_room_arena_allocation){0};}
 }
 free(f->objects);*f=(gh_om_factory){0};return GH_PLACE_OK;
}
