#include "game_actor_model_program30.h"
#include <string.h>
static uint32_t word(const void *p){uint32_t n;memcpy(&n,p,4);return n;}
gh_actor_model_status gh_actor_model_program30_inputs(gh_actor_model *model,unsigned index,gh_skinned_program30_inputs *out,uint32_t *frontier){
 if(!out)return GH_ACTOR_MODEL_ARGUMENT;gh_skinned_program30_inputs next={0};gh_actor_model_status e=gh_actor_model_program2_inputs(model,index,&next.base,frontier);if(e)return e;
 const uint8_t *node;size_t mesh;e=gh_actor_model_node_at(model,index,&node,&mesh);if(e)return e;unsigned parity=word((uint8_t*)model->work+0x210)&1u;const uint8_t *constants=node+0x160+parity*0xd0;
 memcpy(next.half_vector,constants+0x90,12);memcpy(&next.squarings,constants+0xb0,2);*out=next;return GH_ACTOR_MODEL_OK;
}
