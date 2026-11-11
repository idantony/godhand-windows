#include "game_event_se_playback.h"
#include "game_event_play_env.h"
#include "game_effects.h"
#include "bgm_requests.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)

static void put(uint8_t *p,uint32_t value){memcpy(p,&value,4);}

static uint8_t *load(gh_disc *disc,const char *path,size_t *bytes)
{
    size_t index;
    gh_disc_entry entry;
    CHECK(!gh_disc_find(disc,path,&index) && !gh_disc_entry_at(disc,index,&entry) && entry.size<=SIZE_MAX);
    *bytes=(size_t)entry.size;
    uint8_t *buffer=malloc(*bytes);
    CHECK(buffer && !gh_disc_read(disc,index,0,buffer,*bytes));
    return buffer;
}

static gh_game_event_status register_effects(void *context,const uint8_t *data,size_t size,uint32_t *frontier)
{
    gh_game_effects *effects=context;
    gh_effect_status status=gh_game_effects_register_event_002fa0b0(effects,data,size);
    if(status)*frontier=effects->frontier;
    return status?GH_EVENT_CALLEE:GH_EVENT_OK;
}

static gh_game_event_status prefetch(void *context,const gh_event_audio_prefetch_request *source,uint32_t *frontier)
{
    gh_bgm_status status=gh_bgm_event_prefetch_002d1c78(context,source);
    *frontier=gh_bgm_requests_frontier(context);
    return status?GH_EVENT_CALLEE:GH_EVENT_OK;
}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    gh_disc *disc;
    gh_heap *heap;
    CHECK(!gh_disc_open(argv[1],&disc) && !gh_heap_create(16*1024*1024,&heap));
    size_t elf_bytes=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_bytes);
    CHECK(elf && !gh_disc_read_boot_elf(disc,0,elf,elf_bytes));
    gh_game_event event={0};
    gh_game_event_config config={0};
    gh_boot_work work={0};
    CHECK(!gh_game_event_from_elf(&event,elf,elf_bytes) && !gh_game_event_config_from_elf(&config,elf,elf_bytes));
    CHECK(!gh_game_event_init_environment_002975f8(&event,&config,1,elf,elf_bytes));
    gh_dvd_jobs dvd;
    gh_dvd_jobs_init(&dvd,disc);
    gh_event_module module={0};
    gh_event_resource resource={0};
    CHECK(!gh_game_event_module_begin_00297378(&module,&event,&work,&dvd,heap));
    int ready=0;
    for(unsigned i=0;i<8 && !ready;++i)CHECK(!gh_dvd_pump(&dvd) && !gh_game_event_module_poll_00297428(&module,&ready));
    CHECK(ready && module.loaded);
    gh_event_link *link=calloc(1,sizeof(*link));
    CHECK(link && !gh_game_event_link_002be890(link,&module,elf,elf_bytes));
    CHECK(!gh_event_components_construct_00297680(&link->components,1));
    CHECK(!gh_game_event_resource_begin_00297540(&resource,&event,&dvd,heap));
    ready=0;
    for(unsigned i=0;i<8 && !ready;++i)CHECK(!gh_dvd_pump(&dvd) && !gh_game_event_resource_poll_002975d0(&resource,&ready));
    CHECK(ready && resource.loaded);

    gh_event_data_prefix data={0};
    uint32_t frontier;
    int relative;
    CHECK(!gh_event_data_bind_prefix_002977e8(&data,&link->components,&resource,&frontier));
    CHECK(!gh_event_actor_cuts_bind_00297864(&data,&frontier));
    size_t core_size,room_size;
    uint8_t *core=load(disc,"etc/core.dat",&core_size),*room_bytes=load(disc,"st1/r100.dat",&room_size);
    gh_bundle room;
    CHECK(!gh_bundle_open(room_bytes,room_size,&room));
    gh_game_effects *effects=calloc(1,sizeof(*effects));
    uint8_t *esp=calloc(1,0x100180),*emitters=calloc(1,0x26060);
    CHECK(effects && esp && emitters && !gh_game_effects_initialize_00305a68(effects,esp,0x100180,emitters,0x26060,core,core_size,&room));
    CHECK(!gh_event_data_bind_tail_00297880(&data,&link->components,register_effects,effects,&frontier));
    gh_event_sequence_view sequence;
    CHECK(!gh_event_sequence_at(&data,8,&sequence) && !sequence.count && !sequence.records);
    CHECK(!gh_event_sequence_at(&data,9,&sequence) && !sequence.count && !sequence.records);

    gh_audio_registry *registry;
    gh_sound_control control={0};
    gh_bgm_requests *requests;
    gh_adx_key key;
    CHECK(!gh_audio_registry_create(&registry) && !gh_audio_startup_load(registry,disc));
    CHECK(!gh_sound_control_from_startup(&control,disc,registry));
    CHECK(!gh_bgm_requests_create(registry,&control,disc,heap,&requests) && !gh_adx_key_from_elf(elf,elf_bytes,&key));
    CHECK(!gh_bgm_requests_enable_native(requests,&key,1,GH_BGM_GAIN_NATIVE_DB_REFERENCE));
    gh_event_audio_channel1 audio={0};
    CHECK(!gh_event_audio_channel1_bind(&audio,&control,&dvd,heap) && !gh_event_audio_channel1_bind_registry(&audio,registry));
    CHECK(!gh_event_audio_channel1_bind_prefetch(&audio,prefetch,requests) && !gh_event_audio_channel1_bind_lifecycle(&audio,requests));
    CHECK(!gh_event_audio_channel1_service(&audio,GH_EVENT_AUDIO_PREPARE_002D1900,1,0,&ready,&frontier));
    for(unsigned i=0;i<8 && gh_dvd_check(&dvd,audio.job);++i)CHECK(!gh_dvd_pump(&dvd));
    CHECK(!gh_event_audio_channel1_tick_002d0068(&audio) && !gh_event_audio_channel1_tick_002d0068(&audio) && audio.flags==15);
    CHECK(!gh_event_audio_channel1_service(&audio,GH_EVENT_AUDIO_START_002D1A30,1,-1,&ready,&frontier));

    gh_screen_fade fade={0};
    gh_display_state display={.initialized=1,.screen_width=512,.gs_y_origin=1824};
    CHECK(!gh_event_env_init_play_00005930(&link->components,&config,&work,&fade,&display,&frontier));
    CHECK(!gh_event_se_move_play_00009aa8(link,&data,&audio,&frontier,&relative));
    gh_event_component_view env;
    CHECK(!gh_event_component_at(&link->components,0,&env));
    /* Supply the observed EventEnv publication boundary explicitly. */
    put(env.bytes+0x10,0);
    put(env.bytes+0x14,0);
    CHECK(!gh_event_se_move_play_00009aa8(link,&data,&audio,&frontier,&relative));
    put(env.bytes+0x28,1);
    put(env.bytes+0x14,0x3f800000);
    CHECK(!gh_event_se_move_play_00009aa8(link,&data,&audio,&frontier,&relative));
    put(env.bytes+0x14,0);
    uint8_t *debug_flags=(uint8_t*)module.allocation.pointer+0xd1e0;
    CHECK(!debug_flags[0]);
    debug_flags[0]=1;
    CHECK(!gh_event_se_move_play_00009aa8(link,&data,&audio,&frontier,&relative));
    debug_flags[0]=0;
    CHECK(gh_event_se_move_play_00009aa8(link,&data,&audio,&frontier,&relative)==GH_EVENT_UNSUPPORTED);
    CHECK(frontier==0x3734f0 && !relative);
    gh_bgm_request_view view;
    CHECK(!gh_bgm_requests_inspect(requests,0,&view) && view.request_flags==1 && view.stream_flags==3 && !view.transport.started);

    CHECK(!gh_event_se_quit_play_00009cb8(link,&audio,&frontier,&relative) && !audio.flags && audio.event_no==-1);
    CHECK(gh_bgm_requests_active(requests)==1 && !gh_bgm_requests_inspect(requests,0,&view) && view.encoded.data);
    put((uint8_t*)&work+0x5e4,0x20000000);
    for(unsigned i=0;i<10;++i)CHECK(!gh_bgm_requests_update_002d0938(requests,&work) && gh_bgm_requests_active(requests)==1);
    CHECK(!gh_bgm_requests_update_002d0938(requests,&work) && !gh_bgm_requests_active(requests));
    gh_bgm_requests_destroy(requests);
    CHECK(!gh_event_audio_channel1_shutdown(&audio));
    gh_audio_registry_destroy(registry);
    free(effects);free(esp);free(emitters);free(core);free(room_bytes);
    free(link);
    CHECK(!gh_game_event_resource_abort(&resource) && !gh_game_event_module_abort(&module) && !gh_heap_used(heap));
    CHECK(!gh_heap_destroy(heap));
    free(elf);gh_disc_close(disc);
    puts("PASS actual EV01 EventSe cut gates, no-device start boundary and DataEnd teardown");
    return 0;
}
