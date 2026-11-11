#include "game_actor_model_material.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static float number(const void*p){float n;memcpy(&n,p,4);return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float divf(float a,float b){volatile float n=a/b;return n;}
static gh_actor_model_status fail(uint32_t*f,uint32_t pc,gh_actor_model_status e){*f=pc;return e;}
static gh_actor_model_status prepare(gh_actor_model *model,unsigned index,gh_stage_textures *textures,
 int existing,gh_actor_materials *out,uint32_t *frontier){
 const uint8_t *node;size_t mesh;const gh_actor_node_packets *packet;
 gh_actor_model_status e=gh_actor_model_node_at(model,index,&node,&mesh);if(e)return e;
 e=gh_actor_model_packet_at(model,index,&packet);if(e)return e;
 if(!packet->ready||packet->node!=node||packet->stream_count>16||!packet->allocation.pointer||packet->allocation.owner!=packet->heap)return fail(frontier,0x148690,GH_ACTOR_MODEL_BOUNDS);
 const uint8_t *root=model->root.bytes;uint32_t flags=word(node+0x380),work608=word((uint8_t*)model->work+0x608);
 if((flags&0x40000000)&&work608)return fail(frontier,0x148bd8,GH_ACTOR_MODEL_UNSUPPORTED);
 if(!(flags&0x4000000)&&!(flags&0x8000000)&&(flags&0x2000000))return fail(frontier,0x31cdc0,GH_ACTOR_MODEL_UNSUPPORTED);
 /* Nonzero signed387 biases CBP bits37..50. The cache decoder currently
  * resolves the source palette only; reject rather than render wrong colors. */
 if(node[0x387])return fail(frontier,0x148690,GH_ACTOR_MODEL_UNSUPPORTED);
 uint32_t mode=word(root+0x2c4),tfx=0;
 float alpha=number(root+0x24c);alpha=mul(alpha,divf((float)root[0x2b6],255));alpha=mul(alpha,divf((float)root[0x2b7],255));alpha=mul(alpha,divf((float)root[0x2bc],255));
 if(!isfinite(alpha))return fail(frontier,0x14b4d0,GH_ACTOR_MODEL_UNSUPPORTED);
 if((flags&0x20000000)&&alpha==1)tfx=3;
 gh_actor_materials next={.model=model,.textures=textures,.node_index=index,.count=packet->stream_count,.parity=word((uint8_t*)model->work+0x210)&1u};
 for(size_t i=0;i<packet->stream_count;i++) {
  const gh_actor_packet_stream *stream=packet->streams+i;unsigned source=stream->input.material,member=source;
  if(word(root+0x254)&0x10000000u){if(model->root.size<0x2d4+256)return fail(frontier,0x148690,GH_ACTOR_MODEL_BOUNDS);uint8_t replacement=root[0x2d4+source];if(replacement!=255&&!(word(node+0x410)&0x40000000))member=replacement;}
  if(flags&4)member=0;
  next.streams[i].source_member=(uint8_t)source;next.streams[i].selected_member=(uint8_t)member;
  if(member==255){
   /* 154DA0 wrote TEX0 from 747BB8 + (node384 + raw slot) * B0.
    * 148690 skips replacing FF. The unloaded FF entry is zero-filled,
    * so this is a PSMCT32 1x1/TCC0 sample from display page base zero. */
   gh_stage_channel constructor_slot;
   if(source!=255||stream->material_slot!=255||node[0x384]||
      gh_stage_texture_channel(textures,255,&constructor_slot)!=GH_STAGE_UNAVAILABLE)
    return fail(frontier,0x154da0,GH_ACTOR_MODEL_UNSUPPORTED);
   next.streams[i].samples_display_base0=1;
   continue;
  }
  const gh_stage_material *material=NULL;
  gh_stage_status status=existing?
   gh_stage_tm3_cached(textures,packet->texture.bytes,packet->texture.size,member,mode,&material):
   gh_stage_tm3_resolve(textures,packet->texture.bytes,packet->texture.size,member,mode,&material);
  if(status)return fail(frontier,status==GH_STAGE_BOUNDS?0x1522c0:0x151398,status==GH_STAGE_MEMORY?GH_ACTOR_MODEL_MEMORY:status==GH_STAGE_BOUNDS?GH_ACTOR_MODEL_BOUNDS:GH_ACTOR_MODEL_UNSUPPORTED);
  uint64_t tex0=(material->source_tex0&UINT64_C(0xfff80007ffffffff))|UINT64_C(0x2000000400000000)|(((material->source_tex0>>37)&0x3fff)<<37)|((uint64_t)tfx<<35);
  next.streams[i]=(gh_actor_stream_material){material,tex0,(uint8_t)source,(uint8_t)member,1,0};
  size_t offset=stream->frame_offset[next.parity];if(offset>packet->bytes||0x28>packet->bytes-offset)return fail(frontier,0x148690,GH_ACTOR_MODEL_BOUNDS);
  if(existing){
   uint64_t stored;
   memcpy(&stored,(const uint8_t*)packet->allocation.pointer+offset+0x20,8);
   if(stored!=tex0)return fail(frontier,0x14b9a0,GH_ACTOR_MODEL_UNSUPPORTED);
  }
 }
 /* Preflight all streams before publishing; no half-ready returned view.
  * Side effects in the unique texture cache are ordinary deduplicated loads. */
 if(!existing)for(size_t i=0;i<next.count;i++)if(next.streams[i].written)memcpy((uint8_t*)packet->allocation.pointer+packet->streams[i].frame_offset[next.parity]+0x20,&next.streams[i].tex0,8);
 next.ready=1;*out=next;return GH_ACTOR_MODEL_OK;
}
static gh_actor_model_status materials(gh_actor_model *model,unsigned index,gh_stage_textures *textures,
 int existing,gh_actor_materials *out,uint32_t *frontier){
 uint32_t local=0;if(!frontier)frontier=&local;*frontier=0;if(!model||!model->ready||model->native_identity!=model||!model->root.bytes||!model->work||!textures||!out)return GH_ACTOR_MODEL_ARGUMENT;
 fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}
 gh_actor_model_status e=prepare(model,index,textures,existing,out,frontier);if(fesetenv(&env))return GH_ACTOR_MODEL_UNSUPPORTED;return e;
}
gh_actor_model_status gh_actor_materials_0014b4d0(gh_actor_model *model,unsigned index,
 gh_stage_textures *textures,gh_actor_materials *out,uint32_t *frontier){
 return materials(model,index,textures,0,out,frontier);
}
gh_actor_model_status gh_actor_materials_existing_0014b9a0(gh_actor_model *model,unsigned index,
 gh_stage_textures *textures,gh_actor_materials *out,uint32_t *frontier){
 return materials(model,index,textures,1,out,frontier);
}
