/* Native execution driver. Unrecovered original consumers remain explicit
 * frontiers; reaching one never means the game has finished or is playable. */
#include "platform/startup_host.h"
#include "platform/window_host.h"
#include "platform/mc_renderer.h"
#include "platform/ui_fade_renderer.h"
#include "core/ui_order.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

struct NativeApp {
    gh_startup_host *host=nullptr;
    gh_window_host *window=nullptr;
    gh_mc_renderer *renderer=nullptr;
    gh_startup_snapshot snapshot={};
    std::vector<unsigned char> elf;
    std::vector<unsigned char> queued_pixels;
    std::vector<gh_ui_order_group> groups;
    bool frame_building=false,display_pending=false,building_scene=false,pending_scene=false;
    uint64_t prepared_mc_serial=0,pending_mc_serial=0,pending_fade_serial=0;
    uint64_t prepared_scene_serial=0,presented_scene=0;
    uint64_t resource_epoch=0,frame_serial=0,presented_mc=0,presented_movie=0;
    uint64_t presented_fade_serial=0;
    unsigned surface_kind=0; /*1=actual UI512448,2=Movie640448*/
    gh_window_error window_error=GH_WINDOW_OK;
};
static gh_id_error begin_mc(void *opaque)
{
    auto &app=*(NativeApp*)opaque;gh_startup_snapshot s={};
    if(gh_startup_host_snapshot(app.host,&s)!=GH_HOST_OK)return GH_ID_ARGUMENT;
    if(s.mc_ui_epoch!=app.resource_epoch){gh_mc_renderer_reset_resources(app.renderer);app.resource_epoch=s.mc_ui_epoch;}
    gh_mc_renderer_begin(app.renderer);app.groups.clear();app.frame_building=true;app.building_scene=false;app.surface_kind=1;return GH_ID_OK;
}
static gh_id_error begin_scene(void *opaque,uint32_t *)
{
    auto &app=*(NativeApp*)opaque;gh_startup_snapshot s={};
    if(gh_startup_host_snapshot(app.host,&s)!=GH_HOST_OK)return GH_ID_ARGUMENT;
    if(s.mc_ui_epoch!=app.resource_epoch){gh_mc_renderer_reset_resources(app.renderer);app.resource_epoch=s.mc_ui_epoch;}
    gh_mc_renderer_begin(app.renderer);app.groups.clear();app.frame_building=true;app.building_scene=true;app.surface_kind=1;return GH_ID_OK;
}
static gh_id_error draw_mc(void *opaque,const gh_id_draw_job *job)
{
    auto &app=*(NativeApp*)opaque;gh_startup_snapshot s={};
    if(gh_startup_host_snapshot(app.host,&s)!=GH_HOST_OK)return GH_ID_ARGUMENT;
    gh_id_render_view view={s.font,s.menu,s.id,s.mc_ui,s.mc_messages};gh_ui_ot_tag tags[10];size_t count=0;
    gh_id_error error=gh_mc_renderer_queue_view_groups(app.renderer,&view,job,tags,10,&count);
    if(error==GH_ID_OK){try {for(size_t i=0;i<count;++i)app.groups.push_back({tags[i].bank,(uint32_t)tags[i].depth,(uint64_t)tags[i].index});}
        catch(const std::bad_alloc&){return GH_ID_MEMORY;}}
    return error;
}
static gh_id_error draw_scene(void *opaque,const gh_id_base *base,const gh_id_draw_job *job,uint32_t *frontier)
{
    auto &app=*(NativeApp*)opaque;gh_startup_snapshot s={};
    if(gh_startup_host_snapshot(app.host,&s)!=GH_HOST_OK)return GH_ID_ARGUMENT;
    gh_id_render_view view={s.font,s.menu,s.id,base,nullptr};gh_ui_ot_tag tags[10];size_t count=0;
    gh_id_error error=gh_mc_renderer_queue_view_groups(app.renderer,&view,job,tags,10,&count);
    if(error==GH_ID_OK){try {for(size_t i=0;i<count;++i)app.groups.push_back({tags[i].bank,(uint32_t)tags[i].depth,(uint64_t)tags[i].index});}
        catch(const std::bad_alloc&){return GH_ID_MEMORY;}}
    else if(frontier){gh_mc_render_status status={};gh_mc_renderer_inspect(app.renderer,&status);*frontier=status.frontier?status.frontier:job->consumer_address;}
    return error;
}
static gh_id_error draw_fan(void *opaque,const gh_cockpit_fan *fan,uint32_t *frontier)
{
    auto &app=*(NativeApp*)opaque;gh_ui_ot_tag tag={};
    gh_id_error error=gh_mc_renderer_queue_fan(app.renderer,fan,&tag);
    if(error==GH_ID_OK){try {app.groups.push_back({tag.bank,(uint32_t)tag.depth,(uint64_t)tag.index});}
        catch(const std::bad_alloc&){return GH_ID_MEMORY;}}
    else if(frontier)*frontier=0x15c410;return error;
}
static gh_id_error draw_cover(void *opaque,const gh_child_cover_quad *quad,uint32_t *frontier)
{
    auto &app=*(NativeApp*)opaque;gh_ui_ot_tag tag={};
    gh_id_error error=gh_mc_renderer_queue_cover(app.renderer,quad,&tag);
    if(error==GH_ID_OK){try {app.groups.push_back({tag.bank,(uint32_t)tag.depth,(uint64_t)tag.index});}
        catch(const std::bad_alloc&){return GH_ID_MEMORY;}}
    else if(frontier)*frontier=0x15b880;return error;
}
static gh_id_error post_mc(void *,uint16_t message)
{
    std::fprintf(stderr,"Unrecovered original ID postMessage %04X\n",message);
    return GH_ID_UNSUPPORTED;
}
static int present_movie(void *opaque,const unsigned char *rgba,size_t bytes,unsigned width,unsigned height,uint64_t)
{
    auto &app=*(NativeApp*)opaque;
    app.window_error=gh_window_present(app.window,rgba,bytes,width,height,4,3);
    if(app.window_error!=GH_WINDOW_OK)return 0;
    app.surface_kind=2;++app.presented_movie;return 1;
}
static gh_id_error draw_fade(void *opaque,const gh_fade_draw *draw)
{
    auto &app=*(NativeApp*)opaque;
    if(!draw||app.surface_kind!=1||!app.frame_building)return GH_ID_UNSUPPORTED;
    gh_startup_snapshot s={};if(gh_startup_host_snapshot(app.host,&s)!=GH_HOST_OK)return GH_ID_ARGUMENT;
    const gh_display_state *display=s.display;float aspect=0;
    if(!display||!display->initialized||display->screen_width!=512||display->screen_lines!=448)return GH_ID_UNSUPPORTED;
    std::memcpy(&aspect,&display->pixel_aspect_bits,4);
    gh_ui_fade_context context={512,448,display->gs_y_origin,aspect,1,1,0,0,0};gh_ui_ot_tag tag={};
    gh_id_error error=gh_mc_renderer_queue_fade(app.renderer,draw,&context,&tag);
    if(error==GH_ID_OK){try {app.groups.push_back({tag.bank,(uint32_t)tag.depth,(uint64_t)tag.index});}
        catch(const std::bad_alloc&){return GH_ID_MEMORY;}}
    return error;
}
static gh_id_error prepare_ui_display(NativeApp &app,bool scene)
{
    size_t written=0,omitted=0;uint32_t clear=0;
    if(!app.snapshot.work||!app.frame_building)return GH_ID_ARGUMENT;
    std::vector<size_t> order(app.groups.size());
    if(gh_ui_order_indices(app.groups.data(),app.groups.size(),app.snapshot.flags,(uint32_t)app.snapshot.work->mode,
        order.data(),order.size(),&written,&omitted)!=GH_UI_ORDER_OK||omitted||written!=order.size())return GH_ID_UNSUPPORTED;
    std::memcpy(&clear,(const unsigned char*)app.snapshot.work+0x60c,4);
    gh_id_error error=gh_mc_renderer_flush_queued(app.renderer,order.data(),order.size(),clear);
    if(error!=GH_ID_OK)return error;
    size_t bytes=0;const unsigned char *base=gh_mc_renderer_pixels(app.renderer,&bytes);
    if(!base||bytes!=512*448*4)return GH_ID_UNSUPPORTED;
    app.queued_pixels.assign(base,base+bytes);app.display_pending=true;
    app.pending_scene=scene;app.pending_fade_serial=app.snapshot.fade_draw_serial;
    app.pending_mc_serial=app.snapshot.mc_frame_serial;
    if(scene)app.prepared_scene_serial=app.snapshot.scene_frame_serial;
    else app.prepared_mc_serial=app.snapshot.mc_frame_serial;
    app.frame_building=false;return GH_ID_OK;
}
static void usage()
{
    std::fprintf(stderr,"usage: godhand-windows --iso ISO --profile DIRECTORY [--ffmpeg EXE] [--show|--hidden] [--muted] [--prefix] [--max-ticks N]\n");
}
int main(int argc,char **argv)
{
    const char *iso=nullptr,*profile=nullptr,*ffmpeg=nullptr;
    bool show=false,muted=false,prefix=false;uint64_t max_ticks=0;
#ifdef GH_NATIVE_STARTUP_QA
    bool qa_confirm=false,qa_new_game=false,qa_captured=false;unsigned qa_choice_step=0;gh_native_input qa_input={};
    const char *qa_capture=nullptr;uint64_t qa_capture_frame=1;uint32_t qa_title_trace=UINT32_MAX;
#endif
    for(int i=1;i<argc;++i){
        if(!std::strcmp(argv[i],"--show"))show=true;
        else if(!std::strcmp(argv[i],"--hidden"))show=false;
#ifdef GH_NATIVE_STARTUP_QA
        else if(!std::strcmp(argv[i],"--qa-new-game")){qa_new_game=true;qa_confirm=true;}
        else if(!std::strcmp(argv[i],"--qa-capture")&&i+1<argc)qa_capture=argv[++i];
        else if(!std::strcmp(argv[i],"--qa-capture-frame")&&i+1<argc)qa_capture_frame=std::strtoull(argv[++i],nullptr,10);
#endif
        else if(!std::strcmp(argv[i],"--muted"))muted=true;
        else if(!std::strcmp(argv[i],"--prefix"))prefix=true;
#ifdef GH_NATIVE_STARTUP_QA
        else if(!std::strcmp(argv[i],"--qa-confirm"))qa_confirm=true;
#endif
        else if((!std::strcmp(argv[i],"--iso")||!std::strcmp(argv[i],"--profile")||!std::strcmp(argv[i],"--ffmpeg")||!std::strcmp(argv[i],"--max-ticks"))&&i+1<argc){
            const char *key=argv[i++];
            if(!std::strcmp(key,"--iso"))iso=argv[i];
            else if(!std::strcmp(key,"--profile"))profile=argv[i];
            else if(!std::strcmp(key,"--ffmpeg"))ffmpeg=argv[i];
            else {char *end=nullptr;max_ticks=std::strtoull(argv[i],&end,10);if(!max_ticks||!end||*end){usage();return 2;}}
        }else {usage();return 2;}
    }
    if(!iso||!profile){usage();return 2;}
#ifdef GH_NATIVE_STARTUP_QA
    if(qa_confirm&&(!muted||show||!max_ticks)){std::fprintf(stderr,"Synthetic input QA requires --muted --hidden and --max-ticks.\n");return 2;}
#endif
    bool synthetic_input=false;
#ifdef GH_NATIVE_STARTUP_QA
    synthetic_input=qa_confirm;
#endif
    NativeApp app;gh_disc *disc=nullptr;int result=1;gh_host_error host_error=GH_HOST_OK;
    gh_frame_clock clock={};
    do {
        if(gh_disc_open(iso,&disc)!=GH_DISC_OK)break;
        app.elf.resize((size_t)gh_disc_boot_elf_size(disc));
        if(app.elf.empty()||gh_disc_read_boot_elf(disc,0,app.elf.data(),app.elf.size())!=GH_DISC_OK)break;
        gh_disc_close(disc);disc=nullptr;
        gh_window_config window={};window.width=640;window.height=480;window.show=show;
        window.title="God Hand native source port";window.xinput_index=0;
        gh_native_input_defaults(&window.input);
        // Actual US boot language1, phase569=2: second original mapping table,
        // repeat multiplier1. Other language/boot modes need their own profile.
        if(!gh_native_input_map_from_elf(app.elf.data(),app.elf.size(),1,&window.map))break;
#ifdef GH_NATIVE_STARTUP_QA
        if(qa_confirm){
            if(!gh_native_input_init(&qa_input,&window.input,window.map))break;
            gh_native_input_focus(&qa_input,1);
            std::fprintf(stderr,"QA fixture: synthetic digital keys through the original ELF mapping; no user input claimed.\n");
        }
#endif
        app.window_error=gh_window_create(&window,&app.window);
        if(app.window_error!=GH_WINDOW_OK)break;
        if(gh_mc_renderer_create(app.elf.data(),app.elf.size(),1,&app.renderer)!=GH_ID_OK)break;
        if(gh_mc_renderer_enable_reference_polygons(app.renderer,1)!=GH_ID_OK)break;
        host_error=gh_startup_host_create_with_profile(iso,profile,1,0,64*1024*1024,&app.host);
        if(host_error!=GH_HOST_OK)break;
        host_error=gh_startup_host_set_mc_consumers(app.host,draw_mc,&app,post_mc,&app);
        if(host_error!=GH_HOST_OK)break;
        host_error=gh_startup_host_set_mc_frame_begin(app.host,begin_mc,&app);
        if(host_error!=GH_HOST_OK)break;
        host_error=gh_startup_host_set_fade_consumer(app.host,draw_fade,&app);
        if(host_error!=GH_HOST_OK)break;
        gh_host_scene_ui_callbacks scene_callbacks={&app,begin_scene,draw_scene,draw_fan,draw_cover};
        host_error=gh_startup_host_set_scene_ui_consumers(app.host,&scene_callbacks);
        if(host_error!=GH_HOST_OK)break;
        if(!prefix){
            gh_startup_continuation config={};config.enabled=1;config.muted=muted;
            config.audio_dsp_policy=GH_SE_DSP_NATIVE_LINEAR_REFERENCE;
            config.ffmpeg_path=ffmpeg;config.movie_present=present_movie;config.movie_present_context=&app;
            host_error=gh_startup_host_set_continuation(app.host,&config);
            if(host_error!=GH_HOST_OK)break;
        }
        if(!gh_frame_clock_init(&clock,30000,1001))break;
        for(;;){
            gh_native_input_snapshot input={};app.window_error=gh_window_poll(app.window,&input);
            if(app.window_error!=GH_WINDOW_OK){result=app.window_error==GH_WINDOW_CLOSED?0:1;break;}
            // The original outer loop submits the previous parity's OT before
            // updating tasks. Keep the completed bitmap, not borrowed resources.
            if(app.display_pending){
                app.window_error=gh_window_present(app.window,app.queued_pixels.data(),app.queued_pixels.size(),512,448,4,3);
                if(app.window_error!=GH_WINDOW_OK)break;
                app.frame_serial=app.pending_mc_serial;app.presented_fade_serial=app.pending_fade_serial;
                if(app.pending_scene)++app.presented_scene;else ++app.presented_mc;
                app.display_pending=false;
            }
#ifdef GH_NATIVE_STARTUP_QA
            if(qa_confirm){
                unsigned key=0;
                if(app.snapshot.mc_phase==3&&app.snapshot.mc_subphase==1)key='Z';
                else if(app.snapshot.mc_phase==3&&app.snapshot.mc_subphase==2){
                    if(qa_choice_step==0){key=39;++qa_choice_step;}
                    else if(qa_choice_step==1){key='Z';++qa_choice_step;}
                }else qa_choice_step=0;
                if(qa_new_game&&app.snapshot.title_live){
                    if(app.snapshot.title_mode==1&&app.snapshot.ticks%30==0)key='Z';
                    else if(app.snapshot.title_mode==3&&app.snapshot.title_phase>=1&&app.snapshot.title_phase<=3
                        &&app.snapshot.title_subphase==1&&(app.snapshot.title_phase==1||app.snapshot.title_timer>=5)&&app.snapshot.ticks%6==0)key='Z';
                }
                gh_native_input_key(&qa_input,'Z',key=='Z');gh_native_input_key(&qa_input,39,key==39);
                if(!gh_native_input_tick(&qa_input,0,&input))break;
            }
#endif
            host_error=gh_startup_host_set_input(app.host,&input);if(host_error!=GH_HOST_OK)break;
            gh_host_phase phase=gh_startup_host_tick(app.host);
            host_error=gh_startup_host_snapshot(app.host,&app.snapshot);if(host_error!=GH_HOST_OK)break;
#ifdef GH_NATIVE_STARTUP_QA
            if(app.snapshot.title_live){uint32_t trace=(uint32_t)app.snapshot.title_mode<<16|(uint32_t)app.snapshot.title_phase<<8|app.snapshot.title_subphase;
                if(trace!=qa_title_trace){std::fprintf(stderr,"QA Title tick%llu move%llu mode%u phase%u sub%u timer%u selection%u level%u\n",
                    (unsigned long long)app.snapshot.ticks,(unsigned long long)app.snapshot.title_move_serial,
                    app.snapshot.title_mode,app.snapshot.title_phase,app.snapshot.title_subphase,
                    app.snapshot.title_timer,app.snapshot.title_game_selection,app.snapshot.title_level);qa_title_trace=trace;}
            }
#endif
            if(app.snapshot.mc_frame_ready&&app.snapshot.mc_frame_serial!=app.prepared_mc_serial){
                gh_id_error error=prepare_ui_display(app,false);
                if(error!=GH_ID_OK){std::fprintf(stderr,"Native MC OT preparation failed: %u\n",(unsigned)error);break;}
            }else if(app.snapshot.scene_frame_ready&&app.snapshot.scene_frame_serial!=app.prepared_scene_serial){
                gh_id_error error=prepare_ui_display(app,true);
                if(error!=GH_ID_OK){std::fprintf(stderr,"Native scene OT preparation failed: %u\n",(unsigned)error);break;}
#ifdef GH_NATIVE_STARTUP_QA
                if(qa_capture&&!qa_captured&&app.snapshot.scene_frame_serial>=qa_capture_frame){
                    FILE *capture=std::fopen(qa_capture,"wbx");
                    if(!capture||std::fwrite(app.queued_pixels.data(),1,app.queued_pixels.size(),capture)!=app.queued_pixels.size()){
                        if(capture)std::fclose(capture);std::fprintf(stderr,"QA scene capture failed\n");break;}
                    if(std::fclose(capture))break;qa_captured=true;
                    std::fprintf(stderr,"QA scene capture: frame%llu, 512x448 rawRGBA\n",(unsigned long long)app.snapshot.scene_frame_serial);
                }
#endif
            }
            if(phase!=GH_HOST_RUNNING){
                // Prefix mode is a bounded developer check. Full execution
                // must report a frontier as unfinished, not successful play.
                result=prefix&&phase==GH_HOST_FRONTIER&&app.presented_mc>0?0:4;break;
            }
            if(max_ticks&&app.snapshot.ticks>=max_ticks){result=3;break;}
            app.window_error=gh_window_wait_frame(app.window,&clock);
            if(app.window_error!=GH_WINDOW_OK){result=app.window_error==GH_WINDOW_CLOSED?0:1;break;}
        }
    }while(false);
    if(disc)gh_disc_close(disc);
    gh_mc_render_status renderer={};gh_mc_renderer_inspect(app.renderer,&renderer);
    uint32_t system_flags5d8=0;
    if(app.snapshot.work)std::memcpy(&system_flags5d8,(const unsigned char*)app.snapshot.work+0x5d8,4);
    std::printf("{\"scope\":\"native original startup execution; unfinished source port\","
        "\"playable_game_verified\":false,\"hidden\":%s,\"muted\":%s,\"prefix_mode\":%s,\"synthetic_input\":%s,"
        "\"ticks\":%llu,\"host_phase\":%u,\"host_error\":%u,\"subsystem_error\":%d,"
        "\"frontier_module\":\"%s\",\"frontier\":\"%08X\","
        "\"mc_frames_presented\":%llu,\"mc_frame_serial\":%llu,\"mc_ui_live\":%s,\"mc_ui_epoch\":%llu,"
        "\"movie_frames_presented\":%llu,\"movie_eof\":%s,\"movie_audio_done\":%s,\"movie_complete\":%s,"
        "\"mc_phase\":%u,\"mc_subphase\":%u,\"game_state\":%u,\"assets_loaded\":%s,"
        "\"work_flags584\":\"%08X\",\"system_flags5d8\":\"%08X\","
        "\"scene_frames_presented\":%llu,\"scene_frame_serial\":%llu,"
        "\"renderer_error\":%u,\"renderer_frontier\":\"%08X\",\"window_error\":%u,\"exit_code\":%d}\n",
        show?"false":"true",muted?"true":"false",prefix?"true":"false",synthetic_input?"true":"false",
        (unsigned long long)app.snapshot.ticks,(unsigned)app.snapshot.phase,
        (unsigned)(host_error!=GH_HOST_OK?host_error:app.snapshot.error),app.snapshot.subsystem_error,
        app.snapshot.frontier_module?app.snapshot.frontier_module:"main ELF",app.snapshot.unavailable_entry,
        (unsigned long long)app.presented_mc,(unsigned long long)app.snapshot.mc_frame_serial,
        app.snapshot.mc_ui_live?"true":"false",(unsigned long long)app.snapshot.mc_ui_epoch,
        (unsigned long long)app.presented_movie,app.snapshot.movie_video.video_eof?"true":"false",
        app.snapshot.movie_video.audio_done?"true":"false",app.snapshot.movie_video.complete?"true":"false",
        app.snapshot.mc_phase,app.snapshot.mc_subphase,(unsigned)app.snapshot.game_state,
        app.snapshot.game_assets_loaded?"true":"false",app.snapshot.flags,system_flags5d8,
        (unsigned long long)app.presented_scene,(unsigned long long)app.snapshot.scene_frame_serial,(unsigned)renderer.id_error,renderer.frontier,
        (unsigned)app.window_error,result);
    if(app.host){gh_host_error cleanup=gh_startup_host_destroy(app.host);if(cleanup!=GH_HOST_OK){std::fprintf(stderr,"Native host cleanup failed: %u\n",(unsigned)cleanup);result=1;}}
    gh_mc_renderer_destroy(app.renderer);gh_window_destroy(app.window);
    return result;
}
