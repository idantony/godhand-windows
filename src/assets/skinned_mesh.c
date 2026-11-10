/* Actual155138/1551C0/1551E0/154E98, independent readable source.
 * Native semantic references replace guest DMA addresses; no VIF interpreter. */
#include "skinned_mesh.h"
#include <string.h>
#include <math.h>
static uint32_t w(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned h(const uint8_t*p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static int32_t sh(const uint8_t*p){unsigned n=h(p);return n<32768?(int32_t)n:(int32_t)n-65536;}
static int fits(size_t n,size_t o,size_t k){return o<=n&&k<=n-o;}
static int relative(size_t n,size_t base,uint32_t word,size_t*out){int64_t delta=word<0x80000000u?(int64_t)word:(int64_t)word-INT64_C(4294967296);int64_t sum=(int64_t)base+delta;if(sum<0||(uint64_t)sum>n)return 0;*out=(size_t)sum;return 1;}
gh_static_mesh_status gh_skinned_mesh_stream_count(const gh_model_view*v,size_t mesh,size_t*out){
 if(!v||!out)return GH_STATIC_ARGUMENT;gh_model_mesh m;if(gh_model_mesh_at(v,mesh,&m)!=GH_MODEL_OK||!fits(v->size,m.data_offset,0x20))return GH_STATIC_BOUNDS;
 unsigned parts=h(v->bytes+m.data_offset+8),count=h(v->bytes+m.data_offset+10);
 if(parts<2)return GH_STATIC_UNSUPPORTED;if(!count||count>16)return GH_STATIC_UNSUPPORTED;
 if(!fits(v->size,m.data_offset+0x20,(size_t)count*4))return GH_STATIC_BOUNDS;*out=count;return GH_STATIC_OK;
}
gh_static_mesh_status gh_skinned_mesh_stream_at(const gh_model_view*v,size_t mesh,size_t index,gh_skinned_mesh_stream*out){
 if(!v||!out)return GH_STATIC_ARGUMENT;size_t count;gh_static_mesh_status status=gh_skinned_mesh_stream_count(v,mesh,&count);if(status)return status;if(index>=count)return GH_STATIC_BOUNDS;
 gh_model_mesh m;if(gh_model_mesh_at(v,mesh,&m)!=GH_MODEL_OK)return GH_STATIC_BOUNDS;size_t base;
 if(!relative(v->size,m.data_offset,w(v->bytes+m.data_offset+0x20+index*4),&base)||!fits(v->size,base,24))return GH_STATIC_BOUNDS;
 gh_skinned_mesh_stream s={0};s.bytes=v->bytes;s.size=v->size;s.data_offset=m.data_offset;s.header_offset=base;s.vertices=(uint16_t)h(v->bytes+base+20);s.parts=(uint16_t)h(v->bytes+m.data_offset+8);s.material=v->bytes[base+22];
 if(!s.vertices)return GH_STATIC_UNSUPPORTED;
 size_t*offsets[5]={&s.position_offset,&s.normal_offset,&s.uv_offset,&s.color_offset,&s.skin_offset};const size_t stride[5]={16,4,4,4,8};
 for(unsigned i=0;i<5;i++)if(!relative(v->size,base,w(v->bytes+base+i*4),offsets[i])||!fits(v->size,*offsets[i],(size_t)s.vertices*stride[i]))return GH_STATIC_BOUNDS;
 s.parity_packet_bytes=(uint32_t)(((unsigned)s.vertices+23)/24)*0x90u+0x50u;*out=s;return GH_STATIC_OK;
}
gh_static_mesh_status gh_skinned_mesh_batch_count(const gh_skinned_mesh_stream*s,size_t*out){if(!s||!s->bytes||!s->vertices||!out)return GH_STATIC_ARGUMENT;*out=((size_t)s->vertices+23)/24;return GH_STATIC_OK;}
gh_static_mesh_status gh_skinned_mesh_batch_at(const gh_skinned_mesh_stream*s,size_t i,gh_skinned_mesh_batch*out){
 if(!s||!out)return GH_STATIC_ARGUMENT;size_t count;if(gh_skinned_mesh_batch_count(s,&count))return GH_STATIC_ARGUMENT;if(i>=count)return GH_STATIC_BOUNDS;
 gh_skinned_mesh_batch b={0};b.first_vertex=i*24;size_t remain=s->vertices-b.first_vertex;b.logical_vertices=remain<26?remain:26;b.fetched_vertices=(b.logical_vertices+7)&~(size_t)7;size_t fetch=b.fetched_vertices;
 b.position_offset=s->position_offset+b.first_vertex*16;b.normal_offset=s->normal_offset+b.first_vertex*4;b.uv_offset=s->uv_offset+b.first_vertex*4;b.color_offset=s->color_offset+b.first_vertex*4;b.skin_offset=s->skin_offset+b.first_vertex*8;
 if(!fits(s->size,b.position_offset,fetch*16)||!fits(s->size,b.normal_offset,fetch*4)||!fits(s->size,b.uv_offset,fetch*4)||!fits(s->size,b.color_offset,fetch*4)||!fits(s->size,b.skin_offset,fetch*8))return GH_STATIC_BOUNDS;
 b.position_qwords=(uint32_t)fetch;b.normal_qwords=b.uv_qwords=b.color_qwords=(uint32_t)fetch/4;b.skin_qwords=(uint32_t)fetch/2;
 b.position_unpack=0x6c008002u|(uint32_t)fetch<<16;b.normal_unpack=0x6e008007u|(uint32_t)fetch<<16;b.uv_unpack=0x65008004u|(uint32_t)fetch<<16;b.color_unpack=0x6e00c003u|(uint32_t)fetch<<16;b.skin_unpack=0x6e00c005u|(uint32_t)(fetch*2)<<16;
 b.gif_strip[0]=UINT64_C(0x303e400000008000)|b.logical_vertices;b.gif_strip[1]=0x412;b.gif_fan[0]=UINT64_C(0x303ec00000008000)|b.logical_vertices;b.gif_fan[1]=0x412;*out=b;return GH_STATIC_OK;
}
gh_static_mesh_status gh_skinned_mesh_vertex_at(const gh_skinned_mesh_stream*s,size_t i,gh_skinned_mesh_vertex*out){
 if(!s||!s->bytes||!out)return GH_STATIC_ARGUMENT;if(i>=s->vertices)return GH_STATIC_BOUNDS;size_t p=s->position_offset+i*16,n=s->normal_offset+i*4,u=s->uv_offset+i*4,c=s->color_offset+i*4,k=s->skin_offset+i*8;
 if(!fits(s->size,p,16)||!fits(s->size,n,4)||!fits(s->size,u,4)||!fits(s->size,c,4)||!fits(s->size,k,8))return GH_STATIC_BOUNDS;
 gh_skinned_mesh_vertex v={0};for(unsigned j=0;j<3;j++){memcpy(v.position+j,s->bytes+p+j*4,4);if(!isfinite(v.position[j]))return GH_STATIC_UNSUPPORTED;}
 v.raw_control=w(s->bytes+p+12);memcpy(v.normal,s->bytes+n,4);memcpy(v.color,s->bytes+c,4);memcpy(v.bone_indices,s->bytes+k,4);memcpy(v.weights,s->bytes+k+4,4);v.uv[0]=sh(s->bytes+u);v.uv[1]=sh(s->bytes+u+2);*out=v;return GH_STATIC_OK;
}
