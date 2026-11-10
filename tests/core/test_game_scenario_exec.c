#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "game_scenario_exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {gh_scene_scenario_view scenario;gh_task_threads*threads;int calls,resumed,child_calls;int32_t child_slot;} context;
static void child(void*p,int32_t arg){context*c=p;C(arg==123);c->child_calls++;}
static void parent(void*p,int32_t arg){context*c=p;C(arg==77);c->calls++;C(c->scenario.manager->current_index==2);uint32_t f=0;C(!gh_scenario_exec_002c3890(c->scenario,c->threads,-1,child,c,123,&c->child_slot,&f)&&c->child_slot==3);C(c->scenario.manager->records[3].attributes==3);C(!gh_task_threads_sleep(c->threads,1));c->resumed++;}
typedef struct {gh_scene_scenario_view s;gh_task_threads*threads;gh_scenario_exec_status result;int32_t slot;uint32_t frontier;int foreign_getter_null;} foreign_call;
static DWORD WINAPI foreign_thread(void*p){foreign_call*c=p;c->foreign_getter_null=gh_task_threads_manager(c->threads)==NULL;c->result=gh_scenario_exec_002c3890(c->s,c->threads,-1,NULL,NULL,0,&c->slot,&c->frontier);return 0;}
int main(void){uint8_t bytes[0x114];memset(bytes,0xa5,sizeof(bytes));gh_task_record32 records[16]={0};for(unsigned i=0;i<16;i++)records[i].thread_id=-1;gh_task_manager manager={.records=records,.capacity=16,.count=16,.current_index=-1};gh_task_threads*threads;C(!gh_task_threads_create(&manager,&threads));gh_scene_scenario_view s={bytes,sizeof(bytes),&manager};context c={.scenario=s,.threads=threads};uint32_t f=0;int32_t slot=99;
 C(gh_scenario_exec_002c3890(s,threads,-1,NULL,&c,0,&slot,&f)==GH_SCENARIO_EXEC_MISSING&&slot==-1&&f==0x2d5890&&bytes[0x64]==0xa5&&!records[2].running);
 C(!gh_scenario_exec_002c3890(s,threads,-1,parent,&c,77,&slot,&f)&&slot==2&&!c.calls);for(unsigned i=0;i<sizeof(bytes);i++)C(bytes[i]==(i>=0x64&&i<0x6c?0:0xa5));records[2].attributes=3;
 C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&c.calls==1&&!c.resumed&&c.child_calls==1&&c.child_slot==3);for(unsigned i=0x6c;i<0x74;i++)C(!bytes[i]);C(bytes[0x63]==0xa5&&bytes[0x74]==0xa5);
 C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&c.resumed==1&&!records[2].running&&!records[3].running);
 /* Originalscan skips0/1; fullselecteddomain returns-1 without requiringbody. */
 for(unsigned i=2;i<16;i++)records[i].running=1;C(!gh_scenario_exec_002c3890(s,threads,-1,NULL,NULL,0,&slot,&f)&&slot==-1);gh_task_record32 full_before[16];memcpy(full_before,records,sizeof(records));uint8_t scalar_before[sizeof(bytes)];memcpy(scalar_before,bytes,sizeof(bytes));foreign_call foreign_call_state={.s=s,.threads=threads,.slot=99};HANDLE native_thread=CreateThread(NULL,0,foreign_thread,&foreign_call_state,0,NULL);C(native_thread&&WaitForSingleObject(native_thread,5000)==WAIT_OBJECT_0);CloseHandle(native_thread);C(foreign_call_state.foreign_getter_null&&foreign_call_state.result==GH_SCENARIO_EXEC_ARGUMENT&&foreign_call_state.slot==-1&&!memcmp(full_before,records,sizeof(records))&&!memcmp(scalar_before,bytes,sizeof(bytes)));for(unsigned i=2;i<16;i++)records[i].running=0;
 gh_task_manager foreign=manager;s.manager=&foreign;C(gh_scenario_exec_002c3890(s,threads,0,child,&c,123,&slot,&f)==GH_SCENARIO_EXEC_ARGUMENT);C(!records[0].running);C(!gh_task_threads_destroy(threads));puts("PASS actualScenario16 selection/noearlyexecution/realcalleeCstackSleep/attrinherit/exact8byteclear/poolfull and foreignbinding checks");return 0;}
