#ifndef GH_STARTUP_QA_REPEAT_SQUARE_H
#define GH_STARTUP_QA_REPEAT_SQUARE_H

#include "startup_host.h"
#include "startup_qa_r100_exit.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

struct QARepeatSquareProbe {
    bool enabled=false,triangle=false,pair=false,continue_after_pair=false,packet_only=false;
    bool saw_event=false;
    unsigned attempts=0,edges=0,animations=0,hits=0;
    uint32_t player_handle=0,enemy_handle=0;
    uint64_t defeat_scene=0;
    QAR100ExitProbe exit;

    bool complete() const {return continue_after_pair?exit.room3_scene!=0:defeat_scene!=0;}

    gh_native_gamepad_sample sample(const gh_startup_snapshot &s)
    {
        gh_native_gamepad_sample pad={};
        if(!enabled)return pad;
        if(defeat_scene)return continue_after_pair?exit.sample(s):pad;
        pad.connected=1;
        if(s.current_room==0x100&&s.event_number==1&&s.event_state)saw_event=true;
        uint32_t flags=0;
        if(s.work)std::memcpy(&flags,reinterpret_cast<const uint8_t*>(s.work)+0x5d8,4);
        if(!saw_event||s.current_room!=0x100||s.game_state!=4||s.event_state||
           s.event_motion_camera_active||!s.player_pose_ready||s.player_update_frontier||
           !s.work||(flags&0x82000000u)||!s.player_handle||kill_scene[1])return pad;
        const Enemy enemy=selected_enemy(s);
        if(!enemy.handle||!enemy.hp)return pad;

        if(!player_handle){
            player_handle=s.player_handle;
            enemy_handle=enemy.handle;
            approach_start=s.player_updates_completed+2;
            next_square=approach_start;
        }
        if(s.player_handle!=player_handle)return pad;
        if(pair&&kill_scene[0]&&!enemy_handle){
            if(enemy.handle==kill_handle[0])return pad;
            enemy_handle=enemy.handle;
            next_square=s.player_updates_completed+3;
        }
        if(enemy.handle!=enemy_handle||
           s.player_updates_completed<approach_start)return pad;

        const uint64_t moved=s.player_updates_completed-approach_start;
        if(!kill_scene[0]&&moved<27){
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

        const float dx=enemy.position[0]-s.player_world_position[0];
        const float dy=enemy.position[1]-s.player_world_position[1];
        const float dz=enemy.position[2]-s.player_world_position[2];
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
        pad.buttons=triangle?0x8000:0x4000;
        ++attempts;
        attack_pending=true;
        press_tick=s.ticks;
        next_square=s.player_updates_completed+3;
        edge_seen=false;
        target_seen=false;
        motion_reported=false;
        std::fprintf(stderr,"QA_REPEAT_PRESS tick=%llu attempt=%u player=%u enemy=%u hp=%u xz=%.7g,%.7g distance2=%.7g\n",
            (unsigned long long)s.ticks,attempts,player_handle,enemy_handle,(unsigned)enemy.hp,
            (double)s.player_world_position[0],(double)s.player_world_position[2],(double)distance2);
        return pad;
    }

    void observe(const gh_startup_snapshot &s,const gh_native_input_snapshot &input)
    {
        if(enabled&&continue_after_pair&&defeat_scene){exit.observe(s,input);return;}
        if(!enabled||!player_handle||s.player_handle!=player_handle)return;
        if(attack_pending&&!edge_seen&&s.ticks==press_tick+1&&(input.raw_pressed&(triangle?0x10u:0x80u))){
            edge_seen=true;
            ++edges;
        }
        if(!s.scene_frame_ready||s.scene_frame_serial<=last_presented||s.current_room!=0x100)return;
        const Enemy enemy=selected_enemy(s);
        if(!kill_scene[1]&&(!enemy_handle||enemy.handle!=enemy_handle))return;
        if(attack_pending&&edge_seen&&s.player_attack_target==enemy_handle)target_seen=true;
        Frame frame={};
        frame.serial=s.scene_frame_serial;
        frame.tick=s.ticks;
        frame.attempt=attempts;
        frame.player_hp=s.player_hp;
        frame.player_state=s.player_state[0];
        frame.player_reaction=s.player_state[1];
        frame.hp=enemy.hp;
        frame.hit_kind=enemy.hit_kind;
        frame.hit_power=enemy.hit_power;
        frame.dead=enemy.state[0]==2;
        frame.placement_known=s.r100_placement_known;
        frame.placement_dead=s.r100_placement_dead;
        frame.scenario_flags=s.r100_scenario_flags;
        frame.attributed=attack_pending&&edge_seen&&target_seen;
        if(attack_pending&&edge_seen&&!motion_reported&&!s.player_update_frontier&&
           s.player_state_entry==0x10d3a8u&&!s.player_state[0]&&s.player_state[1]==0x0b&&
           s.player_state[2]==1&&s.player_attack_skill==(triangle?29u:0u)&&
           std::isfinite(s.player_motion_sample)&&s.player_motion_sample>=0){
            frame.sample=s.player_motion_sample;
            frame.motion=true;
        }
        pending=frame;
    }

    void presented(uint64_t serial)
    {
        if(enabled&&continue_after_pair&&defeat_scene){exit.presented(serial);return;}
        if(!enabled||defeat_scene||serial<=last_presented)return;
        const bool consecutive=serial-last_presented==1;
        last_presented=serial;
        if(pending.serial!=serial){
            hp_known=player_hp_known=false;
            motion_attempt=0;
            return;
        }
        if(consecutive&&player_hp_known&&pending.player_hp<last_player_hp){
            std::fprintf(stderr,"QA_PLAYER_HP_%s scene=%llu tick=%llu player=%u hp=%u->%u state=%u/%u\n",
                frame_label(),(unsigned long long)serial,(unsigned long long)pending.tick,player_handle,
                (unsigned)last_player_hp,(unsigned)pending.player_hp,(unsigned)pending.player_state,
                (unsigned)pending.player_reaction);
        }
        player_hp_known=true;
        last_player_hp=pending.player_hp;
        if(pair&&kill_scene[1]){
            if((pending.placement_known&3u)==3u&&(pending.placement_dead&3u)==3u){
                defeat_scene=serial;
                room_dead_mask=pending.placement_dead&3u;
                scenario_flags=pending.scenario_flags;
                std::fprintf(stderr,"QA_REPEAT_PAIR_%s scene=%llu stamp19_scene=%llu stamp20_scene=%llu scenario=%08x\n",
                    frame_label(),(unsigned long long)serial,(unsigned long long)kill_scene[0],
                    (unsigned long long)kill_scene[1],pending.scenario_flags);
            }
            return;
        }
        if(pending.motion&&!motion_reported&&motion_attempt==pending.attempt&&pending.sample>last_sample){
            motion_reported=true;
            ++animations;
            std::fprintf(stderr,"QA_REPEAT_ATTACK_%s scene=%llu tick=%llu attempt=%u player=%u enemy=%u sample=%.9g\n",
                frame_label(),(unsigned long long)serial,(unsigned long long)pending.tick,pending.attempt,player_handle,enemy_handle,
                (double)pending.sample);
        }
        motion_attempt=pending.motion?pending.attempt:0;
        last_sample=pending.sample;
        if(hp_known&&pending.attributed&&pending.hp<last_hp&&pending.hit_kind&&(triangle?pending.hit_power==12:pending.hit_power>0)){
            ++hits;
            std::fprintf(stderr,"QA_REPEAT_HIT_%s scene=%llu tick=%llu attempt=%u player=%u enemy=%u hp=%u->%u kind=%u power=%d\n",
                frame_label(),(unsigned long long)serial,(unsigned long long)pending.tick,pending.attempt,player_handle,enemy_handle,
                (unsigned)last_hp,(unsigned)pending.hp,(unsigned)pending.hit_kind,(int)pending.hit_power);
            if(!pending.hp)lethal_hit_scene=serial;
        }
        if(lethal_hit_scene&&!pending.hp&&pending.dead&&!defeat_scene){
            const unsigned index=kill_scene[0]?1:0;
            kill_scene[index]=serial;
            kill_handle[index]=enemy_handle;
            std::fprintf(stderr,"QA_REPEAT_DEFEAT_%s scene=%llu tick=%llu lethal_scene=%llu player=%u enemy=%u stamp=%u hp=0\n",
                frame_label(),(unsigned long long)serial,(unsigned long long)pending.tick,(unsigned long long)lethal_hit_scene,
                player_handle,enemy_handle,19+index);
            if(!pair)defeat_scene=serial;
            else{
                enemy_handle=0;
                lethal_hit_scene=0;
                attack_pending=edge_seen=target_seen=motion_reported=hp_known=false;
                motion_attempt=0;
                last_sample=0;
                return;
            }
        }
        last_hp=pending.hp;
        hp_known=true;
    }

    void report() const
    {
        if(enabled)std::fprintf(stderr,
            "QA_REPEAT_STATUS {\"saw_event\":%s,\"player\":%u,\"enemy\":%u,\"attempts\":%u,\"edges\":%u,"
            "\"animations\":%u,\"hits\":%u,\"last_hp\":%d,\"defeat_scene\":%llu,"
            "\"stamp19_scene\":%llu,\"stamp20_scene\":%llu,\"stamp19_handle\":%u,\"stamp20_handle\":%u,\"room_dead_mask\":%u,\"scenario_flags\":%u}\n",
            saw_event?"true":"false",player_handle,enemy_handle,attempts,edges,animations,hits,
            hp_known?(int)last_hp:-1,(unsigned long long)defeat_scene,
            (unsigned long long)kill_scene[0],(unsigned long long)kill_scene[1],kill_handle[0],kill_handle[1],room_dead_mask,scenario_flags);
        if(continue_after_pair)std::fprintf(stderr,
            "QA_R100_EXIT_STATUS {\"door_tick\":%llu,\"continue_scene\":%llu,\"state5_tick\":%llu,"
            "\"room3_scene\":%llu,\"door_presses\":%u,\"continue_presses\":%u,\"route_index\":%u,\"battle_gate_waiting\":%u}\n",
            (unsigned long long)exit.door_tick,(unsigned long long)exit.continue_scene,
            (unsigned long long)exit.state5_tick,(unsigned long long)exit.room3_scene,
            exit.door_presses,exit.continue_presses,exit.route_index,(unsigned)exit.battle_gate_waiting);
    }

private:
    const char *frame_label() const {return packet_only?"PACKET_COMPLETE":"PRESENTED";}

    struct Enemy {
        uint32_t handle;
        uint16_t hp,hit_kind;
        int32_t hit_power;
        const float *position;
        const uint8_t *state;
    };
    Enemy selected_enemy(const gh_startup_snapshot &s) const
    {
        if(pair&&kill_scene[0])return {s.r100_second_enemy_handle,s.r100_second_enemy_hp,
            s.r100_second_enemy_player_hit_kind,s.r100_second_enemy_player_hit_power,
            s.r100_second_enemy_position,s.r100_second_enemy_state};
        return {s.r100_first_enemy_handle,s.r100_first_enemy_hp,s.r100_first_enemy_player_hit_kind,
            s.r100_first_enemy_player_hit_power,s.r100_first_enemy_position,s.r100_first_enemy_state};
    }

    struct Frame {
        uint64_t serial=0,tick=0;
        unsigned attempt=0;
        uint32_t scenario_flags=0;
        uint8_t placement_known=0,placement_dead=0,player_state=0,player_reaction=0;
        uint16_t hp=0,hit_kind=0,player_hp=0;
        int32_t hit_power=0;
        float sample=0;
        bool dead=false,attributed=false,motion=false;
    } pending;

    uint64_t approach_start=0,next_square=0,press_tick=0,last_presented=0,lethal_hit_scene=0;
    uint64_t kill_scene[2]={};
    uint32_t kill_handle[2]={},room_dead_mask=0,scenario_flags=0;
    uint16_t last_hp=0,last_player_hp=0;
    float last_sample=0;
    unsigned motion_attempt=0;
    bool attack_pending=false,edge_seen=false,target_seen=false,motion_reported=false,hp_known=false,player_hp_known=false;
};

#endif
