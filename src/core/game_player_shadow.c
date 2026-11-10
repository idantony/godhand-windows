#include "game_player_shadow.h"
#include <string.h>
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static void q(uint8_t*p,uint64_t v){memcpy(p,&v,8);}
static void packet(uint8_t*p){
 q(p,0x10000074);ww(p+8,0x11000000);ww(p+12,0x50000074);q(p+0x10,UINT64_C(0x102e400000008073));q(p+0x18,0xe);q(p+0x20,UINT64_C(0x8000000044));q(p+0x28,0x42);q(p+0x30,0x5040c);q(p+0x38,0x47);q(p+0x40,0);q(p+0x48,8);q(p+0x50,UINT64_C(0x13a0000f8));q(p+0x58,0x4e);q(p+0x60,UINT64_C(0x2003800599422600));q(p+0x68,6);q(p+0x70,0x60);q(p+0x78,0x14);
 for(unsigned i=0;i<12;i++){uint8_t*r=p+0x80+i*0x90;q(r,UINT64_C(0x3f80000080ffffff));q(r+8,1);ww(r+0x10,0);ww(r+0x14,0);q(r+0x18,2);ww(r+0x30,0);ww(r+0x34,0x3f800000);q(r+0x38,2);ww(r+0x50,0x3f800000);ww(r+0x54,0);q(r+0x58,2);ww(r+0x70,0x3f800000);ww(r+0x74,0x3f800000);q(r+0x78,2);q(r+0x28,0xd);q(r+0x48,0xd);q(r+0x68,5);q(r+0x88,5);}
 q(p+0x740,0x3a0000f8);q(p+0x748,0x4e);
}
gh_title_player_status gh_game_player_shadow_initialize_0014ef50(gh_player_shadow*s,gh_title_player_view*v){
 if(!s||!v||!v->bytes)return GH_PLAYER_ARGUMENT;if(v->size<0x1550)return GH_PLAYER_BOUNDS;if(s->initialized&&s->object!=v)return GH_PLAYER_ARGUMENT;
 ww(v->bytes+0x44c,0);ww(v->bytes+0x450,0);packet(v->bytes+0x6b0);packet(v->bytes+0xe00);*s=(gh_player_shadow){v,{v->bytes+0x6b0,v->bytes+0xe00},0x3c3f68,1};return GH_PLAYER_OK;
}
gh_title_player_status gh_game_player_initial_part_flags(gh_actor_model*m){
 if(!m||!m->bound||!m->root.bytes)return GH_PLAYER_ARGUMENT;m->root.bytes[0x140]=4;
 for(unsigned i=27;i<=28;i++){if(i>=m->parts_allocated)continue;const gh_actor_part*p;if(gh_actor_model_part_at(m,i,&p))return GH_PLAYER_BOUNDS;ww(p->bytes+0x154,w(p->bytes+0x154)|(i==27?0x1100000:0x2100000));}return GH_PLAYER_OK;
}
