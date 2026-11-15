#ifndef GH_STARTUP_QA_TITLE_H
#define GH_STARTUP_QA_TITLE_H

#include "startup_host.h"

struct QAStartupTitleProbe {
    unsigned memory_card_step=0;
    uint64_t level_confirm_tick=0;

    void sample(const gh_startup_snapshot &s,gh_native_input &input,bool new_game,bool load)
    {
        constexpr unsigned cross='Z',right=39;
        unsigned key=0;
        if(s.mc_load_mode){
            const bool choosing=s.mc_phase==3||s.mc_phase==4;
            const bool ready=s.mc_subphase==1||s.mc_subphase==2;
            if(load&&choosing&&ready&&s.mc_present_mask&&s.ticks%6==0)key=cross;
        }else if(s.mc_phase==3&&s.mc_subphase==1){
            key=cross;
        }else if(s.mc_phase==3&&s.mc_subphase==2){
            if(memory_card_step==0){key=right;++memory_card_step;}
            else if(memory_card_step==1){key=cross;++memory_card_step;}
        }else{
            memory_card_step=0;
        }
        if((new_game||load)&&s.title_live){
            const bool timed_level=new_game&&!load&&s.title_phase==3&&level_confirm_tick;
            const bool confirm_due=timed_level?s.ticks>=level_confirm_tick&&
                (s.ticks-level_confirm_tick)%6==0:s.ticks%6==0;
            if(s.title_mode==1&&s.ticks%30==0)key=cross;
            else if(s.title_mode==3&&s.title_phase>=1&&s.title_phase<=3&&s.title_subphase==1&&
                    (s.title_phase==1||s.title_timer>=5)&&confirm_due){
                if(!load||s.title_phase==1||s.title_game_selection==1)key=cross;
                else if(s.title_phase==2&&s.title_game_selection==0)key=right;
            }
        }
        gh_native_input_key(&input,cross,key==cross);
        gh_native_input_key(&input,right,key==right);
    }
};

#endif
