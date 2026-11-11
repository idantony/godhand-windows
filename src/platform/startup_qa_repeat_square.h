#ifndef GH_STARTUP_QA_REPEAT_SQUARE_H
#define GH_STARTUP_QA_REPEAT_SQUARE_H

#include "startup_host.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

struct QARepeatSquareProbe {
    bool enabled=false;
    bool saw_event=false;
    unsigned attempts=0,edges=0,animations=0,hits=0;
    uint32_t player_handle=0,enemy_handle=0;
    uint64_t defeat_scene=0;

    gh_native_gamepad_sample sample(const gh_startup_snapshot &s)
    {
        gh_native_gamepad_sample pad={};
        if(!enabled||defeat_scene)return pad;
        pad.connected=1;
        if(s.current_room==0x100&&s.event_number==1&&s.event_state)saw_event=true;
        uint32_t flags=0;
        if(s.work)std::memcpy(&flags,reinterpret_cast<const uint8_t*>(s.work)+0x5d8,4);
        if(!saw_event||s.current_room!=0x100||s.game_state!=4||s.event_state||
           s.event_motion_camera_active||!s.player_pose_ready||s.player_update_frontier||
           !s.work||(flags&0x82000000u)||!s.player_handle||
           !s.r100_first_enemy_handle||!s.r100_first_enemy_hp)return pad;

        if(!player_handle){
            player_handle=s.player_handle;
            enemy_handle=s.r100_first_enemy_handle;
            approach_start=s.player_updates_completed+2;
            next_square=approach_start;
        }
        if(s.player_handle!=player_handle||s.r100_first_enemy_handle!=enemy_handle||
           s.player_updates_completed<approach_start)return pad;

        const uint64_t moved=s.player_updates_completed-approach_start;
        if(moved<27){
            // Same original pad route as approach-square, indexed by completed Move.
            pad.left_x=moved<7?28000:0;
            pad.left_y=moved<7?18000:32767;
            next_square=s.player_updates_completed+3;
            return pad;
        }

        const bool idle=!s.player_state[0]&&!s.player_state[1]&&!s.player_square_buffer;
        if(attack_pending){
            if(!idle||s.player_updates_completed<next_square)return pad;
            attack_pending=false;
        }

        const float dx=s.r100_first_enemy_position[0]-s.player_world_position[0];
        const float dy=s.r100_first_enemy_position[1]-s.player_world_position[1];
        const float dz=s.r100_first_enemy_position[2]-s.player_world_position[2];
        const float distance2=dx*dx+dz*dz;
        if(!std::isfinite(distance2)||!std::isfinite(dy)||!std::isfinite(s.player_yaw)||
           std::fabs(dy)>0.75f)return pad;
        constexpr float tau=6.28318530718f;
        const float turn=distance2>0.0001f?std::remainder(std::atan2(dx,dz)-s.player_yaw,tau):0;
        const bool reachable=distance2<=1.18f*1.18f&&(!attempts||std::fabs(turn)<=0.70f);
        if(!reachable){
            if(!idle&&s.player_state_entry!=0x10c300u&&s.player_state_entry!=0x10c5a8u)return pad;
            float steering=0;
            if(std::fabs(turn)>0.30f)
                steering=std::copysign(std::clamp(std::fabs(turn),0.65f,1.15f),turn);
            const float x=-std::sin(steering),y=std::cos(steering);
            // Keep the dominant axis above the original per-axis deadzone.
            const float scale=17408.0f/std::max(std::fabs(x),y);
            pad.left_x=static_cast<int16_t>(x*scale);
            pad.left_y=static_cast<int16_t>(y*scale);
            next_square=s.player_updates_completed+3;
            return pad;
        }
        if(!idle||s.player_updates_completed<next_square)return pad;
        pad.buttons=0x4000;
        ++attempts;
        attack_pending=true;
        press_tick=s.ticks;
        next_square=s.player_updates_completed+3;
        edge_seen=false;
        target_seen=false;
        motion_reported=false;
        std::fprintf(stderr,"QA_REPEAT_PRESS tick=%llu attempt=%u player=%u enemy=%u hp=%u xz=%.7g,%.7g distance2=%.7g\n",
            (unsigned long long)s.ticks,attempts,player_handle,enemy_handle,(unsigned)s.r100_first_enemy_hp,
            (double)s.player_world_position[0],(double)s.player_world_position[2],(double)distance2);
        return pad;
    }

    void observe(const gh_startup_snapshot &s,const gh_native_input_snapshot &input)
    {
        if(!enabled||!player_handle||s.player_handle!=player_handle)return;
        if(attack_pending&&!edge_seen&&s.ticks==press_tick+1&&(input.raw_pressed&0x80u)){
            edge_seen=true;
            ++edges;
        }
        if(!s.scene_frame_ready||s.scene_frame_serial<=last_presented||s.current_room!=0x100||
           s.r100_first_enemy_handle!=enemy_handle)return;
        if(attack_pending&&edge_seen&&s.player_attack_target==enemy_handle)target_seen=true;
        Frame frame={};
        frame.serial=s.scene_frame_serial;
        frame.tick=s.ticks;
        frame.attempt=attempts;
        frame.hp=s.r100_first_enemy_hp;
        frame.hit_kind=s.r100_first_enemy_player_hit_kind;
        frame.hit_power=s.r100_first_enemy_player_hit_power;
        frame.dead=s.r100_first_enemy_state[0]==2;
        frame.attributed=attack_pending&&edge_seen&&target_seen;
        if(attack_pending&&edge_seen&&!motion_reported&&!s.player_update_frontier&&
           s.player_state_entry==0x10d3a8u&&!s.player_state[0]&&s.player_state[1]==0x0b&&
           s.player_state[2]==1&&std::isfinite(s.player_motion_sample)&&s.player_motion_sample>=0){
            frame.sample=s.player_motion_sample;
            frame.motion=true;
        }
        pending=frame;
    }

    void presented(uint64_t serial)
    {
        if(!enabled||serial<=last_presented)return;
        last_presented=serial;
        if(pending.serial!=serial){
            hp_known=false;
            motion_attempt=0;
            return;
        }
        if(pending.motion&&!motion_reported&&motion_attempt==pending.attempt&&pending.sample>last_sample){
            motion_reported=true;
            ++animations;
            std::fprintf(stderr,"QA_REPEAT_ATTACK_PRESENTED scene=%llu tick=%llu attempt=%u player=%u enemy=%u sample=%.9g\n",
                (unsigned long long)serial,(unsigned long long)pending.tick,pending.attempt,player_handle,enemy_handle,
                (double)pending.sample);
        }
        motion_attempt=pending.motion?pending.attempt:0;
        last_sample=pending.sample;
        if(hp_known&&pending.attributed&&pending.hp<last_hp&&pending.hit_kind&&pending.hit_power>0){
            ++hits;
            std::fprintf(stderr,"QA_REPEAT_HIT_PRESENTED scene=%llu tick=%llu attempt=%u player=%u enemy=%u hp=%u->%u kind=%u power=%d\n",
                (unsigned long long)serial,(unsigned long long)pending.tick,pending.attempt,player_handle,enemy_handle,
                (unsigned)last_hp,(unsigned)pending.hp,(unsigned)pending.hit_kind,(int)pending.hit_power);
            if(!pending.hp)lethal_hit_scene=serial;
        }
        if(lethal_hit_scene&&!pending.hp&&pending.dead&&!defeat_scene){
            defeat_scene=serial;
            std::fprintf(stderr,"QA_REPEAT_DEFEAT_PRESENTED scene=%llu tick=%llu lethal_scene=%llu player=%u enemy=%u stamp=19 hp=0\n",
                (unsigned long long)serial,(unsigned long long)pending.tick,(unsigned long long)lethal_hit_scene,
                player_handle,enemy_handle);
        }
        last_hp=pending.hp;
        hp_known=true;
    }

    void report() const
    {
        if(enabled)std::fprintf(stderr,
            "QA_REPEAT_STATUS {\"saw_event\":%s,\"player\":%u,\"enemy\":%u,\"attempts\":%u,\"edges\":%u,"
            "\"animations\":%u,\"hits\":%u,\"last_hp\":%d,\"defeat_scene\":%llu}\n",
            saw_event?"true":"false",player_handle,enemy_handle,attempts,edges,animations,hits,
            hp_known?(int)last_hp:-1,(unsigned long long)defeat_scene);
    }

private:
    struct Frame {
        uint64_t serial=0,tick=0;
        unsigned attempt=0;
        uint16_t hp=0,hit_kind=0;
        int32_t hit_power=0;
        float sample=0;
        bool dead=false,attributed=false,motion=false;
    } pending;

    uint64_t approach_start=0,next_square=0,press_tick=0,last_presented=0,lethal_hit_scene=0;
    uint16_t last_hp=0;
    float last_sample=0;
    unsigned motion_attempt=0;
    bool attack_pending=false,edge_seen=false,target_seen=false,motion_reported=false,hp_known=false;
};

#endif
