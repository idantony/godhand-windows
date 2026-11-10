/* Actual155800 typed semantic decoder. Upstream MIT pin
 * eaae18d0dfab1f9a0df276ae5ca1581ad326faf3; evidence records actual ELF. */
#include "static_mesh.h"
#include <string.h>
static int fits(size_t n,size_t o,size_t bytes){return o<=n&&bytes<=n-o;}
static uint16_t u16(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static int32_t s16(const uint8_t*p){uint16_t v=u16(p);return v<32768?v:(int32_t)v-65536;}
static gh_static_mesh_status check(const gh_scene_mesh_stream*s,uint32_t flags,uint32_t*f){
 if(f)*f=0;if(!s||!s->bytes||!s->vertices)return GH_STATIC_ARGUMENT;
 if(!(flags&0x1000000u)){if(f)*f=0x155800;return GH_STATIC_UNSUPPORTED;}return GH_STATIC_OK;
}
gh_static_mesh_status gh_static_mesh_batch_count(const gh_scene_mesh_stream*s,size_t*out){
 if(!s||!s->bytes||!s->vertices||!out)return GH_STATIC_ARGUMENT;
 *out=s->vertices<=38?1:1+((size_t)s->vertices-38+35)/36;return GH_STATIC_OK;
}
gh_static_mesh_status gh_static_mesh_batch_at(const gh_scene_mesh_stream*s,uint32_t flags,size_t index,gh_static_mesh_batch*out,uint32_t*f){
 gh_static_mesh_status e=check(s,flags,f);if(e)return e;if(!out)return GH_STATIC_ARGUMENT;
 size_t count;if(gh_static_mesh_batch_count(s,&count)||index>=count)return GH_STATIC_BOUNDS;
 size_t first=index*36,remaining=s->vertices-first,logical=remaining<38?remaining:38,fetch=(logical+7)&~(size_t)7;
 if(s->position_offset>SIZE_MAX-first*8||s->uv_offset>SIZE_MAX-first*4||s->color_offset>SIZE_MAX-first*4)return GH_STATIC_BOUNDS;
 gh_static_mesh_batch b={0};b.first_vertex=first;b.logical_vertices=logical;b.fetched_vertices=fetch;
 b.position_offset=s->position_offset+first*8;b.uv_offset=s->uv_offset+first*4;b.color_offset=s->color_offset+first*4;
 if(!fits(s->size,b.position_offset,fetch*8)||!fits(s->size,b.uv_offset,fetch*4)||!fits(s->size,b.color_offset,fetch*4))return GH_STATIC_BOUNDS;
 b.position_ref_qwords=(uint32_t)(fetch/2);b.uv_ref_qwords=b.color_ref_qwords=(uint32_t)(fetch/4);
 b.position_unpack=0x6d008002u|(uint32_t)fetch<<16;b.color_unpack=0x6e00c003u|(uint32_t)fetch<<16;b.uv_unpack=0x65008004u|(uint32_t)fetch<<16;
 b.gif_strip[0]=UINT64_C(0x303e400000008000)|logical;if(flags&0x10)b.gif_strip[0]=UINT64_C(0x302e400000008000)|logical;
 b.gif_strip[1]=0x412;b.gif_fan[0]=b.gif_strip[0]|UINT64_C(0x800000000000);b.gif_fan[1]=0x412;b.material=s->material;*out=b;return GH_STATIC_OK;
}
gh_static_mesh_status gh_static_mesh_vertex_at(const gh_scene_mesh_stream*s,uint32_t flags,size_t index,gh_static_mesh_vertex*out,uint32_t*f){
 gh_static_mesh_status e=check(s,flags,f);if(e)return e;if(!out)return GH_STATIC_ARGUMENT;if(index>=s->vertices)return GH_STATIC_BOUNDS;
 if(s->position_offset>SIZE_MAX-index*8||s->uv_offset>SIZE_MAX-index*4||s->color_offset>SIZE_MAX-index*4)return GH_STATIC_BOUNDS;
 size_t p=s->position_offset+index*8,u=s->uv_offset+index*4,c=s->color_offset+index*4;
 if(!fits(s->size,p,8)||!fits(s->size,u,4)||!fits(s->size,c,4))return GH_STATIC_BOUNDS;
 gh_static_mesh_vertex v={0};for(unsigned j=0;j<4;j++){v.position[j]=s16(s->bytes+p+j*2);v.color[j]=s->bytes[c+j];}
 v.raw_w=u16(s->bytes+p+6);for(unsigned j=0;j<2;j++)v.uv[j]=s16(s->bytes+u+j*2);*out=v;return GH_STATIC_OK;
}
