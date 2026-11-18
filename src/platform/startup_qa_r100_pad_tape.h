#ifndef GH_STARTUP_QA_R100_PAD_TAPE_H
#define GH_STARTUP_QA_R100_PAD_TAPE_H

#include "startup_host.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

struct QAR100PadTape {
    bool enabled=false;
    uint64_t victory_scene=0;

    template <class Path>
    bool load(const Path &path)
    {
        std::ifstream input(path);
        std::string line;
        if(!std::getline(input,line))return false;
        const bool has_triggers=line=="tick,move,buttons,left_x,left_y,right_x,right_y,connected,left_trigger,right_trigger";
        if(!has_triggers&&line!="tick,move,buttons,left_x,left_y,right_x,right_y,connected")return false;
        const size_t columns=has_triggers?10:8;
        uint64_t last_tick=0,tick_at_move=0;
        while(std::getline(input,line)){
            std::array<int64_t,10> row={};
            if(!parse(line,row,columns)||row[0]<=0||uint64_t(row[0])<=last_tick||
               row[1]<0||row[2]<0||row[2]>65535||row[7]<0||row[7]>1||
               row[8]<0||row[8]>255||row[9]<0||row[9]>255)return false;
            for(unsigned axis=3;axis<7;++axis)
                if(row[axis]<-32768||row[axis]>32767)return false;
            last_tick=uint64_t(row[0]);
            const uint64_t move=uint64_t(row[1]);
            if(!samples.empty()&&move<samples.back().move)return false;
            const std::array<int64_t,8> fields={row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9]};
            if(!samples.empty()&&move==samples.back().move){
                ++tick_at_move;
                if(fields==samples.back().fields)continue;
            }else tick_at_move=0;
            samples.push_back({move,tick_at_move,fields});
        }
        enabled=input.eof()&&!samples.empty();
        return enabled;
    }

    gh_native_gamepad_sample sample(const gh_startup_snapshot &snapshot)
    {
        gh_native_gamepad_sample pad={};
        pad.connected=1;
        if(snapshot.player_updates_completed<4)trace_startup_random(snapshot);
        if(snapshot.current_room!=0x100||snapshot.game_state!=4)return pad;
        if(snapshot.event_number==1&&snapshot.event_state)saw_ev01=true;
        if(!saw_ev01||snapshot.event_state)return pad;
        const uint64_t move=snapshot.player_updates_completed;
        if(last_sample!=move)ticks_at_move=0;
        while(next<samples.size()&&samples[next].move<move)++next;
        if(next==samples.size()||samples[next].move!=move)return pad;
        // A soft event can pause Player Move while the input task keeps sampling.
        while(next+1<samples.size()&&samples[next+1].move==move&&
              samples[next+1].tick_at_move<=ticks_at_move)++next;
        ++ticks_at_move;
        const auto &fields=samples[next].fields;
        pad.buttons=uint16_t(fields[0]);
        pad.left_x=int16_t(fields[1]);
        pad.left_y=int16_t(fields[2]);
        pad.right_x=int16_t(fields[3]);
        pad.right_y=int16_t(fields[4]);
        pad.connected=uint8_t(fields[5]);
        pad.left_trigger=uint8_t(fields[6]);
        pad.right_trigger=uint8_t(fields[7]);
        if(last_sample!=move){
            ++sampled_moves;
            last_sample=move;
            trace(snapshot,pad);
        }
        return pad;
    }

    bool presented(const gh_startup_snapshot &snapshot,uint64_t scene)
    {
        if(victory_scene||!enabled||!saw_ev01||!sampled_moves||snapshot.current_room!=0x100||
           snapshot.game_state!=4||snapshot.event_state)return false;
        const uint32_t observed[]={snapshot.r100_first_enemy_handle,snapshot.r100_second_enemy_handle};
        const unsigned known_dead=snapshot.r100_placement_known&snapshot.r100_placement_dead&3u;
        for(unsigned slot=0;slot<2;++slot){
            if(observed[slot])handles[slot]=observed[slot];
            if((known_dead&(1u<<slot))&&handles[slot]&&!deaths[slot])deaths[slot]=scene;
        }
        if(known_dead!=3||!deaths[0]||!deaths[1]||handles[0]==handles[1]||
           !(snapshot.r100_scenario_flags&0x01000000u)||!snapshot.player_hp)return false;
        victory_scene=scene;
        room_flags=snapshot.r100_scenario_flags;
        death_mask=known_dead;
        player_hp=snapshot.player_hp;
        return true;
    }

    void report(const char *tag="QA_R100_PAD_TAPE") const
    {
        if(!enabled)return;
        std::fprintf(stderr,"%s {\"sampled_moves\":%llu,\"victory_scene\":%llu,"
            "\"stamp19_scene\":%llu,\"stamp20_scene\":%llu,\"stamp19_handle\":%u,"
            "\"stamp20_handle\":%u,\"death_mask\":%u,\"scenario_flags\":%u,\"player_hp\":%u}\n",
            tag,(unsigned long long)sampled_moves,(unsigned long long)victory_scene,
            (unsigned long long)deaths[0],(unsigned long long)deaths[1],
            handles[0],handles[1],death_mask,room_flags,player_hp);
    }

private:
    struct Sample {
        uint64_t move,tick_at_move;
        std::array<int64_t,8> fields;
    };
    std::vector<Sample> samples;
    size_t next=0;
    bool saw_ev01=false;
    uint64_t ticks_at_move=0;
    uint64_t last_sample=UINT64_MAX,sampled_moves=0,deaths[2]={};
    uint32_t handles[2]={},room_flags=0;
    unsigned death_mask=0,player_hp=0;

    static uint32_t word(const void *field)
    {
        uint32_t value;
        std::memcpy(&value,field,sizeof(value));
        return value;
    }

    static void trace(const gh_startup_snapshot &snapshot,const gh_native_gamepad_sample &pad)
    {
        const auto &position=snapshot.player_world_position;
        const auto &first=snapshot.r100_first_enemy_position;
        const auto &second=snapshot.r100_second_enemy_position;
        std::fprintf(stderr,"QA_R100_PAD_STATE {\"move\":%llu,\"tick\":%llu,\"scene\":%llu,"
            "\"pad\":[%u,%d,%d,%d,%d,%u],\"hp\":[%u,%u,%u],\"state\":[%u,%u,%u],"
            "\"position_bits\":[%u,%u,%u,%u,%u,%u,%u,%u,%u],\"yaw_bits\":%u,"
            "\"scenario\":%u,\"dead\":%u,\"target\":%u,\"skill\":%u}\n",
            (unsigned long long)snapshot.player_updates_completed,(unsigned long long)snapshot.ticks,
            (unsigned long long)snapshot.scene_frame_serial,
            unsigned(pad.buttons),int(pad.left_x),int(pad.left_y),int(pad.right_x),int(pad.right_y),unsigned(pad.connected),
            unsigned(snapshot.player_hp),unsigned(snapshot.r100_first_enemy_hp),unsigned(snapshot.r100_second_enemy_hp),
            word(snapshot.player_state),word(snapshot.r100_first_enemy_state),word(snapshot.r100_second_enemy_state),
            word(position),word(position+1),word(position+2),word(first),word(first+1),word(first+2),
            word(second),word(second+1),word(second+2),word(&snapshot.player_yaw),
            snapshot.r100_scenario_flags,unsigned(snapshot.r100_placement_dead),
            snapshot.player_attack_target,snapshot.player_attack_skill);
    }

    static void trace_startup_random(const gh_startup_snapshot &snapshot)
    {
        std::fprintf(stderr,"QA_R100_STARTUP_MT {\"tick\":%llu,\"room\":%u,\"main\":%u,"
            "\"title_move\":%llu,\"movie_frames\":%llu,\"movie_complete\":%u,"
            "\"event\":%d,\"event_state\":%u,\"cut\":%d,\"cut_frame_bits\":%u,"
            "\"scene\":%llu,\"move\":%llu,\"rng_index\":%u,\"rng_block_word\":%u}\n",
            (unsigned long long)snapshot.ticks,unsigned(snapshot.current_room),unsigned(snapshot.game_state),
            (unsigned long long)snapshot.title_move_serial,(unsigned long long)snapshot.movie_video.frames_presented,
            unsigned(snapshot.movie_video.complete),snapshot.event_number,unsigned(snapshot.event_state),
            snapshot.event_cut,word(&snapshot.event_cut_frame),(unsigned long long)snapshot.scene_frame_serial,
            (unsigned long long)snapshot.player_updates_completed,
            snapshot.gameplay_random_index,snapshot.gameplay_random_block_word);
    }

    static bool parse(std::string_view line,std::array<int64_t,10> &row,size_t columns)
    {
        for(size_t field=0;field<columns;++field){
            const size_t delimiter=line.find(',');
            const bool final=field+1==columns;
            if(final!=(delimiter==std::string_view::npos))return false;
            const auto token=line.substr(0,delimiter);
            const auto end=token.data()+token.size();
            const auto result=std::from_chars(token.data(),end,row[field]);
            if(result.ec!=std::errc{}||result.ptr!=end)return false;
            if(!final)line.remove_prefix(delimiter+1);
        }
        return true;
    }
};
#endif
