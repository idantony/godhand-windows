#include "static_world_queue.h"
#include "../core/game_stage.h"
#include <stdlib.h>
#include <string.h>
/* Host memory ceilings, not recovered guest heap capacities. */
enum {MAX_GROUPS=4096,MAX_PACKETS=65536};
typedef struct {gh_static_world_packet view;gh_static_gs_vertex *v;uint32_t *i;} packet;
typedef struct {gh_static_world_group view;packet *packets;} group;
struct gh_static_world_queue {group *groups;size_t count,packets;};
static void release(group*g){for(size_t i=0;i<g->view.packets;i++){free(g->packets[i].v);free(g->packets[i].i);}free(g->packets);memset(g,0,sizeof(*g));}
gh_static_world_status gh_static_world_queue_create(gh_static_world_queue**out){if(!out)return GH_WORLD_ARGUMENT;gh_static_world_queue*q=calloc(1,sizeof(*q));if(!q)return GH_WORLD_MEMORY;*out=q;return GH_WORLD_OK;}
void gh_static_world_queue_reset(gh_static_world_queue*q){if(q){for(size_t i=0;i<q->count;i++)release(q->groups+i);free(q->groups);memset(q,0,sizeof(*q));}}
void gh_static_world_queue_destroy(gh_static_world_queue*q){if(q){gh_static_world_queue_reset(q);free(q);}}
size_t gh_static_world_queue_count(const gh_static_world_queue*q){return q?q->count:0;}
gh_static_world_status gh_static_world_queue_group_at(const gh_static_world_queue*q,size_t i,gh_static_world_group*out){if(!q||!out)return GH_WORLD_ARGUMENT;if(i>=q->count)return GH_WORLD_BOUNDS;*out=q->groups[i].view;return GH_WORLD_OK;}
gh_static_world_status gh_static_world_queue_packet_at(const gh_static_world_queue*q,size_t i,size_t p,gh_static_world_packet*out){if(!q||!out)return GH_WORLD_ARGUMENT;if(i>=q->count||p>=q->groups[i].view.packets)return GH_WORLD_BOUNDS;*out=q->groups[i].packets[p].view;return GH_WORLD_OK;}
static gh_static_world_status push(group*g,const gh_static_gs_vertex*v,const uint32_t*idx,const gh_static_packet_range*r,const gh_object_static_command*c,size_t sub,size_t batch){
 if(g->view.packets==MAX_PACKETS||r->vertices>362||r->triangles>288||sub>=c->material_count)return GH_WORLD_BOUNDS;
 uint8_t display_base0=(c->display_base0_streams>>sub)&1u;
 if(display_base0&&(r->material!=255||!(r->primitive&0x10u)))return GH_WORLD_UNSUPPORTED;
 for(size_t j=0;j<r->triangles*3;j++)if(idx[r->index_offset+j]>=r->vertices)return GH_WORLD_BOUNDS;
 packet p={0};p.v=malloc((r->vertices?r->vertices:1)*sizeof(*p.v));p.i=malloc((r->triangles?r->triangles*3:1)*sizeof(*p.i));if(!p.v||!p.i){free(p.v);free(p.i);return GH_WORLD_MEMORY;}
 memcpy(p.v,v+r->vertex_offset,r->vertices*sizeof(*v));memcpy(p.i,idx+r->index_offset,r->triangles*3*sizeof(*idx));
 p.view=(gh_static_world_packet){p.v,r->vertices,p.i,r->triangles,r->primitive,r->material,c->materials[sub],sub,batch,display_base0};
 packet*a=realloc(g->packets,(g->view.packets+1)*sizeof(*a));if(!a){free(p.v);free(p.i);return GH_WORLD_MEMORY;}g->packets=a;g->packets[g->view.packets++]=p;return GH_WORLD_OK;
}
gh_static_world_status gh_static_world_queue_append(gh_static_world_queue*q,const gh_object_static_command*c,size_t*out,gh_static_transform_frontier*f){
 if(!q||!c||!out||!f||!c->object)return GH_WORLD_ARGUMENT;
 if(q->count==MAX_GROUPS)return GH_WORLD_BOUNDS;
 if(c->kind!=GH_OBJECT_SETUP&&c->kind!=GH_OBJECT_NODE_GEOMETRY)return GH_WORLD_ARGUMENT;
 if((c->bank!=1&&c->bank!=2)||!c->depth||c->handle!=c->object->handle||!c->object->constructor_complete)return GH_WORLD_ARGUMENT;
 group g={0};g.view.command=*c;g.view.captured_fog_rgb=c->object->fog_rgb;gh_static_world_status status=GH_WORLD_OK;
 if(c->kind==GH_OBJECT_NODE_GEOMETRY){
  if(c->selected_program!=10&&c->selected_program!=14){f->ee_entry=0x155dc8;f->vu_byte_pc=0;return GH_WORLD_UNSUPPORTED;}
  if(c->material_count!=c->object->descriptor.submesh_count||!c->material_count||c->material_count>16||
     c->display_base0_streams>>c->material_count)return GH_WORLD_ARGUMENT;
  for(unsigned s=0;s<c->material_count;s++){
   const gh_stage_material *m=c->materials[s];
   unsigned display_base0=(c->display_base0_streams>>s)&1u;
   gh_scene_mesh_stream stream;if(gh_scene_mesh_stream_at(&c->object->descriptor,s,&stream)!=GH_MESH_OK){status=GH_WORLD_BOUNDS;goto fail;}
   if(display_base0?(m||stream.material!=255):
      (!m||stream.material==255||!m->rgba_gs||!m->width||!m->height||m->rgba_bytes!=(size_t)m->width*m->height*4)){
    status=GH_WORLD_ARGUMENT;goto fail;
   }
   size_t batches;if(gh_static_mesh_batch_count(&stream,&batches)!=GH_STATIC_OK){status=GH_WORLD_BOUNDS;goto fail;}
   for(size_t b=0;b<batches;b++){
    gh_static_gs_vertex v[362];uint32_t idx[864];gh_static_packet_range ranges[37];size_t nr=0;gh_static_mesh_status st;
    if(c->selected_program==10){gh_static_packet_result result;st=gh_static_mesh_program10_packets(&stream,c->object->descriptor.node_flags,b,&c->packet.inputs,c->packet.projection,GH_STATIC_BINARY32_REFERENCE,v,362,idx,864,ranges,37,&result,f);if(st==GH_STATIC_OK)nr=result.packets;}
    else{gh_static_geometry_result result;st=gh_static_mesh_transform_batch(&stream,c->object->descriptor.node_flags,b,&c->packet.inputs,GH_STATIC_BINARY32_REFERENCE,v,362,idx,864,&result,f);if(st==GH_STATIC_OK){ranges[0]=(gh_static_packet_range){0,result.vertices,0,result.triangles,result.primitive,result.material};nr=1;}}
    if(st!=GH_STATIC_OK){status=st==GH_STATIC_UNSUPPORTED?GH_WORLD_UNSUPPORTED:GH_WORLD_BOUNDS;goto fail;}
    for(size_t r=0;r<nr;r++){status=push(&g,v,idx,ranges+r,c,s,b);if(status!=GH_WORLD_OK)goto fail;}
   }
  }
 }
 if(g.view.packets>MAX_PACKETS-q->packets){status=GH_WORLD_BOUNDS;goto fail;}
 group*a=realloc(q->groups,(q->count+1)*sizeof(*a));if(!a){status=GH_WORLD_MEMORY;goto fail;}q->groups=a;q->groups[q->count]=g;*out=q->count++;q->packets+=g.view.packets;return GH_WORLD_OK;
fail:release(&g);return status;
}
