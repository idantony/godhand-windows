#include "r003_event.h"
#include "../core/game_event_resource.h"
#include "../core/game_scenario_exec.h"
#include <string.h>

_Static_assert(sizeof(gh_event_reserve_work)==0x30,"Event+38 reserve extent");
_Static_assert(offsetof(gh_event_reserve_work,position)==0x10,"Event+38 XYZ");
_Static_assert(offsetof(gh_event_reserve_work,yaw)==0x20,"Event+38 yaw");

enum {CHECK_ASSET,WAIT_SOFT,PLAY_START,PLAYING,END_SOFT,RESERVE_DONE,JUMP};

static gh_r003_status fail(gh_r003_event *s,uint32_t frontier,gh_r003_status status)
{
    s->frontier=frontier;
    return status;
}

static gh_r003_status step(gh_r003_event *s)
{
    gh_r003_bindings *b=&s->bindings;
    gh_game_event *event=b->play.event;
    gh_event_reserve_work *reserve=b->reserve;

    if(s->task_stage==CHECK_ASSET){
        /* 296370 maps the signed event number through the identity 2962A8. */
        memcpy(&s->event_number,&reserve->mode,4);
        char path[32];
        if(gh_game_event_resource_path_00297540(s->event_number,path))
            return fail(s,0x296370,GH_R003_CALLEE);
        size_t index;
        const gh_disc_error found=gh_disc_find(b->disc,path,&index);
        if(found!=GH_DISC_OK&&found!=GH_DISC_NOT_FOUND)
            return fail(s,0x2011f8,GH_R003_CALLEE);
        s->task_stage=found==GH_DISC_OK?WAIT_SOFT:RESERVE_DONE;
    }
    if(s->task_stage==WAIT_SOFT){
        uint32_t flags;
        memcpy(&flags,(const uint8_t *)b->play.work+0x5e4,4);
        if(flags&0x10)return GH_R003_OK;
        const gh_soft_event_status status=gh_scenario_start_soft_event_002c39b0(&s->soft_start,&b->soft,4);
        if(status)return fail(s,s->soft_start.frontier?s->soft_start.frontier:0x2c39b0,
            status==GH_SOFT_EVENT_UNSUPPORTED?GH_R003_UNSUPPORTED:GH_R003_CALLEE);
        s->task_stage=PLAY_START;
    }
    if(s->task_stage==PLAY_START){
        uint32_t frontier=0;
        const gh_game_event_status status=gh_game_event_play_start_00296458(&b->play,s->event_number,&frontier);
        if(status)return fail(s,frontier?frontier:0x296458,
            status==GH_EVENT_UNSUPPORTED?GH_R003_UNSUPPORTED:GH_R003_CALLEE);
        s->task_stage=PLAYING;
    }
    if(s->task_stage==PLAYING){
        if(event->fields.flags&1)return GH_R003_OK;
        s->task_stage=END_SOFT;
    }
    if(s->task_stage==END_SOFT){
        if(!b->end_soft)return fail(s,0x2c3c10,GH_R003_UNSUPPORTED);
        uint32_t frontier=0;
        const gh_game_event_status status=b->end_soft(b->context,&frontier);
        if(status)return fail(s,frontier?frontier:0x2c3c10,
            status==GH_EVENT_UNSUPPORTED?GH_R003_UNSUPPORTED:GH_R003_CALLEE);
        s->task_stage=RESERVE_DONE;
    }
    if(s->task_stage==RESERVE_DONE){
        reserve->flags&=~1u;
        /* Original REL+278 selects either the indexed or positioned jump. */
        if(!(reserve->flags&2))return fail(s,0x2c4510,GH_R003_UNSUPPORTED);
        if((uint16_t)b->play.work->room==reserve->destination_room){
            s->finished=1;
            return GH_R003_OK;
        }
        s->jump_room=reserve->destination_room;
        s->jump_force=(uint8_t)!(reserve->flags&0x10u);
        memcpy(s->jump_position,reserve->position,sizeof(s->jump_position));
        memcpy(&s->jump_yaw,&reserve->yaw,4);
        s->task_stage=JUMP;
    }
    if(!b->begin_jump)return fail(s,0x2c4548,GH_R003_UNSUPPORTED);
    uint32_t frontier=0;
    const gh_door_status status=b->begin_jump(b->context,s->jump_room,s->jump_position,
        s->jump_yaw,0,0,s->jump_force,&frontier);
    if(status)return fail(s,frontier?frontier:0x2c4548,
        status==GH_DOOR_UNSUPPORTED?GH_R003_UNSUPPORTED:GH_R003_CALLEE);
    s->finished=1;
    return GH_R003_OK;
}

static void run(void *context,int32_t argument)
{
    (void)argument;
    gh_r003_event *s=context;
    while(!s->finished){
        s->frontier=0;
        s->status=step(s);
        if(s->finished)return;
        if(gh_task_threads_sleep(s->bindings.threads,1)){
            s->status=fail(s,0x2d5aa0,GH_R003_CALLEE);
            return;
        }
    }
}

gh_r003_status gh_r003_event_init_000001a0(gh_r003_event *s,const gh_r003_bindings *b)
{
    if(!s||!b||!b->module||!b->module->bound||b->module->room!=3||b->module->init_offset!=0x1a0||
       b->module->move_offset!=0x228||!b->heap||!b->disc||!b->threads||!b->reserve||!b->play.event||
       !b->play.event->initialized||!b->play.work||b->play.work->room!=3||b->soft.work!=b->play.work||
       b->soft.scenario.bytes!=b->scenario.bytes||b->soft.scenario.manager!=b->scenario.manager)
        return GH_R003_ARGUMENT;
    if(s->work.pointer&&(s->bindings.module!=b->module||s->bindings.heap!=b->heap||
       s->bindings.play.event!=b->play.event||s->bindings.threads!=b->threads||s->bindings.reserve!=b->reserve))
        return GH_R003_ARGUMENT;
    if(s->initialized)return GH_R003_OK;
    if(!s->work.pointer){
        if(gh_heap_allocate(b->heap,12,16,&s->work))return fail(s,0x30f550,GH_R003_MEMORY);
        const uint32_t initial[]={0,0x3f000000,0x3c23d70a};
        memcpy(s->work.pointer,initial,sizeof(initial));
        s->bindings=*b;
    }
    s->slot=-1;
    if((b->reserve->flags&3)==3){
        const gh_scenario_exec_status status=gh_scenario_exec_002c3890(b->scenario,b->threads,-1,run,s,0,
            &s->slot,&s->frontier);
        if(status)return fail(s,s->frontier?s->frontier:0x2c3890,GH_R003_CALLEE);
    }
    s->initialized=1;
    s->frontier=0;
    return GH_R003_OK;
}

gh_r003_status gh_r003_event_release(gh_r003_event *s)
{
    if(!s)return GH_R003_ARGUMENT;
    if(s->slot>=0&&s->initialized&&!s->finished){
        const gh_task_manager *manager=s->bindings.scenario.manager;
        if(!manager||!manager->records||(size_t)s->slot>=manager->count||manager->records[s->slot].running)
            return GH_R003_UNSUPPORTED;
    }
    if(s->work.pointer&&gh_heap_release(s->bindings.heap,s->work))return GH_R003_CALLEE;
    *s=(gh_r003_event){0};
    return GH_R003_OK;
}
