#include "r100_exit.h"
#include "../core/game_scenario_exec.h"
#include "../core/game_scenario_soft_event.h"
#include <string.h>

_Static_assert(sizeof(gh_event_reserve_work)==0x30,"Event room jump work extent");
_Static_assert(offsetof(gh_event_reserve_work,position)==0x10,"Event destination XYZ");
_Static_assert(offsetof(gh_event_reserve_work,yaw)==0x20,"Event destination yaw");

static uint32_t word(const uint8_t *bytes)
{
    uint32_t value;
    memcpy(&value,bytes,sizeof(value));
    return value;
}

static void store(uint8_t *bytes,uint32_t value)
{
    memcpy(bytes,&value,sizeof(value));
}

static gh_scene_at_status stop(uint32_t *frontier,uint32_t address,gh_scene_at_status status)
{
    *frontier=address;
    return status;
}

static void exit_task_00000a38(void *context,int32_t argument)
{
    gh_r100_exit *owner=context;
    const gh_r100_exit_bindings *b=&owner->bindings;
    gh_event_reserve_work *event=b->event_work;
    if(argument!=0||!event){
        owner->frontier=0xa38;
        owner->status=GH_AT_UNSUPPORTED;
        while(gh_task_threads_sleep(b->threads,1)==GH_THREAD_OK){}
        return;
    }

    store(b->room->active+4,word(b->room->active+4)|UINT32_C(0x400000));
    memset(event,0,sizeof(*event));
    event->mode=2;
    event->destination_room=0x105;
    const uint32_t xyz[]={0xc0875c29,0xc03c28f6,0x429a6666};
    const uint32_t yaw=0xc010a3d7;
    memcpy(event->position,xyz,sizeof(xyz));
    memcpy(&event->yaw,&yaw,sizeof(yaw));
    event->flags=3;
    event->flags|=8;

    if(b->work->room!=3){
        const float zero_vec[3]={0};
        for(;;){
            uint32_t frontier=0;
            gh_door_status status=b->begin_room?b->begin_room(b->context,3,zero_vec,0,0,2,0,&frontier):GH_DOOR_UNSUPPORTED;
            if(status==GH_DOOR_OK)break;
            owner->frontier=frontier?frontier:0x2c4548;
            owner->status=status==GH_DOOR_BOUNDS?GH_AT_BOUNDS:GH_AT_UNSUPPORTED;
            if(gh_task_threads_sleep(b->threads,1)!=GH_THREAD_OK)return;
        }
    }
    owner->frontier=0;
    owner->status=GH_AT_OK;
    owner->pending=0;
}

static gh_scene_at_status owned_jump(const gh_r100_exit *owner,gh_at_native_unit *jump,
    uint32_t *frontier)
{
    const gh_r100_exit_bindings *b=&owner->bindings;
    if(!b->work||b->work->room!=0x100||!b->at)
        return stop(frontier,0x2c2138,GH_AT_ARGUMENT);
    gh_scene_at_status lookup=gh_scene_at_get_jump_002c2ae0(b->at,0,jump);
    if(lookup)return stop(frontier,0x2c2ae0,lookup);
    if(!jump->bytes||jump->size!=0x9c||jump->bytes[0x35]!=1||
       jump->bytes[0x44]!=0xff||word(jump->bytes+0x3c)!=0xa38||word(jump->bytes+0x40))
        return stop(frontier,0x2c2138,GH_AT_UNSUPPORTED);
    return GH_AT_OK;
}

gh_scene_at_status gh_r100_exit_spawn_001f7858(void *context,
    const gh_scene_at_task_request *request,uint32_t *frontier)
{
    gh_r100_exit *owner=context;
    if(!owner||!request||!frontier)return GH_AT_ARGUMENT;
    *frontier=0;
    if(request->entry_address!=0x2c2138||request->argument_mode!=2||
       request->raw_argument||request->group!=4||request->mode!=0x20)
        return stop(frontier,0x1f7858,GH_AT_UNSUPPORTED);
    gh_at_native_unit jump;
    gh_scene_at_status lookup=owned_jump(owner,&jump,frontier);
    if(lookup)return lookup;
    if(request->unit.bytes!=jump.bytes||request->unit.handle!=jump.handle||
       request->unit.size!=jump.size||jump.bytes[0x3b]!=8)
        return stop(frontier,0x1f7858,GH_AT_UNSUPPORTED);
    const gh_action_set_request action={.priority=4,.life=0x20,.callback_id=0x2c2138,
        .argument=jump.bytes,.kind=2};
    int emitted;
    unsigned index;
    gh_action_set_status status=gh_action_button_set_001f7858(owner->bindings.buttons,
        &action,&emitted,&index,frontier);
    return status?stop(frontier,*frontier?*frontier:0x1f7858,GH_AT_UNSUPPORTED):GH_AT_OK;
}

gh_action_step_status gh_r100_exit_action_002c2138(void *context,uint32_t entry,
    void *unit_argument,uint32_t *frontier)
{
    gh_r100_exit *owner=context;
    if(!owner||!frontier)return GH_ACTION_STEP_ARGUMENT;
    *frontier=entry;
    const gh_r100_exit_bindings *b=&owner->bindings;
    if(entry!=0x2c2138||!b->room||!b->room->active||b->room->size<8||
       !b->scenario.bytes||b->scenario.size<0x54||!b->threads||!b->event_work||owner->pending)
        return GH_ACTION_STEP_UNSUPPORTED;
    gh_at_native_unit jump;
    if(owned_jump(owner,&jump,frontier)||jump.bytes!=unit_argument||jump.bytes[0x3b]!=8)
        return GH_ACTION_STEP_UNSUPPORTED;
    if(gh_scenario_clear_pad_0031b410(b->work)){
        *frontier=0x31b410;
        return GH_ACTION_STEP_CALLEE;
    }
    store((uint8_t*)b->work+8,0);
    store(jump.bytes+0x50,word(jump.bytes+0x50)|UINT32_C(0x02000000));

    /* 2C38D8 uses 2D54F0 with original a2=0 and signed a3=-1. The native
     * task adapter carries that zero argument. 2C3968 clears attributes. */
    int32_t slot;
    if(gh_scenario_exec_002c3890(b->scenario,b->threads,-1,exit_task_00000a38,owner,0,&slot,frontier)){
        if(!*frontier)*frontier=0x2c38d8;
        return GH_ACTION_STEP_CALLEE;
    }
    /* 31B410 leaves a1 pointing at the canonical pad. Keep its original
     * address token in Scenario attributes, including the -1/+50 case. */
    const size_t offset=slot<0?0x50:0x58+(size_t)slot*8;
    if(offset>b->scenario.size||b->scenario.size-offset<4){
        *frontier=0x2c2180;
        return GH_ACTION_STEP_BOUNDS;
    }
    store(b->scenario.bytes+offset,0x747618);
    owner->slot=slot;
    owner->pending=slot>=0;
    *frontier=0;
    return GH_ACTION_STEP_OK;
}
