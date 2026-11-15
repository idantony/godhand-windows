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

static float gate_vector_mul(float a,float b)
{
    volatile float result=a*b;
    return result;
}

static float gate_vector_div(float a,float b)
{
    volatile float result=a/b;
    return result;
}

static float gate_float(const void *p)
{
    float value;
    memcpy(&value,p,sizeof(value));
    return value;
}

static int gate_round(fenv_t *saved)
{
    if(fegetenv(saved))return 0;
    if(!fesetround(FE_TONEAREST))return 1;
    (void)fesetenv(saved);
    return 0;
}

/* 1C7310 and the translational branch of 1C3FC0 write the same position
 * aliases and visit the mounted actors' original +74 virtual in order. */
static gh_scene_status gate_translate(gh_r105_room *state,gh_actor_model *model,
    const float displacement[3],int relative,uint32_t *frontier)
{
    const gh_r105_room_bindings *b=&state->bindings;
    uint8_t *work=state->work.pointer;
    float position[3],mounted_positions[8][3],player_position[3];
    if((work[3]&3u)!=1u){
        *frontier=0x1c7310;
        return GH_SCENE_UNSUPPORTED;
    }
    memcpy(position,model->root.alias_f0,sizeof(position));
    if(state->gate_player_mounted){
        *frontier=0x12ce60;
        if(!b->player->alias_f0||!b->place_mounted_player)return GH_SCENE_UNSUPPORTED;
        memcpy(player_position,b->player->alias_f0,sizeof(player_position));
        for(unsigned i=0;i<3;++i)
            if(!isfinite(player_position[i]))return GH_SCENE_UNSUPPORTED;
    }
    for(unsigned i=0;i<8;++i){
        if(!state->gate_mount[i])continue;
        gh_enemy_ems_live mount;
        if(!b->enemies||gh_enemy_ems_live_at(*b->enemies,state->gate_mount[i],&mount)||
           !mount.enemy202||!mount.actor||!mount.model||!mount.actor->view.alias_f0){
            *frontier=0x1c7310;
            return GH_SCENE_UNSUPPORTED;
        }
        memcpy(mounted_positions[i],mount.actor->view.alias_f0,sizeof(mounted_positions[i]));
    }
    for(unsigned axis=0;axis<3;++axis){
        if(!isfinite(position[axis])||!isfinite(displacement[axis])){
            *frontier=0x1c7310;
            return GH_SCENE_UNSUPPORTED;
        }
        for(unsigned i=0;i<8;++i)
            if(state->gate_mount[i]&&!isfinite(mounted_positions[i][axis])){
                *frontier=0x1c7310;
                return GH_SCENE_UNSUPPORTED;
            }
    }
    fenv_t saved;
    if(!gate_round(&saved)){
        *frontier=0x1c7310;
        return GH_SCENE_UNSUPPORTED;
    }
    for(unsigned axis=0;axis<3;++axis){
        const float delta=relative?displacement[axis]:gate_vector_add(displacement[axis],-position[axis]);
        position[axis]=gate_vector_add(position[axis],delta);
        if(state->gate_player_mounted)
            player_position[axis]=gate_vector_add(player_position[axis],delta);
        for(unsigned i=0;i<8;++i)
            if(state->gate_mount[i])
                mounted_positions[i][axis]=gate_vector_add(mounted_positions[i][axis],delta);
    }
    if(fesetenv(&saved)){
        *frontier=0x1c7310;
        return GH_SCENE_UNSUPPORTED;
    }
    memcpy(model->root.alias_f0,position,sizeof(position));
    if(state->gate_player_mounted){
        uint32_t xyz[3];
        memcpy(xyz,player_position,sizeof(xyz));
        const gh_scene_status status=b->place_mounted_player(b->context,xyz,frontier);
        if(status)return status;
    }
    for(unsigned i=0;i<8;++i){
        if(!state->gate_mount[i])continue;
        if(gh_enemy_ems_set_position_00276228(*b->enemies,state->gate_mount[i],
           mounted_positions[i],frontier))return GH_SCENE_CALLEE;
    }
    return GH_SCENE_OK;
}

/* 1C7138 chooses endpoints. The 1C7CA0 caller then refreshes hierarchies
 * after the 1C7310 position pass, even though R105 disables rotation. */
static gh_scene_status gate_set_reverse(gh_r105_room *state,gh_actor_model *model,
    int reverse,uint32_t *frontier)
{
    uint8_t *work=state->work.pointer;
    float base[3],raised[3],angles[3],turned[3],offset[3],rotation[3];
    memcpy(base,work+0x70,sizeof(base));
    memcpy(angles,work+0x80,sizeof(angles));
    memcpy(offset,work+0x90,sizeof(offset));
    memcpy(rotation,work+0xa0,sizeof(rotation));
    for(unsigned i=0;i<3;++i){
        if(!isfinite(base[i])||!isfinite(angles[i])||!isfinite(offset[i])||!isfinite(rotation[i])){
            *frontier=0x1c7138;
            return GH_SCENE_UNSUPPORTED;
        }
    }
    fenv_t saved;
    if(!gate_round(&saved)){
        *frontier=0x1c7138;
        return GH_SCENE_UNSUPPORTED;
    }
    for(unsigned i=0;i<3;++i){
        raised[i]=gate_vector_add(base[i],offset[i]);
        turned[i]=gate_vector_add(angles[i],rotation[i]);
    }
    if(fesetenv(&saved)){
        *frontier=0x1c7138;
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
    gh_scene_status status=gate_translate(state,model,reverse?raised:base,0,frontier);
    if(status)return status;
    if(gh_actor_model_update_hierarchy_00149eb8(model,GH_STATIC_BINARY32_REFERENCE)){
        *frontier=model->frontier?model->frontier:0x149eb8;
        return GH_SCENE_CALLEE;
    }
    if(state->gate_player_mounted){
        *frontier=0x149eb8;
        if(!state->bindings.refresh_mounted_player)return GH_SCENE_UNSUPPORTED;
        const gh_scene_status status=state->bindings.refresh_mounted_player(state->bindings.context,frontier);
        if(status)return status;
    }
    for(unsigned i=0;i<8;++i){
        if(!state->gate_mount[i])continue;
        gh_enemy_ems_live mount;
        if(gh_enemy_ems_live_at(*state->bindings.enemies,state->gate_mount[i],&mount)||!mount.model){
            *frontier=0x149eb8;
            return GH_SCENE_UNSUPPORTED;
        }
        if(gh_actor_model_update_hierarchy_00149eb8(mount.model,GH_STATIC_BINARY32_REFERENCE)){
            *frontier=mount.model->frontier?mount.model->frontier:0x149eb8;
            return GH_SCENE_CALLEE;
        }
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
        put_word(work+0x2c,0);
        put_word(work+0x30,0x42700000u);
        put_word(work+0x94,0x40c00000u);
        for(unsigned i=0x130;i<=0x13c;i+=4)put_word(work+i,0x3e4ccccdu);
        state->gate=gate;
        state->gate_stage=1;
    }
    if(state->gate_stage==1){
        const gh_scene_status status=gate_set_reverse(state,model,0,frontier);
        if(status)return status;
        static const uint32_t vibration[]={0x41000000,0x41000000,0x40000000,0x3f000000,0x40000000};
        memcpy(work+0x140,vibration,sizeof(vibration));
        work[3]|=8u;
        state->gate_stage=2;
    }
    if(state->gate_stage==2){
        const gh_scene_status status=gate_set_reverse(state,model,1,frontier);
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

/* 1C3FC0, authored R105 translational profile. 1C6CB8 advances the
 * canonical MT twice per visit, including the terminal visit and the
 * discarded angular vibration for this rotation-disabled gate. */
static gh_scene_status step_timed_gate(gh_r105_room *state,gh_actor_model *model,
    int *moving,uint32_t *frontier)
{
    const gh_r105_room_bindings *b=&state->bindings;
    uint8_t *work=state->work.pointer;
    *frontier=0x1c3fc0;
    if(work[2]||work[3]!=9||work[0]>7||!b->random||b->random!=&b->init->random||
       word(work+0x30)!=0x42700000u||word(work+0x90)||
       word(work+0x94)!=0x40c00000u||word(work+0x98)||
       word(work+0x130)!=0x3e4ccccdu||word(work+0x134)!=0x3e4ccccdu)
        return GH_SCENE_UNSUPPORTED;
    float elapsed=gate_float(work+0x2c);
    const float total=gate_float(work+0x30);
    const float enemy_rate=gate_float(b->init->enemies+0x548);
    if(!isfinite(elapsed)||!isfinite(total)||!isfinite(enemy_rate)||enemy_rate<=0||
       !isfinite(b->work->canonical_rate574)||b->work->canonical_rate574<0)
        return GH_SCENE_UNSUPPORTED;
    fenv_t saved;
    if(!gate_round(&saved))return GH_SCENE_UNSUPPORTED;
    elapsed=gate_vector_add(elapsed,gate_vector_mul(enemy_rate,b->work->canonical_rate574));
    if(!isfinite(elapsed)){
        (void)fesetenv(&saved);
        return GH_SCENE_UNSUPPORTED;
    }
    uint32_t raw,angular;
    if(gh_game_mt_next_0030f348(b->random,&raw)||gh_game_mt_next_0030f348(b->random,&angular)){
        (void)fesetenv(&saved);
        *frontier=0x31d020;
        return GH_SCENE_CALLEE;
    }
    const uint32_t jitter_bits=gh_frame_jitter_bits_0031d020_word(raw);
    const float amp=gate_float(work+(elapsed<=gate_float(work+0x140)?0x148:
        elapsed>gate_vector_add(total,-gate_float(work+0x144))?0x150:0x14c));
    const float vibration=gate_vector_mul(0.01f,gate_vector_mul(gate_float(&jitter_bits),amp));
    float start[3],base[3],extra[3]={0,vibration,0};
    memcpy(start,work+0xd0,sizeof(start));
    memcpy(base,work+0xb0,sizeof(base));
    float accel=gate_float(work+0x34),coast=gate_float(work+0x38),decel=gate_float(work+0x3c);
    if(!work[0]){
        const float in=gate_float(work+0x130),out=gate_float(work+0x134);
        const float sum=gate_vector_add(in,out);
        const float rem=gate_vector_add(1,-sum);
        const float ratio=gate_vector_div(rem,gate_vector_add(rem,gate_vector_mul(sum,0.5f)));
        accel=gate_vector_mul(total,in);
        decel=gate_vector_mul(total,out);
        coast=gate_vector_add(total,-gate_vector_add(accel,decel));
        memcpy(work+0x34,&accel,4);
        memcpy(work+0x38,&coast,4);
        memcpy(work+0x3c,&decel,4);
        for(unsigned i=0;i<3;++i){
            const float distance=gate_vector_add(base[i],-start[i]);
            const float middle=gate_vector_mul(distance,ratio);
            const float ramp=gate_vector_add(distance,-middle);
            const float first=gate_vector_mul(ramp,gate_vector_div(in,sum));
            const float last=gate_vector_add(ramp,-first);
            const float acceleration=gate_vector_mul(first,gate_vector_div(2,gate_vector_mul(accel,accel)));
            const float braking=gate_vector_mul(last,gate_vector_div(-2,gate_vector_mul(decel,decel)));
            const float velocity=gate_vector_mul(middle,gate_vector_div(1,coast));
            memcpy(work+0xf0+i*4,&acceleration,4);
            memcpy(work+0x100+i*4,&braking,4);
            memcpy(work+0x50+i*4,&velocity,4);
        }
        elapsed=1;
        work[0]=1;
    }
    memcpy(work+0x2c,&elapsed,4);
    const float coast_end=gate_vector_add(accel,coast);
    if(work[0]==7){
        *moving=0;
        return fesetenv(&saved)?GH_SCENE_UNSUPPORTED:GH_SCENE_OK;
    }
    if(elapsed>gate_vector_add(coast_end,decel)){
        work[0]=7;
        if(fesetenv(&saved))return GH_SCENE_UNSUPPORTED;
        *moving=0;
        return gate_translate(state,model,base,0,frontier);
    }
    work[0]=(uint8_t)(elapsed<=accel?1:elapsed<=coast_end?3:5);
    const float ramp_elapsed=work[0]==1?elapsed:accel;
    const float ramp_sq=gate_vector_mul(gate_vector_mul(ramp_elapsed,ramp_elapsed),0.5f);
    for(unsigned i=0;i<3;++i){
        const float acceleration=gate_float(work+0xf0+i*4);
        const float velocity=gate_float(work+0x50+i*4);
        float shift=gate_vector_mul(acceleration,ramp_sq);
        if(work[0]==3)
            shift=gate_vector_add(shift,gate_vector_mul(velocity,gate_vector_add(elapsed,-accel)));
        else if(work[0]==5){
            const float time=gate_vector_add(elapsed,-coast_end);
            shift=gate_vector_add(shift,gate_vector_mul(velocity,coast));
            shift=gate_vector_add(shift,gate_vector_mul(velocity,time));
            const float braking=gate_float(work+0x100+i*4);
            shift=gate_vector_add(shift,gate_vector_mul(braking,gate_vector_mul(gate_vector_mul(time,time),0.5f)));
        }
        extra[i]=gate_vector_add(shift,extra[i]);
    }
    if(fesetenv(&saved))return GH_SCENE_UNSUPPORTED;
    gh_scene_status status=gate_translate(state,model,start,0,frontier);
    if(!status)status=gate_translate(state,model,extra,1,frontier);
    *moving=1;
    return status;
}

static gh_scene_status resolve_battle_gate(gh_r105_room *state,gh_actor_model **model,
    uint32_t *frontier)
{
    const gh_om_factory *factory=state->bindings.gate_factory;
    const gh_om_object *gate=state->gate;
    *frontier=0x1c7ca0;
    if(!gate||!factory||!factory->bound||gh_om_factory_model_at(factory,gate->handle,model)||
       !(*model)->ready||(*model)->root.bytes!=gate->view.bytes||
       (*model)->root.alias_f0!=gate->view.alias_f0)
        return GH_SCENE_UNSUPPORTED;
    return GH_SCENE_OK;
}

static gh_scene_status reveal_battle_gate(gh_r105_room *state,uint32_t *frontier)
{
    const gh_r105_room_bindings *b=&state->bindings;
    gh_r100_om_view view;
    *frontier=0x14fd98;
    if(!b->object_view||!b->gate_factory)return GH_SCENE_UNSUPPORTED;
    if(gh_r100_find_om_0014fd98(b->gate_factory->bindings.pools,b->object_view,b->context,
       "om021",&view,frontier))return GH_SCENE_CALLEE;
    if(view.bytes){
        if(!state->gate||view.bytes!=state->gate->view.bytes||view.size<0x5a4){
            *frontier=0x14fd98;
            return GH_SCENE_UNSUPPORTED;
        }
        put_word(view.bytes+0x5a0,word(view.bytes+0x5a0)&~1u);
    }
    return GH_SCENE_OK;
}

static gh_scene_status run_timed_gate_00000508(gh_r105_room *state,gh_actor_model *model,
    int reverse,uint32_t *frontier)
{
    const gh_r105_room_bindings *b=&state->bindings;
    gh_scene_status status=gate_set_reverse(state,model,reverse,frontier);
    if(status)return status;
    *frontier=0x2cb8a0;
    if(!b->program_se)return GH_SCENE_UNSUPPORTED;
    if(b->program_se(b->context,0x10,frontier))return GH_SCENE_CALLEE;
    for(;;){
        int moving;
        status=step_timed_gate(state,model,&moving,frontier);
        if(status)return status;
        if(!moving)break;
        if(gh_task_threads_sleep(b->threads,1)){
            *frontier=0x2d5aa0;
            return GH_SCENE_CALLEE;
        }
    }
    *frontier=0x2cb8a0;
    if(reverse&&b->program_se(b->context,0x11,frontier))return GH_SCENE_CALLEE;
    *frontier=0;
    return GH_SCENE_OK;
}

static gh_scene_status move_battle_gate(gh_r105_room *state,uint32_t *frontier)
{
    const gh_r105_room_bindings *b=&state->bindings;
    gh_actor_model *model=NULL;
    gh_scene_status status=resolve_battle_gate(state,&model,frontier);
    if(status)return status;
    status=gate_set_reverse(state,model,0,frontier);
    if(status)return status;
    for(uint8_t stamp=7;stamp<=8;++stamp){
        gh_game_title_player *actor=NULL;
        *frontier=0x290868;
        if(gh_enemy_manager_get_00290868(b->init,stamp,&actor))return GH_SCENE_CALLEE;
        if(!actor)continue;
        uint32_t handle;
        if(gh_enemy_ems_handle_for_actor(*b->enemies,actor,&handle)||!handle||
           actor->view.original_vtable214!=0x4479c8)
            return GH_SCENE_UNSUPPORTED;
        for(unsigned i=0;i<8;++i)
            if(!state->gate_mount[i]){
                state->gate_mount[i]=handle;
                break;
            }
    }
    status=reveal_battle_gate(state,frontier);
    if(status)return status;
    status=run_timed_gate_00000508(state,model,1,frontier);
    if(status)return status;
    memset(state->gate_mount,0,sizeof(state->gate_mount));
    *frontier=0;
    return GH_SCENE_OK;
}

static gh_scene_status finish_battle_gate(gh_r105_room *state,int saved,uint32_t *frontier)
{
    const gh_r105_room_bindings *b=&state->bindings;
    if(saved){
        gh_actor_model *model=NULL;
        gh_scene_status status=resolve_battle_gate(state,&model,frontier);
        if(status)return status;
        status=gate_set_reverse(state,model,0,frontier);
        if(status)return status;
        status=reveal_battle_gate(state,frontier);
        if(status)return status;
    }
    *frontier=0x1332d0;
    if(!b->collision)return GH_SCENE_UNSUPPORTED;
    if(b->collision(b->context,"aaa178",1,frontier))return GH_SCENE_CALLEE;
    gh_at_native_unit jump;
    *frontier=0x2c2ae0;
    if(gh_scene_at_get_jump_002c2ae0(b->at,1,&jump))return GH_SCENE_CALLEE;
    if(jump.bytes){
        int original;
        *frontier=0x2c28f8;
        if(gh_scene_at_set_enabled(b->at,jump,1,&original))return GH_SCENE_CALLEE;
    }
    *frontier=0;
    return GH_SCENE_OK;
}

static gh_scene_status finish_battle_event(gh_r105_room *state,uint32_t *frontier)
{
    const gh_r105_room_bindings *b=&state->bindings;
    float center[3]={0};
    *frontier=0x2c3118;
    gh_at_native_unit unit;
    if(gh_scene_at_find_id_002c3118(b->at,2,&unit))return GH_SCENE_CALLEE;
    *frontier=0x2c0e78;
    if(gh_scene_at_center_002c0e78(unit,center))return GH_SCENE_CALLEE;
    for(uint8_t stamp=7;stamp<=8;++stamp){
        *frontier=0x295ce8;
        if(gh_enemy_ems_start_stamp_action_00295ce8(*b->enemies,b->init,stamp,frontier))
            return GH_SCENE_CALLEE;
        *frontier=0x295f80;
        if(!b->goto_player_00295f80||!b->player->alias_f0)return GH_SCENE_UNSUPPORTED;
        float position[3];
        memcpy(position,b->player->alias_f0,sizeof(position));
        if(b->goto_player_00295f80(b->context,stamp,position,frontier))return GH_SCENE_CALLEE;
        *frontier=0x296070;
        if(gh_enemy_ems_set_stamp_keep_00296070(*b->enemies,b->init,stamp,center,frontier))
            return GH_SCENE_CALLEE;
    }
    if(!b->camera_ended){*frontier=0x2c4078;return GH_SCENE_UNSUPPORTED;}
    for(;;){
        int ended=0;
        *frontier=0x2c4040;
        if(b->camera_ended(b->context,&ended,frontier))return GH_SCENE_CALLEE;
        if(ended)break;
        if(gh_task_threads_sleep(b->threads,1)){
            *frontier=0x2d5aa0;
            return GH_SCENE_CALLEE;
        }
    }
    *frontier=0x2c3c10;
    if(!b->end_soft_event)return GH_SCENE_UNSUPPORTED;
    if(b->end_soft_event(b->context,&state->battle_soft_end,frontier))return GH_SCENE_CALLEE;
    return finish_battle_gate(state,0,frontier);
}

static void watch_battle(void *context,int32_t argument)
{
    gh_r105_room *state=context;
    const gh_r105_room_bindings *b=&state->bindings;
    (void)argument;
    uint32_t frontier=0;
    if(word(b->room->active+4)&0x10000000u){
        if(finish_battle_gate(state,1,&frontier))
            state->watcher_frontier[1]=frontier?frontier:0x1c7ca0;
        else state->watcher_slot[1]=-1;
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
        break;
    }
    put_word(b->room->active+4,word(b->room->active+4)|0x10000000u);
    if(!b->start_soft_event||b->start_soft_event(b->context,&state->battle_soft_start,0,&frontier)){
        state->watcher_frontier[1]=frontier?frontier:0x2c39b0;
        return;
    }
    if(!b->set_room_camera||b->set_room_camera(b->context,&state->battle_camera,"A00",&frontier)){
        state->watcher_frontier[1]=frontier?frontier:0x2c3ed8;
        return;
    }
    for(unsigned i=0;i<2;i++){
        gh_game_title_player *actor;
        if(!b->enemies||!*b->enemies||
           gh_enemy_ems_set_002952d0(*b->enemies,&state->battle_spawn[i],(uint8_t)(7+i),&actor,&frontier)){
            state->watcher_frontier[1]=frontier?frontier:0x2952d0;
            return;
        }
    }
    for(uint8_t stamp=7;stamp<=8;stamp++){
        if(gh_enemy_ems_set_stamp_suspend_00295bc0(*b->enemies,b->init,stamp,0,&frontier)){
            state->watcher_frontier[1]=frontier?frontier:0x295bc0;
            return;
        }
    }
    for(uint8_t stamp=7;stamp<=8;stamp++){
        if(gh_enemy_ems_set_stamp_alpha_00296230(*b->enemies,b->init,stamp,0,&frontier)){
            state->watcher_frontier[1]=frontier?frontier:0x296230;
            return;
        }
    }
    if(move_battle_gate(state,&frontier)){
        state->watcher_frontier[1]=frontier?frontier:0x1c7ca0;
        return;
    }
    if(gh_task_threads_sleep(b->threads,15)){
        state->watcher_frontier[1]=0x2d5aa0;
        return;
    }
    if(!b->program_se||b->program_se(b->context,0x32,&frontier)){
        state->watcher_frontier[1]=frontier?frontier:0x2cb8a0;
        return;
    }
    if(finish_battle_event(state,&frontier)){
        state->watcher_frontier[1]=frontier?frontier:0x2c0e78;
        return;
    }
    state->watcher_slot[1]=-1;
}

static void jump_gate_task_00000508(void *context,int32_t argument)
{
    gh_r105_room *state=context;
    uint32_t frontier=0;
    if(argument){
        state->jump_frontier=0x508;
        return;
    }
    gh_actor_model *model=NULL;
    if(resolve_battle_gate(state,&model,&frontier)||run_timed_gate_00000508(state,model,0,&frontier)){
        state->jump_frontier=frontier?frontier:0x1c3f38;
        return;
    }
    state->jump_gate_pending=0;
}

static gh_scene_status jump_fade_00307208(const gh_r105_room_bindings *b,uint32_t *frontier)
{
    *frontier=0x307208;
    if(!b->fade||!b->display||!b->display->initialized)return GH_SCENE_UNSUPPORTED;
    const uint8_t *work=(const uint8_t *)b->work;
    const gh_screen_fade_context display={b->display->screen_width,b->display->gs_y_origin,
        word(work+0x564),word(work+0x5e0)};
    if(gh_screen_fade_start_00307208(b->fade,&display,0,10,0,0,0xff000000u,30))
        return GH_SCENE_CALLEE;
    *frontier=0;
    return GH_SCENE_OK;
}

static gh_scene_status jump_room_000008dc(gh_r105_room *state,uint32_t *frontier)
{
    const gh_r105_room_bindings *b=&state->bindings;
    uint8_t *room_flags=b->room->active+4;
    if(word(room_flags)&0x80000000u){
        *frontier=0x2c2870;
        return GH_SCENE_UNSUPPORTED;
    }

    *frontier=0x1598;
    if(!b->reserve||!b->request_room)return GH_SCENE_UNSUPPORTED;
    put_word(room_flags,word(room_flags)|0x80000000u);
    *b->reserve=(gh_event_reserve_work){0};
    b->reserve->mode=4;
    b->reserve->destination_room=0x102;
    const uint32_t xyz[]={0x4180a3d7,0x3e051eb8,0x41c1999a},yaw=0xc048f5c3;
    memcpy(b->reserve->position,xyz,sizeof(xyz));
    memcpy(&b->reserve->yaw,&yaw,sizeof(yaw));
    b->reserve->flags=0x13;
    const float zero[3]={0};
    *frontier=0x2c4548;
    if(b->request_room(b->context,2,zero,0,0,0,1,frontier))return GH_SCENE_CALLEE;
    *frontier=0;
    return GH_SCENE_OK;
}

static void jump_exit_00000768(void *context,int32_t argument)
{
    gh_r105_room *state=context;
    const gh_r105_room_bindings *b=&state->bindings;
    uint32_t frontier=0;
    state->jump_frontier=0x768;
    if(argument)return;

    gh_actor_model *model=NULL;
    if(resolve_battle_gate(state,&model,&frontier)||gate_set_reverse(state,model,0,&frontier)){
        state->jump_frontier=frontier?frontier:0x1c7ca0;
        return;
    }
    if(!b->start_soft_event||b->start_soft_event(b->context,&state->jump_soft_start,0,&frontier)){
        state->jump_frontier=frontier?frontier:0x2c39b0;
        return;
    }
    if(!b->set_room_camera||b->set_room_camera(b->context,&state->jump_camera,"A01",&frontier)){
        state->jump_frontier=frontier?frontier:0x2c3ed8;
        return;
    }
    if(!b->jump_unsuspend_player||b->jump_unsuspend_player(b->context,&frontier)){
        state->jump_frontier=frontier?frontier:0x12c7d0;
        return;
    }
    uint8_t *player=b->player->bytes;
    if(word(player+0x698)||word(player+0x6a0)){
        state->jump_frontier=word(player+0x698)?0x12ac60:0x12b9f0;
        return;
    }
    /* 12C0D8 writes the original PL00 event-wait selector. */
    memcpy(player+0x2f4,(const uint8_t[]){0,0x45,0,0},4);
    for(unsigned i=0;i<8;++i){
        if(state->gate_mount[i]){
            state->jump_frontier=0x834;
            return;
        }
    }
    state->gate_player_mounted=1;
    state->jump_frontier=0;
    if(gh_scenario_exec_002c3890(b->scenario,b->threads,-1,jump_gate_task_00000508,
       state,0,&state->jump_gate_slot,&frontier)){
        state->jump_frontier=frontier?frontier:0x2c3920;
        return;
    }
    state->jump_gate_pending=(uint8_t)(state->jump_gate_slot>=0);
    if(gh_task_threads_sleep(b->threads,30)){
        state->jump_frontier=0x2d5aa0;
        return;
    }
    if(jump_fade_00307208(b,&frontier)){
        state->jump_frontier=frontier?frontier:0x307208;
        return;
    }
    if(gh_task_threads_sleep(b->threads,35)){
        state->jump_frontier=0x2d5aa0;
        return;
    }
    if(!b->end_soft_event||b->end_soft_event(b->context,&state->jump_soft_end,&frontier)){
        state->jump_frontier=frontier?frontier:0x2c3c10;
        return;
    }
    if(jump_room_000008dc(state,&frontier)){
        state->jump_frontier=frontier?frontier:0x8dc;
        return;
    }
    state->jump_pending=0;
}

static gh_scene_at_status resolve_exit_jump(gh_r105_room *state,gh_at_native_unit *jump,
    uint32_t *frontier)
{
    *frontier=0x2c2ae0;
    if(!state->ready||!state->bindings.at||state->bindings.work->room!=0x105)
        return GH_AT_ARGUMENT;
    gh_scene_at_status status=gh_scene_at_get_jump_002c2ae0(state->bindings.at,1,jump);
    if(status)return status;
    *frontier=0x2c2138;
    if(!jump->bytes||jump->size!=0x9c||!(jump->bytes[0x34]&1)||jump->bytes[0x35]!=1||
       jump->bytes[0x44]!=0xff||word(jump->bytes+0x3c)!=0x768||word(jump->bytes+0x40))
        return GH_AT_UNSUPPORTED;
    *frontier=0;
    return GH_AT_OK;
}

gh_scene_at_status gh_r105_jump_offer_001f7858(gh_r105_room *state,
    const gh_scene_at_task_request *request,uint32_t *frontier)
{
    if(!state||!request||!frontier)return GH_AT_ARGUMENT;
    *frontier=0x1f7858;
    if(request->entry_address!=0x2c2138||request->argument_mode!=2||
       request->raw_argument||request->group!=4||request->mode!=0x20||!state->bindings.buttons)
        return GH_AT_UNSUPPORTED;
    gh_at_native_unit jump;
    gh_scene_at_status status=resolve_exit_jump(state,&jump,frontier);
    if(status)return status;
    *frontier=0x1f7858;
    if(request->unit.handle!=jump.handle||request->unit.bytes!=jump.bytes||
       request->unit.size!=jump.size||jump.bytes[0x3b]!=8)
        return GH_AT_UNSUPPORTED;
    const gh_action_set_request action={.priority=request->group,.life=request->mode,
        .callback_id=0x2c2138,.argument=jump.bytes,.kind=2};
    int emitted;
    unsigned index;
    gh_action_set_status result=gh_action_button_set_001f7858(state->bindings.buttons,&action,
        &emitted,&index,frontier);
    return result?GH_AT_UNSUPPORTED:GH_AT_OK;
}

gh_action_step_status gh_r105_jump_action_002c2138(gh_r105_room *state,
    void *unit_argument,uint32_t *frontier)
{
    if(!state||!frontier)return GH_ACTION_STEP_ARGUMENT;
    *frontier=0x2c2138;
    const gh_r105_room_bindings *b=&state->bindings;
    if(state->jump_pending||!b->scenario.bytes||b->scenario.size<0x54||!b->threads)
        return GH_ACTION_STEP_UNSUPPORTED;
    gh_at_native_unit jump;
    if(resolve_exit_jump(state,&jump,frontier)||jump.bytes!=unit_argument||jump.bytes[0x3b]!=8){
        if(!*frontier)*frontier=0x2c2138;
        return GH_ACTION_STEP_UNSUPPORTED;
    }
    if(gh_scenario_clear_pad_0031b410(b->work)){
        *frontier=0x31b410;
        return GH_ACTION_STEP_CALLEE;
    }
    put_word((uint8_t *)b->work+8,0);
    put_word(jump.bytes+0x50,word(jump.bytes+0x50)|0x02000000u);
    int32_t slot;
    if(gh_scenario_exec_002c3890(b->scenario,b->threads,-1,jump_exit_00000768,state,0,&slot,frontier)){
        if(!*frontier)*frontier=0x2c38d8;
        return GH_ACTION_STEP_CALLEE;
    }
    const size_t attribute=slot<0?0x50:0x58+(size_t)slot*8;
    if(attribute>b->scenario.size||b->scenario.size-attribute<4){
        *frontier=0x2c2180;
        return GH_ACTION_STEP_BOUNDS;
    }
    put_word(b->scenario.bytes+attribute,0x747618);
    state->jump_slot=slot;
    state->jump_pending=(uint8_t)(slot>=0);
    *frontier=0;
    return GH_ACTION_STEP_OK;
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
        const int32_t exit_tasks[]={state->jump_pending?state->jump_slot:-1,
            state->jump_gate_pending?state->jump_gate_slot:-1};
        for(unsigned i=0;i<2;++i){
            const int32_t slot=exit_tasks[i];
            if(slot>=0&&((size_t)slot>=manager->count||manager->records[slot].running))
                return GH_SCENE_UNSUPPORTED;
        }
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
    if(state->jump_frontier){
        *frontier=state->jump_frontier;
        return GH_SCENE_UNSUPPORTED;
    }
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
