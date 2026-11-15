#include "r105_room.h"
#include "../core/game_enemy_ems.h"
#include "../core/game_om_frame.h"
#include "r100_counter_task.h"
#include "r100_scripts.h"
#include "../core/game_scenario_exec.h"
#include <fenv.h>
#include <math.h>
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

static float gate_vector_add(float a,float b)
{
    volatile float result=a+b;
    return result;
}

/* 1C7138 chooses the end points and 1C7310 applies the translation to the
 * sole bound root. The eight associated actor slots are empty on REL 05F8. */
static gh_scene_status gate_set_reverse(uint8_t *work,gh_actor_model *model,int reverse,
    uint32_t *frontier)
{
    float base[3],raised[3],current[3],position[3],angles[3],turned[3],offset[3],rotation[3];
    memcpy(base,work+0x70,sizeof(base));
    memcpy(current,model->root.alias_f0,sizeof(current));
    memcpy(angles,work+0x80,sizeof(angles));
    memcpy(offset,work+0x90,sizeof(offset));
    memcpy(rotation,work+0xa0,sizeof(rotation));
    for(unsigned i=0;i<3;++i){
        if(!isfinite(base[i])||!isfinite(current[i])||!isfinite(angles[i])||
           !isfinite(offset[i])||!isfinite(rotation[i])){
            *frontier=0x1c7138;
            return GH_SCENE_UNSUPPORTED;
        }
    }
    fenv_t saved;
    if(fegetenv(&saved)||fesetround(FE_TONEAREST)){
        *frontier=0x1c7138;
        return GH_SCENE_UNSUPPORTED;
    }
    for(unsigned i=0;i<3;++i){
        raised[i]=gate_vector_add(base[i],offset[i]);
        turned[i]=gate_vector_add(angles[i],rotation[i]);
        const float wanted=reverse?raised[i]:base[i];
        position[i]=gate_vector_add(current[i],gate_vector_add(wanted,-current[i]));
    }
    if(fesetenv(&saved)){
        *frontier=0x1c7310;
        return GH_SCENE_UNSUPPORTED;
    }
    memcpy(work+0xb0,reverse?base:raised,sizeof(base));
    memcpy(work+0xd0,reverse?raised:base,sizeof(base));
    memcpy(work+0xc0,reverse?angles:turned,sizeof(angles));
    memcpy(work+0xe0,reverse?turned:angles,sizeof(angles));
    work[0]=0;
    work[1]=(uint8_t)reverse;
    put_word(work+0x130,word(work+(reverse?0x13c:0x138)));
    put_word(work+0x134,word(work+(reverse?0x138:0x13c)));
    memcpy(model->root.alias_f0,position,sizeof(position));
    if(gh_actor_model_update_hierarchy_00149eb8(model,GH_STATIC_BINARY32_REFERENCE)){
        *frontier=model->frontier?model->frontier:0x149eb8;
        return GH_SCENE_CALLEE;
    }
    return GH_SCENE_OK;
}

gh_scene_status gh_r105_gate_000005f8(gh_r105_room *state,const gh_om_factory *factory,
    gh_r100_om_scalar_view object_view,
    gh_r100_init_status (*collision)(void *,const char *,int,uint32_t *),
    void *context,uint32_t *frontier)
{
    if(!frontier)return GH_SCENE_ARGUMENT;
    *frontier=0x5f8;
    if(!state||!state->work.pointer||!state->bindings.at||
       !factory||!factory->bound||factory->bindings.work!=state->bindings.work||
       factory->bindings.room_state!=state->bindings.room)return GH_SCENE_ARGUMENT;
    gh_r100_om_view view;
    if(gh_r100_find_om_0014fd98(factory->bindings.pools,object_view,context,"om021",&view,frontier))
        return GH_SCENE_CALLEE;
    if(!view.bytes&&!state->gate)return GH_SCENE_OK;
    const gh_om_object *gate=NULL;
    for(size_t i=0;i<factory->count;++i){
        const gh_om_object *candidate=&factory->objects[i];
        if(candidate->view.bytes==view.bytes){gate=candidate;break;}
    }
    gh_actor_model *model=NULL;
    if(!gate||!gate->registered||gate->stage!=7||gate->source.type!=0x355||
       gate->view.size!=view.size||gate->view.size<0x5a4||
       (state->gate&&state->gate!=gate)||gh_om_factory_model_at(factory,gate->handle,&model)||
       !model->ready||model->root.bytes!=view.bytes||model->root.alias_f0!=gate->view.alias_f0){
        *frontier=0x1c6898;
        return GH_SCENE_UNSUPPORTED;
    }
    uint8_t *work=state->work.pointer;
    if(!state->gate_stage){
        if(!collision){*frontier=0x1332d0;return GH_SCENE_UNSUPPORTED;}
        for(unsigned i=0x0c;i<=0x28;i+=4)
            if(word(work+i)){*frontier=0x1c7310;return GH_SCENE_UNSUPPORTED;}
        memcpy(work+0x70,model->root.alias_f0,12);
        memcpy(work+0x80,view.bytes+0x100,12);
        memcpy(work+0xa0,work+0x80,12); /* 1C6968 passes the saved Euler. */
        work[2]=0;
        work[3]=1;
        put_word(work+0x30,0x42700000u);
        put_word(work+0x94,0x40c00000u);
        for(unsigned i=0x130;i<=0x13c;i+=4)put_word(work+i,0x3e4ccccdu);
        state->gate=gate;
        state->gate_stage=1;
    }
    if(state->gate_stage==1){
        const gh_scene_status status=gate_set_reverse(work,model,0,frontier);
        if(status)return status;
        static const uint32_t vibration[]={0x41000000,0x41000000,0x40000000,0x3f000000,0x40000000};
        memcpy(work+0x140,vibration,sizeof(vibration));
        work[3]|=8u;
        state->gate_stage=2;
    }
    if(state->gate_stage==2){
        const gh_scene_status status=gate_set_reverse(work,model,1,frontier);
        if(status)return status;
        put_word(gate->view.bytes+0x5a0,word(view.bytes+0x5a0)|1u);
        state->gate_stage=3;
    }
    if(state->gate_stage==3){
        *frontier=0x1332d0;
        if(!collision)return GH_SCENE_UNSUPPORTED;
        if(collision(context,"aaa178",0,frontier)){
            if(!*frontier)*frontier=0x1332d0;
            return GH_SCENE_CALLEE;
        }
        state->gate_stage=4;
    }
    if(state->gate_stage==4){
        gh_at_native_unit jump;
        gh_scene_at_manager *at=state->bindings.at;
        int original;
        *frontier=0x2c2ae0;
        if(gh_scene_at_get_jump_002c2ae0(at,1,&jump))return GH_SCENE_CALLEE;
        *frontier=0x2c2750;
        if(gh_scene_at_set_jump_info_002c2750(at,jump,0xff,0x768,0,0))return GH_SCENE_CALLEE;
        *frontier=0x2c29a0;
        if(gh_scene_at_set_enabled(at,jump,0,&original))return GH_SCENE_CALLEE;
        state->gate_stage=5;
    }
    *frontier=0;
    return GH_SCENE_OK;
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
    const gh_r105_room_bindings *b=&state->bindings;
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
        if(hit)break;
    }
    if(!b->goto_player_00295f80||!b->player->alias_f0){
        state->watcher_frontier[0]=!b->player->alias_f0?0x2db6b0:0x295f80;
        return;
    }
    for(uint8_t stamp=3;stamp<=5;stamp+=2){
        float position[3];
        uint32_t frontier=0;
        memcpy(position,b->player->alias_f0,sizeof(position));
        if(b->goto_player_00295f80(b->context,stamp,position,&frontier)){
            state->watcher_frontier[0]=frontier?frontier:0x295f80;
            return;
        }
    }
    state->watcher_slot[0]=-1;
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
