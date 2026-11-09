#include "game_event.h"
#include <string.h>
_Static_assert(sizeof(gh_game_event_prefix)==0x38,"verified event prefix");
_Static_assert(offsetof(gh_game_event_prefix,flags)==8,"event flags offset");
_Static_assert(offsetof(gh_game_event_prefix,position_bits)==0x20,"event position");
_Static_assert(offsetof(gh_game_event_prefix,skip_ok)==0x34,"event skip flag");
static uint32_t u32(const uint8_t *p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned u16(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
gh_game_event_status gh_game_event_from_elf(gh_game_event *event,const void *data,size_t bytes)
{
    const uint8_t *elf=data;uint32_t table;unsigned count,stride;
    if(!event||!elf)return GH_EVENT_ARGUMENT;
    if(bytes<52)return GH_EVENT_BOUNDS;
    if(memcmp(elf,"\177ELF\1\1\1",7)||u16(elf+18)!=8)return GH_EVENT_FORMAT;
    table=u32(elf+28);count=u16(elf+44);stride=u16(elf+42);
    if(stride<32||count>128||table>bytes||count>(bytes-table)/stride)return GH_EVENT_BOUNDS;
    for(unsigned i=0;i<count;++i){
        const uint8_t *p=elf+table+(size_t)i*stride;
        uint32_t offset=u32(p+4),base=u32(p+8),files=u32(p+16),memory=u32(p+20);
        if(u32(p)!=1||base>0x586af0u||(uint64_t)0x586af0+sizeof(event->fields)>(uint64_t)base+memory)continue;
        if(files>memory||offset>bytes||files>bytes-offset)return GH_EVENT_BOUNDS;
        gh_game_event candidate={0};uint32_t delta=0x586af0u-base;
        if(delta<files){size_t take=files-delta;if(take>sizeof(candidate.fields))take=sizeof(candidate.fields);
            memcpy(&candidate.fields,elf+offset+delta,take);}
        candidate.initialized=1;*event=candidate;return GH_EVENT_OK;
    }
    return GH_EVENT_BOUNDS;
}
gh_game_event_status gh_game_event_set_stage(gh_game_event *event,unsigned stage,gh_event_native_stage call,void *context)
{
    if(!event||!event->initialized||stage>=6)return GH_EVENT_ARGUMENT;
    event->stages[stage]=call;event->contexts[stage]=context;return GH_EVENT_OK;
}
static gh_game_event_status invoke(gh_game_event *event,unsigned stage,int *done)
{
    static const uint32_t addresses[]={0x2965f0,0x296818,0x296958,0x296c28,0x297038,0x296dd8};
    event->frontier=addresses[stage];
    if(!event->stages[stage])return GH_EVENT_UNSUPPORTED;
    event->callee_error=event->stages[stage](event->contexts[stage],event,done);
    if(event->callee_error)return GH_EVENT_CALLEE;
    event->frontier=0;return GH_EVENT_OK;
}
gh_game_event_status gh_game_event_update_00296530(gh_game_event *event)
{
    int done=0;gh_game_event_status status;
    if(!event||!event->initialized)return GH_EVENT_ARGUMENT;
    event->frontier=0;event->callee_error=0;
    uint32_t flags=event->fields.flags;
    if(flags&8)return invoke(event,5,&done);
    if(!(flags&1)||event->fields.state>=5)return GH_EVENT_OK;
    /* Select once. A native callee can change self.state, but the original
     * switch then falls through its compiled cases rather than redispatching. */
    for(unsigned stage=event->fields.state;stage<5;++stage){
        done=0;status=invoke(event,stage,&done);if(status)return status;
        if(stage<4&&!done)return GH_EVENT_OK;
    }
    return GH_EVENT_OK;
}
