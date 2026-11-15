/*Actual30918B290/18B378,base1C7D98 (not1C27E8).*/
#include "game_om309.h"
#include "game_om_base.h"
#include "game_actor_model_nodes.h"
#include <string.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}static void half(uint8_t*p,size_t o,uint16_t n){memcpy(p+o,&n,2);}static void vf0(uint8_t*p,size_t o){memset(p+o,0,12);put(p,o+12,0x3f800000);}
gh_scene_placement_status gh_om309_construct_0018b290(void*data,size_t n,gh_title_player_view*out){if(!data||!out)return GH_PLACE_ARGUMENT;if(n<0x9c0)return GH_PLACE_BOUNDS;gh_scene_placement_status e=gh_om_pushable_construct_001c7d98(data,n,out);if(e)return e;uint8_t*p=data;put(p,0x214,0x4237e8);for(unsigned i=0;i<9;i++){size_t o=0x660+(size_t)i*0x60;for(unsigned j=0;j<5;j++)vf0(p,o+j*16);p[o+0x54]=0;}out->original_vtable214=0x4237e8;return GH_PLACE_OK;}
/* Original 313DE0 -> 18A598, base 1B6E90 with eight VF0 records. */
gh_scene_placement_status gh_om308_construct_0018a598(void *data,size_t size,gh_title_player_view *out)
{
 if(!data||!out)return GH_PLACE_ARGUMENT;
 if(size<0x910)return GH_PLACE_BOUNDS;
 gh_scene_placement_status e=gh_om_base_construct_001b6e90(data,size,out);
 if(e)return e;
 uint8_t *p=data;
 put(p,0x214,0x423700);
 for(unsigned i=0;i<8;++i){
  const size_t offset=0x610+(size_t)i*0x60;
  for(unsigned j=0;j<5;++j)vf0(p,offset+j*16);
  p[offset+0x54]=0;
 }
 out->original_vtable214=0x423700;
 return GH_PLACE_OK;
}

static gh_scene_placement_status construct_layered_fragments(void *data,size_t size,
 gh_title_player_view *out,uint32_t vtable)
{
 if(!data||!out)return GH_PLACE_ARGUMENT;
 if(size<0x8a0)return GH_PLACE_BOUNDS;
 gh_scene_placement_status e=gh_om_pushable_construct_001c7d98(data,size,out);
 if(e)return e;
 uint8_t *p=data;
 put(p,0x214,vtable);
 for(unsigned i=0;i<6;++i){
  const size_t offset=0x660+(size_t)i*0x60;
  for(unsigned j=0;j<5;++j)vf0(p,offset+j*16);
  p[offset+0x54]=0;
 }
 out->original_vtable214=vtable;
 return GH_PLACE_OK;
}

gh_scene_placement_status gh_om30b_construct_0018cea0(void *data,size_t size,gh_title_player_view *out)
{
 return construct_layered_fragments(data,size,out,0x4239c8);
}

gh_scene_placement_status gh_om30c_construct_0018dc58(void *data,size_t size,gh_title_player_view *out)
{
 return construct_layered_fragments(data,size,out,0x423ab8);
}

gh_scene_placement_status gh_om331_construct_0019c588(void *data,size_t size,gh_title_player_view *out)
{
 return construct_layered_fragments(data,size,out,0x425c78);
}

static gh_scene_placement_status fail(gh_om309_state*s,uint32_t f,gh_scene_placement_status e){s->frontier=f;return e;}
static gh_scene_placement_status initialize_fragment_cylinder(gh_om309_state*s,gh_om_object*o,
 const gh_om_initializer_bindings*b,int*out,unsigned kind,size_t hitarea_offset,size_t segment_offset)
{
 if(!s||!o||o->source.type!=kind||!out)return GH_PLACE_ARGUMENT;*out=0;s->frontier=0;int common_return=0;gh_scene_placement_status e=gh_om_common_initialize_001b6fb8(&s->common,o,b,&common_return);if(e)return fail(s,s->common.frontier,e);if(!common_return)return fail(s,0x1b6fb8,GH_PLACE_UNSUPPORTED);uint8_t*p=o->view.bytes;
 if(s->stage==0){if(!b->init)return fail(s,0x1fe218,GH_PLACE_UNSUPPORTED);uint32_t frontier=0;int original;gh_hitarea_status he=gh_hitarea_object_create_001fe218(b->init,&o->view,2,&s->common.hitarea,&original,&frontier);if(he)return fail(s,frontier?frontier:0x1fe218,GH_PLACE_UNSUPPORTED);s->common.hitarea_created=(uint8_t)original;put(p,hitarea_offset,0);s->stage=1;}
 if(s->stage==1){if(s->common.hitarea_created){if(!b->shapes)return fail(s,0x1fdc00,GH_PLACE_UNSUPPORTED);const uint32_t zero[3]={0};uint32_t frontier=0;int index;gh_hitarea_status he=gh_hitarea_add_cylinder_001fdc00(&s->common.hitarea,b->shapes,p+0x80,zero,zero,0x3fa4dd2f,0x3f028f5c,&index,&frontier);if(he)return fail(s,frontier?frontier:0x1fdc00,GH_PLACE_UNSUPPORTED);}s->stage=2;}
 if(s->stage==2){
  half(p,0x548,1);half(p,0x54a,1);
  gh_actor_model *model=&s->common.model;
  const int layered=kind==0x30b||kind==0x30c||kind==0x331;
  if(layered){
   for(unsigned i=0;i<model->script.mesh_count;++i){
    const gh_actor_node_packets *node;
    if(gh_actor_model_packet_at(model,i,&node))return fail(s,0x14b6f8,GH_PLACE_BOUNDS);
    const unsigned layer=node->node[0x40c];
    if(layer==0)put(node->node,0x380,w(node->node+0x380)&~1u);
    if(layer==0x40)put(node->node,0x380,w(node->node+0x380)|1u);
   }
  }else{
   const gh_actor_node_packets *first,*second;
   const unsigned index=kind==0x308||model->script.mesh_count>1?1:0;
   if(gh_actor_model_packet_at(model,0,&first)||gh_actor_model_packet_at(model,index,&second))
    return fail(s,0x14b6f8,GH_PLACE_BOUNDS);
   put(first->node,0x380,w(first->node+0x380)|1u);
   put(second->node,0x380,w(second->node+0x380)&~1u);
  }
  const unsigned limit=layered?6:9;
  for(unsigned i=1;i<limit&&i<model->part_count;++i){
   const gh_actor_part *part;
   if(gh_actor_model_part_at(model,i,&part))return fail(s,0x14bfe0,GH_PLACE_BOUNDS);
   put(part->bytes,0x154,w(part->bytes+0x154)|0x18u);
  }
  s->segments=p+segment_offset;s->stage=5;
 }
 *out=1;return GH_PLACE_OK;}
gh_scene_placement_status gh_om308_initialize_0018a680(gh_om309_state *s,gh_om_object *o,
 const gh_om_initializer_bindings *b,int *out)
{
 /* Original cylinder uses height 0x3fa4dd2f and radius 1.02/2. */
 return initialize_fragment_cylinder(s,o,b,out,0x308,0x600,0x610);
}

gh_scene_placement_status gh_om309_initialize_0018b378(gh_om309_state *s,gh_om_object *o,
 const gh_om_initializer_bindings *b,int *out)
{
 return initialize_fragment_cylinder(s,o,b,out,0x309,0x650,0x660);
}

gh_scene_placement_status gh_om30b_initialize_0018cf88(gh_om309_state *s,gh_om_object *o,
 const gh_om_initializer_bindings *b,int *out)
{
 return initialize_fragment_cylinder(s,o,b,out,0x30b,0x650,0x660);
}

gh_scene_placement_status gh_om30c_initialize_0018dd40(gh_om309_state *s,gh_om_object *o,
 const gh_om_initializer_bindings *b,int *out)
{
 return initialize_fragment_cylinder(s,o,b,out,0x30c,0x650,0x660);
}

gh_scene_placement_status gh_om331_initialize_0019c670(gh_om309_state *s,gh_om_object *o,
 const gh_om_initializer_bindings *b,int *out)
{
 return initialize_fragment_cylinder(s,o,b,out,0x331,0x650,0x660);
}

gh_scene_placement_status gh_om309_abort(gh_om309_state*s){return gh_om303_abort(s);}
