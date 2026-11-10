/* Original149738 whole ordered node loop,14B640 boxunion and14BFE0.
 * Native semantic packetrefs replace PS2 DMA addresses; no skin execution. */
#include "game_actor_model_nodes.h"
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
typedef struct {uint32_t handle;uint8_t*bytes;gh_actor_node_packets packet;gh_model_mesh mesh;uint32_t program[2];} actor_node;
struct gh_actor_model_nodes {actor_node *entries;size_t allocated,complete,count;gh_r207_allocation object_allocation;size_t object_bytes,palette_offset[2],part_table_offset;gh_actor_object_packets object;uint8_t object_ready;};
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
static float add(float a,float b){volatile float n=a+b;return n;}static float sub(float a,float b){volatile float n=a-b;return n;}static float mul(float a,float b){volatile float n=a*b;return n;}
static gh_actor_model_status fail(gh_actor_model*s,uint32_t f,unsigned d,gh_actor_model_status e){s->frontier=f;s->detail=d;return e;}
static void merge(float*a,const float*b){for(unsigned k=0;k<3;k++){float l=sub(a[k],a[k+3]),r=add(a[k],a[k+3]),x=sub(b[k],b[k+3]),y=add(b[k],b[k+3]);if(l<x)x=l;if(y<r)y=r;float half=mul(sub(y,x),.5f);a[k]=add(half,x);a[k+3]=half;}}
static gh_actor_model_status append(gh_actor_model*s,actor_node*n){uint8_t*root=s->root.bytes,*node=n->bytes;gh_model_mesh*m=&n->mesh;
 put(node,0x414,0);if(m->alpha){put(node,0x380,w(node+0x380)|0x20000000);put(root,0x254,w(root+0x254)|0x20000000);}if(m->flags&0x2000)put(node,0x380,w(node+0x380)|0x40000000);node[0x40c]=m->layer;if(m->flags&0x100000)put(node,0x380,w(node+0x380)|0x800000);if(m->flags&8)put(node,0x410,w(node+0x410)|0x20000000);
 if((uint8_t)(root[0x2b1]+1)==1)memcpy(root+0x260,n->packet.box,24);else{fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x1f91d0,0,GH_ACTOR_MODEL_UNSUPPORTED);float box[6];memcpy(box,root+0x260,24);merge(box,n->packet.box);int ok=fesetenv(&saved);if(ok)return fail(s,0x1f91d0,0,GH_ACTOR_MODEL_UNSUPPORTED);memcpy(root+0x260,box,24);}root[0x2b1]=(uint8_t)(root[0x2b1]+1);return GH_ACTOR_MODEL_OK;}
gh_actor_model_status gh_actor_model_build_nodes(gh_actor_model*s,gh_static_transform_policy policy,int*out){
 if(!s||!s->bound||s->stage<3||!s->packet_heap||!out)return GH_ACTOR_MODEL_ARGUMENT;*out=0;
 if(!s->nodes){struct gh_actor_model_nodes*n=calloc(1,sizeof(*n));if(!n)return GH_ACTOR_MODEL_MEMORY;n->count=s->script.mesh_count;n->entries=calloc(n->count,sizeof(*n->entries));if(!n->entries){free(n);return GH_ACTOR_MODEL_MEMORY;}s->nodes=n;n->entries[0].handle=s->first_node_handle;n->entries[0].bytes=s->first_node;n->allocated=1;}
 struct gh_actor_model_nodes*n=s->nodes;
 while(n->complete<n->count){size_t i=n->complete;actor_node*e=n->entries+i;if(i>=n->allocated){if(w((uint8_t*)s->work+0x594)&0x40000){s->work->flags|=0x400000;return fail(s,0x149310,(unsigned)i,GH_ACTOR_MODEL_MEMORY);}gh_room_block_pool*p=&s->pools->blocks[1];void*bytes=NULL;if(gh_room_block_take_002a97a0(p,w((uint8_t*)s->work+0x594),&e->handle,&bytes)||!bytes){s->work->flags|=0x400000;return fail(s,0x149310,(unsigned)i,GH_ACTOR_MODEL_MEMORY);}p->remaining1c--;if(p->remaining1c<p->limit24)p->limit24=p->remaining1c;e->bytes=bytes;memset(bytes,0,0x480);const unsigned ones[]={0x2a8,0x1b0,0x1c4,0x1d8,0x280,0x294};for(unsigned j=0;j<6;j++)put(bytes,ones[j],0x3f800000);n->allocated++;}
 if(gh_model_mesh_at(&s->script,i,&e->mesh))return fail(s,0x149738,(unsigned)i,GH_ACTOR_MODEL_BOUNDS);int original;gh_actor_model_status status=gh_actor_packets_build_00148420(&e->packet,e->bytes,0x480,&s->script,i,&s->texture,s->texture_flags|0x1080,s->packet_argument,s->packet_heap,policy,&original);if(status){if(status==GH_ACTOR_MODEL_MEMORY)s->work->flags|=0x200000;return fail(s,e->packet.frontier,(unsigned)i,status);}if(!original)return fail(s,0x148420,(unsigned)i,GH_ACTOR_MODEL_UNSUPPORTED);status=append(s,e);if(status)return status;n->complete++;}
 uint8_t*root=s->root.bytes;int special=0;for(size_t i=0;i<n->count;i++)if(n->entries[i].mesh.flags&0x1000000)special=1;if(!special)put(root,0x250,w(root+0x250)|0x400);
 if(!s->borrowed_fog_rgb)return fail(s,0x14bfe0,0,GH_ACTOR_MODEL_UNSUPPORTED);
 /*14BFE0 boundedpart palette. Original formulas assume <=42; larger source
  * would underflow the42-part VU window and remains explicitunsupported. */
 if(s->part_count>42)return fail(s,0x14bfe0,s->part_count,GH_ACTOR_MODEL_UNSUPPORTED);
 if(!n->object_ready){n->object_bytes=(size_t)s->part_count*0x84+0x3c0;if(!n->object_allocation.pointer&&gh_heap_allocate(s->packet_heap,n->object_bytes,64,&n->object_allocation)){s->work->flags|=0x200000;return fail(s,0x30ee08,0,GH_ACTOR_MODEL_MEMORY);}size_t stride=((size_t)s->part_count*4+1)*16;n->palette_offset[0]=0x3a0;n->palette_offset[1]=0x3a0+stride;n->part_table_offset=0x3a0+stride*2;if(n->part_table_offset+(size_t)s->part_count*4>n->object_bytes)return fail(s,0x14bfe0,0,GH_ACTOR_MODEL_BOUNDS);memcpy(&n->object.fog_rgb,s->borrowed_fog_rgb,4);n->object.fog_rgb&=0xffffffu;
 /* Raw238/220..234/278 pointers remainzero. Typed allocation and offsets are
  * canonical, not hostptrs embedded in PS2words. Programselectors reference
  * recovered native kernel identities, not an interpreter/DMA upload ack. */
 for(unsigned p=0;p<2;p++){n->object.object_program[p]=2;n->object.node_selector[p]=0x80000002u;n->object.header_offset[p]=(size_t)p*0x80;n->object.constants_offset[p]=0x100+(size_t)p*0x150;n->object.palette_offset[p]=n->palette_offset[p];for(size_t i=0;i<n->count;i++)n->entries[i].program[p]=0x80000002u;}n->object.parts=s->parts;n->object.part_count=s->part_count;n->object.part_table_offset=n->part_table_offset;n->object.bytes=n->object_bytes;n->object.palette_unpack=0x6c000000u|(s->part_count*4u)<<16;n->object.dma_constants=0x30000015;n->object.cycle=0x1000404;n->object.gif=UINT64_C(0x1000000000008001);n->object.gs_fog_register=0x3d;n->object.dma_palette=0x30000000u|(s->part_count*4u+1);n->object.ret=0x70000000;n->object.flush=0x11000000;n->object.direct=0x50000002;n->object.mscnt=0x11000000;n->object.ready=1;n->object_ready=1;}
 for(size_t i=0;i<n->count;i++)if(n->entries[i].mesh.flags&16)put(n->entries[i].bytes,0x380,w(n->entries[i].bytes+0x380)|1);
 s->stage=4;s->ready=1;s->frontier=0;s->detail=0;*out=1;return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_model_node_at(const gh_actor_model*s,unsigned i,const uint8_t**out,size_t*mesh){if(!s||!out||!mesh||!s->nodes)return GH_ACTOR_MODEL_ARGUMENT;if(i>=s->nodes->complete)return GH_ACTOR_MODEL_BOUNDS;*out=s->nodes->entries[i].bytes;*mesh=i;return GH_ACTOR_MODEL_OK;}
gh_actor_model_status gh_actor_model_packet_at(const gh_actor_model*s,unsigned i,const gh_actor_node_packets**out){if(!s||!out||!s->nodes)return GH_ACTOR_MODEL_ARGUMENT;if(i>=s->nodes->complete)return GH_ACTOR_MODEL_BOUNDS;*out=&s->nodes->entries[i].packet;return GH_ACTOR_MODEL_OK;}
gh_actor_model_status gh_actor_model_release_nodes(gh_actor_model*s){if(!s)return GH_ACTOR_MODEL_ARGUMENT;struct gh_actor_model_nodes*n=s->nodes;if(!n)return GH_ACTOR_MODEL_OK;for(size_t i=n->allocated;i;i--){actor_node*e=n->entries+i-1;gh_actor_model_status status=gh_actor_packets_release(&e->packet);if(status)return status;if(i>1&&e->handle){if(gh_room_block_put_002a9800(&s->pools->blocks[1],e->handle))return GH_ACTOR_MODEL_BUSY;s->pools->blocks[1].remaining1c++;}}if(n->object_allocation.pointer&&gh_heap_release(s->packet_heap,n->object_allocation))return GH_ACTOR_MODEL_MEMORY;free(n->entries);free(n);s->nodes=NULL;s->root.bytes[0x2b1]=0;put(s->root.bytes,0x25c,0);return GH_ACTOR_MODEL_OK;}

gh_actor_model_status gh_actor_model_object_packet(const gh_actor_model*s,const gh_actor_object_packets**out){if(!s||!out||!s->nodes)return GH_ACTOR_MODEL_ARGUMENT;*out=&s->nodes->object;return GH_ACTOR_MODEL_OK;}
