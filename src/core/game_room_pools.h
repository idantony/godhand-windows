#ifndef GH_GAME_ROOM_POOLS_H
#define GH_GAME_ROOM_POOLS_H
#include "../platform/memory.h"
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_POOL_OK,GH_POOL_ARGUMENT,GH_POOL_BOUNDS,GH_POOL_MEMORY,GH_POOL_BUSY,GH_POOL_UNSUPPORTED} gh_room_pool_status;
typedef struct {gh_r207_allocation allocation;uint32_t count,capacity;} gh_room_vector;
struct gh_room_arena_native;
typedef struct {gh_r207_allocation allocation;uint32_t bytes,free_payload,first_offset;
 struct gh_room_arena_native *native;const void *native_owner;uint32_t live_allocations;} gh_room_arena;
typedef struct {
 gh_r207_allocation allocation;uint32_t payload_bytes,stride,count,free_head,used_head,used_count;
 uint32_t bytes20,remaining18,remaining1c,limit24;uint8_t *active;
} gh_room_block_pool;
typedef struct {
 void *object;const void *owner;uint32_t kind,handle;
} gh_room_object_binding;
typedef struct gh_room_pools {
 gh_heap *heap;gh_room_vector vectors[4];gh_room_arena arenas[3];gh_room_block_pool blocks[2];
 int initialized;uint32_t failed_entry;int detail;
 /* Host-only shared namespace. Original four-byte vectors contain handles,
  * never truncated native pointers. Handles are not recycled while live. */
 gh_room_object_binding *objects;size_t object_slots,object_capacity;uint32_t next_object_handle,next_arena_handle;
} gh_room_pools;
typedef struct {gh_room_pools *pools;void *pointer;uint32_t original_global,handle,bytes;} gh_room_arena_allocation;
/* Actual2A9538 gap allocator, bounded within SAME RHT arena backing. Typed
 * host links replace guest headerpointers; nativepayload alignment follows
 * actualhost address. Work594bit80000/size0/full returnoriginalNULL. */
gh_room_pool_status gh_room_arena_allocate_002a9538(gh_room_pools *,uint32_t original_global,uint32_t work594,uint32_t bytes,uint32_t alignment,gh_room_arena_allocation *);
gh_room_pool_status gh_room_arena_free_002a9680(gh_room_pools *,gh_room_arena_allocation);
/* Native bookkeeping only; refuses live allocations, doesnot freebacking. */
gh_room_pool_status gh_room_arena_dispose_metadata(gh_room_arena *);
/* Actual2A5D80 using realRHT. Caller zero-inits; three arenas and both concrete
 * fixed block pools own actual native storage. STL metadata is typed hoststate,
 * four-byte element capacity/payload is original. No guest pointer truncation.
 * Reached allocation errors retain all earlier owned storage for release. */
gh_room_pool_status gh_game_room_pools_initialize_002a5d80(gh_room_pools *,const gh_bundle *,gh_heap *);
/* Active pool objects require their concrete unrecovered destructors; release
 * then returns their exactfrontier, never silently destroys unknownobjects. */
gh_room_pool_status gh_game_room_pools_release(gh_room_pools *);
/* Actual2A97A0: Work+594 bit80000 inhibits allocation; zerohandle/pointer means
 * originalNULL (includingpoolfull). Nativehandles are one-based indices. */
gh_room_pool_status gh_room_block_take_002a97a0(gh_room_block_pool *,uint32_t work594,uint32_t *handle,void **payload);
gh_room_pool_status gh_room_block_put_002a9800(gh_room_block_pool *,uint32_t handle);
/* Canonical object-pointer registries allocated by the SAME 2A5D80 call.
 * These addresses name original globals for semantic selection only; no guest
 * memory is dereferenced. Positive elements still require typed native actors.
 * An initialized empty vector is distinct from an absent pool constructor. */
gh_room_pool_status gh_room_registry_view(const gh_room_pools *,uint32_t original_global,const gh_room_vector **);
/* Native equivalents of the paired 30FCC0/3129D0 registration. mask bits
 * select vectors0..3. Capacity/duplicates are checked before any mutation.
 * One object has one shared handle even when present in several lists. */
gh_room_pool_status gh_room_object_register(gh_room_pools *,void *object,const void *owner,
 uint32_t kind,uint32_t vector_mask,uint32_t *handle);
gh_room_pool_status gh_room_object_lookup(const gh_room_pools *,uint32_t handle,
 uint32_t kind,const void *owner,void **object);
/* Stable erase from every list. Foreign owner and unknown handles fail before
 * mutation. Caller destroys the actual typed object after unregistering. */
gh_room_pool_status gh_room_object_unregister(gh_room_pools *,uint32_t handle,const void *owner);
/* Ordered original room-exit primitives. These preserve other pool owners.
 * Live typed objects/used blocks retain their concrete destructor frontier. */
gh_room_pool_status gh_room_registries_release_0030f5f0(gh_room_pools *);
gh_room_pool_status gh_room_model_pool_release_00150078(gh_room_pools *);
gh_room_pool_status gh_room_aux_pool_release_001492c8(gh_room_pools *);
gh_room_pool_status gh_room_arena_clear_002a9528(gh_room_pools *,uint32_t original_global);
#ifdef __cplusplus
}
#endif
#endif
