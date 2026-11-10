#include "game_scene_meshes.h"
#include <stdlib.h>
#include <string.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t u16(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static void w32(uint8_t*p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(8*i));}
static void w16(uint8_t*p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static int span(size_t n,size_t o,size_t z){return o<=n&&z<=n-o;}
gh_scene_mesh_status gh_scene_mesh_factory_serial_from_elf(const void*raw,size_t n,uint8_t*out){
 if(!raw||!out)return GH_MESH_ARGUMENT;const uint8_t*b=raw;
 if(n<52)return GH_MESH_BOUNDS;
 if(memcmp(b,"\177ELF\1\1\1",7)||u16(b+18)!=8)return GH_MESH_UNSUPPORTED;
 uint32_t table=u32(b+28);unsigned stride=u16(b+42),count=u16(b+44);
 if(stride<32||!count||count>128||!span(n,table,(size_t)stride*count))return GH_MESH_BOUNDS;
 unsigned found=0;uint8_t value=0;
 for(unsigned i=0;i<count;i++){const uint8_t*p=b+table+(size_t)i*stride;if(u32(p)!=1)continue;
  uint32_t offset=u32(p+4),base=u32(p+8),files=u32(p+16),memory=u32(p+20);
  if(files>memory||!span(n,offset,files)||(uint64_t)base+memory>UINT64_C(0x100000000))return GH_MESH_BOUNDS;
  if(base<=0x3c3d18u&&(uint64_t)0x3c3d18u<(uint64_t)base+memory){
   if(found++)return GH_MESH_BOUNDS;uint32_t delta=0x3c3d18u-base;
   value=delta<files?b[(size_t)offset+delta]:0;
  }
 }
 if(!found)return GH_MESH_BOUNDS;*out=value;return GH_MESH_OK;
}
static int relative(size_t n,size_t base,uint32_t raw,size_t size,size_t*out){
 int64_t delta=raw<0x80000000u?(int64_t)raw:(int64_t)raw-INT64_C(4294967296);
 if(base>INT64_MAX)return 0;int64_t o=(int64_t)base+delta;
 if(o<0||(uint64_t)o>SIZE_MAX||!span(n,(size_t)o,size))return 0;*out=(size_t)o;return 1;
}
gh_scene_mesh_status gh_scene_mesh_stream_at(const gh_scene_mesh_descriptor*d,unsigned index,gh_scene_mesh_stream*out){
 if(!d||!out||!d->script.bytes)return GH_MESH_ARGUMENT;
 const uint8_t*b=d->script.bytes;size_t n=d->script.size,m=d->mesh.data_offset,h;
 if(!span(n,m,0x20)||memcmp(b+m,"mdb\0",4))return GH_MESH_BOUNDS;
 if(d->mesh.data_num!=1||!(d->node_flags&0x1000000u))return GH_MESH_UNSUPPORTED;
 unsigned count=u16(b+m+10)&255u;
 if(!count||count>16||index>=count||!span(n,m+0x20,(size_t)count*4))return GH_MESH_BOUNDS;
 if(!relative(n,m,u32(b+m+0x20+index*4),32,&h))return GH_MESH_BOUNDS;
 gh_scene_mesh_stream s={0};s.bytes=b;s.size=n;s.header_offset=h;s.vertices=u16(b+h+0x14);s.material=b[h+0x16];
 if(!s.vertices)return GH_MESH_UNSUPPORTED;
 /* Real155800 V4_16 positions (flags01000000), V2_16 UV and V4_8 color.
  * Rounded DMA fetches are checked as well as logical vertices. */
 size_t padded=((size_t)s.vertices+7)&~(size_t)7;
 if(!relative(n,h,u32(b+h),padded*8,&s.position_offset)||
    !relative(n,h,u32(b+h+8),padded*4,&s.uv_offset)||
    !relative(n,h,u32(b+h+12),padded*4,&s.color_offset))return GH_MESH_BOUNDS;
 /* 155800 advances36 but emits up to38, retaining two strip vertices.
  * Its final rounded REF can extend beyond the logical attribute array;
  * validate against the containing SCR, never synthesize padding. */
 for(size_t base=0;base<s.vertices;base+=36){
  size_t take=s.vertices-base;if(take>38)take=38;size_t fetch=(take+7)&~(size_t)7;
  if(!span(n,s.position_offset+base*8,fetch*8)||!span(n,s.uv_offset+base*4,fetch*4)||
     !span(n,s.color_offset+base*4,fetch*4))return GH_MESH_BOUNDS;
  if(s.vertices-base<=36)break;
 }
 s.parity_packet_bytes=0x50+((s.vertices+23u)/24u)*0x70;
 *out=s;return GH_MESH_OK;
}
gh_scene_mesh_status gh_scene_mesh_position(const gh_scene_mesh_stream*s,size_t i,int16_t xyz[3],uint16_t*w){
 if(!s||!xyz||!w||!s->bytes)return GH_MESH_ARGUMENT;
 if(i>=s->vertices||!span(s->size,s->position_offset,((size_t)s->vertices+7u)/8u*64u))return GH_MESH_BOUNDS;
 const uint8_t*p=s->bytes+s->position_offset+i*8;
 for(unsigned j=0;j<3;j++){uint16_t x=u16(p+2*j);int32_t v=x<32768?x:(int32_t)x-65536;xyz[j]=(int16_t)v;}*w=u16(p+6);return GH_MESH_OK;
}
static gh_scene_mesh_status failure(gh_scene_meshes*s,uint32_t f,int detail,gh_scene_mesh_status e){s->frontier=f;s->detail=detail;return e;}
gh_scene_mesh_status gh_scene_meshes_bind_construction(gh_scene_meshes*s,const gh_scene_mesh_construction_bindings*b){
 if(!s||!b||!b->textures||!b->light||!b->elf||!b->elf_bytes)return GH_MESH_ARGUMENT;
 s->construction=*b;return GH_MESH_OK;
}
gh_scene_mesh_status gh_scene_meshes_finish_object_reference(gh_scene_meshes *,gh_scene_mesh_object *,const gh_stage_textures *,const gh_game_world_light *,const void *,size_t);
static gh_scene_mesh_object *object_at(gh_scene_meshes*s,size_t i){return i?(s->remaining_objects?&s->remaining_objects[i-1]:NULL):&s->first;}
gh_scene_mesh_status gh_scene_meshes_object_at(const gh_scene_meshes*s,size_t i,const gh_scene_mesh_object**out){
 if(!s||!out)return GH_MESH_ARGUMENT;*out=NULL;if(i>=s->count)return GH_MESH_BOUNDS;
 const gh_scene_mesh_object*o=i?(s->remaining_objects?&s->remaining_objects[i-1]:NULL):&s->first;
 if(!o||!o->allocation.pointer)return GH_MESH_BUSY;*out=o;return GH_MESH_OK;
}
static gh_scene_mesh_status run_construction(gh_scene_meshes*s);
/* Type0 312E88 ->14E598->1495A0->14FF80. Pointer/vtable words are
 * semantic native metadata, never guest pointers dereferenced by host code. */
static void factory_initialize(gh_scene_mesh_object*o){
 uint8_t*p=o->allocation.pointer;memset(p,0,0x360);
 for(unsigned matrix=0;matrix<2;matrix++)for(unsigned i=0;i<4;i++)w32(p+matrix*0x80+i*20,0x3f800000);
 const unsigned vecw[]={0xc0,0xe0,0x100,0x130,0x1a0,0x310,0x340};
 for(unsigned i=0;i<sizeof(vecw)/sizeof(*vecw);i++)w32(p+vecw[i]+12,0x3f800000);
 for(unsigned i=0;i<4;i++){w32(p+0x110+i*4,0x3f800000);w32(p+0x120+i*4,0x3f800000);w32(p+0x240+i*4,0x3f800000);}
 w32(p+0x2c0,10);w32(p+0x2c4,3);w16(p+0x2ae,1000);w16(p+0x2b2,180);
 p[0x2b0]=200;p[0x2b6]=p[0x2b7]=p[0x2bc]=255;p[0x2be]=254;
 o->local_matrix_offset=0x30;o->rotation_matrix_offset=0xb0;
 /* 30FE68(type0,FFFF): 3078E0 returns -1, skips asset lookup. */
 w16(p+0x2fe,0);w16(p+0x300,0);o->factory_ready=1;
}
gh_scene_mesh_status gh_scene_meshes_construct_0031d1b0(gh_scene_meshes*s,gh_game_scene_tables*t,gh_boot_work*w,const gh_bundle*room,gh_room_pools*pools,gh_heap*heap){
 if(!s||!t||!w||!room||!pools||!heap)return GH_MESH_ARGUMENT;
 if(s->prepared)return run_construction(s);
 if(s->descriptors||s->first.allocation.pointer)return GH_MESH_BUSY;
 s->pools=pools;s->heap=heap;s->tables=t;s->work=w;s->frontier=0;
 memset(t->scroll_pairs,0,sizeof(t->scroll_pairs));memset(t->scroll_words,0,sizeof(t->scroll_words));((uint8_t*)w)[0x600]=0;
 gh_bundle_entry scp;gh_bundle_error e=gh_bundle_find(room,"SCP",0,&scp);
 if(e==GH_BUNDLE_NOT_FOUND){t->scroll_texture=(gh_scene_table_view){0};w32((uint8_t*)w+0x5fc,0);return GH_MESH_OK;}
 if(e||gh_bundle_open(scp.bytes,scp.size,&s->scp)!=GH_BUNDLE_OK)return failure(s,0x31ccf0,e,GH_MESH_BOUNDS);
 gh_bundle_entry tex;unsigned t3l=gh_bundle_find(&s->scp,"T3L",0,&tex)==GH_BUNDLE_OK;
 if(!t3l){e=gh_bundle_find(&s->scp,"TM3",0,&tex);if(e!=GH_BUNDLE_OK&&e!=GH_BUNDLE_NOT_FOUND)return failure(s,0x31ccf0,e,GH_MESH_BOUNDS);}
 t->scroll_texture=(gh_scene_table_view){0};if(t3l||e==GH_BUNDLE_OK)t->scroll_texture=(gh_scene_table_view){tex.bytes,tex.size};w32((uint8_t*)w+0x5fc,t->scroll_texture.bytes?1:0);
 size_t count=0;gh_model_view v;
 for(uint32_t i=0;;i++){gh_bundle_entry scr;e=gh_bundle_find(&s->scp,"SCR",i,&scr);if(e==GH_BUNDLE_NOT_FOUND)break;
  if(e||gh_model_open_scr(scr.bytes,scr.size,&v)!=GH_MODEL_OK)return failure(s,0x31d658,e,GH_MESH_BOUNDS);
  if(count>SIZE_MAX-v.mesh_count)return failure(s,0x31d658,0,GH_MESH_BOUNDS);count+=v.mesh_count;
 }
 if(!count)return GH_MESH_OK;if(count>SIZE_MAX/sizeof(*s->descriptors))return GH_MESH_BOUNDS;
 s->descriptors=calloc(count,sizeof(*s->descriptors));if(!s->descriptors)return GH_MESH_MEMORY;
 for(uint32_t i=0;;i++){gh_bundle_entry scr;e=gh_bundle_find(&s->scp,"SCR",i,&scr);if(e==GH_BUNDLE_NOT_FOUND)break;
  if(e||gh_model_open_scr(scr.bytes,scr.size,&v)!=GH_MODEL_OK)return failure(s,0x31d658,e,GH_MESH_BOUNDS);
  for(unsigned j=0;j<v.mesh_count;j++){gh_scene_mesh_descriptor*d=&s->descriptors[s->count++];d->script=v;d->script_index=i;d->mesh_index=j;
   if(gh_model_mesh_at(&v,j,&d->mesh)!=GH_MODEL_OK||!span(v.size,d->mesh.data_offset,12))return failure(s,0x31d658,0,GH_MESH_BOUNDS);
   d->submesh_count=u16(v.bytes+d->mesh.data_offset+10)&255u;d->node_flags=0x1001000u|(t3l?0x2000000u:0);
  }
 }
 if(count-1>SIZE_MAX/sizeof(*s->remaining_objects))return GH_MESH_BOUNDS;
 s->remaining_objects=count>1?calloc(count-1,sizeof(*s->remaining_objects)):NULL;
 if(count>1&&!s->remaining_objects)return GH_MESH_MEMORY;s->prepared=1;
 return run_construction(s);
}
static gh_scene_mesh_status build_prefix(gh_scene_meshes*s,gh_scene_mesh_object*o){
 gh_room_pools*pools=s->pools;gh_heap*heap=s->heap;gh_boot_work*w=s->work;
 if(!o->prefix_stage){
  const gh_room_vector *all,*scroll;
  if(gh_room_registry_view(pools,0x754c30,&all)||gh_room_registry_view(pools,0x754c38,&scroll))return failure(s,0x30fb70,0,GH_MESH_BUSY);
  if(all->count==all->capacity||scroll->count==scroll->capacity)return failure(s,0x30fb70,0,GH_MESH_MEMORY);
  if(gh_heap_allocate(heap,0x360,16,&o->allocation)!=GH_MEM_OK)return failure(s,0x312e88,0,GH_MESH_MEMORY);
  o->descriptor=s->descriptors[s->current_index];factory_initialize(o);o->prefix_stage=1;
 }
 if(o->prefix_stage==1){
  if(!s->factory_serial)return failure(s,0x30fe68,0,GH_MESH_UNSUPPORTED);
  ((uint8_t*)o->allocation.pointer)[0x2fc]=*s->factory_serial;*s->factory_serial=(uint8_t)(*s->factory_serial+1u);o->prefix_stage=2;
 }
 if(o->prefix_stage==2){
  gh_room_pool_status pe=gh_room_object_register(pools,o,s,GH_ROOM_OBJECT_SCROLL_MESH,3,&o->handle);
  if(pe)return failure(s,0x30fcc0,pe,GH_MESH_MEMORY);o->prefix_stage=3;
 }
 /* Same-class31FAE8 RTTI cast is identity. */
 if(o->prefix_stage==3){
  if(u32((uint8_t*)w+0x594)&0x40000u)return failure(s,0x149310,0,GH_MESH_MEMORY);
  gh_room_block_pool*block=&pools->blocks[1];if(!block->remaining1c)return failure(s,0x149488,0,GH_MESH_BOUNDS);
  gh_room_pool_status pe=gh_room_block_take_002a97a0(block,u32((uint8_t*)w+0x594),&o->node_handle,&o->node);
  if(pe||!o->node)return failure(s,0x149488,pe,GH_MESH_MEMORY);
  block->remaining1c--;if(block->remaining1c<block->limit24)block->limit24=block->remaining1c;
  uint8_t*node=o->node;memset(node,0,0x480);const unsigned ones[]={0x2a8,0x1b0,0x1c4,0x1d8,0x280,0x294};
  for(unsigned i=0;i<sizeof(ones)/sizeof(*ones);i++)w32(node+ones[i],0x3f800000);
  w32(node+0x380,o->descriptor.node_flags);node[0x386]=(uint8_t)o->descriptor.mesh.data_num;o->prefix_stage=4;
 }
 if(o->prefix_stage==4){
  const gh_scene_mesh_descriptor*desc=&o->descriptor;uint8_t*node=o->node;
  if(desc->mesh.data_num>=2)return failure(s,0x155138,0,GH_MESH_UNSUPPORTED);
  size_t bytes=0;for(unsigned i=0;i<desc->submesh_count;i++){
   gh_scene_mesh_stream stream;gh_scene_mesh_status e=gh_scene_mesh_stream_at(desc,i,&stream);
   if(e)return failure(s,0x155248,(int)i,e);bytes+=(size_t)stream.parity_packet_bytes*2;
  }
  if(!bytes)return failure(s,0x155248,0,GH_MESH_UNSUPPORTED);
  if(gh_heap_allocate(heap,bytes,16,&o->packet_allocation)!=GH_MEM_OK)return failure(s,0x148420,0,GH_MESH_MEMORY);
  o->packet_bytes=bytes;node[0x385]=(uint8_t)desc->submesh_count;
  for(unsigned i=0;i<4;i++)w32(node+0x3a0+i*4,0x3f800000);
  for(unsigned parity=0;parity<2;parity++)for(unsigned matrix=0;matrix<2;matrix++){
   uint8_t*target=node+0x170+parity*0xd0+matrix*0x80;memset(target,0,0x40);for(unsigned i=0;i<4;i++)w32(target+i*20,0x3f800000);
  }
  o->test=UINT64_C(0x507fd);o->tex1=UINT64_C(0xffffff3800000360);o->alpha=UINT64_C(0x8000000044);o->clamp=0;o->vif_unpack=0x6c0c0008;o->prefix_stage=5;
 }
 return GH_MESH_OK;
}
static gh_scene_mesh_status run_construction(gh_scene_meshes*s){
 for(;s->current_index<s->count;){
  gh_scene_mesh_object*o=object_at(s,s->current_index);gh_scene_mesh_status e=build_prefix(s,o);if(e)return e;
  const gh_scene_mesh_construction_bindings*b=&s->construction;
  if(!b->textures||!b->light||!b->elf)return failure(s,0x155678,0,GH_MESH_UNSUPPORTED);
  e=gh_scene_meshes_finish_object_reference(s,o,b->textures,b->light,b->elf,b->elf_bytes);if(e)return e;
  s->constructed_count++;s->current_index++;
 }
 s->frontier=0;s->detail=0;return GH_MESH_OK;
}

gh_scene_mesh_status gh_scene_meshes_release_prefix(gh_scene_meshes*s){
 if(!s)return GH_MESH_ARGUMENT;
 for(size_t i=s->count;i;i--){gh_scene_mesh_object*o=object_at(s,i-1);if(!o)continue;
  if(o->object_packet_allocation.pointer&&gh_heap_release(s->heap,o->object_packet_allocation)!=GH_MEM_OK)return GH_MESH_MEMORY;
  o->object_packet_allocation=(gh_r207_allocation){0};
  if(o->packet_allocation.pointer&&gh_heap_release(s->heap,o->packet_allocation)!=GH_MEM_OK)return GH_MESH_MEMORY;
  o->packet_allocation=(gh_r207_allocation){0};
  if(o->node_handle){gh_room_block_pool*b=&s->pools->blocks[1];if(gh_room_block_put_002a9800(b,o->node_handle)!=GH_POOL_OK)return GH_MESH_BUSY;b->remaining1c++;o->node_handle=0;o->node=NULL;}
  if(o->handle){if(gh_room_object_unregister(s->pools,o->handle,s)!=GH_POOL_OK)return GH_MESH_BUSY;o->handle=0;}
  if(o->allocation.pointer&&gh_heap_release(s->heap,o->allocation)!=GH_MEM_OK)return GH_MESH_MEMORY;
  o->allocation=(gh_r207_allocation){0};
 }
 free(s->remaining_objects);free(s->descriptors);memset(s,0,sizeof(*s));return GH_MESH_OK;
}
