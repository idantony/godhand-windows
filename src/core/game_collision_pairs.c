/* Actual2B8AE0/2B8B50/2B8D88; MIT LICENSE.upstream. */
#include "game_collision_pairs.h"
#include <string.h>
gh_collision_status gh_collision_pairs_construct_002b8ae0(gh_collision_pairs*s,gh_heap*h){
 if(!s||!h)return GH_COLLISION_ARGUMENT;if(s->constructed)return GH_COLLISION_BUSY;
 *s=(gh_collision_pairs){0};s->heap=h;s->constructed=1;return GH_COLLISION_OK;
}
gh_collision_status gh_collision_pair_set_002b8d88(gh_collision_pairs*s,int mode,unsigned a,unsigned b){
 if(!s||!s->constructed||a>=32||b>=32)return GH_COLLISION_ARGUMENT;
 if(mode==1){s->pair_masks[a]|=UINT32_C(1)<<b;s->pair_masks[b]|=UINT32_C(1)<<a;}
 else{s->pair_masks[a]&=~(UINT32_C(1)<<b);s->pair_masks[b]|=~(UINT32_C(1)<<a);}
 return GH_COLLISION_OK;
}
gh_collision_status gh_collision_pairs_initialize_002b8b50(gh_collision_pairs*s){
 static const uint8_t pairs[26][2]={{1,4},{1,6},{1,8},{2,3},{2,5},{2,7},{3,2},{3,6},{3,8},{4,1},{4,5},{4,7},{5,2},{5,4},{5,8},{6,1},{6,3},{6,7},{7,2},{7,4},{7,6},{7,8},{8,1},{8,3},{8,5},{8,7}};
 if(!s||!s->constructed||!s->heap)return GH_COLLISION_ARGUMENT;
 if(s->records.pointer)return GH_COLLISION_BUSY;
 s->used=0;s->count=0;
 /* OriginalarrayFree seesNULL after2BA1B0; nativeowner rejectsdoubleinit above.
  * Virtualallocator2BADF8 usesMemoryAlloc(bytes,16,defaultheap), fill2BAE40
  * receives originalthirdargument(stride*count) in preserveda2. */
 gh_mem_error e=gh_heap_allocate(s->heap,GH_COLLISION_RECORD_BYTES*GH_COLLISION_CAPACITY,16,&s->records);
 s->count=GH_COLLISION_CAPACITY;
 if(e!=GH_MEM_OK)return GH_COLLISION_MEMORY;
 memset(s->records.pointer,0,GH_COLLISION_RECORD_BYTES*GH_COLLISION_CAPACITY);
 for(unsigned i=0;i<26;i++)(void)gh_collision_pair_set_002b8d88(s,1,pairs[i][0],pairs[i][1]);
 return GH_COLLISION_OK;
}
gh_collision_status gh_collision_pairs_release(gh_collision_pairs*s){
 if(!s||!s->constructed||!s->heap)return GH_COLLISION_ARGUMENT;
 if(s->records.pointer&&gh_heap_release(s->heap,s->records)!=GH_MEM_OK)return GH_COLLISION_MEMORY;
 s->records=(gh_r207_allocation){0};s->count=0;s->used=0;return GH_COLLISION_OK;
}
