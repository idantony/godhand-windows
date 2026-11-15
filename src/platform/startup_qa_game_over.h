#ifndef GH_STARTUP_QA_GAME_OVER_H
#define GH_STARTUP_QA_GAME_OVER_H

#include "startup_host.h"

struct QAGameOverRetryProbe {
    enum class Capture { none, menu, reloaded };
    bool enabled=false,confirm_edge=false;
    uint16_t room=0,player_hp=0;
    uint32_t continues_before=0,continues_after=0;
    uint64_t menu_scene=0,menu_epoch=0,press_tick=0,continue_tick=0,handoff_tick=0;
    uint64_t first_reloaded_scene=0,reloaded_scene=0,reloaded_epoch=0;

    Capture presented(const gh_startup_snapshot &s,gh_qa_game_over_state game_over,bool visible)
    {
        if(!enabled||s.phase!=GH_HOST_RUNNING||s.game_state!=4||!s.scene_frame_ready||
           !s.scene_frame_serial)return Capture::none;
        if(!menu_scene&&(s.current_room==0x100||s.current_room==0x105)&&
           game_over.retry_ready&&s.player_pose_ready&&!s.player_hp&&s.player_state[0]==2){
            room=s.current_room;
            menu_scene=s.scene_frame_serial;
            menu_epoch=s.ui_resource_epoch;
            continues_before=game_over.continue_count;
            return Capture::menu;
        }
        if(!handoff_tick||reloaded_scene||s.current_room!=room||
           s.scene_frame_serial<=menu_scene||s.ui_resource_epoch<=menu_epoch||
           !s.player_pose_ready||!s.player_hp)return Capture::none;
        if(!first_reloaded_scene)first_reloaded_scene=s.scene_frame_serial;
        if(!visible)return Capture::none;
        reloaded_scene=s.scene_frame_serial;
        reloaded_epoch=s.ui_resource_epoch;
        player_hp=s.player_hp;
        return Capture::reloaded;
    }

    bool wants_confirm(const gh_startup_snapshot &s,gh_qa_game_over_state game_over) const
    {
        return menu_scene&&!press_tick&&game_over.retry_ready&&s.game_state==4&&
            s.current_room==room&&s.scene_frame_serial==menu_scene&&s.ui_resource_epoch==menu_epoch;
    }

    void record_confirm(uint64_t tick,const gh_native_input_snapshot &input)
    {
        press_tick=tick;
        confirm_edge=(input.raw_pressed&0x40u)&&(input.pressed&0x10000000u);
    }

    void observe(const gh_startup_snapshot &s,gh_qa_game_over_state game_over)
    {
        if(!confirm_edge||s.ticks<=press_tick)return;
        if(!continue_tick&&game_over.continue_count==continues_before+1u){
            continue_tick=s.ticks;
            continues_after=game_over.continue_count;
        }
        if(!handoff_tick&&continue_tick&&s.game_state==5&&s.current_room==room&&s.next_room==room)
            handoff_tick=s.ticks;
    }
};
#endif
