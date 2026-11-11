#include "game_event_env_frame.h"

#include <string.h>

static uint32_t word(const uint8_t *p)
{
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}

static void store(uint8_t *p,uint32_t value)
{
    for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(8*i));
}

static float frame(const uint8_t *p)
{
    uint32_t bits=word(p);
    float result;
    memcpy(&result,&bits,4);
    return result;
}

static void store_frame(uint8_t *p,float value)
{
    uint32_t bits;
    memcpy(&bits,&value,4);
    store(p,bits);
}

static int whole_frame(float value)
{
    return value>=0 && value<16777216.0f && (float)(uint32_t)value==value;
}

static uint8_t *environment(gh_event_timing *timing,gh_event_components *components)
{
    gh_event_component_view env;
    if(!timing||!components||!components->constructed||timing->components!=components||
       timing->event_number!=components->event_no||!timing->allocation.pointer||
       !timing->count||timing->count>UINT8_MAX||gh_event_component_at(components,0,&env)||
       !env.bytes||env.original_offset!=0xd200||env.size!=0x44||word(env.bytes+8)!=timing->count)
        return NULL;
    return env.bytes;
}

gh_game_event_status gh_event_env_update_frame_000059e0(gh_event_timing *timing,
    gh_event_components *components,uint32_t *frontier)
{
    if(!frontier)return GH_EVENT_ARGUMENT;
    *frontier=0x59e0;
    uint8_t *env=environment(timing,components);
    if(!env)return GH_EVENT_ARGUMENT;
    if(word(env+0x0c)){*frontier=0x6b68;return GH_EVENT_UNSUPPORTED;}
    uint32_t flags=word(env+0x28),current_cut=word(env+0x10);
    float current_frame=frame(env+0x14);
    if(current_cut!=UINT32_MAX && (current_cut>=timing->count||!whole_frame(current_frame)))
        return GH_EVENT_BOUNDS;
    uint32_t cut=flags&0x100?current_cut:word(env+0x18);
    float next=flags&0x100?current_frame:frame(env+0x1c);
    if(cut>=timing->count||!whole_frame(next))return GH_EVENT_BOUNDS;
    gh_event_cut_timing *rows=timing->allocation.pointer;
    if(!whole_frame(rows[cut].start_frame))return GH_EVENT_BOUNDS;
    uint32_t seen=(uint32_t)next+1;
    uint32_t next_start=(uint32_t)rows[cut].start_frame+seen;
    if(cut+1<timing->count && next_start>=16777216u)return GH_EVENT_BOUNDS;

    if(flags&0x100){store(env+0x18,cut);store_frame(env+0x1c,next);}
    flags=(flags&~1u)|(uint32_t)(current_cut!=cut||current_frame!=next);
    store(env+0x28,flags);
    store(env+0x10,cut);
    store_frame(env+0x14,next);
    rows[cut].frames_seen=(float)seen;
    if(cut+1<timing->count)rows[cut+1].start_frame=(float)next_start;
    *frontier=0;
    return GH_EVENT_OK;
}

static gh_game_event_status end_request(const gh_event_env_frame_bindings *b,
    uint32_t *flags,int skip,uint32_t *frontier,int *relative)
{
    if(*flags&2)return GH_EVENT_OK;
    if(skip || !(b->config->flags&0x10)){
        if(!b->fade||!b->display||!b->display->initialized){*frontier=0x307208;*relative=0;return GH_EVENT_ARGUMENT;}
        const uint8_t *work=(const uint8_t *)b->work;
        gh_screen_fade_context context={b->display->screen_width,b->display->gs_y_origin,
            word(work+0x564),word(work+0x5e0)};
        if(gh_screen_fade_start_00307208(b->fade,&context,0,10,0,0,0xff000000u,15)){
            *frontier=0x307208;*relative=0;
            return GH_EVENT_CALLEE;
        }
        *flags|=4;
    }
    if(skip)*flags|=0x20;
    *flags|=2;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_env_finish_000064c8(const gh_event_env_end_owners *owners,
    uint32_t *frontier,int *relative)
{
    if(!frontier||!relative)return GH_EVENT_ARGUMENT;
    *frontier=0x2db6b0;*relative=0;
    if(!owners||!owners->work5f4)return GH_EVENT_UNSUPPORTED;
    gh_game_title_player *player=owners->work5f4;
    if(!player->ready||!player->view.bytes||player->view.size<0x6ac)return GH_EVENT_BOUNDS;

    /* Original GetPlayer and GetJacket both resolve before the first store. */
    *frontier=0x2db6c0;
    if(!owners->work5f8)return GH_EVENT_UNSUPPORTED;
    gh_game_player_child *child=*owners->work5f8;
    uint8_t *body=player->view.bytes;
    store(body+0x250,word(body+0x250)&~0x8002u);
    if(child){
        *frontier=0x6554;*relative=1;
        uint8_t *jacket=child->object.view.bytes;
        if(!child->complete||!child->object.ready||!jacket||jacket==body||child->object.view.size<0x254)
            return GH_EVENT_BOUNDS;
        uint32_t flags=word(jacket+0x250);
        if(flags&0x8000)store(jacket+0x250,flags&~0x8002u);
    }

    *frontier=0x656c;*relative=1;
    const gh_event_weapon_slot *slot=owners->weapon_6a8;
    if(!slot||!slot->known)return GH_EVENT_UNSUPPORTED;
    const gh_event_weapon_attachment *weapon=slot->current;
    if(!weapon){
        if(word(body+0x6a8))return GH_EVENT_UNSUPPORTED;
    }else{
        *frontier=0x6578;
        if(!weapon->weapon||!weapon->weapon->bytes||weapon->weapon->size<0x254||
           !weapon->parent||weapon->parent->bytes!=body)
            return GH_EVENT_BOUNDS;
        uint8_t *bytes=weapon->weapon->bytes;
        uint32_t flags=word(bytes+0x250);
        if(flags&0x8000){
            store(bytes+0x250,flags&~0x8000u);
            weapon=slot->current; /* Original reload of player+6A8 at 65A0. */
            bytes=weapon->weapon->bytes;
            store(bytes+0x250,word(bytes+0x250)&~2u);
        }
    }
    *frontier=0;*relative=0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_env_calc_next_00005b78(const gh_event_env_frame_bindings *b,
    int *original_end,uint32_t *frontier,int *relative)
{
    if(!original_end||!frontier||!relative)return GH_EVENT_ARGUMENT;
    *original_end=0;*frontier=0x5b78;*relative=1;
    if(!b||!b->work||!b->config||!b->config->initialized||!b->data||b->data->components!=b->components)
        return GH_EVENT_ARGUMENT;
    uint8_t *env=environment(b->timing,b->components);
    if(!env)return GH_EVENT_ARGUMENT;
    if(word(env+0x0c)){*frontier=0x6b68;return GH_EVENT_UNSUPPORTED;}
    uint32_t cut=word(env+0x10),next_cut=word(env+0x18),flags=word(env+0x28)&~0x30000000u;
    float current=frame(env+0x14),next=frame(env+0x1c);
    if(cut>=b->timing->count||next_cut>=b->timing->count||!whole_frame(current)||!whole_frame(next))
        return GH_EVENT_BOUNDS;
    gh_event_cut_timing *rows=b->timing->allocation.pointer;
    gh_event_cut_timing *row=&rows[cut];
    if(row->kind){
        const uint32_t kind_pc[]={0x5ca8,0x5dd8,0x5e38,0x6098};
        *frontier=row->kind<4?kind_pc[row->kind]:0x5b78;
        return GH_EVENT_UNSUPPORTED;
    }
    if(row->next_cut!=UINT8_MAX && row->next_cut>=b->timing->count){*frontier=0x60e8;return GH_EVENT_BOUNDS;}
    uint16_t length;
    gh_game_event_status status=gh_event_camera_cut_length_00004e60(b->data,cut,&length);
    if(status||!length){*frontier=0x4e60;return status?status:GH_EVENT_BOUNDS;}

    uint32_t input=word((const uint8_t*)b->work+0x1a0);
    if((input&0x800000u) && !(flags&8) && b->fade && !b->fade->state &&
       (row->next_cut!=word(env+0x2c)||next<(float)((int)length-35))){
        status=end_request(b,&flags,1,frontier,relative);
        if(status)return status;
    }
    if(flags&0x40)flags&=~0x40u;
    else{
        next=(float)((uint32_t)current+1u);
        int end_at=(int)length-((b->config->flags&0x10)?0:30);
        if(row->next_cut==UINT8_MAX && end_at>=0 && next>=(float)end_at){
            status=end_request(b,&flags,0,frontier,relative);
            if(status)return status;
        }
        if(next>(float)(length-1) && !(row->reserved_word&0x200u)){
            if(row->next_cut==UINT8_MAX){
                status=end_request(b,&flags,0,frontier,relative);
                if(status)return status;
            }else{next_cut=row->next_cut;next=0;}
        }
    }
    store(env+0x18,next_cut);
    store_frame(env+0x1c,next);
    store(env+0x28,flags);
    if((flags&2) && (!(flags&4) || (b->fade && (b->fade->state&4)))){
        status=gh_event_env_finish_000064c8(b->end_owners,frontier,relative);
        if(status)return status;
        *original_end=1;
    }
    *frontier=0;*relative=0;
    return GH_EVENT_OK;
}
