#include "game_event_release_stage.h"

#include "game_scene_aux.h"
#include <string.h>

static uint32_t word(const void *bytes)
{
    uint32_t result;
    memcpy(&result, bytes, sizeof(result));
    return result;
}

static void put(void *bytes, uint32_t value)
{
    memcpy(bytes, &value, sizeof(value));
}

static gh_game_event_status leaf(const gh_event_release_bindings *b,
    uint32_t site, uint32_t target, uint32_t *frontier, int *relative)
{
    *frontier=target;
    *relative=0;
    if(!b->leaf)return GH_EVENT_UNSUPPORTED;
    uint32_t child=0;
    gh_game_event_status status=b->leaf(b->context,site,&child,relative);
    if(status&&child)*frontier=child;
    return status;
}

static gh_game_event_status black_fade(const gh_event_release_bindings *b, uint32_t *frontier)
{
    *frontier=0x307208;
    if(!b->fade||!b->display||!b->display->initialized)return GH_EVENT_UNSUPPORTED;
    const uint8_t *work=(const uint8_t *)b->work;
    gh_screen_fade_context c={b->display->screen_width,b->display->gs_y_origin,word(work+0x564),word(work+0x5e0)};
    return gh_screen_fade_start_00307208(b->fade,&c,0,8,0,0xff000000u,0xff000000u,0)?GH_EVENT_CALLEE:GH_EVENT_OK;
}

static gh_game_event_status release_timing(gh_event_timing *timing, uint32_t *frontier, int *relative)
{
    *frontier=0x5578;
    *relative=1;
    if(!timing||!timing->components)return GH_EVENT_UNSUPPORTED;
    gh_event_component_view env;
    if(gh_event_component_at((gh_event_components *)timing->components,0,&env)||
        env.original_offset!=0xd200||env.size!=0x44)return GH_EVENT_BOUNDS;
    put(env.bytes+8,0);
    return gh_event_timing_abort(timing);
}

static gh_game_event_status cleanup(gh_event_release_stage *s,const gh_event_release_bindings *b,
    uint32_t *frontier,int *relative)
{
    gh_game_event_prefix *event=&b->event->fields;
    uint8_t *work=(uint8_t *)b->work;
    gh_game_event_status status;
    const uint32_t config=b->config->flags;
    for(;s->cleanup_step<15;++s->cleanup_step){
        *relative=0;
        switch(s->cleanup_step){
        case 0:
            if(event->flags&0x100u){
                status=gh_event_actor_release_work_00297ad0(b->actors,frontier);
                if(status){*relative=*frontier==0x1428||*frontier==0x0c60;return status;}
            }
            break;
        case 1:
            if(event->flags&0x100u){status=release_timing(b->timing,frontier,relative);if(status)return status;}
            break;
        case 2:
            if(event->text_token){status=leaf(b,0x29714c,0x2a9680,frontier,relative);if(status)return status;}
            break;
        case 3:
            status=leaf(b,0x297158,0x297158,frontier,relative);
            if(status)return status;
            break;
        case 4:
            status=leaf(b,0x2971f4,0x2c4440,frontier,relative);
            if(status)return status;
            break;
        case 5:
            put(work+0x5e4,word(work+0x5e4)&~0x20000000u);
            put(work+0x5d8,(word(work+0x5d8)&~0x200000u)|0x40000000u);
            break;
        case 6:
            if(event->flags&0x80u){status=leaf(b,0x297244,0x65c8,frontier,relative);if(status)return status;}
            break;
        case 7:
            if(event->flags&0x80u){status=leaf(b,0x29724c,0x2974f0,frontier,relative);if(status)return status;}
            break;
        case 8:
            *frontier=0x297660;
            status=gh_game_event_reset_00297660(b->event);
            if(status)return status;
            break;
        case 9:
            if(!(config&0x841u)){status=leaf(b,0x2972b0,0x294ca0,frontier,relative);if(status)return status;}
            break;
        case 10:
            if(!(config&0x841u)){
                *frontier=0x30e5c0;
                if(!b->background_rgb_604700)return GH_EVENT_UNSUPPORTED;
                const uint8_t *rgb=b->background_rgb_604700;
                gh_scene_background_mode_0030e5c0(b->work,1,rgb[0],rgb[1],rgb[2]);
            }
            break;
        case 11:
            if(!(config&0x841u)||(config&1u))put(work+0x5e0,word(work+0x5e0)&~0x2000000u);
            break;
        case 12:
            if(!(config&0x841u)){
                *frontier=0x307208;
                if(!b->fade||!b->display||!b->display->initialized)return GH_EVENT_UNSUPPORTED;
                gh_screen_fade_context c={b->display->screen_width,b->display->gs_y_origin,word(work+0x564),word(work+0x5e0)};
                if(gh_screen_fade_start_00307208(b->fade,&c,0,8,0,0xff000000u,0,15))return GH_EVENT_CALLEE;
            }
            break;
        case 13:
            if(!(config&0x841u)){
                *frontier=0x297318;
                if(!b->screen_state_5cae40)return GH_EVENT_UNSUPPORTED;
                *b->screen_state_5cae40=15;
            }
            break;
        case 14:
            if(!(config&0x40u)){status=leaf(b,0x297358,0x2b65f0,frontier,relative);if(status)return status;}
            break;
        }
    }
    s->complete=1;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_release_00297038(gh_event_release_stage *s,const gh_event_release_bindings *b,
    uint32_t *frontier,int *relative)
{
    if(!frontier||!relative)return GH_EVENT_ARGUMENT;
    *frontier=0x297038;
    *relative=0;
    if(!s||!b||!b->event||!b->event->initialized||!b->work||!b->config||!b->config->initialized||
        (s->owner&&s->owner!=b->event))return GH_EVENT_ARGUMENT;
    if(s->complete){*frontier=0;return GH_EVENT_OK;}
    gh_game_event_prefix *event=&b->event->fields;
    if(!s->owner){if(event->state!=4)return GH_EVENT_ARGUMENT;s->owner=b->event;}
    gh_game_event_status status;
    if(!s->in_cleanup){
        if((event->flags&0x180u)==0x180u){
            if(!event->phase){
                if(!(b->config->flags&0x40u)){
                    if(s->initial_step==0){status=black_fade(b,frontier);if(status)return status;s->initial_step=1;}
                    if(s->initial_step==1){gh_scene_background_mode_0030e5c0(b->work,1,0,0,0);s->initial_step=2;}
                    if(s->initial_step==2){status=leaf(b,0x2970dc,0x2b65b0,frontier,relative);if(status)return status;s->initial_step=3;}
                }
                status=gh_event_actor_start_release_00297a88(b->actors,frontier);
                if(status)return status;
            }
            if(event->phase!=8){++event->phase;*frontier=0;return GH_EVENT_OK;}
            int ready;
            status=gh_event_actor_wait_release_00297aa8(b->actors,&ready,frontier);
            if(status||!ready)return status;
        }
        s->in_cleanup=1;
    }
    status=cleanup(s,b,frontier,relative);
    if(!status){*frontier=0;*relative=0;}
    return status;
}
