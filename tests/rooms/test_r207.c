#include "../../src/rooms/r207.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {
    unsigned alloc_calls, release_calls;
    uint32_t requested, alignment;
    void *heap, *freed_owner, *freed_pointer;
    gh_r207_allocation result;
} recorder;
static gh_r207_allocation allocate(void *ctx,uint32_t size,uint32_t align,void *heap) {
    recorder *r=ctx;r->alloc_calls++;r->requested=size;r->alignment=align;r->heap=heap;
    return r->result;
}
static void release(void *ctx,void *owner,void *ptr) {
    recorder *r=ctx;r->release_calls++;r->freed_owner=owner;r->freed_pointer=ptr;
}
int main(void) {
    recorder r={0};
    int default_heap, actual_owner, payload;
    gh_r207_memory m={&r,&default_heap,allocate,release};
    gh_r207_state state={0};
    gh_r207_allocation a;
    r.result.pointer=&payload;r.result.owner=&actual_owner;
    gh_r207_init(&state,&m);
    CHECK(r.alloc_calls==1 && r.requested==1 && r.alignment==16);
    CHECK(r.heap==&default_heap && state.room_work.pointer==&payload);
    /* Allocation belongs to a different heap than the default: deletion must
     * pass its recorded owner, matching the original pointer-0x20 load. */
    gh_r207_builtin_delete(&m,state.room_work);
    CHECK(r.release_calls==1 && r.freed_owner==&actual_owner && r.freed_pointer==&payload);
    a.pointer=NULL;a.owner=&actual_owner;
    gh_r207_builtin_delete(&m,a);gh_r207_builtin_vec_delete(&m,a);
    CHECK(r.release_calls==1);
    a=gh_r207_builtin_vec_new(&m,UINT32_MAX);
    CHECK(r.requested==UINT32_MAX && r.alignment==16 && a.pointer==&payload);
    gh_r207_builtin_vec_delete(&m,a);CHECK(r.release_calls==2);
    /* No invented fallback allocation/exception on callback failure. */
    r.result.pointer=NULL;r.result.owner=NULL;
    gh_r207_init(&state,&m);CHECK(state.room_work.pointer==NULL && r.alloc_calls==3);
    gh_r207_move();CHECK(r.alloc_calls==3 && r.release_calls==2);
    puts("PASS r207: original allocation contract, owner routing, null-delete, init and empty move");
    return 0;
}
