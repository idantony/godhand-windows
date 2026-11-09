/* Native host mechanism, not a recovery of cHeap internals. The verified r207
 * import calls size/alignment/default heap and releases through allocation
 * owner. Host metadata is kept separately, never at a guest pointer offset. */
#include "memory.h"
#include <stdlib.h>
#include <stdint.h>
typedef struct block {void *raw,*pointer;size_t size;struct block *next;} block;
struct gh_heap {size_t capacity,used;block *blocks;};
gh_mem_error gh_heap_create(size_t capacity,gh_heap **out) {
    gh_heap *h;if(!out)return GH_MEM_ARGUMENT;*out=NULL;
    if(!capacity)return GH_MEM_ARGUMENT;
    h=calloc(1,sizeof(*h));if(!h)return GH_MEM_SYSTEM;
    h->capacity=capacity;*out=h;return GH_MEM_OK;
}
gh_mem_error gh_heap_destroy(gh_heap *h) {
    if(!h)return GH_MEM_ARGUMENT;
    if(h->blocks)return GH_MEM_BUSY;
    free(h);return GH_MEM_OK;
}
size_t gh_heap_used(const gh_heap *h){return h?h->used:0;}
gh_mem_error gh_heap_allocate(gh_heap *h,size_t size,size_t align,gh_r207_allocation *out) {
    block *b;uintptr_t address;
    if(!out)return GH_MEM_ARGUMENT;out->pointer=NULL;out->owner=NULL;
    if(!h||!size||align<16||(align&(align-1)))return GH_MEM_ARGUMENT;
    if(size>SIZE_MAX-(align-1))return GH_MEM_OVERFLOW;
    if(size>h->capacity-h->used)return GH_MEM_CAPACITY;
    b=malloc(sizeof(*b));if(!b)return GH_MEM_SYSTEM;
    b->raw=malloc(size+align-1);if(!b->raw){free(b);return GH_MEM_SYSTEM;}
    address=(uintptr_t)b->raw;
    if(address>UINTPTR_MAX-(align-1)){free(b->raw);free(b);return GH_MEM_OVERFLOW;}
    b->pointer=(void*)((address+align-1)&~(uintptr_t)(align-1));b->size=size;
    b->next=h->blocks;h->blocks=b;h->used+=size;
    out->pointer=b->pointer;out->owner=h;return GH_MEM_OK;
}
gh_mem_error gh_heap_release(gh_heap *h,gh_r207_allocation a) {
    block **link,*b;
    if(!h)return GH_MEM_ARGUMENT;
    if(a.owner!=h)return GH_MEM_OWNER;
    if(!a.pointer)return GH_MEM_ARGUMENT;
    for(link=&h->blocks;*link;link=&(*link)->next)if((*link)->pointer==a.pointer)break;
    if(!*link)return GH_MEM_OWNER;
    b=*link;*link=b->next;h->used-=b->size;free(b->raw);free(b);return GH_MEM_OK;
}
static gh_r207_allocation cb_alloc(void *ctx,uint32_t n,uint32_t alignment,void *heap) {
    gh_memory_binding *binding=ctx;gh_r207_allocation a={0};
    if(heap!=binding->heap){binding->last_error=GH_MEM_OWNER;return a;}
    binding->last_error=gh_heap_allocate(binding->heap,n,alignment,&a);return a;
}
static void cb_free(void *ctx,void *owner,void *ptr) {
    gh_memory_binding *binding=ctx;gh_r207_allocation a={ptr,owner};
    binding->last_error=gh_heap_release(binding->heap,a);
}
gh_mem_error gh_memory_bind(gh_memory_binding *b,gh_heap *h,gh_r207_memory *out) {
    if(!b||!h||!out)return GH_MEM_ARGUMENT;
    b->heap=h;b->last_error=GH_MEM_OK;
    out->context=b;out->default_heap=h;out->allocate=cb_alloc;out->release=cb_free;
    return GH_MEM_OK;
}
