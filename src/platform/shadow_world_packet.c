#include "shadow_world_packet.h"
#include <string.h>
static uint32_t word(const void *p){uint32_t n;memcpy(&n,p,4);return n;}
static uint64_t quad(const void *p){uint64_t n;memcpy(&n,p,8);return n;}
static int64_t area(const uint64_t *xyz,unsigned a,unsigned b,unsigned c){
 int64_t ax=(uint16_t)xyz[a],ay=(uint16_t)(xyz[a]>>16);
 int64_t bx=(uint16_t)xyz[b],by=(uint16_t)(xyz[b]>>16);
 int64_t cx=(uint16_t)xyz[c],cy=(uint16_t)(xyz[c]>>16);
 return (bx-ax)*(cy-ay)-(by-ay)*(cx-ax);
}
static gh_shadow_packet_status unsupported(uint32_t *f,uint32_t pc){if(f)*f=pc;return GH_SHADOW_PACKET_UNSUPPORTED;}
gh_shadow_packet_status gh_shadow_world_packet_capture(const gh_shadow_group *g,gh_shadow_world_packet *out,uint32_t *f){
 if(f)*f=0;if(!g||!out||!g->ready||!g->owner||!g->owner->initialized||!g->owner->object||!g->owner->object->bytes||g->parity>1)return GH_SHADOW_PACKET_ARGUMENT;
 if(g->bank!=2||!g->depth||g->depth>1022||g->primitive!=0x5c||g->tex0!=UINT64_C(0x2003800599422600)||g->tex1!=0x60||g->alpha!=UINT64_C(0x8000000044)||g->test!=0x5040c||g->clamp)return unsupported(f,0x31a600);
 const gh_title_player_view *v=g->owner->object;uintptr_t base=(uintptr_t)v->bytes,packet=(uintptr_t)g->owner->packets[g->parity];
 if(packet<base||packet-base>v->size||0x750>v->size-(packet-base))return GH_SHADOW_PACKET_ARGUMENT;
 const uint8_t *p=(const uint8_t*)packet;
 if(quad(p+0x20)!=g->alpha||quad(p+0x30)!=g->test||quad(p+0x40)!=g->clamp||quad(p+0x50)!=UINT64_C(0x13a0000f8)||quad(p+0x60)!=g->tex0||quad(p+0x70)!=g->tex1||quad(p+0x740)!=UINT64_C(0x3a0000f8))return unsupported(f,0x31a600);
 gh_shadow_world_packet result={0};result.group=*g;
 static const unsigned kicks[2][3]={{0,1,2},{2,1,3}};
 static const uint32_t st[4][2]={{0,0},{0,0x3f800000},{0x3f800000,0},{0x3f800000,0x3f800000}};
 for(unsigned slot=0;slot<12;slot++){
  const uint8_t *r=p+0x80+slot*0x90;uint64_t xyz[4];unsigned used[4]={0},remap[4]={0};
  if(quad(r+8)!=1||word(r+4)!=0x3f800000)return unsupported(f,0x15ec60);
  for(unsigned k=0;k<4;k++){
   if(quad(r+0x18+k*0x20)!=2||word(r+0x10+k*0x20)!=st[k][0]||word(r+0x14+k*0x20)!=st[k][1]||quad(r+0x28+k*0x20)!=(k<2?0xdu:5u))return unsupported(f,0x15ec60);
   xyz[k]=quad(r+0x20+k*0x20);
  }
  unsigned live[2];for(unsigned t=0;t<2;t++){live[t]=area(xyz,kicks[t][0],kicks[t][1],kicks[t][2])!=0;if(live[t])for(unsigned k=0;k<3;k++)used[kicks[t][k]]=1;}
  for(unsigned k=0;k<4;k++)if(used[k]){
   uint32_t z=(uint32_t)(xyz[k]>>32);if(z>65535)return unsupported(f,0x15ec60);
   remap[k]=(unsigned)result.vertex_count;gh_static_gs_vertex *vertex=result.vertices+result.vertex_count++;
   vertex->stq[0]=st[k][0]?1:0;vertex->stq[1]=st[k][1]?1:0;vertex->stq[2]=1;
   for(unsigned c=0;c<4;c++)vertex->rgba[c]=(word(r)>>(8*c))&255;
   vertex->xyz_fixed4[0]=(uint16_t)xyz[k];vertex->xyz_fixed4[1]=(uint16_t)(xyz[k]>>16);vertex->xyz_fixed4[2]=(int32_t)(z<<4);
  }
  for(unsigned t=0;t<2;t++)if(live[t]){for(unsigned k=0;k<3;k++)result.indices[result.triangles*3+k]=remap[kicks[t][k]];result.triangles++;}
 }
 *out=result;return GH_SHADOW_PACKET_OK;
}
