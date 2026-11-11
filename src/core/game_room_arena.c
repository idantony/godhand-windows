#include "game_room_pools.h"
#include <stdlib.h>
#include <string.h>
typedef struct {uint32_t handle,offset,consumed,free_bytes,previous,next;uint8_t active;} arena_node;
struct gh_room_arena_native {gh_room_pools *owner;arena_node *nodes;size_t count,capacity;};
static void w(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static gh_room_arena*arena(gh_room_pools*s,uint32_t addr){if(!s)return NULL;if(addr==0x754c20)return&s->arenas[0];if(addr==0x754240)return&s->arenas[1];if(addr==0x754c10)return&s->arenas[2];return NULL;}
static void header(gh_room_arena*a,arena_node*n,uint32_t owner_id){uint8_t*p=(uint8_t*)a->allocation.pointer+n->offset;w(p,owner_id);w(p+4,0);w(p+8,0);w(p+12,n->free_bytes);w(p+16,n->consumed);}
gh_room_pool_status gh_room_arena_allocate_002a9538(gh_room_pools*s,uint32_t addr,uint32_t flags,uint32_t bytes,uint32_t alignment,gh_room_arena_allocation*out){
 if(!out)return GH_POOL_ARGUMENT;*out=(gh_room_arena_allocation){0};gh_room_arena*a=arena(s,addr);if(!a||!s->heap||a->native_owner!=s)return GH_POOL_ARGUMENT;
 if((flags&0x80000)||!bytes)return GH_POOL_OK;if(!alignment||(alignment&(alignment-1)))return GH_POOL_ARGUMENT;if(bytes>UINT32_MAX-32)return GH_POOL_BOUNDS;
 if(!a->allocation.pointer||a->bytes<32)return GH_POOL_OK;
 if(!a->native){struct gh_room_arena_native*t=calloc(1,sizeof(*t));if(!t)return GH_POOL_MEMORY;t->capacity=16;t->nodes=calloc(t->capacity,sizeof(*t->nodes));if(!t->nodes){free(t);return GH_POOL_MEMORY;}t->owner=s;t->count=1;t->nodes[0]=(arena_node){0,0,32,a->bytes-32,0,0,1};a->native=t;}
 struct gh_room_arena_native*t=a->native;if(t->owner!=s)return GH_POOL_ARGUMENT;uint32_t current=0;uintptr_t base=(uintptr_t)a->allocation.pointer;size_t guard=0;
 do{if(current>=t->count||++guard>t->count)return GH_POOL_BOUNDS;arena_node*n=&t->nodes[current];if(!n->active||n->offset>a->bytes||n->consumed>a->bytes-n->offset||n->next>=t->count)return GH_POOL_BOUNDS;
  uint64_t start=(uint64_t)n->offset+n->consumed;uintptr_t first=base+(uintptr_t)start;if(first<base||UINTPTR_MAX-first<31u+alignment)return GH_POOL_BOUNDS;uintptr_t ptr=(first+31u+alignment)&~((uintptr_t)alignment-1);if(ptr<base||ptr-base<32)return GH_POOL_BOUNDS;uint64_t offset=(uint64_t)(ptr-base)-32,padding=offset-start,total=(uint64_t)bytes+32+padding;
  if(total<=n->free_bytes){if(offset>a->bytes-32||bytes>a->bytes-offset-32||s->next_arena_handle==UINT32_MAX)return GH_POOL_BOUNDS;size_t slot=1;while(slot<t->count&&t->nodes[slot].active)slot++;
   if(slot==t->capacity){size_t cap=t->capacity*2;if(cap>UINT32_MAX||cap>SIZE_MAX/sizeof(*t->nodes))return GH_POOL_BOUNDS;arena_node*nodes=realloc(t->nodes,cap*sizeof(*nodes));if(!nodes)return GH_POOL_MEMORY;memset(nodes+t->capacity,0,(cap-t->capacity)*sizeof(*nodes));t->nodes=nodes;t->capacity=cap;n=&nodes[current];}
   uint32_t next=n->next,handle=++s->next_arena_handle;t->nodes[slot]=(arena_node){handle,(uint32_t)offset,bytes+32,n->free_bytes-(uint32_t)total,current,next,1};n->next=(uint32_t)slot;n->free_bytes=(uint32_t)padding;if(next){if(next>=t->count)return GH_POOL_BOUNDS;t->nodes[next].previous=(uint32_t)slot;}if(slot==t->count)t->count++;
   unsigned owner_id=(unsigned)(a-s->arenas)+1;header(a,n,owner_id);header(a,&t->nodes[slot],owner_id);a->free_payload=t->nodes[0].free_bytes;a->live_allocations++;*out=(gh_room_arena_allocation){s,(void*)ptr,addr,handle,bytes};return GH_POOL_OK;
  }current=n->next;
 }while(current);return GH_POOL_OK;
}
gh_room_pool_status gh_room_arena_free_002a9680(gh_room_pools*s,gh_room_arena_allocation allocation){
 if(!s)return GH_POOL_ARGUMENT;if(!allocation.pointer)return GH_POOL_OK;if(allocation.pools!=s)return GH_POOL_ARGUMENT;gh_room_arena*a=arena(s,allocation.original_global);if(!a||a->native_owner!=s||!a->native||!a->allocation.pointer||!allocation.pointer||!allocation.handle)return GH_POOL_ARGUMENT;struct gh_room_arena_native*t=a->native;if(t->owner!=s)return GH_POOL_ARGUMENT;
 size_t i=1;while(i<t->count&&(!t->nodes[i].active||t->nodes[i].handle!=allocation.handle))i++;if(i==t->count)return GH_POOL_ARGUMENT;arena_node*n=&t->nodes[i];if((uint8_t*)a->allocation.pointer+n->offset+32!=allocation.pointer||n->consumed!=allocation.bytes+32||n->previous>=t->count||n->next>=t->count||!a->live_allocations)return GH_POOL_ARGUMENT;
 arena_node*previous=&t->nodes[n->previous];if(!previous->active||previous->next!=i||(n->next&&t->nodes[n->next].previous!=i))return GH_POOL_BOUNDS;uint64_t merged=(uint64_t)previous->free_bytes+n->consumed+n->free_bytes;if(merged>UINT32_MAX)return GH_POOL_BOUNDS;previous->next=n->next;previous->free_bytes=(uint32_t)merged;if(n->next)t->nodes[n->next].previous=n->previous;n->active=0;a->live_allocations--;header(a,previous,(unsigned)(a-s->arenas)+1);a->free_payload=t->nodes[0].free_bytes;return GH_POOL_OK;
}
/* ESP14's original factory reserves 0x4E0 bytes, but the visibility sweep
 * reads these two offsets from its address in the shared room arena. */
gh_room_pool_status gh_room_arena_model_visibility_field(
    const gh_room_arena_allocation *object, unsigned offset, uint8_t **out)
{
    if (!out) return GH_POOL_ARGUMENT;
    *out = NULL;
    const size_t width = offset == 0x538 ? 8 : offset == 0x5a0 ? 4 : 0;
    if (!width || !object || !object->pointer || !object->handle ||
        object->original_global != 0x754c10 || object->bytes != 0x4e0)
        return GH_POOL_ARGUMENT;
    const gh_room_arena *a = arena(object->pools, object->original_global);
    if (!a || a->native_owner != object->pools || !a->allocation.pointer ||
        !a->native || a->native->owner != object->pools || a->bytes < 32)
        return GH_POOL_ARGUMENT;

    const struct gh_room_arena_native *native = a->native;
    size_t index = 1;
    while (index < native->count && (!native->nodes[index].active ||
        native->nodes[index].handle != object->handle))
        ++index;
    if (index == native->count) return GH_POOL_ARGUMENT;
    const arena_node *node = &native->nodes[index];
    if (node->offset > a->bytes - 32 || node->consumed != object->bytes + 32 ||
        node->consumed > a->bytes - node->offset)
        return GH_POOL_BOUNDS;
    uint8_t *base = a->allocation.pointer;
    size_t payload = (size_t)node->offset + 32;
    if (base + payload != object->pointer) return GH_POOL_ARGUMENT;
    if (offset > a->bytes - payload || width > a->bytes - payload - offset)
        return GH_POOL_BOUNDS;
    size_t start = payload + offset, end = start + width;
    for (size_t i = 0; i < native->count; ++i) {
        const arena_node *block = &native->nodes[i];
        if (!block->active) continue;
        if (block->offset > a->bytes - 32 ||
            ((size_t)block->offset < end && start < (size_t)block->offset + 32))
            return GH_POOL_BOUNDS;
    }
    *out = base + start;
    return GH_POOL_OK;
}

gh_room_pool_status gh_room_arena_dispose_metadata(gh_room_arena*a){if(!a)return GH_POOL_ARGUMENT;if(a->live_allocations)return GH_POOL_BUSY;if(a->native){free(a->native->nodes);free(a->native);a->native=NULL;}return GH_POOL_OK;}
