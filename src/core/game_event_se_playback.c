#include "game_event_se_playback.h"

#include <string.h>

static uint32_t word(const uint8_t *p)
{
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}

static int live_link(const gh_event_link *link)
{
    return link && link->bound && link->constructors_done && link->components.constructed &&
        link->module && link->module->loaded && link->module_job==link->module->job &&
        link->module->allocation.pointer && link->components.module==link->module->allocation.pointer;
}

gh_game_event_status gh_event_se_move_play_00009aa8(gh_event_link *link,
    const gh_event_data_prefix *data,gh_event_audio_channel1 *audio,uint32_t *frontier,int *relative)
{
    if(!frontier||!relative)return GH_EVENT_ARGUMENT;
    *frontier=0x9aa8;*relative=1;
    if(!live_link(link)||!data||data->components!=&link->components||!audio)return GH_EVENT_ARGUMENT;
    if(link->components.event_no!=1)return GH_EVENT_UNSUPPORTED;
    gh_event_component_view env,se;
    if(gh_event_component_at(&link->components,0,&env)||env.original_offset!=0xd200||env.size<0x2c||
       gh_event_component_at(&link->components,8,&se)||se.original_offset!=0xd9f0||se.size<0x18)
        return GH_EVENT_BOUNDS;
    gh_event_sequence_view sequence;
    gh_game_event_status status=gh_event_sequence_at(data,8,&sequence);
    if(status)return status;
    uint32_t flags=word(env.bytes+0x28);
    if((flags&1) && (sequence.count||word(se.bytes+0x14))){
        *frontier=0xa1a0;
        return GH_EVENT_UNSUPPORTED;
    }

    /* EventDebug at D1E0 is separate from the 15 constructed components.
     * The supported retail path has no writer to this module word. */
    gh_event_symbol_binding debug;
    if(gh_game_event_link_symbol(link,"EventDebug",&debug)||debug.imported||debug.original_address!=0xd1e0||
       link->module->file_bytes<0xd1e4){*frontier=0x9b2c;return GH_EVENT_FORMAT;}
    const uint8_t *module=link->module->allocation.pointer;
    if(word(module+0xd1e0)&1)goto done;
    if(word(env.bytes+0x0c)){*frontier=0x6928;return GH_EVENT_UNSUPPORTED;}
    if(!(flags&1) || (word(env.bytes+0x14)&0x7fffffffu))goto done;

    int32_t cut;
    uint32_t raw_cut=word(env.bytes+0x10);
    memcpy(&cut,&raw_cut,sizeof(cut));
    if(cut<0){*frontier=0x9b70;return GH_EVENT_UNSUPPORTED;}
    *relative=0;
    int ready;
    status=gh_event_audio_channel1_service(audio,GH_EVENT_AUDIO_START_002D1A30,1,cut,&ready,frontier);
    if(status){if(!*frontier)*frontier=0x2d1a30;return status;}
done:
    *frontier=0;*relative=0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_se_quit_play_00009cb8(gh_event_link *link,
    gh_event_audio_channel1 *audio,uint32_t *frontier,int *relative)
{
    if(!frontier||!relative)return GH_EVENT_ARGUMENT;
    *frontier=0x9cb8;*relative=1;
    if(!live_link(link)||!audio)return GH_EVENT_ARGUMENT;
    if(link->components.event_no!=1)return GH_EVENT_UNSUPPORTED;
    *relative=0;
    gh_game_event_status status=gh_event_audio_channel1_data_end_002d1958(audio);
    *frontier=status?(audio->frontier?audio->frontier:0x2d1958):0;
    return status;
}
