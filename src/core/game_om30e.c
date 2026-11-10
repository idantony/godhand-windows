/*Actual30E18EC98/18ED30->1BFAC8/1BFB28, notsegmentclass.*/
#include "game_om30e.h"
#include "game_om_base.h"
#include "game_actor_model_nodes.h"
#include <string.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}static void half(uint8_t*p,size_t o,uint16_t n){memcpy(p+o,&n,2);}
gh_scene_placement_status gh_om30e_construct_0018ec98(void*data,size_t n,gh_title_player_view*out){if(!data||!out)return GH_PLACE_ARGUMENT;if(n<0x610)return GH_PLACE_BOUNDS;gh_scene_placement_status e=gh_om_base_construct_001b6e90(data,n,out);if(e)return e;uint8_t*p=data;put(p,0x214,0x4297d8);put(p,0x214,0x423c70);out->original_vtable214=0x423c70;return GH_PLACE_OK;}
static gh_scene_placement_status fail(gh_om30e_state*s,uint32_t f,gh_scene_placement_status e){s->frontier=f;return e;}
gh_scene_placement_status gh_om30e_initialize_0018ed30(gh_om30e_state*s,gh_om_object*o,const gh_om_initializer_bindings*b,int*out){if(!s||!o||o->source.type!=0x30e||!out)return GH_PLACE_ARGUMENT;*out=0;s->frontier=0;int common_return=0;gh_scene_placement_status e=gh_om_common_initialize_001b6fb8(&s->common,o,b,&common_return);if(e)return fail(s,s->common.frontier,e);if(!common_return)return fail(s,0x1b6fb8,GH_PLACE_UNSUPPORTED);uint8_t*p=o->view.bytes;
 if(s->stage==0){if(!b->init)return fail(s,0x1fe218,GH_PLACE_UNSUPPORTED);uint32_t frontier=0;int original;gh_hitarea_status he=gh_hitarea_object_create_001fe218(b->init,&o->view,2,&s->common.hitarea,&original,&frontier);if(he)return fail(s,frontier?frontier:0x1fe218,GH_PLACE_UNSUPPORTED);s->common.hitarea_created=(uint8_t)original;put(p,0x600,0);s->stage=1;}
 if(s->stage==1){if(s->common.hitarea_created){if(!b->shapes)return fail(s,0x1fd9d8,GH_PLACE_UNSUPPORTED);const uint32_t rotation[3]={0},offset[3]={0,0xbf800000,0},size[3]={0x3f6809d4,0x40000000,0x3e4ccccd};uint32_t frontier=0;int index;gh_hitarea_status he=gh_hitarea_add_box_001fd9d8(&s->common.hitarea,b->shapes,p+0x80,offset,rotation,size,&index,&frontier);if(he)return fail(s,frontier?frontier:0x1fd9d8,GH_PLACE_UNSUPPORTED);}s->stage=2;}
 if(s->stage==2){half(p,0x548,1);half(p,0x54a,1);const gh_actor_node_packets*first;if(gh_actor_model_packet_at(&s->common.model,0,&first))return fail(s,0x14b6f8,GH_PLACE_BOUNDS);put(first->node,0x380,w(first->node+0x380)&~1u);if(s->common.hitarea_created)put(s->common.hitarea.bytes,0,w(s->common.hitarea.bytes)|1u);s->stage=5;}
 *out=1;return GH_PLACE_OK;}
gh_scene_placement_status gh_om30e_abort(gh_om30e_state*s){return gh_om303_abort(s);}
