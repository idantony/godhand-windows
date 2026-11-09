#ifndef GH_GAME_ROOM_POOLS_H
#define GH_GAME_ROOM_POOLS_H
#include "../platform/memory.h"
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_POOL_OK,GH_POOL_ARGUMENT,GH_POOL_BOUNDS,GH_POOL_MEMORY,GH_POOL_BUSY,GH_POOL_UNSUPPORTED} gh_room_pool_status;
typedef struct {gh_r207_allocation allocation;uint32_t count,capacity;} gh_room_vector;
typedef struct {gh_r207_allocation allocation;uint32_t bytes,free_payload,first_offset;} gh_room_arena;
typedef struct {
 gh_r207_allocation allocation;uint32_t payload_bytes,stride,count,free_head,used_head,used_count;
 uint32_t bytes20,remaining18,remaining1c,limit24;uint8_t *active;
} gh_room_block_pool;
typedef struct {
 gh_heap *heap;gh_room_vector vectors[4];gh_room_arena arenas[3];gh_room_block_pool blocks[2];
 int initialized;uint32_t failed_entry;int detail;
} gh_room_pools;
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
#ifdef __cplusplus
}
#endif
#endif
