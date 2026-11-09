#ifndef GH_MEMORY_H
#define GH_MEMORY_H
#include <stddef.h>
#include "../rooms/r207.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_MEM_OK,GH_MEM_ARGUMENT,GH_MEM_CAPACITY,GH_MEM_OVERFLOW,GH_MEM_SYSTEM,GH_MEM_OWNER,GH_MEM_BUSY} gh_mem_error;
typedef struct gh_heap gh_heap;
gh_mem_error gh_heap_create(size_t capacity,gh_heap **out);
gh_mem_error gh_heap_destroy(gh_heap *heap);
gh_mem_error gh_heap_allocate(gh_heap *,size_t size,size_t alignment,gh_r207_allocation *out);
gh_mem_error gh_heap_release(gh_heap *,gh_r207_allocation allocation);
size_t gh_heap_used(const gh_heap *);
/* Explicit per-binding error channel; no process-global allocator state.
 * Capacity measures requested payload bytes, excluding host bookkeeping.
 * Zero-size requests and alignment below 16 are rejected host policies. */
typedef struct {gh_heap *heap;gh_mem_error last_error;} gh_memory_binding;
gh_mem_error gh_memory_bind(gh_memory_binding *,gh_heap *,gh_r207_memory *out);
#ifdef __cplusplus
}
#endif
#endif
