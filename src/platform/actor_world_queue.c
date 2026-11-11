#include "actor_world_queue.h"
#include <stdlib.h>
#include <string.h>
/* Explicit host ceilings, not recovered game heap capacities. */
enum {MAX_GROUPS=4096,MAX_PACKETS=65536};
typedef struct {gh_actor_world_packet view;gh_static_gs_vertex *vertices;uint32_t *indices;} packet;
typedef struct {gh_actor_world_group view;packet *packets;} group;
struct gh_actor_world_queue {group *groups;size_t count,packets;};
static gh_actor_world_status fail(gh_static_transform_frontier *f,uint32_t pc,gh_actor_world_status e){if(f){f->ee_entry=pc;f->vu_byte_pc=0;}return e;}
static void release(group *g){for(size_t i=0;i<g->view.packets;i++){free(g->packets[i].vertices);free(g->packets[i].indices);}free(g->packets);memset(g,0,sizeof(*g));}
gh_actor_world_status gh_actor_world_queue_create(gh_actor_world_queue **out){if(!out)return GH_ACTOR_WORLD_ARGUMENT;*out=NULL;gh_actor_world_queue *q=calloc(1,sizeof(*q));if(!q)return GH_ACTOR_WORLD_MEMORY;*out=q;return GH_ACTOR_WORLD_OK;}
void gh_actor_world_queue_reset(gh_actor_world_queue *q){if(q){for(size_t i=0;i<q->count;i++)release(q->groups+i);free(q->groups);memset(q,0,sizeof(*q));}}
void gh_actor_world_queue_destroy(gh_actor_world_queue *q){if(q){gh_actor_world_queue_reset(q);free(q);}}
size_t gh_actor_world_queue_count(const gh_actor_world_queue *q){return q?q->count:0;}
gh_actor_world_status gh_actor_world_queue_group_at(const gh_actor_world_queue *q,size_t i,gh_actor_world_group *out){if(!q||!out)return GH_ACTOR_WORLD_ARGUMENT;if(i>=q->count)return GH_ACTOR_WORLD_BOUNDS;*out=q->groups[i].view;return GH_ACTOR_WORLD_OK;}
gh_actor_world_status gh_actor_world_queue_packet_at(const gh_actor_world_queue *q,size_t i,size_t p,gh_actor_world_packet *out){if(!q||!out)return GH_ACTOR_WORLD_ARGUMENT;if(i>=q->count||p>=q->groups[i].view.packets)return GH_ACTOR_WORLD_BOUNDS;*out=q->groups[i].packets[p].view;return GH_ACTOR_WORLD_OK;}
static gh_actor_world_status push(group *g,const gh_static_gs_vertex *vertices,const uint32_t *indices,const gh_static_geometry_result *result,const gh_actor_stream_material *material,size_t stream,size_t batch){
 if(result->vertices>26||result->triangles>24||g->view.packets==MAX_PACKETS)return GH_ACTOR_WORLD_BOUNDS;
 for(size_t i=0;i<result->triangles*3;i++)if(indices[i]>=result->vertices)return GH_ACTOR_WORLD_BOUNDS;
 packet next={0};next.vertices=malloc((result->vertices?result->vertices:1)*sizeof(*vertices));next.indices=malloc((result->triangles?result->triangles*3:1)*sizeof(*indices));if(!next.vertices||!next.indices){free(next.vertices);free(next.indices);return GH_ACTOR_WORLD_MEMORY;}
 memcpy(next.vertices,vertices,result->vertices*sizeof(*vertices));memcpy(next.indices,indices,result->triangles*3*sizeof(*indices));next.view=(gh_actor_world_packet){next.vertices,result->vertices,next.indices,result->triangles,result->primitive,material->material,material->tex0,stream,batch,material->samples_display_base0};
 packet *grown=realloc(g->packets,(g->view.packets+1)*sizeof(*grown));if(!grown){free(next.vertices);free(next.indices);return GH_ACTOR_WORLD_MEMORY;}g->packets=grown;g->packets[g->view.packets++]=next;return GH_ACTOR_WORLD_OK;
}
gh_actor_world_status gh_actor_world_queue_append(gh_actor_world_queue *q,const gh_actor_draw_command *command,size_t *index,gh_static_transform_frontier *frontier){
 if(frontier)*frontier=(gh_static_transform_frontier){0};if(!q||!command||!index||!frontier||!command->model)return GH_ACTOR_WORLD_ARGUMENT;
 gh_actor_model *model=command->model;if(!model->ready||model->native_identity!=model||!command->packet_group||command->parity>1||!command->depth||(command->bank!=1&&command->bank!=2))return GH_ACTOR_WORLD_ARGUMENT;
 if(q->count==MAX_GROUPS)return GH_ACTOR_WORLD_BOUNDS;
 group next={0};next.view.command=*command;gh_actor_world_status status=GH_ACTOR_WORLD_OK;
 if(command->kind==GH_ACTOR_DRAW_SETUP){
  const gh_actor_object_packets *object;if(gh_actor_model_object_packet(model,&object)||!object->ready||command->object!=object||command->node_index!=SIZE_MAX)return GH_ACTOR_WORLD_ARGUMENT;
 }else if(command->kind==GH_ACTOR_DRAW_GEOMETRY){
  if(command->selected_program!=2&&command->selected_program!=30)return fail(frontier,0x155ae8,GH_ACTOR_WORLD_UNSUPPORTED);
  if(command->node_index>UINT32_MAX)return GH_ACTOR_WORLD_BOUNDS;
  const gh_actor_node_packets *packet;if(gh_actor_model_packet_at(model,(unsigned)command->node_index,&packet)||packet!=command->packet||!packet->ready)return GH_ACTOR_WORLD_ARGUMENT;
  uint32_t node_flags;memcpy(&node_flags,packet->node+0x380,4);if(command->selected_program==30&&!(node_flags&0x20000000u))return fail(frontier,0x155ae8,GH_ACTOR_WORLD_UNSUPPORTED);
  const gh_actor_materials *materials=&command->materials;
  if(!materials->ready||materials->model!=model||materials->node_index!=command->node_index||materials->parity!=command->parity||!materials->textures||materials->count!=packet->stream_count||!materials->count||materials->count>16)return fail(frontier,0x151368,GH_ACTOR_WORLD_UNSUPPORTED);
  const gh_actor_object_packets *object;uint8_t *palette;if(gh_actor_model_object_packet(model,&object)||gh_actor_model_object_span(model,object->palette_offset[command->parity]+16,model->part_count*64,&palette))return GH_ACTOR_WORLD_BOUNDS;
  if(command->inputs.palette!=palette||command->inputs.part_count!=model->part_count||command->inputs.palette_bytes!=model->part_count*64)return GH_ACTOR_WORLD_ARGUMENT;
  for(size_t stream=0;stream<packet->stream_count;stream++){
   const gh_actor_stream_material *material=materials->streams+stream;const gh_stage_material *image=material->material;
   const gh_actor_packet_stream *source=packet->streams+stream;
   if(material->samples_display_base0){
    if(material->written||image||material->tex0||source->input.material!=255||
       source->material_slot!=255||material->source_member!=255||material->selected_member!=255||
       packet->node[0x384]||command->selected_program!=2||command->clamp){
     status=fail(frontier,0x154da0,GH_ACTOR_WORLD_UNSUPPORTED);goto failed;
    }
   }else{
    if(!material->written||!image){status=fail(frontier,0x151368,GH_ACTOR_WORLD_UNSUPPORTED);goto failed;}
    if(((material->tex0>>35)&3)!=(command->selected_program==30?3u:0u)){status=fail(frontier,0x148690,GH_ACTOR_WORLD_UNSUPPORTED);goto failed;}
    if(!((material->tex0>>34)&1)||!image->rgba_gs||!image->width||!image->height||image->rgba_bytes!=(size_t)image->width*image->height*4){status=fail(frontier,0x151398,GH_ACTOR_WORLD_UNSUPPORTED);goto failed;}
   }
   size_t batches;if(gh_skinned_mesh_batch_count(&source->input,&batches)||batches!=source->batches){status=GH_ACTOR_WORLD_BOUNDS;goto failed;}
   for(size_t batch=0;batch<batches;batch++){
    gh_static_gs_vertex vertices[26];uint32_t indices[72];gh_static_geometry_result result;
    gh_static_mesh_status e;
    if(command->selected_program==30){gh_skinned_program30_inputs highlighted={.base=command->inputs,.squarings=command->specular_squarings};memcpy(highlighted.half_vector,command->specular_half,12);e=gh_skinned_transform_program30_batch(&source->input,batch,&highlighted,GH_STATIC_BINARY32_REFERENCE,vertices,26,indices,72,&result,frontier);}
    else e=gh_skinned_transform_program2_batch(&source->input,batch,&command->inputs,GH_STATIC_BINARY32_REFERENCE,vertices,26,indices,72,&result,frontier);
    if(e){status=e==GH_STATIC_UNSUPPORTED?GH_ACTOR_WORLD_UNSUPPORTED:GH_ACTOR_WORLD_BOUNDS;goto failed;}
    if(material->samples_display_base0&&result.primitive!=0x7c&&result.primitive!=0x7d){
     status=fail(frontier,0x154e98,GH_ACTOR_WORLD_UNSUPPORTED);goto failed;
    }
    status=push(&next,vertices,indices,&result,material,stream,batch);if(status)goto failed;
   }
  }
 }else return GH_ACTOR_WORLD_ARGUMENT;
 if(next.view.packets>MAX_PACKETS-q->packets){status=GH_ACTOR_WORLD_BOUNDS;goto failed;}
 group *grown=realloc(q->groups,(q->count+1)*sizeof(*grown));if(!grown){status=GH_ACTOR_WORLD_MEMORY;goto failed;}q->groups=grown;q->groups[q->count]=next;*index=q->count++;q->packets+=next.view.packets;return GH_ACTOR_WORLD_OK;
failed:release(&next);return status;
}
