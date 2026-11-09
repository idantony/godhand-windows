#include "game_model_packets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){gh_game_model_packets*s=malloc(sizeof(*s));CHECK(s);memset(s,0xa5,sizeof(*s));gh_game_model_packets_reset_001509a0(s);CHECK(!s->packet_count3c);
 for(unsigned i=0;i<1280;i++){gh_model_packet32*p=s->packets+i;CHECK(p->vif_direct08==0x11000000&&p->gif_ad10==UINT64_C(0x1000000000008004)&&p->gif_regs18==14);
 CHECK(p->gs_bitbltbuf28==0x50&&!p->bitbltbuf30&&p->gs_trxpos38==0x51&&p->gs_trxreg48==0x52&&!p->trxreg50&&p->gs_trxdir58==0x53&&!p->trxdir68&&!p->field78);
 CHECK(p->dma_end80==0x70000002&&!p->field88&&p->vif_direct8c==0x50000002&&p->gif_ad90==UINT64_C(0x1000000000008001)&&p->gif_regs98==14&&p->mask_a0==255&&p->gs_texflush_a8==63);
 const uint8_t*b=(const uint8_t*)p;const unsigned holes[][2]={{0,8},{12,16},{32,40},{64,72},{96,104},{112,120},{124,128},{176,192}};for(unsigned k=0;k<8;k++)for(unsigned n=holes[k][0];n<holes[k][1];n++)CHECK(b[n]==0xa5);
 }
 s->packets[1279].unknown20=UINT64_C(0x1234567887654321);s->packet_count3c=2;gh_game_model_packets_reset_001509a0(s);CHECK(!s->packet_count3c&&s->packets[1279].unknown20==UINT64_C(0x1234567887654321));free(s);puts("PASS original1280modelpacket templates/packed64bittags/untouchedbytes/repeatedreset");return 0;}
