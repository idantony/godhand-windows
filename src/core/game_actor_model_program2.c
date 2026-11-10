#include "game_actor_model_program2.h"
#include <string.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
gh_actor_model_status gh_actor_model_program2_inputs(gh_actor_model *model,unsigned index,gh_skinned_program2_inputs *out,uint32_t *frontier){
 if(frontier)*frontier=0;if(!model||!out||!model->ready||model->native_identity!=model||!model->work||!model->part_count||model->part_count>42)return GH_ACTOR_MODEL_ARGUMENT;
 const gh_actor_object_packets *object;const uint8_t *node;size_t mesh;gh_actor_model_status e=gh_actor_model_object_packet(model,&object);if(e)return e;e=gh_actor_model_node_at(model,index,&node,&mesh);if(e)return e;
 unsigned parity=word((uint8_t*)model->work+0x210)&1u;uint8_t *constants,*palette;
 e=gh_actor_model_object_span(model,object->constants_offset[parity],0x150,&constants);if(e)return e;
 e=gh_actor_model_object_span(model,object->palette_offset[parity]+16,model->part_count*64,&palette);if(e)return e;
 const uint8_t *light=node+0x160+parity*0xd0;gh_skinned_program2_inputs next={.palette=palette,.palette_bytes=model->part_count*64,.part_count=model->part_count};
 memcpy(next.screen,constants+0x10,64);memcpy(next.clip,constants+0x50,64);memcpy(next.normal_light,light+0x10,64);memcpy(next.light_color,light+0x50,64);memcpy(next.uv,light+0xc0,8);memcpy(&next.fog,light+0x2c,4);*out=next;return GH_ACTOR_MODEL_OK;
}
