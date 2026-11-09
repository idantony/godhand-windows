#include "game_title_player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
int main(void){uint8_t*raw=malloc(GH_TITLE_PLAYER_BYTES);CHECK(raw);memset(raw,0xa5,GH_TITLE_PLAYER_BYTES);gh_title_player_view v={0};
 CHECK(gh_title_player_construct_00107e40(raw,GH_TITLE_PLAYER_BYTES-1,&v)==GH_PLAYER_BOUNDS&&raw[0]==0xa5&&!v.bytes);
 CHECK(gh_title_player_construct_00107e40(raw,GH_TITLE_PLAYER_BYTES,&v)==GH_PLAYER_OK&&v.bytes==raw&&v.alias_d0==raw+0x30&&v.alias_f0==raw+0xb0);
 CHECK(!u32(raw+0xd0)&&!u32(raw+0xf0)&&u32(raw+0x214)==0x41d220&&u32(raw+0x4ac)==0x41b800);
 for(unsigned m=0;m<2;m++)for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)CHECK(u32(raw+m*0x80+r*16+c*4)==(r==c?0x3f800000u:0));
 const unsigned vf[]={0xc0,0xe0,0x100,0x130,0x1a0,0x310,0x320,0x330,0x360,0x370,0x380,0x390,0x460,0x480,0x490,0x4c0,0x550,0x580,0x590,0x630,0x650,0x660,0x1580,0x1590,0x15e0};
 for(unsigned i=0;i<sizeof(vf)/sizeof(vf[0]);i++)CHECK(!u32(raw+vf[i])&&!u32(raw+vf[i]+4)&&!u32(raw+vf[i]+8)&&u32(raw+vf[i]+12)==0x3f800000);
 CHECK(u32(raw+0x240)==0x3f800000&&u32(raw+0x24c)==0x3f800000&&!u32(raw+0x260)&&!u32(raw+0x274));
 CHECK(u32(raw+0x2c0)==10&&u32(raw+0x2c4)==3&&raw[0x2b0]==200&&raw[0x2ae]==0xe8&&raw[0x2af]==3&&raw[0x2b2]==180&&raw[0x2be]==254);
 CHECK(u32(raw+0x4a0)==0xffffffffu&&u32(raw+0x4d0)==65535&&u32(raw+0x52c)==0x3f800000&&raw[0x531]==255);
 const unsigned unknown[]={0x40,0x7f,0xd4,0xf4,0x141,0x210,0x23c,0x278,0x2ac,0x2f4,0x5f0,0x640,0x697,0x1690-1};for(unsigned i=0;i<sizeof(unknown)/sizeof(unknown[0]);i++)CHECK(raw[unknown[i]]==0xa5);
 free(raw);gh_heap*h;CHECK(gh_heap_create(GH_TITLE_PLAYER_BYTES,&h)==GH_MEM_OK);gh_boot_work w={.room=0x20};gh_game_title_player p={0};
 CHECK(gh_game_title_player_create_0031c4e0(&p,h,&w)==GH_PLAYER_OK&&p.ready&&gh_heap_used(h)==GH_TITLE_PLAYER_BYTES&&!((uintptr_t)p.view.bytes&15));
 int16_t hp;uint32_t frontier;CHECK(gh_title_player_read_vitality(&p.view,&hp,&frontier)==GH_PLAYER_UNSUPPORTED&&frontier==0x1f9bec);
 CHECK(gh_game_title_player_create_0031c4e0(&p,h,&w)==GH_PLAYER_BUSY);CHECK(gh_game_title_player_release(&p)==GH_PLAYER_OK&&!p.ready&&gh_heap_used(h)==0);CHECK(gh_heap_destroy(h)==GH_MEM_OK);
 CHECK(gh_heap_create(GH_TITLE_PLAYER_BYTES,&h)==GH_MEM_OK);
 gh_title_memory_profile zero={GH_TITLE_MEMORY_ZERO_REFERENCE,0,0};
 CHECK(gh_game_title_player_create_reference_0031c4e0(&p,h,&w,&zero)==GH_PLAYER_OK);
 CHECK(gh_title_player_read_vitality(&p.view,&hp,&frontier)==GH_PLAYER_OK&&hp==0&&p.view.bytes[0x2f4]==0);
 CHECK(gh_game_title_player_release(&p)==GH_PLAYER_OK);
 uint8_t provided[GH_TITLE_PLAYER_BYTES];memset(provided,0x5a,sizeof(provided));provided[0x54a]=0xd2;provided[0x54b]=4;
 gh_title_memory_profile input={GH_TITLE_MEMORY_PROVIDED_BYTES,provided,sizeof(provided)};
 CHECK(gh_game_title_player_create_reference_0031c4e0(&p,h,&w,&input)==GH_PLAYER_OK);
 CHECK(gh_title_player_read_vitality(&p.view,&hp,&frontier)==GH_PLAYER_OK&&hp==1234&&p.view.bytes[0x2f4]==0x5a);
 CHECK(gh_game_title_player_release(&p)==GH_PLAYER_OK&&gh_heap_destroy(h)==GH_MEM_OK);
 CHECK(gh_heap_create(GH_TITLE_PLAYER_BYTES-1,&h)==GH_MEM_OK);CHECK(gh_game_title_player_create_0031c4e0(&p,h,&w)==GH_PLAYER_MEMORY&&!p.ready&&!p.allocation.pointer&&p.frontier==0x30f550);w.room=0x21;CHECK(gh_game_title_player_create_0031c4e0(&p,h,&w)==GH_PLAYER_UNSUPPORTED&&p.frontier==0x31c4e0);CHECK(gh_heap_destroy(h)==GH_MEM_OK);
 puts("PASS R020 real1690allocation/inheritedconstructor fields/nativealiases/unknownbytepreservation");return 0;}
