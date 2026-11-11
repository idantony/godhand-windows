#include "game_scene_meshes.h"
#include "game_stage.h"
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static gh_scene_mesh_status fail(gh_scene_meshes*s,uint32_t f,int d,gh_scene_mesh_status e){s->frontier=f;s->detail=d;return e;}
gh_scene_mesh_status gh_scene_meshes_material_00148d30(gh_scene_meshes*s,size_t index,unsigned sub,
 gh_stage_textures*textures,const gh_stage_material**out){
 if(!s||!textures||!out)return GH_MESH_ARGUMENT;*out=NULL;
 if(index>=s->count)return GH_MESH_BOUNDS;
 gh_scene_mesh_object*o=index?(s->remaining_objects?&s->remaining_objects[index-1]:NULL):&s->first;
 if(!o||!o->constructor_complete)return fail(s,0x148d30,0,GH_MESH_BUSY);
 if(sub>=o->descriptor.submesh_count||sub>=16)return GH_MESH_BOUNDS;
 gh_scene_mesh_material*material=&o->materials[sub];
 if(material->owner!=textures)return fail(s,0x148d30,0,GH_MESH_ARGUMENT);
 if(!s->tables||!s->tables->scroll_texture.bytes)return fail(s,0x148d30,0,GH_MESH_UNSUPPORTED);
 if(o->descriptor.node_flags&0x2000000u)return fail(s,0x31cdc0,0,GH_MESH_UNSUPPORTED);
 gh_scene_mesh_stream stream;gh_scene_mesh_status e=gh_scene_mesh_stream_at(&o->descriptor,sub,&stream);if(e)return e;
 /* 148D30 retains the per-stream TEX0_1 that 155678 initialized.
  * TEX0 zero is the textured 1x1 display page at physical base zero. */
 if(stream.material==255){
  const uint8_t *node=o->node;
  if(!node||node[0x384]||(u32(node+0x380)&0x8800u)||
     !material->initial_resolved||material->initial_tex0||material->tm3_override_required)
   return fail(s,0x155678,255,GH_MESH_UNSUPPORTED);
  material->consumer_visits++;s->frontier=0;s->detail=0;
  return GH_MESH_OK;
 }
 const gh_stage_material*decoded;gh_stage_status se=gh_stage_tm3_resolve(textures,s->tables->scroll_texture.bytes,
  s->tables->scroll_texture.size,stream.material,u32((uint8_t*)o->allocation.pointer+0x2c4),&decoded);
 if(se)return fail(s,0x151398,se,se==GH_STAGE_BOUNDS?GH_MESH_BOUNDS:se==GH_STAGE_MEMORY?GH_MESH_MEMORY:GH_MESH_UNSUPPORTED);
 material->decoded=decoded;material->consumer_visits++;*out=decoded;s->frontier=0;s->detail=0;
 /* render_ready deliberately remainsfalse: decoding is not GPU execution. */
 return GH_MESH_OK;
}
