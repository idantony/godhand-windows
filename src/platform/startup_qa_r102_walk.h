#ifndef GH_STARTUP_QA_R102_WALK_H
#define GH_STARTUP_QA_R102_WALK_H

#include "startup_host.h"
#include <cstdio>

struct QAR102WalkProbe {
    bool enabled=false;
    bool complete=false;

    bool applies(const gh_startup_snapshot &snapshot) const
    {
        return enabled&&snapshot.current_room==0x102&&snapshot.game_state==4;
    }

    gh_native_gamepad_sample sample(const gh_startup_snapshot &snapshot,uint64_t room_frames)
    {
        gh_native_gamepad_sample pad={};
        if(!applies(snapshot)||!snapshot.player_pose_ready||room_frames<8||inputs==24)
            return pad;
        if(!inputs){
            start_room_frame=room_frames;
            start_scene=snapshot.scene_frame_serial;
            player=snapshot.player_handle;
            start_hp=snapshot.player_hp;
            start_x=previous_x=snapshot.player_world_position[0];
            start_z=previous_z=snapshot.player_world_position[2];
        }
        pad.connected=1;
        pad.left_y=32767;
        ++inputs;
        return pad;
    }

    void observe_frame(const gh_startup_snapshot &snapshot,uint64_t scene,uint64_t room_frames)
    {
        if(!applies(snapshot)||!inputs||!snapshot.player_pose_ready||
           snapshot.player_handle!=player||snapshot.scene_frame_serial!=scene||
           scene<=start_scene||scene<=last_scene)return;
        last_scene=scene;
        end_hp=snapshot.player_hp;
        const float x=snapshot.player_world_position[0],z=snapshot.player_world_position[2];
        if(x!=previous_x||z!=previous_z)++moving_frames;
        previous_x=x;
        previous_z=z;
        const float dx=x-start_x,dz=z-start_z;
        distance_sq=dx*dx+dz*dz;
        complete=inputs==24&&room_frames>=start_room_frame+28&&
            moving_frames>=3&&distance_sq>=0.0625f&&end_hp==start_hp;
    }

    void report(const char *marker,uint64_t room_frames) const
    {
        if(!enabled)return;
        std::fprintf(stderr,"%s {\"inputs\":%u,\"first_scene\":%llu,\"last_scene\":%llu,"
            "\"first_room_frame\":%llu,\"last_room_frame\":%llu,\"moving_frames\":%u,"
            "\"start_xz\":[%.6f,%.6f],\"end_xz\":[%.6f,%.6f],\"distance_squared\":%.6f,"
            "\"hp_before\":%u,\"hp_after\":%u,\"complete\":%s}\n",
            marker,inputs,(unsigned long long)start_scene,(unsigned long long)last_scene,
            (unsigned long long)start_room_frame,(unsigned long long)room_frames,moving_frames,
            (double)start_x,(double)start_z,(double)previous_x,(double)previous_z,
            (double)distance_sq,start_hp,end_hp,complete?"true":"false");
    }

private:
    uint64_t start_room_frame=0,start_scene=0,last_scene=0;
    uint32_t player=0;
    unsigned inputs=0,moving_frames=0,start_hp=0,end_hp=0;
    float start_x=0,start_z=0,previous_x=0,previous_z=0,distance_sq=0;
};

#endif
