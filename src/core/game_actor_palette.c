/* Original 1503B8 and 14C7E8's ordered palette suffix. Actual patched ELF;
 * upstream MIT pin eaae18d0dfab1f9a0df276ae5ca1581ad326faf3. */
#include "game_actor_palette.h"
#include "game_actor_matrix.h"
#include <string.h>

gh_actor_model_status gh_actor_part_palette_001503b8(const gh_actor_part *part,
 gh_static_transform_policy policy,float out[16],uint32_t *frontier)
{
 if(!part||!part->bytes||!out||!frontier)return GH_ACTOR_MODEL_ARGUMENT;
 *frontier=0;
 float world[16],translation[16]={0},result[16];
 for(unsigned i=0;i<4;i++)translation[i*5]=1;
 memcpy(world,part->bytes+0x80,sizeof(world));
 memcpy(translation+12,part->bytes+0xc0,12);
 gh_actor_matrix_status e=gh_actor_matrix_product_00147548(world,translation,result,policy,frontier);
 if(e)return e==GH_ACTOR_MATRIX_ARGUMENT?GH_ACTOR_MODEL_ARGUMENT:GH_ACTOR_MODEL_UNSUPPORTED;
 memcpy(out,result,sizeof(result));return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_actor_palette_from_parts(const gh_actor_model *model,
 gh_static_transform_policy policy,float (*out)[16],size_t capacity,size_t *count,
 uint32_t *frontier)
{
 if(!model||!model->ready||!model->parts_ready||!out||!count||!frontier)return GH_ACTOR_MODEL_ARGUMENT;
 *frontier=0;
 const gh_actor_object_packets *packet=NULL;
 if(gh_actor_model_object_packet(model,&packet)||!packet||!packet->ready||packet->parts!=model->parts||
    packet->part_count!=model->part_count)return GH_ACTOR_MODEL_ARGUMENT;
 if(!model->part_count||model->part_count>42||capacity<model->part_count)return GH_ACTOR_MODEL_BOUNDS;
 float pending[42][16];
 for(unsigned i=0;i<model->part_count;i++){
  gh_actor_model_status e=gh_actor_part_palette_001503b8(model->parts+i,policy,pending[i],frontier);
  if(e)return e;
 }
 memcpy(out,pending,(size_t)model->part_count*sizeof(pending[0]));
 *count=model->part_count;return GH_ACTOR_MODEL_OK;
}
