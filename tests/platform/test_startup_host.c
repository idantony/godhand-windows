#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../../src/platform/startup_host.h"
#include <stdio.h>
#include <stdlib.h>
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
int main(int argc,char **argv)
{
    gh_startup_host *h=0;gh_startup_snapshot s;gh_font_glyph glyph;unsigned i;
    gh_sound_span programs,system,samples,afd;
    uint32_t core_flags,pl00_flags,bgm_flags;
    CHECK(argc==2);
    CHECK(gh_startup_host_create(argv[1],-1,0,4*1024*1024,&h)==GH_HOST_ARGUMENT && !h);
    CHECK(gh_startup_host_create(argv[1],1,0,4*1024*1024,&h)==GH_HOST_OK);
    CHECK(gh_startup_host_snapshot(h,&s)==GH_HOST_OK && s.phase==GH_HOST_RUNNING && !s.ticks);
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
