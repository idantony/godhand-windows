#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "task_threads.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static int failures;
#define CHECK(x) do { if(!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);++failures;} } while(0)
typedef struct scenario {
    gh_task_threads *backend;
    int trace[16]; unsigned used;
    int shared, resumed, entered;
} scenario;
static void mark(scenario *s,int n) { CHECK(s->used<16);if(s->used<16)s->trace[s->used++]=n; }
static void task_a(void *context,int32_t arg)
{
    scenario *s=context;
    volatile int local=arg+73;
    ++s->entered;mark(s,10);s->shared=1;
    CHECK(gh_task_threads_cancel(s->backend,2)==GH_THREAD_BUSY);
    CHECK(gh_task_threads_destroy(s->backend)==GH_THREAD_BUSY);
    CHECK(gh_task_threads_sleep(s->backend,2)==GH_THREAD_OK);
    CHECK(local==68 && s->shared==4); /* actual fiber stack survives yield */
    ++s->resumed;mark(s,11);s->shared=5;
}
static void task_b(void *context,int32_t arg)
{
    scenario *s=context;
    volatile int local=arg;
    ++s->entered;mark(s,20);CHECK(s->shared==1);s->shared=2;
    CHECK(gh_task_threads_sleep(s->backend,1)==GH_THREAD_OK);
    CHECK(local==123);++s->resumed;mark(s,21);s->shared=4;
}
static void sleeper(void *context,int32_t arg)
{
    scenario *s=context;
    ++s->entered;
    CHECK(gh_task_threads_sleep(s->backend,arg)==GH_THREAD_OK);
    ++s->resumed;
}
static DWORD WINAPI foreign_cancel(void *context)
{
    scenario *s=context;
    CHECK(gh_task_threads_replace_current(s->backend,sleeper,s,1)==GH_THREAD_WRONG_THREAD);
    return (DWORD)gh_task_threads_cancel(s->backend,2);
}
static void replacement(void *context,int32_t arg)
{
    scenario *s=context;
    CHECK(arg==456);s->shared=72;
    CHECK(gh_task_threads_sleep(s->backend,1)==GH_THREAD_OK);
    s->shared=73;
}
static void replaced(void *context,int32_t arg)
{
    scenario *s=context;
    CHECK(arg==123);s->shared=71;
    CHECK(gh_task_threads_replace_current(s->backend,0,s,9)==GH_THREAD_INVALID);
    (void)gh_task_threads_replace_current(s->backend,replacement,s,456);
    s->shared=-999; /* must never execute on the retired original stack */
}
typedef struct foreign_visit {
    gh_task_manager *manager;
    void *owner_fiber;
    gh_task_run_result result;
    int returned;
} foreign_visit;
static VOID WINAPI foreign_tick(void *context)
{
    foreign_visit *v=context;
    CHECK(gh_task_threads_replace_current((gh_task_threads*)v->manager->step_context,replacement,0,0)==GH_THREAD_INVALID);
    v->result=gh_tasks_run_matching_002d52a0(v->manager);
    v->returned=1;
    SwitchToFiber(v->owner_fiber);
    abort();
}
static void tick(gh_task_manager *m) { CHECK(gh_tasks_run_matching_002d52a0(m).status==GH_TASK_OK); }
int main(void)
{
    gh_task_record32 records[4];
    gh_task_manager m={records,4,0,-1,4,0,0,0};
    scenario s;
    int32_t a,b,full;
    unsigned i;
    HANDLE thread;DWORD exitcode;
    memset(records,0,sizeof(records));memset(&s,0,sizeof(s));
    CHECK(gh_task_threads_create(&m,&s.backend)==GH_THREAD_OK);
    CHECK(gh_task_threads_sleep(s.backend,1)==GH_THREAD_INVALID);
    CHECK(gh_task_threads_replace_current(s.backend,replacement,&s,0)==GH_THREAD_INVALID);
    CHECK(gh_task_threads_start(s.backend,-1,task_a,&s,-5,&a)==GH_THREAD_OK && a==2);
    CHECK(gh_task_threads_start(s.backend,-1,task_b,&s,123,&b)==GH_THREAD_OK && b==3);
    CHECK(s.entered==0); /* start marks state2; first selected tick runs it */
    CHECK(gh_task_threads_start(s.backend,-1,sleeper,&s,1,&full)==GH_THREAD_OK && full==-1);
    CHECK(gh_task_threads_destroy(s.backend)==GH_THREAD_BUSY);
    /* Same OS thread, different fiber must reject before touching task state.
     * That caller must regain control; owner can then run the pending tasks. */
    {
        foreign_visit v={0};
        void *foreign;
        v.manager=&m;v.owner_fiber=GetCurrentFiber();
        foreign=CreateFiber(0,foreign_tick,&v);
        CHECK(foreign!=0);
        if(foreign) {
            SwitchToFiber(foreign);
            CHECK(v.returned==1);
            CHECK(v.result.status==GH_TASK_STEP_FAILED);
            CHECK(v.result.platform_error==GH_THREAD_INVALID);
            CHECK(s.entered==0 && records[2].running==2 && records[3].running==2);
            DeleteFiber(foreign);
        }
    }
    tick(&m);CHECK(s.used==2 && s.trace[0]==10 && s.trace[1]==20 && s.shared==2);
    CHECK(records[2].running==1 && records[3].running==1);
    tick(&m);CHECK(s.used==3 && s.trace[2]==21 && s.resumed==1);
    CHECK(records[2].running==1 && records[3].running==0 && records[3].thread_id==-1);
    tick(&m);CHECK(s.used==4 && s.trace[3]==11 && s.shared==5 && s.resumed==2);
    CHECK(!m.current && m.current_index==-1 && records[2].running==0);
    /* Cancellation of a suspended stack prevents execution after its yield. */
    CHECK(gh_task_threads_start(s.backend,2,sleeper,&s,5,&a)==GH_THREAD_OK && a==2);
    tick(&m);i=(unsigned)s.resumed;
    thread=CreateThread(0,0,foreign_cancel,&s,0,0);
    CHECK(thread!=0);
    if(thread) {WaitForSingleObject(thread,INFINITE);GetExitCodeThread(thread,&exitcode);CloseHandle(thread);CHECK(exitcode==GH_THREAD_WRONG_THREAD);}
    CHECK(gh_task_threads_cancel(s.backend,2)==GH_THREAD_OK);
    tick(&m);CHECK(s.resumed==(int)i && records[2].running==0);
    /* Original zero-count underflow, not an invented immediate yield. */
    CHECK(gh_task_threads_start(s.backend,2,sleeper,&s,0,&a)==GH_THREAD_OK);
    tick(&m);i=(unsigned)s.resumed;
    for(unsigned n=0;n<65535;++n) tick(&m);
    CHECK(s.resumed==(int)i && records[2].running==1);
    tick(&m);CHECK(s.resumed==(int)i+1 && records[2].running==0);
    /* Filtering delays visits and therefore delays sleep countdown. */
    CHECK(gh_task_threads_start(s.backend,2,sleeper,&s,1,&a)==GH_THREAD_OK);
    records[2].attributes=0;m.flags=1;i=(unsigned)s.entered;
    tick(&m);CHECK(s.entered==(int)i && records[2].running==2);
    records[2].attributes=1;tick(&m);CHECK(records[2].running==1);
    records[2].attributes=0;tick(&m);CHECK(records[2].running==1);
    records[2].attributes=1;tick(&m);CHECK(records[2].running==0);
    /* Tail restart deletes old stack, preserves attr/slot and starts later. */
    m.flags=0;records[2].attributes=0x55;
    CHECK(gh_task_threads_start(s.backend,2,replaced,&s,123,&a)==GH_THREAD_OK);
    records[2].attributes=0x55;
    tick(&m);CHECK(s.shared==71 && records[2].running==2 && records[2].thread_id==-1);
    CHECK(records[2].attributes==0x55);
    tick(&m);CHECK(s.shared==72 && records[2].running==1);
    tick(&m);CHECK(s.shared==73 && records[2].running==0);
    CHECK(gh_task_threads_cancel(s.backend,2)==GH_THREAD_OK);
    /* Foreign/external kernel-resumed state4 must not be silently accepted. */
    m.flags=0;records[2].running=4;
    CHECK(gh_tasks_run_matching_002d52a0(&m).status==GH_TASK_STEP_FAILED);
    CHECK(gh_task_threads_cancel(s.backend,2)==GH_THREAD_UNSUPPORTED);
    records[2].running=0;
    CHECK(gh_task_threads_destroy(s.backend)==GH_THREAD_OK && !m.step);
    CHECK(!IsThreadAFiber());
    if(failures)return 1;
    puts("Native fiber task stacks/sleep/exit/cancellation passed; limited serial backend only");
    return 0;
}
