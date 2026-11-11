/* Actual patched ELF constructors; upstream MIT LICENSE.upstream. */
#include "game_enemy_factory.h"
#include <string.h>
static void word(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
static void half(uint8_t*p,size_t o,uint16_t v){memcpy(p+o,&v,2);}
static void vf0(uint8_t*p,size_t o){memset(p+o,0,12);word(p,o+12,0x3f800000);}
gh_title_player_status gh_enemy_base_construct_0028eb00(void*raw,size_t n,gh_title_player_view*out){
 if(!raw||!out)return GH_PLAYER_ARGUMENT;
 if(n<0x660)return GH_PLAYER_BOUNDS;
 gh_title_player_view view;
 gh_title_player_status e=gh_game_object_construct_002a7338(raw,n,&view);if(e)return e;
 uint8_t*p=raw;word(p,0x214,0x44a660);vf0(p,0x5c0);vf0(p,0x5d0);
 word(p,0x610,0);vf0(p,0x650);half(p,0x2ae,600);word(p,0x4ac,0x44a630);
 word(p,0x610,0);half(p,0x62e,0);word(p,0x644,0);
 view.original_vtable214=0x44a660;view.original_kind4ac=0x44a630;*out=view;return GH_PLAYER_OK;
}
gh_title_player_status gh_enemy202_construct_00201fa0(void*raw,size_t n,gh_title_player_view*out){
 if(!raw||!out)return GH_PLAYER_ARGUMENT;
 if(n<GH_EM202_BYTES)return GH_PLAYER_BOUNDS;
 gh_title_player_view view;gh_title_player_status e=gh_enemy_base_construct_0028eb00(raw,n,&view);if(e)return e;
 uint8_t*p=raw;word(p,0x214,0x4479c8);
 static const uint16_t offsets[]={0x6c0,0x6d0,0x1630,0x1660,0x1680,0x1690,0x16a0,0x16b0,0x1840,0x1950};
 for(size_t i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++)vf0(p,offsets[i]);
 p[0x1651]=255;for(unsigned i=0;i<8;i++)vf0(p,0x18a0+16*i);
 word(p,0x4ac,0x42c7f0);view.original_vtable214=0x4479c8;view.original_kind4ac=0x42c7f0;*out=view;return GH_PLAYER_OK;
}
gh_title_player_status gh_enemy202_allocate_003130e0(gh_game_title_player*s,gh_heap*heap,const gh_boot_work*work,int*original){
 if(!s||!heap||!work||!original)return GH_PLAYER_ARGUMENT;
 *original=0;if(s->allocation.pointer||s->arena_allocation.pointer)return GH_PLAYER_BUSY;
 uint32_t flags;memcpy(&flags,(const uint8_t*)work+0x594,4);if(flags&0x100000)return GH_PLAYER_OK;
 gh_r207_allocation allocation;
 if(gh_heap_allocate(heap,GH_EM202_BYTES,16,&allocation)!=GH_MEM_OK)return GH_PLAYER_MEMORY;
 memset(allocation.pointer,0,GH_EM202_BYTES);
 gh_title_player_status e=gh_enemy202_construct_00201fa0(allocation.pointer,GH_EM202_BYTES,&s->view);
 if(e){(void)gh_heap_release(heap,allocation);return e;}
 s->allocation=allocation;s->heap=heap;s->allocation_domain=GH_PLAYER_ALLOCATION_DEFAULT_HEAP;
 s->view.initial_memory=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;
 /* ready means constructorcomplete only, matching existing allocation wrapper. */
 s->ready=1;s->frontier=0;*original=1;return GH_PLAYER_OK;
}
