/* Native integration of recovered0030D358, not an emulator or new game loop.
 * Actual control-flow evidence: startup-task-evidence.json and ghidra-startup.json.
 * Host policies/timing substitutions: startup-host-verification.json. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "startup_host.h"
#include "../core/dvd_jobs.h"
#include <stdlib.h>
#include <string.h>

typedef struct loaded_bundle {
    gh_r207_allocation allocation;
    size_t bytes;
    gh_bundle view;
    int opened;
} loaded_bundle;
struct gh_startup_host {
    gh_disc *disc;
    gh_heap *heap;
    gh_audio_registry *audio;
    gh_task_threads *threads;
    gh_task_record32 records[4];
    gh_task_manager manager;
    gh_dvd_jobs dvd;
    gh_boot_work work;
    gh_startup_task task;
    gh_startup_platform platform;
    loaded_bundle bundles[2];
    unsigned bundle_count;
    gh_font_state font;
    gh_menu_state menu;
    gh_id_state id,*id_global;
    gh_bundle_entry idd;
    gh_host_phase phase;
    gh_host_error error;
    int subsystem_error,startup_returned;
    uint32_t unavailable_entry;
    uint64_t ticks;
    DWORD owner_thread;
    void *owner_fiber;
};
static int on_owner(const gh_startup_host *h)
{return GetCurrentThreadId()==h->owner_thread && IsThreadAFiber() && GetCurrentFiber()==h->owner_fiber;}
static int failure(gh_startup_host *h,gh_host_error error,int detail)
{h->error=error;h->subsystem_error=detail;return (int)error;}
static int sound_startup(void *context)
{
    gh_startup_host *h=context;
    gh_audio_status e=gh_audio_startup_load(h->audio,h->disc);
    return e?failure(h,GH_HOST_AUDIO,(int)e):0;
}
static int read_alloc(void *context,const char *path,void **out,int32_t *job)
{
    gh_startup_host *h=context;loaded_bundle *b;size_t index;gh_disc_entry entry;
    uint32_t id;
    if(!out || !job || h->bundle_count>=2)return failure(h,GH_HOST_ARGUMENT,0);
    if(gh_disc_find(h->disc,path,&index)!=GH_DISC_OK ||
       gh_disc_entry_at(h->disc,index,&entry)!=GH_DISC_OK || entry.size>SIZE_MAX)
        return failure(h,GH_HOST_DISC,0);
    b=&h->bundles[h->bundle_count];
    id=gh_dvd_read_alloc(&h->dvd,path,&b->allocation,h->heap,0,0,0,0);
    if(!id)return failure(h,GH_HOST_DISC,(int)h->dvd.last_error);
    b->bytes=(size_t)entry.size;++h->bundle_count;
    *out=b->allocation.pointer;
    /* Preserve a32-bit job token without signed conversion assumptions. */
    memcpy(job,&id,sizeof(id));return 0;
}
static int wait_frame(void *context,int32_t job)
{
    gh_startup_host *h=context;uint32_t id;gh_dvd_error e;
    memcpy(&id,&job,sizeof(id));
    e=gh_dvd_check_wait_frame(&h->dvd,id,h->threads);
    return e?failure(h,GH_HOST_DISC,(int)e):0;
}
static loaded_bundle *language_bundle(gh_startup_host *h,const gh_startup_task *task)
{
    unsigned i;
    for(i=0;i<h->bundle_count;++i) {
        loaded_bundle *b=&h->bundles[i];
        if(b->allocation.pointer==task->language_data) {
            if(!b->opened) {
                gh_bundle_error e=gh_bundle_open(b->allocation.pointer,b->bytes,&b->view);
                if(e) {failure(h,GH_HOST_ASSET,(int)e);return 0;}
                b->opened=1;
            }
            return b;
        }
    }
    failure(h,GH_HOST_ASSET,GH_BUNDLE_NOT_FOUND);return 0;
}
static int bind_fonts(void *context,gh_startup_task *task)
{
    gh_startup_host *h=context;loaded_bundle *b=language_bundle(h,task);gh_font_error e;
    if(!b)return (int)h->error;
    e=gh_font_initialize(&h->font,&b->view,h->work.language);
    return e?failure(h,GH_HOST_ASSET,(int)e):0;
}
static int load_menu(void *context,gh_startup_task *task)
{
    gh_startup_host *h=context;loaded_bundle *b=language_bundle(h,task);gh_menu_error e;
    if(!b)return (int)h->error;
    e=gh_menu_load_tables_002ae990(&h->menu,&b->view);
    return e?failure(h,GH_HOST_ASSET,(int)e):0;
}
static int bind_id(void *context,gh_startup_task *task)
{
    gh_startup_host *h=context;gh_menu_error e;(void)task;
    e=gh_id_bind_global(&h->id_global,&h->id);
    return e?failure(h,GH_HOST_ASSET,(int)e):0;
}
static int find_id(void *context,void *data,const char *name,int32_t index,void **result)
{
    gh_startup_host *h=context;loaded_bundle *b;gh_bundle_error e;
    if(!result || data!=h->task.language_data || index<0)
        return failure(h,GH_HOST_ARGUMENT,0);
    b=language_bundle(h,&h->task);if(!b)return (int)h->error;
    e=gh_bundle_find(&b->view,name,(uint32_t)index,&h->idd);
    if(e)return failure(h,GH_HOST_ASSET,(int)e);
    *result=(void *)h->idd.bytes;return 0;
}
static int init_id(void *context,void *data)
{
    gh_startup_host *h=context;gh_menu_error e;
    if(!h->id_global || data!=h->idd.bytes)return failure(h,GH_HOST_ARGUMENT,0);
    e=gh_id_system_init_002acc70(h->id_global,&h->idd);
    return e?failure(h,GH_HOST_ASSET,(int)e):0;
}
static int register_task(void *context,uint32_t entry,int32_t slot)
{
    gh_startup_host *h=context;(void)slot;
    /* In particular002AE8A8 loads mcport.rel and calls its imported Main.
     * No successful placeholder may hide that original-game dependency. */
    h->unavailable_entry=entry;
    return failure(h,GH_HOST_ENTRY_UNAVAILABLE,0);
}
static void startup_entry(void *context,int32_t argument)
{
    gh_startup_host *h=context;int result;(void)argument;
    result=gh_startup_task_0030d358(&h->task,&h->platform);
    h->startup_returned=1;
    h->phase=!result?GH_HOST_COMPLETE:
        (h->error==GH_HOST_ENTRY_UNAVAILABLE?GH_HOST_FRONTIER:GH_HOST_FAILED);
}
gh_host_error gh_startup_host_create(const char *iso,int32_t language,
    uint32_t flags598,size_t capacity,gh_startup_host **out)
{
    gh_startup_host *h;gh_host_error e=GH_HOST_MEMORY;int32_t slot;
    if(!out)return GH_HOST_ARGUMENT;
    *out=0;if(!iso || !capacity || (uint32_t)language>=7u)return GH_HOST_ARGUMENT;
    h=calloc(1,sizeof(*h));if(!h)return GH_HOST_MEMORY;
    h->task.failed_operation=GH_START_OP_COUNT;
    if(gh_disc_open(iso,&h->disc)!=GH_DISC_OK) {e=GH_HOST_DISC;goto failed;}
    if(gh_heap_create(capacity,&h->heap)!=GH_MEM_OK)goto failed;
    if(gh_audio_registry_create(&h->audio)!=GH_AUDIO_OK)goto failed;
    h->manager.records=h->records;h->manager.capacity=4;h->manager.count=4;
    h->manager.current_index=-1;
    if(gh_task_threads_create(&h->manager,&h->threads)!=GH_THREAD_OK) {e=GH_HOST_THREADS;goto failed;}
    h->owner_thread=GetCurrentThreadId();h->owner_fiber=GetCurrentFiber();
    gh_dvd_jobs_init(&h->dvd,h->disc);
    h->work.language=language;h->work.flags598=flags598;h->task.work=&h->work;
    h->platform=(gh_startup_platform){h,sound_startup,read_alloc,wait_frame,
        bind_fonts,load_menu,bind_id,find_id,init_id,register_task};
    if(gh_task_threads_start(h->threads,3,startup_entry,h,0,&slot)!=GH_THREAD_OK || slot!=3)
        {e=GH_HOST_THREADS;goto failed;}
    *out=h;return GH_HOST_OK;
failed:
    if(h->threads) {
        unsigned i;
        for(i=0;i<4;++i)(void)gh_task_threads_cancel(h->threads,(int32_t)i);
        (void)gh_task_threads_destroy(h->threads);
    }
    if(h->audio)gh_audio_registry_destroy(h->audio);
    if(h->heap)(void)gh_heap_destroy(h->heap);
    if(h->disc)gh_disc_close(h->disc);
    free(h);return e;
}
gh_host_phase gh_startup_host_tick(gh_startup_host *h)
{
    gh_task_run_result r;gh_dvd_error e;
    if(!h)return GH_HOST_FAILED;
    if(!on_owner(h))return GH_HOST_FAILED;
    if(h->phase!=GH_HOST_RUNNING)return h->phase;
    r=gh_tasks_run_matching_002d52a0(&h->manager);
    if(r.status) {failure(h,GH_HOST_THREADS,r.platform_error);h->phase=GH_HOST_FAILED;return h->phase;}
    ++h->ticks;
    e=gh_dvd_pump(&h->dvd);
    if(e) {failure(h,GH_HOST_DISC,(int)e);h->phase=GH_HOST_FAILED;}
    return h->phase;
}
gh_host_error gh_startup_host_snapshot(const gh_startup_host *h,gh_startup_snapshot *out)
{
    if(!h || !out)return GH_HOST_ARGUMENT;
    *out=(gh_startup_snapshot){h->phase,h->error,h->subsystem_error,
        h->task.failed_operation,h->unavailable_entry,h->ticks,h->work.flags,
        h->bundle_count,h->startup_returned,&h->font,&h->menu,h->id_global,h->audio};
    return GH_HOST_OK;
}
gh_host_error gh_startup_host_destroy(gh_startup_host *h)
{
    unsigned i;
    if(!h)return GH_HOST_ARGUMENT;
    if(!on_owner(h))return GH_HOST_THREADS;
    /* Cancel before freeing any views retained on a suspended C stack. */
    for(i=0;i<4;++i)if(gh_task_threads_cancel(h->threads,(int32_t)i)!=GH_THREAD_OK)
        return GH_HOST_THREADS;
    if(gh_task_threads_destroy(h->threads)!=GH_THREAD_OK)return GH_HOST_THREADS;
    for(;;) {
        for(i=0;i<GH_DVD_JOB_COUNT;++i)if(h->dvd.jobs[i].state)break;
        if(i==GH_DVD_JOB_COUNT)break;
        if(!gh_dvd_cancel(&h->dvd,h->dvd.jobs[i].id))return GH_HOST_DISC;
    }
    for(i=0;i<h->bundle_count;++i)
        (void)gh_heap_release(h->heap,h->bundles[i].allocation);
    gh_audio_registry_destroy(h->audio);
    if(gh_heap_destroy(h->heap)!=GH_MEM_OK)return GH_HOST_MEMORY;
    gh_disc_close(h->disc);free(h);return GH_HOST_OK;
}
