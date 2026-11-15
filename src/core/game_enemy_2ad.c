#include "game_actor_motion_step.h"
#include "game_enemy_2ad.h"
#include "game_player_targets.h"
#include "id_layout_rotation.h"
#include "../rooms/r100_counter_task.h"
#include <fenv.h>
#include <math.h>
#include <string.h>

static uint32_t read_word(const uint8_t *p)
{
    uint32_t value;
    memcpy(&value,p,sizeof(value));
    return value;
}

static void put_word(uint8_t *p,uint32_t value)
{
    memcpy(p,&value,sizeof(value));
}

gh_title_player_status gh_enemy2ad_construct_0028e018(void *raw,size_t size,gh_title_player_view *out)
{
    if(!raw||!out)return GH_PLAYER_ARGUMENT;
    if(size<GH_EM2AD_BYTES)return GH_PLAYER_BOUNDS;
    gh_title_player_view view;
    gh_title_player_status status=gh_enemy_base_construct_0028eb00(raw,size,&view);
    if(status)return status;
    uint8_t *p=raw;
    put_word(p+0x214,0x44a528);
    memset(p+0x1520,0,12);
    put_word(p+0x152c,0x3f800000);
    p[0x1541]=0xff;
    put_word(p+0x4ac,0x44a510);
    view.original_vtable214=0x44a528;
    view.original_kind4ac=0x44a510;
    *out=view;
    return GH_PLAYER_OK;
}

gh_title_player_status gh_enemy2ad_allocate_00313970(gh_game_title_player *actor,gh_heap *room_heap,
    const gh_boot_work *work,int *original_nonnull)
{
    if(!actor||!room_heap||!work||!original_nonnull)return GH_PLAYER_ARGUMENT;
    *original_nonnull=0;
    if(actor->allocation.pointer||actor->arena_allocation.pointer)return GH_PLAYER_BUSY;
    if(read_word((const uint8_t *)work+0x594)&0x100000u)return GH_PLAYER_OK;
    gh_r207_allocation allocation;
    gh_mem_error allocated=gh_heap_allocate(room_heap,GH_EM2AD_BYTES,16,&allocation);
    if(allocated==GH_MEM_CAPACITY)return GH_PLAYER_OK;
    if(allocated)return GH_PLAYER_MEMORY;
    memset(allocation.pointer,0,GH_EM2AD_BYTES);
    gh_title_player_view view;
    gh_title_player_status status=gh_enemy2ad_construct_0028e018(allocation.pointer,
        GH_EM2AD_BYTES,&view);
    if(status){
        (void)gh_heap_release(room_heap,allocation);
        return status;
    }
    view.initial_memory=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;
    *actor=(gh_game_title_player){.allocation=allocation,.heap=room_heap,.view=view,.ready=1,
        .allocation_domain=GH_PLAYER_ALLOCATION_DEFAULT_HEAP};
    *original_nonnull=1;
    return GH_PLAYER_OK;
}

gh_effect_status gh_enemy2ad_effect_data(const gh_object_view *resource,gh_effect_view *out)
{
    if(!resource||!out||!resource->bytes||resource->kind!=0x2ad)return GH_EFFECT_ARGUMENT;
    if(!resource->file_extent_known||resource->file_extent>resource->size||resource->file_extent<0x14)
        return GH_EFFECT_BOUNDS;
    const uint32_t offset=read_word(resource->bytes+0x10);
    if(offset<0x14||offset>=resource->file_extent)return GH_EFFECT_BOUNDS;
    *out=(gh_effect_view){resource->bytes+offset,resource->file_extent-offset};
    return GH_EFFECT_OK;
}

gh_actor_model_status gh_enemy2ad_model_0028ea38(gh_enemy_model *state,const gh_title_player_view *actor,
    const gh_object_view *resource,gh_boot_work *work,gh_room_pools *pools,gh_heap *heap,
    const void *fog,gh_static_transform_policy policy,int *original_return)
{
    if(!state||!actor||!actor->bytes||actor->size<GH_EM2AD_BYTES||!resource||!resource->bytes||
       !work||!pools||!heap||!fog||!original_return)return GH_ACTOR_MODEL_ARGUMENT;
    *original_return=0;
    if(actor->original_vtable214!=0x44a528||resource->kind!=0x2ad||
       !resource->file_extent_known||resource->file_extent>resource->size)return GH_ACTOR_MODEL_ARGUMENT;
    if(state->stage&&(state->resource!=resource->bytes||
       (state->model.bound&&state->model.root.bytes!=actor->bytes)))return GH_ACTOR_MODEL_BUSY;
    if(!state->stage){
        gh_bundle bundle;
        gh_bundle_entry geometry={0},texture={0};
        gh_bundle_error status=gh_bundle_open(resource->bytes,resource->file_extent,&bundle);
        if(!status)status=gh_bundle_find(&bundle,"MD",0,&geometry);
        if(status&&status!=GH_BUNDLE_NOT_FOUND){
            state->frontier=0x31ccf0;
            return GH_ACTOR_MODEL_BOUNDS;
        }
        status=gh_bundle_find(&bundle,"TM3",0,&texture);
        if(status&&status!=GH_BUNDLE_NOT_FOUND){
            state->frontier=0x31ccf0;
            return GH_ACTOR_MODEL_BOUNDS;
        }
        state->geometry=geometry;state->texture=texture;
        state->resource=resource->bytes;state->stage=1;
    }
    state->frontier=0;
    if(!state->texture.bytes){state->stage=2;return GH_ACTOR_MODEL_OK;}
    gh_actor_model_status status=gh_actor_model_bind_packet_inputs(&state->model,heap,fog);
    if(status)return status;
    status=gh_actor_model_initialize_00149738(&state->model,actor,work,pools,
        state->geometry.bytes?&state->geometry:NULL,&state->texture,0,0,policy,original_return);
    state->frontier=state->model.frontier;
    if(!status)state->stage=2;
    return status;
}

gh_actor_model_status gh_enemy2ad_init_prefix_0028e0e0(gh_enemy2ad_init *state,
    const gh_title_player_view *actor,const gh_enemy_request *request,const gh_object_view *resource,
    gh_boot_work *work,gh_core_save_view save,gh_game_effects *effects,gh_room_pools *pools,
    gh_heap *heap,const void *fog,gh_game_cockpit *cockpit,gh_game_mt *random,
    gh_static_transform_policy policy,int *original)
{
    if(!state||!actor||!actor->bytes||actor->size<GH_EM2AD_BYTES||!request||!resource||
       !work||!original||!save.bytes||save.size<0x18||!effects)return GH_ACTOR_MODEL_ARGUMENT;
    *original=0;
    if(work->room!=0x105||actor->original_vtable214!=0x44a528||request->object_kind!=0x2ad||
       request->dependency_kind!=0x2ad||request->entry_number!=63){
        state->frontier=0x28e0e0;
        return GH_ACTOR_MODEL_UNSUPPORTED;
    }
    if(state->stage&&(state->actor!=actor||state->resource!=resource->bytes||
       state->effects_owner!=effects))return GH_ACTOR_MODEL_BUSY;
    uint8_t *p=actor->bytes;
    if(!state->stage){
        put_word(p+0x1510,request->object_kind);
        memcpy(p+0x1520,request->position_xyz,12);
        put_word(p+0x1530,request->yaw_bits);
        put_word(p+0x1534,request->flags);
        put_word(p+0x1538,request->dependency_kind);
        put_word(p+0x153c,request->dependency_flags);
        p[0x1540]=request->appearance;
        p[0x1541]=request->entry_number;
        put_word(p+0x564,request->dependency_kind);
        gh_effect_view data;
        gh_effect_status status=gh_enemy2ad_effect_data(resource,&data);
        if(!status)status=gh_game_effects_register_2ad_00305758(effects,data);
        if(status){
            state->frontier=effects->frontier?effects->frontier:0x305758;
            return status==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MODEL_UNSUPPORTED:GH_ACTOR_MODEL_BOUNDS;
        }
        state->actor=actor;
        state->resource=resource->bytes;
        state->effects_owner=effects;
        state->stage=1;
    }
    if(state->stage==1){
        state->stage=(read_word(save.bytes+0x14)&0x4000u)?5:2;
    }
    if(state->stage==2){
        int model_ready;
        gh_actor_model_status status=gh_enemy2ad_model_0028ea38(&state->model,actor,resource,work,
            pools,heap,fog,policy,&model_ready);
        if(status){state->frontier=state->model.frontier;return status;}
        state->stage=model_ready?3:5;
    }
    if(state->stage==3){
        int base_ready;
        gh_actor_model_status status=gh_enemy_base_initialize_0028ebf0(&state->base,actor,request,
            0,cockpit,random,&base_ready);
        if(status){state->frontier=state->base.frontier;return status;}
        put_word(p+0x110,0x3f000000);
        put_word(p+0x114,0x3f000000);
        put_word(p+0x118,0x3f000000);
        state->stage=4;
    }
    if(state->stage==4){
        state->frontier=0x2a8578;
        return GH_ACTOR_MODEL_UNSUPPORTED;
    }
    state->frontier=0;
    return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_enemy2ad_init_tail_0028e320(gh_enemy2ad_init_tail *call,gh_enemy2ad_init *prefix,
    gh_title_player_view *actor,const gh_object_view *resource,const gh_enemy2ad_tail_bindings *bindings,
    gh_static_transform_policy policy,int *original)
{
    if(!call||!prefix||!actor||!resource||!bindings||!original||!bindings->rate||
       prefix->stage!=4||prefix->actor!=actor||prefix->resource!=resource->bytes||
       resource->file_extent<0x14||!resource->file_extent_known||resource->file_extent>resource->size)
        return GH_ACTOR_MODEL_ARGUMENT;
    *original=0;
    gh_actor_model *model=&prefix->model.model;
    if(bindings->motion.model!=model||bindings->solve.model!=model||
       bindings->motion.resource!=resource->bytes||bindings->motion.resource_bytes!=resource->file_extent||
       bindings->effect.model!=model||bindings->effect.storage.effects!=prefix->effects_owner)
        return GH_ACTOR_MODEL_ARGUMENT;
    if(call->prefix&&call->prefix!=prefix)return GH_ACTOR_MODEL_BUSY;
    call->prefix=prefix;
    uint8_t *p=actor->bytes;
    uint32_t pc=0;
    if(!call->stage){
        gh_actor_motion_status status=gh_actor_motion_begin_002a8578(&call->motion,
            &bindings->motion,read_word(resource->bytes+0xc),SIZE_MAX,0);
        if(status){
            call->frontier=call->motion.frontier?call->motion.frontier:0x2a8578;
            return GH_ACTOR_MODEL_UNSUPPORTED;
        }
        call->stage=1;
    }
    if(call->stage==1){
        int ended;
        if(gh_actor_motion_step_00157c18(&call->motion,*bindings->rate,policy,&ended,&pc)){
            call->frontier=pc?pc:0x157c18;
            return GH_ACTOR_MODEL_UNSUPPORTED;
        }
        call->stage=2;
    }
    if(call->stage==2){
        gh_actor_model_status status=gh_actor_model_update_hierarchy_00149eb8(model,policy);
        if(status){call->frontier=model->frontier?model->frontier:0x149eb8;return status;}
        call->stage=3;
    }
    if(call->stage==3){
        if(gh_actor_motion_solve_00152a20(&call->solve,&bindings->solve,policy,&pc)){
            call->frontier=pc?pc:0x152a20;
            return GH_ACTOR_MODEL_UNSUPPORTED;
        }
        call->stage=4;
    }
    if(call->stage==4){
        gh_actor_model_status status=gh_actor_finish_model_resume_0014a038(&call->finish,model,policy);
        if(status){call->frontier=model->frontier?model->frontier:0x14a038;return status;}
        put_word(p+0x258,read_word(p+0x258)|0x80000000u);
        p[0x140]=4;
        p[0x531]=0;
        call->stage=5;
    }
    if(call->stage==5){
        if(gh_game_actor_shadow_initialize_0014ef50(&call->shadow,actor,0x670,0x3c3f68)){
            call->frontier=0x14ef50;
            return GH_ACTOR_MODEL_UNSUPPORTED;
        }
        call->stage=6;
    }
    if(call->stage==6){
        const gh_effect_create_request request={.bank=0xd7,.index=1,.actor=actor,.mode=-1,.selector=-1};
        gh_effect_status status=call->effect.active?GH_EFFECT_OK:gh_effect_create_begin(&call->effect,&request);
        if(!status)status=gh_game_effect_create_00306528(&call->effect,&bindings->effect);
        if(status){
            call->frontier=call->effect.frontier?call->effect.frontier:0x306528;
            return GH_ACTOR_MODEL_UNSUPPORTED;
        }
        put_word(p+0x1550,0);
        call->stage=7;
    }
    call->frontier=0;
    *original=1;
    return GH_ACTOR_MODEL_OK;
}

static gh_actor_motion_status move_stop(uint32_t *frontier,uint32_t pc)
{
    *frontier=pc;
    return GH_ACTOR_MOTION_UNSUPPORTED;
}

static int load_position(const gh_title_player_view *actor,float xyz[4])
{
    if(!actor||!actor->bytes||!actor->alias_f0)return 0;
    const uintptr_t start=(uintptr_t)actor->bytes,point=(uintptr_t)actor->alias_f0;
    if(point<start||point-start>actor->size||actor->size-(point-start)<16)return 0;
    memcpy(xyz,actor->alias_f0,16);
    return isfinite(xyz[0])&&isfinite(xyz[1])&&isfinite(xyz[2]);
}

static float scalar(const void *p)
{
    float value;
    memcpy(&value,p,4);
    return value;
}

static int nearest(fenv_t *saved)
{
    if(feholdexcept(saved))return 0;
    if(!fesetround(FE_TONEAREST))return 1;
    (void)fesetenv(saved);
    return 0;
}

static gh_actor_motion_status try_scene_task(const gh_game_title_player *actor,
    const gh_enemy2ad_move_bindings *bindings,uint32_t *frontier)
{
    if(!bindings->scenario14)return move_stop(frontier,0x28e5a4);
    if(*bindings->scenario14==1)return GH_ACTOR_MOTION_OK;
    int dead;
    if(gh_r100_enemy_dead_0028fb38(bindings->room,actor,&dead))
        return move_stop(frontier,0x28fb38);
    if(dead)return GH_ACTOR_MOTION_OK;
    const gh_title_player_view *player=bindings->opacity?bindings->opacity->current_player:NULL;
    float from[4],to[4];
    if(!load_position(player,from)||!load_position(&actor->view,to))
        return move_stop(frontier,0x2db6b0);
    fenv_t saved;
    if(!nearest(&saved))return move_stop(frontier,0x1f7e28);
    volatile float x=from[0]-to[0],y=from[1]-to[1],z=from[2]-to[2];
    volatile float xx=x*x,yy=y*y,zz=z*z,xy=xx+yy,total=xy+zz;
    const int in_range=fabsf(y)<2.0f&&total<9.0f;
    if(fesetenv(&saved)||!isfinite(total))return move_stop(frontier,0x1f7e28);
    if(!in_range)return GH_ACTOR_MOTION_OK;
    if(player->size<0x108)return move_stop(frontier,0x2da2d0);
    const float yaw=scalar(player->bytes+0x104),limit=0x1.eb7c16p+0f;
    float delta;
    uint32_t pc=0;
    if(from[0]==to[0]&&from[2]==to[2]){
        /* The bool path uses the non-positive X branch at coincident XZ. */
        float angle;
        if(!nearest(&saved))return move_stop(frontier,0x2da2d0);
        volatile float raw=-0x1.921fb6p+0f-yaw;
        const int wrapped=!gh_id_angle_reference_001f7d48(raw,&angle);
        if(fesetenv(&saved)||!wrapped)return move_stop(frontier,0x2da2d0);
        delta=angle;
    }else if(gh_player_target_angle_002da038(bindings->elf,bindings->elf_bytes,from,to,yaw,limit,&delta,&pc)){
        return move_stop(frontier,pc?pc:0x2da2d0);
    }
    if(!(fabsf(delta)<limit))return GH_ACTOR_MOTION_OK;
    gh_scene_ray_request ray={.mask=7,.segmented=1};
    memcpy(ray.from,from,16);
    memcpy(ray.to,to,16);
    int hit;
    if(!bindings->solve.actors||gh_scene_ray_query_00131e10(bindings->solve.actors,&ray,&hit,&pc))
        return move_stop(frontier,pc?pc:0x131e10);
    return hit?GH_ACTOR_MOTION_OK:move_stop(frontier,0x28e7c8);
}

static gh_actor_motion_status turn_to_player(gh_title_player_view *actor,const gh_enemy2ad_move_bindings *bindings,
    uint32_t *frontier)
{
    float from[4],to[4];
    if(!bindings->opacity||!load_position(bindings->opacity->current_player,to))
        return move_stop(frontier,0x2db6b0);
    if(!load_position(actor,from))return move_stop(frontier,0x2a7498);
    const float heading=scalar(actor->bytes+0x104);
    fenv_t saved;
    if(!nearest(&saved))return move_stop(frontier,0x2a7498);
    volatile float speed=scalar(actor->bytes+0x5a8)*0x1.921fb6p-4f;
    volatile float limit=speed*scalar((const uint8_t *)bindings->solve.work+0x55c);
    float delta;
    uint32_t pc=0;
    const gh_player_target_status status=gh_player_target_angle_002da038(
        bindings->elf,bindings->elf_bytes,from,to,heading,limit,&delta,&pc);
    float result;
    int wrapped=0;
    if(!status){
        volatile float next=heading+delta;
        wrapped=!gh_id_angle_reference_001f7d48(next,&result);
    }
    if(fesetenv(&saved)||status||!wrapped)return move_stop(frontier,pc?pc:0x2a7498);
    memcpy(actor->bytes+0x104,&result,4);
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy2ad_move_0028e420(gh_enemy2ad_move *state,gh_enemy2ad_init_tail *init,
    gh_game_title_player *actor,const gh_enemy2ad_move_bindings *bindings,uint32_t *frontier)
{
    if(!state||!init||!actor||!bindings||!frontier||!bindings->solve.work||init->stage!=7||
       !init->prefix||init->prefix->actor!=&actor->view||!actor->view.bytes||
       actor->view.size<GH_EM2AD_BYTES||bindings->solve.model!=&init->prefix->model.model)
        return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    uint8_t *p=actor->view.bytes;
    gh_actor_model *model=bindings->solve.model;
    uint32_t pc=0;
    if(!state->stage){
        if(read_word((const uint8_t *)bindings->solve.work+0x5d8)&0x40000000u)
            return GH_ACTOR_MOTION_OK;
        gh_actor_motion_status status=try_scene_task(actor,bindings,frontier);
        if(status)return status;
        state->stage=p[0x2f6]==0?1:p[0x2f6]==1?2:4;
    }
    if(state->stage==1){
        gh_actor_motion_status status=GH_ACTOR_MOTION_OK;
        if(!state->begin.active){
            gh_actor_motion_bindings next=init->motion.b;
            next.effect=bindings->release_effect;
            next.context=bindings->effect_context;
            status=gh_actor_motion_call_open_002a8578(&state->begin,&init->motion,&next,
                read_word(next.resource+0xc),SIZE_MAX,0);
        }
        if(!status)status=gh_actor_motion_call_step_002a8578(&state->begin);
        if(status)return move_stop(frontier,state->begin.frontier?state->begin.frontier:0x2a8578);
        p[0x2f6]++;
        state->stage=2;
    }
    if(state->stage==2){
        gh_actor_motion_status status=turn_to_player(&actor->view,bindings,frontier);
        if(status)return status;
        state->stage=3;
    }
    if(state->stage==3){
        int ended;
        if(!bindings->rate||gh_actor_motion_step_00157c18(&init->motion,*bindings->rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc))
            return move_stop(frontier,pc?pc:0x157c18);
        state->stage=4;
    }
    if(state->stage==4){
        if(p[0x3ae]||p[0x3af])return move_stop(frontier,0x14fb48);
        state->stage=5;
    }
    if(state->stage==5){
        if(gh_actor_model_update_hierarchy_00149eb8(model,GH_STATIC_BINARY32_REFERENCE))
            return move_stop(frontier,model->frontier?model->frontier:0x149eb8);
        state->stage=6;
    }
    if(state->stage==6){
        if(gh_actor_motion_solve_00152a20(&state->solve,&bindings->solve,GH_STATIC_BINARY32_REFERENCE,&pc))
            return move_stop(frontier,pc?pc:0x152a20);
        state->stage=7;
    }
    if(state->stage==7){
        if(gh_actor_finish_model_resume_0014a038(&state->finish,model,GH_STATIC_BINARY32_REFERENCE))
            return move_stop(frontier,model->frontier?model->frontier:0x14a038);
        state->stage=8;
    }
    if(state->stage==8){
        if(gh_game_object_opacity_002a87e8(&actor->view,NULL,bindings->solve.work,bindings->opacity,
            GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
            return move_stop(frontier,pc?pc:0x2a87e8);
        state->stage=9;
    }
    if(state->stage==9){
        const uint32_t handle=read_word(p+0x1550);
        gh_se_voice_info voice;
        gh_voice_status exists=GH_VOICE_BOUNDS;
        if(handle){
            if(!bindings->voices)return move_stop(frontier,0x2cbcf0);
            exists=gh_se_voice_query(bindings->voices,handle,&voice);
            if(exists!=GH_VOICE_OK&&exists!=GH_VOICE_BOUNDS)return move_stop(frontier,0x2cbcf0);
        }
        if(exists==GH_VOICE_BOUNDS){
            state->sound=(gh_motion_sound_request){.model=model,.actor=&actor->view,.bank=1,.program=0x12};
            state->stage=10;
        }else state->stage=11;
    }
    if(state->stage==10){
        uint32_t handle;
        if(!bindings->play_sound)return move_stop(frontier,0x2cba48);
        gh_actor_motion_status status=bindings->play_sound(bindings->sound_context,&state->sound,&handle,&pc);
        if(status)return move_stop(frontier,pc?pc:0x2cba48);
        put_word(p+0x1550,handle);
        state->stage=11;
    }
    float pos[4];
    if(!load_position(&actor->view,pos))return move_stop(frontier,0x28e540);
    if(actor->view.alias_f0!=p+0x490)memcpy(p+0x490,pos,12);
    *state=(gh_enemy2ad_move){0};
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy2ad_dtor_motion_0028e068(gh_actor_motion_call *release,
    gh_enemy2ad_init_tail *init,gh_motion_effect_consumer effect,void *context,uint32_t *frontier)
{
    if(!release||!init||!frontier||init->stage!=7||!init->prefix||
       !init->prefix->actor||!init->prefix->actor->bytes||
       init->prefix->actor->size<GH_EM2AD_BYTES)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    put_word(init->prefix->actor->bytes+0x214,0x44a528);
    gh_actor_motion_status status=GH_ACTOR_MOTION_OK;
    if(!release->active)
        status=gh_actor_motion_call_open_release_002a84a8(release,&init->motion,effect,context);
    if(!status)status=gh_actor_motion_call_release_step_002a84a8(release);
    if(status)*frontier=release->frontier?release->frontier:0x2a84a8;
    else *frontier=0x305778;
    return status?status:GH_ACTOR_MOTION_UNSUPPORTED;
}
