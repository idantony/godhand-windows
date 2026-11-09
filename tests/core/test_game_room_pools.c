#include "game_room_pools.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t u16(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
int main(int argc,char**argv){
 gh_disc*d;CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK);
 const char*paths[]={"st0/r020.dat","st2/r207.dat"};
 for(unsigned sample=0;sample<2;sample++){
  size_t index;gh_disc_entry file;CHECK(gh_disc_find(d,paths[sample],&index)==GH_DISC_OK&&gh_disc_entry_at(d,index,&file)==GH_DISC_OK);
  uint8_t *data=malloc((size_t)file.size);CHECK(data&&gh_disc_read(d,index,0,data,(size_t)file.size)==GH_DISC_OK);
  gh_bundle bundle;gh_bundle_entry rht;CHECK(gh_bundle_open(data,(size_t)file.size,&bundle)==GH_BUNDLE_OK&&gh_bundle_find(&bundle,"RHT",0,&rht)==GH_BUNDLE_OK);
  uint8_t immutable[20];memcpy(immutable,rht.bytes,20);gh_heap*heap;gh_room_pools pools={0};CHECK(gh_heap_create(8*1024*1024,&heap)==GH_MEM_OK);
  CHECK(gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap)==GH_POOL_OK&&pools.initialized);
  uint32_t caps[]={u16(rht.bytes)+u16(rht.bytes+2),u16(rht.bytes),128,u16(rht.bytes+2)};
  size_t total=0;for(unsigned i=0;i<4;i++){CHECK(pools.vectors[i].capacity==caps[i]&&!pools.vectors[i].count&&pools.vectors[i].allocation.pointer);total+=caps[i]*4;}
  uint32_t sizes[]={u16(rht.bytes)*0x380u+0x20,u32(rht.bytes+12),u32(rht.bytes+4)};
  for(unsigned i=0;i<3;i++){
   gh_room_arena*a=&pools.arenas[i];const uint8_t *header=a->allocation.pointer;
   CHECK(a->bytes==sizes[i]&&a->free_payload==sizes[i]-32&&a->first_offset==32);
   CHECK(u32(header)==i+1&&!u32(header+4)&&!u32(header+8)&&u32(header+12)==sizes[i]-32&&u32(header+16)==32);total+=sizes[i];
  }
  for(unsigned pool=0;pool<2;pool++){
   gh_room_block_pool*p=&pools.blocks[pool];uint32_t expected_count=u16(rht.bytes+16+pool*2),handle;void*payload;
   CHECK(p->count==expected_count&&p->payload_bytes==(pool?0x480u:0x220u)&&p->stride==(pool?0x500u:0x230u)&&p->free_head==1&&!p->used_head);
   CHECK(p->bytes20==p->stride*p->count&&p->remaining18==p->count&&p->remaining1c==p->count&&p->limit24==p->count);
   total+=(size_t)p->stride*p->count;
   CHECK(gh_room_block_take_002a97a0(p,0x80000,&handle,&payload)==GH_POOL_OK&&!handle&&!payload&&p->free_head==1);
   for(unsigned i=0;i<p->count;i++){
    uint8_t *n=(uint8_t*)p->allocation.pointer+(size_t)i*p->stride+p->payload_bytes;
    CHECK(u32(n)==i&&u32(n+4)==(i+1==p->count?0:i+2));
   }
  }
  CHECK(gh_heap_used(heap)==total&&!memcmp(immutable,rht.bytes,20));
  CHECK(gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap)==GH_POOL_BUSY);
  for(unsigned pool=0;pool<2;pool++){
   gh_room_block_pool*p=&pools.blocks[pool];uint32_t handle;void*payload;
   for(uint32_t h=1;h<=p->count;h++){CHECK(gh_room_block_take_002a97a0(p,0,&handle,&payload)==GH_POOL_OK&&handle==h&&payload==(uint8_t*)p->allocation.pointer+(size_t)(h-1)*p->stride);}
   CHECK(gh_room_block_take_002a97a0(p,0,&handle,&payload)==GH_POOL_OK&&!handle&&!payload&&p->used_count==p->count);
   CHECK(gh_game_room_pools_release(&pools)==GH_POOL_UNSUPPORTED&&pools.failed_entry==(pool?0x149378u:0x150418u)&&gh_heap_used(heap)==total);
   /* Remove middle node, then recycle it to used-list head exactly. */
   CHECK(gh_room_block_put_002a9800(p,2)==GH_POOL_OK);
   CHECK(gh_room_block_take_002a97a0(p,0,&handle,&payload)==GH_POOL_OK&&handle==2&&p->used_head==2);
   for(uint32_t h=1;h<=p->count;h++)CHECK(gh_room_block_put_002a9800(p,h)==GH_POOL_OK);
   CHECK(!p->used_head&&!p->used_count&&gh_room_block_put_002a9800(p,1)==GH_POOL_BOUNDS);
  }
  printf("%s real RHT nativepayload bytes%zu, blocks%u/%u\n",paths[sample],total,pools.blocks[0].count,pools.blocks[1].count);
  CHECK(gh_game_room_pools_release(&pools)==GH_POOL_OK&&gh_heap_used(heap)==0&&gh_heap_destroy(heap)==GH_MEM_OK);
  gh_room_pools partial={0};CHECK(gh_heap_create(4096,&heap)==GH_MEM_OK);
  CHECK(gh_game_room_pools_initialize_002a5d80(&partial,&bundle,heap)==GH_POOL_MEMORY&&partial.failed_entry==0x3127b0&&partial.vectors[0].allocation.pointer);
  CHECK(gh_game_room_pools_release(&partial)==GH_POOL_OK&&gh_heap_used(heap)==0&&gh_heap_destroy(heap)==GH_MEM_OK);
  free(data);
 }
 gh_disc_close(d);puts("PASS actual RHT vectors/arenas/fixed pools, true all-block allocation/list order, destructor frontier and failure ownership");return 0;
}
