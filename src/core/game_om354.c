/* Actual354315C10/1A8150/1A8200: one segment, realdynamicgeometry, noHitArea. */
#include "game_om354.h"
#include "game_om_base.h"
#include "game_actor_model_nodes.h"
#include <string.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}static void vf0(uint8_t*p,size_t o){memset(p+o,0,12);put(p,o+12,0x3f800000);}
gh_scene_placement_status gh_om354_construct_001a8150(void*data,size_t n,gh_title_player_view*out){if(!data||!out)return GH_PLACE_ARGUMENT;if(n<0x6b0)return GH_PLACE_BOUNDS;gh_scene_placement_status e=gh_om_door_construct_001bbeb8(data,n,out);if(e)return e;uint8_t*p=data;put(p,0x214,0x427690);for(unsigned j=0;j<5;j++)vf0(p,0x650+j*16);p[0x6a4]=0;out->original_vtable214=0x427690;return GH_PLACE_OK;}
static gh_scene_placement_status fail(gh_om354_state*s,uint32_t f,gh_scene_placement_status e){s->frontier=f;return e;}
gh_scene_placement_status gh_om354_initialize_001a8200(gh_om354_state*s,gh_om_object*o,const gh_om_initializer_bindings*b,int*out){if(!s||!o||o->source.type!=0x354||!out)return GH_PLACE_ARGUMENT;*out=0;int original=0;gh_scene_placement_status e=gh_om_common_initialize_001b6fb8(s,o,b,&original);if(e)return e;if(!original)return fail(s,0x1b6fb8,GH_PLACE_UNSUPPORTED);uint8_t*p=o->view.bytes;if(s->stage==3){uint16_t hp=1;memcpy(p+0x548,&hp,2);memcpy(p+0x54a,&hp,2);const uint32_t size[3]={0x40200000,0x406ccccd,0x3e4ccccd};memcpy(p+0x610,size,12);s->stage=4;}
 if(s->stage==4){if(!b->dynamic||!b->door_shared)return fail(s,0x1bd228,GH_PLACE_UNSUPPORTED);e=gh_om_door_geometry_001bd228(&s->geometry_state,o,b->dynamic,b->door_shared,GH_STATIC_BINARY32_REFERENCE);if(e)return fail(s,s->geometry_state.frontier,e);s->stage=5;}
 if(s->stage==5){p[0x624]=1;put(p,0x628,0);s->door_segments=p+0x650;for(unsigned i=0;i<s->model.script.mesh_count;i++){const gh_actor_node_packets*packet;if(gh_actor_model_packet_at(&s->model,i,&packet))return fail(s,0x14b6f8,GH_PLACE_BOUNDS);uint8_t*n=packet->node;if(n[0x40c]==0)put(n,0x380,w(n+0x380)&~1u);else if(n[0x40c]==0x40)put(n,0x380,w(n+0x380)|1u);}put(p,0x5b8,w(p+0x5b8)|2u);s->stage=6;}
 s->frontier=0;*out=1;return GH_PLACE_OK;}
gh_scene_placement_status gh_om354_abort(gh_om354_state*s){return gh_om_initialize_abort(s);}
