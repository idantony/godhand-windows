#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../../src/platform/memory_card.h"
#include "../../src/platform/task_threads.h"
#include "../../src/core/mc_boot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct flow {
    gh_mc_boot_checks *state;gh_memory_card *card;gh_task_threads *threads;
    uint32_t flags;int completed;
} flow;
static void check_task(void *context,int32_t unused)
{
    flow *f=context;gh_mc_boot_error e;
    CHECK(unused==91);
    while(f->state->phase<2) {
        e=gh_mc_boot_check_control(f->state,f->card,&f->flags);
        if(e==GH_MC_BOOT_NEEDS_SLEEP) {
            CHECK(f->state->phase==0 && f->state->delay_frames>0);
            CHECK(gh_task_threads_sleep(f->threads,(int32_t)f->state->delay_frames)==GH_THREAD_OK);
            CHECK(gh_mc_boot_delay_completed(f->state)==GH_MC_BOOT_OK);
        } else {
            CHECK(e==GH_MC_BOOT_OK);
        }
        if(f->state->phase<2)CHECK(gh_task_threads_sleep(f->threads,1)==GH_THREAD_OK);
    }
    CHECK(gh_mc_boot_check_control(f->state,f->card,&f->flags)==GH_MC_BOOT_UNSUPPORTED);
    f->completed=1; /* check slice finished, NOT original MC Main completed */
}
int main(int argc,char **argv)
{
    char canonical[1200],profile[1400],folder[1500],icon[1600],absent[1500];
    static const char suffix[]="\\memory-card-tests\\scratch";
    size_t canonical_length;
    gh_memory_card *card,*missing;gh_mc_result result;int done;FILE *file;
    gh_heap *heap;gh_mc_boot_checks state;uint32_t flags=4;
    gh_task_record32 records[3]={{0}};gh_task_manager manager={records,3,0,-1,3,0,0,0};
    flow f={&state,0,0,4,0};int32_t slot;unsigned ticks=0,i;int32_t sentinel;
    CHECK(argc==2 && GetFullPathNameA(argv[1],sizeof(canonical),canonical,0));
    canonical_length=strlen(canonical);
    while(canonical_length>3 && canonical[canonical_length-1]=='\\')canonical[--canonical_length]=0;
    CHECK(canonical_length>=sizeof(suffix)-1);
    CHECK(!_stricmp(canonical+canonical_length-(sizeof(suffix)-1),suffix));
    /* Accept the build's external scratch root, with canonical absolute path
     * and a task-specific directory suffix; only a newly created PID child is
     * mutated/deleted, never recursive cleanup or preexisting profile data. */
    CHECK(snprintf(profile,sizeof(profile),"%s\\profile-%lu",canonical,(unsigned long)GetCurrentProcessId())>0);
    CHECK(CreateDirectoryA(profile,0));
    CHECK(snprintf(folder,sizeof(folder),"%s\\BASLUS-21503GODHD003",profile)>0);
    CHECK(CreateDirectoryA(folder,0));
    CHECK(snprintf(icon,sizeof(icon),"%s\\icon.sys",folder)>0);
    file=fopen(icon,"wb");CHECK(file);CHECK(fwrite("test-icon",1,9,file)==9);CHECK(!fclose(file));
    CHECK(snprintf(absent,sizeof(absent),"%s\\not-inserted",profile)>0);
    CHECK(gh_memory_card_open(profile,&card)==GH_MC_OK);
    CHECK(gh_memory_card_request_info(card)==GH_MC_OK);
    CHECK(gh_memory_card_request_icon(card,0)==GH_MC_BUSY);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_OK && done);
    CHECK(result.result==0 && result.card_type==2 && result.formatted==1 && result.free_clusters>=150);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_OK && !done);
    CHECK(gh_memory_card_request_icon(card,3)==GH_MC_OK);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_OK && done && result.result==1);
    CHECK(gh_memory_card_request_icon(card,0)==GH_MC_OK);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_OK && done && result.result==0);
    CHECK(gh_memory_card_request_icon(card,20)==GH_MC_ARGUMENT);
    CHECK(gh_heap_create(65536,&heap)==GH_MEM_OK);
    CHECK(gh_mc_boot_checks_create(&state,heap)==GH_MC_BOOT_OK);
    CHECK(gh_heap_used(heap)==0x5000 && (uintptr_t)state.icon_work.pointer%64==0);
    for(i=0;i<20;++i) {
        memcpy(&sentinel,(unsigned char*)state.icon_work.pointer+i*0x400+4,4);
        CHECK(sentinel==-1);
    }
    f.card=card;
    CHECK(gh_task_threads_create(&manager,&f.threads)==GH_THREAD_OK);
    CHECK(gh_task_threads_start(f.threads,-1,check_task,&f,91,&slot)==GH_THREAD_OK);
    while(!f.completed && ticks<100) {CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);++ticks;}
    CHECK(f.completed && ticks==32 && state.phase==2 && !state.done);
    CHECK(state.present_mask==(1u<<3) && state.file_index==20 && !(f.flags&4));
    CHECK(gh_task_threads_destroy(f.threads)==GH_THREAD_OK);
    CHECK(gh_mc_boot_checks_release(&state,heap)==GH_MC_BOOT_OK);
    /* A genuinely missing native profile enters the original card-error path;
     * it is not silently treated as an empty inserted/formatted card. */
    CHECK(gh_memory_card_open(absent,&missing)==GH_MC_OK);
    CHECK(gh_mc_boot_checks_create(&state,heap)==GH_MC_BOOT_OK);
    CHECK(gh_mc_boot_check_control(&state,missing,&flags)==GH_MC_BOOT_OK);
    CHECK(gh_mc_boot_check_control(&state,missing,&flags)==GH_MC_BOOT_OK);
    CHECK(state.phase==3 && state.error==1 && !state.done);
    CHECK(gh_mc_boot_check_control(&state,missing,&flags)==GH_MC_BOOT_UNSUPPORTED);
    CHECK(gh_mc_boot_checks_release(&state,heap)==GH_MC_BOOT_OK);
    gh_memory_card_close(missing);gh_memory_card_close(card);
    CHECK(gh_heap_used(heap)==0 && gh_heap_destroy(heap)==GH_MEM_OK);
    CHECK(DeleteFileA(icon));CHECK(RemoveDirectoryA(folder));CHECK(RemoveDirectoryA(profile));
    puts("PASS real native card queries and recovered slot/file check slice with32 real fiber ticks; MC Main/UI/load pending");
    return 0;
}
