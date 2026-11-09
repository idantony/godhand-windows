#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../../src/platform/task_threads.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct scenario {
    gh_task_threads *parent,*child;
    gh_task_manager *child_manager,*parent_manager;
    unsigned child_entered,child_resumed,parent_returned,retired_continued;
    int value;
} scenario;
static void child_second(void *context,int32_t arg)
{
    scenario *s=context;CHECK(arg==99);s->value=20;
    CHECK(gh_task_threads_sleep(s->child,1)==GH_THREAD_OK);
    ++s->child_resumed;s->value=21;
}
static void child_first(void *context,int32_t arg)
{
    scenario *s=context;volatile int local=arg;
    ++s->child_entered;s->value=10;
    CHECK(gh_task_threads_run_nested(s->child,s->parent).status==GH_TASK_STEP_FAILED);
    CHECK(gh_task_threads_sleep(s->child,1)==GH_THREAD_OK);
    CHECK(local==77);s->value=11;
    (void)gh_task_threads_replace_current(s->child,child_second,s,99);
    ++s->retired_continued;
}
static void parent_entry(void *context,int32_t arg)
{
    scenario *s=context;int32_t slot;volatile int parent_local=arg;
    CHECK(gh_task_threads_start(s->child,2,child_first,s,77,&slot)==GH_THREAD_OK && slot==2);
    CHECK(gh_task_threads_run_nested(s->parent,s->child).status==GH_TASK_OK && s->value==10);
    CHECK(s->parent_manager->records[2].thread_id>0 && s->child_manager->records[2].thread_id>0);
    CHECK(s->parent_manager->records[2].thread_id!=s->child_manager->records[2].thread_id);
    CHECK(gh_task_threads_sleep(s->parent,1)==GH_THREAD_OK);
    CHECK(parent_local==123 && s->child_entered==1);
    CHECK(gh_task_threads_run_nested(s->parent,s->child).status==GH_TASK_OK && s->value==11);
    CHECK(gh_task_threads_sleep(s->parent,1)==GH_THREAD_OK);
    CHECK(gh_task_threads_run_nested(s->parent,s->child).status==GH_TASK_OK && s->value==20);
    CHECK(gh_task_threads_sleep(s->parent,1)==GH_THREAD_OK);
    CHECK(gh_task_threads_run_nested(s->parent,s->child).status==GH_TASK_OK && s->value==21);
    CHECK(parent_local==123);++s->parent_returned;
}
typedef struct foreign {scenario *s;void *root;gh_task_run_result nested,direct;} foreign;
static VOID WINAPI foreign_entry(void *context)
{
    foreign *f=context;
    f->nested=gh_task_threads_run_nested(f->s->parent,f->s->child);
    f->direct=gh_tasks_run_matching_002d52a0(f->s->child_manager);
    SwitchToFiber(f->root);abort();
}
static void nested_failure(void *context,int32_t arg)
{
    scenario *s=context;gh_task_run_result r;(void)arg;
    s->child_manager->records[2].running=4;
    r=gh_task_threads_run_nested(s->parent,s->child);
    CHECK(r.status==GH_TASK_STEP_FAILED && r.platform_error==GH_THREAD_UNSUPPORTED);
    s->child_manager->records[2].running=0;
}
typedef struct construct_context {gh_task_manager *manager;gh_task_threads **backend;int done;} construct_context;
static void construct_child(void *context,int32_t arg)
{
    construct_context *c=context;(void)arg;
    CHECK(gh_task_threads_create(c->manager,c->backend)==GH_THREAD_OK);c->done=1;
}
int main(void)
{
    gh_task_record32 p_records[4]={{0}},c_records[16]={{0}},late_records[3]={{0}};
    gh_task_manager p={p_records,4,0,-1,4,0,0,0},c={c_records,16,0,-1,16,0,0,0};
    gh_task_manager late={late_records,3,0,-1,3,0,0,0};gh_task_threads *late_backend=0;
    scenario s={0};int32_t slot;foreign f={0};void *fiber;construct_context cc={&late,&late_backend,0};
    CHECK(!IsThreadAFiber());CHECK(gh_task_threads_create(&p,&s.parent)==GH_THREAD_OK);
    CHECK(gh_task_threads_create(&c,&s.child)==GH_THREAD_OK);s.child_manager=&c;s.parent_manager=&p;
    CHECK(gh_task_threads_run_nested(s.parent,s.child).status==GH_TASK_STEP_FAILED);
    f.s=&s;f.root=GetCurrentFiber();fiber=CreateFiber(0,foreign_entry,&f);CHECK(fiber);
    SwitchToFiber(fiber);DeleteFiber(fiber);
    CHECK(f.nested.status==GH_TASK_STEP_FAILED && f.direct.status==GH_TASK_STEP_FAILED);
    CHECK(gh_task_threads_start(s.parent,2,parent_entry,&s,123,&slot)==GH_THREAD_OK);
    for(unsigned i=0;i<4;++i)CHECK(gh_tasks_run_matching_002d52a0(&p).status==GH_TASK_OK);
    CHECK(s.child_entered==1 && s.child_resumed==1 && s.parent_returned==1 && !s.retired_continued);
    CHECK(gh_tasks_run_matching_002d52a0(&c).status==GH_TASK_OK); /* root restored */
    CHECK(gh_task_threads_start(s.parent,2,nested_failure,&s,0,&slot)==GH_THREAD_OK);
    CHECK(gh_tasks_run_matching_002d52a0(&p).status==GH_TASK_OK);
    CHECK(gh_tasks_run_matching_002d52a0(&c).status==GH_TASK_OK);
    CHECK(gh_task_threads_start(s.parent,2,construct_child,&cc,0,&slot)==GH_THREAD_OK);
    CHECK(gh_tasks_run_matching_002d52a0(&p).status==GH_TASK_OK && cc.done);
    CHECK(gh_tasks_run_matching_002d52a0(&late).status==GH_TASK_OK); /* stable root owner */
    CHECK(gh_task_threads_destroy(s.parent)==GH_THREAD_BUSY && IsThreadAFiber());
    CHECK(gh_task_threads_destroy(late_backend)==GH_THREAD_OK);
    CHECK(gh_task_threads_destroy(s.child)==GH_THREAD_OK);
    CHECK(gh_task_threads_destroy(s.parent)==GH_THREAD_OK && !IsThreadAFiber());
    puts("PASS independent managers/nested stacks/sleep/tail restart/error restoration and root lifetime");return 0;
}
