/* Originaldynamicnodepacket producer as readable native semantics.
 * Evidence analysis/game-actor-packets-evidence.json, not a DMA interpreter. */
#include "game_actor_packets.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}
static void quad(uint8_t*p,size_t o,uint64_t n){memcpy(p+o,&n,8);}
static void identity(uint8_t*p,size_t o){memset(p+o,0,64);for(unsigned j=0;j<4;j++)put(p,o+j*20,0x3f800000);}
static float add(float a,float b){volatile float n=a+b;return n;}
static float sub(float a,float b){volatile float n=a-b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static gh_actor_model_status fail(gh_actor_node_packets*s,uint32_t pc,unsigned detail,gh_actor_model_status e){s->frontier=pc;s->detail=detail;return e;}
static gh_actor_model_status bounds(gh_actor_node_packets*s){
 float lo[3]={1e9f,1e9f,1e9f},hi[3]={-1e9f,-1e9f,-1e9f};
 for(size_t i=0;i<s->stream_count;i++)for(size_t j=0;j<s->streams[i].input.vertices;j++){
  gh_skinned_mesh_vertex vertex;gh_static_mesh_status e=gh_skinned_mesh_vertex_at(&s->streams[i].input,j,&vertex);if(e)return fail(s,0x1490b0,(unsigned)i,GH_ACTOR_MODEL_BOUNDS);
  for(unsigned k=0;k<3;k++){if(vertex.position[k]<lo[k])lo[k]=vertex.position[k];if(vertex.position[k]>hi[k])hi[k]=vertex.position[k];}
 }
 fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x1f8a88,0,GH_ACTOR_MODEL_UNSUPPORTED);
 for(unsigned k=0;k<3;k++){float half=mul(sub(hi[k],lo[k]),.5f);s->box[k]=add(half,lo[k]);s->box[k+3]=half;}
 if(fesetenv(&saved))return fail(s,0x1f8a88,0,GH_ACTOR_MODEL_UNSUPPORTED);return GH_ACTOR_MODEL_OK;
}
static void header(gh_actor_node_packets*s,unsigned parity){
 uint8_t*n=s->node;size_t h=(size_t)parity*0xb0,c=0x160+(size_t)parity*0xd0;
 identity(n,c+0x10);identity(n,c+0x90);put(n,c,0);put(n,c+4,0);put(n,c+8,0x1000404);put(n,c+12,0x6c0c00c8);
 /* Addresscomponentsareexplicitlyremoved; typedstate carries theactual
  * references to constantsatc andtheparity streamallocation. */
 memset(n+h,0,0xb0);quad(n,h,0x3000000d);quad(n,h+0x10,0x10000005);put(n,h+0x1c,0x50000005);quad(n,h+0x20,UINT64_C(0x1000000000008004));put(n,h+0x28,0xe);
 quad(n,h+0x30,s->test);quad(n,h+0x38,0x47);quad(n,h+0x40,s->tex1);quad(n,h+0x48,0x14);quad(n,h+0x50,s->alpha);quad(n,h+0x58,0x42);quad(n,h+0x60,s->clamp);quad(n,h+0x68,8);
 quad(n,h+0x70,0x10000000);quad(n,h+0x80,0x10000000);quad(n,h+0x90,0x50000000);quad(n,h+0xa0,0x70000000);put(n,h+0xac,0x11000000);
}
gh_actor_model_status gh_actor_packets_build_00148420(gh_actor_node_packets*s,void*node,size_t node_bytes,
 const gh_model_view*script,size_t mesh,const gh_bundle_entry*texture,uint32_t flags,uint32_t argument,gh_heap*heap,gh_static_transform_policy policy,int*out){
 if(!s||!node||node_bytes<0x480||!script||!script->bytes||!texture||!texture->bytes||!heap||!out)return GH_ACTOR_MODEL_ARGUMENT;*out=0;s->frontier=0;s->detail=0;
 if(s->bound&&(s->node!=node||s->script.bytes!=script->bytes||s->mesh_index!=mesh||s->texture.bytes!=texture->bytes||s->flags!=flags||s->argument!=argument||s->heap!=heap))return GH_ACTOR_MODEL_BUSY;
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x1490b0,0,GH_ACTOR_MODEL_UNSUPPORTED);
 if((flags&~0xe000000u)!=0x1080||((flags&0xe000000u)!=0x2000000u&&(flags&0xe000000u)!=0x4000000u&&(flags&0xe000000u)!=0x8000000u))return fail(s,0x148420,flags,GH_ACTOR_MODEL_UNSUPPORTED);
 if(s->ready){*out=1;return GH_ACTOR_MODEL_OK;}
 if(!s->bound){
  if(word((uint8_t*)node+0x408))return fail(s,0x2a9680,0,GH_ACTOR_MODEL_UNSUPPORTED);
  size_t count;gh_static_mesh_status e=gh_skinned_mesh_stream_count(script,mesh,&count);if(e)return fail(s,0x155138,e,GH_ACTOR_MODEL_BOUNDS);
  s->node=node;s->script=*script;s->mesh_index=mesh;s->texture=*texture;s->flags=flags;s->argument=argument;s->heap=heap;s->stream_count=count;
  size_t per_frame=0,total_batches=0;
  for(size_t i=0;i<count;i++){
   gh_actor_packet_stream*t=s->streams+i;e=gh_skinned_mesh_stream_at(script,mesh,i,&t->input);if(e)return fail(s,0x155138,(unsigned)i,GH_ACTOR_MODEL_BOUNDS);
   if(gh_skinned_mesh_batch_count(&t->input,&t->batches))return GH_ACTOR_MODEL_BOUNDS;t->first_batch=total_batches;total_batches+=t->batches;t->frame_offset[0]=per_frame;per_frame+=t->input.parity_packet_bytes;t->material_slot=t->input.material; // node384 reset0 before154DA0
  }
  s->bytes=per_frame*2;s->batch_count=total_batches;s->batches=calloc(total_batches,sizeof(*s->batches));if(!s->batches)return GH_ACTOR_MODEL_MEMORY;
  for(size_t i=0;i<count;i++){gh_actor_packet_stream*t=s->streams+i;t->frame_offset[1]=per_frame+t->frame_offset[0];
   for(size_t j=0;j<t->batches;j++){gh_actor_packet_batch*b=s->batches+t->first_batch+j;e=gh_skinned_mesh_batch_at(&t->input,j,&b->input);if(e){free(s->batches);s->batches=NULL;return fail(s,0x154e98,(unsigned)i,GH_ACTOR_MODEL_BOUNDS);}b->stream=i;for(unsigned p=0;p<2;p++)b->frame_offset[p]=t->frame_offset[p]+0x40+j*0x90;}
  }
  /* Original148420 prefixbeforepacketallocation. Guest388/400/408 aretyped
   * script/texture/allocation references above, rawwords kept0. */
  put(node,0x400,0);((uint8_t*)node)[0x384]=0;put(node,0x380,flags);put(node,0x410,0);put(node,0x388,0);((uint8_t*)node)[0x386]=(uint8_t)s->streams[0].input.parts;s->bound=1;
 }
 if(!s->allocation.pointer){if(gh_heap_allocate(heap,s->bytes,16,&s->allocation)!=GH_MEM_OK)return fail(s,0x30ee08,0,GH_ACTOR_MODEL_MEMORY);}
 s->test=(flags&0x80)?0x5000d:0x5000c;s->alpha=(flags&0x80)?UINT64_C(0x8000000044):UINT64_C(0x800000002a);s->tex1=UINT64_C(0xffffff3800000360);s->clamp=0;
 put(node,0x38c,0);put(node,0x390,0);for(unsigned j=0;j<4;j++)put(node,0x3a0+j*4,0x3f800000);((uint8_t*)node)[0x385]=(uint8_t)s->stream_count;
 header(s,0);header(s,1);gh_actor_model_status status=bounds(s);if(status)return status;s->ready=1;*out=1;return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_packets_release(gh_actor_node_packets*s){if(!s)return GH_ACTOR_MODEL_ARGUMENT;if(s->allocation.pointer&&gh_heap_release(s->heap,s->allocation)!=GH_MEM_OK)return GH_ACTOR_MODEL_MEMORY;free(s->batches);*s=(gh_actor_node_packets){0};return GH_ACTOR_MODEL_OK;}
