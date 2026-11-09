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
        CHECK(s.unavailable_entry==0x2be708 && s.game_state==3 && s.game_assets_loaded);
        CHECK(gh_startup_host_destroy(h)==GH_HOST_OK);
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
    CHECK(s.game_state==1 && s.game_rate==0.5f && s.game_assets_loaded);
    CHECK(s.game_texture_banks==2);
    CHECK(s.ticks>90 && s.startup_returned);
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
