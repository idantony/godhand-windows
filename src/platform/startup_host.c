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
#include "../core/frame_state.h"
#include "../core/sound_scene_tick.h"
#include "../core/game_event.h"
#include "../core/display_state.h"
#include "../core/game_scene_aux.h"
#include "../core/game_camera_projection.h"
#include "../core/game_camera_follow.h"
#include "../core/game_camera_envelope.h"
#include "../core/game_math_globals.h"
#include "../core/game_event_environment.h"
#include "../core/game_event_start.h"
#include "../core/game_event_clear.h"
#include "../core/game_hitarea_objects.h"
#include "../core/game_camera_shake.h"
#include "../core/game_object_step.h"
#include "../core/game_render_projection.h"
#include "../core/game_render_steps.h"
#include "../core/game_scene_at_step.h"
#include "../core/game_scene_post.h"
#include "../core/game_message_step.h"
#include "../core/sound_spatial_step.h"
#include "../core/game_scene_exit.h"
#include "../core/sound_scene_exit.h"
#include "../core/game_object_sound.h"
#include "../core/game_scene_exit_leaves.h"
#include "../core/game_world_light_step.h"
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
    gh_object_static_emit world_emit;void *world_context;
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
    gh_frame_random frame_random;
    gh_screen_fade fade;
    uint8_t em_set_first_byte;
    gh_host_fade_consumer fade_consumer;
    void *fade_context;
    uint64_t fade_draw_serial;
    gh_bgm_requests *bgm;
    gh_sound_scene sound_scene;
    gh_game_event event;
    gh_game_event_config event_config;
    gh_event_start_state event_start;
    gh_event_clear_state event_clear;
    gh_display_state display;
    gh_game_scene_aux scene_aux;
    gh_game_object_step object_step;
    gh_game_render_projection render_projection;
    gh_camera_follow camera_follow;gh_camera_envelope camera_envelope;
    gh_player_shared_vector player_shared_vector;
    gh_game_math_globals math_globals;
    gh_om_door_shared door_shared;
    gh_player_node_names player_node_names;gh_motion_ik_globals motion_ik_globals;
    uint8_t camera_update_stage;
    gh_render_draw_env draw_env;
    gh_r020_reference_profile title_reference;
    void *boot_reference;size_t boot_reference_bytes;int cockpit_frame_bound;
    gh_host_scene_ui_callbacks scene_ui;
    uint64_t scene_frame_serial;int scene_frame_pending,scene_frame_ready,scene_frame_open;
    gh_game_system_menu system_menu;
    gh_game_scene_exit scene_exit;
    gh_sound_scene_exit sound_exit;
    uint8_t factory_serial;int factory_serial_ready;
};
static int register_task(void *,uint32_t,int32_t);
static void movie_entry(void *,int32_t);
static uint32_t scene_profile_clock(void *context)
{(void)context;return GetTickCount();} /*native diagnostic timing, never world/game ticks*/
static gh_r020_status scene_program_se(void *context,unsigned bank,unsigned program,uint32_t *frontier)
{
    gh_startup_host *h=context;gh_game_services_scene_view scene;uint32_t handle=0;
    if(bank>INT16_MAX||program>INT16_MAX||!h->voices||gh_game_services_get_scene_view(h->services,&scene))return GH_R020_ARGUMENT;
    gh_voice_status e=gh_se_voice_bind_origin_seh(h->voices,scene.sound_scene->seh.bytes,scene.sound_scene->seh.size);
    if(!e)e=gh_se_voice_request_origin(h->voices,(int16_t)bank,(int16_t)program,-1,-1,0,0,&handle);
    if(e){if(frontier)*frontier=gh_se_voice_frontier(h->voices);return GH_R020_UNSUPPORTED;}
    return GH_R020_OK;
}
static gh_r020_status scene_logo_se(void *context,uint32_t *frontier)
{return scene_program_se(context,0,0x2d,frontier);}
static gh_game_door_bindings scene_door_bindings(gh_startup_host *h,const gh_game_services_scene_view *s)
{
    gh_game_door_bindings b={.work=&h->work,.loop=&h->game_loop,
        .scenario={s->scenario_bytes,s->scenario_bytes_size,s->scenario},.messages=s->scene_ui,
        .prepare=s->prepare,.player=s->title_player,.scene_step=s->step,.fade=&h->fade,.display=&h->display};
    return b;
}
static gh_r020_status scene_begin_room_jump(void *context,const gh_r020_room_jump_request *request,uint32_t *frontier)
{
    gh_startup_host *h=context;gh_game_services_scene_view s;
    if(!request||gh_game_services_get_scene_view(h->services,&s))return GH_R020_ARGUMENT;
    gh_game_door_bindings b=scene_door_bindings(h,&s);
    gh_door_status e=gh_game_door_request_001ffeb8(s.door,&b,request->room,request->position,request->yaw,
        request->kind,request->flags,request->transition);
    if(e&&frontier)*frontier=s.door->frontier;
    return e?(e==GH_DOOR_UNSUPPORTED?GH_R020_UNSUPPORTED:GH_R020_ARGUMENT):GH_R020_OK;
}
static gh_r020_status scene_title_draw(void *context,const gh_id_base *base,const gh_id_draw_job *job,uint32_t *frontier)
{
    gh_startup_host *h=context;
    if(!h->scene_ui.draw){if(frontier)*frontier=job->consumer_address;return GH_R020_UNSUPPORTED;}
    gh_id_error e=h->scene_ui.draw(h->scene_ui.context,base,job,frontier);
    return e?(e==GH_ID_UNSUPPORTED?GH_R020_UNSUPPORTED:GH_R020_ID):GH_R020_OK;
}
static gh_id_error scene_cockpit_trans(void *context,gh_id_base *base,uint32_t *frontier)
{
    gh_startup_host *h=context;gh_id_draw_job jobs[640];size_t count=0;gh_id_frontier f={0};
    gh_id_error e=gh_id_jobs_collect_visible(base,jobs,640,&count,&f);
    if(e){if(frontier)*frontier=f.address;return e;}
    for(size_t i=0;i<count;++i){
        if(!h->scene_ui.draw){if(frontier)*frontier=jobs[i].consumer_address;return GH_ID_UNSUPPORTED;}
        e=h->scene_ui.draw(h->scene_ui.context,base,&jobs[i],frontier);if(e)return e;
    }
    return GH_ID_OK;
}
static gh_id_error scene_cockpit_order(void *context,uint32_t bank,uint32_t depth,uint32_t *frontier)
{
    gh_startup_host *h=context;(void)frontier;
    if(!h->draw_env.ready)return GH_ID_ARGUMENT;
    h->draw_env.scalar.bank=bank;h->draw_env.scalar.depth=depth;return GH_ID_OK;
}
static gh_id_error scene_cockpit_fan(void *context,const gh_cockpit_fan *fan,uint32_t *frontier)
{
    gh_startup_host *h=context;
    if(!h->scene_ui.fan){if(frontier)*frontier=0x15c410;return GH_ID_UNSUPPORTED;}
    return h->scene_ui.fan(h->scene_ui.context,fan,frontier);
}
static gh_child_status scene_menu_cover(void *context,const gh_child_cover_quad *quad,int *emitted,uint32_t *frontier)
{
    gh_startup_host *h=context;*emitted=0;
    if(!h->scene_ui.cover){if(frontier)*frontier=0x15b880;return GH_CHILD_UNSUPPORTED;}
    gh_id_error e=h->scene_ui.cover(h->scene_ui.context,quad,frontier);
    if(e)return e==GH_ID_MEMORY?GH_CHILD_MEMORY:e==GH_ID_UNSUPPORTED?GH_CHILD_UNSUPPORTED:GH_CHILD_ARGUMENT;
    *emitted=1;return GH_CHILD_OK;
}
static gh_game_services_status scene_start_loading_menu(void *context,uint32_t *frontier)
{
    gh_startup_host *h=context;gh_game_services_scene_view s;
    if(gh_game_services_get_scene_view(h->services,&s))return GH_SERVICES_ARGUMENT;
    gh_system_menu_bindings b={.cockpit=s.cockpit,.runtime=h->id_runtime,.random=&h->game_init.random,
        .global_threads=h->threads,.enabled_3c2555=s.system_menu_enabled,
        .frame={.context=h,.trans=scene_cockpit_trans,.set_draw_order=scene_cockpit_order,
            .save={h->save_bytes,sizeof(h->save_bytes)},.fan=scene_cockpit_fan},
        .animation_profile={1,0},.binary32_reference=1};
    gh_system_menu_status e=gh_system_menu_start_002b65b0(&h->system_menu,&b);
    if(e&&frontier)*frontier=h->system_menu.frontier?h->system_menu.frontier:0x2b65b0;
    return !e?GH_SERVICES_OK:e==GH_SYSTEM_MENU_UNSUPPORTED?GH_SERVICES_FRONTIER:e==GH_SYSTEM_MENU_THREAD?GH_SERVICES_THREAD:GH_SERVICES_ASSET;
}
static gh_game_event_status event_object_view(void *context,uint32_t handle,gh_title_player_view*out,uint32_t*frontier)
{
    gh_startup_host*h=context;gh_game_services_status e=gh_game_services_actor_view(h->services,handle,out);
    if(e&&frontier)*frontier=0x14fd98;
    return !e?GH_EVENT_OK:e==GH_SERVICES_FRONTIER?GH_EVENT_UNSUPPORTED:GH_EVENT_CALLEE;
}
static gh_game_event_status event_loading_menu(void *context,uint32_t *frontier)
{
    gh_game_services_status e=scene_start_loading_menu(context,frontier);
    return !e?GH_EVENT_OK:e==GH_SERVICES_FRONTIER?GH_EVENT_UNSUPPORTED:GH_EVENT_CALLEE;
}
static int event_start_stage(void *context,gh_game_event*event,int*done)
{
    gh_startup_host*h=context;gh_game_services_scene_view s;
    if(event!=&h->event||gh_game_services_get_scene_view(h->services,&s))return GH_EVENT_ARGUMENT;
    gh_screen_fade_context fade={h->display.screen_width,h->display.gs_y_origin,0,0};
    memcpy(&fade.frame_shift,(uint8_t*)&h->work+0x564,4);memcpy(&fade.input_mask_5e0,(uint8_t*)&h->work+0x5e0,4);
    gh_event_start_bindings b={.event=event,.config=&h->event_config,.work=&h->work,.fade=&h->fade,.fade_context=&fade,
        .tables=s.scene_tables,.pools=s.pools,.meshes=s.meshes,.object_view=event_object_view,
        .system_menu_start=event_loading_menu,.context=h};
    gh_game_event_status e=gh_event_start_002965f0(&h->event_start,&b,done);
    if(e)event->frontier=h->event_start.frontier?h->event_start.frontier:0x2965f0;
    return (int)e;
}
static int event_clear_stage(void *context,gh_game_event*event,int*done)
{
    gh_startup_host*h=context;gh_game_services_scene_view s;
    if(event!=&h->event||gh_game_services_get_scene_view(h->services,&s))return GH_EVENT_ARGUMENT;
    gh_player_abort_bindings abort={.player=s.title_player,.sound=&h->sound_control};
    gh_event_clear_bindings b={.event=event,.config=&h->event_config,.init=&h->game_init,
        .pools=s.pools,.global_objects=s.objects,.player_abort=&abort,.object_view=event_object_view,.context=h};
    gh_game_event_status e=gh_event_clear_00296818(&h->event_clear,&b,done);
    if(e)event->frontier=h->event_clear.frontier?h->event_clear.frontier:0x296818;
    return (int)e;
}
static gh_scene_step_status scene_consumer(void *context,uint32_t address,uint32_t argument,uint32_t *frontier)
{
    gh_startup_host *h=context;gh_game_services_scene_view s;(void)argument;
    if(gh_game_services_get_scene_view(h->services,&s))return GH_SCENE_STEP_ARGUMENT;
    gh_scene_aux_status aux=GH_AUX_OK;
    if(address==0x2c1618){
        int original_return;gh_scene_at_status e=gh_scene_at_step_native_002c1618(s.scene_at,s.scene_tables,&s.title_player->view,&h->work,NULL,&original_return,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x30f090){
        if(!h->object_step.static_bound){
            gh_object_static_bindings b={.meshes=s.meshes,.scene_at=s.scene_at,.camera=s.pl_camera,
                .player=&s.title_player->view,.textures=h->game_stage.textures,.world_light=s.world_light,
                .projection=&h->render_projection,.emit=h->world_emit,.context=h->world_context};
            gh_object_step_status e=gh_game_object_step_bind_static(&h->object_step,&b,h->boot_reference,h->boot_reference_bytes);
            if(e){if(frontier)*frontier=address;return GH_SCENE_STEP_CALLEE;}
        }
        gh_object_step_status e=gh_game_move_objects_0030f090(&h->object_step,s.pools,&h->work,scene_profile_clock,h,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x30f650){
        gh_object_step_status e=gh_game_objects_trans_static_0030f650(&h->object_step,s.pools,&h->work,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x1601d8){
        gh_child_status e=gh_game_children_move_001601d8(h->game_children,&h->work,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x2c5470){
        gh_scene_post_status e=gh_scene_scroll_forward_002c5470(s.pools,&h->work,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x307b28){
        gh_scene_post_bindings b={&h->work,s.pools,s.scene_tables,&h->game_init,s.pl_camera,&s.title_player->view,GH_CAMERA_NATIVE_BINARY32_REFERENCE};
        gh_scene_post_status e=gh_scene_model_hide_00307b28(&b,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x2aefa8){
        gh_scene_ui_status e=gh_scene_message_update_002aefa8(s.scene_ui,&h->work,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x1fe748){
        gh_game_objects *objects=argument==0x580d20?s.objects:argument==0x5864e0?s.room_objects:0;
        if(!objects)return GH_SCENE_STEP_ARGUMENT;
        gh_object_status e=gh_game_objects_update_001fe748(objects,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x2d7560){
        gh_light_status e=gh_game_world_light_move_002d7560(s.world_light,&h->work,s.pools,GH_CAMERA_NATIVE_BINARY32_REFERENCE);
        if(e&&frontier)*frontier=s.world_light->frontier?s.world_light->frontier:address;
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x2ca518){
        gh_sound_scene_status e=gh_sound_spatial_step_002ca518(s.sound_scene,&h->work,&h->game_init,
            s.pl_camera,&s.title_player->view,GH_CAMERA_NATIVE_BINARY32_REFERENCE,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x140e58||address==0x1410a8){
        gh_cockpit_frame_callbacks callbacks={.context=h,.trans=scene_cockpit_trans,.set_draw_order=scene_cockpit_order,
            .save={h->save_bytes,sizeof(h->save_bytes)},.fan=scene_cockpit_fan};
        gh_id_error e=address==0x140e58?gh_game_cockpit_move_00140e58(s.cockpit,&h->work,&callbacks):gh_game_cockpit_trans_001410a8(s.cockpit,&h->work,&callbacks);
        if(e&&frontier)*frontier=gh_game_cockpit_frontier(s.cockpit);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x1538f0){
        gh_camera_projection_display display={&h->display.screen_width,&h->display.screen_height_bits,
            &h->display.pixel_aspect_bits,&h->display.initialized,GH_CAMERA_NATIVE_BINARY32_REFERENCE};
        gh_camera_status e=gh_game_render_projection_001538f0(&h->render_projection,s.pl_camera,&display,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x2c54e8){
        gh_render_step_status e=gh_render_draw_init_002c54e8(&h->draw_env);
        if(e&&frontier)*frontier=address;
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x2c3818)return GH_SCENE_STEP_OK; /*actualJRRA/NOP*/
    else if(address==0x2af028){
        gh_render_step_status e=gh_render_messages_002af028(s.scene_ui,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x2fb310){
        uint32_t slot;gh_render_step_status e=gh_render_particles_002fb310(&h->work,s.effects,s.scene_buffer_a,s.scene_buffer_a_size,frontier,&slot);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x160b90){
        gh_child_status e=gh_game_children_draw_00160b90(h->game_children,&h->work,&h->draw_env,scene_menu_cover,h,frontier);
        return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else if(address==0x2bf2d0)aux=gh_scene_cursor_step_002bf2d0(&h->scene_aux,&h->work,s.prepare->fields);
    else if(address==0x30f228)aux=gh_scene_effects_step_0030f228(&h->scene_aux,&h->work,scene_profile_clock,h);
    else if(address==0x1ffe10){
        gh_game_door_bindings b=scene_door_bindings(h,&s);gh_door_status e=gh_game_door_step_001ffe10(s.door,&b);
        if(e&&frontier)*frontier=s.door->frontier;return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }
    else if(address==0x138fc8){
        gh_camera_projection_display display={&h->display.screen_width,&h->display.screen_height_bits,
            &h->display.pixel_aspect_bits,&h->display.initialized,GH_CAMERA_NATIVE_BINARY32_REFERENCE};
        gh_camera_status e=GH_CAMERA_OK;uint32_t pc=0;
        if(!h->camera_update_stage){
            e=gh_game_camera_step_follow_00138fc8(s.pl_camera,&h->game_init,s.acd,&s.title_player->view,
                &h->work,&h->camera_follow,s.scene_tables,NULL,NULL,&pc);
            if(e==GH_CAMERA_UNSUPPORTED&&pc==0x13a3c0)h->camera_update_stage=1;
            else if(e==GH_CAMERA_UNSUPPORTED&&pc==0x138558)h->camera_update_stage=2;
            else if(e==GH_CAMERA_UNSUPPORTED&&pc==0x138740)h->camera_update_stage=3;
            else {if(frontier)*frontier=pc;return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;}
            e=GH_CAMERA_OK; /* the exact continuation below now supplies it */
        }
        if(h->camera_update_stage==1){e=gh_game_camera_finish_follow_00139c88(&h->camera_envelope,&h->camera_follow,&pc);if(!e)h->camera_update_stage=2;}
        if(!e&&h->camera_update_stage==2){e=gh_game_camera_shake_step_00138558(s.pl_camera,&pc);if(!e)h->camera_update_stage=3;}
        if(!e&&h->camera_update_stage==3){e=gh_game_camera_project_00138740(s.pl_camera,&display,&pc);if(!e)h->camera_update_stage=0;}
        if(frontier)*frontier=pc;return e?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
    }else {if(frontier)*frontier=address;return GH_SCENE_STEP_UNSUPPORTED;}
    if(aux&&frontier)*frontier=h->scene_aux.frontier?h->scene_aux.frontier:address;
    return aux?GH_SCENE_STEP_CALLEE:GH_SCENE_STEP_OK;
}
static int on_owner(const gh_startup_host *h)
{return GetCurrentThreadId()==h->owner_thread && IsThreadAFiber() && GetCurrentFiber()==h->owner_fiber;}
static int failure(gh_startup_host *h,gh_host_error error,int detail)
{h->error=error;h->subsystem_error=detail;return (int)error;}
static int entry_frontier(gh_startup_host *h,uint32_t address,const char *module,int detail)
{h->unavailable_entry=address;h->frontier_module=module;return failure(h,GH_HOST_ENTRY_UNAVAILABLE,detail);}
static char *copy_string(const char *s)
{size_t n=strlen(s)+1;char *p=malloc(n);if(p)memcpy(p,s,n);return p;}
static int initialize_native_bgm(gh_startup_host *h)
{
    gh_adx_key key;gh_adx_error decoded;gh_bgm_status status;
    uint64_t bytes=gh_disc_boot_elf_size(h->disc);void *elf;
    if(h->bgm)return failure(h,GH_HOST_AUDIO,GH_BGM_BUSY);
    if(!bytes||bytes>SIZE_MAX)return failure(h,GH_HOST_DISC,GH_ADX_BOUNDS);
    elf=malloc((size_t)bytes);if(!elf)return failure(h,GH_HOST_MEMORY,GH_ADX_MEMORY);
    if(gh_disc_read_boot_elf(h->disc,0,elf,(size_t)bytes)!=GH_DISC_OK){free(elf);return failure(h,GH_HOST_DISC,GH_ADX_BOUNDS);}
    h->boot_reference=elf;h->boot_reference_bytes=(size_t)bytes;
    decoded=gh_adx_key_from_elf(elf,(size_t)bytes,&key);
    gh_game_event_status event_status=gh_game_event_from_elf(&h->event,elf,(size_t)bytes);
    if(!event_status)event_status=gh_game_event_config_from_elf(&h->event_config,elf,(size_t)bytes);
    if(!event_status)event_status=gh_game_event_set_stage(&h->event,0,event_start_stage,h);
    if(!event_status)event_status=gh_game_event_set_stage(&h->event,1,event_clear_stage,h);
    gh_display_status display_status=gh_display_state_from_elf(&h->display,elf,(size_t)bytes);
    if(!display_status)display_status=gh_display_static_initialize_002a48a8(&h->display,1,0xffff);
    gh_render_step_status env_status=gh_render_draw_env_from_elf(&h->draw_env,elf,(size_t)bytes);
    gh_scene_mesh_status factory_status=gh_scene_mesh_factory_serial_from_elf(elf,(size_t)bytes,&h->factory_serial);
    uint32_t camera_frontier=0;gh_camera_status camera_status=gh_game_camera_envelope_from_elf(&h->camera_envelope,elf,(size_t)bytes,&camera_frontier);
    gh_title_player_status player_vector_status=gh_player_shared_vector_from_elf(&h->player_shared_vector,elf,(size_t)bytes);
    gh_scene_placement_status door_shared_status=gh_om_door_shared_from_elf(&h->door_shared,elf,(size_t)bytes);
    gh_player_nodes_status node_names_status=gh_player_node_names_from_elf(&h->player_node_names,elf,(size_t)bytes);
    gh_actor_motion_status motion_globals_status=gh_motion_ik_globals_from_elf(&h->motion_ik_globals,elf,(size_t)bytes);
    gh_math_globals_status math_status=gh_game_math_globals_from_elf(&h->math_globals,elf,(size_t)bytes);
    if(!math_status)math_status=gh_game_math_static_initialize_003079c8(&h->math_globals,&h->work,1,0xffff);
    if(decoded)return failure(h,GH_HOST_AUDIO,decoded);
    if(event_status)return failure(h,GH_HOST_ASSET,event_status);
    if(display_status)return failure(h,GH_HOST_ASSET,display_status);
    if(env_status)return failure(h,GH_HOST_ASSET,env_status);
    if(factory_status)return failure(h,GH_HOST_ASSET,factory_status);h->factory_serial_ready=1;
    if(camera_status)return entry_frontier(h,camera_frontier,0,camera_status);
    if(player_vector_status)return failure(h,GH_HOST_ASSET,player_vector_status);
    if(door_shared_status)return failure(h,GH_HOST_ASSET,door_shared_status);
    if(node_names_status)return failure(h,GH_HOST_ASSET,node_names_status);
    if(motion_globals_status)return failure(h,GH_HOST_ASSET,motion_globals_status);
    if(math_status)return failure(h,GH_HOST_ASSET,math_status);
    status=gh_bgm_requests_create(h->audio,&h->sound_control,h->disc,h->heap,&h->bgm);
    if(!status)status=gh_bgm_requests_enable_native(h->bgm,&key,h->continuation.muted,GH_BGM_GAIN_NATIVE_DB_REFERENCE);
    return status?failure(h,GH_HOST_AUDIO,status):0;
}
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
    if(!h->sound_scene.startup_bound){
        gh_sound_scene_status scene=gh_sound_scene_from_startup(&h->sound_scene,&h->sound_control,h->audio);
        if(scene)return failure(h,GH_HOST_AUDIO,scene);
    }
    if(h->continuation.enabled){
        gh_se_mix mix;int32_t native=0;gh_voice_status se;
        se=gh_audio_output_create(&h->audio_output,&native);
        if(se)return failure(h,GH_HOST_AUDIO,native?native:(int)se);
        se=gh_audio_output_set_gain(h->audio_output,h->continuation.muted?0.0f:1.0f);
        if(!se)se=gh_se_mix_boot_defaults_002c9e78(&mix,h->work.flags,h->continuation.audio_dsp_policy);
        if(!se)se=gh_se_voice_create(h->audio,h->audio_output,&mix,&h->voices);
        if(se)return failure(h,GH_HOST_AUDIO,se);
        if(initialize_native_bgm(h))return (int)h->error;
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
        b.fade=&h->fade;b.audio_registry=h->audio;b.em_set_first_byte=&h->em_set_first_byte;
        b.bgm_requests=h->bgm;
        b.sound_scene=&h->sound_scene;
        b.event=&h->event;
        b.display=&h->display;
        b.title_callbacks=(gh_r020_move_callbacks){.context=h,.draw_entry=scene_title_draw,.logo_se=scene_logo_se,
            .program_se=scene_program_se,.begin_room_jump=scene_begin_room_jump};
        b.title_reference_profile=&h->title_reference;
        b.factory_serial=h->factory_serial_ready?&h->factory_serial:0;
        b.boot_elf=h->boot_reference;b.boot_elf_bytes=h->boot_reference_bytes;
        b.start_loading_menu=scene_start_loading_menu;b.loading_context=h;
        b.camera_follow=&h->camera_follow;
        b.camera_envelope=&h->camera_envelope;
        b.player_shared_vector=&h->player_shared_vector;
        b.math_globals=&h->math_globals;
        b.door_shared=&h->door_shared;
        b.player_node_names=&h->player_node_names;b.motion_ik_globals=&h->motion_ik_globals;
        b.event_config=&h->event_config;b.packed=&h->mc_messages;
        e=gh_game_services_create(&b,&h->services);
        if(e)return failure(h,GH_HOST_ASSET,e);
    }
    e=state==1?gh_game_services_state1(h->services):state==2?
        gh_game_services_state2(h->services):gh_game_services_state3(h->services);
    if(e==GH_SERVICES_FRONTIER)return entry_frontier(h,gh_game_services_frontier(h->services),gh_game_services_frontier_module(h->services),gh_game_services_detail(h->services));
    if(!e&&state==1&&!h->cockpit_frame_bound){
        gh_game_services_scene_view s;
        if(gh_game_services_get_scene_view(h->services,&s)!=GH_SERVICES_OK)return failure(h,GH_HOST_ASSET,GH_SERVICES_ARGUMENT);
        gh_id_error id=gh_game_cockpit_bind_frame_elf(s.cockpit,h->boot_reference,h->boot_reference_bytes);
        if(id)return failure(h,GH_HOST_ASSET,id);h->cockpit_frame_bound=1;
    }
    if(e){
        /* Preserve the actual failed callee for asset/ownership errors too;
         * only an unrecovered implementation is a FRONTIER phase. */
        h->unavailable_entry=gh_game_services_frontier(h->services);
        h->frontier_module=gh_game_services_frontier_module(h->services);
        return failure(h,GH_HOST_ASSET,gh_game_services_detail(h->services));
    }
    return 0;
}
static int initialize_state2(void *context,gh_game_loop *loop)
{gh_startup_host *h=context;(void)loop;return service_state(h,2);}
static int initialize_state3(void *context,gh_game_loop *loop)
{gh_startup_host *h=context;(void)loop;return service_state(h,3);}
static gh_scene_step_status scene_scenario_status(void *context,uint32_t *frontier)
{gh_startup_host*h=context;return gh_game_services_task_status(h->services,frontier);}
static int update_state4(void *context,gh_game_loop *loop)
{
    gh_startup_host *h=context;gh_game_services_scene_view s;(void)loop;
    if(!h->services||gh_game_services_get_scene_view(h->services,&s))return entry_frontier(h,0x2a6690,0,GH_SCENE_STEP_ARGUMENT);
    gh_game_scene_step_bindings b={.work=&h->work,.init=&h->game_init,.prepare=s.prepare,
        .save={h->save_bytes,sizeof(h->save_bytes)},.collision=s.collision_pairs,.actors=s.scene_tables,
        .scenario={s.scenario_bytes,s.scenario_bytes_size,s.scenario},.main_threads=h->threads,
        .scenario_threads=s.scenario_threads,.room_move=&s.module->hook12,.title_player=s.title_player,
        .fade=&h->fade,.display=&h->display,.event=&h->event,.scene_at=s.scene_at,.action_buttons=s.action_buttons,
        .cockpit_action_flags_1414=gh_game_cockpit_action_flags_1414(s.cockpit),.binary32_reference=1,
        .scenario_callback104=s.scenario_callback104,.external=scene_consumer,.external_context=h,
        .profile_counter=scene_profile_clock,.profile_context=h,.scenario_status=scene_scenario_status,.scenario_status_context=h};
    gh_scene_step_status e=gh_game_scene_step_002a6690(s.step,&b);
    if(e)return entry_frontier(h,s.step->frontier?s.step->frontier:0x2a6690,
        gh_game_services_frontier_module(h->services),s.step->detail?s.step->detail:(int)e);
    h->scene_frame_pending=1;
    return 0;
}
static gh_scene_exit_status scene_exit_consumer(void *context,uint32_t address,uint32_t argument,int *original_return,uint32_t *frontier)
{
    gh_startup_host *h=context;(void)argument;gh_sound_exit_status e;
    if(address==0x1fdfa8||address==0x2a71f0||address==0x2a7280||address==0x2d73e0){gh_game_services_scene_view s;uint32_t f=0;int detail;
        if(gh_game_services_get_scene_view(h->services,&s))return GH_SCENE_EXIT_ARGUMENT;
        if(address==0x1fdfa8)detail=gh_game_object_pool_release_001fdfa8(&h->game_init,&f);
        else if(address==0x2a71f0)detail=gh_collision_pairs_clear_002a71f0(s.collision_pairs,&f);
        else if(address==0x2a7280){int ignored=0;detail=gh_collision_pairs_free_002a7280(s.collision_pairs,&ignored);}
        else {detail=gh_world_light_exit_002d73e0(s.world_light);f=s.world_light->frontier;}
        if(detail&&frontier)*frontier=f?f:address;
        return detail?GH_SCENE_EXIT_CALLEE:GH_SCENE_EXIT_OK;
    }
    if(address==0x30f5f0||address==0x150078||address==0x1492c8||address==0x2a9528){gh_game_services_scene_view s;
        if(gh_game_services_get_scene_view(h->services,&s))return GH_SCENE_EXIT_ARGUMENT;
        gh_room_pool_status pe=address==0x30f5f0?gh_room_registries_release_0030f5f0(s.pools):
            address==0x150078?gh_room_model_pool_release_00150078(s.pools):
            address==0x1492c8?gh_room_aux_pool_release_001492c8(s.pools):gh_room_arena_clear_002a9528(s.pools,argument);
        if(pe&&frontier)*frontier=s.pools->failed_entry?s.pools->failed_entry:address;
        return pe?GH_SCENE_EXIT_CALLEE:GH_SCENE_EXIT_OK;
    }
    if(address==0x1fef30||address==0x1ff028){gh_game_services_scene_view s;uint32_t f=0;
        if(gh_game_services_get_scene_view(h->services,&s))return GH_SCENE_EXIT_ARGUMENT;
        gh_game_objects *objects=argument==0x580d20?s.objects:argument==0x5864e0?s.room_objects:0;
        if(!objects)return GH_SCENE_EXIT_ARGUMENT;
        gh_object_sound_binding sound={&h->sound_control,h->voices};
        gh_object_status oe=address==0x1fef30?gh_game_objects_release_nonpinned_001fef30(objects,gh_sound_object_slot_release_002cb340,&sound,&f):
            gh_game_objects_reset_records_001ff028(objects,&f);
        if(oe&&frontier)*frontier=f?f:address;
        return oe?GH_SCENE_EXIT_CALLEE:GH_SCENE_EXIT_OK;
    }
    if(address==0x12e800||address==0x138d40||address==0x141628||address==0x1612a8||address==0x2ad108||address==0x201018){
        gh_game_services_scene_view s;uint32_t f=0;int detail=0;
        if(gh_game_services_get_scene_view(h->services,&s))return GH_SCENE_EXIT_ARGUMENT;
        if(address==0x12e800){detail=gh_scene_actors_reset_0012e800(s.scene_tables);f=s.scene_tables->frontier;}
        else if(address==0x138d40)detail=gh_game_camera_restore_00138d40(&h->game_init);
        else if(address==0x141628){detail=gh_game_cockpit_room_quit_00141628(s.cockpit,
            (gh_core_save_view){h->save_bytes,sizeof(h->save_bytes)},h->game_init.heat);f=gh_game_cockpit_frontier(s.cockpit);}
        else if(address==0x1612a8)detail=gh_game_children_quit_001612a8(h->game_children,&h->work,&f);
        else if(address==0x2ad108){int ignored=0;detail=gh_id_pool_compact_002ad108(h->id_runtime,&ignored);}
        else detail=gh_dvd_jobs_reset_00201018(&h->dvd);
        if(detail&&frontier)*frontier=f?f:address;
        return detail?GH_SCENE_EXIT_CALLEE:GH_SCENE_EXIT_OK;
    }
    if(address==0x2caa00){
        e=gh_sound_scene_exit_bind(&h->sound_exit,&h->sound_scene,h->bgm,h->voices);
        if(!e)e=gh_sound_scene_exit_begin_002caa00(&h->sound_exit);
    }else if(address==0x2cab58)e=gh_sound_scene_exit_wait_002cab58(&h->sound_exit,original_return);
    else if(address==0x2ca910)e=gh_sound_scene_exit_finalize_002ca910(&h->sound_exit);
    else {if(frontier)*frontier=address;return GH_SCENE_EXIT_UNSUPPORTED;}
    if(e&&frontier)*frontier=h->sound_exit.frontier?h->sound_exit.frontier:address;
    return e?(e==GH_SOUND_EXIT_UNSUPPORTED?GH_SCENE_EXIT_UNSUPPORTED:GH_SCENE_EXIT_CALLEE):GH_SCENE_EXIT_OK;
}
static int update_state5(void *context,gh_game_loop *loop)
{
    gh_startup_host *h=context;gh_game_services_scene_view s;
    if(!h->services||gh_game_services_get_scene_view(h->services,&s))return entry_frontier(h,0x2a67d0,0,GH_SCENE_EXIT_ARGUMENT);
    gh_game_scene_exit_bindings b={.loop=loop,.save={h->save_bytes,sizeof(h->save_bytes)},.fade=&h->fade,.movies=h->movies,
        .system_menu=&h->system_menu,.system_menu_bindings={.cockpit=s.cockpit,.runtime=h->id_runtime,
            .random=&h->game_init.random,.global_threads=h->threads,.enabled_3c2555=s.system_menu_enabled,
            .frame={.context=h,.trans=scene_cockpit_trans,.set_draw_order=scene_cockpit_order,
                .save={h->save_bytes,sizeof(h->save_bytes)},.fan=scene_cockpit_fan},
            .animation_profile={1,0},.binary32_reference=1},
        .scenario={s.scenario_bytes,s.scenario_bytes_size,s.scenario},.scenario_threads=s.scenario_threads,
        .scenario_exit_ec=s.scenario_exit,.event=&h->event,.module=s.module,.pools=s.pools,
        .objects=s.objects,.room_objects=s.room_objects,.scene_at=s.scene_at,
        .scene_buffer_a=s.scene_buffer_a,.scene_buffer_a_size=s.scene_buffer_a_size,
        .scene_buffer_b=s.scene_buffer_b,.scene_buffer_b_size=s.scene_buffer_b_size,
        .external=scene_exit_consumer,.context=h};
    gh_scene_exit_status e=gh_game_scene_exit_002a67d0(&h->scene_exit,&b);
    if(e)return entry_frontier(h,h->scene_exit.frontier?h->scene_exit.frontier:0x2a67d0,0,h->scene_exit.detail?h->scene_exit.detail:(int)e);
    if(loop->game_state[0]==2||loop->game_state[0]==3){gh_game_services_status next=gh_game_services_begin_next_room(h->services);
        if(next)return failure(h,GH_HOST_ASSET,next);}
    return 0;
}
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
    if(gh_system_menu_from_disc(&h->system_menu,h->disc)!=GH_SYSTEM_MENU_OK){e=GH_HOST_ASSET;goto failed;}
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
    {uint32_t frontier=0;if(gh_hitarea_objects_construct_001fde00(&h->game_init,&frontier)){
        e=GH_HOST_ASSET;goto failed;}}
    h->title_reference=(gh_r020_reference_profile){GH_R020_MEMORY_ZERO_REFERENCE,0,0,1};
    if(gh_game_object_step_construct_002c53a8(&h->object_step,&h->game_init)!=GH_OBJSTEP_OK)goto failed;
    {gh_core_save_view save={h->save_bytes,sizeof(h->save_bytes)};
     if(gh_core_save_system_init(save)!=GH_INIT_OK)goto failed;}
    h->game_loop.work=&h->work;h->game_loop.threads=h->threads;
    h->game_platform.context=h;h->game_platform.dvd_cover=game_cover;
    h->game_platform.modal_input=game_modal;h->game_platform.states[0]=game_initialize;
    h->game_platform.states[1]=initialize_stage;
    h->game_platform.states[2]=initialize_state2;h->game_platform.states[3]=initialize_state3;
    h->game_platform.states[4]=update_state4;
    h->game_platform.states[5]=update_state5;
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
      free(h->boot_reference);free(h->iso_path);free(h);return e;
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
    h->continuation.ffmpeg_path=h->ffmpeg_path;
    if(c->enabled && gh_frame_seed_0031ceb8(&h->frame_random,&h->game_init.random,GH_FRAME_CHOP_REFERENCE)!=GH_FRAME_OK)
        return GH_HOST_ASSET;
    return GH_HOST_OK;
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
gh_host_error gh_startup_host_set_fade_consumer(gh_startup_host *h,gh_host_fade_consumer consumer,void *context)
{
    if(!h || h->ticks || h->phase!=GH_HOST_RUNNING)return GH_HOST_ARGUMENT;
    if(!on_owner(h))return GH_HOST_THREADS;
    h->fade_consumer=consumer;h->fade_context=context;return GH_HOST_OK;
}
gh_host_error gh_startup_host_set_scene_ui_consumers(gh_startup_host *h,const gh_host_scene_ui_callbacks *callbacks)
{
    if(!h||!callbacks||h->ticks||h->phase!=GH_HOST_RUNNING)return GH_HOST_ARGUMENT;
    if(!on_owner(h))return GH_HOST_THREADS;h->scene_ui=*callbacks;return GH_HOST_OK;
}
gh_host_error gh_startup_host_set_world_consumer(gh_startup_host *h,gh_object_static_emit emit,void *context)
{
    if(!h||h->ticks||h->phase!=GH_HOST_RUNNING)return GH_HOST_ARGUMENT;
    if(!on_owner(h))return GH_HOST_THREADS;
    h->world_emit=emit;h->world_context=context;return GH_HOST_OK;
}
gh_host_phase gh_startup_host_tick(gh_startup_host *h)
{
    gh_task_run_result r;gh_dvd_error e;unsigned i;int active=0;
    if(!h)return GH_HOST_FAILED;
    if(!on_owner(h))return GH_HOST_FAILED;
    if(h->phase!=GH_HOST_RUNNING)return h->phase;
    if(h->continuation.enabled){
        /* Actual30AC40 relative order. Other outer-loop GS/OT/boot operations
         * are still outside this developer host, not successful empty calls. */
        if(gh_frame_rate_policy(&h->work)!=GH_FRAME_OK){
            failure(h,GH_HOST_ASSET,GH_FRAME_ARGUMENT);h->phase=GH_HOST_FAILED;return h->phase;
        }
        gh_movie_status movie=gh_movie_pool_update_002b4a50(h->movies,&h->work);
        if(movie){if(!h->error)failure(h,GH_HOST_ASSET,movie);h->phase=GH_HOST_FAILED;return h->phase;}
        if(gh_frame_counter_advance(&h->work)!=GH_FRAME_OK ||
           gh_frame_random_before_tasks_0030b008(&h->frame_random,&h->game_init.random,&h->work)!=GH_FRAME_OK){
            failure(h,GH_HOST_ASSET,GH_FRAME_RANDOM);h->phase=GH_HOST_FAILED;return h->phase;
        }
    }
    /* Outer-frame packet lifetime precedes EVERY GlobalTask visit. A loading
     * SystemMenu fiber may run before Main, including during state5 or I/O. */
    h->scene_frame_open=0;
    if(h->continuation.enabled&&h->services&&(h->game_loop.game_state[0]>=4||h->system_menu.task_live)){
        h->scene_frame_ready=0;
        if(h->scene_ui.begin){uint32_t frontier=0;gh_id_error id=h->scene_ui.begin(h->scene_ui.context,&frontier);
            if(id){entry_frontier(h,frontier?frontier:0x31a338,0,id);h->phase=GH_HOST_FRONTIER;return h->phase;}}
        h->scene_frame_open=1;
    }
    r=gh_tasks_run_matching_002d52a0(&h->manager);
    if(r.status) {failure(h,GH_HOST_THREADS,r.platform_error);h->phase=GH_HOST_FAILED;return h->phase;}
    ++h->ticks;
    if(h->system_menu.error&&!h->error)entry_frontier(h,h->system_menu.frontier?h->system_menu.frontier:0x2b6600,0,h->system_menu.error);
    if(h->error)h->phase=h->error==GH_HOST_ENTRY_UNAVAILABLE?GH_HOST_FRONTIER:GH_HOST_FAILED;
    if(h->scene_frame_open&&h->game_loop.game_state[0]!=4&&!h->error)h->scene_frame_pending=1;
    /* Consumers are synchronous. A pending frame reaches owner only at its
     * real Sleep after all jobs succeeded, never at an intermediate glyph. */
    if(h->mc_frame_pending && h->mc_slot>=0 && h->records[h->mc_slot].running==1 && !h->error){
        ++h->mc_frame_serial;h->mc_frame_ready=1;h->mc_frame_pending=0;
    }
    if(h->scene_frame_pending&&!h->error){++h->scene_frame_serial;h->scene_frame_ready=1;h->scene_frame_pending=0;}
    if(h->continuation.enabled && !h->error){
        gh_screen_fade_context context={h->display.screen_width,h->display.gs_y_origin,0,0};gh_fade_draw draw;
        memcpy(&context.frame_shift,(unsigned char*)&h->work+0x564,4);
        memcpy(&context.input_mask_5e0,(unsigned char*)&h->work+0x5e0,4);
        gh_fade_error fade=gh_screen_fade_step_00307388(&h->fade,&context,&draw);
        if(fade || (draw.visible && (!h->fade_consumer || h->fade_consumer(h->fade_context,&draw)!=GH_ID_OK))){
            (void)entry_frontier(h,0x307388,0,fade?fade:GH_ID_UNSUPPORTED);
            h->phase=GH_HOST_FRONTIER;return h->phase;
        }
        if(draw.visible)++h->fade_draw_serial;
    }
    e=gh_dvd_pump(&h->dvd);
    if(e) {failure(h,GH_HOST_DISC,(int)e);h->phase=GH_HOST_FAILED;}
    if(h->phase==GH_HOST_RUNNING && h->continuation.enabled && h->sound_scene.startup_bound){
        gh_sound_scene_status scene=gh_sound_scene_tick_002caf28(&h->sound_scene,&h->work,h->game_children);
        if(scene){
            if(scene==GH_SOUND_SCENE_UNSUPPORTED){
                entry_frontier(h,h->sound_scene.frontier?h->sound_scene.frontier:0x2caf28,0,scene);h->phase=GH_HOST_FRONTIER;
            }else{failure(h,GH_HOST_AUDIO,scene);h->phase=GH_HOST_FAILED;}
        }
    }
    if(h->phase==GH_HOST_RUNNING && h->bgm){
        /* Actual second callee of2CA470, before its later SE flush. Other
         * sound sub-updates are not silently implemented by this subset. */
        gh_bgm_status bgm=gh_bgm_requests_update_002d0938(h->bgm,&h->work);
        if(bgm){
            if(bgm==GH_BGM_UNSUPPORTED){
                uint32_t frontier=gh_bgm_requests_frontier(h->bgm);
                entry_frontier(h,frontier?frontier:0x2d0938,0,bgm);h->phase=GH_HOST_FRONTIER;
            }else{failure(h,GH_HOST_AUDIO,bgm);h->phase=GH_HOST_FAILED;}
        }
    }
    if(h->phase==GH_HOST_RUNNING && h->voices){
        gh_voice_status audio=gh_sound_control_flush_se_002ca5d8(&h->sound_control,h->voices);
        if(audio){failure(h,GH_HOST_AUDIO,audio);h->phase=GH_HOST_FAILED;}
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
    out->fade=&h->fade;out->fade_draw_serial=h->fade_draw_serial;
    out->bgm=h->bgm;
    out->display=&h->display;
    out->scene_frame_serial=h->scene_frame_serial;out->scene_frame_ready=h->scene_frame_ready;
    out->ui_resource_epoch=gh_game_services_ui_resource_epoch(h->services);
    out->current_room=(uint16_t)h->work.room;memcpy(&out->next_room,(const uint8_t*)&h->work+0x5b4,2);
    if(h->services){gh_game_services_scene_view scene;
        if(!gh_game_services_get_scene_view(h->services,&scene)&&scene.title){gh_r020_title_view title;
            if(!gh_r020_title_inspect(scene.title,&title)&&title.field_bytes>=0x20){
                out->title_live=1;out->title_mode=title.fields[0];out->title_phase=title.fields[1];out->title_subphase=title.fields[2];
                memcpy(&out->title_timer,title.fields+8,4);memcpy(&out->title_game_selection,title.fields+0x14,4);
                memcpy(&out->title_level,title.fields+0x1c,4);out->title_move_serial=title.move_serial;
            }
        }
    }
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
    /* Close actual BGM endpoints before encoded source/heap/registry owners. */
    if(h->bgm){gh_bgm_requests_destroy(h->bgm);h->bgm=0;}
    if(gh_sound_scene_release_loading(&h->sound_scene,&h->dvd,h->heap)!=GH_SOUND_SCENE_OK)return GH_HOST_AUDIO;
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
    gh_disc_close(h->disc);free(h->boot_reference);free(h->ffmpeg_path);free(h->iso_path);free(h);return GH_HOST_OK;
}
