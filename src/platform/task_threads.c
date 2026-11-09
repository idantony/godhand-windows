/* Copyright (c) 2026 Lucas Picoli and god-hand-decomp contributors.
 * Reconstructed source reference: upstream commit
 * eaae18d0dfab1f9a0df276ae5ca1581ad326faf3. MIT notice: ../core/LICENSE.upstream.
 * God Hand native Windows platform replacement for the recovered serial
 * task/manager handshake. Source semantics/provenance: task-thread-semantics.json.
 * No game instructions are interpreted by this backend. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "task_threads.h"

typedef struct task_slot {
    struct gh_task_threads *backend;
    void *fiber;
    gh_native_task_entry entry;
    void *context;
    int32_t argument;
    size_t index;
    int registered;
} task_slot;
struct gh_task_threads {
    gh_task_manager *manager;
    gh_task_record32 *records;
    size_t capacity;
    task_slot *slots;
    void *caller_fiber;
    DWORD thread_id;
    int converted;
    task_slot *executing;
};
static _Thread_local gh_task_threads *thread_backend;
static gh_thread_status check(gh_task_threads *b)
{
    if(!b || !b->manager || b->manager->records!=b->records ||
       b->manager->capacity!=b->capacity || b->manager->count>b->capacity)
        return GH_THREAD_INVALID;
    return GetCurrentThreadId()==b->thread_id ? GH_THREAD_OK : GH_THREAD_WRONG_THREAD;
}
static uint16_t sleep_count(const gh_task_record32 *w)
{
    uint16_t v; memcpy(&v,w->unknown0e,2); return v;
}
static void set_sleep(gh_task_record32 *w,uint16_t n) { memcpy(w->unknown0e,&n,2); }
static VOID WINAPI enter_task(void *parameter)
{
    task_slot *s=parameter;
    gh_task_threads *b=s->backend;
    s->entry(s->context,s->argument);
    /* TaskFnWork_runAndExit -> cTaskWork_exit: signal manager and exit.
     * Fiber deletion must occur back on caller's stack. */
    b->records[s->index].running=0;
    b->records[s->index].thread_id=-1;
    SwitchToFiber(b->caller_fiber);
    /* Completed fibers are deleted, never resumed. */
    abort();
}
gh_thread_status gh_task_threads_create(gh_task_manager *m,gh_task_threads **out)
{
    gh_task_threads *b;
    size_t i;
    if(!out) return GH_THREAD_INVALID;
    *out=0;
    if(thread_backend) return GH_THREAD_BUSY;
    if(!m || !m->records || !m->capacity || m->capacity>65535 ||
       m->count>m->capacity || m->step) return GH_THREAD_INVALID;
    for(i=0;i<m->count;++i) if(m->records[i].running) return GH_THREAD_BUSY;
    b=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*b));
    if(!b) return GH_THREAD_NO_MEMORY;
    b->slots=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,m->capacity*sizeof(*b->slots));
    if(!b->slots) { HeapFree(GetProcessHeap(),0,b); return GH_THREAD_NO_MEMORY; }
    b->converted=!IsThreadAFiber();
    b->caller_fiber=b->converted?ConvertThreadToFiberEx(0,FIBER_FLAG_FLOAT_SWITCH):GetCurrentFiber();
    if(!b->caller_fiber) {HeapFree(GetProcessHeap(),0,b->slots);HeapFree(GetProcessHeap(),0,b);return GH_THREAD_NO_MEMORY;}
    b->manager=m;b->records=m->records;b->capacity=m->capacity;b->thread_id=GetCurrentThreadId();
    for(i=0;i<b->capacity;++i) {b->slots[i].backend=b;b->slots[i].index=i;}
    m->step=gh_task_threads_step;m->step_context=b;*out=b;thread_backend=b;
    return GH_THREAD_OK;
}
gh_thread_status gh_task_threads_start(gh_task_threads *b,int32_t request,
    gh_native_task_entry entry,void *context,int32_t argument,int32_t *slot)
{
    gh_task_selection selected;
    task_slot *s;
    gh_thread_status status=check(b);
    if(slot) *slot=-1;
    if(status) return status;
    if(!slot || !entry) return GH_THREAD_INVALID;
    selected=gh_tasks_select_002d5b30(b->manager,request);
    if(selected.status) return GH_THREAD_INVALID;
    if(selected.index==-1) return GH_THREAD_OK;
    s=&b->slots[selected.index];
    if(s->registered || s->fiber) return GH_THREAD_BUSY;
    s->entry=entry;s->context=context;s->argument=argument;s->registered=1;
    b->records[selected.index].running=2;
    b->records[selected.index].thread_id=-1;
    (void)gh_tasks_inherit_002d5440(b->manager,selected.index);
    *slot=selected.index;
    return GH_THREAD_OK;
}
int gh_task_threads_step(void *context,gh_task_manager *manager,gh_task_record32 *work)
{
    gh_task_threads *b=context;
    task_slot *s;
    uint16_t count;
    gh_thread_status status=check(b);
    if(status) return status;
    /* Only the creating caller may drive a task. An unrelated fiber on the
     * same OS thread would otherwise resume the wrong caller stack when the
     * task yields/returns to caller_fiber. Reject before any task mutation. */
    if(!IsThreadAFiber() || GetCurrentFiber()!=b->caller_fiber)
        return GH_THREAD_INVALID;
    if(manager!=b->manager || b->executing || manager->current_index<0 ||
       (size_t)manager->current_index>=b->capacity ||
       work!=&b->records[manager->current_index]) return GH_THREAD_INVALID;
    s=&b->slots[manager->current_index];
    if(!work->running) return GH_THREAD_OK;
    if(!s->registered) return GH_THREAD_UNSUPPORTED;
    if(work->running==1) {
        if(!s->fiber) return GH_THREAD_INVALID;
        count=(uint16_t)(sleep_count(work)-1);set_sleep(work,count);
        if(count) return GH_THREAD_OK;
    } else if(work->running==2) {
        if(s->fiber) return GH_THREAD_INVALID;
        s->fiber=CreateFiberEx(0,0,FIBER_FLAG_FLOAT_SWITCH,enter_task,s);
        if(!s->fiber) return GH_THREAD_NO_MEMORY;
        work->thread_id=manager->current_index; /* host diagnostic token, not EE id */
    } else return GH_THREAD_UNSUPPORTED;
    work->running=0x7f;
    b->executing=s;
    SwitchToFiber(s->fiber);
    b->executing=0;
    if(!work->running) {
        DeleteFiber(s->fiber);s->fiber=0;s->registered=0;s->entry=0;
    }
    return GH_THREAD_OK;
}
gh_thread_status gh_task_threads_sleep(gh_task_threads *b,int32_t frames)
{
    gh_thread_status status=check(b);
    if(status) return status;
    if(!b->executing || GetCurrentFiber()!=b->executing->fiber) return GH_THREAD_INVALID;
    b->records[b->executing->index].running=1;
    set_sleep(&b->records[b->executing->index],(uint16_t)frames);
    SwitchToFiber(b->caller_fiber);
    return GH_THREAD_OK;
}
gh_thread_status gh_task_threads_cancel(gh_task_threads *b,int32_t slot)
{
    task_slot *s;
    gh_thread_status status=check(b);
    if(status) return status;
    if(slot<0 || slot>=b->manager->count) return GH_THREAD_INVALID;
    s=&b->slots[slot];
    if(b->executing==s) return GH_THREAD_BUSY;
    if(b->executing) return GH_THREAD_UNSUPPORTED;
    if(!s->registered) return b->records[slot].running?GH_THREAD_UNSUPPORTED:GH_THREAD_OK;
    if(s->fiber) DeleteFiber(s->fiber);
    s->fiber=0;s->registered=0;s->entry=0;
    b->records[slot].running=0;b->records[slot].thread_id=-1;
    return GH_THREAD_OK;
}
gh_thread_status gh_task_threads_destroy(gh_task_threads *b)
{
    size_t i;
    gh_thread_status status=check(b);
    if(status) return status;
    if(b->executing) return GH_THREAD_BUSY;
    if(GetCurrentFiber()!=b->caller_fiber) return GH_THREAD_INVALID;
    for(i=0;i<b->capacity;++i) if(b->slots[i].fiber || b->slots[i].registered) return GH_THREAD_BUSY;
    if(b->converted && !ConvertFiberToThread()) return GH_THREAD_INVALID;
    b->manager->step=0;b->manager->step_context=0;thread_backend=0;
    HeapFree(GetProcessHeap(),0,b->slots);HeapFree(GetProcessHeap(),0,b);
    return GH_THREAD_OK;
}
