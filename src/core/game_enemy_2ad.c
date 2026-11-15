#include "game_actor_motion_step.h"
#include "game_enemy_2ad.h"
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
