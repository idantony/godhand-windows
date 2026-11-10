#include "game_objects.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {gh_game_objects *manager;gh_object_resource_bindings bindings;gh_object_resource_load op;gh_object_view view;gh_object_status status;uint32_t frontier;int done;int reference;} task;
static void load(void*p,int32_t kind){task*t=p;t->status=gh_game_objects_load_wait_001fe8a0(&t->op,t->manager,(uint32_t)kind,&t->bindings,t->reference,&t->view,&t->frontier);t->done=1;}
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));gh_heap*heap;C(!gh_heap_create(8*1024*1024,&heap));gh_dvd_jobs dvd;gh_dvd_jobs_init(&dvd,d);gh_task_record32 records[4]={0};gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};gh_task_threads*threads;C(!gh_task_threads_create(&manager,&threads));
 gh_stage_slots slots={0};gh_game_objects*objects;C(!gh_game_objects_create(&slots,0,&objects)&&!gh_stage_slots_reset(&slots,heap));
 task t={.manager=objects,.bindings={.dvd=&dvd,.heap=heap,.threads=threads},.reference=1};int32_t slot;C(!gh_task_threads_start(threads,2,load,&t,0x300,&slot));
 C(!gh_tasks_run_matching_002d52a0(&manager).status);
 gh_object_view pending;C(!gh_game_objects_find(objects,0x300,&pending)&&pending.state==1&&(pending.flags&1)&&!t.done&&pending.file_extent_known&&pending.file_extent<=pending.size);
 uint32_t job=dvd.id_counter;const uint8_t*first=pending.bytes;size_t used=gh_heap_used(heap);unsigned visits=0;
 while(!t.done&&visits++<12){C(!gh_tasks_run_matching_002d52a0(&manager).status&&!gh_dvd_pump(&dvd));}
 C(t.done&&t.status==GH_OBJECT_OK&&!t.frontier&&t.op.stage==4&&t.view.bytes==first&&t.view.state==3&&t.view.use_count==1&&!(t.view.flags&1)&&gh_heap_used(heap)==used);
 size_t index;gh_disc_entry entry;C(!gh_disc_find(d,"om/om00.dat",&index)&&!gh_disc_entry_at(d,index,&entry)&&t.view.file_extent==entry.size&&t.view.size==((entry.size+2047)&~UINT64_C(2047)));
 uint8_t*expected=malloc(t.view.size);C(expected&&!gh_disc_read_blocks(d,index,0,expected,t.view.size/2048)&&!memcmp(expected,t.view.bytes,t.view.size));free(expected);
 // Resuming completedoperation neverreacquires; neworiginalcall does.
 C(!gh_game_objects_load_wait_001fe8a0(&t.op,objects,0x300,&t.bindings,1,&t.view,&t.frontier)&&t.view.use_count==1&&dvd.id_counter==job);
 gh_object_resource_load existing={0};gh_object_resource_bindings noio={0};gh_object_view same;
 C(!gh_game_objects_load_wait_001fe8a0(&existing,objects,0x300,&noio,0,&same,&t.frontier)&&same.bytes==first&&same.use_count==2&&dvd.id_counter==job&&gh_heap_used(heap)==used);
 C(gh_game_objects_set_file_extent(objects,0x300,first+1,1)==GH_OBJECT_BOUNDS&&gh_game_objects_set_file_extent(objects,0x300,first,t.view.size+1)==GH_OBJECT_BOUNDS);
 C(gh_game_objects_reset_records_001ff028(objects,&t.frontier)==GH_OBJECT_BUSY&&t.frontier==0x2a9680);
 // A realnonempty soundslot isNOT silentlyready. The lateunsupportedreturn
 // retains actualloadedbuffer/recordstate1 andoperation2 forrealcontinuation.
 task blocked={.manager=objects,.bindings=t.bindings,.reference=0};uint8_t*free_record=slots.records+0x5c;C(!word(free_record));uint32_t sound=0;memcpy(free_record+0x24,&sound,4);
 C(!gh_task_threads_start(threads,2,load,&blocked,0x303,&slot));visits=0;while(!blocked.done&&visits++<12){C(!gh_tasks_run_matching_002d52a0(&manager).status&&!gh_dvd_pump(&dvd));}
 C(blocked.done&&blocked.status==GH_OBJECT_UNSUPPORTED&&blocked.frontier==0x2cb778&&blocked.op.stage==2&&word(free_record)==1);
 uint32_t empty=UINT32_MAX;memcpy(free_record+0x24,&empty,4);blocked.done=0;
 C(!gh_task_threads_start(threads,2,load,&blocked,0x303,&slot));while(!blocked.done)C(!gh_tasks_run_matching_002d52a0(&manager).status);
 C(blocked.status==GH_OBJECT_OK&&blocked.view.state==2&&blocked.view.use_count==-100);
 // Nativeabort: canceltheactualtask beforemanager destroysownedpendingjob.
 gh_game_objects_destroy(objects);C(!gh_heap_used(heap));slots=(gh_stage_slots){0};C(!gh_game_objects_create(&slots,0,&objects)&&!gh_stage_slots_reset(&slots,heap));
 task abort={.manager=objects,.bindings=t.bindings,.reference=1};C(!gh_task_threads_start(threads,2,load,&abort,0x304,&slot));C(!gh_tasks_run_matching_002d52a0(&manager).status&&!abort.done&&gh_heap_used(heap)>0);
 C(!gh_task_threads_cancel(threads,2));uint32_t pending_job=dvd.id_counter;gh_game_objects_destroy(objects);objects=NULL;C(!gh_dvd_check(&dvd,pending_job)&&!gh_heap_used(heap));
 C(!gh_task_threads_destroy(threads));C(!gh_heap_used(heap)&&!gh_heap_destroy(heap));gh_disc_close(d);
 printf("PASS actual1FE8A0 om00/om03 DVD+fiber resources, fullsectorbytes/fileextent-capacity/state/ref/pinnedreuse/noIOshortcut/unknownsoundfrontier/ownedcleanup in%u visits; notmodelready\n",visits);return 0;
}
