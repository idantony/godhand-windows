#ifndef GH_STARTUP_QA_R102_PUNCH_H
#define GH_STARTUP_QA_R102_PUNCH_H

#include "startup_host.h"
#include <cstdio>
#include <cmath>

struct QAR102PunchProbe {
    static constexpr uint64_t steer_start=180;
    static constexpr uint64_t earliest_press=180;
    static constexpr uint64_t capture_count=32;
    bool enabled=false;
    uint64_t hit_scene=0,press_room_frame=0;

    bool applies(const gh_startup_snapshot &s) const
    {
        return enabled&&s.current_room==0x102&&s.game_state==4&&s.player_pose_ready;
    }

    gh_native_gamepad_sample sample(uint64_t room_frames) const
    {
        gh_native_gamepad_sample pad={};
        pad.connected=1;
        if(room_frames>=8&&room_frames<180)pad.left_y=32767;
        if(room_frames==184)pad.buttons=0x4000;
        return pad;
    }

    gh_native_gamepad_sample sample_live(const gh_startup_host *host,
                                         const gh_startup_snapshot &s,uint64_t room_frames)
    {
        gh_native_gamepad_sample pad={};
        pad.connected=1;
        if(room_frames<steer_start){
            if(room_frames>=8)pad.left_y=32767;
            return pad;
        }
        if(press_room_frame||s.player_state[0]!=0)return pad;
        const gh_qa_enemy_status actor=gh_startup_host_qa_nearest_em00(host,s.player_world_position);
        if(!actor.ready)return pad;
        const float dx=actor.position[0]-s.player_world_position[0];
        const float dy=actor.position[1]-s.player_world_position[1];
        const float dz=actor.position[2]-s.player_world_position[2];
        const float radius2=dx*dx+dz*dz;
        const float turn=std::remainder(std::atan2(dx,dz)-s.player_yaw,6.2831855f);
        if(!std::isfinite(radius2)||!std::isfinite(turn)||std::fabs(dy)>1.25f)return pad;
        if(room_frames%8==0&&room_frames!=last_steer_log){
            std::fprintf(stderr,"QA_R102_STEER frame=%llu tick=%llu target=%u stamp=%u hp=%u distance=%.3f turn=%.3f\n",
                (unsigned long long)room_frames,(unsigned long long)s.ticks,(unsigned)actor.handle,
                (unsigned)actor.stamp,(unsigned)actor.hp,(double)std::sqrt(radius2),(double)turn);
            last_steer_log=room_frames;
        }
        if(radius2<=2.25f*2.25f&&std::fabs(turn)<=2.15f){
            if(room_frames>=earliest_press){
                target=actor.handle;
                stamp=actor.stamp;
                before=actor.hp;
                press_room_frame=room_frames;
                pad.buttons=0x4000;
            }
            return pad;
        }
        return pad;
    }

    void observe(const gh_startup_host *host,const gh_startup_snapshot &s,
                 const gh_native_input_snapshot &controls)
    {
        if(!applies(s))return;
        if(!press_tick&&(controls.raw_pressed&0x80u))press_tick=s.ticks;
        if(!press_tick||hit_tick)return;
        const uint32_t handle=s.player_attack_target?s.player_attack_target:target;
        if(!handle)return;
        const gh_qa_enemy_status actor=gh_startup_host_qa_enemy_status(host,handle);
        if(!actor.ready)return;
        if(target!=handle){
            target=handle;
            stamp=actor.stamp;
            before=actor.hp;
            return;
        }
        if(actor.hp>=before||actor.state!=1)return;
        after=actor.hp;
        hit_tick=s.ticks;
        reaction=actor.substate;
        phase=actor.phase;
    }

    void presented(uint64_t scene)
    {
        if(hit_tick&&!hit_scene)hit_scene=scene;
    }

    void report(const char *marker) const
    {
        if(!enabled)return;
        std::fprintf(stderr,"%s {\"press_tick\":%llu,\"press_room_frame\":%llu,\"hit_tick\":%llu,\"hit_scene\":%llu,"
            "\"target\":%u,\"stamp\":%u,\"hp_before\":%u,\"hp_after\":%u,"
            "\"reaction\":%u,\"reaction_phase\":%u}\n",marker,(unsigned long long)press_tick,
            (unsigned long long)press_room_frame,(unsigned long long)hit_tick,
            (unsigned long long)hit_scene,(unsigned)target,
            (unsigned)stamp,(unsigned)before,(unsigned)after,(unsigned)reaction,(unsigned)phase);
    }

private:
    uint64_t press_tick=0,hit_tick=0,last_steer_log=0;
    uint32_t target=0;
    uint16_t stamp=0,before=0,after=0;
    uint8_t reaction=0,phase=0;
};

#endif
