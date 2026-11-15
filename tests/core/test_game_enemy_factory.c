#include "game_enemy_factory.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
int main(void){
 uint8_t bytes[GH_EM202_BYTES+16],base[GH_EM202_BYTES+16];gh_title_player_view v;
 memset(bytes,0xa5,sizeof(bytes));memcpy(base,bytes,sizeof(bytes));
 CHECK(gh_enemy202_construct_00201fa0(bytes,GH_EM202_BYTES-1,&v)==GH_PLAYER_BOUNDS);
 CHECK(memcmp(base,bytes,sizeof(bytes))==0);
 CHECK(gh_enemy202_construct_00201fa0(bytes,GH_EM202_BYTES,&v)==GH_PLAYER_OK);
 CHECK(v.alias_d0==bytes+0x30&&v.alias_f0==bytes+0xb0);
 CHECK(v.original_vtable214==0x4479c8&&v.original_kind4ac==0x42c7f0);
 CHECK(word(bytes,0x214)==0x4479c8&&word(bytes,0x4ac)==0x42c7f0);
 CHECK(bytes[0x2ae]==0x58&&bytes[0x2af]==2&&bytes[0x1651]==255);
 /* Unwritten HP, gaps and allocation suffix preserve supplied input. */
 CHECK(bytes[0x54a]==0xa5&&bytes[0x1670]==0xa5&&bytes[0x197f]==0xa5);
 CHECK(memcmp(bytes+GH_EM202_BYTES,base+GH_EM202_BYTES,16)==0);
 for(unsigned i=0;i<8;i++){CHECK(word(bytes,0x18a0+16*i)==0);CHECK(word(bytes,0x18ac+16*i)==0x3f800000);}
 gh_heap*heap=NULL;CHECK(gh_heap_create(GH_EM202_BYTES,&heap)==GH_MEM_OK);
 gh_game_title_player object={0};gh_boot_work work={0};int original=-1;uint32_t inhibit=0x100000;
 memcpy((uint8_t*)&work+0x594,&inhibit,4);
 CHECK(gh_enemy202_allocate_003130e0(&object,heap,&work,&original)==GH_PLAYER_OK&&original==0);
 CHECK(gh_heap_used(heap)==0);inhibit=0;memcpy((uint8_t*)&work+0x594,&inhibit,4);
 CHECK(gh_enemy202_allocate_003130e0(&object,heap,&work,&original)==GH_PLAYER_OK&&original==1);
 CHECK(gh_heap_used(heap)==GH_EM202_BYTES&&object.view.bytes[0x54a]==0);
 CHECK(object.view.initial_memory==GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO);
 CHECK(gh_enemy202_allocate_003130e0(&object,heap,&work,&original)==GH_PLAYER_BUSY);
 CHECK(gh_game_title_player_release(&object)==GH_PLAYER_OK&&gh_heap_used(heap)==0);
 CHECK(gh_heap_destroy(heap)==GH_MEM_OK);
 CHECK(gh_heap_create(32,&heap)==GH_MEM_OK);
 CHECK(gh_enemy202_allocate_003130e0(&object,heap,&work,&original)==GH_PLAYER_MEMORY);
 CHECK(!object.allocation.pointer&&original==0&&gh_heap_used(heap)==0);
 CHECK(gh_heap_destroy(heap)==GH_MEM_OK);
 puts("enemy202 original constructor/allocation PASS; no virtual Init claim");return 0;
}
