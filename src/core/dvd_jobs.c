/* Copyright (c) 2026 Lucas Picoli and god-hand-decomp contributors.
 * MIT notice: LICENSE.upstream. Upstream eaae18d0dfab1f9a0df276ae5ca1581ad326faf3.
 * Actual local ELF sources/ranges and host timing boundary: dvd-job-semantics.json.
 */
#include "dvd_jobs.h"
#include <string.h>
#include <limits.h>

static void clear_job(gh_dvd_job *j) {memset(j,0,sizeof(*j));}
static void start_next(gh_dvd_jobs *q)
{
    unsigned i,j,n=0;
    /* 201290 compacts holes, then selection-sorts unsigned arg6C descending.
     * Swapping (rather than stable sorting) is observable for equal priorities. */
    for(i=0;i<GH_DVD_JOB_COUNT;++i) if(q->jobs[i].state) {
        if(n!=i) {q->jobs[n]=q->jobs[i];clear_job(&q->jobs[i]);}
        ++n;
    }
    for(i=0;i+1<n;++i) {
        unsigned best=i;
        for(j=i+1;j<n;++j) if(q->jobs[best].priority<q->jobs[j].priority) best=j;
        if(best!=i) {gh_dvd_job temp=q->jobs[i];q->jobs[i]=q->jobs[best];q->jobs[best]=temp;}
    }
    q->current=n?0:-1;q->queue_state=n?2:0;
}
void gh_dvd_jobs_init(gh_dvd_jobs *q,gh_disc *d)
{
    if(q) {memset(q,0,sizeof(*q));q->disc=d;q->current=-1;}
}
gh_dvd_error gh_dvd_pump(gh_dvd_jobs *q)
{
    unsigned i;
    if(!q || !q->disc) return GH_DVD_ARGUMENT;
    if(q->queue_state==1) {start_next(q);return GH_DVD_OK;}
    if(q->queue_state==2) {
        gh_dvd_job *j;
        gh_disc_error error;
        uint32_t id;
        if(q->current<0 || q->current>=GH_DVD_JOB_COUNT) return GH_DVD_ARGUMENT;
        j=&q->jobs[q->current];id=j->id;
        error=gh_disc_read_blocks(q->disc,j->disc_index,0,j->allocation.pointer,j->blocks);
        clear_job(j);q->current=-1;q->queue_state=1;
        start_next(q);
        if(error) {q->last_error=GH_DVD_IO;q->failed_id=id;return GH_DVD_IO;}
        return GH_DVD_OK;
    }
    for(i=0;i<GH_DVD_JOB_COUNT;++i) if(q->jobs[i].state) {q->queue_state=1;break;}
    return GH_DVD_OK;
}
static gh_dvd_job *free_job(gh_dvd_jobs *q)
{
    unsigned i;
    for(i=0;i<GH_DVD_JOB_COUNT;++i) if(!q->jobs[i].state) return &q->jobs[i];
    /* 2011B0: pressure drains the active queue; it is not a nonblocking
     * queue-full rejection. Every step performs real I/O. */
    while(q->current!=-1) if(gh_dvd_pump(q)!=GH_DVD_OK) return 0;
    for(i=0;i<GH_DVD_JOB_COUNT;++i) if(!q->jobs[i].state) return &q->jobs[i];
    return 0;
}
static uint32_t enqueue(gh_dvd_jobs *q,const char *name,gh_r207_allocation *out,
    gh_heap *heap,uint32_t id,uint32_t override,uint32_t *size,uint32_t arg70,uint32_t priority)
{
    gh_dvd_job *j;
    gh_disc_entry entry;
    gh_disc_error disc_error;
    size_t index;
    uint64_t bytes,maximum;
    if(out) {out->pointer=0;out->owner=0;}
    if(!q || !q->disc || !name || !out || !heap) {if(q)q->last_error=GH_DVD_ARGUMENT;return 0;}
    q->last_error=GH_DVD_OK;
    if(arg70) {q->last_error=GH_DVD_UNSUPPORTED;return 0;}
    j=free_job(q);
    if(!j) {if(q->last_error==GH_DVD_OK)q->last_error=GH_DVD_MEMORY;return 0;}
    disc_error=gh_disc_find(q->disc,name,&index);
    if(disc_error || gh_disc_entry_at(q->disc,index,&entry)) {q->last_error=GH_DVD_NOT_FOUND;return 0;}
    bytes=override?override:entry.size;maximum=(entry.size+0x7ff)&~UINT64_C(0x7ff);
    bytes=(bytes+0x7ff)&~UINT64_C(0x7ff);
    if(!bytes || bytes>UINT32_MAX || bytes>maximum || entry.size>UINT32_MAX) {q->last_error=GH_DVD_BOUNDS;return 0;}
    j->state=1;j->file_number=entry.afs_index;j->byte_size=(uint32_t)entry.size;
    j->blocks=(uint32_t)(bytes>>11);j->disc_index=index;j->priority=priority;
    if(!id) {++q->id_counter;if(!q->id_counter)q->id_counter=1;id=q->id_counter;}
    j->id=id;
    if(gh_heap_allocate(heap,(size_t)bytes,0x80,&j->allocation)!=GH_MEM_OK) {
        clear_job(j);q->last_error=GH_DVD_MEMORY;return 0;
    }
    *out=j->allocation;
    if(size)*size=(uint32_t)bytes;
    if(q->current==-1) start_next(q);
    return id;
}
uint32_t gh_dvd_read_alloc(gh_dvd_jobs *q,const char *name,gh_r207_allocation *out,
    gh_heap *heap,uint32_t id,uint32_t override,uint32_t arg70,uint32_t priority)
{return enqueue(q,name,out,heap,id,override,0,arg70,priority);}
uint32_t gh_dvd_read_alloc_sized(gh_dvd_jobs *q,const char *name,gh_r207_allocation *out,
    gh_heap *heap,uint32_t id,uint32_t *size,uint32_t arg70,uint32_t priority)
{return enqueue(q,name,out,heap,id,0,size,arg70,priority);}
int gh_dvd_check(const gh_dvd_jobs *q,uint32_t id)
{
    unsigned i;
    if(!q || !id)return 0;
    if(q->current!=-1 && q->jobs[q->current].id==id)return 2;
    for(i=0;i<GH_DVD_JOB_COUNT;++i)if(q->jobs[i].state && q->jobs[i].id==id)return 1;
    return 0;
}
gh_dvd_error gh_dvd_check_wait_frame(gh_dvd_jobs *q,uint32_t id,gh_task_threads *threads)
{
    if(!q || !threads)return GH_DVD_ARGUMENT;
    while(gh_dvd_check(q,id))if(gh_task_threads_sleep(threads,1)!=GH_THREAD_OK)return GH_DVD_TASK_ERROR;
    if(gh_task_threads_sleep(threads,1)!=GH_THREAD_OK)return GH_DVD_TASK_ERROR;
    return q->failed_id==id && q->last_error==GH_DVD_IO?GH_DVD_IO:GH_DVD_OK;
}
int gh_dvd_cancel(gh_dvd_jobs *q,uint32_t id)
{
    unsigned i;int found=0;
    if(!q || q->current==-1)return 0;
    if(q->jobs[q->current].id==id) {
        clear_job(&q->jobs[q->current]);q->current=-1;start_next(q);return 1;
    }
    for(i=0;i<GH_DVD_JOB_COUNT;++i)if(q->jobs[i].id==id) {clear_job(&q->jobs[i]);found=1;}
    return found;
}

gh_dvd_error gh_dvd_jobs_reset_00201018(gh_dvd_jobs*q){
 if(!q||!q->disc||q->current < -1||q->current>=GH_DVD_JOB_COUNT)return GH_DVD_ARGUMENT;
 memset(q->jobs,0,sizeof(q->jobs));q->current=-1;q->queue_state=0;return GH_DVD_OK;
}
