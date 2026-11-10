#include "game_scene_exit_leaves.h"
#include <string.h>
static uint32_t u32(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void w32(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
gh_collision_status gh_collision_pairs_clear_002a71f0(gh_collision_pairs*s,uint32_t*f){
 if(f)*f=0;if(!s||!s->constructed)return GH_COLLISION_ARGUMENT;
 if(!s->records.pointer)return GH_COLLISION_OK;
 if(s->count> GH_COLLISION_CAPACITY)return GH_COLLISION_ARGUMENT;
 for(uint32_t i=0;i<s->count;i++)if((u32((uint8_t*)s->records.pointer+(size_t)i*GH_COLLISION_RECORD_BYTES)&0x201)==1){if(f)*f=0x2bae98;return GH_COLLISION_BUSY;}
 return GH_COLLISION_OK;
}
gh_collision_status gh_collision_pairs_free_002a7280(gh_collision_pairs*s,int*original){
 if(!s||!s->constructed||!s->heap||!original)return GH_COLLISION_ARGUMENT;*original=s->records.pointer!=NULL;
 if(s->records.pointer&&gh_heap_release(s->heap,s->records)!=GH_MEM_OK)return GH_COLLISION_MEMORY;
 s->records=(gh_r207_allocation){0};return GH_COLLISION_OK;
}
gh_light_status gh_world_light_exit_002d73e0(gh_game_world_light*s){
 if(!s||!s->constructed)return GH_LIGHT_ARGUMENT;
 /* Native ownedpreset release is the actual8slot free loop. Other borrowed
  * resourceviews and untouched scalars survive exactly as original. */
 gh_light_status e=gh_game_world_light_release_presets(s);if(e)return e;
 memset(s->scalar+0x80,0,0x7000);
 for(unsigned i=0;i<256;i++){uint8_t*p=s->scalar+0x80+(size_t)i*0x70;p[0]=0xff;p[1]=0;}
 w32(s->scalar+0x78,0);
 for(unsigned i=0;i<8;i++){w32(s->scalar+0x15d74+i*0x80,0);w32(s->scalar+0x16100+i*4,0);}
 w32(s->scalar+0x16120,0);w32(s->scalar+0x16130,0);memset(s->native_light_owners,0,sizeof(s->native_light_owners));return GH_LIGHT_OK;
}
