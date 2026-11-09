/* Native integration of recovered0030D358, not an emulator or new game loop.
 * Actual control-flow evidence: startup-task-evidence.json and ghidra-startup.json.
 * Host policies/timing substitutions: startup-host-verification.json. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "startup_host.h"
#include "../core/dvd_jobs.h"
#include "../core/game_gate.h"
#include "../core/game_init.h"
#include "../core/game_stage.h"
#include "../core/game_children.h"
#include "../core/mc_control_flow.h"
#include "../core/mc_exit.h"
#include "../core/movie_task.h"
#include "game_services.h"
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
    gh_stage_case1 game_stage;
    gh_game_children *game_children;
    gh_mc_control_state mc_control;
    gh_mc_draw_consumer mc_draw;
    gh_id_post_message mc_post;
    void *mc_draw_context,*mc_post_context;
    gh_startup_continuation continuation;
    char *iso_path,*ffmpeg_path;
    gh_host_mc_frame_begin mc_begin;
    void *mc_begin_context;
    uint64_t mc_ui_epoch,mc_frame_serial;
    int mc_frame_ready,mc_frame_pending,mc_slot;
    gh_mc_control_flow mc_flow;
    gh_mc_exit_state mc_exit;
    gh_audio_output *audio_output;
    gh_se_voice_engine *voices;
    gh_sound_control sound_control;
    gh_movie_pool *movies;
    gh_movie_task movie_task;
    gh_video_status movie_video;
    gh_game_services *services;
};
static int register_task(void *,uint32_t,int32_t);
static void movie_entry(void *,int32_t);
static int on_owner(const gh_startup_host *h)
{return GetCurrentThreadId()==h->owner_thread && IsThreadAFiber() && GetCurrentFiber()==h->owner_fiber;}
static int failure(gh_startup_host *h,gh_host_error error,int detail)
{h->error=error;h->subsystem_error=detail;return (int)error;}
static int entry_frontier(gh_startup_host *h,uint32_t address,const char *module,int detail)
{h->unavailable_entry=address;h->frontier_module=module;return failure(h,GH_HOST_ENTRY_UNAVAILABLE,detail);}
static char *copy_string(const char *s)
{size_t n=strlen(s)+1;char *p=malloc(n);if(p)memcpy(p,s,n);return p;}
static int sound_startup(void *context)
{
    gh_startup_host *h=context;
    /* Current first boot skips the later conditional reload. Do not replace
     * live banks beneath voices until that lifecycle has a preserved-serial
     * native consumer; make a future actual reload an explicit frontier. */
    if(h->voices)return entry_frontier(h,0x2ca798,0,GH_VOICE_UNSUPPORTED);
    gh_audio_status e=gh_audio_startup_load(h->audio,h->disc);
    if(e)return failure(h,GH_HOST_AUDIO,(int)e);
    if(!h->sound_control.initialized){gh_voice_status se=gh_sound_control_from_startup(&h->sound_control,h->disc,h->audio);if(se)return failure(h,GH_HOST_AUDIO,se);}
    if(h->continuation.enabled){
        gh_se_mix mix;int32_t native=0;gh_voice_status se;
        se=gh_audio_output_create(&h->audio_output,&native);
        if(se)return failure(h,GH_HOST_AUDIO,native?native:(int)se);
        se=gh_audio_output_set_gain(h->audio_output,h->continuation.muted?0.0f:1.0f);
        if(!se)se=gh_se_mix_boot_defaults_002c9e78(&mix,h->work.flags,h->continuation.audio_dsp_policy);
        if(!se)se=gh_se_voice_create(h->audio,h->audio_output,&mix,&h->voices);
        if(se)return failure(h,GH_HOST_AUDIO,se);
    }
    return 0;
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
static int initialize_children(void *context,gh_stage_case1 *stage)
{
    gh_startup_host *h=context;gh_child_status e;(void)stage;
    if(!h->game_children) {
        e=gh_game_children_create(0,&h->game_children);
        if(e)return failure(h,GH_HOST_MEMORY,(int)e);
    }
    return (int)gh_game_children_initialize_0015fd98(h->game_children);
}
static int service_state(gh_startup_host *h,unsigned state)
{
    gh_game_services_status e;
    if(!h->services){
        loaded_bundle *language=language_bundle(h,&h->task);
        gh_game_services_bindings b;
        if(!language)return (int)h->error;
        memset(&b,0,sizeof(b));b.assets=&h->game_assets;b.loop=&h->game_loop;b.init=&h->game_init;
        b.stage=&h->game_stage;b.children=&h->game_children;
        b.save=(gh_core_save_view){h->save_bytes,sizeof(h->save_bytes)};b.rate=&h->game_rate;
        b.work=&h->work;b.threads=h->threads;b.id=&h->id;b.id_global=&h->id_global;b.runtime=&h->id_runtime;
        b.font=&h->font;b.menu=&h->menu;b.core_bin=h->bundles[0].allocation.pointer;
        b.core_bin_size=h->bundles[0].bytes;b.language=&language->view;
        b.sound_control=&h->sound_control;b.movie_pool=h->movies;
        e=gh_game_services_create(&b,&h->services);
        if(e)return failure(h,GH_HOST_ASSET,e);
    }
    e=state==1?gh_game_services_state1(h->services):state==2?
        gh_game_services_state2(h->services):gh_game_services_state3(h->services);
    if(e==GH_SERVICES_FRONTIER)return entry_frontier(h,gh_game_services_frontier(h->services),0,gh_game_services_detail(h->services));
    return e?failure(h,GH_HOST_ASSET,(int)e):0;
}
static int initialize_state2(void *context,gh_game_loop *loop)
{gh_startup_host *h=context;(void)loop;return service_state(h,2);}
static int initialize_state3(void *context,gh_game_loop *loop)
{gh_startup_host *h=context;(void)loop;return service_state(h,3);}
static int initialize_stage(void *context,gh_game_loop *loop)
{
    gh_startup_host *h=context;gh_stage_status e;gh_stage_case1 *s=&h->game_stage;
    if(h->continuation.enabled)return service_state(h,1);
    loaded_bundle *language=language_bundle(h,&h->task);
    if(!language)return (int)h->error;
    if(!s->textures) {
        e=gh_stage_textures_create(&s->textures);
        if(e)return failure(h,GH_HOST_MEMORY,(int)e);
    }
    s->assets=&h->game_assets;s->core_bin=h->bundles[0].allocation.pointer;
    s->core_bin_bytes=h->bundles[0].bytes;s->language=&language->view;
    s->font=&h->font;s->menu=&h->menu;s->id=&h->id;s->id_global=&h->id_global;
    s->game_state=&loop->game_state[0];s->save=(gh_core_save_view){h->save_bytes,sizeof(h->save_bytes)};
    s->context=h;s->init_children_15fd98=initialize_children;
    e=gh_game_stage_002a5bb8(s);
    if(e && e!=GH_STAGE_UNAVAILABLE && e!=GH_STAGE_UNSUPPORTED)
        (void)failure(h,GH_HOST_ASSET,(int)e);
    return (int)e;
}
static void game_main_entry(void *context,int32_t argument)
{
    gh_startup_host *h=context;gh_game_status e;(void)argument;
    e=gh_game_main_002a5998(&h->game_loop,&h->game_platform);
    if(e) {
        if(h->error && h->error!=GH_HOST_ENTRY_UNAVAILABLE) {h->phase=GH_HOST_FAILED;return;}
        if(e==GH_GAME_UNAVAILABLE) {
            if(!h->unavailable_entry)h->unavailable_entry=h->game_gate.failed_entry?
                h->game_gate.failed_entry:(h->game_loop.failed_entry==0x2a5bb8 && h->game_stage.failed_entry?
                    h->game_stage.failed_entry:h->game_loop.failed_entry);
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
static gh_id_error missing_mc_draw(void *context,const gh_id_draw_job *job)
{(void)context;(void)job;return GH_ID_UNSUPPORTED;}
static int sound_dispatch(void *context,const gh_mc_sound_request *request)
{
    gh_startup_host *h=context;uint32_t handle=0;gh_voice_status e;
    if(!h->voices)return GH_VOICE_UNSUPPORTED;
    e=gh_se_voice_request_origin(h->voices,(int16_t)request->bank,(int16_t)request->sound_id,
        request->parameter3,request->parameter4,request->parameter5,request->parameter6,&handle);
    return e?(int)e:handle?0:(int)GH_VOICE_CAPACITY;
}
static gh_id_error begin_mc_frame(gh_startup_host *h)
{
    gh_id_error e=h->mc_begin?h->mc_begin(h->mc_begin_context):GH_ID_OK;
    if(e)return e;
    h->mc_frame_ready=0;h->mc_frame_pending=1;return GH_ID_OK;
}
static gh_id_error mc_sleep_frame(gh_startup_host *h)
{
    gh_id_error e=begin_mc_frame(h);if(e)return e;
    e=gh_mc_control_sleep_frame(&h->mc_control,&h->mc,&h->mc_bindings,
        h->mc_post,h->mc_post_context,h->mc_draw?h->mc_draw:missing_mc_draw,h->mc_draw_context);
    if(e)h->mc_frame_pending=0;return e;
}
static void mc_frontier(gh_startup_host *h,uint32_t address,int detail)
{h->mc_frame_pending=0;(void)entry_frontier(h,address,address<0x100000?"mcport.rel":0,detail);}
static void mc_entry(void *context,int32_t argument)
{
    gh_startup_host *h=context;gh_mc_task_status e;(void)argument;
    e=gh_mc_task_prepare_002ae8a8(&h->mc,&h->mc_bindings);
    if(e==GH_MC_TASK_CONTROL_FRONTIER) {
        ++h->mc_ui_epoch;
        if(h->continuation.enabled){
            for(;;){
                gh_mc_boot_error control=gh_mc_control_flow_advance(&h->mc_flow,&h->mc_control,&h->mc,&h->mc_bindings,sound_dispatch,h);
                while(control==GH_MC_BOOT_NEEDS_SLEEP){
                    gh_id_error frame=begin_mc_frame(h);
                    if(frame){mc_frontier(h,0x2aac28,frame);return;}
                    control=gh_mc_control_flow_delay_frame(&h->mc_flow,&h->mc_control,&h->mc,&h->mc_bindings,
                        h->mc_post,h->mc_post_context,h->mc_draw?h->mc_draw:missing_mc_draw,h->mc_draw_context);
                    if(control!=GH_MC_BOOT_OK && control!=GH_MC_BOOT_NEEDS_SLEEP)h->mc_frame_pending=0;
                }
                if(control!=GH_MC_BOOT_OK){mc_frontier(h,h->mc_flow.first_unported?h->mc_flow.first_unported:0x2e8,control);return;}
                if(h->mc.checks.done){
                    gh_id_error frame=begin_mc_frame(h);
                    if(frame){mc_frontier(h,0x2aac28,frame);return;}
                    e=gh_mc_exit_run_final_frame(&h->mc_exit,&h->mc_control,&h->mc,&h->mc_bindings,
                        h->mc_post,h->mc_post_context,h->mc_draw?h->mc_draw:missing_mc_draw,h->mc_draw_context);
                    if(e!=GH_MC_TASK_OK){mc_frontier(h,h->mc_exit.frontier_address,(int)e);return;}
                    ++h->mc_ui_epoch;h->mc_frame_ready=0;
                    gh_thread_status tail=gh_task_threads_replace_current(h->threads,movie_entry,h,0);
                    (void)failure(h,GH_HOST_THREADS,tail);h->phase=GH_HOST_FAILED;return;
                }
                gh_id_error frame=mc_sleep_frame(h);
                if(frame){mc_frontier(h,h->mc_control.first_unported?h->mc_control.first_unported:0x2aac28,frame);return;}
            }
        }
        /* Execute the actual first Control and Sleep prefix. The next missing
         * draw is reported before claiming a completed frame or sleeping. */
        gh_mc_boot_error control=gh_mc_control_advance(&h->mc_control,&h->mc,&h->mc_bindings);
        if(control==GH_MC_BOOT_OK) {
            gh_id_error draw=mc_sleep_frame(h);
            if(draw!=GH_ID_OK) {
                h->unavailable_entry=h->mc_control.first_unported;
                h->frontier_module=h->unavailable_entry<0x100000?"mcport.rel":0;
                (void)failure(h,GH_HOST_ENTRY_UNAVAILABLE,(int)draw);return;
            }
            /* Further Main/control/exit integration is distinct from prefix
             * completion; never return whole-task success here. */
            h->unavailable_entry=0x2e8;h->frontier_module="mcport.rel";
            (void)failure(h,GH_HOST_ENTRY_UNAVAILABLE,GH_MC_TASK_CONTROL_FRONTIER);return;
        }
        h->unavailable_entry=h->mc_control.first_unported?h->mc_control.first_unported:0x2e8;
        h->frontier_module="mcport.rel";
        (void)failure(h,GH_HOST_ENTRY_UNAVAILABLE,(int)control);return;
    } else if(e==GH_MC_TASK_UNSUPPORTED) {
        h->unavailable_entry=h->mc.frontier_offset;
        h->frontier_module=h->mc.frontier_offset<0x100000?"mcport.rel":0;
        (void)failure(h,GH_HOST_ENTRY_UNAVAILABLE,(int)e);
    } else if(e!=GH_MC_TASK_OK) {
        (void)failure(h,GH_HOST_ASSET,(int)e);h->phase=GH_HOST_FAILED;
    }
}
static gh_movie_status movie_provider(void *context,gh_movie_pool *pool,unsigned slot,
    void *native,gh_movie_pool_operation operation,uint32_t *frontier)
{
    gh_startup_host *h=context;gh_video_player *player=native;
    if(operation==GH_MOVIE_STOP_002D42E0){
        gh_video_close(player);
        return gh_movie_pool_track_closed(pool,slot,native);
    }
    gh_video_error e=gh_video_poll(player,&h->movie_video);
    if(e){*frontier=0x2d3750;(void)failure(h,GH_HOST_ASSET,(int)e);return GH_MOVIE_NATIVE;}
    if(h->movie_video.complete){gh_video_close(player);return gh_movie_pool_track_closed(pool,slot,native);}
    return GH_MOVIE_OK;
}
static int movie_start(void *context,int32_t id,int32_t argument,int32_t *handle)
{
    gh_startup_host *h=context;gh_video_player *player;int slot;uint32_t token;
    if(id || argument!=1 || !handle)return failure(h,GH_HOST_ARGUMENT,0);
    if(!h->ffmpeg_path || !h->continuation.movie_present)
        return entry_frontier(h,0x2b4be8,0,GH_VIDEO_ARGUMENT);
    gh_movie_status e=gh_movie_pool_first_free_002b4f38(h->movies,&slot);
    if(e || slot<0)return failure(h,GH_HOST_ASSET,e?e:GH_MOVIE_BUSY);
    gh_video_error video=gh_video_open_movie0(h->iso_path,h->ffmpeg_path,
        h->continuation.movie_present,h->continuation.movie_present_context,&player);
    if(video)return failure(h,GH_HOST_ASSET,video);
    video=gh_video_set_muted(player,h->continuation.muted!=0);
    if(video){gh_video_close(player);return failure(h,GH_HOST_ASSET,video);}
    e=gh_movie_pool_commit_started(h->movies,(unsigned)slot,player,UINT32_C(0x40000023),&h->work,&token);
    if(e){gh_video_close(player);return failure(h,GH_HOST_ASSET,e);}
    memcpy(handle,&token,4);return 0;
}
static int movie_status(void *context,int32_t handle,int32_t *status)
{
    gh_startup_host *h=context;uint32_t token;memcpy(&token,&handle,4);
    if(!status)return failure(h,GH_HOST_ARGUMENT,0);*status=0;
    for(unsigned i=0;i<3;++i){gh_movie_track_view track;
        if(gh_movie_pool_track_at(h->movies,i,&track)!=GH_MOVIE_OK)return failure(h,GH_HOST_ASSET,0);
        /* This task tests only alive/nonzero. The live value comes from the
         * actual owned player, not a simulated frame counter or fake EOF. */
        if(track.cached_handle==token && track.native_player){*status=1;break;}
    }
    return 0;
}
static int movie_sleep(void *context,int32_t frames)
{gh_startup_host *h=context;gh_thread_status e=gh_task_threads_sleep(h->threads,frames);return e?failure(h,GH_HOST_THREADS,e):0;}
static int movie_stop(void *context,int32_t handle)
{gh_startup_host *h=context;uint32_t token;memcpy(&token,&handle,4);gh_movie_status e=gh_movie_pool_stop_002b4d68(h->movies,&h->work,token);return e?failure(h,GH_HOST_ASSET,e):0;}
static void movie_entry(void *context,int32_t argument)
{
    gh_startup_host *h=context;(void)argument;
    gh_movie_platform p={h,movie_start,movie_status,movie_sleep,movie_stop,register_task};
    int e=gh_movie_task_0030da08(&h->movie_task,&p);
    if(e && h->error!=GH_HOST_ENTRY_UNAVAILABLE)h->phase=GH_HOST_FAILED;
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
        if(entry==0x2ae8a8)h->mc_slot=chosen;
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
    h->mc_slot=-1;h->iso_path=copy_string(iso);if(!h->iso_path)goto failed;
    h->task.failed_operation=GH_START_OP_COUNT;
    if(gh_disc_open(iso,&h->disc)!=GH_DISC_OK) {e=GH_HOST_DISC;goto failed;}
    if(gh_heap_create(capacity,&h->heap)!=GH_MEM_OK)goto failed;
    if(gh_audio_registry_create(&h->audio)!=GH_AUDIO_OK)goto failed;
    if(gh_movie_pool_create_002b4960(&h->movies)!=GH_MOVIE_OK)goto failed;
    if(gh_movie_pool_set_provider(h->movies,movie_provider,h)!=GH_MOVIE_OK)goto failed;
    if(gh_memory_card_open(profile,&h->card)!=GH_MC_OK) {e=GH_HOST_ARGUMENT;goto failed;}
    h->manager.records=h->records;h->manager.capacity=4;h->manager.count=4;
    h->manager.current_index=-1;
    if(gh_task_threads_create(&h->manager,&h->threads)!=GH_THREAD_OK) {e=GH_HOST_THREADS;goto failed;}
    h->owner_thread=GetCurrentThreadId();h->owner_fiber=GetCurrentFiber();
    gh_dvd_jobs_init(&h->dvd,h->disc);
    h->work.language=language;h->work.flags598=flags598;h->task.work=&h->work;
    h->work.phase569=2; /* Actual cold0030B728 caller value. */
    /* Observed caller0030B728 inputs, not a claim its other setup ran. */
    h->work.room=0x20;
    {uint32_t one=0x3f800000;memcpy((unsigned char*)&h->work+0x574,&one,4);}
    gh_game_init_state_initialize(&h->game_init);
    {gh_core_save_view save={h->save_bytes,sizeof(h->save_bytes)};
     if(gh_core_save_system_init(save)!=GH_INIT_OK)goto failed;}
    h->game_loop.work=&h->work;h->game_loop.threads=h->threads;
    h->game_platform.context=h;h->game_platform.dvd_cover=game_cover;
    h->game_platform.modal_input=game_modal;h->game_platform.states[0]=game_initialize;
    h->game_platform.states[1]=initialize_stage;
    h->game_platform.states[2]=initialize_state2;h->game_platform.states[3]=initialize_state3;
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
    if(h->movies)(void)gh_movie_pool_destroy(h->movies);
    if(h->card)gh_memory_card_close(h->card);
    if(h->heap)(void)gh_heap_destroy(h->heap);
    if(h->disc)gh_disc_close(h->disc);
    free(h->iso_path);free(h);return e;
}
gh_host_error gh_startup_host_set_mc_consumers(gh_startup_host *h,gh_mc_draw_consumer draw,
    void *draw_context,gh_id_post_message post,void *post_context)
{
    if(!h || h->ticks || h->phase!=GH_HOST_RUNNING)return GH_HOST_ARGUMENT;
    if(!on_owner(h))return GH_HOST_THREADS;
    h->mc_draw=draw;h->mc_draw_context=draw_context;
    h->mc_post=post;h->mc_post_context=post_context;return GH_HOST_OK;
}
gh_host_error gh_startup_host_set_continuation(gh_startup_host *h,const gh_startup_continuation *c)
{
    char *path=0;
    if(!h || !c || h->ticks || h->phase!=GH_HOST_RUNNING)return GH_HOST_ARGUMENT;
    if(!on_owner(h))return GH_HOST_THREADS;
    if((c->enabled!=0 && c->enabled!=1) || (c->muted!=0 && c->muted!=1) ||
       (c->enabled && c->audio_dsp_policy!=GH_SE_DSP_NATIVE_LINEAR_REFERENCE))return GH_HOST_ARGUMENT;
    if(c->ffmpeg_path){if(!*c->ffmpeg_path)return GH_HOST_ARGUMENT;path=copy_string(c->ffmpeg_path);if(!path)return GH_HOST_MEMORY;}
    free(h->ffmpeg_path);h->ffmpeg_path=path;h->continuation=*c;
    h->continuation.ffmpeg_path=h->ffmpeg_path;return GH_HOST_OK;
}
gh_host_error gh_startup_host_set_mc_frame_begin(gh_startup_host *h,gh_host_mc_frame_begin begin,void *context)
{
    if(!h || h->ticks || h->phase!=GH_HOST_RUNNING)return GH_HOST_ARGUMENT;
    if(!on_owner(h))return GH_HOST_THREADS;
    h->mc_begin=begin;h->mc_begin_context=context;return GH_HOST_OK;
}
gh_host_error gh_startup_host_set_input(gh_startup_host *h,const gh_native_input_snapshot *input)
{
    unsigned char *raw;
    if(!h || !input || h->phase!=GH_HOST_RUNNING)return GH_HOST_ARGUMENT;
    if(!on_owner(h))return GH_HOST_THREADS;
    raw=(unsigned char*)&h->work;
    memcpy(raw,&input->raw_held,4);memcpy(raw+8,&input->raw_pressed,4);
    memcpy(raw+0xc,&input->raw_released,4);memcpy(raw+0x18,&input->raw_repeat_fast,4);
    memcpy(raw+0x1c,&input->raw_repeat_menu,4);
    memcpy(raw+0x190,&input->held,8);memcpy(raw+0x1a0,&input->pressed,8);
    memcpy(raw+0x1a8,&input->released,8);memcpy(raw+0x1b8,&input->repeat_fast,8);
    memcpy(raw+0x1b0,&input->repeat_menu,8);return GH_HOST_OK;
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
    /* Consumers are synchronous. A pending frame reaches owner only at its
     * real Sleep after all jobs succeeded, never at an intermediate glyph. */
    if(h->mc_frame_pending && h->mc_slot>=0 && h->records[h->mc_slot].running==1 && !h->error){
        ++h->mc_frame_serial;h->mc_frame_ready=1;h->mc_frame_pending=0;
    }
    e=gh_dvd_pump(&h->dvd);
    if(e) {failure(h,GH_HOST_DISC,(int)e);h->phase=GH_HOST_FAILED;}
    if(h->phase==GH_HOST_RUNNING && h->voices){
        gh_voice_status audio=gh_sound_control_flush_se_002ca5d8(&h->sound_control,h->voices);
        if(audio){failure(h,GH_HOST_AUDIO,audio);h->phase=GH_HOST_FAILED;}
    }
    if(h->phase==GH_HOST_RUNNING && h->continuation.enabled){
        gh_movie_status movie=gh_movie_pool_update_002b4a50(h->movies,&h->work);
        if(movie){if(!h->error)failure(h,GH_HOST_ASSET,movie);h->phase=GH_HOST_FAILED;}
    }
    if(h->phase==GH_HOST_RUNNING && h->startup_returned) {
        for(i=0;i<4;++i)if(h->records[i].running)active=1;
        if(!active)h->phase=h->error==GH_HOST_ENTRY_UNAVAILABLE?GH_HOST_FRONTIER:GH_HOST_COMPLETE;
    }
    return h->phase;
}
gh_host_error gh_startup_host_snapshot(const gh_startup_host *h,gh_startup_snapshot *out)
{
    if(!h || !out)return GH_HOST_ARGUMENT;
    memset(out,0,sizeof(*out));
    out->phase=h->phase;out->error=h->error;out->subsystem_error=h->subsystem_error;
    out->failed_operation=h->task.failed_operation;out->unavailable_entry=h->unavailable_entry;
    out->ticks=h->ticks;out->flags=h->work.flags;out->loaded_bundles=h->bundle_count;
    out->startup_returned=h->startup_returned;out->font=&h->font;out->menu=&h->menu;
    out->id=h->id_global;out->audio=h->audio;out->game_state=h->game_loop.game_state[0];
    out->game_assets_loaded=h->game_assets_loaded;out->game_rate=h->game_rate;
    out->frontier_module=h->frontier_module;
    out->mc_ui=h->mc.ui_initialized?&h->mc.id_base:0;
    out->mc_messages=h->mc.ui_initialized?&h->mc_messages:0;
    out->mc_ui_epoch=h->mc_ui_epoch;out->mc_frame_serial=h->mc_frame_serial;
    out->mc_ui_live=h->mc.ui_initialized!=0;out->mc_frame_ready=h->mc_frame_ready;
    out->mc_phase=h->mc.checks.phase;out->mc_subphase=h->mc.checks.subphase;out->mc_done=h->mc.checks.done;
    out->normal_continuation=h->continuation.enabled;out->work=&h->work;
    out->sound_control=h->sound_control.initialized?&h->sound_control:0;
    out->movie_pool=h->movies;out->movie_video=h->movie_video;
    if(h->game_stage.textures) {
        gh_stage_texture_cursor cursor;
        if(gh_stage_texture_cursor_get(h->game_stage.textures,&cursor)==GH_STAGE_OK)
            out->game_texture_banks=cursor.count;
    }
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
    for(;;) {
        for(i=0;i<GH_DVD_JOB_COUNT;++i)if(h->dvd.jobs[i].state)break;
        if(i==GH_DVD_JOB_COUNT)break;
        if(!gh_dvd_cancel(&h->dvd,h->dvd.jobs[i].id))return GH_HOST_DISC;
    }
    /* Scenario/Cockpit hold the canonical runtime/assets and nested backend.
     * Cancel their parent first, then release these owners before root backend. */
    if(h->services){
        if(gh_game_services_destroy(h->services)!=GH_SERVICES_OK)return GH_HOST_ASSET;
        h->services=0;
    }
    h->mc_bindings.id_runtime=h->id_runtime;
    if(h->mc.stage && h->mc.stage!=5 && gh_mc_task_abort(&h->mc,&h->mc_bindings)!=GH_MC_TASK_OK)
        return GH_HOST_ASSET;
    if(h->id_runtime){if(gh_id_runtime_destroy(h->id_runtime)!=GH_ID_OK)return GH_HOST_ASSET;h->id_runtime=0;}
    for(i=0;i<h->bundle_count;++i)
        (void)gh_heap_release(h->heap,h->bundles[i].allocation);
    gh_stage_skills_clear(&h->game_stage.skills);
    gh_stage_textures_destroy(h->game_stage.textures);
    h->game_stage.textures=0;
    gh_game_children_destroy(h->game_children);
    h->game_children=0;
    if(gh_game_assets_release(&h->game_assets)!=GH_GAME_OK)return GH_HOST_MEMORY;
    if(h->voices){gh_se_voice_destroy(h->voices);h->voices=0;}
    if(h->audio_output){gh_audio_output_destroy(h->audio_output);h->audio_output=0;}
    if(h->movies){
        for(unsigned slot=0;slot<3;++slot){gh_movie_track_view track;
            if(gh_movie_pool_track_at(h->movies,slot,&track)!=GH_MOVIE_OK)return GH_HOST_ASSET;
            if(track.native_player){gh_video_close(track.native_player);
                if(gh_movie_pool_track_closed(h->movies,slot,track.native_player)!=GH_MOVIE_OK)return GH_HOST_ASSET;}
        }
        if(gh_movie_pool_destroy(h->movies)!=GH_MOVIE_OK)return GH_HOST_ASSET;h->movies=0;
    }
    gh_audio_registry_destroy(h->audio);h->audio=0;
    gh_memory_card_close(h->card);
    if(gh_heap_destroy(h->heap)!=GH_MEM_OK)return GH_HOST_MEMORY;
    if(gh_task_threads_destroy(h->threads)!=GH_THREAD_OK)return GH_HOST_THREADS;
    gh_disc_close(h->disc);free(h->ffmpeg_path);free(h->iso_path);free(h);return GH_HOST_OK;
}
