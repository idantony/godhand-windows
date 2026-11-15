/* Focused host integration test. Compile with the other native modules and
 * omit the separate startup_host.c object, since this test uses its internals. */
#include "../../src/platform/startup_host.c"
#include <stdio.h>

#define CHECK(expr) do { \
    if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); exit(1); } \
} while (0)

static void verify_event_prefix(gh_startup_host *host,int finish_io)
{
    uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0};
    gh_title_player_view player;
    uint32_t frontier;
    CHECK(!gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&player));
    CHECK(!gh_game_event_from_elf(&host->event,host->boot_reference,host->boot_reference_bytes));
    host->event_config=(gh_game_event_config){0};
    CHECK(!gh_game_event_config_from_elf(&host->event_config,host->boot_reference,host->boot_reference_bytes));
    CHECK(!gh_game_event_set_stage(&host->event,2,event_create_stage,host));
    gh_event_play_start_bindings play={.event=&host->event,.config=&host->event_config,
        .work=&host->work,.player=&player,.fade=&host->fade,.display=&host->display,
        .elf=host->boot_reference,.elf_bytes=host->boot_reference_bytes};
    CHECK(!gh_game_event_play_start_00296458(&play,1,&frontier));
    CHECK(host->event.fields.data_number==1 && (host->event.fields.flags&1));
    /* The fixture starts at the output of the two existing stages. */
    host->event.fields.state=2;
    host->event.fields.phase=0;
    size_t heap_before=gh_heap_used(host->heap);
    CHECK(gh_game_event_update_00296530(&host->event)==GH_EVENT_CALLEE);
    CHECK(host->event.frontier==0x2d1900 && host->event.callee_error==GH_EVENT_UNSUPPORTED);
    CHECK(host->event_module.requested && host->event_module.file_bytes==80431);
    CHECK(host->event_create.substep==2 && host->event_create.bank==52);
    CHECK(!host->event_link.bound && !host->event_resource.requested);
    uint32_t job=host->event_module.job, id_counter=host->dvd.id_counter;
    CHECK(job && host->event.fields.load_handle==job && gh_dvd_check(&host->dvd,job));
    if(finish_io){
        for(unsigned i=0;i<8 && gh_dvd_check(&host->dvd,job);++i)CHECK(!gh_dvd_pump(&host->dvd));
        CHECK(!gh_dvd_check(&host->dvd,job));
    }
    CHECK(gh_game_event_update_00296530(&host->event)==GH_EVENT_CALLEE);
    CHECK(host->event.frontier==0x2d1900 && host->event_module.job==job && host->dvd.id_counter==id_counter);
    CHECK(host->event.fields.state==2 && host->event.fields.phase==0 && (host->event.fields.flags&1));
    uint32_t resource_job=0;
    if(finish_io){
        /* Exercise later I/O providers separately from the blocked original call. */
        int loaded=0;
        CHECK(!gh_game_event_module_poll_00297428(&host->event_module,&loaded) && loaded);
        CHECK(!event_module_link(host,&frontier) && !frontier && !host->event_link_relative);
        CHECK(host->event_link.bound && host->event_link.constructor_calls==1 && host->event_link.module==&host->event_module);
        CHECK(!event_module_link(host,&frontier) && host->event_link.constructor_calls==1);
        CHECK(!(host->event.fields.flags&0x80));
        CHECK(!gh_game_event_resource_begin_00297540(&host->event_resource,&host->event,&host->dvd,host->heap));
        resource_job=host->event_resource.job;
        CHECK(resource_job && resource_job!=job && host->event_resource.file_bytes==873872);
        CHECK(gh_dvd_check(&host->dvd,resource_job));
    }
    CHECK(!event_abort_resources(host) && !gh_dvd_check(&host->dvd,job));
    CHECK(!resource_job || !gh_dvd_check(&host->dvd,resource_job));
    CHECK(gh_heap_used(host->heap)==heap_before && !host->event_module.allocation.pointer && !host->event_link.bound);
    CHECK(!event_abort_resources(host));
    host->event_create=(gh_event_create_state){0};
}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    gh_startup_host *host=calloc(1,sizeof(*host));
    CHECK(host && !gh_disc_open(argv[1],&host->disc));
    size_t event_index;
    gh_disc_entry entry;
    CHECK(!gh_disc_find(host->disc,"event/ev01.dat",&event_index));
    CHECK(!gh_disc_entry_at(host->disc,event_index,&entry) && entry.size==873872);
    host->boot_reference_bytes=(size_t)gh_disc_boot_elf_size(host->disc);
    host->boot_reference=malloc(host->boot_reference_bytes);
    CHECK(host->boot_reference && !gh_disc_read_boot_elf(host->disc,0,host->boot_reference,host->boot_reference_bytes));
    CHECK(!gh_display_state_from_elf(&host->display,host->boot_reference,host->boot_reference_bytes));
    CHECK(!gh_display_static_initialize_002a48a8(&host->display,1,0xffff));
    CHECK(!gh_heap_create(2*1024*1024,&host->heap));
    gh_dvd_jobs_init(&host->dvd,host->disc);
    int32_t native;
    gh_se_mix mix;
    CHECK(!gh_audio_registry_create(&host->audio) && !gh_audio_startup_load(host->audio,host->disc));
    CHECK(!gh_audio_output_create(&host->audio_output,&native) && !gh_audio_output_set_gain(host->audio_output,0));
    CHECK(!gh_se_mix_boot_defaults_002c9e78(&mix,0x4000,GH_SE_DSP_NATIVE_LINEAR_REFERENCE));
    CHECK(!gh_se_voice_create(host->audio,host->audio_output,&mix,&host->voices));
    verify_event_prefix(host,0);
    verify_event_prefix(host,1);
    gh_se_voice_destroy(host->voices);
    gh_audio_output_destroy(host->audio_output);
    gh_audio_registry_destroy(host->audio);
    CHECK(!gh_heap_destroy(host->heap));
    gh_disc_close(host->disc);
    free(host->boot_reference);
    free(host);
    puts("PASS ev01 native host stage2: stable 2D1900, one DVD job, pending/completed cleanup");
    return 0;
}
