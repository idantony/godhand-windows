#include "r105_room.h"
#include "../core/game_enemy_ems.h"
#include "r100_counter_task.h"
#include "r100_scripts.h"
#include "../core/game_scenario_exec.h"
#include <string.h>

static uint32_t word(const void *p)
{
    uint32_t value;
    memcpy(&value,p,4);
    return value;
}

static void put_word(void *p,uint32_t value)
{
    memcpy(p,&value,4);
}

static gh_scene_status stop(gh_r105_room *state,uint32_t frontier,gh_scene_status status)
{
    state->frontier=frontier;
    return status;
}

static void initialize_work(uint8_t *work)
{
    static const uint32_t origin[4]={0,0,0,0x3f800000};
    for(size_t offset=0x50;offset<=0x120;offset+=16)
        memcpy(work+offset,origin,sizeof(origin));
    memset(work,0,3);
    memset(work+4,0,0x28);
    memset(work+0x30,0,0x10);
    memset(work+0x130,0,0x10);
}

static int at_hit(gh_r105_room *state,unsigned id,int *inside)
{
    gh_at_native_unit unit;
    const gh_scene_at_status status=gh_scene_at_find_id_002c3118(state->bindings.at,id,&unit);
    if(status)return 0;
    *inside=unit.bytes&&(word(unit.bytes+0x50)&0x80000000u);
    return 1;
}

static int enemy_dead(gh_r105_room *state,uint8_t stamp,int *dead)
{
    const gh_r105_room_bindings *b=&state->bindings;
    gh_game_title_player *actor;
    if(gh_enemy_manager_get_00290868(b->init,stamp,&actor))return 0;
    if(!actor){
        *dead=1;
        return 1;
    }
    return !gh_r100_enemy_dead_0028fb38(b->room,actor,dead);
}

static void watch_at0(void *context,int32_t argument)
{
    gh_r105_room *state=context;
    (void)argument;
    for(;;){
        if(gh_task_threads_sleep(state->bindings.threads,1)){
            state->watcher_frontier[0]=0x2d5aa0;
            return;
        }
        int hit;
        if(!at_hit(state,0,&hit)){
            state->watcher_frontier[0]=0x2c2a98;
            return;
        }
        if(hit){
            state->watcher_frontier[0]=0x295f80;
            return;
        }
    }
}

static void watch_battle(void *context,int32_t argument)
{
    gh_r105_room *state=context;
    const gh_r105_room_bindings *b=&state->bindings;
    (void)argument;
    if(word(b->room->active+4)&0x10000000u){
        state->watcher_frontier[1]=0x1c7ca0;
        return;
    }
    for(;;){
        if(gh_task_threads_sleep(b->threads,1)){
            state->watcher_frontier[1]=0x2d5aa0;
            return;
        }
        int dead3,dead5;
        if(!enemy_dead(state,3,&dead3)||!enemy_dead(state,5,&dead5)){
            state->watcher_frontier[1]=0x295ca8;
            return;
        }
        if(!dead3&&!dead5)continue;
        int hit;
        if(!at_hit(state,3,&hit)){
            state->watcher_frontier[1]=0x2c2a98;
            return;
        }
        if(hit||(word((const uint8_t*)b->work+0x5e4)&0x40000000u))continue;
        const uint8_t *player=b->player->bytes;
        if(!(word(player+0x15f4)&0x400u)&&(player[0x2f4]||player[0x2f5]>=4))continue;
        put_word(b->room->active+4,word(b->room->active+4)|0x10000000u);
        state->watcher_frontier[1]=0x2c39b0;
        return;
    }
}

gh_scene_status gh_r105_initialize_000001a8(gh_r105_room *state,const gh_r105_room_bindings *b)
{
    if(!state||!b||!b->module||!b->module->bound||b->module->room!=0x105||
       b->module->init_offset!=0x1a8||b->module->move_offset!=0x398||!b->heap||
       !b->work||b->work->room!=0x105||!b->room||!b->room->active||b->room->size<8||
       !b->threads||!b->at||!b->at->ready||!b->init||!b->player||!b->player->bytes||
       b->player->size<0x15f8||!b->save.bytes||b->save.size<GH_CORE_SAVE_BYTES)
        return GH_SCENE_ARGUMENT;
    if(state->work.pointer&&(state->bindings.module!=b->module||state->bindings.heap!=b->heap||
       state->bindings.work!=b->work||state->bindings.room!=b->room||state->bindings.threads!=b->threads||
       state->bindings.at!=b->at||state->bindings.save.bytes!=b->save.bytes||
       state->bindings.scenario.manager!=b->scenario.manager||state->bindings.scenario.bytes!=b->scenario.bytes||
       state->bindings.init!=b->init||state->bindings.player!=b->player))
        return GH_SCENE_ARGUMENT;
    if(state->ready)return GH_SCENE_OK;
    state->bindings=*b; /* A missing leaf can be supplied when retrying its stage. */
    if(!state->work.pointer){
        if(gh_heap_allocate(b->heap,0x170,16,&state->work))
            return stop(state,0x30ee08,GH_SCENE_CALLEE);
        initialize_work(state->work.pointer);
        state->watcher_slot[0]=state->watcher_slot[1]=-1;
        state->stage=1;
    }
    static const gh_native_task_entry entries[2]={watch_at0,watch_battle};
    while(state->stage<=2){
        const unsigned i=state->stage-1;
        uint32_t frontier=0;
        if(gh_scenario_exec_002c3890(b->scenario,b->threads,-1,entries[i],state,0,
            &state->watcher_slot[i],&frontier))
            return stop(state,frontier?frontier:0x2c3890,GH_SCENE_CALLEE);
        ++state->stage;
    }
    if(state->stage==3){
        const uint32_t flags=word(b->room->active+4);
        put_word(b->room->active+4,flags|0x40000000u);
        state->stage=(flags&0x40000000u)||(b->work->flags&0x04000000u)?5:4;
    }
    if(state->stage==4){
        if(!b->paper_0012c8d8)return stop(state,0x12c8d8,GH_SCENE_UNSUPPORTED);
        uint32_t frontier=0;
        const gh_scene_status status=b->paper_0012c8d8(b->context,&frontier);
        if(status)return stop(state,frontier?frontier:0x12c8d8,status);
        state->stage=5;
    }
    if(state->stage==5){
        gh_at_native_unit unit;
        if(gh_scene_at_find_id_002c3118(b->at,4,&unit)||
           gh_scene_at_set_jump_info_002c2750(b->at,unit,0xff,0x488,0,0))
            return stop(state,0x2c2790,GH_SCENE_CALLEE);
        state->stage=6;
    }
    if(state->stage==6){
        if(!b->omsub_000005f8)return stop(state,0x5f8,GH_SCENE_UNSUPPORTED);
        uint32_t frontier=0;
        const gh_scene_status status=b->omsub_000005f8(b->context,state->work.pointer,&frontier);
        if(status)return stop(state,frontier?frontier:0x5f8,status);
        static const uint32_t wind[]={0,0x3f000000,0x3c23d70a};
        memcpy((uint8_t*)state->work.pointer+0x160,wind,sizeof(wind));
        state->stage=7;
    }
    int16_t clear_count;
    if(gh_r100_get_clear_num_001fa6c0(b->save,b->work,&clear_count))
        return stop(state,0x1fa6c0,GH_SCENE_CALLEE);
    if(!clear_count&&gh_core_save_difficulty(b->save)!=2){
        if(!b->tutorial_000003b8)return stop(state,0x3b8,GH_SCENE_UNSUPPORTED);
        uint32_t frontier=0;
        const gh_scene_status status=b->tutorial_000003b8(b->context,&frontier);
        if(status)return stop(state,frontier?frontier:0x3b8,status);
    }
    state->stage=8;
    state->ready=1;
    state->frontier=0;
    return GH_SCENE_OK;
}

gh_scene_status gh_r105_room_release(gh_r105_room *state)
{
    if(!state)return GH_SCENE_ARGUMENT;
    if(state->work.pointer){
        const gh_task_manager *manager=state->bindings.scenario.manager;
        if(!manager||!manager->records)return GH_SCENE_ARGUMENT;
        for(unsigned i=0;i<2;i++){
            const int32_t slot=state->watcher_slot[i];
            if(slot>=0&&((size_t)slot>=manager->count||manager->records[slot].running))
                return GH_SCENE_UNSUPPORTED;
        }
        if(gh_heap_release(state->bindings.heap,state->work))return GH_SCENE_CALLEE;
    }
    *state=(gh_r105_room){0};
    return GH_SCENE_OK;
}


gh_scene_status gh_r105_move_00000398(gh_r105_room *state,const gh_om_factory *factory,
    gh_game_mt *mt,gh_frame_random *random,gh_r100_om_scalar_view other_view,
    void *other_context,uint32_t *frontier)
{
    if(!frontier)return GH_SCENE_ARGUMENT;
    *frontier=0x398;
    if(!state||!state->ready||!state->work.pointer)return GH_SCENE_ARGUMENT;
    for(unsigned i=0;i<2;++i){
        if(state->watcher_frontier[i]){
            *frontier=state->watcher_frontier[i];
            return GH_SCENE_UNSUPPORTED;
        }
    }
    const gh_r003_status result=gh_room_move_flag_wind(state->bindings.work,
        (uint8_t *)state->work.pointer+0x160,factory,mt,random,other_view,other_context,0x10f8,frontier);
    if(!result)return GH_SCENE_OK;
    return result==GH_R003_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
}
