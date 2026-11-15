#include "../../src/platform/startup_qa_game_over.h"
#include <cstdio>

#define CHECK(condition) do { if(!(condition)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#condition);return 1;} } while(false)

int main()
{
    using Capture=QAGameOverRetryProbe::Capture;
    const unsigned rooms[]={0x100,0x105};
    for(unsigned room : rooms){
        QAGameOverRetryProbe retry;
        retry.enabled=true;
        gh_startup_snapshot s={};
        s.phase=GH_HOST_RUNNING;
        s.game_state=4;
        s.current_room=(uint16_t)room;
        s.next_room=(uint16_t)room;
        s.ticks=100;
        s.ui_resource_epoch=2;
        s.scene_frame_ready=1;
        s.scene_frame_serial=80;
        s.player_pose_ready=1;
        s.player_state[0]=2;
        const gh_qa_game_over_state ready={0,1}, idle={0,0}, accepted={1,0};
        CHECK(!retry.wants_confirm(s,ready)&&retry.presented(s,idle,true)==Capture::none);
        CHECK(retry.presented(s,ready,true)==Capture::menu&&retry.wants_confirm(s,ready));
        CHECK(retry.room==room&&retry.menu_scene==80&&retry.menu_epoch==2);
        auto bad_input=retry;
        gh_native_input_snapshot input={};
        input.raw_pressed=0x80;
        input.pressed=0x10000000;
        bad_input.record_confirm(s.ticks,input);
        CHECK(!bad_input.confirm_edge&&!bad_input.wants_confirm(s,ready));
        input.raw_pressed=0x40;
        retry.record_confirm(s.ticks,input);
        CHECK(retry.confirm_edge&&!retry.wants_confirm(s,ready));
        ++s.ticks;
        retry.observe(s,accepted);
        CHECK(retry.continue_tick==101&&retry.continues_after==1&&!retry.handoff_tick);
        s.game_state=5;
        s.next_room=3;
        retry.observe(s,accepted);
        CHECK(!retry.handoff_tick);
        s.next_room=(uint16_t)room;
        s.ticks=130;
        retry.observe(s,accepted);
        CHECK(retry.handoff_tick==130);
        s.game_state=4;
        s.scene_frame_serial=81;
        s.player_hp=100;
        CHECK(retry.presented(s,idle,true)==Capture::none&&!retry.first_reloaded_scene);
        s.ui_resource_epoch=3;
        CHECK(retry.presented(s,idle,false)==Capture::none&&retry.first_reloaded_scene==81);
        CHECK(!retry.reloaded_scene);
        s.scene_frame_serial=83;
        CHECK(retry.presented(s,idle,true)==Capture::reloaded&&retry.reloaded_scene==83);
        CHECK(retry.reloaded_epoch==3&&retry.player_hp==100);
    }
    std::puts("PASS R100/R105 presented Game Over, single mapped Cross, exact continue and visible retry ordering");
    return 0;
}
