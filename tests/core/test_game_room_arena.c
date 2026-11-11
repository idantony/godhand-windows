#include "game_room_pools.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static void test_visibility_alias(gh_room_pools *pools)
{
    gh_room_arena_allocation model, sibling, pad;
    uint8_t *key, *flags;
    C(!gh_room_arena_allocate_002a9538(pools,0x754c10,0,0x4e0,16,&model) && model.pointer);
    C(!gh_room_arena_allocate_002a9538(pools,0x754c10,0,0x4e0,16,&sibling) && sibling.pointer);
    uint8_t *neighbor=sibling.pointer;
    C(!gh_room_arena_model_visibility_field(&model,0x538,&key) && key==neighbor+0x38);
    C(!gh_room_arena_model_visibility_field(&model,0x5a0,&flags) && flags==neighbor+0xa0);
    uint32_t old=0x3f800003u, updated;
    memcpy(neighbor+0xa0,&old,4);
    memcpy(&updated,flags,4);
    updated&=~2u;
    memcpy(flags,&updated,4);
    memcpy(&updated,neighbor+0xa0,4);
    C(updated==0x3f800001u);
    C(!gh_room_arena_free_002a9680(pools,sibling));
    C(!gh_room_arena_allocate_002a9538(pools,0x754c10,0,0x30,16,&pad) && pad.pointer);
    C(!gh_room_arena_allocate_002a9538(pools,0x754c10,0,0x4e0,16,&sibling) && sibling.pointer);
    C(gh_room_arena_model_visibility_field(&model,0x538,&key)==GH_POOL_BOUNDS && !key);
    C(!gh_room_arena_model_visibility_field(&model,0x5a0,&flags) &&
        flags==(uint8_t*)sibling.pointer+0x50);
    C(gh_room_arena_model_visibility_field(&model,0x53c,&key)==GH_POOL_ARGUMENT && !key);
    C(!gh_room_arena_free_002a9680(pools,model));
    C(gh_room_arena_model_visibility_field(&model,0x5a0,&flags)==GH_POOL_ARGUMENT && !flags);
    C(!gh_room_arena_free_002a9680(pools,pad) && !gh_room_arena_free_002a9680(pools,sibling));
}
int main(int argc,char**argv){gh_disc*d;size_t index;gh_disc_entry entry;C(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&index)&&!gh_disc_entry_at(d,index,&entry));uint8_t*p=malloc((size_t)entry.size);C(p&&!gh_disc_read(d,index,0,p,(size_t)entry.size));gh_bundle b;C(!gh_bundle_open(p,(size_t)entry.size,&b));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&b,heap));size_t used=gh_heap_used(heap);gh_room_arena*a=&pools.arenas[2];gh_room_arena_allocation first,second,third,reused,small;
 C(!gh_room_arena_allocate_002a9538(&pools,0x754c10,0x80000,100,16,&first)&&!first.pointer&&!a->native&&!a->live_allocations);C(!gh_room_arena_allocate_002a9538(&pools,0x754c10,0,0,16,&first)&&!first.pointer);C(gh_room_arena_allocate_002a9538(&pools,0x754c10,0,32,3,&first)==GH_POOL_ARGUMENT);
 C(!gh_room_arena_allocate_002a9538(&pools,0x754c10,0,0x1690,16,&first)&&first.pointer&&!((uintptr_t)first.pointer&15));C((uint8_t*)first.pointer>=(uint8_t*)a->allocation.pointer+32&&(uint8_t*)first.pointer+first.bytes<=(uint8_t*)a->allocation.pointer+a->bytes);C(!gh_room_arena_allocate_002a9538(&pools,0x754c10,0,257,128,&second)&&second.pointer&&!((uintptr_t)second.pointer&127));C(!gh_room_arena_allocate_002a9538(&pools,0x754c10,0,64,16,&third)&&third.pointer&&a->live_allocations==3&&gh_heap_used(heap)==used);memset(first.pointer,0xab,first.bytes);memset(second.pointer,0xbc,second.bytes);memset(third.pointer,0xcd,third.bytes);
 C(!gh_room_arena_free_002a9680(&pools,second));C(!gh_room_arena_allocate_002a9538(&pools,0x754c10,0,128,64,&reused)&&reused.pointer&&reused.handle!=second.handle&&(((uint8_t*)reused.pointer+128<=(uint8_t*)third.pointer)||((uint8_t*)third.pointer+64<=(uint8_t*)reused.pointer)));C(!gh_room_arena_free_002a9680(&pools,first));C(!gh_room_arena_allocate_002a9538(&pools,0x754c10,0,32,16,&small)&&small.pointer==first.pointer&&small.handle!=first.handle);C(gh_room_arena_free_002a9680(&pools,first)==GH_POOL_ARGUMENT);C(((uint8_t*)third.pointer)[0]==0xcd);
 gh_room_pools foreign=pools;C(gh_room_arena_free_002a9680(&foreign,third)==GH_POOL_ARGUMENT);gh_room_arena_allocation refused;C(gh_room_arena_allocate_002a9538(&foreign,0x754c10,0,8,16,&refused)==GH_POOL_ARGUMENT&&!refused.pointer);C(gh_room_arena_clear_002a9528(&pools,0x754c10)==GH_POOL_UNSUPPORTED&&pools.failed_entry==0x2a9680&&a->live_allocations==3);C(gh_game_room_pools_release(&pools)==GH_POOL_UNSUPPORTED&&gh_heap_used(heap)==used);
 C(!gh_room_arena_free_002a9680(&pools,third)&&!gh_room_arena_free_002a9680(&pools,reused)&&!gh_room_arena_free_002a9680(&pools,small)&&!a->live_allocations&&a->free_payload==a->bytes-32);uint32_t serial=pools.next_arena_handle;C(!gh_room_arena_dispose_metadata(a)&&!a->native&&!gh_room_arena_allocate_002a9538(&pools,0x754c10,0,48,16,&small)&&small.handle>serial&&!gh_room_arena_free_002a9680(&pools,small));C(!gh_room_arena_free_002a9680(&pools,(gh_room_arena_allocation){0}));C(!gh_room_arena_allocate_002a9538(&pools,0x754c10,0,UINT32_MAX-32,16,&small)&&!small.pointer);test_visibility_alias(&pools);C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(p);gh_disc_close(d);puts("PASS realR100arena boundedfirstfit/alignment/split/nonLIFOfree/merge, SAMEbacking noextraheapusage, liveowner guards and stale/foreignhandle rejection");return 0;}
