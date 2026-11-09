#include "tasks.h"
#include <stdio.h>
#include <string.h>
static int failures;
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); ++failures; } } while(0)
typedef struct trace { int seen[16]; unsigned n; int action; } trace;
static int step(void *context,gh_task_manager *m,gh_task_record32 *work)
{
    trace *t=context;
    CHECK(m->current==work && m->current_index>=0);
    CHECK(work==&m->records[m->current_index]);
    CHECK(t->n<16);
    if(t->n<16) t->seen[t->n++]=m->current_index;
    if(t->action==1 && m->current_index==0) {
        m->flags=2; m->records[2].attributes=2; m->count=4;
    }
    if(t->action==2 && m->current_index==0) m->count=1;
    if(t->action==3) return -19;
    return 0;
}
int main(void)
{
    gh_task_record32 records[5];
    trace t={{0},0,0};
    gh_task_manager m={records,5,0,-1,5,0,step,&t};
    gh_task_selection choice;
    gh_task_run_result r;
    unsigned i;
    memset(records,0,sizeof(records));
    /* Slots 0/1 are reserved only for automatic search. Selecting twice does
     * not reserve: the original selector never writes running. */
    CHECK(gh_tasks_select_002d5b30(&m,-1).index==2);
    CHECK(gh_tasks_select_002d5b30(&m,-7).index==2);
    CHECK(records[2].running==0);
    CHECK(gh_tasks_select_002d5b30(&m,0).index==0);
    records[2].running=0x8000;
    CHECK(gh_tasks_select_002d5b30(&m,2).index==-1);
    CHECK(gh_tasks_select_002d5b30(&m,-1).index==3);
    records[3].running=1; records[4].running=1;
    CHECK(gh_tasks_select_002d5b30(&m,-1).index==-1);
    CHECK(gh_tasks_select_002d5b30(&m,5).index==-1);
    m.count=2;
    CHECK(gh_tasks_select_002d5b30(&m,-1).index==-1);
    m.count=6; choice=gh_tasks_select_002d5b30(&m,0);
    CHECK(choice.status==GH_TASK_INVALID_STATE);
    m.count=5;
    /* Copy the full attr byte, including high bits; idle/-1 does nothing. */
    records[0].attributes=0xa7; records[3].attributes=0x44;
    CHECK(gh_tasks_inherit_002d5440(&m,3)==GH_TASK_OK && records[3].attributes==0x44);
    m.current=&records[0];m.current_index=0;
    CHECK(gh_tasks_inherit_002d5440(&m,-1)==GH_TASK_OK);
    CHECK(gh_tasks_inherit_002d5440(&m,3)==GH_TASK_OK && records[3].attributes==0xa7);
    CHECK(gh_tasks_inherit_002d5440(&m,6)==GH_TASK_INVALID_STATE);
    /* Filtering requires both enabled low bits; high flags are ignored.
     * running is deliberately zero: visitation delegates thread activity. */
    for(i=0;i<5;++i) {records[i].running=0;records[i].attributes=(uint8_t)i;}
    m.flags=0x80000003u;t.n=0;
    r=gh_tasks_run_matching_002d52a0(&m);
    CHECK(r.status==GH_TASK_OK && t.n==1 && t.seen[0]==3);
    CHECK(!m.current && m.current_index==-1);
    m.flags=0x80000000u;t.n=0;
    r=gh_tasks_run_matching_002d52a0(&m);
    CHECK(r.status==GH_TASK_OK && t.n==5);
    for(i=0;i<t.n;++i) CHECK(t.seen[i]==(int)i);
    /* Actual loop reloads flags and count after a callback: future slots can
     * become eligible and count can grow within allocated memory. */
    memset(records,0,sizeof(records));m.count=2;m.flags=0;t.n=0;t.action=1;
    records[3].attributes=2;
    r=gh_tasks_run_matching_002d52a0(&m);
    CHECK(r.status==GH_TASK_OK && t.n==3);
    CHECK(t.seen[0]==0 && t.seen[1]==2 && t.seen[2]==3);
    m.count=5;m.flags=0;t.n=0;t.action=2;
    r=gh_tasks_run_matching_002d52a0(&m);
    CHECK(r.status==GH_TASK_OK && t.n==1);
    m.count=5;t.n=0;t.action=3;
    r=gh_tasks_run_matching_002d52a0(&m);
    CHECK(r.status==GH_TASK_STEP_FAILED && r.platform_error==-19 && t.n==1);
    CHECK(m.current==records && m.current_index==0);
    m.step=0;t.n=0;
    CHECK(gh_tasks_run_matching_002d52a0(&m).status==GH_TASK_MISSING_STEP && t.n==0);
    m.count=0;
    CHECK(gh_tasks_run_matching_002d52a0(&m).status==GH_TASK_OK);
    CHECK(!m.current && m.current_index==-1);
    if(failures) return 1;
    puts("Task selection/inheritance/filter traversal passed; EE thread runtime pending");
    return 0;
}
