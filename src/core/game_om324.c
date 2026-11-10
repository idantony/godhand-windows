/*Actual324198CA8/198D40->1BFAC8/1BFB28, notsegmentclass.*/
#include "game_om324.h"
#include "game_om_base.h"
#include "game_actor_model_nodes.h"
#include <string.h>
#include <fenv.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}static void half(uint8_t*p,size_t o,uint16_t n){memcpy(p+o,&n,2);}
gh_scene_placement_status gh_om324_construct_00198ca8(void*data,size_t n,gh_title_player_view*out){if(!data||!out)return GH_PLACE_ARGUMENT;if(n<0x610)return GH_PLACE_BOUNDS;gh_scene_placement_status e=gh_om_base_construct_001b6e90(data,n,out);if(e)return e;uint8_t*p=data;put(p,0x214,0x4297d8);put(p,0x214,0x425000);out->original_vtable214=0x425000;return GH_PLACE_OK;}
static gh_scene_placement_status fail(gh_om324_state*s,uint32_t f,gh_scene_placement_status e){s->frontier=f;return e;}
gh_scene_placement_status gh_om324_initialize_00198d40(gh_om324_state*s,gh_om_object*o,const gh_om_initializer_bindings*b,int*out){if(!s||!o||o->source.type!=0x324||!out)return GH_PLACE_ARGUMENT;*out=0;s->frontier=0;int common_return=0;gh_scene_placement_status e=gh_om_common_initialize_001b6fb8(&s->common,o,b,&common_return);if(e)return fail(s,s->common.frontier,e);if(!common_return)return fail(s,0x1b6fb8,GH_PLACE_UNSUPPORTED);uint8_t*p=o->view.bytes;
 if(s->stage==0){if(!b->init)return fail(s,0x1fe218,GH_PLACE_UNSUPPORTED);uint32_t frontier=0;int original;gh_hitarea_status he=gh_hitarea_object_create_001fe218(b->init,&o->view,2,&s->common.hitarea,&original,&frontier);if(he)return fail(s,frontier?frontier:0x1fe218,GH_PLACE_UNSUPPORTED);s->common.hitarea_created=(uint8_t)original;put(p,0x600,0);s->stage=1;}
 if(s->stage==1){if(s->common.hitarea_created){if(!b->shapes)return fail(s,0x1fd9d8,GH_PLACE_UNSUPPORTED);const uint32_t rotation[3]={0},offset[3]={0,0xbf19999a,0},size[3]={0x40000000,0x3f99999a,0x3e4ccccd};uint32_t frontier=0;int index;gh_hitarea_status he=gh_hitarea_add_box_001fd9d8(&s->common.hitarea,b->shapes,p+0x80,offset,rotation,size,&index,&frontier);if(he)return fail(s,frontier?frontier:0x1fd9d8,GH_PLACE_UNSUPPORTED);}s->stage=2;}
 if(s->stage==2){uint32_t values[3],frontier=0;gh_scene_placement_status scale_status=gh_om324_scale_reference(o->source.bytes[0x2c],values,&frontier);if(scale_status)return fail(s,frontier,scale_status);half(p,0x548,1);half(p,0x54a,1);memcpy(p+0x110,values,12);const gh_actor_node_packets*first;if(gh_actor_model_packet_at(&s->common.model,0,&first))return fail(s,0x14b6f8,GH_PLACE_BOUNDS);put(first->node,0x380,w(first->node+0x380)&~1u);if(s->common.hitarea_created)put(s->common.hitarea.bytes,0,w(s->common.hitarea.bytes)|1u);s->stage=5;}
 *out=1;return GH_PLACE_OK;}
gh_scene_placement_status gh_om324_abort(gh_om324_state*s){return gh_om303_abort(s);}

/* Source dimensions vary; the collision box remains2/1.2/.2. Reference
 * policy is explicit and does not claim EE hardware division bit accuracy. */
gh_scene_placement_status gh_om324_scale_reference(uint8_t selector,uint32_t out[3],uint32_t*f){if(!out||!f)return GH_PLACE_ARGUMENT;*f=0;uint32_t numerator[3]={0x40000000,0x3f99999a,0x3e4ccccd},denominator[3]={0x40000000,0x3f99999a,0x3e4ccccd};if(selector==1){numerator[0]=0x3f800000;numerator[1]=0x3fb33333;}else if(selector==2){numerator[0]=numerator[1]=0x3f666666;}fenv_t saved;if(feholdexcept(&saved)){*f=0x198ecc;return GH_PLACE_UNSUPPORTED;}if(fesetround(FE_TONEAREST)){fesetenv(&saved);*f=0x198ecc;return GH_PLACE_UNSUPPORTED;}uint32_t values[3];for(unsigned k=0;k<3;k++){float a,b;memcpy(&a,numerator+k,4);memcpy(&b,denominator+k,4);volatile float value=a/b;float result=value;memcpy(values+k,&result,4);}if(fesetenv(&saved)){*f=0x198ecc;return GH_PLACE_UNSUPPORTED;}memcpy(out,values,12);return GH_PLACE_OK;}
