#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "game_event_audio.h"
#include "bgm_requests.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); exit(1); } \
} while (0)

static gh_game_event_status prefetch(void *context,const gh_event_audio_prefetch_request *request,uint32_t *frontier)
{
    gh_bgm_requests *bgm=context;
    gh_bgm_status status=gh_bgm_event_prefetch_002d1c78(bgm,request);
    *frontier=gh_bgm_requests_frontier(bgm);
    if(status==GH_BGM_UNSUPPORTED)return GH_EVENT_UNSUPPORTED;
    return status?GH_EVENT_CALLEE:GH_EVENT_OK;
}

int main(int argc,char **argv)
{
    CHECK(argc==2 || (argc==3 && !strcmp(argv[2],"--hardware-audio")));
    gh_disc *disc;
    gh_heap *heap;
    gh_audio_registry *registry;
    gh_sound_control control={0};
    gh_dvd_jobs dvd;
    gh_event_audio_channel1 state={0};
    CHECK(!gh_disc_open(argv[1],&disc));
    CHECK(!gh_heap_create(16*1024*1024,&heap));
    CHECK(!gh_audio_registry_create(&registry) && !gh_audio_startup_load(registry,disc));
    CHECK(!gh_sound_control_from_startup(&control,disc,registry));
    gh_dvd_jobs_init(&dvd,disc);
    CHECK(!gh_event_audio_channel1_bind(&state,&control,&dvd,heap));
    CHECK(gh_event_audio_channel1_bind(&state,&control,&dvd,heap)==GH_EVENT_ARGUMENT);
    int ready;
    uint32_t frontier;
    CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_PREPARE_002D1900,1,0,&ready,&frontier));
    uint32_t job=state.job;
    CHECK(job && state.flags==1 && state.mode==0x81 && state.event_no==1 && state.afd_bytes==48);
    CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_READY_002D19D8,1,0,&ready,&frontier) && !ready);
    CHECK(!gh_event_audio_channel1_tick_002d0068(&state) && state.flags==1);
    for(unsigned i=0;i<8 && gh_dvd_check(&dvd,job);++i)CHECK(!gh_dvd_pump(&dvd));
    CHECK(!gh_dvd_check(&dvd,job));
    CHECK(gh_event_audio_channel1_tick_002d0068(&state)==GH_EVENT_UNSUPPORTED);
    CHECK(state.frontier==0x322a68 && state.flags==3 && state.row_offset==16 && state.afs_bytes==2211840);
    CHECK(state.work.pointer && gh_heap_used(heap)==2048+284);
    void *work=state.work.pointer;
    CHECK(gh_event_audio_channel1_tick_002d0068(&state)==GH_EVENT_UNSUPPORTED);
    CHECK(state.frontier==0x322a68 && state.work.pointer==work && dvd.id_counter==job);
    CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_READY_002D19D8,1,0,&ready,&frontier) && !ready);
    gh_audio_registry *foreign;
    CHECK(!gh_audio_registry_create(&foreign));
    CHECK(gh_event_audio_channel1_bind_registry(&state,foreign)==GH_EVENT_ARGUMENT);
    gh_audio_registry_destroy(foreign);
    CHECK(!gh_event_audio_channel1_bind_registry(&state,registry));
    CHECK(!gh_event_audio_channel1_tick_002d0068(&state) && state.flags==7 && state.work.pointer==work);
    CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_READY_002D19D8,1,0,&ready,&frontier) && !ready);
    uint64_t offset,size;
    CHECK(!gh_audio_event_afs_member(registry,&state,0,&offset,&size) && offset==0x800 && size==0x21ae3a);
    CHECK(gh_audio_event_afs_member(registry,&state,1,&offset,&size)==GH_AUDIO_BOUNDS);
    CHECK(gh_audio_event_afs_member(registry,&control,0,&offset,&size)==GH_AUDIO_NOT_FOUND);
    CHECK(gh_audio_event_afs_release(registry,&control)==GH_AUDIO_NOT_FOUND);
    CHECK(!gh_event_audio_channel1_tick_002d0068(&state) && state.flags==15);
    CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_READY_002D19D8,1,0,&ready,&frontier) && ready);
    CHECK(gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_START_002D1A30,1,-1,&ready,&frontier)==GH_EVENT_UNSUPPORTED);
    CHECK(frontier==0x2d22b0 && state.flags==15);
    gh_bgm_requests *bgm;
    size_t elf_bytes=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_bytes);
    gh_adx_key key;
    CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_bytes)&&!gh_adx_key_from_elf(elf,elf_bytes,&key));
    gh_game_event event={0};
    CHECK(!gh_game_event_from_elf(&event,elf,elf_bytes));
    free(elf);
    CHECK(!gh_bgm_requests_create(registry,&control,disc,heap,&bgm));
    CHECK(!gh_bgm_requests_enable_native(bgm,&key,1,GH_BGM_GAIN_NATIVE_DB_REFERENCE));
    CHECK(!gh_event_audio_channel1_bind_prefetch(&state,prefetch,bgm));
    gh_game_event_status prefetch_status=gh_event_audio_channel1_service(&state,
        GH_EVENT_AUDIO_START_002D1A30,1,-1,&ready,&frontier);
    if(prefetch_status)fprintf(stderr,"prefetch status %d frontier %08x\n",prefetch_status,frontier);
    CHECK(!prefetch_status && !frontier);
    CHECK(!gh_bgm_requests_pending(bgm) && gh_bgm_requests_active(bgm)==1);
    gh_bgm_request_view view;
    CHECK(!gh_bgm_requests_inspect(bgm,0,&view));
    CHECK(view.logical_id==0x81 && view.event==0 && view.mode==4 && view.request_flags==1 && view.stream_flags==3);
    CHECK(view.encoded.size==0x21ae3a && view.linked && !view.prepared && !view.transport.started && !view.transport.submitted_frames);
    gh_adx_decoder *decoder=NULL;
    CHECK(!gh_adx_open(view.encoded.data,view.encoded.size,&key,&decoder) && decoder);
    gh_adx_close(decoder);
    size_t used=gh_heap_used(heap);
    gh_boot_work work_flags={0};
    uint32_t event_active=0x20000000;
    memcpy((uint8_t*)&work_flags+0x5e4,&event_active,4);
    for(unsigned i=0;i<3;++i)CHECK(!gh_bgm_requests_update_002d0938(bgm,&work_flags));
    CHECK(!gh_bgm_requests_inspect(bgm,0,&view) && view.stream_flags==3 && !view.transport.started);
    CHECK(!gh_bgm_event_fade_002d1768(bgm,0,1,0,1)); /* Room archive 0x80 is absent. */
    CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_START_002D1A30,1,-1,&ready,&frontier));
    CHECK(!frontier && gh_heap_used(heap)==used && gh_bgm_requests_active(bgm)==1);
    CHECK(gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_START_002D1A30,1,0,&ready,&frontier)==GH_EVENT_UNSUPPORTED);
    CHECK(frontier==0x2d1d00);
    CHECK(gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_WAIT_002D1B70,1,0,&ready,&frontier)==GH_EVENT_UNSUPPORTED);
    CHECK(frontier==0x2d1dd0 && !ready);
    CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_WAIT_002D1B70,1,-1,&ready,&frontier) && ready);
    CHECK(!gh_event_audio_channel1_bind_lifecycle(&state,bgm));
    CHECK(!gh_bgm_event_wait_002d1dd0(bgm,1,99,&ready) && ready);
    CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_WAIT_002D1B70,1,0,&ready,&frontier) && ready);
    CHECK(gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_START_002D1A30,1,0,&ready,&frontier)==GH_EVENT_UNSUPPORTED);
    CHECK(frontier==0x3734f0 && !gh_bgm_requests_inspect(bgm,0,&view));
    CHECK(view.request_flags==1 && view.stream_flags==3 && !view.transport.started && !view.transport.submitted_frames);

    /* Actual phase3/4 callers with the real ev01 DVD data. The fixture starts
     * after module/component creation and leaves the data consumer unbound. */
    event.fields.data_number=1;
    event.fields.state=2;
    event.fields.phase=3;
    gh_event_resource resource={0};
    CHECK(!gh_game_event_resource_begin_00297540(&resource,&event,&dvd,heap));
    for(unsigned i=0;i<8 && gh_dvd_check(&dvd,resource.job);++i)CHECK(!gh_dvd_pump(&dvd));
    gh_event_module module={0};
    gh_event_components components={0};
    gh_event_create_state create={0};
    gh_event_create_bindings bindings={.event=&event,.work=&work_flags,.module=&module,
        .resource=&resource,.components=&components,.dvd=&dvd,.heap=heap,
        .audio_channel1=gh_event_audio_channel1_service,.context=&state};
    int done=1;
    CHECK(gh_game_event_create_00296958(&create,&bindings,&done)==GH_EVENT_UNSUPPORTED);
    CHECK(create.frontier==0x2977e8 && !done && event.fields.phase==4 && create.substep==1 && resource.loaded);
    uint32_t bgm_sequence_offset;
    memcpy(&bgm_sequence_offset,(const uint8_t*)resource.allocation.pointer+0x2c,4);
    CHECK(!bgm_sequence_offset);
    CHECK(gh_game_event_create_00296958(&create,&bindings,&done)==GH_EVENT_UNSUPPORTED);
    CHECK(create.frontier==0x2977e8 && gh_bgm_requests_active(bgm)==1);
    CHECK(!gh_game_event_resource_abort(&resource));
    CHECK(!gh_bgm_event_fade_002d1e18(bgm,0,0));
    CHECK(!gh_bgm_requests_inspect(bgm,0,&view) && view.original_scalars[0x1a]==0x80);
    CHECK(!gh_bgm_event_fade_002d1e18(bgm,1,-1));
    for(unsigned i=0;i<15;++i){
        CHECK(!gh_bgm_requests_update_002d0938(bgm,&work_flags) && gh_bgm_requests_active(bgm)==1);
    }
    CHECK(!gh_bgm_requests_inspect(bgm,0,&view) && !view.encoded.data && !view.transport.started);
    CHECK(!gh_bgm_requests_update_002d0938(bgm,&work_flags) && !gh_bgm_requests_active(bgm));
    CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_START_002D1A30,1,-1,&ready,&frontier));
    if(argc==3){
        CHECK(!gh_bgm_event_enable_output(bgm));
        CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_START_002D1A30,1,0,&ready,&frontier));
        CHECK(!gh_bgm_requests_inspect(bgm,0,&view));
        CHECK(!view.request_flags && view.stream_flags==1 && view.transport.started && view.transport.muted);
        CHECK(view.transport.submitted_frames>0);
        int original=0;
        CHECK(!gh_bgm_event_start_002d1d00(bgm,1,0,45,&original) && original==1);
        ready=0;
        for(unsigned i=0;i<200 && !ready;++i){
            Sleep(10);
            CHECK(!gh_bgm_requests_update_002d0938(bgm,&work_flags));
            CHECK(!gh_bgm_event_wait_002d1dd0(bgm,1,0,&ready));
        }
        CHECK(ready && !gh_bgm_requests_inspect(bgm,0,&view) && view.transport.completed_frames>0);
        CHECK(!gh_bgm_event_fade_002d1e18(bgm,1,-1));
        for(unsigned i=0;i<16;++i)CHECK(!gh_bgm_requests_update_002d0938(bgm,&work_flags));
        CHECK(!gh_bgm_requests_active(bgm));
        CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_START_002D1A30,1,-1,&ready,&frontier));
        puts("PASS ev01 muted hardware start, positive wait, fade and unlink");
    }
    /* Archive 0x80 row 0 and archive 0x81 row 0 occupy different slots in
     * the same pool. Use the room's device-free constructor prefix. */
    CHECK(gh_bgm_event_set_prefix_002d15a8(bgm,0,0,0)==GH_BGM_UNSUPPORTED);
    CHECK(gh_bgm_requests_frontier(bgm)==0x329438);
    CHECK(gh_bgm_requests_active(bgm)==1 && gh_bgm_requests_pending(bgm)==1);
    CHECK(!gh_bgm_requests_inspect(bgm,1,&view) && view.logical_id==0x80 && view.event==0 && !view.linked);
    CHECK(!gh_bgm_requests_inspect(bgm,0,&view) && view.logical_id==0x81 && view.event==0 && view.linked);
    gh_bgm_requests_destroy(bgm);
    CHECK(!gh_event_audio_channel1_shutdown(&state) && !gh_heap_used(heap));
    CHECK(gh_audio_event_afs_member(registry,&state,0,&offset,&size)==GH_AUDIO_NOT_FOUND);
    CHECK(gh_audio_startup_ready(registry,NULL,NULL,NULL));

    CHECK(!gh_event_audio_channel1_bind(&state,&control,&dvd,heap));
    CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_PREPARE_002D1900,1,0,&ready,&frontier));
    job=state.job;
    CHECK(job && gh_dvd_check(&dvd,job));
    CHECK(!gh_event_audio_channel1_shutdown(&state) && !gh_dvd_check(&dvd,job) && !gh_heap_used(heap));
    CHECK(!gh_event_audio_channel1_shutdown(&state));
    /* R003 requests Event 2 on the same channel. Prove Create's original
     * pre-roll through the actual AFD, AFS, encrypted ADX and event resource. */
    CHECK(!gh_event_audio_channel1_bind(&state,&control,&dvd,heap));
    CHECK(!gh_event_audio_channel1_bind_registry(&state,registry));
    CHECK(!gh_bgm_requests_create(registry,&control,disc,heap,&bgm));
    CHECK(!gh_bgm_requests_enable_native(bgm,&key,1,GH_BGM_GAIN_NATIVE_DB_REFERENCE));
    CHECK(!gh_event_audio_channel1_bind_prefetch(&state,prefetch,bgm));
    CHECK(!gh_event_audio_channel1_bind_lifecycle(&state,bgm));
    CHECK(!gh_event_audio_channel1_service(&state,GH_EVENT_AUDIO_PREPARE_002D1900,2,0,&ready,&frontier));
    for(unsigned i=0;i<8 && gh_dvd_check(&dvd,state.job);++i)CHECK(!gh_dvd_pump(&dvd));
    CHECK(!gh_event_audio_channel1_tick_002d0068(&state));
    CHECK(!gh_event_audio_channel1_tick_002d0068(&state) && state.flags==15);
    CHECK(!gh_audio_event_afs_member(registry,&state,0,&offset,&size) && offset==0x800 && size==0x4e303a);

    event.fields.data_number=2;
    event.fields.state=2;
    event.fields.phase=3;
    resource=(gh_event_resource){0};
    create=(gh_event_create_state){0};
    CHECK(!gh_game_event_resource_begin_00297540(&resource,&event,&dvd,heap));
    for(unsigned i=0;i<8 && gh_dvd_check(&dvd,resource.job);++i)CHECK(!gh_dvd_pump(&dvd));
    CHECK(gh_game_event_create_00296958(&create,&bindings,&done)==GH_EVENT_UNSUPPORTED);
    if(create.frontier!=0x2977e8)fprintf(stderr,"EV02 Create frontier %08x\n",create.frontier);
    CHECK(create.frontier==0x2977e8 && !done && event.fields.phase==4 && create.substep==1 && resource.loaded);
    CHECK(!gh_bgm_requests_inspect(bgm,0,&view) && view.logical_id==0x81 && view.event==0 &&
          view.encoded.size==size && view.linked && view.request_flags==1 && view.stream_flags==3 &&
          !view.transport.started && !view.transport.submitted_frames);
    CHECK(!gh_adx_open(view.encoded.data,view.encoded.size,&key,&decoder));
    gh_adx_info info;
    CHECK(!gh_adx_inspect(decoder,&info) && info.sample_count==0x458000 && gh_adx_tell(decoder)==0);
    gh_adx_close(decoder);
    CHECK(!gh_game_event_resource_abort(&resource));
    gh_bgm_requests_destroy(bgm);
    CHECK(!gh_event_audio_channel1_shutdown(&state) && !gh_heap_used(heap));
    puts("PASS ev02 actual paused Create prefetch and phase4 handoff without audio output");

    gh_audio_registry_destroy(registry);
    CHECK(!gh_heap_destroy(heap));
    gh_disc_close(disc);
    puts("PASS ev01 paused shared-pool prefetch/link, stage4 data-bind frontier, no device, cleanup");
    return 0;
}
