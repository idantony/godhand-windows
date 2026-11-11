#include "game_scenario_exec.h"
#include <string.h>
gh_scenario_exec_status gh_scenario_exec_002c3890(gh_scene_scenario_view s,gh_task_threads*threads,int32_t request,gh_native_task_entry entry,void*context,int32_t argument,int32_t*slot,uint32_t*f){
 if(f)*f=0;if(!slot)return GH_SCENARIO_EXEC_ARGUMENT;*slot=-1;
 /* Registration uses the bound backend identity. Scenario traversal may
  * wrap the manager step and its context while its script registers a task. */
 if(!s.bytes||!s.manager||!threads||gh_task_threads_manager(threads)!=s.manager||!s.manager->records||s.manager->count>s.manager->capacity||!s.manager->step||s.size<0x54||(size_t)s.manager->count>(s.size-0x54)/8)return GH_SCENARIO_EXEC_ARGUMENT;
 gh_task_selection selected=gh_tasks_select_002d5b30(s.manager,request);if(selected.status!=GH_TASK_OK)return GH_SCENARIO_EXEC_ARGUMENT;
 if(selected.index<0)return GH_SCENARIO_EXEC_OK; /* originalpoolfull doesnotcallentry */
 if(!entry){if(f)*f=0x2d5890;return GH_SCENARIO_EXEC_MISSING;}
 gh_thread_status e=gh_task_threads_start(threads,request,entry,context,argument,slot);if(e){if(f)*f=0x2d5890;return GH_SCENARIO_EXEC_THREAD;}
 if(*slot>=0){if((uint32_t)*slot>=s.manager->count){if(f)*f=0x2c3968;return GH_SCENARIO_EXEC_ARGUMENT;}memset(s.bytes+0x54+(size_t)*slot*8,0,8);}
 return GH_SCENARIO_EXEC_OK;
}
