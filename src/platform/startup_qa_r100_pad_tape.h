#ifndef GH_STARTUP_QA_R100_PAD_TAPE_H
#define GH_STARTUP_QA_R100_PAD_TAPE_H

#include "startup_host.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

struct QAR100PadTape {
    bool enabled=false;
    uint64_t victory_scene=0;

    bool load(const char *path)
    {
        std::ifstream input(path);
        std::string line;
        if(!std::getline(input,line)||
           line!="tick,move,buttons,left_x,left_y,right_x,right_y,connected")return false;
        uint64_t last_tick=0;
        while(std::getline(input,line)){
            std::array<int64_t,8> row={};
            if(!parse(line,row)||row[0]<=0||uint64_t(row[0])<=last_tick||
               row[1]<0||row[2]<0||row[2]>65535||row[7]<0||row[7]>1)return false;
            for(unsigned axis=3;axis<7;++axis)
                if(row[axis]<-32768||row[axis]>32767)return false;
            last_tick=uint64_t(row[0]);
            const uint64_t move=uint64_t(row[1]);
            if(!samples.empty()&&move<samples.back().move)return false;
            const std::array<int64_t,6> fields={row[2],row[3],row[4],row[5],row[6],row[7]};
            if(!samples.empty()&&move==samples.back().move){
                if(fields!=samples.back().fields)return false;
                continue;
            }
            samples.push_back({move,fields});
        }
        enabled=input.eof()&&!samples.empty();
        return enabled;
    }

    gh_native_gamepad_sample sample(const gh_startup_snapshot &snapshot)
    {
        gh_native_gamepad_sample pad={};
        pad.connected=1;
        if(snapshot.current_room!=0x100||snapshot.game_state!=4)return pad;
        if(snapshot.event_number==1&&snapshot.event_state)saw_ev01=true;
        if(!saw_ev01||snapshot.event_state)return pad;
        const uint64_t move=snapshot.player_updates_completed;
        while(next<samples.size()&&samples[next].move<move)++next;
        if(next==samples.size()||samples[next].move!=move)return pad;
        const auto &fields=samples[next].fields;
        pad.buttons=uint16_t(fields[0]);
        pad.left_x=int16_t(fields[1]);
        pad.left_y=int16_t(fields[2]);
        pad.right_x=int16_t(fields[3]);
        pad.right_y=int16_t(fields[4]);
        pad.connected=uint8_t(fields[5]);
        if(last_sample!=move){
            ++sampled_moves;
            last_sample=move;
        }
        return pad;
    }

    bool presented(const gh_startup_snapshot &snapshot,uint64_t scene)
    {
        if(!enabled||!saw_ev01||!sampled_moves||snapshot.current_room!=0x100||
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
        uint64_t move;
        std::array<int64_t,6> fields;
    };
    std::vector<Sample> samples;
    size_t next=0;
    bool saw_ev01=false;
    uint64_t last_sample=UINT64_MAX,sampled_moves=0,deaths[2]={};
    uint32_t handles[2]={},room_flags=0;
    unsigned death_mask=0,player_hp=0;

    static bool parse(std::string_view line,std::array<int64_t,8> &row)
    {
        for(size_t field=0;field<row.size();++field){
            const size_t delimiter=line.find(',');
            const bool final=field+1==row.size();
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
