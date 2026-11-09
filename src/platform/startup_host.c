/* Native integration of recovered0030D358, not an emulator or new game loop.
 * Actual control-flow evidence: startup-task-evidence.json and ghidra-startup.json.
 * Host policies/timing substitutions: startup-host-verification.json. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "startup_host.h"
#include "../core/dvd_jobs.h"
#include "../core/game_gate.h"
#include "../core/game_init.h"
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
    unsigned next_tasks;
    gh_game_assets game_assets;
    gh_game_loop game_loop;
    gh_game_platform game_platform;
    gh_game_gate game_gate;
    gh_game_init_state game_init;
    unsigned char save_bytes[GH_CORE_SAVE_BYTES];
    float game_rate;
    int game_assets_loaded;
    gh_memory_card *card;
    gh_id_runtime *id_runtime;
    gh_mc_task_state mc;
    gh_mc_task_bindings mc_bindings;
    gh_packed_message_state mc_messages;
    int32_t mc_game_suspend;
    const char *frontier_module;
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
    if(e)return failure(h,GH_HOST_ASSET,(int)e);
    {gh_id_error status=gh_id_runtime_create(h->id_global,1,&h->id_runtime);
     return status?failure(h,GH_HOST_ASSET,(int)status):0;}
}
static int game_cover(void *context,gh_game_loop *loop)
{
    gh_startup_host *h=context;(void)loop;
    return (int)gh_game_gate_002a6ca8(&h->game_gate);
}
static int game_modal(void *context,gh_game_loop *loop)
{
    gh_startup_host *h=context;(void)loop;
    return (int)gh_game_gate_002a6da8(&h->game_gate);
}
static int game_initialize(void *context,gh_game_loop *loop)
{
    gh_startup_host *h=context;
    gh_core_save_view save={h->save_bytes,sizeof(h->save_bytes)};
    return (int)gh_game_init_002a5b18(&h->game_init,&h->work,
        &loop->game_state[0],&h->game_rate,save);
}
static void game_main_entry(void *context,int32_t argument)
{
    gh_startup_host *h=context;gh_game_status e;(void)argument;
    e=gh_game_main_002a5998(&h->game_loop,&h->game_platform);
    if(e) {
        if(e==GH_GAME_UNAVAILABLE) {
            if(!h->unavailable_entry)h->unavailable_entry=h->game_gate.failed_entry?
                h->game_gate.failed_entry:h->game_loop.failed_entry;
            (void)failure(h,GH_HOST_ENTRY_UNAVAILABLE,h->game_loop.platform_error);
        } else {
            (void)failure(h,e==GH_GAME_DISC?GH_HOST_DISC:GH_HOST_THREADS,(int)e);
            h->phase=GH_HOST_FAILED;
        }
    }
}
static void game_assets_entry(void *context,int32_t argument)
{
    gh_startup_host *h=context;gh_game_status e;(void)argument;
    gh_core_save_view save={h->save_bytes,sizeof(h->save_bytes)};
    e=gh_game_assets_0030d5c8(&h->game_assets,gh_core_save_costume(save));
    if(!e)h->game_assets_loaded=1;
    else {
        (void)failure(h,e==GH_GAME_MEMORY?GH_HOST_MEMORY:GH_HOST_ASSET,(int)e);
        h->phase=GH_HOST_FAILED;
    }
}
static void mc_entry(void *context,int32_t argument)
{
    gh_startup_host *h=context;gh_mc_task_status e;(void)argument;
    e=gh_mc_task_prepare_002ae8a8(&h->mc,&h->mc_bindings);
    if(e==GH_MC_TASK_CONTROL_FRONTIER || e==GH_MC_TASK_UNSUPPORTED) {
        h->unavailable_entry=h->mc.frontier_offset;
        h->frontier_module=h->mc.frontier_offset<0x100000?"mcport.rel":0;
        (void)failure(h,GH_HOST_ENTRY_UNAVAILABLE,(int)e);
    } else if(e!=GH_MC_TASK_OK) {
        (void)failure(h,GH_HOST_ASSET,(int)e);h->phase=GH_HOST_FAILED;
    }
}
static int register_task(void *context,uint32_t entry,int32_t slot)
{
    gh_startup_host *h=context;gh_native_task_entry native=0;int32_t chosen;
    gh_thread_status e;
    if(entry==0x2a5998)native=game_main_entry;
    else if(entry==0x30d5c8)native=game_assets_entry;
    else if(entry==0x2ae8a8) {
        loaded_bundle *language=language_bundle(h,&h->task);
        if(!language)return (int)h->error;
        h->mc_bindings=(gh_mc_task_bindings){&h->dvd,h->heap,h->heap,h->threads,
            h->card,h->id_global,h->id_runtime,&h->font,&h->mc_messages,
            &language->view,&h->work,&h->mc_game_suspend};
        native=mc_entry;
    }
    if(native) {
        e=gh_task_threads_start(h->threads,slot,native,h,0,&chosen);
        if(e || chosen!=slot)return failure(h,GH_HOST_THREADS,(int)e);
        ++h->next_tasks;return 0;
    }
    /* No successful placeholder may hide another original-game dependency. */
    h->unavailable_entry=entry;
    return failure(h,GH_HOST_ENTRY_UNAVAILABLE,0);
}
static void startup_entry(void *context,int32_t argument)
{
    gh_startup_host *h=context;int result;(void)argument;
    result=gh_startup_task_0030d358(&h->task,&h->platform);
    h->startup_returned=1;
    h->phase=!result?(h->next_tasks?GH_HOST_RUNNING:GH_HOST_COMPLETE):
        (h->error==GH_HOST_ENTRY_UNAVAILABLE?GH_HOST_FRONTIER:GH_HOST_FAILED);
}
gh_host_error gh_startup_host_create(const char *iso,int32_t language,
    uint32_t flags598,size_t capacity,gh_startup_host **out)
{return gh_startup_host_create_with_profile(iso,"userdata/memory_card",language,flags598,capacity,out);}
gh_host_error gh_startup_host_create_with_profile(const char *iso,const char *profile,
    int32_t language,uint32_t flags598,size_t capacity,gh_startup_host **out)
{
    gh_startup_host *h;gh_host_error e=GH_HOST_MEMORY;int32_t slot;
    if(!out)return GH_HOST_ARGUMENT;
    *out=0;if(!iso || !profile || !capacity || (uint32_t)language>=7u)return GH_HOST_ARGUMENT;
    h=calloc(1,sizeof(*h));if(!h)return GH_HOST_MEMORY;
    h->task.failed_operation=GH_START_OP_COUNT;
    if(gh_disc_open(iso,&h->disc)!=GH_DISC_OK) {e=GH_HOST_DISC;goto failed;}
    if(gh_heap_create(capacity,&h->heap)!=GH_MEM_OK)goto failed;
    if(gh_audio_registry_create(&h->audio)!=GH_AUDIO_OK)goto failed;
    if(gh_memory_card_open(profile,&h->card)!=GH_MC_OK) {e=GH_HOST_ARGUMENT;goto failed;}
    h->manager.records=h->records;h->manager.capacity=4;h->manager.count=4;
    h->manager.current_index=-1;
    if(gh_task_threads_create(&h->manager,&h->threads)!=GH_THREAD_OK) {e=GH_HOST_THREADS;goto failed;}
    h->owner_thread=GetCurrentThreadId();h->owner_fiber=GetCurrentFiber();
    gh_dvd_jobs_init(&h->dvd,h->disc);
    h->work.language=language;h->work.flags598=flags598;h->task.work=&h->work;
    gh_game_init_state_initialize(&h->game_init);
    {gh_core_save_view save={h->save_bytes,sizeof(h->save_bytes)};
     if(gh_core_save_system_init(save)!=GH_INIT_OK)goto failed;}
    h->game_loop.work=&h->work;h->game_loop.threads=h->threads;
    h->game_platform.context=h;h->game_platform.dvd_cover=game_cover;
    h->game_platform.modal_input=game_modal;h->game_platform.states[0]=game_initialize;
    h->game_gate.work=&h->work;h->game_gate.dvd=&h->dvd;
    h->game_gate.threads=h->threads;h->game_gate.game_state=h->game_loop.game_state;
    h->game_assets.work=&h->work;h->game_assets.dvd=&h->dvd;
    h->game_assets.heap=h->heap;h->game_assets.threads=h->threads;
    h->game_assets.sound_context=h;h->game_assets.sound_startup=sound_startup;
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
    if(h->card)gh_memory_card_close(h->card);
    if(h->heap)(void)gh_heap_destroy(h->heap);
    if(h->disc)gh_disc_close(h->disc);
    free(h);return e;
}
gh_host_phase gh_startup_host_tick(gh_startup_host *h)
{
    gh_task_run_result r;gh_dvd_error e;unsigned i;int active=0;
    if(!h)return GH_HOST_FAILED;
    if(!on_owner(h))return GH_HOST_FAILED;
    if(h->phase!=GH_HOST_RUNNING)return h->phase;
    r=gh_tasks_run_matching_002d52a0(&h->manager);
    if(r.status) {failure(h,GH_HOST_THREADS,r.platform_error);h->phase=GH_HOST_FAILED;return h->phase;}
    ++h->ticks;
    e=gh_dvd_pump(&h->dvd);
    if(e) {failure(h,GH_HOST_DISC,(int)e);h->phase=GH_HOST_FAILED;}
    if(h->phase==GH_HOST_RUNNING && h->startup_returned) {
        for(i=0;i<4;++i)if(h->records[i].running)active=1;
        if(!active)h->phase=h->error==GH_HOST_ENTRY_UNAVAILABLE?GH_HOST_FRONTIER:GH_HOST_COMPLETE;
    }
    return h->phase;
}
gh_host_error gh_startup_host_snapshot(const gh_startup_host *h,gh_startup_snapshot *out)
{
    if(!h || !out)return GH_HOST_ARGUMENT;
    *out=(gh_startup_snapshot){h->phase,h->error,h->subsystem_error,
        h->task.failed_operation,h->unavailable_entry,h->ticks,h->work.flags,
        h->bundle_count,h->startup_returned,&h->font,&h->menu,h->id_global,h->audio,
        h->game_loop.game_state[0],h->game_assets_loaded,h->game_rate,h->frontier_module,
        h->mc.ui_initialized?&h->mc.id_base:0,h->mc.ui_initialized?&h->mc_messages:0};
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
    if(h->mc.stage && gh_mc_task_abort(&h->mc,&h->mc_bindings)!=GH_MC_TASK_OK)
        return GH_HOST_ASSET;
    if(h->id_runtime && gh_id_runtime_destroy(h->id_runtime)!=GH_ID_OK)return GH_HOST_ASSET;
    for(i=0;i<h->bundle_count;++i)
        (void)gh_heap_release(h->heap,h->bundles[i].allocation);
    if(gh_game_assets_release(&h->game_assets)!=GH_GAME_OK)return GH_HOST_MEMORY;
    gh_audio_registry_destroy(h->audio);
    gh_memory_card_close(h->card);
    if(gh_heap_destroy(h->heap)!=GH_MEM_OK)return GH_HOST_MEMORY;
    gh_disc_close(h->disc);free(h);return GH_HOST_OK;
}
