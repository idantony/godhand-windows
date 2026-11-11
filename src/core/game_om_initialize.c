/*Actual1B7118->1867E8 common1B6FB8, independent readable nativeC.*/
#include "game_om_initialize.h"
#include "game_actor_model_nodes.h"
#include <string.h>
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
static gh_scene_placement_status fail(gh_om_init_state*s,uint32_t f,gh_scene_placement_status e){s->frontier=f;return e;}
gh_scene_placement_status gh_om_common_initialize_001b6fb8(gh_om_init_state*s,gh_om_object*o,const gh_om_initializer_bindings*b,int*out){
 if(!s||!o||!b||!out||!b->work||!b->packet_heap||!b->fog_rgb||!o->registered||!o->view.bytes||o->view.size<0x5f8||!o->resource.bytes||!o->resource.file_extent_known||!o->resource.file_extent||o->resource.file_extent>o->resource.size)return GH_PLACE_ARGUMENT;
 int runtime=o->runtime_created&&o->runtime_kind==0x3eb&&!o->source.bytes;
 if(!runtime&&(o->source.type!=0x300&&o->source.type!=0x303&&o->source.type!=0x304&&o->source.type!=0x305&&o->source.type!=0x307&&o->source.type!=0x30a&&o->source.type!=0x30e&&o->source.type!=0x310&&o->source.type!=0x31a&&o->source.type!=0x320&&o->source.type!=0x322&&o->source.type!=0x323&&o->source.type!=0x324&&o->source.type!=0x339&&o->source.type!=0x33f&&o->source.type!=0x346&&o->source.type!=0x354&&o->source.type!=0x355&&o->source.type!=0x35d&&o->source.type!=0x382&&o->source.type!=0x309&&o->source.type!=0x41c))return GH_PLACE_ARGUMENT;
 if(!runtime&&(!o->source.bytes||o->source.size<0x2e))return GH_PLACE_BOUNDS;
 if(runtime&&w(o->view.bytes+0x5e0))return fail(s,0x1b7de8,GH_PLACE_UNSUPPORTED);
 *out=0;s->frontier=0;
 if(s->object&&(s->object!=o||s->bindings!=b))return GH_PLACE_BUSY;s->object=o;s->bindings=b;uint8_t*p=o->view.bytes;
 if(!s->stage){if(!runtime){put(p,0x5e0,0);uint32_t v=w(o->source.bytes+0x20);if(v)put(p,0x560,v+(v>0x1ff?0x700u:0x300u));}gh_bundle resource;if(gh_bundle_open(o->resource.bytes,o->resource.file_extent,&resource))return fail(s,0x31ccf0,GH_PLACE_BOUNDS);
  gh_bundle_error e=gh_bundle_find(&resource,"MD",0,&s->geometry);if(e!=GH_BUNDLE_OK&&e!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,GH_PLACE_BOUNDS);if(e==GH_BUNDLE_NOT_FOUND)s->geometry=(gh_bundle_entry){0};e=gh_bundle_find(&resource,"TM3",0,&s->texture);if(e==GH_BUNDLE_NOT_FOUND){s->texture_mode=1;e=gh_bundle_find(&resource,"T3L",0,&s->texture);if(e==GH_BUNDLE_NOT_FOUND){s->texture_mode=0;e=gh_bundle_find(&resource,"T32",0,&s->texture);}}if(e!=GH_BUNDLE_OK&&e!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,GH_PLACE_BOUNDS);
  gh_actor_model_status me=gh_actor_model_bind_packet_inputs(&s->model,b->packet_heap,b->fog_rgb);if(me)return fail(s,0x149738,GH_PLACE_BUSY);s->stage=1;}
 if(s->stage==1){gh_actor_model_status e=gh_actor_model_initialize_00149738(&s->model,&o->view,b->work,o->allocation.pools,&s->geometry,&s->texture,0,s->texture_mode,GH_STATIC_BINARY32_REFERENCE,&s->model_return);if(e)return fail(s,s->model.frontier,e==GH_ACTOR_MODEL_UNSUPPORTED?GH_PLACE_UNSUPPORTED:e==GH_ACTOR_MODEL_MEMORY?GH_PLACE_MEMORY:GH_PLACE_BOUNDS);if(!s->model_return)return fail(s,0x312708,GH_PLACE_UNSUPPORTED);s->stage=2;}
 if(s->stage==2){if(!runtime&&o->source.bytes[0x2d])return fail(s,0x1b7de8,GH_PLACE_UNSUPPORTED);p[0x140]=0;put(p,0x5b8,w(p+0x5b8)|0x40u);put(p,0x254,w(p+0x254)|0x88000000u);put(p,0x4a8,w(p+0x4a8)|0x10000000u);uint16_t h=0xffff;memcpy(p+0x570,&h,2);put(p,0x574,2);put(p,0x3a8,0x3f800000);s->stage=3;}

 *out=1;return GH_PLACE_OK;
}
gh_scene_placement_status gh_om_door_initialize_001b7118(gh_om_init_state*s,gh_om_object*o,const gh_om_initializer_bindings*b,int*out){if(!o||o->source.type!=0x300||!out)return GH_PLACE_ARGUMENT;int common_return=0;gh_scene_placement_status ce=gh_om_common_initialize_001b6fb8(s,o,b,&common_return);if(ce)return ce;*out=0;if(!common_return)return fail(s,0x1b6fb8,GH_PLACE_UNSUPPORTED);uint8_t*p=o->view.bytes;
 if(s->stage==3){if(!b->init)return fail(s,0x1fe218,GH_PLACE_UNSUPPORTED);uint32_t frontier=0;int nonnull=0;gh_hitarea_status e=gh_hitarea_object_create_001fe218(b->init,&o->view,2,&s->hitarea,&nonnull,&frontier);if(e)return fail(s,frontier?frontier:0x1fe218,e==GH_HITAREA_MEMORY?GH_PLACE_MEMORY:GH_PLACE_UNSUPPORTED);s->hitarea_created=(uint8_t)nonnull;put(p,0x600,0);s->stage=4;}
 if(s->stage==4){if(s->hitarea_created){if(!b->shapes)return fail(s,0x1fd9d8,GH_PLACE_UNSUPPORTED);const uint32_t offset[3]={0xbf400000,0,0},rotation[3]={0,0,0},size[3]={0x3fc00000,0x4019999a,0x3e4ccccd};uint32_t frontier=0;int original_index;gh_hitarea_status e=gh_hitarea_add_box_001fd9d8(&s->hitarea,b->shapes,p+0x80,offset,rotation,size,&original_index,&frontier);if(e)return fail(s,frontier?frontier:0x1fd9d8,e==GH_HITAREA_MEMORY?GH_PLACE_MEMORY:GH_PLACE_UNSUPPORTED);}
  uint16_t hp=1;memcpy(p+0x548,&hp,2);memcpy(p+0x54a,&hp,2);const uint32_t size[3]={0x3fc00000,0x4019999a,0x3e4ccccd};memcpy(p+0x610,size,12);s->stage=5;}
 if(s->stage==5){if(!b->dynamic||!b->door_shared)return fail(s,0x1bd228,GH_PLACE_UNSUPPORTED);gh_scene_placement_status e=gh_om_door_geometry_001bd228(&s->geometry_state,o,b->dynamic,b->door_shared,GH_STATIC_BINARY32_REFERENCE);if(e)return fail(s,s->geometry_state.frontier,e);s->stage=6;}
 if(s->stage==6){p[0x624]=7;put(p,0x628,0);s->door_segments=p+0x650;
  /*1BBF68->1B7280 layer0visible/layer40hidden, SAME realnode records. */
  for(unsigned i=0;i<s->model.script.mesh_count;i++){const gh_actor_node_packets*packet;if(gh_actor_model_packet_at(&s->model,i,&packet))return fail(s,0x14b6f8,GH_PLACE_BOUNDS);uint8_t*n=packet->node;if(n[0x40c]==0)put(n,0x380,w(n+0x380)&~1u);else if(n[0x40c]==0x40)put(n,0x380,w(n+0x380)|1u);}
  for(unsigned i=1;i<7&&i<s->model.part_count;i++){const gh_actor_part*part;if(gh_actor_model_part_at(&s->model,i,&part))return fail(s,0x1bbf68,GH_PLACE_BOUNDS);put(part->bytes,0x154,w(part->bytes+0x154)|0x18u);}put(p,0x5b8,w(p+0x5b8)|4u);if(s->hitarea_created)put(s->hitarea.bytes,0,w(s->hitarea.bytes)|1u);s->stage=7;}
 s->frontier=0;*out=1;return GH_PLACE_OK;
}

gh_scene_placement_status gh_om_initialize_abort(gh_om_init_state*s){if(!s)return GH_PLACE_ARGUMENT;gh_scene_placement_status ge=gh_om_door_geometry_abort(&s->geometry_state);if(ge)return ge;if(s->hitarea_created){uint32_t frontier=0;gh_hitarea_status he=gh_hitarea_shapes_abort_object(&s->hitarea,&frontier);if(he)return fail(s,frontier?frontier:0x1fd580,GH_PLACE_UNSUPPORTED);he=gh_hitarea_object_release_empty_001fe278(&s->hitarea,&frontier);if(he)return fail(s,frontier?frontier:0x1fe278,GH_PLACE_UNSUPPORTED);s->hitarea_created=0;}gh_actor_model_status e=gh_actor_model_release(&s->model);if(e)return GH_PLACE_BUSY;memset(s,0,sizeof(*s));return GH_PLACE_OK;}
