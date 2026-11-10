#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../../src/platform/startup_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct other_fiber {
    gh_startup_host *host;void *owner;gh_host_error destroyed;gh_host_phase ticked;
} other_fiber;
static VOID WINAPI foreign_call(void *context)
{
    other_fiber *f=context;
    f->destroyed=gh_startup_host_destroy(f->host);
    f->ticked=gh_startup_host_tick(f->host);
    SwitchToFiber(f->owner);abort();
}
/* Scheduling/lifetime fixture only. Production gets actual GPU consumers;
 * these callbacks do not constitute a successful rendering test. */
typedef struct frame_fixture {unsigned begins,jobs;} frame_fixture;
static gh_id_error fixture_begin(void *context){frame_fixture *f=context;++f->begins;return GH_ID_OK;}
static gh_id_error fixture_draw(void *context,const gh_id_draw_job *job){frame_fixture *f=context;CHECK(job && job->entry);++f->jobs;return GH_ID_OK;}
typedef struct fade_publication_fixture {
    gh_startup_host *host;
    unsigned begins,jobs,covers,fans,visible_calls;
    gh_startup_snapshot at_failure;
} fade_publication_fixture;
static gh_id_error fade_scene_begin(void *context,uint32_t *frontier)
{fade_publication_fixture *f=context;(void)frontier;++f->begins;return GH_ID_OK;}
static gh_id_error fade_scene_draw(void *context,const gh_id_base *base,const gh_id_draw_job *job,uint32_t *frontier)
{fade_publication_fixture *f=context;(void)frontier;CHECK(base&&job&&job->entry&&job->entry_index<base->entry_count);++f->jobs;return GH_ID_OK;}
static gh_id_error fade_scene_cover(void *context,const gh_child_cover_quad *quad,uint32_t *frontier)
{fade_publication_fixture *f=context;(void)frontier;CHECK(quad&&quad->bank==6&&quad->depth==5);++f->covers;return GH_ID_OK;}
static gh_id_error fade_scene_fan(void *context,const gh_cockpit_fan *fan,uint32_t *frontier)
{fade_publication_fixture *f=context;(void)frontier;CHECK(fan&&fan->ready);++f->fans;return GH_ID_OK;}
static gh_id_error fail_second_visible_fade(void *context,const gh_fade_draw *draw)
{
    fade_publication_fixture *f=context;
    CHECK(draw&&draw->visible&&draw->w&&draw->h);
    ++f->visible_calls;
    if(f->visible_calls!=2)return GH_ID_OK;
    CHECK(gh_startup_host_snapshot(f->host,&f->at_failure)==GH_HOST_OK);
    return GH_ID_UNSUPPORTED;
}
static void fade_publication_regression(const char *iso)
{
    gh_startup_host *host=NULL;gh_startup_snapshot before={0},after={0};
    gh_startup_continuation continuation={1,1,GH_SE_DSP_NATIVE_LINEAR_REFERENCE,NULL,NULL,NULL};
    fade_publication_fixture fixture={0};
    gh_host_scene_ui_callbacks callbacks={&fixture,fade_scene_begin,fade_scene_draw,fade_scene_fan,fade_scene_cover};
    /* The original negative startup branch reaches actual R020 Init/Move
     * without a Movie codec. These are scheduling/fault fixtures only:
     * successful collectors do not claim GPU rendering or gameplay success.
     * All resources, title state, shared Fade and muted device audio are real.
     * No snapshot const pointer is cast away to manufacture an active Fade. */
    CHECK(gh_startup_host_create(iso,1,0x80000000u,64*1024*1024,&host)==GH_HOST_OK);
    fixture.host=host;
    CHECK(gh_startup_host_set_continuation(host,&continuation)==GH_HOST_OK);
    CHECK(gh_startup_host_set_scene_ui_consumers(host,&callbacks)==GH_HOST_OK);
    CHECK(gh_startup_host_set_fade_consumer(host,fail_second_visible_fade,&fixture)==GH_HOST_OK);
    for(unsigned tick=0;tick<300;++tick){
        gh_native_input_snapshot input={0};input.connected=1;input.focused=1;
        CHECK(gh_startup_host_set_input(host,&input)==GH_HOST_OK);
        CHECK(gh_startup_host_snapshot(host,&before)==GH_HOST_OK);
        if(gh_startup_host_tick(host)!=GH_HOST_RUNNING)break;
    }
    CHECK(gh_startup_host_snapshot(host,&after)==GH_HOST_OK);
    if(after.unavailable_entry!=0x307388)fprintf(stderr,"Fade regression actual frontier %08X phase%d title%u/%u scene%u\n",after.unavailable_entry,after.phase,after.title_mode,after.title_phase,after.scene_stage);
    CHECK(after.phase==GH_HOST_FRONTIER&&after.error==GH_HOST_ENTRY_UNAVAILABLE&&after.unavailable_entry==0x307388);
    CHECK(fixture.visible_calls==2&&fixture.begins>=2&&fixture.covers);
    /* First visible Fade was accepted and its complete frame was published.
     * The second consumer failed AFTER task work and before publication. */
    CHECK(before.scene_frame_ready&&before.scene_frame_serial>0&&before.fade_draw_serial==1);
    CHECK(!after.scene_frame_ready&&after.scene_frame_serial==before.scene_frame_serial);
    CHECK(after.fade_draw_serial==before.fade_draw_serial&&after.ui_resource_epoch==before.ui_resource_epoch);
    CHECK(!fixture.at_failure.scene_frame_ready&&fixture.at_failure.scene_frame_serial==before.scene_frame_serial);
    CHECK(fixture.at_failure.fade_draw_serial==before.fade_draw_serial);
    CHECK(after.mc_frame_serial==before.mc_frame_serial&&after.mc_frame_ready==before.mc_frame_ready);
    CHECK(gh_startup_host_tick(host)==GH_HOST_FRONTIER);
    CHECK(gh_startup_host_snapshot(host,&before)==GH_HOST_OK&&before.ticks==after.ticks&&before.scene_frame_serial==after.scene_frame_serial&&!before.scene_frame_ready&&fixture.visible_calls==2);
    CHECK(gh_startup_host_destroy(host)==GH_HOST_OK);
    puts("PASS actual R020 visible Fade failure retains prior complete frame serial, suppresses ready/publication and latches307388");
}
int main(int argc,char **argv)
{
    gh_startup_host *h=0;gh_startup_snapshot s;gh_font_glyph glyph;unsigned i;
    gh_sound_span programs,system,samples,afd;
    uint32_t core_flags,pl00_flags,bgm_flags;
    CHECK(argc==2 || (argc==3 && !strcmp(argv[2],"--hardware-audio")));
    CHECK(gh_startup_host_create(argv[1],-1,0,4*1024*1024,&h)==GH_HOST_ARGUMENT && !h);
    CHECK(gh_startup_host_create(argv[1],1,0,4*1024*1024,&h)==GH_HOST_OK);
    CHECK(gh_startup_host_snapshot(h,&s)==GH_HOST_OK && s.phase==GH_HOST_RUNNING && !s.ticks);
    CHECK(s.work && s.work->phase569==2 && !s.normal_continuation && s.movie_pool);
    {
        gh_native_input_snapshot input={0};uint64_t logical;uint32_t raw;
        input.raw_pressed=0x40;input.pressed=UINT64_C(0x8123456787654321);
        CHECK(gh_startup_host_set_input(h,&input)==GH_HOST_OK);
        memcpy(&logical,(const unsigned char*)s.work+0x1a0,8);memcpy(&raw,(const unsigned char*)s.work+8,4);
        CHECK(logical==input.pressed && raw==input.raw_pressed);
        memset(&input,0,sizeof(input));CHECK(gh_startup_host_set_input(h,&input)==GH_HOST_OK);
    }
    /* Wrong caller must not cancel a queued task or mutate the session. */
    {
        other_fiber f={h,GetCurrentFiber(),GH_HOST_OK,GH_HOST_RUNNING};
        void *fiber=CreateFiberEx(0,0,FIBER_FLAG_FLOAT_SWITCH,foreign_call,&f);
        CHECK(fiber);SwitchToFiber(fiber);DeleteFiber(fiber);
        CHECK(f.destroyed==GH_HOST_THREADS && f.ticked==GH_HOST_FAILED);
        CHECK(gh_startup_host_snapshot(h,&s)==GH_HOST_OK && !s.ticks && s.phase==GH_HOST_RUNNING);
    }
    for(i=0;i<30 && gh_startup_host_tick(h)==GH_HOST_RUNNING;++i) {}
    CHECK(gh_startup_host_snapshot(h,&s)==GH_HOST_OK);
    CHECK(s.phase==GH_HOST_FRONTIER && s.error==GH_HOST_ENTRY_UNAVAILABLE);
    CHECK(s.unavailable_entry==0x2ab0b8 && s.failed_operation==GH_START_OP_COUNT);
    CHECK(!s.frontier_module && s.mc_ui && s.mc_ui->initialized && s.mc_ui->entry_count==67);
    CHECK(s.mc_ui->frame==1 && s.mc_ui->entries[0].message==0x2007);
    CHECK(s.mc_messages && s.mc_messages->initialized && s.mc_messages->cursor.bytes);
    CHECK(s.startup_returned && s.ticks==8 && s.loaded_bundles==2);
    CHECK((s.flags&0xc4000)==0xc4000);
    CHECK(s.font->slots[0].bound && s.font->slots[0].width==512);
    CHECK(gh_font_glyph_at(s.font,1,&glyph)==GH_FONT_OK && glyph.width==26);
    CHECK(s.menu->tables_bound && s.menu->timer==900 && s.menu->state==0);
    CHECK(s.id && s.id->initialized && s.id->pair[1].bound && s.id->pair[11].bound);
    CHECK(s.id->job_count==1 && s.id->pool_used==67);
    CHECK(gh_audio_bank_views(s.audio,0,&programs,&system,&samples,&afd)==GH_AUDIO_OK);
    CHECK(programs.data && system.data && samples.data);
    CHECK(gh_audio_bank_views(s.audio,3,&programs,&system,&samples,&afd)==GH_AUDIO_OK);
    CHECK(programs.data && system.data && samples.data);
    CHECK(gh_audio_startup_ready(s.audio,&core_flags,&pl00_flags,&bgm_flags));
    CHECK(core_flags==0x7f && pl00_flags==0x7f && bgm_flags==0xf);
    CHECK(gh_startup_host_tick(h)==GH_HOST_FRONTIER);
    CHECK(gh_startup_host_destroy(h)==GH_HOST_OK);
    if(argc==3){
        gh_startup_continuation c={1,1,GH_SE_DSP_NATIVE_LINEAR_REFERENCE,0,0,0};
        frame_fixture fixture={0};unsigned inputs=0;
        CHECK(gh_startup_host_create_with_profile(argv[1],"G:\\middle_thing\\godhand_windows\\build\\startup-normal-host\\absent-profile",1,0,64*1024*1024,&h)==GH_HOST_OK);
        CHECK(gh_startup_host_set_continuation(h,&c)==GH_HOST_OK);
        CHECK(gh_startup_host_set_mc_consumers(h,fixture_draw,&fixture,0,0)==GH_HOST_OK);
        CHECK(gh_startup_host_set_mc_frame_begin(h,fixture_begin,&fixture)==GH_HOST_OK);
        for(i=0;i<100;++i){
            gh_native_input_snapshot input={0};CHECK(gh_startup_host_snapshot(h,&s)==GH_HOST_OK);
            if(s.mc_ui_live && s.mc_phase==3){
                if(s.mc_subphase==1 && !inputs){input.pressed=0x10000000;++inputs;}
                else if(s.mc_subphase==2 && inputs==1){input.repeat_menu=0x04000000;++inputs;}
                else if(s.mc_subphase==2 && inputs==2){input.pressed=0x10000000;++inputs;}
            }
            CHECK(gh_startup_host_set_input(h,&input)==GH_HOST_OK);
            if(gh_startup_host_tick(h)!=GH_HOST_RUNNING)break;
        }
        CHECK(gh_startup_host_snapshot(h,&s)==GH_HOST_OK && s.phase==GH_HOST_FRONTIER && s.unavailable_entry==0x2b4be8);
        CHECK(inputs==3 && s.mc_done && !s.mc_ui_live && s.mc_ui_epoch==2 && !s.mc_frame_ready);
        CHECK(s.mc_frame_serial==fixture.begins && s.mc_frame_serial>=5 && fixture.jobs>0);
        CHECK(s.id->pool_used==0 && s.id->job_count==0 && s.sound_control && s.normal_continuation);
        CHECK(gh_startup_host_set_continuation(h,&c)==GH_HOST_ARGUMENT);
        CHECK(gh_startup_host_destroy(h)==GH_HOST_OK);
        /* True alternate branch through Main/services and real assets; no
         * fake movie/UI consumers are needed on this original branch. */
        CHECK(gh_startup_host_create(argv[1],1,0x80000000u,64*1024*1024,&h)==GH_HOST_OK);
        CHECK(gh_startup_host_set_continuation(h,&c)==GH_HOST_OK);
        for(i=0;i<300 && gh_startup_host_tick(h)==GH_HOST_RUNNING;++i){}
        CHECK(gh_startup_host_snapshot(h,&s)==GH_HOST_OK && s.phase==GH_HOST_FRONTIER);
        /* A missing frontend stops immediately, before the independent asset
         * fiber necessarily finishes. This is not the full Windows fixture. */
        CHECK(s.unavailable_entry==0x15b880 && s.game_state==4);
        CHECK(gh_startup_host_destroy(h)==GH_HOST_OK);
        fade_publication_regression(argv[1]);
    }
    /* Tear down a real suspended startup C stack with live allocations. */
    CHECK(gh_startup_host_create(argv[1],1,0,4*1024*1024,&h)==GH_HOST_OK);
    CHECK(gh_startup_host_tick(h)==GH_HOST_RUNNING);
    CHECK(gh_startup_host_snapshot(h,&s)==GH_HOST_OK && s.loaded_bundles==1);
    CHECK(gh_startup_host_destroy(h)==GH_HOST_OK);
    /* Closing at either module/ID wait cancels suspended real C stacks before
     * the prefix's allocations or typed pool can be freed. */
    for(unsigned stop=6;stop<=7;++stop) {
        CHECK(gh_startup_host_create(argv[1],1,0,4*1024*1024,&h)==GH_HOST_OK);
        for(unsigned tick=0;tick<stop;++tick)CHECK(gh_startup_host_tick(h)==GH_HOST_RUNNING);
        CHECK(gh_startup_host_snapshot(h,&s)==GH_HOST_OK && !s.mc_ui);
        CHECK(gh_startup_host_destroy(h)==GH_HOST_OK);
    }
    /* The alternate original branch must stop at its own first real entry. */
    CHECK(gh_startup_host_create(argv[1],1,0x80000000u,32*1024*1024,&h)==GH_HOST_OK);
    for(i=0;i<200 && gh_startup_host_tick(h)==GH_HOST_RUNNING;++i) {}
    CHECK(gh_startup_host_snapshot(h,&s)==GH_HOST_OK && s.phase==GH_HOST_FRONTIER);
    CHECK(s.unavailable_entry==0x140c88 && s.failed_operation==GH_START_OP_COUNT);
    CHECK(s.game_state==1 && s.game_rate==0.5f);
    CHECK(s.game_texture_banks==2);
    CHECK(s.ticks>0 && s.startup_returned);
    CHECK(gh_startup_host_destroy(h)==GH_HOST_OK);
    /* Actual host allocation failure must stop before menu/ID initialization. */
    CHECK(gh_startup_host_create(argv[1],1,0,16,&h)==GH_HOST_OK);
    CHECK(gh_startup_host_tick(h)==GH_HOST_FAILED);
    CHECK(gh_startup_host_snapshot(h,&s)==GH_HOST_OK);
    CHECK(s.failed_operation==GH_START_READ && !s.loaded_bundles && !s.menu->tables_bound && !s.id);
    CHECK(gh_startup_host_destroy(h)==GH_HOST_OK);
    puts("PASS native startup: real sound banks/DVD fiber waits/font/menu/ID, explicit next-entry frontier and teardown");
    return 0;
}
