/* Actual313A60->186738->1BBEB8->1B6E90->shared2A7338.
 * Shared room arena, serial, source registries and lists are borrowed owners. */
#include "game_om_factory.h"
#include "game_om_initialize.h"
#include "game_om303.h"
#include "game_om_fragments.h"
#include "game_om311.h"
#include "game_om304.h"
#include "game_om305.h"
#include "game_om31a.h"
#include "game_om307.h"
#include "game_om30a.h"
#include "game_om30e.h"
#include "game_om310.h"
#include "game_om320.h"
#include "game_om322.h"
#include "game_om323.h"
#include "game_om324.h"
#include "game_om339.h"
#include "game_om33f.h"
#include "game_om346.h"
#include "game_om354.h"
#include "game_om355.h"
#include "game_om35d.h"
#include "game_om382.h"
#include "game_actor_external_parent.h"
#include "game_om309.h"
#include "game_om_base.h"
#include "game_om3eb.h"
#include "game_om041c.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}
static void half(uint8_t*p,size_t o,uint16_t n){memcpy(p+o,&n,2);}
static void vf0(uint8_t*p,size_t o){memset(p+o,0,12);put(p,o+12,0x3f800000);}
static gh_scene_placement_status fail(gh_om_factory*f,uint32_t pc,uint32_t detail,gh_scene_placement_status e){f->frontier=pc;f->detail=detail;return e;}
static uint32_t creator(uint32_t type){
 const gh_om_fragment_profile *fragment=gh_om_fragment_profile_for(type);
 if(fragment)return fragment->creator;
 switch(type){case 0x300:return 0x313a60;case 0x303:return 0x313bb0;case 0x304:return 0x313c20;case 0x305:return 0x313c90;case 0x307:return 0x313d70;case 0x308:return 0x313de0;case 0x309:return 0x313e50;case 0x30a:return 0x313ec0;case 0x30b:return 0x313f30;case 0x30c:return 0x313fa0;case 0x30e:return 0x314080;case 0x310:return 0x314160;case 0x311:return 0x3141d0;case 0x313:return 0x3142b0;case 0x317:return 0x314470;case 0x318:return 0x3144e0;case 0x31a:return 0x314550;case 0x320:return 0x3147f0;case 0x322:return 0x314860;case 0x323:return 0x3148d0;case 0x324:return 0x314940;case 0x326:return 0x314a20;case 0x330:return 0x314ef0;case 0x331:return 0x314f60;case 0x333:return 0x314fd0;case 0x338:return 0x315200;case 0x339:return 0x315270;case 0x351:return 0x315ac0;case 0x33f:return 0x315510;case 0x346:return 0x315820;case 0x354:return 0x315c10;case 0x355:return 0x315c80;case 0x356:return 0x315cf0;case 0x357:return 0x315d60;case 0x35d:return 0x315f90;case 0x382:return 0x316150;case 0x41c:return 0x317420;case 0x3d3:case 0x3d4:case 0x3d5:return 0x316540;default:return 0x30fe68;}}
static gh_scene_placement_status construct(uint8_t*p,size_t n,gh_title_player_view*out){
 if(gh_om_base_construct_001b6e90(p,n,out)!=GH_PLACE_OK)return GH_PLACE_BOUNDS;
 /* 1BBEB8 */put(p,0x214,0x4292f0);vf0(p,0x610);put(p,0x620,0);put(p,0x634,0);half(p,0x630,0);put(p,0x644,0);
 /* 186738 seven0x60 records; only first5VF0 andbyte54 written. */
 put(p,0x214,0x422f90);for(unsigned i=0;i<7;i++){size_t o=0x650+(size_t)i*0x60;for(unsigned j=0;j<5;j++)vf0(p,o+j*16);p[o+0x54]=0;}
 out->original_vtable214=0x422f90;out->original_kind4ac=0x4289f0;out->initial_memory=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;return GH_PLACE_OK;
}
/* Native bindings for the original creator/vtable targets. Unknown classes
 * have no allocation/empty virtual substitute. */
static size_t class_bytes(uint32_t type){
 const gh_om_fragment_profile *fragment=gh_om_fragment_profile_for(type);
 if(fragment)return fragment->bytes;
 switch(type){case 0x300:return 0x8f0;case 0x303:return 0x9e0;case 0x304:return 0x850;case 0x305:return 0x860;case 0x307:return 0xce0;case 0x30a:return 0x9c0;case 0x30b:case 0x30c:case 0x331:return 0x8a0;case 0x30e:return 0x610;case 0x310:return 0x920;case 0x311:return GH_OM311_BYTES;case 0x313:return 0x8f0;case 0x317:return 0x860;case 0x318:return 0x800;case 0x31a:return GH_OM31A_BYTES;case 0x320:return 0x800;case 0x322:return 0x9e0;case 0x323:return 0x9e0;case 0x324:case 0x326:return 0x610;case 0x338:return GH_OM338_BYTES;case 0x339:return GH_OM339_BYTES;case 0x330:case 0x333:return 0x8f0;case 0x33f:return 0xb40;case 0x351:return 0x690;case 0x346:return 0xda0;case 0x354:return 0x6b0;case 0x355:return GH_OM355_BYTES;case 0x356:return 0x600;case 0x357:return GH_OM357_BYTES;case 0x35d:return 0xb00;case 0x382:return 0x790;case 0x308:return 0x910;case 0x309:return 0x9c0;case 0x41c:return GH_OM041C_BYTES;case 0x3d3:case 0x3d4:case 0x3d5:return 0x690;default:return 0;}}
/* 315BA0 zeroes 0x630 bytes before invoking 1A7788. */
static gh_scene_placement_status construct_om353(void *memory,size_t size,gh_title_player_view *out)
{
 gh_scene_placement_status status=gh_om_base_construct_001b6e90(memory,size,out);
 if(status)return status;
 uint8_t *actor=memory;
 out->original_vtable214=0x4275c8;
 put(actor,0x214,out->original_vtable214);
 memset(actor+0x600,0xff,16);
 memset(actor+0x610,0,3);
 actor[0x614]=0;
 actor[0x615]=0;
 return GH_PLACE_OK;
}

static gh_scene_placement_status construct_om356(void *memory,size_t size,gh_title_player_view *out)
{
 gh_scene_placement_status status=gh_om_base_construct_001b6e90(memory,size,out);
 if(status)return status;
 out->original_vtable214=0x427880; /* 1A86F8 */
 put(memory,0x214,out->original_vtable214);
 return GH_PLACE_OK;
}

static gh_scene_placement_status construct_class(uint32_t type,void*p,size_t bytes,gh_title_player_view*out){
 const gh_om_fragment_profile *fragment=gh_om_fragment_profile_for(type);
 if(fragment)return gh_om_fragment_construct(fragment,p,bytes,out);
 switch(type){case 0x300:return construct(p,bytes,out);case 0x303:return gh_om303_construct_001879c8(p,bytes,out);case 0x304:return gh_om304_construct_00187c88(p,bytes,out);case 0x305:return gh_om305_construct_00188a68(p,bytes,out);case 0x307:return gh_om307_construct_00188f80(p,bytes,out);case 0x30b:return gh_om30b_construct_0018cea0(p,bytes,out);case 0x30c:return gh_om30c_construct_0018dc58(p,bytes,out);case 0x331:return gh_om331_construct_0019c588(p,bytes,out);case 0x30a:return gh_om30a_construct_0018c098(p,bytes,out);case 0x30e:return gh_om30e_construct_0018ec98(p,bytes,out);case 0x313:case 0x330:case 0x333:return gh_om_r101_door_construct(type,p,bytes,out);case 0x351:return gh_om351_construct_001a5fd8(p,bytes,out);case 0x317:case 0x318:return gh_om_r101_fragment_construct(type,p,bytes,out);case 0x310:return gh_om310_construct_0018f110(p,bytes,out);case 0x311:return gh_om311_construct_001901c8(p,bytes,out);case 0x31a:return gh_om31a_construct_001931f0(p,bytes,out);case 0x320:return gh_om320_construct_00195c20(p,bytes,out);case 0x322:return gh_om322_construct_00196058(p,bytes,out);case 0x323:return gh_om323_construct_00197680(p,bytes,out);case 0x324:return gh_om324_construct_00198ca8(p,bytes,out);case 0x326:return gh_om326_construct_00199dc0(p,bytes,out);case 0x338:return gh_om338_construct_0019ded0(p,bytes,out);case 0x339:return gh_om339_construct_0019e1c0(p,bytes,out);case 0x33f:return gh_om33f_construct_0019f0a8(p,bytes,out);case 0x346:return gh_om346_construct_001a0d68(p,bytes,out);case 0x354:return gh_om354_construct_001a8150(p,bytes,out);case 0x357:return gh_om357_construct_001a88b0(p,bytes,out);case 0x355:return gh_om355_construct_001a8348(p,bytes,out);case 0x356:return construct_om356(p,bytes,out);case 0x35d:return gh_om35d_construct_001ae798(p,bytes,out);case 0x382:return gh_om382_construct_001c9140(p,bytes,out);case 0x308:return gh_om308_construct_0018a598(p,bytes,out);case 0x309:return gh_om309_construct_0018b290(p,bytes,out);case 0x3d3:case 0x3d4:case 0x3d5:return gh_om_r101_item_construct_001c0068(p,bytes,out);case 0x41c:return gh_om041c_construct_0017b870(p,bytes,out);default:return GH_PLACE_UNSUPPORTED;}}
static size_t state_bytes(uint32_t type){return type==0x311?sizeof(gh_om311_state):(type==0x41c||type==0x357)?sizeof(gh_om041c_state):type==0x382?sizeof(gh_om382_state):type==0x35d?sizeof(gh_om35d_state):type==0x346?sizeof(gh_om346_state):type==0x323?sizeof(gh_om323_state):type==0x322?sizeof(gh_om322_state):type==0x310?sizeof(gh_om310_state):(type==0x300||type==0x313||type==0x330||type==0x333||type==0x351||type==0x354||type==0x355||type==0x356||(type>=0x3d3&&type<=0x3d5))?sizeof(gh_om_init_state):type==0x307?sizeof(gh_om307_state):sizeof(gh_om303_state);}
static uint32_t state_frontier(const gh_om_object*o){return o->source.type==0x311?((gh_om311_state*)o->initialization)->frontier:(o->source.type==0x41c||o->source.type==0x357)?((gh_om041c_state*)o->initialization)->frontier:o->source.type==0x382?((gh_om382_state*)o->initialization)->frontier:o->source.type==0x35d?((gh_om35d_state*)o->initialization)->frontier:o->source.type==0x346?((gh_om346_state*)o->initialization)->frontier:o->source.type==0x323?((gh_om323_state*)o->initialization)->frontier:o->source.type==0x322?((gh_om322_state*)o->initialization)->frontier:(o->source.type==0x300||o->source.type==0x313||o->source.type==0x330||o->source.type==0x333||o->source.type==0x351||o->source.type==0x354||o->source.type==0x355||o->source.type==0x356||(o->source.type>=0x3d3&&o->source.type<=0x3d5))?o->initialization->frontier:o->source.type==0x307?((gh_om307_state*)o->initialization)->frontier:((gh_om303_state*)o->initialization)->frontier;}
static gh_scene_placement_status initialize_class(gh_om_object*o,const gh_om_initializer_bindings*b,int*out){
 const gh_om_fragment_profile *fragment=gh_om_fragment_profile_for(o->source.type);
 if(fragment)return gh_om_fragment_initialize(fragment,(gh_om303_state*)o->initialization,o,b,out);
 switch(o->source.type){case 0x351:return gh_om351_initialize_001a6098(o->initialization,o,b,out);case 0x300:case 0x313:case 0x330:case 0x333:return gh_om_door_initialize_001b7118(o->initialization,o,b,out);case 0x303:return gh_om303_initialize_00187a78((gh_om303_state*)o->initialization,o,b,out);case 0x304:return gh_om304_initialize_00187d88((gh_om304_state*)o->initialization,o,b,out);case 0x305:return gh_om305_initialize_00188b50((gh_om305_state*)o->initialization,o,b,out);case 0x307:return gh_om307_initialize_001890b0((gh_om307_state*)o->initialization,o,b,out);case 0x30b:return gh_om30b_initialize_0018cf88((gh_om309_state*)o->initialization,o,b,out);case 0x30c:return gh_om30c_initialize_0018dd40((gh_om309_state*)o->initialization,o,b,out);case 0x331:return gh_om331_initialize_0019c670((gh_om309_state*)o->initialization,o,b,out);case 0x30a:return gh_om30a_initialize_0018c180((gh_om30a_state*)o->initialization,o,b,out);case 0x30e:return gh_om30e_initialize_0018ed30((gh_om30e_state*)o->initialization,o,b,out);case 0x310:return gh_om310_initialize_0018f218((gh_om310_state*)o->initialization,o,b,out);case 0x311:return gh_om311_initialize_001902f8((gh_om311_state*)o->initialization,o,b,out);case 0x317:case 0x318:case 0x31a:return gh_om31a_initialize_001932d8((gh_om303_state*)o->initialization,o,b,out);case 0x320:return gh_om320_initialize_00195d08((gh_om320_state*)o->initialization,o,b,out);case 0x322:return gh_om322_initialize_00196188((gh_om322_state*)o->initialization,o,b,out);case 0x323:return gh_om323_initialize_001977b0((gh_om323_state*)o->initialization,o,b,out);case 0x324:return gh_om324_initialize_00198d40((gh_om324_state*)o->initialization,o,b,out);case 0x326:return gh_om326_initialize_00199e58((gh_om30e_state*)o->initialization,o,b,out);case 0x338:return gh_om338_initialize_0019dfb8((gh_om303_state*)o->initialization,o,b,out);case 0x339:return gh_om339_initialize_0019e2a8((gh_om303_state*)o->initialization,o,b,out);case 0x33f:return gh_om33f_initialize_0019f190((gh_om33f_state*)o->initialization,o,b,out);case 0x346:return gh_om346_initialize_001a0e98((gh_om346_state*)o->initialization,o,b,out);case 0x354:return gh_om354_initialize_001a8200(o->initialization,o,b,out);case 0x355:return gh_om355_initialize_001a83e0(o->initialization,o,b,out);
 case 0x356:return gh_om_common_initialize_001b6fb8(o->initialization,o,b,out); /* 1A8758 */case 0x35d:return gh_om35d_initialize_001ae928((gh_om35d_state*)o->initialization,o,b,out);case 0x382:return gh_om382_initialize_001c9268((gh_om382_state*)o->initialization,o,b,out);case 0x308:return gh_om308_initialize_0018a680((gh_om309_state*)o->initialization,o,b,out);case 0x309:return gh_om309_initialize_0018b378((gh_om309_state*)o->initialization,o,b,out);case 0x3d3:case 0x3d4:case 0x3d5:return gh_om_r101_item_initialize_001c0128(o->initialization,o,b,out);case 0x357:case 0x41c:return gh_om041c_initialize_0017b940((gh_om041c_state*)o->initialization,o,b,out);default:return GH_PLACE_UNSUPPORTED;}}
static gh_scene_placement_status abort_class(gh_om_object*o){if(o->runtime_created){
 if(o->runtime_kind==0x3eb||o->runtime_kind==0x353||o->runtime_kind==0x3d6||o->runtime_kind==0x3da||o->runtime_kind==0x3c4||o->runtime_kind==0x3c8||o->runtime_kind==0x3d0)return gh_om_initialize_abort(o->initialization);
 if(o->runtime_kind==0x3be)return gh_om3be_abort((gh_om3be_state *)o->initialization);
 if(o->runtime_kind==0x376||o->runtime_kind==0x363){
  if(o->initialization->model.external_owner&&gh_actor_model_clear_external_parent(&o->initialization->model,o))return GH_PLACE_BUSY;
  return gh_om382_abort((gh_om382_state *)o->initialization);
 }
 return GH_PLACE_UNSUPPORTED;
 }switch(o->source.type){case 0x300:case 0x313:case 0x330:case 0x333:case 0x351:case 0x3d3:case 0x3d4:case 0x3d5:return gh_om_initialize_abort(o->initialization);case 0x303:return gh_om303_abort((gh_om303_state*)o->initialization);case 0x304:return gh_om304_abort((gh_om304_state*)o->initialization);case 0x305:return gh_om305_abort((gh_om305_state*)o->initialization);case 0x307:return gh_om307_abort((gh_om307_state*)o->initialization);case 0x30a:return gh_om30a_abort((gh_om30a_state*)o->initialization);case 0x30e:case 0x326:return gh_om30e_abort((gh_om30e_state*)o->initialization);case 0x310:return gh_om310_abort((gh_om310_state*)o->initialization);case 0x311:return gh_om311_abort((gh_om311_state*)o->initialization);case 0x317:case 0x318:case 0x31a:return gh_om303_abort((gh_om303_state*)o->initialization);case 0x320:return gh_om320_abort((gh_om320_state*)o->initialization);case 0x322:return gh_om322_abort((gh_om322_state*)o->initialization);case 0x323:return gh_om323_abort((gh_om323_state*)o->initialization);case 0x324:return gh_om324_abort((gh_om324_state*)o->initialization);case 0x31b:case 0x336:case 0x337:case 0x338:case 0x339:return gh_om303_abort((gh_om303_state*)o->initialization);case 0x33f:return gh_om33f_abort((gh_om33f_state*)o->initialization);case 0x346:return gh_om346_abort((gh_om346_state*)o->initialization);case 0x354:return gh_om354_abort(o->initialization);case 0x355:case 0x356:return gh_om_initialize_abort(o->initialization);case 0x35d:return gh_om35d_abort((gh_om35d_state*)o->initialization);case 0x382:return gh_om382_abort((gh_om382_state*)o->initialization);case 0x308:case 0x309:case 0x30b:case 0x30c:case 0x331:return gh_om309_abort((gh_om309_state*)o->initialization);case 0x357:case 0x41c:return gh_om041c_abort((gh_om041c_state*)o->initialization);default:return GH_PLACE_UNSUPPORTED;}}
gh_scene_placement_status gh_om_factory_bind(gh_om_factory*f,const gh_om_factory_bindings*b,size_t capacity){
 if(!f||!b||!b->pools||!b->pools->initialized||!b->work||!b->room_state||!b->room_state->active||!b->objects||!b->room_objects||!capacity||capacity>65535)return GH_PLACE_ARGUMENT;
 if(f->bound||f->objects)return GH_PLACE_BUSY;gh_om_object*o=calloc(capacity,sizeof(*o));if(!o)return GH_PLACE_MEMORY;
 f->objects=o;f->capacity=capacity;f->bindings=*b;f->bound=1;return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_factory_create_record(gh_om_factory*f,const gh_om_placement_record*r,int*out){
 if(!f||!f->bound||!r||!r->bytes||r->size<64||!out)return GH_PLACE_ARGUMENT;*out=0;f->frontier=0;f->detail=0;
 /* This slice retains the current partialobject atcount; virtualInit hasnot
  * returned, so the originalouterloop mustnot advance pastit. */
 if(f->runtime_pending)return GH_PLACE_BUSY;
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
 if(o->stage==5&&(r->type==0x41c||r->type==0x357)){
  gh_om041c_state *state=(gh_om041c_state *)o->initialization;
  status=gh_om041c_set_position_0017c3a0(state,r->bytes);
  if(!status)status=gh_om041c_set_euler_0017c408(state,r->bytes+0x10);
  if(status)return fail(f,state->frontier,r->index,status);
  o->stage=6;
 }
 if(o->stage==5&&r->type>=0x3d3&&r->type<=0x3d5){
  status=gh_om_r101_item_position_001c2630(o->initialization,r->bytes);
  if(status)return fail(f,o->initialization->frontier,r->index,status);
  memcpy(p+0x100,r->bytes+0x10,12);
  o->stage=6;
 }
 if(o->stage==5){/*Actual1C3CB8 doorvirtual+74/+7C, after1B7118.*/if(gh_om_fragment_profile_for(r->type)||r->type==0x303||r->type==0x305||r->type==0x317||r->type==0x318||r->type==0x31a||r->type==0x320||r->type==0x338||r->type==0x339){gh_om303_state*state=(gh_om303_state*)o->initialization;gh_scene_placement_status e=gh_om303_set_position_001c3778(state,r->bytes);if(e)return fail(f,state->frontier,r->index,e);e=gh_om303_set_euler_001c3988(state,r->bytes+0x10);if(e)return fail(f,state->frontier,r->index,e);}else{memcpy(p+0x490,r->bytes,12);memcpy(o->view.alias_f0,p+0x490,12);put(p,0x5b8,word(p+0x5b8)|0x10u);memcpy(p+0x100,r->bytes+0x10,12);put(p,0x5b8,word(p+0x5b8)|0x10u);}o->stage=6;}
 if(o->stage==6){if(word(r->bytes+0x20)==0x2c3)return fail(f,0x292af0,r->index,GH_PLACE_UNSUPPORTED);
  if(r->version){float field2ae;memcpy(&field2ae,r->bytes+0x28,4);if(!isfinite(field2ae)||field2ae>=2147483648.0f)return fail(f,0x1c3cb8,r->index,GH_PLACE_UNSUPPORTED);if(field2ae>.001f)half(p,0x2ae,(uint16_t)(int32_t)field2ae);if(r->version>=2){memcpy(p+0x538,r->bytes+0x30,8);memcpy(p+0x540,r->bytes+0x38,8);p[0x2fd]=r->bytes[0x2f];}}o->stage=7;}
 f->count++;*out=1;return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_factory_object_view(const gh_om_factory*f,uint32_t handle,gh_title_player_view*out){
 if(!f||!f->bound||!out||!handle)return GH_PLACE_ARGUMENT;void*object=NULL;
 if(gh_room_object_lookup(f->bindings.pools,handle,GH_ROOM_OBJECT_OM,f,&object)!=GH_POOL_OK)return GH_PLACE_BOUNDS;
 const gh_om_object*o=object;*out=o->view;return GH_PLACE_OK;
}

gh_scene_placement_status gh_om_factory_create_0030fa98(gh_om_factory*f,gh_om_runtime_create*c,uint16_t kind,uint16_t id,gh_om_object**out){
 if(!f||!f->bound||!c||!out)return GH_PLACE_ARGUMENT;*out=NULL;f->frontier=0;f->detail=0;
 if(c->bound&&(c->factory!=f||c->kind!=kind||c->resource_id!=id))return GH_PLACE_BUSY;
 if(c->complete){*out=c->object;return GH_PLACE_OK;}
 if(f->runtime_pending&&f->runtime_pending!=c)return GH_PLACE_BUSY;
 const int16_t room=f->bindings.work->room;
 const int paper=kind==0x376&&gh_om376_paper_room_supported(room);
 const int room_weapon=kind==0x363&&room==0x102;
 const int mount=kind==0x353&&room==0x102;
 const int ticket=kind==0x3da&&room==0x102;
 const int projectile=kind==0x3be&&room==0x100;
 const int item=(kind==0x3d6&&(room==0x101||room==0x103))||
                (kind==0x3c4&&(room==0x102||room==0x103))||
                ((kind==0x3c8||kind==0x3d0)&&room==0x103);
 if(id!=0xffff||(kind!=0x3eb&&!paper&&!room_weapon&&!mount&&!projectile&&!item&&!ticket))
  return fail(f,0x30fe68,kind,GH_PLACE_UNSUPPORTED);
 const int weapon=paper||room_weapon;
 const size_t bytes=weapon?0x790:(mount||projectile)?0x630:(item||ticket)?0x690:0x610;
 const uint32_t constructor=weapon?0x1c9140:mount?0x1a7788:projectile?0x1b8ff8:(item||ticket)?0x1c0068:0x1bf478;
 const uint32_t creator=weapon?0x316150:mount?0x315ba0:projectile?0x316620:(item||ticket)?0x316540:0x316850;
 if(!c->bound){c->factory=f;c->kind=kind;c->resource_id=id;c->bound=1;}
 if(!c->object){const gh_room_vector*all,*om;if(gh_room_registry_view(f->bindings.pools,0x754c30,&all)||gh_room_registry_view(f->bindings.pools,0x754c58,&om))return fail(f,0x30fb70,0,GH_PLACE_ARGUMENT);
  if(all->count>=all->capacity||om->count>=om->capacity){c->complete=1;return GH_PLACE_OK;}
  uint32_t flags=word((const uint8_t*)f->bindings.work+0x594);if(flags&0x10000){c->complete=1;return GH_PLACE_OK;}
  size_t slot=f->count;
  if(projectile){
   for(size_t i=0;i<f->count;i++)if(!f->objects[i].stage){slot=i;break;}
  }
  if(slot>=f->capacity)return fail(f,creator,kind,GH_PLACE_MEMORY);
  gh_om_object*o=f->objects+slot;if(o->stage)return GH_PLACE_BUSY;
  gh_room_pool_status pe=gh_room_arena_allocate_002a9538(f->bindings.pools,0x754c10,flags,bytes,16,&o->allocation);if(pe)return fail(f,0x2a9538,pe,GH_PLACE_MEMORY);
  if(!o->allocation.pointer){c->complete=1;return GH_PLACE_OK;}
  memset(o->allocation.pointer,0,bytes);
  gh_scene_placement_status e=weapon?gh_om382_construct_001c9140(o->allocation.pointer,bytes,&o->view):
      mount?construct_om353(o->allocation.pointer,bytes,&o->view):
      projectile?gh_om3be_construct_001b8ff8(o->allocation.pointer,bytes,&o->view):
      (item||ticket)?gh_om_r101_item_construct_001c0068(o->allocation.pointer,bytes,&o->view):
      gh_om3eb_construct_001bf478(o->allocation.pointer,bytes,&o->view);
  if(e)return fail(f,constructor,kind,e);
  o->view.initial_memory=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;o->runtime_created=1;o->runtime_kind=kind;o->runtime_resource_id=id;o->stage=1;c->object=o;f->runtime_pending=c;
 }
 gh_om_object*o=c->object;uint8_t*p=o->view.bytes;
 if(o->stage==1){if(!f->bindings.factory_serial)return fail(f,0x30fe68,kind,GH_PLACE_UNSUPPORTED);p[0x2fc]=*f->bindings.factory_serial;*f->bindings.factory_serial=(uint8_t)(*f->bindings.factory_serial+1u);half(p,0x2fe,kind);put(p,0x2d0,0x20000000);half(p,0x300,kind);o->stage=2;}
 if(o->stage==2){gh_object_view resource;gh_object_status e=gh_game_objects_find(f->bindings.room_objects,kind,&resource);
  if(e==GH_OBJECT_MISSING){if(!f->bindings.resources)return fail(f,0x1fe8a0,kind,GH_PLACE_UNSUPPORTED);uint32_t pc=0;e=gh_game_objects_load_wait_001fe8a0(&o->resource_load,f->bindings.objects,kind,f->bindings.resources,1,&o->resource,&pc);if(e)return fail(f,pc?pc:0x1fe8a0,e,GH_PLACE_UNSUPPORTED);o->resource_manager=f->bindings.objects;put(p,0x250,word(p+0x250)|0x2000000u);}
  else {if(e!=GH_OBJECT_OK)return fail(f,0x1fea48,e,GH_PLACE_BOUNDS);e=gh_game_objects_acquire_001fea48(f->bindings.room_objects,kind,&o->resource);if(e)return fail(f,0x1fea48,e,GH_PLACE_BOUNDS);o->resource_manager=f->bindings.room_objects;}
  if(!o->resource.bytes)return fail(f,0x312778,kind,GH_PLACE_UNSUPPORTED);o->acquired=1;o->stage=3;
 }
 if(o->stage==3){gh_room_pool_status e=gh_room_object_register(f->bindings.pools,o,f,GH_ROOM_OBJECT_OM,9,&o->handle);if(e)return fail(f,0x30fcc0,e,GH_PLACE_MEMORY);o->registered=1;o->stage=4;}
 /* Original30FA98 ends at registration. The caller, not this constructor,
  * next invokes vtable+44. Never execute OMS placement initialization here. */
 c->complete=1;f->runtime_pending=NULL;
 if(o==f->objects+f->count)f->count++;
 *out=o;return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_factory_initialize_runtime_3eb(gh_om_factory*f,gh_om_object*o,int*out){
 if(!f||!f->bound||!o||!out||!o->runtime_created||o->runtime_kind!=0x3eb||!o->registered)return GH_PLACE_ARGUMENT;
 void*same=NULL;if(gh_room_object_lookup(f->bindings.pools,o->handle,GH_ROOM_OBJECT_OM,f,&same)||same!=o)return GH_PLACE_ARGUMENT;
 if(!f->bindings.initializer)return fail(f,0x1bf4e0,0x3eb,GH_PLACE_UNSUPPORTED);
 if(f->bindings.initializer->work!=(const void*)f->bindings.work)return GH_PLACE_ARGUMENT;
 if(!o->initialization){o->initialization=calloc(1,sizeof(*o->initialization));if(!o->initialization)return GH_PLACE_MEMORY;}
 gh_scene_placement_status e=gh_om3eb_initialize_001bf4e0(o->initialization,o,f->bindings.initializer,out);if(e)return fail(f,o->initialization->frontier?o->initialization->frontier:0x1bf4e0,0x3eb,e);return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_factory_initialize_runtime_ticket_001c0128(gh_om_factory *f,gh_om_object *o)
{
 if(!f||!f->bound||!o||!o->runtime_created||o->runtime_kind!=0x3da||!o->registered)
  return GH_PLACE_ARGUMENT;
 void *same=NULL;
 if(gh_room_object_lookup(f->bindings.pools,o->handle,GH_ROOM_OBJECT_OM,f,&same)||same!=o)
  return GH_PLACE_ARGUMENT;
 const gh_om_initializer_bindings *bindings=f->bindings.initializer;
 if(!bindings)return fail(f,0x1c0128,0x3da,GH_PLACE_UNSUPPORTED);
 if(bindings->work!=f->bindings.work)return GH_PLACE_ARGUMENT;
 if(!o->initialization){
  o->initialization=calloc(1,sizeof(*o->initialization));
  if(!o->initialization)return GH_PLACE_MEMORY;
 }
 uint8_t *actor=o->view.bytes;
 put(actor,0x560,0x905);
 int returned=0;
 gh_scene_placement_status status=gh_om_r101_item_initialize_001c0128(o->initialization,o,bindings,&returned);
 return status||!returned?fail(f,o->initialization->frontier?o->initialization->frontier:0x1c0128,
  0x3da,status?status:GH_PLACE_UNSUPPORTED):GH_PLACE_OK;
}

gh_scene_placement_status gh_om_factory_initialize_runtime_mount_001a7820(gh_om_factory *f,gh_om_object *o)
{
 if(!f||!f->bound||!o||!o->runtime_created||o->runtime_kind!=0x353||!o->registered)
  return GH_PLACE_ARGUMENT;
 void *same=NULL;
 if(gh_room_object_lookup(f->bindings.pools,o->handle,GH_ROOM_OBJECT_OM,f,&same)||same!=o)
  return GH_PLACE_ARGUMENT;
 if(!f->bindings.initializer)return fail(f,0x1a7820,0x353,GH_PLACE_UNSUPPORTED);
 if(f->bindings.initializer->work!=f->bindings.work)return GH_PLACE_ARGUMENT;
 if(!o->initialization){
  o->initialization=calloc(1,sizeof(*o->initialization));
  if(!o->initialization)return GH_PLACE_MEMORY;
 }
 gh_om_init_state *state=o->initialization;
 int original=0;
 const gh_scene_placement_status status=gh_om_common_initialize_001b6fb8(state,o,f->bindings.initializer,&original);
 if(status||!original)return fail(f,state->frontier?state->frontier:0x1a7820,0x353,status?status:GH_PLACE_UNSUPPORTED);
 if(state->stage==3){
  uint8_t *actor=o->view.bytes;
  put(actor,0x5a0,word(actor+0x5a0)|1u);
  put(actor,0x628,0x1c2); /* 1A80F0 */
  put(actor,0x620,0); /* 1A80F8 */
  actor[0x613]=0;
  put(actor,0x5b8,word(actor+0x5b8)|8u);
  state->stage=4;
 }
 return GH_PLACE_OK;
}

gh_scene_placement_status gh_om_factory_initialize_runtime_weapon_001c9268(gh_om_factory *f,gh_om_object *o)
{
 if(!f||!f->bound||!o||!o->runtime_created||(o->runtime_kind!=0x376&&o->runtime_kind!=0x363)||!o->registered)return GH_PLACE_ARGUMENT;
 void *same=NULL;
 if(gh_room_object_lookup(f->bindings.pools,o->handle,GH_ROOM_OBJECT_OM,f,&same)||same!=o)return GH_PLACE_ARGUMENT;
 if(!f->bindings.initializer)return fail(f,0x1c9268,o->runtime_kind,GH_PLACE_UNSUPPORTED);
 if(f->bindings.initializer->work!=f->bindings.work)return GH_PLACE_ARGUMENT;
 if(!o->initialization){
  o->initialization=calloc(1,sizeof(gh_om382_state));
  if(!o->initialization)return GH_PLACE_MEMORY;
 }
 gh_om382_state *state=(gh_om382_state *)o->initialization;
 int initialized=0;
 gh_scene_placement_status status=gh_om382_initialize_001c9268(state,o,f->bindings.initializer,&initialized);
 if(status||!initialized)return fail(f,state->frontier?state->frontier:0x1c9268,o->runtime_kind,status?status:GH_PLACE_UNSUPPORTED);
 return GH_PLACE_OK;
}

gh_scene_placement_status gh_om_factory_initialize_runtime_376(gh_om_factory *factory,gh_om_object *paper)
{
 if(!paper||paper->runtime_kind!=0x376)return GH_PLACE_ARGUMENT;
 return gh_om_factory_initialize_runtime_weapon_001c9268(factory,paper);
}

gh_scene_placement_status gh_om_factory_initialize_runtime_3be(gh_om_factory *f,gh_om_object *o)
{
 if(!f||!f->bound||!o||!o->runtime_created||o->runtime_kind!=0x3be||!o->registered)return GH_PLACE_ARGUMENT;
 void *same=NULL;
 if(gh_room_object_lookup(f->bindings.pools,o->handle,GH_ROOM_OBJECT_OM,f,&same)||same!=o)
  return GH_PLACE_ARGUMENT;
 if(!f->bindings.initializer||f->bindings.initializer->work!=f->bindings.work)
  return fail(f,0x1b90d0,0x3be,GH_PLACE_UNSUPPORTED);
 if(!o->initialization){
  o->initialization=calloc(1,sizeof(gh_om3be_state));
  if(!o->initialization)return GH_PLACE_MEMORY;
 }
 gh_om3be_state *state=(gh_om3be_state *)o->initialization;
 gh_scene_placement_status status=gh_om3be_initialize_001b90d0(state,o,f->bindings.initializer);
 if(status)return fail(f,state->common.frontier?state->common.frontier:0x1b90d0,0x3be,status);
 return GH_PLACE_OK;
}



gh_scene_placement_status gh_om_factory_initialize_runtime_item_001c0128(gh_om_factory *f,gh_om_object *o,int *out)
{
 if(!f||!f->bound||!o||!out||!o->runtime_created||!o->registered||
    o->runtime_resource_id!=0xffff||!f->bindings.initializer||
    f->bindings.initializer->work!=f->bindings.work)return GH_PLACE_ARGUMENT;
 const int16_t room=f->bindings.work->room;
 const uint16_t kind=o->runtime_kind;
 if(!(((room==0x101||room==0x103)&&kind==0x3d6)||
      ((room==0x102||room==0x103)&&kind==0x3c4)||(room==0x103&&(kind==0x3c8||kind==0x3d0))))
  return GH_PLACE_ARGUMENT;
 void *same=NULL;
 if(gh_room_object_lookup(f->bindings.pools,o->handle,GH_ROOM_OBJECT_OM,f,&same)||same!=o)return GH_PLACE_ARGUMENT;
 if(!o->initialization){
  o->initialization=calloc(1,sizeof(*o->initialization));
  if(!o->initialization)return GH_PLACE_MEMORY;
 }
 gh_scene_placement_status status=gh_om_r101_item_initialize_001c0128(o->initialization,o,f->bindings.initializer,out);
 if(status)return fail(f,o->initialization->frontier?o->initialization->frontier:0x1c0128,kind,status);
 return GH_PLACE_OK;
}

gh_scene_placement_status gh_om_factory_release(gh_om_factory*f){
 if(!f)return GH_PLACE_ARGUMENT;
 for(size_t i=0;i<f->capacity;i++){gh_om_object*o=f->objects+i;
  if(o->initialization){gh_scene_placement_status status=abort_class(o);if(status)return status;free(o->initialization);o->initialization=NULL;}
  if(o->registered){if(gh_room_object_unregister(f->bindings.pools,o->handle,f)!=GH_POOL_OK)return GH_PLACE_BUSY;o->registered=0;}
  if(o->acquired){if(gh_game_objects_release_reference_001ff0b0(o->resource_manager,o->runtime_created?o->runtime_kind:o->source.type)!=GH_OBJECT_OK)return GH_PLACE_BUSY;o->acquired=0;}
  if(o->allocation.pointer){if(gh_room_arena_free_002a9680(f->bindings.pools,o->allocation)!=GH_POOL_OK)return GH_PLACE_MEMORY;o->allocation=(gh_room_arena_allocation){0};}
 }
 free(f->objects);*f=(gh_om_factory){0};return GH_PLACE_OK;
}

gh_scene_placement_status gh_om_factory_release_retired(gh_om_factory *factory)
{
 if(!factory||(factory->capacity&&!factory->objects)||factory->count>factory->capacity)
  return GH_PLACE_ARGUMENT;
 if(factory->runtime_pending)return GH_PLACE_BUSY;
 for(size_t i=0;i<factory->capacity;i++){
  const gh_om_object *object=&factory->objects[i];
  if(object->registered||object->acquired||object->allocation.pointer||
     object->initialization||object->view.bytes)return GH_PLACE_BUSY;
 }
 free(factory->objects);
 *factory=(gh_om_factory){0};
 return GH_PLACE_OK;
}
