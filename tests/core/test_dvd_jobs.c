#include "dvd_jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static const char *resource="dll/debugoptimize/r207.rel";
typedef struct wait_context {
    gh_dvd_jobs *dvd;gh_heap *heap;gh_task_threads *threads;
    gh_r207_allocation buffer;uint32_t id;int completed;
} wait_context;
static void load_task(void *context,int32_t unused)
{
    wait_context *w=context;
    volatile int local=unused;
    w->id=gh_dvd_read_alloc(w->dvd,resource,&w->buffer,w->heap,0,0,0,0);
    CHECK(w->id && gh_dvd_check(w->dvd,w->id)==2);
    CHECK(gh_dvd_check_wait_frame(w->dvd,w->id,w->threads)==GH_DVD_OK);
    CHECK(local==37);w->completed=1;
}
static void release(gh_heap *heap,gh_r207_allocation a) {CHECK(gh_heap_release(heap,a)==GH_MEM_OK);}
int main(int argc,char **argv)
{
    gh_disc *disc;gh_heap *heap,*tiny;gh_dvd_jobs q;
    gh_r207_allocation a,b,c,d,allocations[33];
    uint32_t ids[33],size=0,id;size_t index;gh_disc_entry entry;
    unsigned char reference[1720],sector[2048];FILE *f;unsigned i;
    CHECK(argc==3);
    CHECK(gh_disc_open(argv[1],&disc)==GH_DISC_OK);
    CHECK(gh_disc_find(disc,resource,&index)==GH_DISC_OK);
    CHECK(gh_disc_entry_at(disc,index,&entry)==GH_DISC_OK && entry.size==sizeof(reference));
    f=fopen(argv[2],"rb");CHECK(f && fread(reference,1,sizeof(reference),f)==sizeof(reference));fclose(f);
    f=fopen(argv[1],"rb");CHECK(f && _fseeki64(f,(__int64)entry.iso_offset,SEEK_SET)==0);
    CHECK(fread(sector,1,sizeof(sector),f)==sizeof(sector));fclose(f);
    CHECK(gh_heap_create(1024*1024,&heap)==GH_MEM_OK);
    CHECK(gh_heap_create(1024,&tiny)==GH_MEM_OK);
    gh_dvd_jobs_init(&q,disc);
    CHECK(!gh_dvd_read_alloc(&q,"missing/not.real",&a,heap,0,0,0,0));
    CHECK(q.last_error==GH_DVD_NOT_FOUND && !a.pointer);
    CHECK(!gh_dvd_read_alloc(&q,resource,&a,tiny,0,0,0,0));
    CHECK(q.last_error==GH_DVD_MEMORY && !a.pointer && gh_heap_used(tiny)==0);
    CHECK(!gh_dvd_read_alloc(&q,resource,&a,heap,0,0,1,0) && q.last_error==GH_DVD_UNSUPPORTED);
    CHECK(!gh_dvd_read_alloc(&q,resource,&a,heap,0,4096,0,0) && q.last_error==GH_DVD_BOUNDS);
    q.id_counter=UINT32_MAX;
    id=gh_dvd_read_alloc_sized(&q,resource,&a,heap,0,&size,0,0);
    CHECK(id==1 && size==2048 && (uintptr_t)a.pointer%128==0);
    memset(a.pointer,0xa5,size);
    CHECK(gh_dvd_check(&q,id)==2 && ((unsigned char*)a.pointer)[0]==0xa5);
    CHECK(gh_dvd_pump(&q)==GH_DVD_OK && gh_dvd_check(&q,id)==0);
    CHECK(!memcmp(a.pointer,reference,sizeof(reference)));
    CHECK(!memcmp(a.pointer,sector,sizeof(sector))); /* real sector tail, not zero-fill */
    release(heap,a);
    /* First job stays active; remaining jobs selection-sort priority descending.
     * Equal-priority pair22/33 reverses when later max44 is swapped forward. */
    CHECK(gh_dvd_read_alloc(&q,resource,&a,heap,11,0,0,0)==11);
    CHECK(gh_dvd_read_alloc(&q,resource,&b,heap,22,0,0,1)==22);
    CHECK(gh_dvd_read_alloc(&q,resource,&c,heap,33,0,0,1)==33);
    CHECK(gh_dvd_read_alloc(&q,resource,&d,heap,44,0,0,2)==44);
    CHECK(gh_dvd_check(&q,44)==1 && gh_dvd_check(&q,11)==2);
    CHECK(gh_dvd_pump(&q)==GH_DVD_OK && gh_dvd_check(&q,44)==2);
    CHECK(gh_dvd_pump(&q)==GH_DVD_OK && gh_dvd_check(&q,33)==2);
    CHECK(gh_dvd_pump(&q)==GH_DVD_OK && gh_dvd_check(&q,22)==2);
    CHECK(gh_dvd_pump(&q)==GH_DVD_OK);
    release(heap,a);release(heap,b);release(heap,c);release(heap,d);
    /* Full queue reproduces synchronous pressure drain rather than invented
     * immediate failure. The 33rd request only queues after old32 real reads. */
    for(i=0;i<33;++i) {ids[i]=gh_dvd_read_alloc(&q,resource,&allocations[i],heap,100+i,0,0,0);CHECK(ids[i]==100+i);}
    for(i=0;i<32;++i) {CHECK(!gh_dvd_check(&q,ids[i]));CHECK(!memcmp(allocations[i].pointer,reference,sizeof(reference)));}
    CHECK(gh_dvd_check(&q,ids[32])==2 && gh_dvd_pump(&q)==GH_DVD_OK);
    for(i=0;i<33;++i)release(heap,allocations[i]);
    /* Queued and active cancellation retain ownership and do not read output. */
    CHECK(gh_dvd_read_alloc(&q,resource,&a,heap,700,0,0,0)==700);
    CHECK(gh_dvd_read_alloc(&q,resource,&b,heap,701,0,0,0)==701);
    memset(a.pointer,0xa5,2048);memset(b.pointer,0xa5,2048);
    CHECK(gh_dvd_cancel(&q,701)==1 && gh_dvd_check(&q,701)==0);
    CHECK(gh_dvd_cancel(&q,700)==1 && gh_dvd_check(&q,700)==0);
    CHECK(((unsigned char*)a.pointer)[0]==0xa5 && ((unsigned char*)b.pointer)[0]==0xa5);
    CHECK(gh_dvd_cancel(&q,700)==0);release(heap,a);release(heap,b);
    /* Fault injection changes only the lookup index; the real bounded disc
     * layer rejects the actual attempted read. No test adapter fills data. */
    CHECK(gh_dvd_read_alloc(&q,resource,&a,heap,800,1,0,0)==800);
    memset(a.pointer,0xa5,2048);q.jobs[q.current].disc_index=SIZE_MAX;
    CHECK(gh_dvd_pump(&q)==GH_DVD_IO && q.failed_id==800);
    CHECK(gh_dvd_check(&q,800)==0 && ((unsigned char*)a.pointer)[0]==0xa5);
    release(heap,a);
    {
        gh_task_record32 records[3]={{0}};
        gh_task_manager manager={records,3,0,-1,3,0,0,0};
        wait_context w={&q,heap,0,{0},0,0};int32_t slot;
        CHECK(gh_task_threads_create(&manager,&w.threads)==GH_THREAD_OK);
        CHECK(gh_task_threads_start(w.threads,-1,load_task,&w,37,&slot)==GH_THREAD_OK);
        CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
        CHECK(w.id && !w.completed && gh_dvd_check(&q,w.id)==2);
        CHECK(gh_dvd_pump(&q)==GH_DVD_OK);
        CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK && !w.completed);
        CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK && w.completed);
        CHECK(!memcmp(w.buffer.pointer,reference,sizeof(reference)));
        release(heap,w.buffer);CHECK(gh_task_threads_destroy(w.threads)==GH_THREAD_OK);
    }
    CHECK(gh_heap_used(heap)==0 && gh_heap_destroy(heap)==GH_MEM_OK);
    CHECK(gh_heap_destroy(tiny)==GH_MEM_OK);gh_disc_close(disc);
    puts("PASS DVD real ISO sectors/heap/priority/pressure drain/cancel/fiber WaitFrame");
    return 0;
}
