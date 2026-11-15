#ifndef GH_STARTUP_QA_SAVE_MAP_CONTINUE_H
#define GH_STARTUP_QA_SAVE_MAP_CONTINUE_H

#include "startup_host.h"
#include <cstdio>

struct QASaveMapContinueProbe {
    gh_native_gamepad_sample sample(const gh_startup_snapshot &s,uint64_t presented_serial)
    {
        gh_native_gamepad_sample pad={};
        pad.connected=1;
        if(s.save_map_phase!=phase||s.save_map_selected!=selection){
            phase=s.save_map_phase;
            selection=s.save_map_selected;
            std::fprintf(stderr,"QA_SAVE_MAP room=%04X phase=%u choice=%u tick=%llu scene=%llu\n",
                (unsigned)s.current_room,phase,selection,(unsigned long long)s.ticks,
                (unsigned long long)presented_serial);
        }
        const bool was_pressed=pressed;
        pressed=false;
        if(was_pressed||phase!=5||!s.scene_frame_ready||!presented_serial||
           s.scene_frame_serial!=presented_serial)return pad;
        if(selection==0)pad.buttons=0x0002; // D-pad Down.
        if(selection==1)pad.buttons=0x1000; // Cross.
        pressed=pad.buttons!=0;
        if(pressed)std::fprintf(stderr,"QA_SAVE_MAP_PAD room=%04X buttons=%04X tick=%llu scene=%llu\n",
            (unsigned)s.current_room,(unsigned)pad.buttons,(unsigned long long)s.ticks,
            (unsigned long long)presented_serial);
        return pad;
    }

private:
    unsigned phase=UINT32_MAX,selection=UINT32_MAX;
    bool pressed=false;
};

#endif
