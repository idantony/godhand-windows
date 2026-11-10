#include "game_actor_packets.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));gh_heap*heap;C(!gh_heap_create(16*1024*1024,&heap));const char*paths[]={"om/om00.dat","pl/pl00.dat","pl/pl01.dat"};const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};unsigned meshes=0,batches=0,vertices=0;
 for(unsigned resource=0;resource<3;resource++){size_t n;uint8_t*bytes=load(d,paths[resource],&n);gh_bundle b;gh_bundle_entry md,tex;C(!gh_bundle_open(bytes,n,&b)&&!gh_bundle_find(&b,"MD",0,&md)&&!gh_bundle_find(&b,"TM3",0,&tex));gh_model_view model;C(!gh_model_open_scr(md.bytes,md.size,&model));
 for(unsigned mesh=0;mesh<model.mesh_count;mesh++){size_t count;C(!gh_skinned_mesh_stream_count(&model,mesh,&count));for(unsigned mode=0;mode<4;mode++){_Alignas(16) uint8_t node[0x480]={0};gh_actor_node_packets packet={0};int original=0;C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));C(!gh_actor_packets_build_00148420(&packet,node,sizeof(node),&model,mesh,&tex,0x8001080,0,heap,GH_STATIC_BINARY32_REFERENCE,&original)&&original&&packet.ready);C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);C(packet.stream_count==count&&node[0x385]==count&&packet.bytes==gh_heap_used(heap)&&w(node+0x408)==0&&w(node+0x400)==0&&w(node+0x388)==0);
 long double low[3]={1e9L,1e9L,1e9L},high[3]={-1e9L,-1e9L,-1e9L};size_t parity=0,total=0;
 for(size_t i=0;i<count;i++){gh_actor_packet_stream*s=packet.streams+i;const uint8_t*header=model.bytes+s->input.header_offset;unsigned num=(unsigned)header[20]|(unsigned)header[21]<<8;size_t expected=(num+23u)/24u;C(s->input.vertices==num&&s->batches==expected&&s->input.parity_packet_bytes==expected*144+80&&s->material_slot==header[22]&&s->frame_offset[0]==parity);parity+=s->input.parity_packet_bytes;total+=expected;
 for(unsigned j=0;j<num;j++){gh_skinned_mesh_vertex v;C(!gh_skinned_mesh_vertex_at(&s->input,j,&v));const uint8_t*position=model.bytes+s->input.position_offset+j*16;C(!memcmp(v.position,position,12)&&v.raw_control==w(position+12));C(!memcmp(v.weights,model.bytes+s->input.skin_offset+j*8+4,4));for(unsigned k=0;k<3;k++){long double x=v.position[k];if(x<low[k])low[k]=x;if(x>high[k])high[k]=x;}}
 for(size_t j=0;j<expected;j++){gh_actor_packet_batch*t=packet.batches+s->first_batch+j;size_t left=num-j*24,logical=left<26?left:26,fetch=(logical+7)/8*8;C(t->input.first_vertex==j*24&&t->input.logical_vertices==logical&&t->input.fetched_vertices==fetch&&t->input.position_qwords==fetch&&t->input.skin_qwords==fetch/2);C(t->input.position_unpack==(0x6c008002u|(uint32_t)fetch<<16)&&t->input.skin_unpack==(0x6e00c005u|(uint32_t)(2*fetch)<<16));C(t->frame_offset[0]==s->frame_offset[0]+64+j*144&&t->frame_offset[1]==s->frame_offset[1]+64+j*144);C(t->input.position_offset+fetch*16<=model.size&&t->input.skin_offset+fetch*8<=model.size);}
 }
 C(packet.bytes==parity*2&&packet.batch_count==total);for(size_t i=0;i<count;i++)C(packet.streams[i].frame_offset[1]==packet.streams[i].frame_offset[0]+parity);
 for(unsigned k=0;k<3;k++){C(fabsl((long double)packet.box[k]-(low[k]+high[k])/2)<.00001L);C(fabsl((long double)packet.box[k+3]-(high[k]-low[k])/2)<.00001L);}uint64_t used=gh_heap_used(heap);void*allocation=packet.allocation.pointer;C(!gh_actor_packets_build_00148420(&packet,node,sizeof(node),&model,mesh,&tex,0x8001080,0,heap,GH_STATIC_BINARY32_REFERENCE,&original)&&gh_heap_used(heap)==used&&packet.allocation.pointer==allocation);C(!gh_actor_packets_release(&packet)&&!gh_heap_used(heap));if(!mode){meshes++;vertices+=(unsigned)count;batches+=(unsigned)total;}
 }
 }
 free(bytes);}
 C(!fesetround(FE_TONEAREST));_Alignas(16)uint8_t node[0x480]={0};(void)node;C(!gh_heap_destroy(heap));gh_disc_close(d);printf("PASS real OM00/PL00/PL01 %u mesh node producers %u batches; raw five-stream attributes, overlap/paddingrefs, parity/allocation/independent bbox/FEnv/lifetime; NOT fullmodel/GPUready (%u streams)\n",meshes,batches,vertices);return 0;}
