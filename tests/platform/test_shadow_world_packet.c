#include "shadow_world_packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put64(void *p,uint64_t n){memcpy(p,&n,8);}
static void position(gh_player_shadow *s,unsigned vertex,unsigned x,unsigned y,uint32_t z){put64(s->packets[0]+0xa0+vertex*0x20,(uint64_t)x|((uint64_t)y<<16)|((uint64_t)z<<32));}
int main(void){
 _Alignas(16)uint8_t bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view view={.bytes=bytes,.size=sizeof(bytes)};
 gh_player_shadow shadow={0};C(!gh_game_player_shadow_initialize_0014ef50(&shadow,&view));
 gh_shadow_group g={.owner=&shadow,.parity=0,.bank=2,.depth=57,.tex0=UINT64_C(0x2003800599422600),.tex1=0x60,.alpha=UINT64_C(0x8000000044),.test=0x5040c,.primitive=0x5c,.ready=1};
 gh_shadow_world_packet out={0};uint32_t f=0;C(!gh_shadow_world_packet_capture(&g,&out,&f)&&!f&&!out.triangles&&!out.vertex_count&&out.group.ready&&out.group.depth==57);
 for(unsigned i=0;i<4;i++)position(&shadow,i,0,0,UINT32_MAX);
 C(!gh_shadow_world_packet_capture(&g,&out,&f)&&!out.triangles&&!out.vertex_count);
 const unsigned x[4]={28704,28704,28736,28736},y[4]={29216,29248,29216,29248};
 for(unsigned i=0;i<4;i++)position(&shadow,i,x[i],y[i],0x1234+i);
 C(!gh_shadow_world_packet_capture(&g,&out,&f)&&!f&&out.triangles==2&&out.vertex_count==4);
 const uint32_t expected[6]={0,1,2,2,1,3};C(!memcmp(out.indices,expected,sizeof(expected)));
 for(unsigned i=0;i<4;i++){const gh_static_gs_vertex*v=out.vertices+i;C(v->xyz_fixed4[0]==(int32_t)x[i]&&v->xyz_fixed4[1]==(int32_t)y[i]);C(v->xyz_fixed4[2]==(int32_t)((0x1234+i)<<4)&&v->rgba[3]==128&&v->stq[2]==1);C((((uint32_t)v->xyz_fixed4[2]>>4)&0xffffff)==0x1234+i);}
 gh_shadow_world_packet golden=out;
 position(&shadow,3,x[3],y[3],65536);C(gh_shadow_world_packet_capture(&g,&out,&f)==GH_SHADOW_PACKET_UNSUPPORTED&&f==0x15ec60&&!memcmp(&out,&golden,sizeof(out)));
 /* Actual LW/sign-extended packing can poison highXY/Z; inspect the packed
  * word instead of pretending the conceptual XYZ float remained intact. */
 put64(shadow.packets[0]+0xa0+3*0x20,UINT64_MAX);C(gh_shadow_world_packet_capture(&g,&out,&f)==GH_SHADOW_PACKET_UNSUPPORTED&&f==0x15ec60&&!memcmp(&out,&golden,sizeof(out)));
 for(unsigned i=0;i<4;i++)position(&shadow,i,x[i],y[i],65535);
 C(!gh_shadow_world_packet_capture(&g,&out,&f)&&out.vertices[3].xyz_fixed4[2]==1048560);
 position(&shadow,0,0,0,1);position(&shadow,1,65535,0,1);position(&shadow,2,0,65535,1);position(&shadow,3,65535,65535,1);
 C(!gh_shadow_world_packet_capture(&g,&out,&f)&&out.triangles==2);
 golden=out;put64(shadow.packets[0]+0x30,0);C(gh_shadow_world_packet_capture(&g,&out,&f)==GH_SHADOW_PACKET_UNSUPPORTED&&f==0x31a600&&!memcmp(&out,&golden,sizeof(out)));
 puts("PASS rawPACKED XYZ2 capture, exactintegerZ/rastercarrier,12 zero-area slots preserveonegroup,actualpacked sign/overflow strict,striporder/area64bit/atomicfailure; no GPU proof");return 0;
}
