#ifndef GH_STARTUP_QA_R100_EXIT_H
#define GH_STARTUP_QA_R100_EXIT_H
#include "startup_host.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

struct QAR100ExitProbe {
    uint64_t door_tick=0,continue_scene=0,state5_tick=0,room3_scene=0;
    unsigned door_presses=0,continue_presses=0,route_index=0;
    bool saw_a38=false,battle_gate_waiting=false;

    gh_native_gamepad_sample sample(const gh_startup_snapshot &s)
    {
        gh_native_gamepad_sample pad={};
        pad.connected=1;
        if(room3_scene||s.current_room!=0x100||s.game_state!=4)return pad;
        if(saw_a38){
            // Only act on the original menu's presented, interactive phase.
            if(s.save_map_active&&s.save_map_phase==5&&presented_menu_phase==5&&
               s.save_map_selected==presented_menu_selection){
                if(release_pad){release_pad=false;return pad;}
                if(s.save_map_selected==0)pad.buttons=0x0002; // D-pad Down.
                if(s.save_map_selected==1){
                    pad.buttons=0x1000; // Cross, mapped to original menu bit28.
                    continue_press_tick=s.ticks;
                    ++continue_presses;
                }
                release_pad=pad.buttons!=0;
            }
            return pad;
        }
        uint32_t flags=0;
        if(s.work)std::memcpy(&flags,reinterpret_cast<const uint8_t*>(s.work)+0x5d8,4);
        if(!s.work||s.event_state||s.event_motion_camera_active||!s.player_pose_ready||
           s.player_update_frontier||(flags&0x82000000u))return pad;
        if(s.r100_exit_prompt){
            if(!release_pad&&presented_prompt){
                pad.buttons=0x2000; // Circle, mapped to original action bit35.
                ++door_presses;
                release_pad=true;
            }else release_pad=false;
            return pad;
        }
        release_pad=false;
        if(s.player_state[0]||
           (s.player_state[1]&&s.player_state_entry!=0x10c300u&&s.player_state_entry!=0x10c5a8u))
            return pad;
        // Actual R100 floor and clearance rays connect this east route once
        // the original REL1BD0 battle opens H03_gt. Waypoint 3 is inside AT10.
        static constexpr float route[][2]={{3,16},{3,6},{3,1},{3,-4},
            {6,-9},{9,-14},{9,-29},{8,-34},{6,-39},{4,-44},{3,-49},
            {3,-74},{2.60495f,-79.42900f}};
        constexpr unsigned at10=3;
        const bool battle_committed=(s.r100_scenario_flags&0x40000000u)!=0;
        battle_gate_waiting=false;
        const float x=s.player_world_position[0],z=s.player_world_position[2];
        if(!std::isfinite(x)||!std::isfinite(z)||!std::isfinite(s.player_yaw))return pad;
        constexpr unsigned count=sizeof(route)/sizeof(route[0]);
        const unsigned last=battle_committed?count-1:at10;
        while(route_index<last){
            const float dx=route[route_index][0]-x,dz=route[route_index][1]-z;
            if(dx*dx+dz*dz>1.0f&&z>route[route_index][1]+0.45f)break;
            ++route_index;
        }
        const float dx=route[route_index][0]-x,dz=route[route_index][1]-z;
        if(!battle_committed&&route_index==at10&&dx*dx+dz*dz<=1.0f){
            battle_gate_waiting=true;
            return pad;
        }
        if(dx*dx+dz*dz<0.035f)return pad;
        constexpr float tau=6.28318530718f;
        const float turn=std::remainder(std::atan2(dx,dz)-s.player_yaw,tau);
        const float steering=std::clamp(turn,-1.12f,1.12f);
        const float axis_x=-std::sin(steering),axis_y=std::cos(steering);
        const float scale=(dx*dx+dz*dz>4?31000.0f:17408.0f)/std::max(std::fabs(axis_x),axis_y);
        pad.left_x=static_cast<int16_t>(axis_x*scale);
        pad.left_y=static_cast<int16_t>(axis_y*scale);
        return pad;
    }

    void observe(const gh_startup_snapshot &s,const gh_native_input_snapshot &input)
    {
        if(s.current_room==0x100&&door_presses&&(s.r100_scenario_flags&0x00400000u)&&!saw_a38){
            door_tick=s.ticks;
            saw_a38=true;
            release_pad=false;
        }
        if(saw_a38&&continue_presses&&s.ticks==continue_press_tick+1&&
           (input.pressed&UINT64_C(0x10000000)))continue_edge=true;
        if(continue_edge&&s.current_room==0x100&&s.game_state==5&&s.next_room==3&&!state5_tick)
            state5_tick=s.ticks;
        if(!s.scene_frame_ready)return;
        pending={s.scene_frame_serial,s.current_room,s.game_state,s.save_map_phase,
                 s.save_map_selected,s.save_map_active!=0,s.r100_exit_prompt!=0};
    }

    void presented(uint64_t serial)
    {
        if(pending.serial!=serial||!serial)return;
        presented_prompt=pending.room==0x100&&pending.prompt;
        presented_menu_phase=pending.room==0x100&&pending.menu?pending.menu_phase:0;
        presented_menu_selection=pending.menu_selection;
        if(presented_menu_phase==5&&presented_menu_selection==1&&!continue_scene)
            continue_scene=serial;
        if(state5_tick&&continue_scene&&pending.room==3&&pending.game_state==4&&!room3_scene)
            room3_scene=serial;
    }

private:
    struct Frame {
        uint64_t serial=0;
        uint16_t room=0;
        uint8_t game_state=0;
        uint16_t menu_phase=0;
        uint8_t menu_selection=0;
        bool menu=false,prompt=false;
    } pending;
    uint16_t presented_menu_phase=0;
    uint8_t presented_menu_selection=0;
    uint64_t continue_press_tick=0;
    bool presented_prompt=false,release_pad=false,continue_edge=false;
};
#endif
