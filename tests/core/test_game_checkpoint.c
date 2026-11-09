#include "game_checkpoint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
int main(void){gh_game_checkpoint s={0};gh_boot_work w={.room=0x20};gh_checkpoint_bindings b={.work=&w};w.flags598=0x800;CHECK(gh_game_checkpoint_002bf8f8(&s,&b)==GH_CHECKPOINT_SKIPPED&&s.last_skipped&&!s.captured);w.flags598=0;w.flags=0x08000000;CHECK(gh_game_checkpoint_002bf8f8(&s,&b)==GH_CHECKPOINT_SKIPPED);w.flags=0;
 uint8_t*save=malloc(GH_CORE_SAVE_BYTES),*dst=malloc(GH_CHECKPOINT_BYTES);gh_game_prepare*prepare=calloc(1,sizeof(*prepare));gh_game_init_state*init=calloc(1,sizeof(*init));CHECK(save&&dst&&prepare&&init);memset(save,0x9a,GH_CORE_SAVE_BYTES);save[0x154]=3;memset(dst,0xde,GH_CHECKPOINT_BYTES);for(unsigned i=0;i<sizeof(prepare->fields);i++)prepare->fields[i]=(uint8_t)(i*17+3);
 uint16_t stage_b=0x1234;memcpy((uint8_t*)&w+0x5b6,&stage_b,2);put(init->heat,0x42f00000);put(init->heat+4,0x41a00000);
 gh_heap*h;CHECK(gh_heap_create(GH_TITLE_PLAYER_BYTES,&h)==GH_MEM_OK);gh_game_title_player player={0};CHECK(gh_game_title_player_create_0031c4e0(&player,h,&w)==GH_PLAYER_OK);
 b=(gh_checkpoint_bindings){&w,{save,GH_CORE_SAVE_BYTES},prepare,init,&player.view,dst,GH_CHECKPOINT_BYTES};
 CHECK(gh_game_checkpoint_002bf8f8(&s,&b)==GH_CHECKPOINT_UNSUPPORTED&&s.frontier==0x1f9bec&&!s.captured);CHECK(u32(save+0x80)==160&&u32(save+0x84)==0x9a9a9a9a);for(unsigned i=0;i<GH_CHECKPOINT_BYTES;i++)CHECK(dst[i]==0xde);
 CHECK(gh_game_title_player_release(&player)==GH_PLAYER_OK);uint8_t input[GH_TITLE_PLAYER_BYTES];memset(input,0x5a,sizeof(input));input[0x54a]=0xd2;input[0x54b]=4;gh_title_memory_profile profile={GH_TITLE_MEMORY_PROVIDED_BYTES,input,sizeof(input)};
 CHECK(gh_game_title_player_create_reference_0031c4e0(&player,h,&w,&profile)==GH_PLAYER_OK);
 put(player.view.alias_f0,0x3f800000);put(player.view.alias_f0+4,0xc0000000);put(player.view.alias_f0+8,0x40400000);put(player.view.bytes+0x100,0x3e800000);
 CHECK(gh_game_checkpoint_002bf8f8(&s,&b)==GH_CHECKPOINT_OK&&s.captured&&!s.last_skipped&&!s.frontier);
 CHECK(u32(save+0xc)==0x12340020&&u32(save+0x80)==160&&u32(save+0x84)==1234&&u32(save)==0x9a9a9a9a&&save[0x1f]==0x9a);
 CHECK(!memcmp(dst,save,GH_CORE_SAVE_BYTES)&&!memcmp(dst+0x14a0,prepare->fields+0x1660,0x9c30)&&!memcmp(dst+0xb0d0,&s.tail,0x20));
 CHECK(s.tail.position[0]==0x3f800000&&s.tail.position[1]==0xc0000000&&s.tail.position[2]==0x40400000&&s.tail.rotation[0]==0x3e800000&&s.tail.heat==0x41a00000&&s.tail.heat_max==0x42f00000);
 CHECK(gh_game_title_player_release(&player)==GH_PLAYER_OK);profile=(gh_title_memory_profile){GH_TITLE_MEMORY_ZERO_REFERENCE,0,0};CHECK(gh_game_title_player_create_reference_0031c4e0(&player,h,&w,&profile)==GH_PLAYER_OK);w.flags598=0x08000000u;
 CHECK(gh_game_checkpoint_002bf8f8(&s,&b)==GH_CHECKPOINT_OK&&save[0x154]==13&&u32(save+0x80)==360&&u32(save+0x84)==1);
 CHECK(gh_game_title_player_release(&player)==GH_PLAYER_OK&&gh_heap_destroy(h)==GH_MEM_OK);free(save);free(dst);free(prepare);free(init);puts("PASS actualcheckpoint full0 gates/canonicalcopies/suppliedHP1234/zero-referenceHP1/nohostgarbage");return 0;}
