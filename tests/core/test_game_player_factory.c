#include "game_title_player.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
int main(int argc,char**argv){gh_disc*d;size_t index;gh_disc_entry entry;C(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&index)&&!gh_disc_entry_at(d,index,&entry));uint8_t*p=malloc((size_t)entry.size);C(p&&!gh_disc_read(d,index,0,p,(size_t)entry.size));gh_bundle b;C(!gh_bundle_open(p,(size_t)entry.size,&b));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&b,heap));size_t used=gh_heap_used(heap);gh_game_title_player player={0};gh_boot_work work={.room=0x100};int original;
 ww((uint8_t*)&work+0x594,0x10000);C(!gh_game_player_construct_00312f00(&player,&pools,&work,&original)&&!original&&!player.ready&&!player.view.bytes&&!pools.arenas[2].live_allocations);
 ww((uint8_t*)&work+0x594,0x80000);C(!gh_game_player_construct_00312f00(&player,&pools,&work,&original)&&!original&&!player.ready);ww((uint8_t*)&work+0x594,0);
 C(!gh_game_player_construct_00312f00(&player,&pools,&work,&original)&&original&&player.ready&&player.allocation_domain==GH_PLAYER_ALLOCATION_ROOM_ARENA&&player.arena_allocation.original_global==0x754c10&&pools.arenas[2].live_allocations==1&&gh_heap_used(heap)==used);
 C(player.view.bytes==player.allocation.pointer&&player.view.bytes==player.arena_allocation.pointer&&player.view.initial_memory==GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO&&player.view.alias_d0==player.view.bytes+0x30&&player.view.alias_f0==player.view.bytes+0xb0&&w(player.view.bytes+0x214)==0x41d220);
 const unsigned original_zero[]={0x40,0x7f,0xd4,0xf4,0x141,0x210,0x23c,0x278,0x2ac,0x2f4,0x5f0,0x640,0x697,0x161c,0x1620,0x165c,0x168f};for(unsigned i=0;i<sizeof(original_zero)/sizeof(*original_zero);i++)C(player.view.bytes[original_zero[i]]==0);int16_t hp=-1;uint32_t frontier;C(!gh_title_player_read_vitality(&player.view,&hp,&frontier)&&!hp);
 C(gh_game_title_player_create_0031c4e0(&player,heap,&work)==GH_PLAYER_BUSY&&pools.arenas[2].live_allocations==1);C(gh_game_room_pools_release(&pools)==GH_POOL_UNSUPPORTED&&pools.failed_entry==0x2a9680);
 C(!gh_game_title_player_release(&player)&&!player.ready&&!player.view.bytes&&!pools.arenas[2].live_allocations&&gh_heap_used(heap)==used);
 uint8_t base[0x5b0];memset(base,0xa5,sizeof(base));gh_title_player_view view={0};C(gh_game_object_construct_002a7338(base,sizeof(base)-1,&view)==GH_PLAYER_BOUNDS&&base[0]==0xa5);C(!gh_game_object_construct_002a7338(base,sizeof(base),&view)&&view.original_vtable214==0x44ae28&&view.original_kind4ac==0x44adf0&&view.alias_d0==base+0x30&&view.alias_f0==base+0xb0&&view.initial_memory==GH_TITLE_MEMORY_UNSPECIFIED&&base[0x40]==0xa5&&w(base+0x5a8)==0x3f800000);
 C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(p);gh_disc_close(d);puts("PASS actualR100312F00 SAMEarena1690/native107E40 constructor, ORIGINAL_FACTORY_ZERO distinctfromTitleprofile, domaincorrectrelease and shared2A7338 baseexport; no virtualInit ack");return 0;}
