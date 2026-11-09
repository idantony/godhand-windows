#include "game_collision_pairs.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){gh_heap*h;gh_collision_pairs s={0};CHECK(gh_heap_create(300000,&h)==GH_MEM_OK);
 CHECK(gh_collision_pairs_construct_002b8ae0(&s,h)==GH_COLLISION_OK);
 CHECK(gh_collision_pair_set_002b8d88(&s,1,31,0)==GH_COLLISION_OK);
 CHECK(gh_collision_pairs_initialize_002b8b50(&s)==GH_COLLISION_OK);
 CHECK(s.count==400&&s.used==0&&gh_heap_used(h)==249600&&!((uintptr_t)s.records.pointer&15));
 for(unsigned i=0;i<249600;i++)CHECK(((uint8_t*)s.records.pointer)[i]==0);
 const uint32_t expected[9]={0x80000000,0x150,0xa8,0x144,0xa2,0x114,0x8a,0x154,0xaa};
 for(unsigned i=0;i<9;i++)CHECK(s.pair_masks[i]==expected[i]);CHECK(s.pair_masks[31]==1);
 for(unsigned i=1;i<=8;i++)for(unsigned j=1;j<=8;j++)CHECK(((s.pair_masks[i]>>j)&1)==((s.pair_masks[j]>>i)&1));
 void*p=s.records.pointer;CHECK(gh_collision_pairs_initialize_002b8b50(&s)==GH_COLLISION_BUSY&&s.records.pointer==p);
 CHECK(gh_collision_pair_set_002b8d88(&s,0,1,4)==GH_COLLISION_OK);CHECK(s.pair_masks[1]==0x140&&s.pair_masks[4]==0xffffffffu);
 CHECK(gh_collision_pair_set_002b8d88(&s,9,31,31)==GH_COLLISION_OK&&s.pair_masks[31]==0x7fffffffu);
 CHECK(gh_collision_pair_set_002b8d88(&s,1,32,0)==GH_COLLISION_ARGUMENT);
 CHECK(gh_collision_pairs_release(&s)==GH_COLLISION_OK&&gh_heap_used(h)==0&&s.pair_masks[31]==0x7fffffff);
 CHECK(gh_heap_destroy(h)==GH_MEM_OK);
 gh_collision_pairs small={0};CHECK(gh_heap_create(249599,&h)==GH_MEM_OK);CHECK(gh_collision_pairs_construct_002b8ae0(&small,h)==GH_COLLISION_OK);CHECK(gh_collision_pairs_initialize_002b8b50(&small)==GH_COLLISION_MEMORY&&small.count==400&&!small.records.pointer&&gh_heap_used(h)==0);CHECK(gh_heap_destroy(h)==GH_MEM_OK);
 puts("PASS collision pair realallocation/defaultmatrix/preservation/originaldisablequirk/bounds");return 0;}
