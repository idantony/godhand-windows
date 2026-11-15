#include "game_om382.h"
#include "game_ray_query.h"
#include "game_actor_model_nodes.h"
#include "game_actor_external_parent.h"
#include "game_event_player_reset.h"
#include "game_actor_motion_step.h"
#include "game_enemy_weapon.h"
#include "game_player_root_delta.h"
#include "game_actor_matrix.h"
#include "game_scene_at_geometry.h"
#include "id_layout_rotation.h"

#include <fenv.h>
#include <math.h>
#include <string.h>

static uint32_t word(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static uint16_t half(const uint8_t *p){uint16_t v;memcpy(&v,p,2);return v;}
static float value(const uint8_t *p){float v;memcpy(&v,p,4);return v;}
static void put(uint8_t *p,uint32_t v){memcpy(p,&v,4);}
static void put_half(uint8_t *p,uint16_t v){memcpy(p,&v,2);}
static void put_float(uint8_t *p,float v){memcpy(p,&v,4);}
static float add(float a,float b){volatile float v=a+b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}

static gh_scene_placement_status placement_status(gh_actor_model_status status)
{
    switch(status){
    case GH_ACTOR_MODEL_OK:return GH_PLACE_OK;
    case GH_ACTOR_MODEL_ARGUMENT:return GH_PLACE_ARGUMENT;
    case GH_ACTOR_MODEL_BOUNDS:return GH_PLACE_BOUNDS;
    case GH_ACTOR_MODEL_MEMORY:return GH_PLACE_MEMORY;
    case GH_ACTOR_MODEL_BUSY:return GH_PLACE_BUSY;
    default:return GH_PLACE_UNSUPPORTED;
    }
}

gh_scene_placement_status gh_om382_construct_001c9140(void *data,size_t size,gh_title_player_view *out)
{
    return placement_status(gh_om_weapon_construct_001c9140(data,size,out));
}

gh_scene_placement_status gh_om382_initialize_001c9268(gh_om382_state *s,gh_om_object *object,
    const gh_om_initializer_bindings *b,int *out)
{
    if(!s||!object||!b||!out)return GH_PLACE_ARGUMENT;
    const int paper=object->runtime_created&&object->runtime_kind==0x376&&!object->source.bytes;
    const int room_weapon=object->runtime_created&&object->runtime_kind==0x363&&!object->source.bytes;
    if(!paper&&!room_weapon&&(object->runtime_created||object->source.type!=0x382))return GH_PLACE_ARGUMENT;
    *out=0;
    s->frontier=0;
    int initialized=0;
    gh_scene_placement_status status=gh_om_common_initialize_001b6fb8(&s->common,object,b,&initialized);
    if(status||!initialized){
        s->frontier=s->common.frontier?s->common.frontier:0x1b6fb8;
        return status?status:GH_PLACE_UNSUPPORTED;
    }
    uint8_t *p=object->view.bytes;
    if(s->common.stage==3){
        put(p+0x600,0);
        put(p+0x604,0);
        put(p+0x630,0);
        put(p+0x668,room_weapon?60:100);
        s->common.stage=4;
    }
    if(s->common.stage==4){
        if(!b->init||!b->shapes){s->frontier=0x1fe0e8;return GH_PLACE_UNSUPPORTED;}
        gh_actor_model_status model_status=gh_om_weapon_spheres_001c9268(&s->common.model,
            &object->view,b->init,b->shapes,s->hit,s->shape_index,&s->hit_owned,&s->hit_phase,&s->frontier);
        if(model_status)return placement_status(model_status);
        put(p+0x654,(uint32_t)s->shape_index[0]);
        if(s->hit[1].bytes)put(p+0x65c,(uint32_t)s->shape_index[1]);
        s->common.stage=5;
    }
    if(s->common.stage==5){
        for(unsigned i=0;i<s->common.model.script.mesh_count;i++){
            const gh_actor_node_packets *node;
            if(gh_actor_model_packet_at(&s->common.model,i,&node)){
                s->frontier=0x14b6f8;
                return GH_PLACE_BOUNDS;
            }
            if(node->node[0x40c]==0x40&&gh_actor_model_node_or_flags(&s->common.model,i,0x380,1)){
                s->frontier=0x1b7280;
                return GH_PLACE_UNSUPPORTED;
            }
        }
        p[0x2f4]=0;
        p[0x2f5]=(paper||room_weapon)?0:0x17;
        p[0x2f6]=(paper||room_weapon)?2:0;
        p[0x2f7]=0;
        put(p+0x5b8,word(p+0x5b8)&~0x40u);
        p[0x688]=0;
        put(p+0x68c,0);
        if(paper)put(p+0x2d0,(word(p+0x2d0)&~0x20000000u)|0x80000000u);
        s->common.stage=6;
    }
    *out=1;
    return GH_PLACE_OK;
}

gh_scene_placement_status gh_om382_abort(gh_om382_state *s)
{
    if(!s)return GH_PLACE_ARGUMENT;
    const gh_actor_model_status status=gh_om_weapon_spheres_release(s->hit,&s->hit_owned,&s->frontier);
    if(status)return placement_status(status);
    return gh_om_initialize_abort(&s->common);
}

static gh_object_step_status stop(uint32_t *frontier,uint32_t address,gh_object_step_status status)
{
    *frontier=address;
    return status;
}

static gh_object_step_status select_clip(gh_om382_state *s,unsigned index,uint32_t *frontier)
{
    gh_om_object *object=s->common.object;
    if(!s->clip[index].bytes){
        gh_bundle bundle;
        if(gh_bundle_open(object->resource.bytes,object->resource.file_extent,&bundle)||
           gh_bundle_at(&bundle,2+index,&s->clip[index])||memcmp(s->clip[index].tag,"MOT",3))
            return stop(frontier,0x2a8578,GH_OBJSTEP_UNSUPPORTED);
    }
    const gh_actor_motion_bindings b={.model=&s->common.model,.work=s->common.bindings->work,
        .resource=object->resource.bytes,.resource_bytes=object->resource.file_extent};
    gh_actor_motion_status status;
    uint32_t pc=0;
    if(!index){
        status=gh_actor_motion_begin_002a8578(&s->motion,&b,s->clip[index].offset,SIZE_MAX,0);
        pc=s->motion.frontier;
    }else{
        if(!s->select_motion.active){
            status=gh_actor_motion_call_open_002a8578(&s->select_motion,&s->motion,&b,
                s->clip[index].offset,SIZE_MAX,0);
            if(status)return stop(frontier,s->select_motion.frontier?s->select_motion.frontier:0x2a8578,
                GH_OBJSTEP_UNSUPPORTED);
        }
        status=gh_actor_motion_call_step_002a8578(&s->select_motion);
        pc=s->select_motion.frontier;
    }
    return status?stop(frontier,pc?pc:0x2a8578,GH_OBJSTEP_UNSUPPORTED):GH_OBJSTEP_OK;
}

/* Original shared 00/01 (1CA040) after an ordinary weapon or paper attach. */
static gh_object_step_status weapon_attached(gh_om382_state *s,const gh_om_frame_runtime *runtime,
    float rate,uint32_t *frontier)
{
    gh_title_player_view *view=&s->common.object->view;
    uint8_t *p=view->bytes;
    if(!p[0x2f6]){
        p[0x531]=0xff;
        if(!runtime->release_effect)return stop(frontier,0x306200,GH_OBJSTEP_UNSUPPORTED);
        const gh_actor_motion_status status=runtime->release_effect(runtime->context,view,8,2,frontier);
        if(status)return stop(frontier,*frontier?*frontier:0x306200,
            status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE);
        p[0x2f6]=1;
    }
    const float timer=value(p+0x684);
    if(!isfinite(timer))return stop(frontier,0x1ca040,GH_OBJSTEP_UNSUPPORTED);
    if(timer<=0)return GH_OBJSTEP_OK;
    const gh_actor_part *part;
    if(gh_actor_model_part_at(&s->common.model,0,&part))
        return stop(frontier,0x14bfe0,GH_OBJSTEP_CALLEE);
    const int mode=fegetround();
    if(mode<0||fesetround(FE_TOWARDZERO))return stop(frontier,0x1ca040,GH_OBJSTEP_CALLEE);
    const float remaining=add(timer,-rate);
    const float angle=add(value(part->bytes+0x104),mul(rate,0.5235988f));
    if(fesetround(mode))return stop(frontier,0x1ca040,GH_OBJSTEP_CALLEE);
    if(remaining>0){
        float wrapped;
        if(gh_id_angle_reference_001f7d48(angle,&wrapped))
            return stop(frontier,0x1f7d48,GH_OBJSTEP_UNSUPPORTED);
        put_float(part->bytes+0x104,wrapped);
    }else memcpy(part->bytes+0x100,p+0x620,12);
    put_float(p+0x684,remaining);
    return GH_OBJSTEP_OK;
}

/* Original 00/13 (1CE6C8): rotate the local falling component, add the
 * already world-space launch vector, and fade after 30 rate units. */
static gh_object_step_status paper_loose(gh_title_player_view *actor,float rate,
    gh_static_transform_policy policy,uint32_t *frontier)
{
    uint8_t *p=actor->bytes;
    if(p[0x2f6]>1)return GH_OBJSTEP_OK;
    float euler[3],rotation[16],local[4];
    const float fall[3]={0,-0.1f,0};
    memcpy(euler,p+0x100,sizeof(euler));
    const float old_timer=p[0x2f6]?value(p+0x660):30.0f;
    const float opacity=value(p+0x24c);
    if(!isfinite(old_timer)||!isfinite(opacity))return stop(frontier,0x1ce6c8,GH_OBJSTEP_UNSUPPORTED);
    const int mode=fegetround();
    if(mode<0||fesetround(FE_TOWARDZERO))return stop(frontier,0x1ce6c8,GH_OBJSTEP_CALLEE);
    const float x_angle=add(euler[0],mul(rate,0.17453292f));
    const float timer=add(old_timer,-rate);
    const float fade=timer<=0?add(opacity,-mul(rate,0.1f)):opacity;
    if(fesetround(mode))return stop(frontier,0x1ce6c8,GH_OBJSTEP_CALLEE);
    if(gh_id_angle_reference_001f7d48(x_angle,euler))
        return stop(frontier,0x1f7d48,GH_OBJSTEP_UNSUPPORTED);
    if(gh_actor_matrix_rotation_00147728(euler,0,rotation,policy,frontier))
        return stop(frontier,*frontier?*frontier:0x147728,GH_OBJSTEP_UNSUPPORTED);
    if(gh_scene_at_apply_world_001508d8(rotation,fall,local))
        return stop(frontier,0x30a2b0,GH_OBJSTEP_UNSUPPORTED);
    float position[3];
    for(unsigned i=0;i<3;i++){
        const float old=value(actor->alias_f0+4*i),launch=value(p+0x640+4*i);
        position[i]=add(add(old,mul(local[i],rate)),mul(launch,rate));
        if(!isfinite(position[i]))return stop(frontier,0x1ce6c8,GH_OBJSTEP_UNSUPPORTED);
    }
    memcpy(actor->alias_f0,position,sizeof(position));
    put_float(p+0x100,euler[0]);
    put_float(p+0x660,timer);
    put_float(p+0x24c,fade);
    p[0x2f6]=1;
    if(timer<=0&&fade<=0){
        p[0x2f4]=1;
        p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
    }
    return GH_OBJSTEP_OK;
}

/* 1C9AF0, 0363's ordinary unattached state. Its sparkle is emitted before
 * descent. Keep the completed emission when the subsequent ray needs retry. */
static gh_object_step_status grounded_weapon_001c9af0(gh_om382_state *s,const gh_om_frame_runtime *runtime,
    uint8_t *phase,float rate,uint32_t *frontier)
{
    const gh_title_player_view *actor=&s->common.object->view;
    uint8_t *p=actor->bytes;
    if(word(p+0x600)||s->common.model.external_owner||!actor->alias_f0)
        return stop(frontier,0x1cf3a8,GH_OBJSTEP_UNSUPPORTED);
    if(p[0x2f6]==3){
        p[0x664]=p[0x665]=1;
        return GH_OBJSTEP_OK;
    }
    if(p[0x2f6]!=2)return stop(frontier,0x1c9af0,GH_OBJSTEP_UNSUPPORTED);
    if(*phase==1){
        uint16_t timer=(uint16_t)(half(p+0x56a)-1u);
        if((int16_t)timer<1){
            if(!runtime->break_effect)return stop(frontier,0x306838,GH_OBJSTEP_UNSUPPORTED);
            uint32_t position[3];
            memcpy(position,actor->alias_f0,sizeof(position));
            gh_object_step_status status=runtime->break_effect(runtime->context,&s->common.model,position,0,frontier);
            if(status)return status;
            timer=60;
        }
        put_half(p+0x56a,timer);
        *phase=2;
    }
    float old_position[4];
    memcpy(old_position,actor->alias_f0,sizeof(old_position));
    const float old_speed=value(p+0x630);
    float y=add(old_position[1],mul(old_speed,rate));
    float speed=add(old_speed,-mul(rate,0.013f));
    if(!isfinite(speed)||!isfinite(y))return stop(frontier,0x1c9e6c,GH_OBJSTEP_UNSUPPORTED);
    if(speed<-1.95f){
        p[0x2f4]=1;
        p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
    }else{
        float hit[4];
        gh_scene_ray_request ray={.mask=2,.flags=0x534,.hit_position=hit,.segmented=1};
        memcpy(ray.from,old_position,sizeof(old_position));
        memcpy(ray.to,old_position,sizeof(old_position));
        ray.from[1]=add(old_position[1],0.5f);
        ray.to[1]=add(y,-1.0f);
        int did_hit;
        uint32_t pc=0;
        if(gh_scene_ray_query_00131e10(runtime->scene_tables,&ray,&did_hit,&pc))
            return stop(frontier,pc?pc:0x131e10,GH_OBJSTEP_UNSUPPORTED);
        if(did_hit&&y<add(hit[1],0.15f)){
            y=add(hit[1],0.15f);
            speed=0;
        }
    }
    p[0x664]=1;
    put_float(actor->alias_f0+4,y);
    put_float(p+0x630,speed);
    return GH_OBJSTEP_OK;
}

gh_object_step_status gh_om382_move_prefix_001c9860(gh_om382_state *s,const gh_om_frame_runtime *runtime,
    const gh_object_opacity_environment *env,uint8_t *phase,float *rate,
    gh_static_transform_policy policy,uint32_t *frontier)
{
    if(!s||!runtime||!runtime->init||!runtime->canonical_rate||!phase||!rate||!frontier||
       s->common.stage!=6||runtime->init!=s->common.bindings->init)return GH_OBJSTEP_ARGUMENT;
    *frontier=0;
    gh_om_object *object=s->common.object;
    uint8_t *p=object->view.bytes;
    const gh_boot_work *work=s->common.bindings->work;
    if(policy!=GH_STATIC_BINARY32_REFERENCE)return stop(frontier,0x1c9860,GH_OBJSTEP_UNSUPPORTED);

    if(!*phase){
        for(unsigned i=0;i<2;i++)if(s->hit[i].bytes){
            put_half(s->hit[i].bytes+0x46,0);
            put(s->hit[i].bytes+0x4c,0);
            uint32_t pc=0;
            if(gh_hitarea_object_set_active_001fd668(&s->hit[i],0,&pc))
                return stop(frontier,pc?pc:0x1fd668,GH_OBJSTEP_CALLEE);
        }
        *rate=mul(value(runtime->init->enemies+0x548),*runtime->canonical_rate);
        if(!isfinite(*rate)||*rate<0)return stop(frontier,0x292f50,GH_OBJSTEP_UNSUPPORTED);
        put_float(p+0x5a8,*rate);
        const float timer=value(p+0x66c);
        if(timer>0)put_float(p+0x66c,add(timer,-*rate));
        p[0x664]=p[0x665]=0;
        *phase=1;
    }
    if(p[0x2f4]==1){
        *phase=7;
        p[0x531]=0xff; /* 1CF140 delay slot before 1B79B0. */
        return GH_OBJSTEP_OK;
    }
    if(object->runtime_created&&object->runtime_kind==0x376){
        if(p[0x2f4])return stop(frontier,0x1c9860,GH_OBJSTEP_UNSUPPORTED);
        if(*phase!=1)return GH_OBJSTEP_ARGUMENT;
        gh_object_step_status status;
        if(p[0x2f5]==1)status=weapon_attached(s,runtime,*rate,frontier);
        else if(p[0x2f5]==0x13)status=paper_loose(&object->view,*rate,policy,frontier);
        else return stop(frontier,0x1c9a50,GH_OBJSTEP_UNSUPPORTED);
        if(!status)*phase=6;
        return status;
    }
    if(object->runtime_created&&object->runtime_kind==0x363&&!p[0x2f4]&&p[0x2f5]<=1){
        gh_object_step_status status=p[0x2f5]?
            weapon_attached(s,runtime,*rate,frontier):grounded_weapon_001c9af0(s,runtime,phase,*rate,frontier);
        if(!status)*phase=6;
        return status;
    }
    if(p[0x2f4]!=0||p[0x2f5]!=0x17)
        return stop(frontier,p[0x2f4]?0x1c9860:0x1c9a50,GH_OBJSTEP_UNSUPPORTED);
    if(*phase==1){
        put_float(p+0x444,half(p+0x434)&4?-*rate:*rate);
        unsigned step=p[0x2f6];
        if(step==0||step==2){
            gh_object_step_status status=select_clip(s,step/2,frontier);
            if(status)return status;
            if(!step)put_half(p+0x568,0);
            put_float(p+0x674,step?90.0f:150.0f);
            p[0x2f6]++;
        }
        *phase=2;
    }
    if(p[0x2f6]!=1&&p[0x2f6]!=3)return GH_OBJSTEP_OK;
    if(*phase==2){
        int ended=0;
        uint32_t pc=0;
        if(gh_actor_motion_step_00157c18(&s->motion,*runtime->canonical_rate,policy,&ended,&pc))
            return stop(frontier,pc?pc:0x157c18,GH_OBJSTEP_UNSUPPORTED);
        *phase=3;
    }
    if(*phase==3&&p[0x2f6]==1){
        if(!env||!env->player_binding_known||!env->current_player||
           !env->current_player->alias_f0||!object->view.alias_f0)
            return stop(frontier,0x2db6b0,GH_OBJSTEP_UNSUPPORTED);
        const uint8_t *a=env->current_player->alias_f0,*b=object->view.alias_f0;
        const float dx=add(value(a),-value(b)),dz=add(value(a+8),-value(b+8));
        if(!isfinite(dx)||!isfinite(dz))return stop(frontier,0x1f7eb0,GH_OBJSTEP_UNSUPPORTED);
        if(add(mul(dx,dx),mul(dz,dz))<4)put_half(p+0x568,1);
        if(half(p+0x568)){
            const float timer=add(value(p+0x674),-*rate);
            put_float(p+0x674,timer);
            if(timer<=0)p[0x2f6]++;
        }
        *phase=6;
    }else if(*phase==3){
        uint32_t pc=0;
        if(gh_player_root_rotation_0014f9f0(&object->view,work,policy,&pc))
            return stop(frontier,pc?pc:0x14f9f0,GH_OBJSTEP_UNSUPPORTED);
        *phase=4;
    }
    if(*phase==4){
        uint32_t pc=0;
        if(gh_player_root_translation_0014f928(&object->view,work,policy,&pc))
            return stop(frontier,pc?pc:0x14f928,GH_OBJSTEP_UNSUPPORTED);
        *phase=5;
    }
    if(*phase==5){
        const float timer=add(value(p+0x674),-*rate);
        put_float(p+0x674,timer);
        if(timer<=0){
            const float opacity=add(value(p+0x24c),-mul(*rate,0.1f));
            put(p+0x250,word(p+0x250)|0x10u);
            put_float(p+0x24c,opacity<0?0:opacity);
            if(opacity<0){
                p[0x2f4]=1;
                p[0x2f5]=p[0x2f6]=p[0x2f7]=0;
            }
        }
        *phase=6;
    }
    return GH_OBJSTEP_OK;
}


gh_scene_placement_status gh_om376_set_paper_0012c8d8(gh_om376_paper *call,
    gh_om_factory *factory,gh_actor_model *player,gh_core_save_view save,
    gh_game_scene_tables *tables,gh_collision_scroll_mutation *scroll,uint32_t *frontier)
{
    if(!frontier)return GH_PLACE_ARGUMENT;
    *frontier=0x12c8d8;
    if(!call||!factory||!factory->bound||
       !gh_om376_paper_room_supported(factory->bindings.work->room)||
       !player||!player->ready||!player->root.bytes||!player->root.alias_f0||
       player->root.size<0x6ac||player->root.original_vtable214!=0x41d220||
       player->pools!=factory->bindings.pools||!save.bytes||save.size<GH_CORE_SAVE_BYTES||
       !tables||!scroll)return GH_PLACE_ARGUMENT;
    if(call->stage&&(call->player!=player||call->create.factory!=factory))return GH_PLACE_BUSY;
    uint8_t *pl=player->root.bytes;
    if(call->stage>=6||(!call->stage&&word(pl+0x6a8))){
        *frontier=0;
        return GH_PLACE_OK;
    }
    if(!call->stage){
        save.bytes[0x157]=1; /* 001FC120 */
        gh_om_object *paper=NULL;
        const gh_scene_placement_status status=gh_om_factory_create_0030fa98(factory,
            &call->create,0x376,0xffff,&paper);
        if(status){*frontier=factory->frontier;return status;}
        if(!paper){*frontier=0;return GH_PLACE_OK;}
        call->player=player;
        put(pl+0x6a8,paper->handle);
        call->stage=1;
    }
    gh_om_object *paper=call->create.object;
    if(call->stage==1){
        const gh_scene_placement_status status=gh_om_factory_initialize_runtime_376(factory,paper);
        if(status){*frontier=factory->frontier;return status;}
        call->stage=2;
    }
    gh_actor_model *model=&paper->initialization->model;
    uint8_t *p=paper->view.bytes;
    if(call->stage==2){
        const gh_actor_part *part;
        *frontier=0x1cf160;
        if(gh_actor_model_part_at(model,0,&part)||!part->alias_d0)return GH_PLACE_BOUNDS;
        const gh_actor_model_status status=gh_actor_model_bind_external_parent(model,0,player,2,paper);
        if(status)return placement_status(status);
        memcpy(paper->view.alias_f0,player->root.alias_f0,12);
        memcpy(p+0x490,player->root.alias_f0,12);
        memset(p+0x100,0,12);

        const unsigned stage=save.bytes[0xae];
        const uint32_t offset[]={0,0x3d408312u,(stage>=2&&stage<=5)?0xbe1eb852u:0xbe0f5c29u};
        const uint32_t orientation[]={0,0,0};
        memcpy(part->alias_d0,offset,12);
        memcpy(part->bytes+0x100,orientation,12);
        memcpy(p+0x610,offset,12);
        memcpy(p+0x620,orientation,12);
        put(p+0x604,2);
        p[0x2f4]=p[0x2f6]=p[0x2f7]=p[0x664]=0;
        p[0x2f5]=1;
        call->stage=3;
    }
    if(call->stage==3){
        const gh_event_weapon_attachment attachment={&paper->view,model,&player->root,player,2};
        if(gh_event_weapon_follow_001cf3a8(&attachment,GH_STATIC_BINARY32_REFERENCE,frontier))
            return GH_PLACE_UNSUPPORTED;
        call->stage=4;
    }
    if(call->stage==4){
        const gh_actor_model_status status=gh_actor_model_update_hierarchy_00149eb8(model,GH_STATIC_BINARY32_REFERENCE);
        if(status){*frontier=model->frontier?model->frontier:0x149eb8;return placement_status(status);}
        call->stage=5;
    }
    if(call->stage==5){
        uint64_t key;
        uint32_t changed;
        memcpy(&key,p+0x540,8);
        if(gh_collision_scroll_set_key_enabled_001331b8(tables,scroll,key,0,&changed)){
            *frontier=0x1331b8;
            return GH_PLACE_UNSUPPORTED;
        }
        call->stage=6;
    }
    *frontier=0;
    return GH_PLACE_OK;
}

/* 12349C detaches its stored cOm382, then clears the PL00 handle and the
 * actual CoreSave paper byte. The pending hierarchy operation is resumable. */
gh_scene_placement_status gh_om376_release_paper_001cf6e0(gh_om376_paper *call,
    const gh_title_player_view *player,gh_core_save_view save,uint32_t *frontier)
{
    if(!frontier)return GH_PLACE_ARGUMENT;
    *frontier=0x1cf6e0;
    if(!call||call->stage<6||!call->player||!player||!player->bytes||
       player->bytes!=call->player->root.bytes||player->size<0x6ac||!player->alias_f0||
       !save.bytes||save.size<GH_CORE_SAVE_BYTES)return GH_PLACE_ARGUMENT;
    if(call->stage==8){*frontier=0;return GH_PLACE_OK;}
    gh_om_object *paper=call->create.object;
    if(!paper||!paper->registered||paper->runtime_kind!=0x376||!paper->initialization||
       !paper->view.alias_f0||word(player->bytes+0x6a8)!=paper->handle)return GH_PLACE_ARGUMENT;
    uint8_t *p=paper->view.bytes;
    gh_actor_model *model=&paper->initialization->model;
    const gh_actor_part *part;
    if(gh_actor_model_part_at(model,0,&part)||!part->alias_f0||!part->parent_is_model)
        return GH_PLACE_BOUNDS;
    if(call->stage==6){
        const uint8_t *parent;
        if(model->external_parent!=call->player||model->external_owner!=paper||
           gh_actor_model_external_parent_at(model,0,&parent))return GH_PLACE_ARGUMENT;
        float position[3],rotation[16],launch[4];
        const float euler[3]={0,value(player->bytes+0x104),0};
        const float impulse[3]={0.1f,0.05f,-0.1f};
        memcpy(position,player->alias_f0,sizeof(position));
        position[1]=value(part->alias_f0+4);
        if(!isfinite(position[0])||!isfinite(position[1])||!isfinite(position[2]))return GH_PLACE_UNSUPPORTED;
        if(gh_actor_matrix_rotation_00147728(euler,0,rotation,GH_STATIC_BINARY32_REFERENCE,frontier))
            return GH_PLACE_UNSUPPORTED;
        if(gh_scene_at_apply_world_001508d8(rotation,impulse,launch)){
            *frontier=0x30a2b0;
            return GH_PLACE_UNSUPPORTED;
        }
        const gh_actor_model_status status=gh_actor_model_clear_external_parent(model,paper);
        if(status)return placement_status(status);
        memcpy(p+0x490,position,sizeof(position));
        memcpy(paper->view.alias_f0,position,sizeof(position));
        put(p+0x24c,0x3f800000u);
        put(p+0x250,word(p+0x250)&~0x10u);
        put(p+0x630,0x3d851eb8u);
        memcpy(p+0x640,launch,12);
        memset(part->bytes+0x100,0,12);
        call->stage=7;
    }
    const gh_actor_model_status status=gh_actor_model_update_hierarchy_00149eb8(model,GH_STATIC_BINARY32_REFERENCE);
    if(status){
        *frontier=model->frontier?model->frontier:0x149eb8;
        return placement_status(status);
    }
    p[0x2f4]=p[0x2f6]=p[0x2f7]=0;
    p[0x2f5]=0x13;
    put(player->bytes+0x6a8,0);
    save.bytes[0x157]=0;
    call->stage=8;
    *frontier=0;
    return GH_PLACE_OK;
}
