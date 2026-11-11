#include "game_om382.h"
#include "game_actor_model_nodes.h"
#include "game_actor_motion_step.h"
#include "game_enemy_weapon.h"
#include "game_player_root_delta.h"

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
    if(!s||!object||object->source.type!=0x382||!b||!out)return GH_PLACE_ARGUMENT;
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
        put(p+0x668,100);
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
        p[0x2f5]=0x17;
        p[0x2f6]=p[0x2f7]=0;
        put(p+0x5b8,word(p+0x5b8)&~0x40u);
        p[0x688]=0;
        put(p+0x68c,0);
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
