#include "game_enemy_other_move.h"
#include "game_model_texture_scroll.h"
#include "game_event_player_reset.h"
#include "game_actor_matrix.h"
#include "game_scene_at_geometry.h"
#include "game_enemy_timers.h"
#include "frame_state.h"
#include "id_layout_rotation.h"
#include <fenv.h>
#include <math.h>
#include <string.h>

static uint32_t word(const void *p){uint32_t n;memcpy(&n,p,4);return n;}
static uint16_t half(const void *p){uint16_t n;memcpy(&n,p,2);return n;}
static float scalar(const void *p){float n;memcpy(&n,p,4);return n;}
static void put(void *p,uint32_t n){memcpy(p,&n,4);}
static void put_float(void *p,float n){memcpy(p,&n,4);}
static void put_half(void *p,uint16_t n){memcpy(p,&n,2);}
static float add(float a,float b){volatile float n=a+b;return n;}
static float sub(float a,float b){volatile float n=a-b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}

static int begin(fenv_t *env)
{
    if(feholdexcept(env))return 0;
    if(!fesetround(FE_TOWARDZERO))return 1;
    (void)fesetenv(env);
    return 0;
}

static gh_actor_motion_status stop(gh_enemy_other_move *s,uint32_t frontier,
    gh_actor_motion_status status)
{
    s->frontier=frontier;
    return status;
}

static gh_actor_motion_status finished(gh_enemy_other_move *s)
{
    s->original_returned=1;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_other_move_bind_targets(gh_enemy_other_move *s,
    const gh_player_target_bindings *bindings)
{
    if(!s||!bindings||!bindings->pools||!bindings->elf||
       !bindings->resolve||(s->target_bindings&&s->target_bindings!=bindings))
        return GH_ACTOR_MOTION_ARGUMENT;
    s->target_bindings=bindings;
    return GH_ACTOR_MOTION_OK;
}

static int point(const gh_title_player_view *view,float xyz[3])
{
    if(!view->bytes||!view->alias_f0||(uintptr_t)view->alias_f0<(uintptr_t)view->bytes)return 0;
    uintptr_t off=(uintptr_t)view->alias_f0-(uintptr_t)view->bytes;
    if(off>view->size||12>view->size-off)return 0;
    memcpy(xyz,view->alias_f0,12);
    return isfinite(xyz[0])&&isfinite(xyz[1])&&isfinite(xyz[2]);
}

static gh_actor_motion_status mode1_candidate(gh_enemy_other_move *s,
    const gh_title_player_view *root,const float origin[3],const float target[3],
    float maximum_range,float maximum_height,float factor,float *angle,int *accepted)
{
    *accepted=0;
    fenv_t env;
    if(!begin(&env))return stop(s,0x291010,GH_ACTOR_MOTION_CALLEE);
    const float x=sub(origin[0],target[0]),z=sub(origin[2],target[2]);
    const float distance=add(mul(x,x),mul(z,z));
    const float height=fabsf(sub(origin[1],target[1]));
    const float limit=mul(*angle,factor);
    if(fesetenv(&env)||!isfinite(distance)||!isfinite(height))
        return stop(s,0x291010,GH_ACTOR_MOTION_UNSUPPORTED);
    if(distance>maximum_range||height>maximum_height)return GH_ACTOR_MOTION_OK;
    float turn;
    uint32_t pc=0;
    const gh_player_target_bindings *b=s->target_bindings;
    if(gh_player_target_angle_002da038(b->elf,b->elf_bytes,origin,target,
        scalar(root->bytes+0x104),3.14159274f,&turn,&pc))
        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    if(fabsf(turn)>limit)return GH_ACTOR_MOTION_OK;
    *angle=fabsf(turn);
    *accepted=1;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status mode1_lookup_00291010(gh_enemy_other_move *s,
    const gh_enemy2a2_virtual_init *owner,int *found)
{
    const gh_player_target_bindings *b=s->target_bindings;
    const gh_title_player_view *root=owner->prefix.root;
    const gh_game_init_state *manager=owner->pose.bindings.init;
    if(!b||b->pools!=owner->prefix.bindings.pools||b->player!=s->player||!manager)
        return stop(s,0x291010,GH_ACTOR_MOTION_UNSUPPORTED);
    *found=0;
    float origin[3],angle=3.14159274f;
    if(!point(root,origin))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_BOUNDS);
    unsigned cursor=UINT8_MAX;
    if(manager->enemy_top){
        for(unsigned i=0;i<64;i++)if(manager->enemy_top==manager->enemies+i*20){cursor=i;break;}
        if(cursor==UINT8_MAX)return stop(s,0x291010,GH_ACTOR_MOTION_BUSY);
    }
    uint64_t visited=0;
    while(cursor!=UINT8_MAX){
        if(cursor>=64||(visited&(UINT64_C(1)<<cursor)))return stop(s,0x291010,GH_ACTOR_MOTION_BUSY);
        visited|=UINT64_C(1)<<cursor;
        const gh_game_title_player *object=manager->enemy_actors[cursor];
        if(!object||!object->view.bytes||object->view.size<0x618||manager->enemies[cursor*20+16]!=1)
            return stop(s,0x291010,GH_ACTOR_MOTION_BUSY);
        cursor=manager->enemy_next[cursor];
        const gh_title_player_view *view=&object->view;
        uint8_t *p=view->bytes;
        const unsigned kind=half(p+0x2fe);
        if(kind<0x200||kind>=0x280||p[0x617])continue;
        if(view->original_vtable214!=0x4479c8||view->size<0x168c)
            return stop(s,0x29117c,GH_ACTOR_MOTION_UNSUPPORTED);
        if((int16_t)half(p+0x54a)<1&&!(half(p+0x3ac)&4u))continue;
        unsigned dep=word(p+0x564);
        float target[3];
        gh_player_target_status point_status=gh_enemy_target_position_00275978(view,target);
        if(point_status)return stop(s,0x275978,point_status==GH_PLAYER_TARGET_UNSUPPORTED?
            GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_BOUNDS);
        int accepted;
        int special_height=p[0x2f4]==1||(p[0x2f4]==2&&p[0x2f5]!=2);
        float range=dep==0x260?196:100;
        gh_actor_motion_status status=mode1_candidate(s,root,origin,target,range,
            special_height?INFINITY:1.5f,1,&angle,&accepted);
        if(status)return status;
        if(accepted)*found=1;
    }
    const gh_room_vector *registry;
    if(gh_room_registry_view(b->pools,0x754c58,&registry)||registry->count>registry->capacity||
       (registry->count&&!registry->allocation.pointer))return stop(s,0x291510,GH_ACTOR_MOTION_BUSY);
    for(unsigned i=0;i<registry->count;i++){
        uint32_t handle=word((const uint8_t*)registry->allocation.pointer+i*4),pc=0;
        gh_title_player_view view={0};
        if(b->resolve(b->context,handle,&view,&pc))
            return stop(s,pc?pc:0x291510,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!view.bytes||view.size<0x300)return stop(s,0x291510,GH_ACTOR_MOTION_BOUNDS);
        const uint8_t *p=view.bytes;
        unsigned id=half(p+0x2fe);
        if((word(p+0x250)&0x80008000u)||id<0x300||id>=0x500)continue;
        if(view.size<0x5bc)return stop(s,0x1b79e0,GH_ACTOR_MOTION_BOUNDS);
        uint32_t object_flags=word(p+0x5b0),target_flags=word(p+0x5b8);
        if((object_flags&4u)||!(target_flags&0x1000u)||!gh_player_target_object_eligible_001b79e0(&view))continue;
        if(view.original_vtable214!=0x42aa98)return stop(s,0x2915ac,GH_ACTOR_MOTION_UNSUPPORTED);
        float target[3];
        if(!point(&view,target))return stop(s,0x2a9458,GH_ACTOR_MOTION_BOUNDS);
        const int wide=(target_flags&0x400u)!=0;
        int accepted;
        gh_actor_motion_status status=mode1_candidate(s,root,origin,target,wide?400:100,
            wide?1.5f:1,wide?1.25f:1,&angle,&accepted);
        if(status)return status;
        if(accepted)*found=1;
    }
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_other_move_bind_2a2_services(gh_enemy_other_move *s,
    const gh_object_opacity_environment *opacity,gh_motion_effect_consumer effect,void *effect_context,
    gh_motion_sound_consumer sound,void *sound_context)
{
    if(!s||!opacity||!opacity->init||
       (s->opacity&&(s->opacity!=opacity||s->effect!=effect||s->effect_context!=effect_context||
                    s->sound!=sound||s->sound_context!=sound_context)))return GH_ACTOR_MOTION_ARGUMENT;
    s->opacity=opacity;
    s->effect=effect;s->effect_context=effect_context;
    s->sound=sound;s->sound_context=sound_context;
    return GH_ACTOR_MOTION_OK;
}

enum { HEAVY_TIMER_CONVERT=28, HEAVY_TIMER_SOUND, HEAVY_TIMER_DRAW };
enum { HEAVY_LEAVE_BIND=46, HEAVY_LEAVE_STEP, HEAVY_LEAVE_HOME, HEAVY_LEAVE_ROOT };
enum { HEAVY_HOME_ROUTE=51, HEAVY_HOME_SELECT, HEAVY_HOME_BIND, HEAVY_HOME_CUE,
       HEAVY_HOME_TURN, HEAVY_HOME_STEP, HEAVY_HOME_DRAW, HEAVY_HOME_WORLD,
       HEAVY_HOME_SPECIAL_WORLD, HEAVY_HOME_ARRIVAL };

enum { HEAVY_REACTION_SOLID=32, HEAVY_REACTION_SELECT, HEAVY_REACTION_BEGIN,
       HEAVY_REACTION_SOUND, HEAVY_REACTION_STEP, HEAVY_REACTION_WORLD };

enum { HEAVY_POISON_SOLID=38, HEAVY_POISON_BEGIN, HEAVY_POISON_STEP,
       HEAVY_POISON_ROOT, HEAVY_POISON_COUNTDOWN, HEAVY_POISON_VOICE, HEAVY_POISON_RELOAD };

static gh_actor_motion_status heavy_root_matrix_0014a2b0(gh_enemy_other_move *s,
    const gh_enemy2a2_init *prefix)
{
    const gh_title_player_view *root=prefix->root;
    uint8_t *p=root->bytes;
    if(!(word(p+0x250)&0x800u)){
        float origin[3],euler[3],scale[3],matrix[16];
        uint32_t pc=0;
        memcpy(origin,root->alias_f0,sizeof(origin));
        memcpy(euler,p+0x100,sizeof(euler));
        memcpy(scale,p+0x110,sizeof(scale));
        if(gh_actor_matrix_compose_00147a48(origin,euler,scale,p[0x140],matrix,
            prefix->bindings.policy,&pc))
            return stop(s,pc?pc:0x14a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(p+0x80,matrix,sizeof(matrix));
        memcpy(p+0x120,scale,sizeof(scale));
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status heavy_root_motion_002da9b8(gh_enemy_other_move *s,
    const gh_enemy2a2_init *prefix)
{
    gh_actor_motion_status status=heavy_root_matrix_0014a2b0(s,prefix);
    if(status)return status;
    const gh_title_player_view *root=prefix->root;
    const uint8_t *p=root->bytes;
    float matrix[16],delta[3],world[4];
    memcpy(matrix,p+0x80,sizeof(matrix));
    memcpy(delta,p+0x330,sizeof(delta));
    if(word(p+0x33c)!=0x3f800000u)return stop(s,0x2da9b8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(gh_scene_at_apply_world_001508d8(matrix,delta,world))
        return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
    memcpy(root->alias_f0,world,sizeof(world));
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status heavy_poison_00287090(gh_enemy_other_move *s,
    gh_enemy2a2_virtual_init *actor)
{
    gh_enemy2a2_init *prefix=&actor->prefix;
    gh_title_player_view *root=prefix->root;
    uint8_t *p=root->bytes;
    uint32_t pc=0;
    if(s->stage==5){
        if(prefix->bindings.work->room!=0x102||half(p+0x640)!=0x11||
           word(p+0x564)!=0x2a2||p[0x2f6]>1)
            return stop(s,0x287090,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=HEAVY_POISON_SOLID;
    }
    if(s->stage==HEAVY_POISON_SOLID){
        if(gh_collision_solid_set_enabled_001351d8(actor->pose.bindings.solids,root,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!p[0x2f6])put(p+0x15b0,word(p+0x15b0)|1u);
        s->stage=p[0x2f6]?HEAVY_POISON_STEP:HEAVY_POISON_BEGIN;
    }
    if(s->stage==HEAVY_POISON_BEGIN){
        const gh_object_view *resource=&prefix->resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x118||
           resource->file_extent>resource->size||prefix->motion.b.resource!=resource->bytes)
            return stop(s,0x287090,GH_ACTOR_MOTION_BOUNDS);
        if(!s->motion.active){
            gh_actor_motion_bindings b=prefix->motion.b;
            b.effect=s->effect;b.context=s->effect_context;
            b.sound=s->sound;b.sound_context=s->sound_context;
            const uint8_t *row=resource->bytes+0x110;
            gh_actor_motion_status status=gh_actor_motion_call_open_002a8578(
                &s->motion,&prefix->motion,&b,word(row),word(row+4),0);
            if(status)return stop(s,s->motion.frontier?s->motion.frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(&s->motion);
        if(status)return stop(s,s->motion.frontier?s->motion.frontier:0x2a8578,status);
        put_float(p+0x600,10.0f);
        ++p[0x2f6];
        s->stage=HEAVY_POISON_STEP;
    }
    if(s->stage==HEAVY_POISON_STEP){
        if(!actor->pose.bindings.rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(&prefix->motion,
            *actor->pose.bindings.rate,prefix->bindings.policy,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->stage=HEAVY_POISON_ROOT;
    }
    if(s->stage==HEAVY_POISON_ROOT){
        gh_actor_motion_status status=heavy_root_motion_002da9b8(s,prefix);
        if(status)return status;
        s->stage=HEAVY_POISON_COUNTDOWN;
    }
    if(s->stage==HEAVY_POISON_COUNTDOWN){
        fenv_t env;
        const float previous=scalar(p+0x600),rate=scalar(p+0x5a8);
        if(!isfinite(previous)||!isfinite(rate)||!begin(&env))
            return stop(s,0x287090,GH_ACTOR_MOTION_UNSUPPORTED);
        const float time=sub(previous,rate);
        if(fesetenv(&env)||!isfinite(time))return stop(s,0x287090,GH_ACTOR_MOTION_UNSUPPORTED);
        put_float(p+0x600,time);
        if(time<=0){
            s->sound_request=(gh_motion_sound_request){.model=&prefix->model.model,
                .actor=root,.bank=2,.program=0x60};
            s->stage=HEAVY_POISON_VOICE;
        }else s->stage=12;
    }
    if(s->stage==HEAVY_POISON_VOICE){
        if(!s->sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t handle;
        gh_actor_motion_status status=s->sound(s->sound_context,&s->sound_request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->stage=HEAVY_POISON_RELOAD;
    }
    if(s->stage==HEAVY_POISON_RELOAD){
        fenv_t env;
        if(!prefix->bindings.random||!begin(&env))
            return stop(s,0x31d000,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t raw;
        if(gh_game_mt_next_0030f348(prefix->bindings.random,&raw)){
            (void)fesetenv(&env);
            return stop(s,0x31d000,GH_ACTOR_MOTION_ARGUMENT);
        }
        const uint32_t bits=gh_frame_unit_bits_0030f518_word(raw);
        const float timer=add(mul(scalar(&bits),60.0f),90.0f);
        if(fesetenv(&env))return stop(s,0x31d000,GH_ACTOR_MOTION_CALLEE);
        put_float(p+0x600,timer);
        s->stage=12;
    }
    return GH_ACTOR_MOTION_OK;
}

/* State 1 descriptor 3C03E8 -> 288570, substate 1 descriptor
 * 448C70 -> 2887D0. The 288F00 hit enters variant 1 for the
 * R100 stamp 6 and R102 stamp 14 EMA2 placements. */
static gh_actor_motion_status heavy_reaction_002887d0(gh_enemy_other_move *s,
    gh_enemy2a2_virtual_init *actor)
{
    gh_enemy2a2_init *prefix=&actor->prefix;
    gh_title_player_view *root=prefix->root;
    uint8_t *p=root->bytes;
    uint32_t pc=0;
    if(s->stage==5){
        const uint16_t room=prefix->bindings.work->room,stamp=half(p+0x640);
        const int opening_reaction=room==0x100&&stamp==6;
        const int r102_reaction=room==0x102&&stamp==14&&
            gh_enemy2a2_is_r102_entry(&prefix->request);
        if(word(p+0x564)!=0x2a2||p[0x2f5]!=1||(!opening_reaction&&!r102_reaction))
            return stop(s,0x288570,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=HEAVY_REACTION_SOLID;
    }
    if(s->stage==HEAVY_REACTION_SOLID){
        if(gh_collision_solid_set_enabled_001351d8(actor->pose.bindings.solids,root,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x15b0,word(p+0x15b0)|2u);
        s->stage=p[0x2f6]==0?HEAVY_REACTION_SELECT:
            p[0x2f6]==1?HEAVY_REACTION_STEP:12;
    }
    if(s->stage==HEAVY_REACTION_SELECT){
        const gh_object_view *resource=&prefix->resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0xb8||
            resource->file_extent>resource->size||prefix->motion.b.resource!=resource->bytes)
            return stop(s,0x288820,GH_ACTOR_MOTION_BOUNDS);
        uint32_t random;
        if(gh_game_mt_next_0030f348(prefix->bindings.random,&random))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->reaction_resource_offset=(random&1u)?0xa8:0xb0;
        s->stage=HEAVY_REACTION_BEGIN;
    }
    if(s->stage==HEAVY_REACTION_BEGIN){
        if(!s->motion.active){
            gh_actor_motion_bindings b=prefix->motion.b;
            b.effect=s->effect;b.context=s->effect_context;
            b.sound=s->sound;b.sound_context=s->sound_context;
            const uint8_t *row=prefix->resource.bytes+s->reaction_resource_offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                &s->motion,&prefix->motion,&b,word(row),word(row+4),0,10,0,1);
            if(status)return stop(s,s->motion.frontier?s->motion.frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(&s->motion);
        if(status)return stop(s,s->motion.frontier?s->motion.frontier:0x2a8578,status);
        s->stage=HEAVY_REACTION_SOUND;
    }
    if(s->stage==HEAVY_REACTION_SOUND){
        if(!p[0x2f7]){
            if(!s->sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!s->sound_request.model)s->sound_request=(gh_motion_sound_request){
                .model=&prefix->model.model,.actor=root,.bank=1,.program=0x5c};
            uint32_t handle;
            gh_actor_motion_status status=s->sound(s->sound_context,&s->sound_request,&handle,&pc);
            if(status)return stop(s,pc?pc:0x2cba48,status);
        }
        p[0x2f6]++;
        s->stage=HEAVY_REACTION_STEP;
    }
    if(s->stage==HEAVY_REACTION_STEP){
        if(!actor->pose.bindings.rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(&prefix->motion,
            *actor->pose.bindings.rate,prefix->bindings.policy,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->stage=HEAVY_REACTION_WORLD;
        if(ended){
            const int action=(word(p+0x15b0)&0x80u)!=0;
            p[0x2f4]=0;p[0x2f5]=action?7:2;p[0x2f6]=action?0x12:0;p[0x2f7]=0;
            if(action)s->stage=12;
        }
    }
    if(s->stage==HEAVY_REACTION_WORLD){
        gh_actor_motion_status status=heavy_root_matrix_0014a2b0(s,prefix);
        if(status)return status;
        s->stage=12;
    }
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_other_move_bind_navigation(gh_enemy_other_move *s,
    const gh_scene_nearcheck *navigation)
{
    if(!s||!navigation)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->navigation&&s->navigation!=navigation)return GH_ACTOR_MOTION_BUSY;
    s->navigation=navigation;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status heavy_home_route(gh_enemy_other_move *s,
    gh_enemy2a2_virtual_init *actor,const float origin[3])
{
    uint8_t *p=actor->prefix.root->bytes;
    float target[3];
    memcpy(target,p+0x1590,sizeof(target));
    const gh_actor_motion_status status=gh_enemy_route_clear_002a74e0(&s->route,p,origin,
        target,s->navigation,actor->pose.bindings.actors,actor->pose.bindings.init,
        actor->prefix.bindings.work->room);
    if(status)return stop(s,s->route.frontier,status);
    gh_enemy_route_waypoint_002a7ca0(p,p+0x1570);
    return GH_ACTOR_MOTION_OK;
}

/* 286D10 periodically refreshes a real shared PFD route while the heavy
 * enemy follows the selected home clip. Keep the per-call RNG and cue at
 * their original points so resuming a lower routine cannot duplicate them. */
static gh_actor_motion_status heavy_walk_home_00286d10(gh_enemy_other_move *s,
    gh_enemy2a2_virtual_init *actor)
{
    gh_enemy2a2_init *prefix=&actor->prefix;
    gh_title_player_view *root=prefix->root;
    uint8_t *p=root->bytes;
    const gh_boot_work *work=prefix->bindings.work;
    uint32_t pc=0;
    if(s->stage==5){
        float origin[3];
        fenv_t env;
        if(!point(root,origin)||!begin(&env))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
        const float x=sub(scalar(p+0x1590),origin[0]),z=sub(scalar(p+0x1598),origin[2]);
        s->home_distance=add(mul(x,x),mul(z,z));
        if(fesetenv(&env)||!isfinite(s->home_distance))
            return stop(s,0x1f7eb0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x15b0,word(p+0x15b0)&~1u);
        s->stage=((word((const uint8_t *)work+0x210)^p[0x2fc])&7u)?
            HEAVY_HOME_SELECT:HEAVY_HOME_ROUTE;
    }
    if(s->stage==HEAVY_HOME_ROUTE){
        float origin[3];
        if(!point(root,origin))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_status status=heavy_home_route(s,actor,origin);
        if(status)return status;
        s->stage=HEAVY_HOME_SELECT;
    }
    if(s->stage==HEAVY_HOME_SELECT){
        if(p[0x2f6]>3){s->stage=12;return GH_ACTOR_MOTION_OK;}
        if(p[0x2f6]==1||p[0x2f6]==3)s->stage=HEAVY_HOME_TURN;
        else{
            const uint32_t dependency=word(p+0x564);
            const int special=dependency==0x2a7||dependency==0x2ab;
            const int wounded=(int16_t)half(p+0x54a)<(int16_t)half(p+0x548);
            size_t row=0x178;
            if(p[0x2f6]==0){
                if(wounded)row=special?0x40:0xb8;
                else if(special)row=0x38;
                else{
                    uint32_t choice;
                    if(!prefix->bindings.random||
                        gh_game_mt_next_0030f348(prefix->bindings.random,&choice))
                        return stop(s,0x31cfe0,GH_ACTOR_MOTION_ARGUMENT);
                    row=(choice&1u)?0x38:0x40;
                }
            }
            s->reaction_resource_offset=(uint16_t)row;
            s->stage=HEAVY_HOME_BIND;
        }
    }
    if(s->stage==HEAVY_HOME_BIND){
        if(!s->motion.active){
            const size_t row=s->reaction_resource_offset;
            if(!prefix->resource.bytes||prefix->resource.file_extent<row+8)
                return stop(s,0x286d10,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_bindings b=prefix->motion.b;
            b.effect=s->effect;b.context=s->effect_context;
            b.sound=s->sound;b.sound_context=s->sound_context;
            const uint8_t *clip=prefix->resource.bytes+row;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                &s->motion,&prefix->motion,&b,word(clip),word(clip+4),0,10,0,1);
            if(status)return stop(s,s->motion.frontier?s->motion.frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(&s->motion);
        if(status)return stop(s,s->motion.frontier?s->motion.frontier:0x2a8578,status);
        s->sound_request=(gh_motion_sound_request){.model=&prefix->model.model,
            .actor=root,.bank=1,.program=p[0x2f6]?0x5c:0x5a};
        s->stage=HEAVY_HOME_CUE;
    }
    if(s->stage==HEAVY_HOME_CUE){
        if(!s->sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t handle;
        gh_actor_motion_status status=s->sound(s->sound_context,&s->sound_request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        ++p[0x2f6];
        s->stage=HEAVY_HOME_TURN;
    }
    if(s->stage==HEAVY_HOME_TURN){
        const float work_rate=scalar((const uint8_t *)work+0x55c);
        const float yaw=scalar(p+0x104);
        const gh_player_target_bindings *bindings=s->target_bindings;
        float origin[3],target[3],turn;
        fenv_t env;
        if(!bindings||!point(root,origin)||!isfinite(work_rate)||!begin(&env))
            return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(target,p+0x1570,sizeof(target));
        const float cap=mul(0.09817477f,work_rate);
        const gh_player_target_status status=gh_player_target_angle_002da038(bindings->elf,
            bindings->elf_bytes,origin,target,yaw,cap,&turn,&pc);
        float next=status?0:add(yaw,turn);
        if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_id_angle_reference_001f7d48(next,&next))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put_float(p+0x104,next);
        s->stage=HEAVY_HOME_STEP;
    }
    if(s->stage==HEAVY_HOME_STEP){
        if(!actor->pose.bindings.rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(&prefix->motion,
            *actor->pose.bindings.rate,prefix->bindings.policy,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->stage=p[0x2f6]==3?HEAVY_HOME_SPECIAL_WORLD:HEAVY_HOME_WORLD;
        if(ended&&p[0x2f6]==3)p[0x2f6]=0;
        else if(ended&&!(word(p+0x15b0)&0x200u)&&s->home_distance>100.0f){
            const uint32_t dependency=word(p+0x564);
            if(dependency!=0x2a7&&dependency!=0x2ab)s->stage=HEAVY_HOME_DRAW;
        }
    }
    if(s->stage==HEAVY_HOME_DRAW){
        uint32_t choice;
        if(!prefix->bindings.random||gh_game_mt_next_0030f348(prefix->bindings.random,&choice))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_ARGUMENT);
        if(!(choice&7u)){
            p[0x2f6]=2;
            put(p+0x15b0,word(p+0x15b0)|0x200u);
        }
        s->stage=HEAVY_HOME_WORLD;
    }
    if(s->stage==HEAVY_HOME_WORLD||s->stage==HEAVY_HOME_SPECIAL_WORLD){
        gh_actor_motion_status status=heavy_root_motion_002da9b8(s,prefix);
        if(status)return status;
        if(s->stage==HEAVY_HOME_WORLD&&s->home_distance<1.0f)s->stage=HEAVY_HOME_ARRIVAL;
        else s->stage=12;
    }
    if(s->stage==HEAVY_HOME_ARRIVAL){
        fenv_t env;
        if(!begin(&env))return stop(s,0x286fa4,GH_ACTOR_MOTION_CALLEE);
        const float height=fabsf(sub(scalar(p+0x1594),scalar(root->alias_f0+4)));
        if(fesetenv(&env)||!isfinite(height))return stop(s,0x286fa4,GH_ACTOR_MOTION_UNSUPPORTED);
        if(height<1.0f)memset(p+0x2f4,0,4);
        s->stage=12;
    }
    return GH_ACTOR_MOTION_OK;
}

enum {
    HEAVY_IDLE_BIND=61, HEAVY_IDLE_STEP, HEAVY_IDLE_WORLD, HEAVY_IDLE_PROXIMITY,
    HEAVY_IDLE_RISE_TURN, HEAVY_IDLE_RISE_STEP
};

/* 285F78 settles the west cEmA2 after its original walk home, then holds
 * the second clip until the timed mode-1 proximity query permits a rise. */
static gh_actor_motion_status heavy_idle_00285f78(gh_enemy_other_move *s,
    gh_enemy2a2_virtual_init *actor)
{
    gh_enemy2a2_init *prefix=&actor->prefix;
    const gh_boot_work *work=prefix->bindings.work;
    uint8_t *p=prefix->root->bytes;
    uint32_t pc=0;
    if(s->stage==5){
        if(word(p+0x15c4))return stop(s,0x1c2490,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!p[0x2f6]&&!(word(p+0x15b0)&0x40u)&&scalar(p+0x618)<100.0f){
            int found;
            gh_actor_motion_status status=mode1_lookup_00291010(s,actor,&found);
            if(status)return status;
            if(!found)p[0x2f6]=(word(p+0x15b0)&4u)?6:8;
        }
        if(p[0x2f6]>5){
            static const uint32_t phase_entry[]={0x2861d0,0x286208,0x28626c,0x286334,0x2863a4,
                0x2863dc,0x2864c0,0x2864f8,0x286554,0x28658c};
            if(p[0x2f6]<14)return stop(s,phase_entry[p[0x2f6]-4],GH_ACTOR_MOTION_UNSUPPORTED);
            s->stage=12;
            return GH_ACTOR_MOTION_OK;
        }
        if(p[0x2f6]==5)s->stage=HEAVY_IDLE_RISE_TURN;
        else s->stage=(p[0x2f6]&1u)?HEAVY_IDLE_STEP:HEAVY_IDLE_BIND;
    }
    if(s->stage==HEAVY_IDLE_BIND){
        if(!s->motion.active){
            gh_actor_motion_bindings b=prefix->motion.b;
            b.effect=s->effect;b.context=s->effect_context;
            b.sound=s->sound;b.sound_context=s->sound_context;
            const size_t row=p[0x2f6]==4?0x30:(p[0x2f6]?0x28:0x48);
            if(!prefix->resource.bytes||prefix->resource.file_extent<row+8)
                return stop(s,p[0x2f6]==4?0x2861d0:(p[0x2f6]?0x2860d0:0x286078),GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                &s->motion,&prefix->motion,&b,word(prefix->resource.bytes+row),
                word(prefix->resource.bytes+row+4),0,10,0,1);
            if(status)return stop(s,s->motion.frontier?s->motion.frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(&s->motion);
        if(status)return stop(s,s->motion.frontier?s->motion.frontier:0x2a8578,status);
        ++p[0x2f6];
        s->stage=p[0x2f6]==5?HEAVY_IDLE_RISE_TURN:HEAVY_IDLE_STEP;
    }
    if(s->stage==HEAVY_IDLE_RISE_TURN){
        /* 286208 turns towards Gene until the original SEQ bit 1. */
        if(!(half(p+0x3ac)&2u)){
            const gh_player_target_bindings *bindings=s->target_bindings;
            const float rate=scalar(p+0x5a8);
            const float work_rate=scalar((const uint8_t *)work+0x55c);
            const float yaw=scalar(p+0x104);
            float origin[3],target[3],delta;
            fenv_t env;
            if(!bindings||!point(prefix->root,origin)||!point(s->player,target)||
               !isfinite(rate)||!isfinite(work_rate)||!begin(&env))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float cap=mul(mul(rate,0x1.921fb6p-5f),work_rate);
            const gh_player_target_status status=gh_player_target_angle_002da038(
                bindings->elf,bindings->elf_bytes,origin,target,yaw,cap,&delta,&pc);
            float next=status?yaw:add(yaw,delta);
            if(fesetenv(&env)||status||gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            put_float(p+0x104,next);
        }
        s->stage=HEAVY_IDLE_RISE_STEP;
    }
    if(s->stage==HEAVY_IDLE_RISE_STEP){
        if(!actor->pose.bindings.rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(&prefix->motion,
            *actor->pose.bindings.rate,prefix->bindings.policy,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(ended)p[0x2f6]=(word(p+0x15b0)&4u)?6:8;
        s->stage=HEAVY_IDLE_WORLD;
    }
    if(s->stage==HEAVY_IDLE_STEP){
        if(p[0x2f6]==3){
            put(p+0x15b0,word(p+0x15b0)|1u);
            const float opacity=scalar(p+0x24c);
            if(!isfinite(opacity))return stop(s,0x286108,GH_ACTOR_MOTION_UNSUPPORTED);
            if(opacity<=0)s->stage=HEAVY_IDLE_PROXIMITY;
        }
        if(s->stage==HEAVY_IDLE_STEP){
            if(!actor->pose.bindings.rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
            int ended;
            gh_actor_motion_status status=gh_actor_motion_step_00157c18(&prefix->motion,
                *actor->pose.bindings.rate,prefix->bindings.policy,&ended,&pc);
            if(status)return stop(s,pc?pc:0x157c18,status);
            if(ended&&p[0x2f6]==1)++p[0x2f6];
            s->stage=HEAVY_IDLE_WORLD;
        }
    }
    if(s->stage==HEAVY_IDLE_WORLD){
        gh_actor_motion_status status=heavy_root_motion_002da9b8(s,prefix);
        if(status)return status;
        s->stage=p[0x2f6]==3?HEAVY_IDLE_PROXIMITY:12;
    }
    if(s->stage==HEAVY_IDLE_PROXIMITY){
        if(!(word(p+0x15b0)&0x40u)&&!((word((const uint8_t *)work+0x210)^p[0x2fc])&7u)&&
           scalar(p+0x618)<100.0f){
            int found;
            gh_actor_motion_status status=mode1_lookup_00291010(s,actor,&found);
            if(status)return status;
            if(!found)++p[0x2f6];
        }
        s->stage=12;
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status heavy_move_substate_002866f8(gh_enemy_other_move *s,
    gh_enemy2a2_virtual_init *actor)
{
    gh_enemy2a2_init *prefix=&actor->prefix;
    gh_title_player_view *root=prefix->root;
    uint8_t *p=root->bytes;
    const gh_static_transform_policy policy=prefix->bindings.policy;
    uint32_t pc=0;
    if(s->stage==5){
        if(p[0x2f4]!=0)return stop(s,0x2856c0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(p[0x2f5]>12)p[0x2f5]=0;
        if(p[0x2f5]!=2||p[0x2f6]>3)return stop(s,p[0x2f5]==2?0x2866f8:0x285eb8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!s->opacity)return stop(s,0x2866f8,GH_ACTOR_MOTION_UNSUPPORTED);
        int found;
        gh_actor_motion_status status=mode1_lookup_00291010(s,actor,&found);
        if(status)return status;
        s->nearby=found!=0;
        s->stage=6;
    }
    if(s->stage==6){
        if(gh_collision_solid_set_enabled_001351d8(actor->pose.bindings.solids,root,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!p[0x2f6])put(p+0x15b0,word(p+0x15b0)&~1u);
        switch(p[0x2f6]){
        case 0:s->stage=7;break;
        case 1:s->stage=8;break;
        case 2:s->stage=HEAVY_LEAVE_BIND;break;
        default:s->stage=HEAVY_LEAVE_STEP;break;
        }
    }
    if(s->stage==7||s->stage==HEAVY_LEAVE_BIND){
        const int leaving=s->stage==HEAVY_LEAVE_BIND;
        if(!s->motion.active){
            gh_actor_motion_bindings b=prefix->motion.b;
            b.effect=s->effect;b.context=s->effect_context;
            b.sound=s->sound;b.sound_context=s->sound_context;
            const size_t row=leaving?0x30:0x28;
            if(!prefix->resource.bytes||prefix->resource.file_extent<row+8)
                return stop(s,leaving?0x28697c:0x2867ac,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                &s->motion,&prefix->motion,&b,word(prefix->resource.bytes+row),
                word(prefix->resource.bytes+row+4),0,10,0,1);
            if(status)return stop(s,s->motion.frontier?s->motion.frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(&s->motion);
        if(status)return stop(s,s->motion.frontier?s->motion.frontier:0x2a8578,status);
        if(leaving){
            put(p+0x15b0,word(p+0x15b0)&~1u);
            p[0x1560]=0;
            s->stage=HEAVY_LEAVE_STEP;
        }else{
            memmove(p+0x5c0,root->alias_f0,12);
            s->stage=8;
        }
        ++p[0x2f6];
    }
    if(s->stage==8){
        fenv_t env;
        if(!begin(&env))return stop(s,0x2868b0,GH_ACTOR_MOTION_CALLEE);
        float x=add(mul(scalar(root->alias_f0),.99f),mul(scalar(p+0x5c0),.01f));
        float z=add(mul(scalar(root->alias_f0+8),.99f),mul(scalar(p+0x5c8),.01f));
        if(fesetenv(&env)||!isfinite(x)||!isfinite(z))return stop(s,0x2868b0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x15b0,word(p+0x15b0)|2u);
        put_float(root->alias_f0,x);put_float(root->alias_f0+8,z);
        s->stage=9;
    }
    if(s->stage==9){
        int ended;
        if(!actor->pose.bindings.rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(&prefix->motion,
            *actor->pose.bindings.rate,policy,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->stage=10;
    }
    if(s->stage==10){
        gh_actor_motion_status status=heavy_root_motion_002da9b8(s,prefix);
        if(status)return status;
        s->stage=11;
    }
    if(s->stage==11){
        if(!(word(p+0x15b0)&0x40u)){
            if(p[0x1560]!=5){
                if(scalar(p+0x618)<16){
                    gh_scene_ray_request ray={.mask=1,.flags=0x18};
                    if(!point(root,ray.from)||!point(s->player,ray.to))return stop(s,0x289328,GH_ACTOR_MOTION_BOUNDS);
                    fenv_t env;
                    if(!begin(&env))return stop(s,0x289328,GH_ACTOR_MOTION_CALLEE);
                    ray.from[1]=add(ray.from[1],1);ray.to[1]=add(ray.to[1],1);
                    if(fesetenv(&env))return stop(s,0x289328,GH_ACTOR_MOTION_CALLEE);
                    ray.from[3]=ray.to[3]=1;
                    int hit;
                    if(gh_scene_ray_query_00131e10(actor->pose.bindings.actors,&ray,&hit,&pc))
                        return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
                    if(hit!=1)p[0x2f6]=2;
                }
                if(!s->nearby)p[0x2f6]=2;
            }
            if(word(p+0x15b0)&8u)p[0x2f6]=2;
            if(scalar(p+0x15bc)<=0)s->stage=HEAVY_TIMER_DRAW;
        }
        if(s->stage==11)s->stage=12;
    }
    if(s->stage==HEAVY_LEAVE_STEP){
        int ended;
        if(!actor->pose.bindings.rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(&prefix->motion,
            *actor->pose.bindings.rate,policy,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->stage=ended?HEAVY_LEAVE_HOME:HEAVY_LEAVE_ROOT;
    }
    if(s->stage==HEAVY_LEAVE_HOME){
        float origin[4];
        if(!point(root,origin))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_BOUNDS);
        fenv_t env;
        if(!begin(&env))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
        const float x=sub(scalar(p+0x1590),origin[0]);
        const float y=sub(scalar(p+0x1594),origin[1]);
        const float z=sub(scalar(p+0x1598),origin[2]);
        const float range=add(mul(x,x),mul(z,z));
        if(fesetenv(&env)||!isfinite(range)||!isfinite(y))
            return stop(s,0x1f7eb0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(range>=1.0f||fabsf(y)>=1.0f){
            gh_actor_motion_status status=heavy_home_route(s,actor,origin);
            if(status)return status;
            memset(p+0x2f4,0,4);
            p[0x2f5]=4;
            s->stage=HEAVY_LEAVE_ROOT;
        }else{
            memset(p+0x2f4,0,4);
            s->stage=12;
        }
    }
    if(s->stage==HEAVY_LEAVE_ROOT){
        gh_actor_motion_status status=heavy_root_motion_002da9b8(s,prefix);
        if(status)return status;
        s->stage=12;
    }
    /* Original 286928 reloads the timer before the optional 28696C voice. */
    if(s->stage==HEAVY_TIMER_DRAW){
        if(!prefix->bindings.random||gh_game_mt_next_0030f348(prefix->bindings.random,&s->timer_word))
            return stop(s,0x31d000,GH_ACTOR_MOTION_ARGUMENT);
        s->stage=HEAVY_TIMER_CONVERT;
    }
    if(s->stage==HEAVY_TIMER_CONVERT){
        uint32_t timer;
        gh_actor_motion_status status=gh_enemy2a2_mt_timer_reference(s->timer_word,
            GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&timer);
        if(status)return stop(s,0x31d000,status);
        put(p+0x15bc,timer);
        if(s->nearby){
            s->sound_request=(gh_motion_sound_request){.model=&prefix->model.model,
                .actor=root,.bank=1,.program=0x5a};
            s->stage=HEAVY_TIMER_SOUND;
        }else s->stage=12;
    }
    if(s->stage==HEAVY_TIMER_SOUND){
        if(!s->sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t handle;
        gh_actor_motion_status status=s->sound(s->sound_context,&s->sound_request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->stage=12;
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status heavy_move_tail(gh_enemy_other_move *s,gh_enemy2a2_virtual_init *actor)
{
    gh_enemy2a2_init *prefix=&actor->prefix;
    gh_title_player_view *root=prefix->root;
    gh_actor_model *model=&prefix->model.model;
    uint8_t *p=root->bytes;
    const gh_static_transform_policy policy=prefix->bindings.policy;
    uint32_t pc=0;
    if(s->stage==12){
        if(gh_game_object_opacity_002a87e8(root,NULL,prefix->bindings.work,s->opacity,
            GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))return stop(s,pc?pc:0x2a87e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=13;
    }
    if(s->stage==13){
        const uint32_t move_flags=word(p+0x15b0);
        uint32_t flags=word(p+0x250)&~2u;
        const int faded=scalar(p+0x618)>900&&(move_flags&1u)&&p[0x1604]&&!(move_flags&16u);
        if(half(p+0x2f4)!=0x0202){
            float alpha=scalar(p+0x24c);
            fenv_t env;
            if(!isfinite(alpha)||!begin(&env))return stop(s,0x2859a0,GH_ACTOR_MOTION_CALLEE);
            alpha=faded?sub(alpha>1?1:alpha,.1f):add(alpha,.1f);
            if(fesetenv(&env)||!isfinite(alpha))return stop(s,0x2859a0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(faded){if(alpha<=.15f){flags=(flags|2u)&~16u;alpha=0;}else flags|=16u;}
            else if(alpha>=1){alpha=1;flags&=~16u;}else flags|=16u;
            if(move_flags&16u){alpha=1;flags&=~16u;}
            put_float(p+0x24c,alpha);
        }
        if(p[0x623])flags|=2u;
        put(p+0x250,flags);
        if(!point(root,s->old))return stop(s,0x285a40,GH_ACTOR_MOTION_BOUNDS);
        s->stage=faded?31:14;
    }
    if(s->stage==31){
        if(gh_collision_solid_set_enabled_001351d8(actor->pose.bindings.solids,root,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=20;
    }
    if(s->stage==14){
        if(half(p+0x3ae))return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_actor_model_update_hierarchy_00149eb8(model,policy))
            return stop(s,model->frontier?model->frontier:0x149eb8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=15;
    }
    if(s->stage==15){
        gh_motion_solve_bindings b={.model=model,.work=prefix->bindings.work,.actors=actor->pose.bindings.actors,
            .ik_globals=prefix->bindings.ik,.atan_coeff=actor->pose.bindings.atan_coeff};
        gh_actor_motion_status status=gh_actor_motion_solve_00152a20(&s->solve,&b,policy,&pc);
        if(status)return stop(s,pc?pc:0x152a20,status);
        s->stage=16;
    }
    if(s->stage==16){
        if(gh_actor_finish_model_resume_0014a038(&s->finish,model,policy))
            return stop(s,model->frontier?model->frontier:0x14a038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x564)==0x2a7||word(p+0x564)==0x2ab)
            return stop(s,0x2890a0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=17;
    }
    if(s->stage==17){
        int hit;
        if(gh_collision_solid_step_resume_001348e8(actor->pose.bindings.solids,root,policy,&s->solid,&hit,&pc))
            return stop(s,pc?pc:0x1348e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=18;
    }
    s->sphere.work=prefix->bindings.work;
    gh_player_terrain_bindings terrain={actor->pose.bindings.actors,root,gh_terrain_sphere_query_001321d8,&s->sphere};
    if(s->stage==18){
        if(!(word(p+0x15b0)&0x30u)){
            float position[3];int hit;
            if(gh_player_terrain_wall_00132da0(&s->wall,&terrain,.4f,0,position,NULL,0,NULL,&hit,&pc))
                return stop(s,pc?pc:0x132da0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit){put_float(root->alias_f0,position[0]);put_float(root->alias_f0+8,position[2]);}
        }
        s->stage=19;
    }
    if(s->stage==19){
        if(!(word(p+0x15b0)&0x10000u)){
            float position[3],normal[3];int hit;
            if(gh_player_terrain_floor_001329c0(&s->floor,&terrain,10,position,normal,&s->report,&hit,&pc))
                return stop(s,pc?pc:0x1329c0,GH_ACTOR_MOTION_UNSUPPORTED);
            fenv_t env;
            if(!begin(&env))return stop(s,0x285b60,GH_ACTOR_MOTION_CALLEE);
            const float velocity=sub(scalar(p+0x15b8),scalar(p+0x15b4));
            const float y=scalar(root->alias_f0+4),next=add(y,velocity),floor=hit?position[1]:y;
            if(fesetenv(&env)||!isfinite(velocity)||!isfinite(next)||!isfinite(floor))
                return stop(s,0x285b60,GH_ACTOR_MOTION_UNSUPPORTED);
            put_float(p+0x15b8,next<floor?0:velocity);
            put_float(root->alias_f0+4,next<floor?floor:next);
        }
        s->stage=20;
    }
    float now[3],delta[3];fenv_t env;
    if(!point(root,now)||!begin(&env))return stop(s,0x285bd0,GH_ACTOR_MOTION_UNSUPPORTED);
    for(unsigned i=0;i<3;i++)delta[i]=sub(now[i],s->old[i]);
    if(fesetenv(&env))return stop(s,0x285bd0,GH_ACTOR_MOTION_CALLEE);
    memcpy(p+0x550,delta,12);
    memcpy(p+0x490,now,12);
    return finished(s);
}

gh_actor_motion_status gh_enemy2a2_move_002856c0(gh_enemy_other_move *s,
    gh_enemy2a2_virtual_init *actor,const gh_title_player_view *player)
{
    if(!s||!actor||!actor->final.complete||!actor->prefix.root||
       !actor->prefix.bindings.work||!actor->pose.bindings.solids||!player||
       !player->alias_f0||!actor->prefix.root->alias_f0)
        return GH_ACTOR_MOTION_ARGUMENT;
    if(s->owner&&(s->owner!=actor||s->player!=player))return GH_ACTOR_MOTION_BUSY;
    if(s->original_returned)return GH_ACTOR_MOTION_OK;
    s->owner=actor;
    s->player=player;
    s->frontier=0;
    gh_title_player_view *root=actor->prefix.root;
    uint8_t *p=root->bytes;
    const gh_boot_work *work=actor->prefix.bindings.work;
    if(!s->stage){
        if(word((const uint8_t*)work+0x5d8)&0x40000000u)return finished(s);
        fenv_t env;
        if(!begin(&env))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
        float x=sub(scalar(player->alias_f0),scalar(root->alias_f0));
        float z=sub(scalar(player->alias_f0+8),scalar(root->alias_f0+8));
        float range=add(mul(x,x),mul(z,z));
        if(fesetenv(&env)||!isfinite(range))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_UNSUPPORTED);
        put_float(p+0x618,range);
        s->stage=1;
    }
    if(s->stage==1){
        if((int16_t)half(p+0x54a)>0){
            gh_hitarea_object *hit=&actor->pose.hitarea;
            if(word(p+0x670)||!actor->pose.hitarea_created||!hit->bytes)
                return stop(s,0x285c58,GH_ACTOR_MOTION_UNSUPPORTED);
            if((word(hit->bytes+0x60)&1u)&&scalar(p+0x54c)<=0)
                return stop(s,0x285c58,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        put(p+0x15b0,word(p+0x15b0)&0xfffeffddu);
        s->stage=2;
    }
    if(s->stage==2){
        uint32_t pc=0;
        if(gh_collision_solid_set_enabled_001351d8(actor->pose.bindings.solids,root,1,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=3;
    }
    if(s->stage==3){
        const float rate=scalar(p+0x5a8),timer_a=scalar(p+0x15bc),timer_b=scalar(p+0x15c0);
        fenv_t env;
        if(!isfinite(rate)||!isfinite(timer_a)||!isfinite(timer_b)||!begin(&env))
            return stop(s,0x285728,GH_ACTOR_MOTION_UNSUPPORTED);
        if(timer_a>0)put_float(p+0x15bc,sub(timer_a,rate));
        if(timer_b>0)put_float(p+0x15c0,sub(timer_b,rate));
        if(fesetenv(&env))return stop(s,0x285728,GH_ACTOR_MOTION_CALLEE);
        s->stage=4;
    }
    if(s->stage==4){
        int found;
        gh_actor_motion_status status=mode1_lookup_00291010(s,actor,&found);
        if(status)return status;
        if(found)put(p+0x15c0,0x43160000u);
        put(p+0x600,word(p+0x600)&0x7fffffffu);
        s->stage=5;
    }
    if((s->stage==5&&p[0x2f4]==1)||
       (s->stage>=HEAVY_REACTION_SOLID&&s->stage<=HEAVY_REACTION_WORLD)){
        gh_actor_motion_status status=heavy_reaction_002887d0(s,actor);
        if(status)return status;
    }else if((s->stage==5&&!p[0x2f4]&&p[0x2f5]==5)||
             (s->stage>=HEAVY_POISON_SOLID&&s->stage<=HEAVY_POISON_RELOAD)){
        gh_actor_motion_status status=heavy_poison_00287090(s,actor);
        if(status)return status;
    }else if((s->stage==5&&!p[0x2f4]&&p[0x2f5]==4)||
             (s->stage>=HEAVY_HOME_ROUTE&&s->stage<=HEAVY_HOME_ARRIVAL)){
        gh_actor_motion_status status=heavy_walk_home_00286d10(s,actor);
        if(status)return status;
    }else if((s->stage==5&&!half(p+0x2f4)&&work->room==0x100&&
              word(p+0x564)==0x2a2&&half(p+0x640)==6)||
             (s->stage>=HEAVY_IDLE_BIND&&s->stage<=HEAVY_IDLE_RISE_STEP)){
        gh_actor_motion_status status=heavy_idle_00285f78(s,actor);
        if(status)return status;
    }else if(s->stage<12||(s->stage>=HEAVY_TIMER_CONVERT&&s->stage<=HEAVY_TIMER_DRAW)||
             (s->stage>=HEAVY_LEAVE_BIND&&s->stage<=HEAVY_LEAVE_ROOT)){
        gh_actor_motion_status status=heavy_move_substate_002866f8(s,actor);
        if(status)return status;
    }
    return heavy_move_tail(s,actor);
}

gh_actor_motion_status gh_enemy363_move_001c9860(gh_enemy_other_move *s,
    gh_enemy_weapon_init *actor,gh_motion_effect_consumer effect,void *effect_context)
{
    if(!s||!actor||!actor->weapon||actor->attach_stage!=5||!actor->parent||
       !actor->parent_model||!actor->weapon->bindings.init||!actor->weapon->bindings.work)
        return GH_ACTOR_MOTION_ARGUMENT;
    if(s->owner&&s->owner!=actor)return GH_ACTOR_MOTION_BUSY;
    if(s->original_returned)return GH_ACTOR_MOTION_OK;
    s->owner=actor;
    s->frontier=0;
    gh_title_player_view *root=&actor->weapon->object.view;
    uint8_t *p=root->bytes;
    const gh_boot_work *work=actor->weapon->bindings.work;
    const gh_static_transform_policy policy=actor->weapon->bindings.policy;
    if(!s->stage){
        if(word((const uint8_t*)work+0x5d8)&0x20000000u)return finished(s);
        put(p+0x5b4,0);
        s->stage=1;
    }
    while(s->stage<=2){
        unsigned i=s->stage-1;
        if(word(p+0x650+8*i))return stop(s,0x1c9860,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_hitarea_object *hit=&actor->hit[i];
        if(hit->bytes){
            put_half(hit->bytes+0x46,0);
            put(hit->bytes+0x4c,0);
            uint32_t pc=0;
            if(gh_hitarea_object_set_active_001fd668(hit,0,&pc))
                return stop(s,pc?pc:0x1fd668,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        ++s->stage;
    }
    if(s->stage==3){
        const float base=scalar(actor->weapon->bindings.init->enemies+0x548);
        const float timer=scalar(p+0x66c);
        fenv_t env;
        if(!isfinite(base)||!isfinite(work->canonical_rate574)||!isfinite(timer)||!begin(&env))
            return stop(s,0x292f50,GH_ACTOR_MOTION_UNSUPPORTED);
        float rate=mul(base,work->canonical_rate574);
        if(isfinite(rate)){
            put_float(p+0x5a8,rate);
            if(timer>0)put_float(p+0x66c,sub(timer,rate));
        }
        if(fesetenv(&env)||!isfinite(rate))return stop(s,0x292f50,GH_ACTOR_MOTION_UNSUPPORTED);
        p[0x664]=0;p[0x665]=0;
        s->stage=4;
    }
    if(s->stage==4){
        if(p[0x2f4]!=0)return stop(s,0x1c9860,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint16_t kind=half(p+0x2fe);
        /* PMF 3BDEA0 selects 1CA040 for all three held weapon kinds. */
        if(p[0x2f5]!=1||kind<0x361||kind>0x363||kind!=actor->weapon->kind)
            return stop(s,0x1c9a50,GH_ACTOR_MOTION_UNSUPPORTED);
        if(p[0x2f6]==0){
            p[0x531]=0xff;
            s->stage=5;
        }else s->stage=6;
    }
    if(s->stage==5){
        uint32_t pc=0;
        if(!effect)return stop(s,0x306200,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=effect(effect_context,root,8,2,&pc);
        if(status)return stop(s,pc?pc:0x306200,status);
        ++p[0x2f6];
        s->stage=6;
    }
    if(s->stage==6){
        if(scalar(p+0x684)>0)return stop(s,0x1ca040,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=7;
    }
    if(s->stage==7){
        uint32_t pc=0;
        if(gh_game_model_texture_scroll_0014e4d0(&actor->model,scalar(p+0x5a8),&s->texture_cursor,&pc))
            return stop(s,pc?pc:0x14e4d0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=8;
    }
    if(s->stage==8){
        const gh_event_weapon_attachment attachment={root,&actor->model,
            actor->parent,actor->parent_model,word(p+0x604)};
        uint32_t pc=0;
        if(gh_event_weapon_follow_001cf3a8(&attachment,policy,&pc))
            return stop(s,pc?pc:0x1cf3a8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=9;
    }
    if(gh_actor_model_update_hierarchy_00149eb8(&actor->model,policy))
        return stop(s,actor->model.frontier?actor->model.frontier:0x149eb8,GH_ACTOR_MOTION_UNSUPPORTED);
    memmove(p+0x490,root->alias_f0,12);
    memmove(p+0x5d0,p+0x100,12);
    return finished(s);
}
