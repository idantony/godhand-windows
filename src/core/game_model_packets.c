/* MIT LICENSE.upstream; actual1509A0 initialization only. */
#include "game_model_packets.h"
_Static_assert(sizeof(gh_model_packet32)==0xc0,"originalpacketstride");
_Static_assert(offsetof(gh_model_packet32,vif_direct08)==8,"VIFoffset");
_Static_assert(offsetof(gh_model_packet32,gif_ad10)==0x10,"GIFoffset");
_Static_assert(offsetof(gh_model_packet32,dma_end80)==0x80,"DMAoffset");
_Static_assert(offsetof(gh_model_packet32,gs_texflush_a8)==0xa8,"GSoffset");
static void reset(gh_model_packet32*p){
 p->dma_end80=0x70000002;p->field88=0;p->vif_direct8c=0x50000002;
 p->gif_ad90=UINT64_C(0x1000000000008001);p->gif_regs98=0xe;
 p->mask_a0=0xff;p->gs_texflush_a8=0x3f;
 p->vif_direct08=0x11000000;p->gs_bitbltbuf28=0x50;p->bitbltbuf30=0;
 p->gs_trxpos38=0x51;p->gs_trxreg48=0x52;
 p->gif_ad10=UINT64_C(0x1000000000008004);p->gif_regs18=0xe;
 p->trxreg50=0;p->gs_trxdir58=0x53;p->trxdir68=0;p->field78=0;
}
void gh_game_model_packets_reset_001509a0(gh_game_model_packets*s){
 if(!s)return;s->packet_count3c=0;
 for(unsigned i=640;i;i--){reset(s->packets+(i-1)*2);reset(s->packets+(i-1)*2+1);}
}
