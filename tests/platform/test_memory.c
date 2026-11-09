#include "../../src/platform/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){
 gh_heap *a,*b;gh_r207_allocation x,y,bad;gh_memory_binding binding;gh_r207_memory memory;gh_r207_state room={0};
 C(gh_heap_create(32,&a)==GH_MEM_OK);C(gh_heap_create(32,&b)==GH_MEM_OK);
 C(gh_heap_allocate(a,32,16,&x)==GH_MEM_OK);C((uintptr_t)x.pointer%16==0);
 memset(x.pointer,0xa5,32);C(((unsigned char*)x.pointer)[31]==0xa5);
 C(gh_heap_allocate(a,1,16,&y)==GH_MEM_CAPACITY);C(!y.pointer);
 C(gh_heap_used(a)==32 && ((unsigned char*)x.pointer)[31]==0xa5);
 C(gh_heap_allocate(b,7,64,&y)==GH_MEM_OK);C((uintptr_t)y.pointer%64==0);
 C(gh_heap_release(b,x)==GH_MEM_OWNER);C(gh_heap_used(a)==32);
 C(((unsigned char*)x.pointer)[0]==0xa5 && gh_heap_used(b)==7);
 bad=x;bad.owner=b;C(gh_heap_release(b,bad)==GH_MEM_OWNER);
 C(gh_heap_destroy(a)==GH_MEM_BUSY);C(gh_heap_release(a,x)==GH_MEM_OK);
 C(gh_heap_release(a,x)==GH_MEM_OWNER);C(gh_heap_release(b,y)==GH_MEM_OK);
 C(gh_heap_allocate(a,SIZE_MAX,16,&x)==GH_MEM_OVERFLOW);
 C(gh_heap_allocate(a,1,3,&x)==GH_MEM_ARGUMENT);
 C(gh_heap_allocate(a,0,16,&x)==GH_MEM_ARGUMENT);
 C(gh_memory_bind(&binding,a,&memory)==GH_MEM_OK);gh_r207_init(&room,&memory);
 C(binding.last_error==GH_MEM_OK && room.room_work.owner==a && room.room_work.pointer);
 *(unsigned char*)room.room_work.pointer=42;C(*(unsigned char*)room.room_work.pointer==42);
 C(gh_heap_used(a)==1);gh_r207_builtin_delete(&memory,room.room_work);
 C(binding.last_error==GH_MEM_OK && gh_heap_used(a)==0);
 C(gh_heap_destroy(a)==GH_MEM_OK);C(gh_heap_destroy(b)==GH_MEM_OK);
 puts("PASS native heap: real memory, capacity, alignment, ownership and recovered r207 init");return 0;
}
