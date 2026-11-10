#include "game_room_pools.h"
#include <stdlib.h>
#include <string.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int s16(const uint8_t*p){unsigned n=p[0]|(unsigned)p[1]<<8;return n>=32768?(int)n-65536:(int)n;}
static void w32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static gh_room_pool_status alloc(gh_room_pools*s,size_t n,size_t alignment,gh_r207_allocation*out,uint32_t entry){
 gh_mem_error e=gh_heap_allocate(s->heap,n,alignment,out);if(e){s->failed_entry=entry;s->detail=e;return GH_POOL_MEMORY;}return GH_POOL_OK;
}
static uint8_t *node(gh_room_block_pool*p,uint32_t handle){return (uint8_t*)p->allocation.pointer+(size_t)(handle-1)*p->stride+p->payload_bytes;}
static gh_room_pool_status block_initialize(gh_room_pools*s,unsigned i,uint32_t payload,uint32_t count,uint32_t align){
 gh_room_block_pool*p=&s->blocks[i];uint64_t stride=((uint64_t)payload+7+align)&~((uint64_t)align-1),size=stride*count;
 if(!count||size>UINT32_MAX){s->failed_entry=i?0x149400:0x1504b0;return GH_POOL_BOUNDS;}
 gh_room_pool_status e=alloc(s,(size_t)size,align,&p->allocation,i?0x149400:0x1504b0);if(e)return e;
 p->payload_bytes=payload;p->stride=(uint32_t)stride;p->count=count;p->free_head=1;
 p->bytes20=(uint32_t)size;p->remaining18=p->remaining1c=p->limit24=count;
 p->active=calloc(count,1);if(!p->active){s->failed_entry=i?0x149400:0x1504b0;return GH_POOL_MEMORY;}
 for(uint32_t h=1;h<=count;h++){uint8_t*n=node(p,h);w32(n,h==1?0:h-1);w32(n+4,h==count?0:h+1);}
 return GH_POOL_OK;
}
gh_room_pool_status gh_game_room_pools_initialize_002a5d80(gh_room_pools*s,const gh_bundle*bundle,gh_heap*heap){
 gh_bundle_entry r;uint32_t counts[4],sizes[3];int a,b;gh_room_pool_status e;
 if(!s||!bundle||!heap)return GH_POOL_ARGUMENT;
 for(unsigned i=0;i<4;i++)if(s->vectors[i].allocation.pointer)return GH_POOL_BUSY;
 for(unsigned i=0;i<3;i++)if(s->arenas[i].allocation.pointer)return GH_POOL_BUSY;
 for(unsigned i=0;i<2;i++)if(s->blocks[i].allocation.pointer)return GH_POOL_BUSY;
 if(gh_bundle_find(bundle,"RHT",0,&r)!=GH_BUNDLE_OK||r.size<20)return GH_POOL_BOUNDS;
 a=s16(r.bytes);b=s16(r.bytes+2);if(a<0||b<0||(uint64_t)(unsigned)a*0x380+0x20>UINT32_MAX)return GH_POOL_BOUNDS;
 counts[0]=(uint32_t)(a+b);counts[1]=(uint32_t)a;counts[2]=128;counts[3]=(uint32_t)b;
 sizes[0]=(uint32_t)a*0x380+0x20;sizes[1]=u32(r.bytes+12);sizes[2]=u32(r.bytes+4);
 for(unsigned i=0;i<3;i++)if(sizes[i]<0x20)return GH_POOL_BOUNDS;
 int c=s16(r.bytes+16),d=s16(r.bytes+18);if(c<=0||d<=0)return GH_POOL_BOUNDS;
 s->heap=heap;s->failed_entry=0;s->detail=0;s->initialized=0;
 /*3127B0 makes four emptyvector/reserves; payloads stayuninitialized. */
 for(unsigned i=0;i<4;i++){s->vectors[i].count=0;s->vectors[i].capacity=counts[i];if(counts[i]){e=alloc(s,(size_t)counts[i]*4,16,&s->vectors[i].allocation,0x3127b0);if(e)return e;}}
 for(unsigned i=0;i<3;i++){
  gh_room_arena*p=&s->arenas[i];p->native_owner=s;e=alloc(s,sizes[i],16,&p->allocation,0x30ee08);if(e)return e;
  p->bytes=sizes[i];p->free_payload=sizes[i]-0x20;p->first_offset=0x20;
  /*2A9498->2A94F0. Ownerpointer becomesnativearenaidentityi+1. */
  uint8_t*raw=p->allocation.pointer;w32(raw,i+1);w32(raw+4,0);w32(raw+8,0);w32(raw+12,p->free_payload);w32(raw+16,0x20);
 }
 e=block_initialize(s,0,0x220,(uint32_t)c,0x10);if(e)return e;
 e=block_initialize(s,1,0x480,(uint32_t)d,0x80);if(e)return e;
 s->initialized=1;return GH_POOL_OK;
}
gh_room_pool_status gh_room_block_take_002a97a0(gh_room_block_pool*p,uint32_t flags,uint32_t*handle,void**payload){
 if(!p||!handle||!payload||!p->allocation.pointer||!p->active)return GH_POOL_ARGUMENT;*handle=0;*payload=NULL;
 if((flags&0x80000)||!p->free_head)return GH_POOL_OK;
 uint32_t h=p->free_head;if(h>p->count||p->active[h-1])return GH_POOL_BOUNDS;uint8_t*n=node(p,h);uint32_t next=u32(n+4);
 if(next>p->count||p->used_head>p->count)return GH_POOL_BOUNDS;p->free_head=next;if(next)w32(node(p,next),0);
 w32(n,0);w32(n+4,p->used_head);if(p->used_head)w32(node(p,p->used_head),h);p->used_head=h;p->used_count++;p->active[h-1]=1;
 *handle=h;*payload=(uint8_t*)p->allocation.pointer+(size_t)(h-1)*p->stride;return GH_POOL_OK;
}
gh_room_pool_status gh_room_block_put_002a9800(gh_room_block_pool*p,uint32_t h){
 if(!p||!p->allocation.pointer||!p->active)return GH_POOL_ARGUMENT;if(!h)return GH_POOL_OK;
 if(h>p->count||!p->active[h-1])return GH_POOL_BOUNDS;uint8_t*n=node(p,h);uint32_t prev=u32(n),next=u32(n+4);
 if(prev>p->count||next>p->count||p->free_head>p->count)return GH_POOL_BOUNDS;
 if(prev)w32(node(p,prev)+4,next);else p->used_head=next;if(next)w32(node(p,next),prev);
 w32(n,0);w32(n+4,p->free_head);if(p->free_head)w32(node(p,p->free_head),h);p->free_head=h;p->used_count--;p->active[h-1]=0;return GH_POOL_OK;
}
gh_room_pool_status gh_game_room_pools_release(gh_room_pools*s){
 if(!s||!s->heap)return GH_POOL_ARGUMENT;
 for(size_t i=0;i<s->object_slots;i++)if(s->objects[i].object){s->failed_entry=0x30f8b0;return GH_POOL_UNSUPPORTED;}
 for(unsigned i=0;i<3;i++)if(s->arenas[i].live_allocations){s->failed_entry=0x2a9680;return GH_POOL_UNSUPPORTED;}
 for(unsigned i=0;i<2;i++)if(s->blocks[i].used_count){s->failed_entry=i?0x149378:0x150418;return GH_POOL_UNSUPPORTED;}
 for(unsigned i=0;i<2;i++){if(s->blocks[i].allocation.pointer&&gh_heap_release(s->heap,s->blocks[i].allocation)!=GH_MEM_OK)return GH_POOL_MEMORY;free(s->blocks[i].active);memset(&s->blocks[i],0,sizeof(s->blocks[i]));}
 for(unsigned i=0;i<3;i++){if(gh_room_arena_dispose_metadata(&s->arenas[i]))return GH_POOL_BUSY;if(s->arenas[i].allocation.pointer&&gh_heap_release(s->heap,s->arenas[i].allocation)!=GH_MEM_OK)return GH_POOL_MEMORY;memset(&s->arenas[i],0,sizeof(s->arenas[i]));}
 for(unsigned i=0;i<4;i++){if(s->vectors[i].allocation.pointer&&gh_heap_release(s->heap,s->vectors[i].allocation)!=GH_MEM_OK)return GH_POOL_MEMORY;memset(&s->vectors[i],0,sizeof(s->vectors[i]));}
 free(s->objects);s->objects=NULL;s->object_slots=s->object_capacity=0;s->next_object_handle=0;s->next_arena_handle=0;
 s->initialized=0;s->failed_entry=0;s->detail=0;return GH_POOL_OK;
}

gh_room_pool_status gh_room_registry_view(const gh_room_pools*s,uint32_t address,const gh_room_vector**out){
 static const uint32_t globals[]={0x754c30,0x754c38,0x754c50,0x754c58};
 if(!out)return GH_POOL_ARGUMENT;*out=NULL;
 if(!s)return GH_POOL_ARGUMENT;
 if(!s->initialized)return GH_POOL_BUSY;
 for(unsigned i=0;i<4;++i)if(address==globals[i]){
  const gh_room_vector*v=&s->vectors[i];
  if(v->count>v->capacity||(v->capacity&&!v->allocation.pointer))return GH_POOL_BOUNDS;
  *out=v;return GH_POOL_OK;
 }
 return GH_POOL_ARGUMENT;
}

/* Typed host ownership replaces guest pointer representation only. */
gh_room_pool_status gh_room_object_register(gh_room_pools*s,void*object,const void*owner,uint32_t kind,uint32_t mask,uint32_t*out){
 if(!s||!object||!owner||!kind||!out||!mask||(mask&~15u))return GH_POOL_ARGUMENT;
 if(!s->initialized)return GH_POOL_BUSY;
 for(size_t i=0;i<s->object_slots;i++)if(s->objects[i].object==object)return GH_POOL_BUSY;
 for(unsigned i=0;i<4;i++){
  const gh_room_vector*v=NULL;static const uint32_t a[]={0x754c30,0x754c38,0x754c50,0x754c58};
  gh_room_pool_status e=gh_room_registry_view(s,a[i],&v);if(e)return e;
  if((mask&(1u<<i))&&v->count==v->capacity)return GH_POOL_MEMORY;
 }
 if(s->next_object_handle==UINT32_MAX)return GH_POOL_BOUNDS;
 size_t slot=0;while(slot<s->object_slots&&s->objects[slot].object)slot++;
 if(slot==s->object_capacity){
  size_t n=s->object_capacity?s->object_capacity*2:16;
  if(n<s->object_capacity||n>SIZE_MAX/sizeof(*s->objects))return GH_POOL_BOUNDS;
  gh_room_object_binding*p=realloc(s->objects,n*sizeof(*p));if(!p)return GH_POOL_MEMORY;
  memset(p+s->object_capacity,0,(n-s->object_capacity)*sizeof(*p));s->objects=p;s->object_capacity=n;
 }
 uint32_t handle=++s->next_object_handle;
 s->objects[slot]=(gh_room_object_binding){object,owner,kind,handle};if(slot==s->object_slots)s->object_slots++;
 for(unsigned i=0;i<4;i++)if(mask&(1u<<i)){gh_room_vector*v=&s->vectors[i];w32((uint8_t*)v->allocation.pointer+(size_t)v->count*4,handle);v->count++;}
 *out=handle;return GH_POOL_OK;
}
gh_room_pool_status gh_room_object_lookup(const gh_room_pools*s,uint32_t h,uint32_t kind,const void*owner,void**out){
 if(!s||!out||!h||!kind||!owner)return GH_POOL_ARGUMENT;*out=NULL;
 if(!s->initialized)return GH_POOL_BUSY;
 for(size_t i=0;i<s->object_slots;i++)if(s->objects[i].object&&s->objects[i].handle==h){
  const gh_room_object_binding*b=&s->objects[i];if(b->kind!=kind||b->owner!=owner)return GH_POOL_ARGUMENT;
  *out=b->object;return GH_POOL_OK;
 }
 return GH_POOL_BOUNDS;
}
gh_room_pool_status gh_room_object_unregister(gh_room_pools*s,uint32_t h,const void*owner){
 if(!s||!h||!owner)return GH_POOL_ARGUMENT;if(!s->initialized)return GH_POOL_BUSY;
 size_t slot=0;while(slot<s->object_slots&&(!s->objects[slot].object||s->objects[slot].handle!=h))slot++;
 if(slot==s->object_slots)return GH_POOL_BOUNDS;if(s->objects[slot].owner!=owner)return GH_POOL_ARGUMENT;
 /* Validate all lists before stable erasing any of them. */
 for(unsigned i=0;i<4;i++)if(s->vectors[i].count>s->vectors[i].capacity||(s->vectors[i].count&&!s->vectors[i].allocation.pointer))return GH_POOL_BOUNDS;
 for(unsigned i=0;i<4;i++){gh_room_vector*v=&s->vectors[i];uint8_t*p=v->allocation.pointer;uint32_t n=0;
  for(uint32_t j=0;j<v->count;j++){uint32_t x=u32(p+(size_t)j*4);if(x!=h)w32(p+(size_t)n++*4,x);}v->count=n;
 }
 memset(&s->objects[slot],0,sizeof(s->objects[slot]));return GH_POOL_OK;
}


static void exit_metadata(gh_room_pools*s){
 for(unsigned i=0;i<4;i++)if(s->vectors[i].allocation.pointer)return;
 for(unsigned i=0;i<3;i++)if(s->arenas[i].allocation.pointer)return;
 for(unsigned i=0;i<2;i++)if(s->blocks[i].allocation.pointer)return;
 s->initialized=0; /* host construction state only; shared handle serial survives */
}
gh_room_pool_status gh_room_arena_clear_002a9528(gh_room_pools*s,uint32_t address){
 if(!s||!s->heap)return GH_POOL_ARGUMENT;unsigned i;if(address==0x754c20)i=0;else if(address==0x754240)i=1;else if(address==0x754c10)i=2;else return GH_POOL_ARGUMENT;
 /* Original two manager pointers become NULL. Native bounded backing is
  * individually owned rather than a suballocation of original roomheap;
  * relinquish that SAME backing at the proven end of manager lifetime. */
 gh_room_arena*p=&s->arenas[i];if(p->live_allocations){s->failed_entry=0x2a9680;return GH_POOL_UNSUPPORTED;}if(gh_room_arena_dispose_metadata(p))return GH_POOL_BUSY;if(p->allocation.pointer&&gh_heap_release(s->heap,p->allocation)!=GH_MEM_OK){s->failed_entry=0x2a9528;return GH_POOL_MEMORY;}memset(p,0,sizeof(*p));exit_metadata(s);return GH_POOL_OK;
}
gh_room_pool_status gh_room_registries_release_0030f5f0(gh_room_pools*s){
 if(!s||!s->heap)return GH_POOL_ARGUMENT;
 for(size_t i=0;i<s->object_slots;i++)if(s->objects[i].object){s->failed_entry=0x30f8b0;return GH_POOL_UNSUPPORTED;}
 for(unsigned i=0;i<4;i++)if(s->vectors[i].count){s->failed_entry=0x3128b0;return GH_POOL_UNSUPPORTED;}
 for(unsigned i=0;i<4;i++){gh_room_vector*v=&s->vectors[i];if(v->allocation.pointer&&gh_heap_release(s->heap,v->allocation)!=GH_MEM_OK){s->failed_entry=0x3128b0;return GH_POOL_MEMORY;}memset(v,0,sizeof(*v));}
 gh_room_pool_status e=gh_room_arena_clear_002a9528(s,0x754c20);if(e)return e;return gh_room_arena_clear_002a9528(s,0x754c10);
}
static gh_room_pool_status release_empty_block(gh_room_pools*s,unsigned i){
 if(!s||!s->heap)return GH_POOL_ARGUMENT;gh_room_block_pool*p=&s->blocks[i];
 if(p->used_count||p->used_head){s->failed_entry=i?0x149378:0x150418;return GH_POOL_UNSUPPORTED;}
 if(p->allocation.pointer&&gh_heap_release(s->heap,p->allocation)!=GH_MEM_OK){s->failed_entry=i?0x149378:0x150418;return GH_POOL_MEMORY;}
 free(p->active);memset(p,0,sizeof(*p));exit_metadata(s);return GH_POOL_OK;
}
gh_room_pool_status gh_room_model_pool_release_00150078(gh_room_pools*s){return release_empty_block(s,0);}
gh_room_pool_status gh_room_aux_pool_release_001492c8(gh_room_pools*s){return release_empty_block(s,1);}
