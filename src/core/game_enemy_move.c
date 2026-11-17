#include "game_scene_at_geometry.h"
#include "game_scene_at_jump.h"
#include "id_layout_rotation.h"
#include "game_player_root_delta.h"
#include "game_enemy_route_graph.h"
#include "game_player_targets.h"
#include "game_enemy_move.h"
#include "game_om_initialize.h"
#include "game_enemy_row2.h"
#include "game_enemy_request.h"
#include "game_model_texture_scroll.h"
#include "game_ray_query.h"
#include "game_actor_inverse.h"
#include "sdk_rotation_trig.h"
#include "frame_state.h"
#include <fenv.h>
#include <math.h>
#include <string.h>

enum {
    HEADINGS=1, HIT_CHECK, TIMERS, TEXTURES, SOLIDS, ATTACK_AREAS,
    PREDICT_TARGET, SELECT_AI, RANK_AI, AI_CONTINUATION, RETURNED
};

static uint32_t word(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static void put(uint8_t *p,uint32_t v){memcpy(p,&v,4);}
static uint16_t half(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static void put_half(uint8_t *p,uint16_t v){memcpy(p,&v,2);}
static float number(uint32_t bits){float v;memcpy(&v,&bits,4);return v;}
static uint32_t bits(float value){uint32_t v;memcpy(&v,&value,4);return v;}
static float sub(float a,float b){volatile float v=a-b;return v;}
static float add(float a,float b){volatile float v=a+b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static float ordered_abs(float f){return f<0?-f:f;}
static int positive_hp(const uint8_t *p){unsigned hp=half(p+0x54a);return hp&&hp<0x8000;}
static gh_actor_motion_status stop(gh_enemy202_move *s,uint32_t pc,gh_actor_motion_status e)
{
    s->frontier=pc;
    return e;
}

static int position(const gh_title_player_view *view,float xyz[3])
{
    if(!view||!view->bytes||!view->alias_f0)return 0;
    uintptr_t start=(uintptr_t)view->bytes,alias=(uintptr_t)view->alias_f0;
    if(alias<start||alias-start>view->size||view->size-(alias-start)<12)return 0;
    memcpy(xyz,view->alias_f0,12);
    return isfinite(xyz[0])&&isfinite(xyz[1])&&isfinite(xyz[2]);
}

static int set_round(fenv_t *env,int mode)
{
    if(feholdexcept(env))return 0;
    if(!fesetround(mode))return 1;
    (void)fesetenv(env);
    return 0;
}

static const uint8_t *elf_span(const void *image,size_t size,uint32_t address,size_t count_bytes)
{
    const uint8_t *elf=image;
    if(!elf||size<52||memcmp(elf,"\177ELF\1\1\1",7)||half(elf+18)!=8||half(elf+42)!=32)
        return NULL;
    uint32_t offset=word(elf+28);
    unsigned count=half(elf+44);
    if(offset>size||count>(size-offset)/32)return NULL;
    for(unsigned i=0;i<count;i++){
        const uint8_t *ph=elf+offset+(size_t)i*32;
        uint32_t base=word(ph+8),extent=word(ph+16),file=word(ph+4);
        if(word(ph)!=1||address<base||(uint64_t)address+count_bytes>(uint64_t)base+extent)continue;
        uint64_t start=(uint64_t)file+address-base;
        return start<=size&&size-start>=count_bytes?elf+(size_t)start:NULL;
    }
    return NULL;
}

static int body_allows_no_hit(gh_enemy_move_actor *actor)
{
    const gh_hitarea_object *body=&actor->hitareas->units[0];
    const uint8_t *p=actor->entry->object.view.bytes;
    if(actor->hitareas->created[0]){
        gh_title_player_view *owner;
        if(gh_hitarea_object_actor(body,&owner)||(!body->fallback&&owner!=&actor->entry->object.view)||
            body->size<0x64)return -1;
    }
    float drain=number(word(p+0x770)),cooldown=number(word(p+0x54c));
    if(!isfinite(drain)||!isfinite(cooldown)||(drain>0&&positive_hp(p)))return 0;
    if(!actor->hitareas->created[0]||!(word(body->bytes+0x60)&1u)||cooldown>0)return 1;
    return !(half(p+0x3ac)&8u)&&half(body->bytes+0x46)==0x2b;
}

static gh_actor_motion_status tick_timers(gh_enemy202_move *s,uint8_t *p)
{
    static const uint16_t counters[]={0x16dc,0x16e0,0x16ec};
    static const uint16_t timers[]={0x17cc,0x1714,0x16e4,0x1720,0x1724,0x1728,
        0x172c,0x1730,0x173c,0x1740,0x1744,0x1734,0x1748,0x174c,0x1750,
        0x1760,0x1764,0x1768,0x176c,0x1770,0x1754,0x16c0};
    const float rate=number(word(p+0x5a8));
    if(!isfinite(rate))return stop(s,0x2090c4,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t next[sizeof(timers)/sizeof(*timers)];
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x2090c4,GH_ACTOR_MOTION_CALLEE);
    int valid=1;
    for(unsigned i=0;i<sizeof(timers)/sizeof(*timers);i++){
        float value=number(word(p+timers[i]));
        if(!isfinite(value)){valid=0;break;}
        if(timers[i]==0x1754&&(half(p+0x3ac)&8u))value=10;
        else if(value>0&&!(timers[i]==0x16c0&&value==999999.9f))value=sub(value,rate);
        if(!isfinite(value)){valid=0;break;}
        next[i]=bits(value);
    }
    if(fesetenv(&env)||!valid)return stop(s,0x2090c4,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t flags=word(p+0x16d4);
    put(p+0x16d0,word(p+0x16d0)&0x77288b82u);
    put(p+0x16d4,flags&0xffb1dbbbu);
    p[0x617]=(uint8_t)((flags>>21)&1u);
    for(unsigned i=0;i<3;i++){
        uint32_t value=word(p+counters[i]);
        if(value)put(p+counters[i],value-1);
    }
    if(!positive_hp(p))put(p+0x16ec,0);
    uint16_t react=half(p+0x17ca);
    if(react)put_half(p+0x17ca,(uint16_t)(react-1));
    if(number(word(p+0x17cc))>0&&number(next[0])<=0){
        put(p+0x5b4,0);
        put(p+0x16d4,word(p+0x16d4)&~0x10000000u);
    }
    for(unsigned i=0;i<sizeof(timers)/sizeof(*timers);i++)put(p+timers[i],next[i]);
    if(p[0x17b0])p[0x17b0]--;
    return GH_ACTOR_MOTION_OK;
}

static float distance_xz(const float a[3],const float b[3])
{
    float x=sub(a[0],b[0]),z=sub(a[2],b[2]);
    return add(mul(x,x),mul(z,z));
}

static int r101_scenery_enemy(const gh_boot_work *work,const uint8_t *actor)
{
    return work->room==0x101&&word(actor+0x564)==0x227&&half(actor+0x640)==9&&actor[0x1650]==4;
}

static int r101_gate_exchanged_enemy(const gh_boot_work *work,const uint8_t *actor)
{
    const unsigned stamp=half(actor+0x640);
    return work->room==0x101&&word(actor+0x564)==0x227&&actor[0x1650]==4&&
        (stamp==10||stamp==11);
}

static int r105_exchanged_enemy(const gh_boot_work *work,const uint8_t *actor)
{
    return work->room==0x105&&word(actor+0x564)==0x227&&half(actor+0x640)==2&&actor[0x1650]==0;
}

static int r105_opening_enemy(const gh_boot_work *work,const uint8_t *actor)
{
    if(work->room!=0x105||actor[0x1650])return 0;
    const uint32_t dependency=word(actor+0x564);
    const unsigned stamp=half(actor+0x640);
    return (dependency==0x200&&stamp==0)||(dependency==0x203&&stamp==1)||
        (dependency==0x227&&stamp==2);
}

static int r105_battle_enemy(const gh_boot_work *work,const uint8_t *actor)
{
    if(work->room!=0x105||actor[0x1650])return 0;
    const uint32_t dependency=word(actor+0x564);
    const unsigned stamp=half(actor+0x640);
    return (stamp==7&&dependency==0x200)||(stamp==8&&dependency==0x202);
}

static int r102_gate_actor(const gh_boot_work *work,const uint8_t *actor)
{
    const uint32_t dependency=word(actor+0x564);
    const uint16_t stamp=half(actor+0x640);
    return work->room==0x102&&!actor[0x1650]&&
        ((stamp==0x19&&dependency==0x202)||(stamp==0x1a&&dependency==0x203));
}

static int r102_gate_reinforcement(const gh_boot_work *work,const uint8_t *actor)
{
    return word(actor+0x16c4)==2&&r102_gate_actor(work,actor);
}

static int r102_em00_stamp11(const gh_boot_work *work,const uint8_t *p)
{
    return work->room==0x102&&word(p+0x564)==0x200&&half(p+0x640)==11&&p[0x1650]==5;
}

static int r102_em00_stamp3(const gh_boot_work *work,const uint8_t *p)
{
    return work->room==0x102&&word(p+0x564)==0x200&&half(p+0x640)==3&&p[0x1650]==1;
}

static int r102_em00_stamp7_grab(const gh_boot_work *work,const uint8_t *p)
{
    return work->room==0x102&&word(p+0x564)==0x200&&half(p+0x640)==7&&!p[0x1650]&&
        word(p+0x16c4)==2;
}

static int r102_grab_escape_counter(const gh_boot_work *work,const uint8_t *p)
{
    return (r102_em00_stamp11(work,p)||r102_em00_stamp3(work,p)||r102_em00_stamp7_grab(work,p))&&p[0x2f7]==1;
}

static int r102_gate_em0a(const gh_boot_work *work,const uint8_t *actor)
{
    return work->room==0x102&&word(actor+0x564)==0x20a&&half(actor+0x640)==2&&
        actor[0x1650]==5&&!word(actor+0x16c4);
}

static int r102_em03_stamp8(const gh_boot_work *work,const uint8_t *actor)
{
    return work->room==0x102&&word(actor+0x564)==0x203&&half(actor+0x640)==8&&!actor[0x1650];
}

static int r102_east_em27(const gh_boot_work *work,const uint8_t *actor)
{
    return work->room==0x102&&word(actor+0x564)==0x227&&half(actor+0x640)==4&&!actor[0x1650];
}

static int r102_at0b_em02(const gh_boot_work *work,const uint8_t *actor)
{
    return work->room==0x102&&word(actor+0x564)==0x202&&half(actor+0x640)==0x17&&
        !actor[0x1650];
}

static int r102_area11_em02(const gh_boot_work *work,const uint8_t *actor)
{
    return r102_at0b_em02(work,actor)&&word(actor+0x16c4)==2;
}

static int r105_gate_approach(const gh_boot_work *work,const uint8_t *actor)
{
    return r105_battle_enemy(work,actor)&&word(actor+0x16c4)==2;
}

static gh_actor_motion_status rank(gh_enemy202_move *s,uint8_t *p,
    const float player[3],const float self[3])
{
    const gh_game_init_state *manager=s->runtime.entry->b.init;
    unsigned index=64;
    for(unsigned i=0;i<64;i++)if(manager->enemy_top==manager->enemies+i*20){index=i;break;}
    if(index==64)return stop(s,0x2917f8,GH_ACTOR_MOTION_BUSY);
    fenv_t env;
    if(!set_round(&env,FE_TONEAREST))return stop(s,0x2917f8,GH_ACTOR_MOTION_CALLEE);
    float distance=distance_xz(player,self),height=ordered_abs(sub(player[1],self[1]));
    unsigned result=0;
    uint64_t visited=0;
    int valid=isfinite(distance)&&isfinite(height);
    while(valid&&index!=255){
        if(index>=64||(visited&(UINT64_C(1)<<index))){valid=0;break;}
        visited|=UINT64_C(1)<<index;
        const gh_game_title_player *candidate=manager->enemy_actors[index];
        index=manager->enemy_next[index];
        if(!candidate||candidate==&s->runtime.entry->object)continue;
        const gh_title_player_view *view=&candidate->view;
        if(!view->bytes||view->size<=0x617){valid=0;break;}
        unsigned kind=half(view->bytes+0x2fe);
        if(kind<0x200||kind>=0x280||view->bytes[0x617])continue;
        float xyz[3];
        if(!position(view,xyz)){valid=0;break;}
        float d=distance_xz(player,xyz),h=ordered_abs(sub(player[1],xyz[1]));
        if(!isfinite(d)||!isfinite(h)){valid=0;break;}
        if(d<distance ? !(height<2&&h>2) : height>2&&h<2)result++;
    }
    if(fesetenv(&env)||!valid)return stop(s,0x2917f8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(manager->enemies[0x518])result+=3;
    put(p+0x16d8,result);
    return GH_ACTOR_MOTION_OK;
}

static unsigned heat_category(const gh_enemy202_move *s)
{
    const gh_core_save_view save=s->runtime.entry->b.save;
    const uint8_t *table=elf_span(s->elf,s->elf_bytes,0x3bf160,20);
    if(!table||!save.bytes||save.size<0x1e)return 0;
    unsigned raw=half(save.bytes+0x1c);
    int heat=raw<32768?(int)raw:(int)raw-65536;
    unsigned bracket=0;
    while(bracket<5&&(int32_t)word(table+bracket*4)<=heat)bracket++;
    return bracket<3?bracket+1:5;
}

static gh_actor_motion_status state0_schedule(gh_enemy202_move *s,uint8_t *p)
{
    unsigned category=heat_category(s);
    if(!category)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
    float timer=number(word(p+0x1760)),accumulated=number(word(p+0x175c));
    if(!isfinite(timer)||!isfinite(accumulated))
        return stop(s,0x20f51c,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t current_rank=word(p+0x16d8);
    if((category==1||category==2)&&timer>0)put(p+0x16d8,0);
    else if((category==1||category==2)&&current_rank==category){
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x20f51c,GH_ACTOR_MOTION_CALLEE);
        float next=add(accumulated,number(word(p+0x5a8)));
        if(fesetenv(&env)||!isfinite(next))
            return stop(s,0x20f51c,GH_ACTOR_MOTION_UNSUPPORTED);
        if(next>150){put(p+0x175c,0);put(p+0x1760,0x42b40000);}
        else put(p+0x175c,bits(next));
    }else put(p+0x175c,0);

    uint32_t flags=word(p+0x16d4);
    if(word(p+0x16d0)&0x8000u){
        put(p+0x16d0,word(p+0x16d0)&~0x200000u);
        const uint8_t *target=number(word(p+0x16c0))>0?p+0x16b0:s->player->alias_f0;
        memmove(p+0x16a0,target,12);
        s->pending_ai_pc=0x20fa64;
    }else{
        uint32_t frame=word((const uint8_t*)s->runtime.entry->b.work+0x210);
        put(p+0x16d4,flags&~1u);
        s->pending_ai_pc=((word(p+0x17d0)^frame)&7u)==0||(flags&1u)?0x26f328:0x20fa64;
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status target_line_of_sight(gh_enemy202_move *s,uint8_t *p,
    const float actor[3],const float player[3])
{
    gh_scene_ray_request ray={.mask=1,.flags=0x10,.segmented=1};
    memcpy(ray.from,actor,12);memcpy(ray.to,player,12);
    ray.from[3]=ray.to[3]=1;
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x26f3ac,GH_ACTOR_MOTION_CALLEE);
    ray.from[1]=add(ray.from[1],1);ray.to[1]=add(ray.to[1],1);
    if(fesetenv(&env)||!isfinite(ray.from[1])||!isfinite(ray.to[1]))
        return stop(s,0x26f3ac,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->runtime.bindings->actors)return stop(s,0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    int hit;
    uint32_t pc;
    gh_scene_ray_status status=gh_scene_ray_query_00131e10(s->runtime.bindings->actors,&ray,&hit,&pc);
    if(status)return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t flags=word(p+0x16d0);
    put(p+0x16d0,hit?flags&~0x200000u:flags|0x200000u);
    s->visibility_hit=(uint8_t)hit;
    s->visibility_done=1;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status choose_path_target(gh_enemy202_move *s,uint8_t *p,
    const float actor[3],const float player[3])
{
    if(number(word(p+0x16c0))>0){
        memcpy(s->path_target,p+0x16b0,12);
        return GH_ACTOR_MOTION_OK;
    }
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x20f680,GH_ACTOR_MOTION_CALLEE);
    float height=ordered_abs(sub(player[1],actor[1]));
    if(fesetenv(&env)||!isfinite(height))
        return stop(s,0x20f680,GH_ACTOR_MOTION_UNSUPPORTED);
    if(number(word(p+0x618))<9&&height<1.5f){
        memcpy(s->path_target,player,12);
        return GH_ACTOR_MOTION_OK;
    }

    unsigned category=heat_category(s);
    if(!category)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
    unsigned divisor=category==5?7:category>=3?5:3;
    uint32_t serial=word(p+0x17d0);
    /* Original DIV signed. Native initialized serials are nonnegative here. */
    if(serial>INT32_MAX)return stop(s,0x20f730,GH_ACTOR_MOTION_UNSUPPORTED);
    unsigned variation=serial%divisor;
    static const float offsets[7][2]={{0,0},{1,1},{-1,1},{1.5f,1.5f},
        {-1.5f,1.5f},{2,2},{-2,2}};
    float matrix[16];
    memcpy(matrix,s->player->bytes+0x80,64);
    for(unsigned i=0;i<16;i++)if(!isfinite(matrix[i]))
        return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!set_round(&env,FE_TONEAREST))return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
    float target[3];
    for(unsigned i=0;i<3;i++)
        target[i]=add(add(add(mul(matrix[i],offsets[variation][0]),mul(matrix[4+i],0)),
            mul(matrix[8+i],offsets[variation][1])),matrix[12+i]);
    if(fesetenv(&env))return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
    for(unsigned i=0;i<3;i++)if(!isfinite(target[i]))
        return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);

    float intersection[4]={0};
    gh_scene_ray_request ray={.mask=7,.segmented=1,.hit_position=intersection};
    memcpy(ray.from,player,12);memcpy(ray.to,target,12);
    ray.from[3]=ray.to[3]=1;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x20f8a8,GH_ACTOR_MOTION_CALLEE);
    ray.from[1]=add(ray.from[1],0.5f);ray.to[1]=add(ray.to[1],0.5f);
    if(fesetenv(&env)||!isfinite(ray.from[1])||!isfinite(ray.to[1]))
        return stop(s,0x20f8a8,GH_ACTOR_MOTION_UNSUPPORTED);
    int hit;
    uint32_t pc;
    gh_scene_ray_status status=gh_scene_ray_query_00131e10(s->runtime.bindings->actors,&ray,&hit,&pc);
    if(status)return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    if(hit){
        /* 20F8F0: stop 0.3 units before the hit, then remove the ray's
         * half-unit height lift. The VU vector reference stays binary32. */
        if(!set_round(&env,FE_TONEAREST))return stop(s,0x101e28,GH_ACTOR_MOTION_CALLEE);
        float direction[3],backoff[3];
        for(unsigned i=0;i<3;i++)direction[i]=sub(ray.from[i],intersection[i]);
        const float size=sqrtf(add(add(mul(direction[0],direction[0]),
            mul(direction[1],direction[1])),mul(direction[2],direction[2])));
        if(fesetround(FE_TOWARDZERO)){
            (void)fesetenv(&env);
            return stop(s,0x20f968,GH_ACTOR_MOTION_CALLEE);
        }
        volatile float inverse=size>0?1.0f/size:0;
        for(unsigned i=0;i<3;i++)backoff[i]=mul(direction[i],inverse);
        if(fesetround(FE_TONEAREST)){
            (void)fesetenv(&env);
            return stop(s,0x20f9d0,GH_ACTOR_MOTION_CALLEE);
        }
        for(unsigned i=0;i<3;i++)
            target[i]=add(intersection[i],mul(backoff[i],number(0x3e99999a)));
        if(fesetround(FE_TOWARDZERO)){
            (void)fesetenv(&env);
            return stop(s,0x20fa18,GH_ACTOR_MOTION_CALLEE);
        }
        target[1]=sub(target[1],0.5f);
        if(fesetenv(&env)||!isfinite(size)||
           !isfinite(target[0])||!isfinite(target[1])||!isfinite(target[2]))
            return stop(s,0x101e28,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    memcpy(s->path_target,target,12);
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy204_move_bind_springs(gh_enemy202_move *move,
    const gh_cloth_step_bindings *bindings,gh_player_shared_vector *pivot)
{
    if(!move||!bindings||!pivot)return GH_ACTOR_MOTION_ARGUMENT;
    if(move->spring_pivot&&move->spring_pivot!=pivot)return GH_ACTOR_MOTION_BUSY;
    move->spring_bindings=*bindings;
    move->spring_pivot=pivot;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_move_bind_navigation(gh_enemy202_move *s,
    const gh_scene_nearcheck *navigation)
{
    if(!s||!navigation)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->navigation&&s->navigation!=navigation)return GH_ACTOR_MOTION_BUSY;
    s->navigation=navigation;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status route_clear_002a74e0(gh_enemy202_move *s,uint8_t *p,
    const float actor[3])
{
    const gh_enemy_entry_bindings *b=&s->runtime.entry->b;
    const gh_actor_motion_status status=gh_enemy_route_clear_002a74e0(&s->route,p,
        actor,s->path_target,s->navigation,s->runtime.bindings->actors,b->init,b->work->room);
    if(status)return stop(s,s->route.frontier,status);
    if(gh_enemy_route_waypoint_002a7ca0(p,p+0x16a0))
        put(p+0x16d4,word(p+0x16d4)|0x08000000u);
    s->pending_ai_pc=0x20fa64;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_move_bind_opacity(gh_enemy202_move *s,
    const gh_object_opacity_environment *opacity)
{
    if(!s||!opacity)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->opacity&&s->opacity!=opacity)return GH_ACTOR_MOTION_BUSY;
    s->opacity=opacity;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_move_bind_targets(gh_enemy202_move *s,
    const gh_player_target_bindings *targets)
{
    if(!s||!targets||!targets->pools||!targets->actors||!targets->player||!targets->resolve)
        return GH_ACTOR_MOTION_ARGUMENT;
    const gh_player_target_bindings *old=&s->targets;
    if(old->pools&&(old->pools!=targets->pools||old->actors!=targets->actors||
        old->player!=targets->player||old->resolve!=targets->resolve||old->context!=targets->context))
        return GH_ACTOR_MOTION_BUSY;
    s->targets=*targets;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status camera_update_gate(gh_enemy202_move *s,const uint8_t *p,int *update)
{
    const gh_object_opacity_environment *env=s->opacity;
    if(!env||!env->camera||!env->camera->constructed||
        env->init!=s->runtime.entry->b.init||env->init->active_camera!=env->camera->identity)
        return stop(s,0x26ef60,GH_ACTOR_MOTION_UNSUPPORTED);
    float target[3],eye[3],self[3],previous[3];
    memcpy(target,env->camera->fields+0x200,12);
    memcpy(eye,env->camera->fields+0x210,12);
    memcpy(previous,p+0x490,12);
    if(!position(&s->runtime.entry->object.view,self))
        return stop(s,0x26ef60,GH_ACTOR_MOTION_UNSUPPORTED);
    fenv_t fp;
    if(!set_round(&fp,FE_TONEAREST))return stop(s,0x26ef60,GH_ACTOR_MOTION_CALLEE);
    float dx=sub(target[0],eye[0]),dz=sub(target[2],eye[2]);
    float distance=distance_xz(self,eye),prior_distance=distance_xz(self,previous);
    if(fesetenv(&fp)||!isfinite(dx)||!isfinite(dz)||!isfinite(distance)||!isfinite(prior_distance))
        return stop(s,0x26ef60,GH_ACTOR_MOTION_UNSUPPORTED);
    float heading;
    uint32_t pc;
    if(gh_game_angle_001f8180(dx,dz,&heading,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
        return stop(s,pc?pc:0x1f8180,GH_ACTOR_MOTION_UNSUPPORTED);
    *update=1;
    if(p[0x2f4]==1||p[0x2f4]==2||distance<100)return GH_ACTOR_MOTION_OK;
    float angle;
    if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,eye,self,heading,
        number(0x40490fdb),&angle,&pc))
        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    *update=ordered_abs(angle)<number(0x3fc90fdb)||prior_distance>0.0009f;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status adjust_ground_pose_00275668(gh_enemy202_move *s,uint8_t *p)
{
    const gh_actor_part *root_part;
    if(!p[0x2b4])return GH_ACTOR_MOTION_OK;
    if(gh_actor_model_part_at(&s->runtime.entry->initialize.model.model,0,&root_part)||
       !root_part->alias_d0)return stop(s,0x2756f8,GH_ACTOR_MOTION_UNSUPPORTED);
    float normal[3],scale=number(word(p+0x114)),rate=number(word(p+0x5a8));
    float previous=number(word(p+0x1758)),part_y=number(word(root_part->alias_d0+4));
    memcpy(normal,p+0x480,12);
    if(!isfinite(scale)||!isfinite(rate)||!isfinite(previous)||!isfinite(part_y)||
       !isfinite(normal[0])||!isfinite(normal[1])||!isfinite(normal[2]))
        return stop(s,0x27571c,GH_ACTOR_MOTION_UNSUPPORTED);
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x27571c,GH_ACTOR_MOTION_CALLEE);
    float square=add(add(mul(normal[0],normal[0]),mul(normal[1],normal[1])),mul(normal[2],normal[2]));
    float length=(float)sqrt((double)square);
    if(length>0){
        volatile float inverse=1.0f/length;
        for(unsigned i=0;i<3;i++)normal[i]=mul(normal[i],inverse);
    }else memset(normal,0,sizeof(normal));
    float goal=0;
    if(normal[1]!=0&&positive_hp(p)&&!(half(p+0x3ac)&0x42cu)&&p[0x2f4]!=2){
        goal=mul(sub(1,normal[1]),mul(0.5f,scale));
        float cap=mul(0.4f,scale);
        if(goal>cap)goal=cap;
    }
    float history=sub(1,mul(rate,0.15f));
    float current=add(mul(previous,history),mul(goal,sub(1,history)));
    if(current<0)current=0;
    float next_y=sub(part_y,current);
    if(fesetenv(&env)||!isfinite(length)||!isfinite(current)||!isfinite(next_y))
        return stop(s,0x275838,GH_ACTOR_MOTION_UNSUPPORTED);
    memcpy(p+0x480,normal,12);
    put(p+0x1758,bits(current));
    put(root_part->alias_d0+4,bits(next_y));
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status hide_cached_node(gh_enemy202_move *s,
    const gh_player_mesh_reference *node,int hidden,uint32_t pc)
{
    if(!node->bytes)return GH_ACTOR_MOTION_OK;
    gh_actor_model *model=&s->runtime.entry->initialize.model.model;
    const uint8_t *bytes;
    size_t mesh;
    if(node->owner!=model||gh_actor_model_node_at(model,(unsigned)node->mesh_index,&bytes,&mesh)||mesh!=node->mesh_index||
        bytes!=node->bytes||gh_actor_model_node_mask_flags(model,(unsigned)node->mesh_index,
        0x380,~1u,hidden?1u:0u))return stop(s,pc,GH_ACTOR_MOTION_UNSUPPORTED);
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status hide_named_node(gh_enemy202_move *s,uint32_t address)
{
    const uint8_t *name=elf_span(s->elf,s->elf_bytes,address,8);
    if(!name)return stop(s,0x14b730,GH_ACTOR_MOTION_BOUNDS);
    gh_player_mesh_reference node;
    int exists;
    if(gh_actor_model_find_mesh_0014b730(&s->runtime.entry->initialize.model.model,
        name,8,&node,&exists))return stop(s,0x14b730,GH_ACTOR_MOTION_UNSUPPORTED);
    return exists?hide_cached_node(s,&node,0,0x268dc8):GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status turn_head_00269b50(gh_enemy202_move *s,uint8_t *p)
{
    gh_actor_model *model=&s->runtime.entry->initialize.model.model;
    const unsigned indices[]={3,4,26};
    const gh_actor_part *parts[3]={NULL,NULL,NULL};
    for(unsigned i=0;i<3;i++)
        if(p[0x2b4]>indices[i]&&gh_actor_model_part_at(model,indices[i],&parts[i]))
            return stop(s,0x269b50,GH_ACTOR_MOTION_UNSUPPORTED);
    float wanted=0,old=number(word(p+0x16f4)),rate=number(word(p+0x5a8));
    if(!isfinite(old)||!isfinite(rate)||rate<0)return stop(s,0x2da1f8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(word(p+0x16d0)&0x400u){
        float actor[3],player[3];
        uint32_t pc;
        if(!position(&s->runtime.entry->object.view,actor)||!position(s->player,player)||
            gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,player,
                number(word(p+0x104)),number(0x3f860a92),&wanted,&pc))
            return stop(s,0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    fenv_t fp;
    if(!set_round(&fp,FE_TOWARDZERO))return stop(s,0x2da1f8,GH_ACTOR_MOTION_CALLEE);
    float delta=sub(wanted,old),step=mul(0.19634955f,rate);
    const float pi=number(0x40490fdb),tau=number(0x40c90fdb);
    if(delta>pi)delta=sub(delta,tau);
    if(delta<-pi)delta=add(delta,tau);
    if(delta>step)delta=step;
    if(delta<-step)delta=-step;
    float angle=add(old,delta);
    if(fesetenv(&fp)||gh_id_angle_reference_001f7d48(angle,&angle))
        return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
    if(word(p+0x16d4)&0x80000000u)angle=0;
    put(p+0x16f4,bits(angle));
    for(unsigned i=0;i<3;i++)if(parts[i]){
        put(parts[i]->bytes+0x150,i==2?0:bits(angle*0.5f));
        put(parts[i]->bytes+0x154,word(parts[i]->bytes+0x154)|0x800u);
    }
    return GH_ACTOR_MOTION_OK;
}

/* 292F70 updates one of the EnemyM directional threat timers. */
static gh_actor_motion_status attack_direction_00292f70(gh_enemy202_move *s)
{
    float player[3],actor[3],angle;
    uint32_t pc=0;
    if(!position(s->player,player)||!position(&s->runtime.entry->object.view,actor))
        return stop(s,0x292f70,GH_ACTOR_MOTION_BOUNDS);
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
    const float distance=distance_xz(player,actor);
    if(fesetenv(&env)||!isfinite(distance))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
    if(distance<number(0x404f5c28)||distance>25)return GH_ACTOR_MOTION_OK;
    if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,player,actor,
        number(word(s->player->bytes+0x104)),number(0x40490fdb),&angle,&pc))
        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    const float absolute=ordered_abs(angle);
    if(absolute>=number(0x3f490fdb)){
        const size_t offset=absolute>number(0x4016cbe4)?0x550:angle>0?0x554:0x558;
        put(s->runtime.entry->b.init->enemies+offset,2);
    }
    return GH_ACTOR_MOTION_OK;
}

static int r103_arena_em0a(const gh_enemy_entry *enemy);

static gh_actor_motion_status em0a_stun_offer_002dcd50(gh_enemy202_move *s,uint8_t *p)
{
    const gh_game_init_state *init=s->runtime.entry->b.init;
    const gh_title_player_view *actor=&s->runtime.entry->object.view;
    const gh_title_player_view *player=s->player;
    if(!player||!player->bytes||player->size<0x15f8)
        return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(init->enemies[0x53c]||(int16_t)half(player->bytes+0x54a)<=0||
       (word(player->bytes+0x15f4)&0x400000u)||(int32_t)word(init->enemies+0x514)>0)
        return GH_ACTOR_MOTION_OK;
    float from[3],to[3],angle;
    uint32_t pc=0;
    if(!position(player,from)||!position(actor,to))
        return stop(s,0x2dcd50,GH_ACTOR_MOTION_UNSUPPORTED);
    /* 1F7EB0 measures the horizontal distance and uses a separate height test. */
    const float dx=sub(to[0],from[0]),dz=sub(to[2],from[2]);
    const float distance=add(mul(dx,dx),mul(dz,dz));
    if(distance>(p[0x620]?9.0f:4.0f)||fabsf(sub(to[1],from[1]))>0.5f)
        return GH_ACTOR_MOTION_OK;
    if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,from,to,
       number(word(player->bytes+0x104)),3.1415927f,&angle,&pc))
        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    if(fabsf(angle)>0.7853982f)return GH_ACTOR_MOTION_OK;
    gh_scene_ray_request ray={.mask=7,.segmented=1};
    memcpy(ray.from,from,sizeof(from));memcpy(ray.to,to,sizeof(to));
    ray.from[1]=add(from[1],0.5f);ray.to[1]=add(to[1],0.5f);
    ray.from[3]=ray.to[3]=1;
    int hit;
    if(gh_scene_ray_query_00131e10(s->runtime.bindings->actors,&ray,&hit,&pc))
        return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    if(hit)return GH_ACTOR_MOTION_OK;
    if(!s->grab_buttons)return stop(s,0x1f7858,GH_ACTOR_MOTION_UNSUPPORTED);
    /* The original EM0A offer uses action text 10, life 10h and 2DD480. */
    const gh_action_set_request request={.priority=10,.life=0x10,.callback_id=0x2dd480,
        .argument=p};
    int emitted;unsigned slot;
    if(gh_action_button_set_001f7858(s->grab_buttons,&request,&emitted,&slot,&pc))
        return stop(s,pc?pc:0x1f7858,GH_ACTOR_MOTION_UNSUPPORTED);
    p[0x620]=1;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_terrain_tail(gh_enemy202_move *s,uint8_t *p)
{
    const gh_enemy_move_actor *runtime=&s->runtime;
    const gh_title_player_view *actor=&runtime->entry->object.view;
    if(s->substate67_stage==24){
        gh_actor_motion_status status=turn_head_00269b50(s,p);
        if(status)return status;
        /* 26A018/274D18 and 273748 do not mutate these dependencies. */
        if(!position(actor,s->terrain_start))return stop(s,0x209c30,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x474,0);put(p+0x480,0);put(p+0x484,0x3f800000);put(p+0x488,0);
        s->substate67_stage=25;
    }
    uint32_t pc=0;
    int hit;
    if(s->substate67_stage==25){
        if(gh_collision_solid_step_resume_001348e8(runtime->bindings->solids,actor,
            GH_STATIC_BINARY32_REFERENCE,&s->post_solid,&hit,&pc))
            return stop(s,pc?pc:0x1348e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=26;
    }
    gh_player_terrain_bindings binding={runtime->bindings->actors,actor,gh_terrain_sphere_query_001321d8,&s->sphere_call};
    if(s->substate67_stage==26){
        if(!(word(p+0x16d0)&0x20000u)){
            s->sphere_call.work=runtime->entry->b.work;
            if(gh_player_terrain_wall_00132da0(&s->wall_call,&binding,number(word(p+0x16cc)),
                (half(p+0x3ac)&4u)?0x124u:0u,s->terrain_point,NULL,0,NULL,&hit,&pc))
                return stop(s,pc?pc:0x132da0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit){
                put(actor->alias_f0,bits(s->terrain_point[0]));
                put(actor->alias_f0+8,bits(s->terrain_point[2]));
            }
        }
        s->substate67_stage=27;
    }
    if(s->substate67_stage==27){
        if(word(p+0x16d0)&0x10000u)s->substate67_stage=29;
        else{
            if(gh_player_terrain_floor_001329c0(&s->floor_call,&binding,10,s->terrain_point,
                s->terrain_normal,&s->floor_report,&hit,&pc))
                return stop(s,pc?pc:0x1329c0,GH_ACTOR_MOTION_UNSUPPORTED);
            s->floor_hit=(uint8_t)hit;
            s->substate67_stage=28;
        }
    }
    if(s->substate67_stage==28){
        uint32_t attribute=0;
        if(s->floor_hit){
            if(!s->floor_report.attributes||s->floor_report.attribute_bytes<2)
                return stop(s,0x209cf0,GH_ACTOR_MOTION_UNSUPPORTED);
            attribute=half(s->floor_report.attributes);
        }
        float current=number(word(actor->alias_f0+4));
        float velocity=number(word(p+0x1710)),acceleration=number(word(p+0x170c));
        float rate=number(word(p+0x5a8));
        if(!isfinite(current)||!isfinite(velocity)||!isfinite(acceleration)||!isfinite(rate))
            return stop(s,0x209d00,GH_ACTOR_MOTION_UNSUPPORTED);
        fenv_t fp;
        if(!set_round(&fp,FE_TOWARDZERO))return stop(s,0x209d00,GH_ACTOR_MOTION_CALLEE);
        float limit=s->floor_hit?s->terrain_point[1]:sub(current,10);
        float next_velocity=sub(velocity,mul(acceleration,rate));
        if(next_velocity<-1)next_velocity=-1;
        float next_height=add(current,mul(next_velocity,rate));
        if(fesetenv(&fp)||!isfinite(next_height)||!isfinite(limit))
            return stop(s,0x209d00,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t flags=word(p+0x16d0)&~0x4000000u;
        if(next_height<limit){
            next_height=limit;next_velocity=0;flags|=0x4000000u;
        }
        if(s->floor_hit)memcpy(p+0x480,s->terrain_normal,12);
        put(p+0x474,attribute);
        put(p+0x1710,bits(next_velocity));put(actor->alias_f0+4,bits(next_height));put(p+0x16d0,flags);
        s->substate67_stage=29;
    }
    if(s->substate67_stage==29){
        float now[3],delta[3];
        if(!position(actor,now))return stop(s,0x209db4,GH_ACTOR_MOTION_UNSUPPORTED);
        fenv_t fp;
        if(!set_round(&fp,FE_TONEAREST))return stop(s,0x209db4,GH_ACTOR_MOTION_CALLEE);
        for(unsigned i=0;i<3;i++)delta[i]=sub(now[i],s->terrain_start[i]);
        if(fesetenv(&fp))return stop(s,0x209db4,GH_ACTOR_MOTION_CALLEE);
        memcpy(p+0x550,delta,12);
        s->substate67_stage=30;
    }
    if(s->substate67_stage==30){
        float y=number(word(actor->alias_f0+4)),old_y=number(word(p+0x494));
        if(!p[0x17c2]&&y<old_y&&((half(p+0x3ac)&4u)||number(word(p+0x1710))<0)){
            gh_scene_ray_report report={0};
            float point[3];
            gh_scene_ray_request ray={.mask=7,.segmented=1,.native_reports=&report,.report_limit=1,.hit_position=point};
            memcpy(ray.from,p+0x490,16);memcpy(ray.to,actor->alias_f0,16);
            if(gh_scene_ray_query_00131e10(runtime->bindings->actors,&ray,&hit,&pc))
                return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit){
                if(!report.attributes||report.attribute_bytes<8)
                    return stop(s,0x2752a0,GH_ACTOR_MOTION_UNSUPPORTED);
                s->water_effect=!!(word(report.attributes+4)&0x400u);
            }
        }
        s->substate67_stage=31;
    }
    if(s->water_effect)return stop(s,0x306838,GH_ACTOR_MOTION_UNSUPPORTED);
    if(s->substate67_stage==31){
        memcpy(p+0x490,actor->alias_f0,12);
        if(p[0x2f4]!=2){
            float own=number(word(actor->alias_f0+4)),other=number(word(s->player->alias_f0+4));
            fenv_t fp;
            if(!set_round(&fp,FE_TOWARDZERO))return stop(s,0x209e10,GH_ACTOR_MOTION_CALLEE);
            float limit=sub(other,50);
            if(fesetenv(&fp))return stop(s,0x209e10,GH_ACTOR_MOTION_CALLEE);
            if(own<limit){
                p[0x2f4]=2;p[0x2f5]=2;p[0x2f6]=0;p[0x2f7]=1;put_half(p+0x54a,0);
            }
        }
        s->substate67_stage=32;
    }
    uint32_t flags=word(p+0x16d0);
    if(half(p+0x3ac)&0x800u){
        gh_actor_motion_status status=attack_direction_00292f70(s);
        if(status)return status;
    }
    if(flags&2u)runtime->entry->b.init->enemies[0x53a]=2;
    if((flags&0x40000u)||(number(word(p+0x17cc))>0&&positive_hp(p)&&!(half(p+0x3ac)&0xcu))){
        if(r103_arena_em0a(runtime->entry)){
            const gh_actor_motion_status status=em0a_stun_offer_002dcd50(s,p);
            if(status)return status;
        }else{
            const gh_boot_work *work=runtime->entry->b.work;
            if(work->room!=0x102||word(p+0x564)!=0x200||half(p+0x640)!=0x10||p[0x1650]||
               (flags&0x40000u)||!s->player||s->player->size<0x15f8)
                return stop(s,0x2dcd50,GH_ACTOR_MOTION_UNSUPPORTED);
            /* 2DCD50 determines whether the original contextual finish is available. */
            const uint8_t *player=s->player->bytes,*manager=runtime->entry->b.init->enemies;
            if(!manager[0x53c]&&(int16_t)half(player+0x54a)>0&&
               !(word(player+0x15f4)&0x400000u)&&(int32_t)word(manager+0x514)<=0){
                gh_scene_ray_request ray={.mask=7,.segmented=1};
                float angle;
                uint32_t pc=0;
                if(!position(s->player,ray.from)||!position(actor,ray.to))
                    return stop(s,0x275978,GH_ACTOR_MOTION_UNSUPPORTED);
                memcpy(p+0x1680,ray.to,3*sizeof(float));
                fenv_t env;
                if(!set_round(&env,FE_TOWARDZERO))
                    return stop(s,0x2dce58,GH_ACTOR_MOTION_CALLEE);
                const float range=distance_xz(ray.from,ray.to);
                const float height=ordered_abs(sub(ray.from[1],ray.to[1]));
                if(fesetenv(&env)||!isfinite(range)||!isfinite(height))
                    return stop(s,0x2dce58,GH_ACTOR_MOTION_UNSUPPORTED);
                if(range<=(p[0x620]?9.0f:4.0f)&&height<=0.5f){
                    if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,ray.from,ray.to,
                        number(word(player+0x104)),number(0x40490fdb),&angle,&pc))
                        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
                    if(ordered_abs(angle)<=number(0x3f490fdb)){
                        if(!set_round(&env,FE_TOWARDZERO))
                            return stop(s,0x2dcfe0,GH_ACTOR_MOTION_CALLEE);
                        ray.from[1]=add(ray.from[1],0.5f);
                        ray.to[1]=add(ray.to[1],0.5f);
                        if(fesetenv(&env))return stop(s,0x2dcfe0,GH_ACTOR_MOTION_CALLEE);
                        int hit;
                        if(gh_scene_ray_query_00131e10(runtime->bindings->actors,&ray,&hit,&pc))
                            return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
                        if(!hit){
                            if(!s->grab_buttons)return stop(s,0x1f7858,GH_ACTOR_MOTION_UNSUPPORTED);
                            const gh_action_set_request offer={.priority=10,.life=13,
                                .callback_id=0x2dd300,.argument=p};
                            int emitted;
                            unsigned index;
                            p[0x620]=1;
                            if(gh_action_button_set_001f7858(s->grab_buttons,&offer,&emitted,&index,&pc))
                                return stop(s,pc?pc:0x1f7858,GH_ACTOR_MOTION_UNSUPPORTED);
                        }
                    }
                }
            }
        }
    }else p[0x620]=0;
    if(word(p+0x16c4)==0x10&&number(word(p+0x16c0))>0)
        return stop(s,0x209f50,GH_ACTOR_MOTION_UNSUPPORTED);
    if((word(p+0x16d4)&0x20000000u)&&positive_hp(p))
        return stop(s,0x209fa0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(flags&0x1000000u)return stop(s,0x209fe4,GH_ACTOR_MOTION_UNSUPPORTED);
    s->stage=RETURNED;
    s->original_returned=1;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status boss_spring_frame_002714e8(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    const int r104=gh_enemy203_is_r104_entry(&enemy->entry.request,enemy->b.entry.work->room);
    const gh_event_spring_profile *profile=&enemy->tail.spring_profile;
    gh_actor_model *model=&enemy->entry.initialize.model.model;
    if(!enemy->tail.spring_records.pointer||!enemy->tail.springs_ready||
        !profile->loaded||profile->original_scratch!=(r104?0x61a9a0:0x61aa00)||!s->spring_pivot)
        return stop(s,0x2714e8,GH_ACTOR_MOTION_UNSUPPORTED);
    /* Original 2DF950 writes these four 61AA00 sphere records. */
    static const gh_cloth_sphere spheres[]={
        {0,18,18,1.0f,0.11f,0,{0,0,0,1},{0,0,0,1}},
        {0,18,19,0.33f,0.11f,0,{0,0,0,1},{0,0,0,1}},
        {0,18,19,0.66f,0.11f,0,{0,0,0,1},{0,0,0,1}},
        {0,19,19,1.0f,0.11f,0,{0,0,0,1},{0,0,0,1}},
    };
    const size_t sphere_count=r104?2:sizeof(spheres)/sizeof(spheres[0]);
    if(word(p+0x17e0)!=profile->count||word(p+0x1818)!=sphere_count)
        return stop(s,0x2df860,GH_ACTOR_MOTION_UNSUPPORTED);
    const gh_cloth_operands operands={
        .model=model,.parent=model,.config=p+0x17e0,.records=enemy->tail.spring_records.pointer,
        .indices=profile->indices,.coefficients={profile->coefficients[0],profile->coefficients[1],profile->coefficients[2]},
        .spheres=r104?gh_cloth_250_spheres:spheres,.sphere_count=sphere_count,.count=profile->count,
        .tables=(1u<<0)|(1u<<5)|(1u<<6)|(1u<<7)|(1u<<8)|(1u<<9),
    };
    uint32_t frontier;
    const gh_cloth_status status=gh_cloth_frame_00104010(&s->spring_frame,&operands,
        &s->spring_bindings,s->spring_pivot,&frontier);
    return status?stop(s,frontier?frontier:0x2df860,GH_ACTOR_MOTION_UNSUPPORTED):GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_post_pose(gh_enemy202_move *s,uint8_t *p)
{
    const gh_enemy_move_actor *actor=&s->runtime;
    if(s->substate67_stage==17){
        /* 3130E0 zeroes the actor. The verified dependency paths, including
         * 227's table entry at 44621C, return from 271410 without using 1860. */
        uint32_t dependency=word(p+0x564);
        if(s->enemy&&(((dependency==0x214||dependency==0x215)&&
            gh_enemy204_is_r103_entry(&actor->entry->request,actor->entry->b.work->room))||
            gh_enemy203_is_r104_entry(&actor->entry->request,actor->entry->b.work->room))){
            const gh_actor_motion_status status=boss_spring_frame_002714e8(s,p);
            if(status)return status;
        }else if(actor->entry->object.view.initial_memory!=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO||
            (dependency>=0x214&&dependency!=0x220&&dependency!=0x227)||
            word(p+0x1860))return stop(s,0x2714e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=18;
    }
    if(s->substate67_stage==18){
        float timer=number(word(p+0x17cc));
        if(!isfinite(timer))return stop(s,0x268508,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t flags=word(p+0x16d0);
        gh_effect_create_call *pending=&s->stun_effect_create;
        if((timer>0)!=!!(flags&0x80u)||(pending->active&&!pending->complete)){
            const gh_boot_work *work=actor->entry->b.work;
            const int arena=r103_arena_em0a(actor->entry);
            if(!arena&&(work->room!=0x102||word(p+0x564)!=0x200||half(p+0x640)!=0x10||p[0x1650]))
                return stop(s,timer>0?0x306900:0x306200,GH_ACTOR_MOTION_UNSUPPORTED);
            gh_effect_create_bindings bindings=s->impact_bindings;
            bindings.model=&actor->entry->initialize.model.model;
            if(!bindings.storage.effects||bindings.work!=work||
               bindings.random!=&actor->entry->b.init->random)
                return stop(s,0x268508,GH_ACTOR_MOTION_ARGUMENT);
            const gh_title_player_view *view=&actor->entry->object.view;
            if(timer>0){
                if(!pending->active||pending->complete){
                    if((arena?gh_effect_create_em0a_stun_part4_00306900(pending,view):
                       gh_effect_create_gate_guard_stun_part4_00306900(pending,view)))
                        return stop(s,0x306900,GH_ACTOR_MOTION_ARGUMENT);
                    put(p+0x16d0,flags|0x80u);
                }
                const gh_effect_status status=gh_game_effect_create_00306528(pending,&bindings);
                if(status||!pending->complete)
                    return stop(s,pending->frontier?pending->frontier:0x306900,GH_ACTOR_MOTION_UNSUPPORTED);
            }else{
                uint32_t pc=0;
                unsigned slot=0;
                if(gh_game_effect_release_00306200(&bindings.storage,view,3,2,&pc,&slot))
                    return stop(s,pc?pc:0x306200,GH_ACTOR_MOTION_UNSUPPORTED);
                put(p+0x16d0,flags&~0x80u);
            }
        }
        s->substate67_stage=19;
    }
    if(s->substate67_stage==19){
        uint32_t flags=word(p+0x16d0);
        int ambient=actor->entry->b.init->enemies[0x518]!=0;
        gh_effect_create_call *pending=&s->ambient_light_effect;
        if(ambient!=!!(flags&0x100u)||(pending->active&&!pending->complete)){
            const unsigned dependency=word(p+0x564);
            const gh_boot_work *work=actor->entry->b.work;
            if(!ambient)return stop(s,0x306200,GH_ACTOR_MOTION_UNSUPPORTED);
            if(work->room!=0x100)return stop(s,0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
            if(dependency==0x220){
                /* Original 2685D0's 0220 table entry only sets the flag. */
                put(p+0x16d0,flags|0x100u);
            }else{
                uint16_t index;
                switch(dependency){
                case 0x200:case 0x202:case 0x203:index=0xb;break;
                case 0x20a:index=0xc;break;
                default:return stop(s,0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
                }
                gh_effect_create_bindings bindings=s->impact_bindings;
                bindings.model=&actor->entry->initialize.model.model;
                if(!bindings.storage.effects||bindings.work!=work||
                   bindings.random!=&actor->entry->b.init->random)
                    return stop(s,0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
                if(!pending->active){
                    uint32_t scale=word(p+0x114);
                    if(!isfinite(number(scale))||number(scale)<=0.0f)
                        return stop(s,0x2685d0,GH_ACTOR_MOTION_UNSUPPORTED);
                    put(p+0x16d0,flags|0x100u);
                    const gh_effect_create_request request={.bank=0x58,.index=index,
                        .actor=&actor->entry->object.view,.mode=4,.selector=-1};
                    if(gh_effect_create_begin(pending,&request))
                        return stop(s,0x306528,GH_ACTOR_MOTION_ARGUMENT);
                    pending->configuration_nested_scale=scale;
                }
                gh_effect_status status=gh_game_effect_create_00306528(pending,&bindings);
                if(status||!pending->complete)
                    return stop(s,pending->frontier?pending->frontier:0x306528,
                       status==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE);
            }
        }
        s->substate67_stage=20;
    }
    if(s->substate67_stage==20){
        if(actor->nodes->stage<2)return stop(s,0x268dc8,GH_ACTOR_MOTION_UNSUPPORTED);
        for(;s->post_node_index<6;s->post_node_index++){
            gh_actor_motion_status status=hide_cached_node(s,&actor->nodes->nodes[s->post_node_index],1,0x268dc8);
            if(status)return status;
        }
        s->substate67_stage=21;
    }
    while(s->substate67_stage==21||s->substate67_stage==22){
        unsigned bank=s->substate67_stage-21;
        uint32_t flags=word(p+0x16d4);
        int suppress=bank?(flags&0x200u)!=0:(flags&0x10180u)!=0;
        if(!suppress){
            unsigned lower=(!!p[0x17c3]^bank)?0x80u:0x40u;
            uint16_t mode=half(p+0x3ac);
            unsigned index=(mode&lower?1u:0u)|(mode&(lower<<8)?2u:0u);
            gh_actor_motion_status status=index==3?hide_named_node(s,bank?0x4452d0:0x4452c8):
                hide_cached_node(s,&actor->nodes->nodes[bank*3+index],0,0x268dc8);
            if(status)return status;
        }
        ++s->substate67_stage;
    }
    if(s->substate67_stage==23){
        if(actor->nodes->nodes[9].bytes){
            int hidden=!(word(p+0x16d0)&0x400000u);
            for(;s->post_node_index<8;s->post_node_index++){
                gh_actor_motion_status status=hide_cached_node(s,&actor->nodes->nodes[s->post_node_index+3],
                    hidden,0x269ac0);
                if(status)return status;
            }
        }
        s->substate67_stage=24;
    }
    return advance_terrain_tail(s,p);
}

static int r102_route_runner(const gh_boot_work *,const uint8_t *);
static int r102_mount_passenger(const gh_enemy_actor *);
static int r101_arena_airborne(const gh_enemy_actor *);

static int r102_area11_obstacle(const gh_boot_work *work,const uint8_t *p)
{
    return work->room==0x102&&word(p+0x564)==0x200&&half(p+0x640)==7&&!p[0x1650];
}

static int r103_arena_em00(const gh_enemy_entry *enemy)
{
 return enemy->request.dependency_kind==0x200&&
    gh_enemy204_is_r103_entry(&enemy->request,enemy->b.work->room);
}

static int r103_arena_em03(const gh_enemy_entry *enemy)
{
 return enemy->request.dependency_kind==0x203&&
    gh_enemy204_is_r103_entry(&enemy->request,enemy->b.work->room);
}

static int r103_arena_em0a(const gh_enemy_entry *enemy)
{
 return enemy->request.dependency_kind==0x20a&&
    gh_enemy204_is_r103_entry(&enemy->request,enemy->b.work->room);
}

static int r103_arena_boss(const gh_enemy_entry *enemy)
{
 return enemy->request.entry_number<2&&
    gh_enemy204_is_r103_entry(&enemy->request,enemy->b.work->room);
}

static int r103_arena_jumper(const gh_enemy_entry *enemy)
{
 return r103_arena_em00(enemy)||r103_arena_em03(enemy);
}

static int r100_at2_formation(const gh_boot_work *work,const uint8_t *p)
{
    const uint32_t dependency=word(p+0x564);
    const unsigned stamp=half(p+0x640);
    return work->room==0x100&&word(p+0x16c4)==6&&p[0x1650]==2&&
        ((dependency==0x200&&stamp==3)||(dependency==0x202&&stamp==4)||(dependency==0x203&&stamp==5));
}

static int r100_at2_enemy(const gh_boot_work *work,const uint8_t *p)
{
    const unsigned stamp=half(p+0x640);
    const uint32_t dependency=word(p+0x564);
    return work->room==0x100&&!p[0x1650]&&
        ((stamp==29&&dependency==0x202)||(stamp==30&&dependency==0x203));
}

static int r100_at3_enemy(const gh_boot_work *work,const uint8_t *p)
{
    const unsigned stamp=half(p+0x640);
    const uint32_t dependency=word(p+0x564);
    return work->room==0x100&&!p[0x1650]&&
        ((stamp==31&&dependency==0x200)||(stamp==32&&dependency==0x202));
}

static int r100_west_em03_patrol(const gh_boot_work *work,const uint8_t *p)
{
    return work->room==0x100&&word(p+0x564)==0x203&&half(p+0x640)==11&&
        p[0x1650]==5&&word(p+0x16c4)==6;
}

static int r101_em00_stamp12(unsigned room,const uint8_t *p)
{
    return room==0x101&&word(p+0x564)==0x200&&half(p+0x640)==12&&p[0x1650]==4;
}

static int r101_em02_stamp5(unsigned room,const uint8_t *p)
{
    return room==0x101&&word(p+0x564)==0x202&&half(p+0x640)==5&&p[0x1650]==4;
}

static int r101_em00_stamp15(unsigned room,const uint8_t *p)
{
    return room==0x101&&word(p+0x564)==0x200&&half(p+0x640)==15&&!p[0x1650];
}

static int r101_em03_stamp0(unsigned room,const uint8_t *p)
{
    return room==0x101&&word(p+0x564)==0x203&&!half(p+0x640)&&p[0x1650]==1;
}

static gh_actor_motion_status advance_sequence_effects_0014fb48(gh_enemy202_move *s,uint8_t *p)
{
    const uint32_t dependency=word(p+0x564);
    const int devil=s->pending_ai_pc==0x212c08&&dependency==0x220;
    const int charge=s->pending_ai_pc==0x224090&&p[0x2f6]==1&&
        gh_enemy203_is_r104_entry(&s->runtime.entry->request,s->runtime.entry->b.work->room);
    const int em03_ranged=s->pending_ai_pc==0x2269f0&&p[0x2f6]==1&&!p[0x2f7]&&
        gh_enemy203_is_r104_entry(&s->runtime.entry->request,s->runtime.entry->b.work->room);
    const int em03_approach=s->pending_ai_pc==0x215d40&&p[0x2f6]==3&&p[0x2f7]<=1&&
        gh_enemy203_is_r104_entry(&s->runtime.entry->request,s->runtime.entry->b.work->room);
    const int r104_melee=((s->pending_ai_pc==0x21a538&&p[0x2f5]==0x15)||
        (s->pending_ai_pc==0x226450&&p[0x2f5]==0x21)||
        (s->pending_ai_pc==0x232500&&p[0x2f5]==0x45))&&p[0x2f6]==1&&!p[0x2f7]&&
        gh_enemy203_is_r104_entry(&s->runtime.entry->request,s->runtime.entry->b.work->room);
    const int em03_combo=s->pending_ai_pc==0x21fd50&&p[0x2f5]==0x16&&p[0x2f6]==1&&!p[0x2f7]&&
        gh_enemy203_is_r104_entry(&s->runtime.entry->request,s->runtime.entry->b.work->room);
    const int devil_ranged=s->pending_ai_pc==0x2269f0&&dependency==0x220&&p[0x2f7]==2;
    const int devil_melee=s->pending_ai_pc==0x21a538&&dependency==0x220&&!p[0x2f7];
    const int devil_combo=s->pending_ai_pc==0x21fd50&&dependency==0x220&&
        half(p+0x2fe)==0x220&&half(p+0x640)==0xff;
    const gh_boot_work *work=s->runtime.entry->b.work;
    const int r102=s->pending_ai_pc==0x21a538&&work->room==0x102&&
        (dependency==0x200||r102_em03_stamp8(work,p)||r102_east_em27(work,p));
    const int r102_em02=s->pending_ai_pc==0x21a538&&
        (r102_area11_em02(work,p)||(dependency==0x202&&r102_gate_reinforcement(work,p)));
    const int west_em02=s->pending_ai_pc==0x21a538&&dependency==0x202&&r100_at3_enemy(work,p);
    const int r101_wave=s->enemy&&r101_arena_airborne(s->enemy)&&
        ((half(p+0x640)==0x16&&dependency==0x200)||(half(p+0x640)==0x20&&dependency==0x203));
    const int r101=s->pending_ai_pc==0x21a538&&
        (r101_em00_stamp12(work->room,p)||r101_em00_stamp15(work->room,p)||
         r101_em02_stamp5(work->room,p)||r101_em03_stamp0(work->room,p)||r101_scenery_enemy(work,p)||r101_wave);
    const int r105=s->pending_ai_pc==0x21a538&&r105_opening_enemy(work,p);
    const int formation=s->pending_ai_pc==0x21a538&&
        (r100_at2_formation(work,p)||r100_at2_enemy(work,p)||r100_west_em03_patrol(work,p));
    const int arena=s->pending_ai_pc==0x21a538&&r103_arena_jumper(s->runtime.entry);
    const int arena_em03=arena&&dependency==0x203;
    const int arena_boss_attack=s->pending_ai_pc==0x21a538&&r103_arena_boss(s->runtime.entry);
    const int paired_boss=s->pending_ai_pc==0x22fae0&&r103_arena_boss(s->runtime.entry)&&
        p[0x2f6]==1&&p[0x2f7]==1&&word(p+0x1870)==2;
    const int flank_impact=s->pending_ai_pc==0x22cbc8&&
        r103_arena_boss(s->runtime.entry)&&p[0x2f6]==7&&!p[0x2f7];
    const int pincer_charge=(s->pending_ai_pc==0x22c3f8||s->pending_ai_pc==0x22cbc8)&&
        r103_arena_boss(s->runtime.entry)&&p[0x2f6]==11&&!p[0x2f7];
    const int flank_followup=s->pending_ai_pc==0x22cbc8&&r103_arena_boss(s->runtime.entry)&&!p[0x2f7];
    const int flank_lunge=flank_followup&&p[0x2f6]==15;
    const int flank_recover=flank_followup&&p[0x2f6]==17;
    const int arena_slide=s->pending_ai_pc==0x2236e8&&r103_arena_em0a(s->runtime.entry);
    const int arena_escape=s->pending_ai_pc==0x25bcc0&&r103_arena_em00(s->runtime.entry);
    const int story_escape=s->pending_ai_pc==0x25bcc0&&r102_grab_escape_counter(work,p);
    const int escape=arena_escape||story_escape||
        (s->pending_ai_pc==0x25bcc0&&dependency==0x200&&half(p+0x640)==20);
    const int r102_em0a=s->pending_ai_pc==0x21a538&&r102_gate_em0a(work,p);
    const int arena_em0a=s->pending_ai_pc==0x21a538&&r103_arena_em0a(s->runtime.entry);
    const int west_em0a=s->pending_ai_pc==0x21a538&&work->room==0x100&&dependency==0x20a&&
        !half(p+0x640)&&p[0x1650]==3;
    const int em0a=west_em0a||r102_em0a||arena_em0a;
    const int route=(s->pending_ai_pc==0x218c48||s->pending_ai_pc==0x21a218)&&r102_route_runner(work,p);
    const int landing=s->pending_ai_pc==0x240548&&s->enemy&&
        (r102_mount_passenger(s->enemy)||r101_arena_airborne(s->enemy)||r102_area11_obstacle(work,p)||
         r103_arena_jumper(s->runtime.entry)||r103_arena_em0a(s->runtime.entry)||
         r103_arena_boss(s->runtime.entry));
    const int formation_pair=s->pending_ai_pc==0x230f20&&r100_at2_formation(work,p);
    const int explicit_root=landing||formation_pair||flank_impact||flank_recover;
    if(!charge&&!em03_ranged&&!em03_approach&&!em03_combo&&!r104_melee&&!explicit_root&&!pincer_charge&&!flank_lunge&&
       !route&&!r101&&!r102&&!r102_em02&&!west_em02&&!r102_em0a&&!story_escape&&
       !r105&&!arena&&!arena_boss_attack&&!paired_boss&&!arena_escape&&!formation&&!arena_slide&&!arena_em0a&&
       ((!devil&&!devil_ranged&&!devil_melee&&!devil_combo&&!escape&&!em0a&&(s->pending_ai_pc!=0x21a538||
       (dependency!=0x200&&(dependency!=0x203||half(p+0x640)!=19))))||work->room!=0x100))
        return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
    const unsigned count=half(p+0x3ae);
    if(count>8)return stop(s,0x14fb48,GH_ACTOR_MOTION_BOUNDS);
    const gh_title_player_view *actor=&s->runtime.entry->object.view;
    gh_effect_create_call *call=&s->death_effect_create;
    gh_effect_create_bindings bindings=s->impact_bindings;
    bindings.model=&s->runtime.entry->initialize.model.model;
    for(;s->death_effect_cursor<count;s->death_effect_cursor++){
        const uint8_t *entry=p+0x3b0+s->death_effect_cursor*12;
        const uint8_t flags=entry[9];
        if((flags&4u)&&p[0x436])continue;
        const int32_t mode=(int32_t)word(entry+4);
        gh_effect_status status;
        uint32_t pc=0;
        if(flags&0x80u){
            unsigned slot=0;
            if(!bindings.storage.effects)return stop(s,0x306200,GH_ACTOR_MOTION_UNSUPPORTED);
            status=gh_game_effect_release_00306200(&bindings.storage,(flags&1u)?actor:NULL,mode,2,&pc,&slot);
            if(status)return stop(s,pc?pc:0x306200,status==GH_EFFECT_UNSUPPORTED?
                GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE);
            continue;
        }
        if(explicit_root){
            if(entry[0]!=0x58||entry[1]!=2||mode!=-1||entry[8]||flags!=1u)
                return stop(s,0x306900,GH_ACTOR_MOTION_UNSUPPORTED);
        }else if(entry[8]!=0xff)return stop(s,0x306900,GH_ACTOR_MOTION_UNSUPPORTED);
        if(flags&8u){
            if(!r104_melee||s->pending_ai_pc!=0x232500||
               entry[0]!=0xa8||entry[1]!=3||mode!=-1||flags!=8u||
               !actor->alias_f0||!bindings.storage.effects)
                return stop(s,0x306838,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!call->active||call->complete){
                uint32_t location[3];
                memcpy(location,actor->alias_f0,sizeof(location));
                status=gh_effect_create_r104_em03_ground_00306838(call,location);
                if(status)return stop(s,0x306838,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            status=gh_game_effect_create_00306528(call,&bindings);
            if(status||!call->complete)return stop(s,call->frontier?call->frontier:0x306528,
                status==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE);
            continue;
        }
        if(escape&&(entry[0]!=0x58||(entry[1]!=0x27&&entry[1]!=0x59)||mode||flags!=7u))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(arena_slide&&(entry[0]!=0x58||mode||flags!=5u||
           !((entry[1]==0x15&&p[0x2f6]==1)||(entry[1]==0x16&&p[0x2f6]==3)||
             (entry[1]==0x18&&p[0x2f6]==7))))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(pincer_charge&&(entry[0]!=0x58||entry[1]!=(s->pending_ai_pc==0x22c3f8?0x32:0x33)||
           mode||flags!=5u))return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(flank_lunge&&(entry[0]!=0x58||mode||
           !((entry[1]==0x34&&flags==7u)||(entry[1]==0x4f&&flags==5u))))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        const int em0a_strike=entry[1]==0x1e&&flags==7u;
        const int em0a_sweep=(r102_em0a||arena_em0a)&&entry[1]==0x1d&&flags==3u;
        const int em0a_west_pair=west_em0a&&!p[0x2f4]&&p[0x2f5]==0x15&&p[0x2f6]==1&&
            !p[0x2f7]&&entry[1]==0x24&&flags==7u;
        if(em0a&&(entry[0]!=0x58||mode||(!em0a_strike&&!em0a_sweep&&!em0a_west_pair)))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(charge&&(entry[0]!=0x58||entry[1]!=0x1a||mode||flags!=7u))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(em03_ranged&&(entry[0]!=0xa8||entry[1]!=0x14||mode||flags!=7u))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(em03_approach&&(entry[0]!=0xa8||entry[1]!=0xb||mode||flags!=5u))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(r104_melee){
            unsigned cue=0x1c;
            if(p[0x2f5]==0x21)cue=0x13;
            else if(s->reaction_resource_offset==0x1974){
                if(s->substate67_phase==4)cue=7;
                else if(s->substate67_phase==8)cue=9;
                else if(s->substate67_phase==12)cue=0xa;
            }
            else if(s->reaction_resource_offset==0x19d0&&s->substate67_phase==4)cue=0x1a;
            else if(s->reaction_resource_offset==0x19e0&&s->substate67_phase==4)cue=6;
            if(entry[0]!=0xa8||entry[1]!=cue||mode||flags!=7u)
                return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(em03_combo&&(entry[0]!=0xa8||(entry[1]!=2&&entry[1]!=8)||mode||flags!=7u))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(devil&&(entry[0]!=0x58||entry[1]!=0x3f||mode||flags!=7u))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(devil_combo){
            const unsigned phase=p[0x2f6];
            const unsigned row=phase==3?0:phase==5?1:2;
            if((phase!=3&&phase!=5&&phase!=7)||entry[0]!=0x58||
               (entry[1]!=0x35+row&&entry[1]!=0x49+row)||mode||flags!=(phase==5?3u:7u))
                return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if((devil_ranged&&(entry[0]!=0x58||entry[1]<0x42||entry[1]>0x45||mode||flags!=5u))||
           (devil_melee&&(entry[0]!=0x58||entry[1]!=0x4e||mode||flags!=7u))||
           (route&&(entry[0]!=0x58||entry[1]!=0x5d||mode||flags!=5u))||
           (arena_boss_attack&&(entry[0]!=0x58||
            (entry[1]!=0x2c&&(dependency!=0x214||entry[1]!=0x2b))||mode||flags!=3u))||
           (paired_boss&&(entry[0]!=0x58||entry[1]!=0x50||mode||flags!=7u))||
           ((r102_em02||arena_em03||west_em02)&&(entry[0]!=0x58||entry[1]!=0x2d||mode||flags!=3u)))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!bindings.storage.effects)
            return stop(s,0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!call->active||call->complete){
            const gh_effect_create_request request={.bank=entry[0],.index=entry[1],
                .actor=(flags&1u)?actor:NULL,.mode=mode,.selector=-1};
            /* Original 14FB48 explicit part 0 takes 306900, with unit scale. */
            status=explicit_root?gh_effect_create_enemy_death_part0_00306900(call,actor,mode):
                gh_effect_create_actor_sequence_0014fb48(call,&request,word(p+0x114));
            if(status)return stop(s,0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        status=gh_game_effect_create_00306528(call,&bindings);
        if(status||!call->complete)return stop(s,call->frontier?call->frontier:0x306528,
            status==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE);
    }
    return GH_ACTOR_MOTION_OK;
}

static int r103_heavy_recoil_getup(const gh_enemy_actor *enemy,const uint8_t *p)
{
    const uint32_t power=word(p+0x17b4);
    return r103_arena_boss(&enemy->entry)&&positive_hp(p)&&
        (power==25||power==80)&&!p[0x2f7];
}

static gh_actor_motion_status ground_sequence_0014fb48(gh_enemy202_move *s,const uint8_t *p)
{
    const unsigned count=half(p+0x3ae);
    if(count>8)return stop(s,0x14fb48,GH_ACTOR_MOTION_BOUNDS);
    const int em0a_formation=s->pending_ai_pc==0x23a950&&word(p+0x564)==0x20a&&
        !half(p+0x640)&&p[0x1650]==3&&s->enemy->b.entry.work->room==0x100;
    const gh_boot_work *work=s->enemy->b.entry.work;
    const int opening_getup=work->room==0x100&&word(p+0x564)==0x200&&half(p+0x640)==20;
    const int getup=s->pending_ai_pc==0x257948&&
        (opening_getup||r102_em00_stamp11(work,p)||r102_em00_stamp7_grab(work,p)||r103_heavy_recoil_getup(s->enemy,p));
    const int blowaway=s->pending_ai_pc==0x254450&&r102_gate_em0a(work,p);
    if(!(blowaway||em0a_formation||(s->dispatch_pc==0x25e600&&(s->pending_ai_pc==0x25e790||s->pending_ai_pc==0x25ed00))||
         (s->dispatch_pc==0x24c0f0&&(s->pending_ai_pc==0x253de8||getup||
           (s->pending_ai_pc==0x257948&&r103_arena_em00(&s->enemy->entry)))))||
       !s->impact_bindings.storage.effects)return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
    gh_effect_create_bindings bindings=s->impact_bindings;
    bindings.model=&s->enemy->entry.initialize.model.model;
    for(;s->death_effect_cursor<count;++s->death_effect_cursor){
        const uint8_t *record=p+0x3b0+s->death_effect_cursor*12;
        if(record[0]!=0x58||record[1]!=2||record[8]||record[9]!=1||
           word(record+4)!=UINT32_MAX)return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_effect_create_call *call=&s->death_effect_create;
        if(!call->active||call->complete){
            gh_effect_status status=gh_effect_create_enemy_death_part0_00306900(call,
                &s->enemy->entry.object.view,-1);
            if(status)return stop(s,0x306900,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        gh_effect_status status=gh_game_effect_create_00306528(call,&bindings);
        if(status||!call->complete)return stop(s,call->frontier?call->frontier:0x306528,
            status==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE);
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status death_ground_pitch_0026eb20(gh_enemy202_move *s,uint8_t *p)
{
    const gh_boot_work *work=s->enemy?s->enemy->b.entry.work:NULL;
    const int counter_motion=s->pending_ai_pc==0x25bcc0||s->pending_ai_pc==0x257948;
    const int opening_counter=work&&counter_motion&&work->room==0x100&&
        word(p+0x564)==0x200&&half(p+0x640)==20;
    const int arena_counter=s->enemy&&counter_motion&&r103_arena_em00(&s->enemy->entry);
    /* The final counter sample commits the fall or getup before the terrain tail. */
    const unsigned state=half(p+0x2f4);
    const int counter_tail=s->pending_ai_pc==0x25bcc0&&
        (p[0x2f7]==1||(!p[0x2f7]&&(state==0x0d01||(state==0x0202&&!positive_hp(p)))));
    const int story_counter=(r102_em00_stamp11(work,p)||r102_em00_stamp3(work,p)||r102_em00_stamp7_grab(work,p))&&
        (counter_tail||s->pending_ai_pc==0x257948);
    const int heavy_recoil=s->enemy&&s->pending_ai_pc==0x257948&&r103_heavy_recoil_getup(s->enemy,p);
    const int counter=s->dispatch_pc==0x24c0f0&&(opening_counter||arena_counter||story_counter||heavy_recoil);
    const int em0a_formation=s->pending_ai_pc==0x23a950&&word(p+0x564)==0x20a&&
        !half(p+0x640)&&p[0x1650]==3&&s->enemy->b.entry.work->room==0x100;
    const int em0a_fall=s->dispatch_pc==0x24c0f0&&r102_gate_em0a(work,p)&&
        (s->pending_ai_pc==0x254450||s->pending_ai_pc==0x257948);
    const int arena_slide=(s->pending_ai_pc==0x2236e8||
        (s->dispatch_pc==0x24c0f0&&s->pending_ai_pc==0x257948))&&
        r103_arena_em0a(&s->enemy->entry);
    if(!counter&&!em0a_formation&&!em0a_fall&&!arena_slide&&!((s->dispatch_pc==0x25e600&&(s->pending_ai_pc==0x25e790||s->pending_ai_pc==0x25ed00||
          s->pending_ai_pc==0x25f1f0))||(s->dispatch_pc==0x24c0f0&&s->pending_ai_pc==0x253de8)))
        return stop(s,0x26eb20,GH_ACTOR_MOTION_UNSUPPORTED);
    float matrix[16];
    memcpy(matrix,p+0x80,sizeof(matrix));
    for(;s->death_ground_cursor<2;++s->death_ground_cursor){
        const unsigned i=s->death_ground_cursor;
        const float local[3]={0,0,i?-.25f:.25f};
        float *point=s->death_ground[i];
        if(gh_scene_at_apply_world_001508d8(matrix,local,point))
            return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
        float hit_point[4]={0,0,0,1};
        /* Actual 2DB6D0 initializes both endpoints to VF0. It never copies
         * the input position into either endpoint; only a miss reads its Y. */
        gh_scene_ray_request ray={.from={0,-49,0,1},.to={0,0,0,1},
            .mask=2,.segmented=1,.hit_position=hit_point};
        int hit=0;
        uint32_t pc=0;
        if(gh_scene_ray_query_00131e10(s->enemy->b.actors,&ray,&hit,&pc))
            return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
        if(hit==1)point[1]=hit_point[1];
    }
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x26eb20,GH_ACTOR_MOTION_CALLEE);
    const float dx=sub(s->death_ground[0][0],s->death_ground[1][0]);
    const float dz=sub(s->death_ground[0][2],s->death_ground[1][2]);
    const float dy=sub(s->death_ground[0][1],s->death_ground[1][1]);
    volatile float distance=sqrtf(add(mul(dx,dx),mul(dz,dz)));
    if(fesetenv(&env)||!isfinite(distance)||!isfinite(dy))
        return stop(s,0x1f7e80,GH_ACTOR_MOTION_UNSUPPORTED);
    float angle;
    uint32_t pc=0;
    if(gh_game_angle_001f8180(dy,distance,&angle,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
        return stop(s,pc?pc:0x1f8180,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x26eb20,GH_ACTOR_MOTION_CALLEE);
    const float pitch=mul(mul(number(word(p+0x1708)),.9f),mul(-angle,.1f));
    if(fesetenv(&env)||!isfinite(pitch))return stop(s,0x26eb20,GH_ACTOR_MOTION_UNSUPPORTED);
    put(p+0x1708,ordered_abs(pitch)<.01f?0:bits(pitch));
    if(ordered_abs(pitch)>=.01f)put(p+0x100,bits(pitch));
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_body_finish(gh_enemy202_move *s,uint8_t *p)
{
    if(s->substate67_stage==8){
        int update=0;
        uint32_t flags=word(p+0x16d0);
        if(!(half(p+0x3ac)&4u)&&!(flags&0x20u)){
            gh_actor_motion_status status=camera_update_gate(s,p,&update);
            if(status)return status;
        }
        if(!update||(flags&0x38000u))put(p+0x258,word(p+0x258)|0x80000000u);
        s->substate67_stage=9;
    }
    if(s->substate67_stage==9){
        int special=half(p+0x2f4)==0x0202;
        if(!special&&half(p+0x3ae)){
            gh_actor_motion_status status=(s->dispatch_pc==0x25e600||s->pending_ai_pc==0x253de8||
                s->pending_ai_pc==0x257948||s->pending_ai_pc==0x23a950||s->pending_ai_pc==0x254450)?
                ground_sequence_0014fb48(s,p):advance_sequence_effects_0014fb48(s,p);
            if(status)return status;
        }
        uint32_t flags=word(p+0x250)&~2u;
        if(!special&&!(word(p+0x16d0)&0x08000000u)){
            float alpha=number(word(p+0x24c));
            if(!isfinite(alpha))return stop(s,0x20999c,GH_ACTOR_MOTION_UNSUPPORTED);
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x20999c,GH_ACTOR_MOTION_CALLEE);
            int fade_out=p[0x1868]&&(word(p+0x16d0)&0x8000u)&&number(word(p+0x618))>900;
            float next=fade_out?sub(alpha>1?1:alpha,0.1f):add(alpha,0.1f);
            if(fesetenv(&env)||!isfinite(next))
                return stop(s,0x20999c,GH_ACTOR_MOTION_UNSUPPORTED);
            if(fade_out){
                if(next>0.15f)flags|=16u;
                else{flags=(flags|2u)&~16u;next=0;}
            }else if(next>=1){flags&=~16u;next=1;}
            else flags|=16u;
            put(p+0x24c,bits(next));
        }
        if(p[0x623])flags|=2u;
        put(p+0x250,flags);
        s->substate67_stage=10;
    }
    if(s->substate67_stage==10){
        if(p[0x1868]&&(word(p+0x16d0)&0x8000u)){
            int update;
            gh_actor_motion_status status=camera_update_gate(s,p,&update);
            if(status)return status;
            s->substate67_stage=12;
            if(!update||number(word(p+0x24c))<=0){
                s->substate67_stage=13;
            }
        }else{
            uint32_t dependency=word(p+0x564);
            gh_actor_model *model=&s->runtime.entry->initialize.model.model;
            const gh_actor_part *parts[2]={NULL,NULL};
            for(unsigned i=0;i<2;i++)
                if(p[0x2b4]>i+1&&gh_actor_model_part_at(model,i+1,&parts[i]))
                    return stop(s,0x26a3d8,GH_ACTOR_MOTION_UNSUPPORTED);
            float roll=number(word(p+0x16fc));
            if(!isfinite(roll))return stop(s,0x26a508,GH_ACTOR_MOTION_UNSUPPORTED);
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x26a508,GH_ACTOR_MOTION_CALLEE);
            const float decay=dependency==0x223||dependency==0x275||dependency==0x276?-0.6f:0.6f;
            float next=mul(roll,decay);
            if(fesetenv(&env)||!isfinite(next))
                return stop(s,0x26a508,GH_ACTOR_MOTION_UNSUPPORTED);
            for(unsigned i=0;i<2;i++)if(parts[i]){
                uint8_t *part=parts[i]->bytes;
                put(part+0x150,roll==0?0:bits(roll));
                uint32_t flags=word(part+0x154);
                put(part+0x154,roll==0?flags&~0x800u:flags|0x800u);
            }
            put(p+0x16fc,ordered_abs(next)<=0.017453292f?0:bits(next));
            s->substate67_stage=11;
        }
    }
    if(s->substate67_stage==11){
        /* These two original texture updates return for the other dependencies. */
        const uint32_t dependency=word(p+0x564);
        if(dependency==0x251)return stop(s,0x26a558,GH_ACTOR_MOTION_UNSUPPORTED);
        if(dependency==0x265)return stop(s,0x273f98,GH_ACTOR_MOTION_UNSUPPORTED);
        if(half(p+0x3ac)&8u){
            gh_actor_motion_status status=death_ground_pitch_0026eb20(s,p);
            if(status)return status;
        }else{
            float roll=number(word(p+0x1708));
            if(!isfinite(roll))return stop(s,0x26eb20,GH_ACTOR_MOTION_UNSUPPORTED);
            /* In the actual flag-clear branch the desired angle is zero. */
            put(p+0x1708,0);
        }
        s->substate67_stage=12;
    }
    const gh_enemy_move_actor *actor=&s->runtime;
    gh_actor_model *model=&actor->entry->initialize.model.model;
    if(s->substate67_stage==12){
        gh_actor_motion_status status=adjust_ground_pose_00275668(s,p);
        if(status)return status;
        s->substate67_stage=14;
    }
    if(s->substate67_stage==13)s->substate67_stage=18;
    if(s->substate67_stage==14){
        if(gh_actor_model_update_hierarchy_00149eb8(model,GH_STATIC_BINARY32_REFERENCE))
            return stop(s,model->frontier?model->frontier:0x149eb8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=15;
    }
    if(s->substate67_stage==15){
        gh_motion_solve_bindings b={.model=model,.work=actor->entry->b.work,.actors=actor->bindings->actors,
            .ray=actor->bindings->ray,.context=actor->bindings->ray_context,.ik_globals=actor->bindings->ik,.atan_coeff=actor->bindings->atan_coeff};
        uint32_t pc;
        gh_actor_motion_status status=gh_actor_motion_solve_00152a20(&s->post_solve,&b,
            GH_STATIC_BINARY32_REFERENCE,&pc);
        if(status)return stop(s,pc?pc:0x152a20,status);
        s->substate67_stage=16;
    }
    if(s->substate67_stage==16){
        if(gh_actor_finish_model_resume_0014a038(&s->post_finish,model,GH_STATIC_BINARY32_REFERENCE))
            return stop(s,model->frontier?model->frontier:0x14a038,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=17;
    }
    return advance_post_pose(s,p);
}

static gh_actor_motion_status choose_after_substate_002705d8(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_boot_work *work=enemy->b.entry.work;
    if(word(p+0x738))return stop(s,0x1d0408,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t dependency=word(p+0x564);
    const int r101_box=r101_scenery_enemy(work,p)&&
        ((p[0x2f4]==1&&(p[0x2f5]==0x11||(!p[0x2f5]&&p[0x2f6]==1)))||(p[0x2f4]==0&&
        ((p[0x2f5]==1&&p[0x2f6]==2)||(p[0x2f5]==9&&p[0x2f6]==1)||
         p[0x2f5]==5||p[0x2f5]==6)));
    const int r101_gate_guard=r101_gate_exchanged_enemy(work,p)&&
        !p[0x2f4]&&((p[0x2f5]==1&&p[0x2f6]==2)||
        ((p[0x2f5]==6||p[0x2f5]==0x6b)&&p[0x2f6]==1));
    if(dependency!=0x200&&dependency!=0x202&&dependency!=0x203&&dependency!=0x20a&&
        !r103_arena_boss(&enemy->entry)&&!r101_box&&!r101_gate_guard&&!r102_route_runner(work,p)&&!r105_exchanged_enemy(work,p)&&
        !r102_east_em27(work,p)&&!gh_enemy203_is_r104_entry(&enemy->entry.request,work->room))
        return stop(s,0x2705d8,GH_ACTOR_MOTION_UNSUPPORTED);
    float escape=number(word(p+0x16c0)),override=number(word(p+0x1734));
    if(!isfinite(escape)||!isfinite(override))return stop(s,0x27069c,GH_ACTOR_MOTION_UNSUPPORTED);
    unsigned next;
    if(escape>0)next=0x13;
    else{
        if(work->room==0x200||work->room==0x204)return stop(s,0x274150,GH_ACTOR_MOTION_UNSUPPORTED);
        if(override>0)next=0x89;
        else{
            if(work->room==0x700)return stop(s,0x274e98,GH_ACTOR_MOTION_UNSUPPORTED);
            uint8_t pending=p[0x1864];
            if(pending){
                unsigned heat=heat_category(s);
                if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
                if(heat>=3){
                    uint32_t draw;
                    if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
                        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                    if(draw&1u)pending=0;
                }
                if(word(p+0x16ec)||number(word(p+0x760))>number(0x3fc90fdb))pending=0;
            }
            uint16_t player_mode=half(s->player->bytes+0x2f4);
            p[0x1864]=0;
            if(pending&&player_mode!=0x501&&player_mode!=0x601)next=0x6c;
            else{
                float actor[3],target[3],turn;
                uint32_t pc=0;
                memcpy(target,p+0x16a0,12);
                if(!position(&enemy->entry.object.view,actor)||
                    gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                        number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
                next=turn>number(0x3fc90fdb)?8:0;
                if(next==8)put(p+0x16d4,word(p+0x16d4)|1u);
            }
        }
    }
    p[0x2f4]=0;p[0x2f5]=(uint8_t)next;p[0x2f6]=0;p[0x2f7]=0;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_state0_tail(gh_enemy202_move *s,uint8_t *p)
{
    const gh_enemy_move_actor *actor=&s->runtime;
    uint32_t pc=0;
    if(s->substate67_stage==5){
        if(word(p+0x16d0)&0x08008000u){
            s->substate67_stage=number(word(p+0x618))<400?37:38;
        }else{
            float duration=number(word(p+0x1704));
            if(!isfinite(duration))return stop(s,0x26ea50,GH_ACTOR_MOTION_UNSUPPORTED);
            if(duration>0){
                fenv_t env;
                if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x26ea50,GH_ACTOR_MOTION_CALLEE);
                float remaining=sub(duration,0.02f),alpha=sub(1,remaining);
                if(fesetenv(&env)||!isfinite(alpha))return stop(s,0x26ea50,GH_ACTOR_MOTION_UNSUPPORTED);
                if(gh_collision_solid_set_enabled_001351d8(actor->bindings->solids,&actor->entry->object.view,0,&pc))
                    return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
                put(p+0x1704,bits(remaining));put(p+0x24c,bits(alpha>=1?1:alpha));
                uint32_t flags=word(p+0x250);
                put(p+0x250,alpha>=1?flags&~16u:flags|16u);
                s->substate67_stage=38;
            }else s->substate67_stage=37;
        }
    }
    if(s->substate67_stage==37){
        if(s->opacity&&(s->opacity->init!=actor->entry->b.init||
            s->opacity->current_player!=s->player))return GH_ACTOR_MOTION_ARGUMENT;
        if(gh_game_object_opacity_002a87e8(&actor->entry->object.view,NULL,actor->entry->b.work,
            s->opacity,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x2a87e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=38;
    }
    if(s->substate67_stage==38){
        p[0x1866]=0;
        p[0x1867]=0;
        s->substate67_stage=6;
    }
    if(s->substate67_stage==6){
        uint32_t counter=word(p+0x5b4);
        if(counter&&counter<=INT32_MAX){
            float timer=number(word(p+0x5b8)),rate=number(word(p+0x5a8));
            if(!isfinite(timer)||!isfinite(rate))return stop(s,0x28ed90,GH_ACTOR_MOTION_UNSUPPORTED);
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x28ed90,GH_ACTOR_MOTION_CALLEE);
            float remaining=sub(timer,rate);
            if(fesetenv(&env)||!isfinite(remaining))
                return stop(s,0x28ed90,GH_ACTOR_MOTION_UNSUPPORTED);
            if(remaining<=0){counter=counter>30?counter-30:0;remaining=90;}
            put(p+0x5b4,counter);
            put(p+0x5b8,bits(remaining));
        }
        s->substate67_stage=7;
    }
    if(s->substate67_stage==7){
        gh_hitarea_object *body=&actor->hitareas->units[0];
        gh_title_player_view *owner=NULL;
        const int retired_body=s->dispatch_pc==0x25e600&&s->pending_ai_pc==0x25f1f0&&
            p[0x2f6]==4&&(word(p+0x250)&0x80000000u)&&!actor->hitareas->created[0]&&!word(p+0x698);
        if(!retired_body&&(!actor->hitareas->created[0]||gh_hitarea_object_actor(body,&owner)||
           (!body->fallback&&owner!=&actor->entry->object.view)))
            return stop(s,0x1fd610,GH_ACTOR_MOTION_UNSUPPORTED);
        /* 312708 has cleared the original body pointer before Move resumes. */
        if(!retired_body&&!body->fallback){
            float body_timer=number(word(p+0x1770));
            if(!isfinite(body_timer))return stop(s,0x2097e0,GH_ACTOR_MOTION_UNSUPPORTED);
            for(;s->post_body_index<2;s->post_body_index++){
                int16_t index=(int16_t)half(p+0x69c+s->post_body_index*4);
                if(gh_hitarea_object_set_node_active_001fd610(body,index,body_timer==0,&pc))
                    return stop(s,pc?pc:0x1fd610,GH_ACTOR_MOTION_UNSUPPORTED);
            }
        }
        uint32_t timer=word(p+0x16f8);
        if(timer){
            put(p+0x16f8,timer-1);
            put(p+0x16d0,word(p+0x16d0)|0x1000u);
        }
        s->substate67_stage=8;
    }
    return advance_body_finish(s,p);
}

static gh_actor_motion_status advance_substate67(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    if(!s->substate67_stage){
        s->substate67_phase=p[0x2f6];
        switch(s->substate67_phase){
        case 0:
            p[0x1864]=0;
            s->substate67_stage=1;
            break;
        case 2:
        case 4:
            s->substate67_stage=1;
            break;
        case 1:
        case 5:
            s->substate67_stage=2;
            break;
        case 3:
            s->substate67_stage=33;
            break;
        default:
            s->substate67_stage=5;
            break;
        }
    }
    if(s->substate67_stage==1){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            size_t offset=s->substate67_phase==4?0xc10:0xc08;
            if(!resource->bytes||!resource->file_extent_known||
                resource->file_extent<offset+8||resource->file_extent>resource->size)
                return stop(s,0x23ab18,GH_ACTOR_MOTION_BOUNDS);
            uint16_t flags=p[0x17c3]?2:0;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+offset),word(resource->bytes+offset+4),
                0,10,flags,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(s->substate67_phase==0)put(p+0x16d0,(word(p+0x16d0)|0x8000u)&~2u);
        if(s->substate67_phase==2)put(p+0x5f0,1);
        ++p[0x2f6];
        s->substate67_phase=p[0x2f6];
        s->substate67_stage=s->substate67_phase==3?33:2;
    }
    if(s->substate67_stage==33){
        put(p+0x16d0,word(p+0x16d0)|0x400000u);
        int maximum=(int16_t)half(p+0x548),hp=(int16_t)half(p+0x54a)+1;
        put_half(p+0x54a,(uint16_t)(hp<maximum?hp:maximum));
        uint32_t frame=word((const uint8_t*)enemy->b.entry.work+0x210);
        if(((frame^word(p+0x17d0))&7u)==0){
            if(!position(s->player,s->path_target))return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x16d4,word(p+0x16d4)&~0x08000000u);
            s->route=(gh_enemy_route_call){0};
            s->substate67_stage=34;
        }else s->substate67_stage=35;
    }
    if(s->substate67_stage==34){
        float actor[3];
        if(!position(&enemy->entry.object.view,actor))return stop(s,0x2a74e0,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=route_clear_002a74e0(s,p,actor);
        if(status)return status;
        s->pending_ai_pc=0x23aad0;
        if(number(word(p+0x510))<8){
            put(p+0x5f0,0);
            put(p+0x16d0,(word(p+0x16d0)&~0x8000u)|2u);
        }
        s->substate67_stage=35;
    }
    if(s->substate67_stage==35){
        if(word(p+0x16ec))p[0x2f6]=4;
        s->substate67_stage=2;
    }
    uint32_t pc=0;
    if(s->substate67_stage==2){
        put(p+0x16d0,word(p+0x16d0)|0x400000u);
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(s->substate67_phase==3){
            if(ended&&!word(p+0x5f0))p[0x2f6]=4;
            s->substate67_stage=3;
        }else if(ended){
            if(s->substate67_phase==5)s->substate67_stage=36;
            else{++p[0x2f6];s->substate67_stage=5;}
        }else s->substate67_stage=3;
    }
    if(s->substate67_stage==36){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    if(s->substate67_stage==3){
        if(gh_player_root_rotation_0014f9f0(&enemy->entry.object.view,enemy->b.entry.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=4;
    }
    if(s->substate67_stage==4){
        if(gh_player_root_translation_0014f928(&enemy->entry.object.view,enemy->b.entry.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

gh_actor_motion_status gh_enemy202_move_bind_player_aux_698(gh_enemy202_move *s,
    const gh_title_player_view *aux)
{
    if(!s||s->stage||(aux&&(!aux->bytes||aux->size<0x300)))return GH_ACTOR_MOTION_ARGUMENT;
    s->player_aux_698=aux;
    s->player_aux_698_known=1;
    return GH_ACTOR_MOTION_OK;
}

static void enemy_decision(uint8_t *p,uint8_t state,uint8_t substate,uint8_t detail)
{
    p[0x2f4]=state;p[0x2f5]=substate;p[0x2f6]=0;p[0x2f7]=detail;
}

/* Original 26E588 default home radius for ordinary enemies. */
static gh_actor_motion_status enemy_home_range_0026e588(gh_enemy202_move *s,
    uint8_t *p,float distance,int *chosen)
{
    if(distance<16.0f||(word(p+0x1644)&0x40000000u)||(int32_t)word(p+0x16ec)>=1)
        return GH_ACTOR_MOTION_OK;
    float home[3],player[3];
    memcpy(home,p+0x1660,12);
    if(!position(s->player,player))return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
    const float radius=number(word(p+0x16c8));
    const float home_distance=distance_xz(home,player),range=mul(radius,radius);
    if(fesetenv(&env)||!isfinite(home_distance)||!isfinite(range))
        return stop(s,0x1f7eb0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(home_distance<range)return GH_ACTOR_MOTION_OK;
    if(word(p+0x738))return stop(s,0x1d0408,GH_ACTOR_MOTION_UNSUPPORTED);
    put(p+0x16d0,word(p+0x16d0)&~2u);
    memcpy(p+0x16b0,home,12);
    put(p+0x16c0,0x43e10000);
    put(p+0x16c4,6);
    enemy_decision(p,0,0x13,0);
    *chosen=1;
    return GH_ACTOR_MOTION_OK;
}

/* Ordinary active dependencies beyond every 211C08 attack radius. */
static gh_actor_motion_status attack_choice_ordinary_far(gh_enemy202_move *s,
    uint8_t *p, float distance, float face, int *chosen)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_game_init_state *init=enemy->b.entry.init;
    const uint8_t *manager=init->enemies,*work=(const uint8_t *)enemy->b.entry.work;
    const uint32_t behavior=word(p+0x16d0),route=word(p+0x16d4);
    if(!isfinite(distance)||!isfinite(face)||distance<=144.0f)
        return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(number(word(p+0x16c0))>0)enemy_decision(p,0,0x13,0);
    else if(number(word(p+0x1734))>0)enemy_decision(p,0,0x89,0);
    else{
        if((int32_t)word(manager+0x530)>0)return stop(s,0x26e258,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=enemy_home_range_0026e588(s,p,distance,chosen);
        if(status||*chosen)return status;
        if(number(word(p+0x770))>0)enemy_decision(p,0,0x36,0);
        else{
            if((int16_t)half(s->player->bytes+0x54a)<=0||(word(work+0x5e4)&0x01000000u)||manager[0x5b5])
                return GH_ACTOR_MOTION_OK;
            if(manager[0x53d])enemy_decision(p,0,0x6c,0);
            else{
                if(manager[0x53c])return GH_ACTOR_MOTION_OK;
                if(number(word(p+0x17cc))>0)return stop(s,0x270c78,GH_ACTOR_MOTION_UNSUPPORTED);
                if(p[0x1864]||word(p+0x16dc)||(int32_t)word(manager+0x514)>0||
                    (!word(p+0x16ec)&&(int32_t)word(manager+0x510)>0)||!(behavior&0x200000u))
                    return GH_ACTOR_MOTION_OK;
                float actor[3],player[3],target[3],turn,player_turn;
                uint32_t pc=0;
                if(!position(&enemy->entry.object.view,actor)||!position(s->player,player))
                    return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
                fenv_t env;
                if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x2630f0,GH_ACTOR_MOTION_CALLEE);
                float height=fabsf(sub(actor[1],player[1]));
                float remaining=sub(number(word(p+0x171c)),number(word(p+0x5a8)));
                if(fesetenv(&env)||!isfinite(remaining))return stop(s,0x2630f0,GH_ACTOR_MOTION_CALLEE);
                if(height>1.5f)return GH_ACTOR_MOTION_OK;
                memcpy(target,p+0x1690,12);
                if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                    number(word(p+0x104)),number(0x40490fdb),&turn,&pc)||
                    gh_player_target_angle_002da038(s->elf,s->elf_bytes,player,actor,
                    number(word(s->player->bytes+0x104)),number(0x40490fdb),&player_turn,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
                const unsigned heat=heat_category(s);
                if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
                if(heat<5&&face>1.3089969f&&(route&0x08000000u)){
                    uint32_t draw;
                    if(gh_game_mt_next_0030f348(&init->random,&draw))
                        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                    if(draw&1u){
                        put(p+0x16d4,route|1u);
                        enemy_decision(p,0,8,1);
                        *chosen=1;
                        return GH_ACTOR_MOTION_OK;
                    }
                }
                static const float actor_limits[]={0,0.5235988f,1.0471976f,1.2217305f,1.5707964f,2.3561945f};
                static const float player_limits[]={0,1.0471976f,1.5707964f,2.3561945f,2.3561945f,3.1415927f};
                if(fabsf(turn)>actor_limits[heat])return GH_ACTOR_MOTION_OK;
                if(fabsf(player_turn)>player_limits[heat]){
                    if(!word(work+0x56c)&&heat<3)return GH_ACTOR_MOTION_OK;
                    put(p+0x171c,bits(remaining));
                }else put(p+0x171c,heat<3?0x44160000u:0x43960000u);
                /* Both the held and ordinary dep0200 attack selections
                 * return zero this far from the player (2663A8/268034). */
                return GH_ACTOR_MOTION_OK;
            }
        }
    }
    *chosen=1;
    return GH_ACTOR_MOTION_OK;
}

static int enemy_pickup_kind(unsigned kind)
{
    switch(kind){
    case 0x361:case 0x362:case 0x363:case 0x368:
    case 0x379:case 0x380:case 0x3af:case 0x3b7:return 1;
    default:return 0;
    }
}

/* Original 2DDAB0 uses the room weapon list in insertion order. The
 * approach search skips weapons already reserved by another enemy; pickup
 * mode skips that timer gate while retaining the eligibility and ray tests. */
static gh_actor_motion_status find_ground_weapon_002ddab0(gh_enemy202_move *s,
    float max_range_sq,int pickup,gh_player_target *found,float target[3])
{
    const gh_enemy_actor *enemy=s->enemy;
    const gh_player_target_bindings *b=&s->targets;
    const gh_room_vector *registry;
    if(b->pools!=enemy->b.entry.pools||b->actors!=enemy->b.actors||b->player!=s->player||
        !b->resolve||gh_room_registry_view(b->pools,0x754c58,&registry)||
        registry->count>registry->capacity||(registry->count&&!registry->allocation.pointer))
        return stop(s,0x2ddab0,GH_ACTOR_MOTION_UNSUPPORTED);

    float origin[3];
    if(!position(&enemy->entry.object.view,origin))
        return stop(s,0x2ddab0,GH_ACTOR_MOTION_BOUNDS);
    *found=(gh_player_target){0};
    for(uint32_t i=0;i<registry->count;i++){
        const uint32_t handle=word((const uint8_t *)registry->allocation.pointer+i*4);
        gh_title_player_view view={0};
        uint32_t pc=0;
        if(b->resolve(b->context,handle,&view,&pc))
            return stop(s,pc?pc:0x2ddab0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!view.bytes||view.size<0x300)return stop(s,0x2ddab0,GH_ACTOR_MOTION_BOUNDS);
        uint8_t *object=view.bytes;
        if((word(object+0x250)&0x80008000u)||!enemy_pickup_kind(half(object+0x2fe)))continue;
        if(view.size<0x670)return stop(s,0x1d07e0,GH_ACTOR_MOTION_BOUNDS);
        if(!object[0x664])continue;
        if(!pickup){
            const float timer=number(word(object+0x66c));
            if(!isfinite(timer))return stop(s,0x1d0ae0,GH_ACTOR_MOTION_BOUNDS);
            if(timer>0)continue;
        }

        if(!position(&view,target))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_BOUNDS);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
        const float range=distance_xz(origin,target),height=sub(origin[1],target[1]);
        gh_scene_ray_request ray={.mask=7,.segmented=1,
            .from={origin[0],add(origin[1],0.5f),origin[2],1},
            .to={target[0],add(target[1],0.5f),target[2],1}};
        if(fesetenv(&env)||!isfinite(range)||!isfinite(height))
            return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
        if(range>max_range_sq||height<-.5f||height>1.7f)continue;
        int hit;
        if(gh_scene_ray_query_00131e10(b->actors,&ray,&hit,&pc))
            return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!hit){
            found->handle=handle;
            found->view=view;
            return GH_ACTOR_MOTION_OK;
        }
    }
    return GH_ACTOR_MOTION_OK;
}

/* Original 2DDAB0 mode zero, then 272588's player-distance gate and SetGoto. */
static gh_actor_motion_status enemy_periodic_pickup_00272588(gh_enemy202_move *s,uint8_t *p,int *chosen)
{
    float player[3],target[3];
    if(!position(s->player,player))return stop(s,0x2ddab0,GH_ACTOR_MOTION_BOUNDS);
    gh_player_target found;
    gh_actor_motion_status status=find_ground_weapon_002ddab0(s,100.0f,0,&found,target);
    if(status||!found.handle)return status;

    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
    const float player_range=distance_xz(player,target);
    if(fesetenv(&env)||!isfinite(player_range))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
    /* A found weapon close to the player rejects this invocation. */
    if(player_range<6.25f)return GH_ACTOR_MOTION_OK;
    if(word(p+0x738))return stop(s,0x1d0408,GH_ACTOR_MOTION_UNSUPPORTED);
    put(found.view.bytes+0x66c,0x43e10000);
    memcpy(p+0x16b0,target,12);
    put(p+0x16c0,0x43960000);
    put(p+0x16c4,8);
    enemy_decision(p,0,0x13,0);
    *chosen=1;
    return GH_ACTOR_MOTION_OK;
}

static void set_attack_timers_0026ee40(gh_game_init_state *init,uint8_t *p,
    unsigned heat,unsigned lock)
{
    uint8_t *manager=init->enemies;
    const unsigned divisor=heat==5?0:heat>=3?4:2;
    const unsigned shared=divisor?30/divisor:0;
    if(shared&&(int32_t)word(manager+0x510)<(int32_t)shared)put(manager+0x510,shared);
    put(p+0x16dc,divisor?lock/divisor:0);
}

static gh_actor_motion_status commit_attack_0026ee40(gh_enemy202_move *s,uint8_t *p,
    gh_game_mt random,unsigned heat,unsigned substate,unsigned lock,unsigned delay,int *chosen)
{
    gh_game_init_state *init=s->enemy->b.entry.init;
    set_attack_timers_0026ee40(init,p,heat,lock);
    if(delay)put(p+0x16e0,delay);
    init->random=random;
    enemy_decision(p,0,(uint8_t)substate,0);
    *chosen=1;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status enemy_peer_query(gh_enemy202_move *s,const float origin[3],
    float yaw,float cone,const gh_title_player_view *exclude,gh_player_target *peer)
{
    const gh_enemy_actor *enemy=s->enemy;
    const gh_player_target_bindings *b=&s->targets;
    if(b->pools!=enemy->b.entry.pools||b->actors!=enemy->b.actors||b->player!=s->player)
        return stop(s,0x291010,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t pc=0;
    const gh_player_target_status status=gh_enemy_target_from_00291010(b,enemy->b.entry.init,
        origin,yaw,1.5f,cone,exclude,peer,&pc);
    return status?stop(s,pc?pc:0x291010,status==GH_PLAYER_TARGET_UNSUPPORTED?
        GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE):GH_ACTOR_MOTION_OK;
}

static int same_class_attack_peer(const uint8_t *actor,const gh_player_target *peer)
{
    if(!peer->handle||peer->view.size<0x568)return 0;
    const uint8_t *candidate=peer->view.bytes;
    if(candidate[0x2f4]||half(candidate+0x2fe)!=half(actor+0x2fe))return 0;
    const uint32_t dependency=word(candidate+0x564);
    if(dependency<0x220)
        return dependency!=0x21f&&(dependency<0x205||(dependency>0x209&&
            (dependency<0x20f||dependency>0x211)));
    if(dependency<0x242)return dependency!=0x224&&dependency!=0x226&&dependency!=0x241;
    return dependency<0x270||dependency>0x274;
}

/* Original 2630F0 facing and flank gates for ordinary enemies. */
static int r100_armed_em02(const gh_enemy_actor *enemy,const uint8_t *p)
{
    return enemy->b.entry.work->room==0x100&&word(p+0x564)==0x202&&
        half(p+0x640)==18&&p[0x1650]==9;
}

static int r100_far_stamp18(const gh_enemy_actor *enemy,const uint8_t *p,float distance)
{
    return r100_armed_em02(enemy,p)&&distance>144.0f;
}

static int r100_stamp18_armed_approach(const gh_enemy_actor *enemy,const uint8_t *p)
{
    return r100_armed_em02(enemy,p)&&
        (!(word(p+0x16d0)&0x200000u)||(p[0x2f4]==0&&p[0x2f5]==9&&p[0x2f6]==1))&&
        gh_enemy_timers_weapon_kind(&enemy->timers);
}

static float boss_pair_inverse_length(const float p[3])
{
    const volatile float length=sqrtf(add(add(mul(p[0],p[0]),mul(p[1],p[1])),mul(p[2],p[2])));
    const volatile float reciprocal=length>0.0f?1.0f/length:0.0f;
    return reciprocal;
}

/* 293138 casts both original segmented rays before advancing the canonical RNG. */
static gh_actor_motion_status boss_pair_00293138(gh_enemy202_move *s,gh_game_mt *random,int *chosen)
{
    gh_game_init_state *init=s->enemy->b.entry.init;
    gh_game_title_player *first=init->enemy_r103_boss[0],*second=init->enemy_r103_boss[1];
    if(!first||!second||init->enemies[0x53c]||init->heat[0x10])return GH_ACTOR_MOTION_OK;
    gh_title_player_view *a=&first->view,*b=&second->view;
    if(first!=&s->enemy->entry.object||!a->bytes||!b->bytes||a->size<0x58c||
        b->size<0x58c||word(a->bytes+0x564)!=0x214||word(b->bytes+0x564)!=0x215)
        return stop(s,0x293138,GH_ACTOR_MOTION_BOUNDS);
    if(!positive_hp(a->bytes)||!positive_hp(b->bytes)||a->bytes[0x2f4]||b->bytes[0x2f4]||
        (half(a->bytes+0x3ac)&0x90c)||(half(b->bytes+0x3ac)&0x90c))return GH_ACTOR_MOTION_OK;

    float gene[3],matrix[16],center[3],side[3],delta[3],yaw;
    if(!position(s->player,gene))return stop(s,0x2db6b0,GH_ACTOR_MOTION_BOUNDS);
    memcpy(matrix,s->player->bytes+0x80,sizeof(matrix));
    gh_scene_ray_request ray={.mask=7,.segmented=1};
    uint32_t pc=0;
    if(gh_scene_at_apply_world_001508d8(matrix,(const float[3]){-2.0f,0.5f,15.0f},ray.to))
        return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x293254,GH_ACTOR_MOTION_CALLEE);
    const float raised=add(gene[1],0.5f);
    if(fesetenv(&env))return stop(s,0x293254,GH_ACTOR_MOTION_CALLEE);
    memcpy(ray.from,(const float[4]){gene[0],raised,gene[2],1.0f},sizeof(ray.from));
    int hit;
    if(gh_scene_ray_query_00131e10(s->enemy->b.actors,&ray,&hit,&pc))
        return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    if(hit)return GH_ACTOR_MOTION_OK;

    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x293310,GH_ACTOR_MOTION_CALLEE);
    for(unsigned i=0;i<3;++i)delta[i]=sub(ray.to[i],ray.from[i]);
    const float inv=boss_pair_inverse_length(delta);
    for(unsigned i=0;i<3;++i)center[i]=add(ray.from[i],mul(mul(delta[i],inv),12.0f));
    center[1]=gene[1];
    const float dx=sub(center[0],gene[0]),dz=sub(center[2],gene[2]);
    if(fesetenv(&env))return stop(s,0x293310,GH_ACTOR_MOTION_CALLEE);
    if(gh_game_angle_001f8180(dx,dz,&yaw,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
        return stop(s,pc?pc:0x1f8180,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x293424,GH_ACTOR_MOTION_CALLEE);
    const float side_angle=sub(yaw,number(0x3fc90fdb));
    const float side_radius=add(mul(number(word(a->bytes+0x118)),number(0x3fe66666)),2.0f);
    if(fesetenv(&env)||!isfinite(side_radius))return stop(s,0x293494,GH_ACTOR_MOTION_BOUNDS);
    if(gh_id_angle_reference_001f7d48(side_angle,&yaw))
        return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
    float sine,cosine;
    if(gh_sdk_rotation_sincos_reference_0030a5d0(yaw,&sine,&cosine,
        GH_STATIC_BINARY32_REFERENCE,&pc))return stop(s,pc?pc:0x147988,GH_ACTOR_MOTION_UNSUPPORTED);

    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x2934a4,GH_ACTOR_MOTION_CALLEE);
    const float side_height=add(center[1],0.5f);
    memcpy(ray.from,(const float[4]){center[0],side_height,center[2],1.0f},sizeof(ray.from));
    memcpy(ray.to,(const float[4]){add(center[0],mul(side_radius,sine)),
        side_height,add(center[2],mul(side_radius,cosine)),1.0f},sizeof(ray.to));
    if(fesetenv(&env))return stop(s,0x2934a4,GH_ACTOR_MOTION_CALLEE);
    if(gh_scene_ray_query_00131e10(s->enemy->b.actors,&ray,&hit,&pc))
        return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    if(hit)return GH_ACTOR_MOTION_OK;

    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x29352c,GH_ACTOR_MOTION_CALLEE);
    /* The second 101E28 and normalization both read the central position
     * from SP+30, even though the ray delta has just been stored at SP+40. */
    const float side_inv=boss_pair_inverse_length(center);
    for(unsigned i=0;i<3;++i)side[i]=add(center[i],mul(mul(center[i],side_inv),number(0x3fe66666)));
    side[1]=gene[1];
    if(fesetenv(&env)||!isfinite(side[0])||!isfinite(side[2]))
        return stop(s,0x29352c,GH_ACTOR_MOTION_BOUNDS);
    uint32_t draw;
    if(gh_game_mt_next_0030f348(random,&draw))return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
    const gh_title_player_view *front=(draw&1u)?a:b,*flank=(draw&1u)?b:a;
    memcpy(front->bytes+0x580,center,12);
    memcpy(flank->bytes+0x580,side,12);
    enemy_decision(front->bytes,0,0x3b,0);
    enemy_decision(flank->bytes,0,0x3c,0);
    memset(init->enemies+0x5a0,0,6);
    *chosen=1;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status boss_attack_range_00263ad0(gh_enemy202_move *s,uint8_t *p,
    float distance,float height,const float actor[3],const float gene[3],unsigned heat,
    gh_game_mt random,int *chosen)
{
    gh_game_init_state *init=s->enemy->b.entry.init;
    const int em15=word(p+0x564)==0x215;
    const float timer=number(word(p+0x16e4)),paired=number(word(init->enemies+0x544));
    if(!isfinite(timer)||!isfinite(paired))
        return stop(s,0x263ad0,GH_ACTOR_MOTION_BOUNDS);
    if(word(p+0x6ec)||gh_enemy_timers_weapon_kind(&s->enemy->timers))
        return stop(s,0x263560,GH_ACTOR_MOTION_UNSUPPORTED);
    if(em15&&timer<=0.0f){
        put(p+0x16e4,0x44610000);
        put(init->enemies+0x544,heat==5?0x43960000:0x43520000);
    }
    if(heat>=2&&number(word(init->enemies+0x544))>0.0f){
        init->random=random;
        if(distance<25.0f){
            enemy_decision(p,0,distance<9.0f?0x0a:0x17,1);
            *chosen=1;
        }
        return GH_ACTOR_MOTION_OK;
    }
    if(!em15&&timer<=0.0f){
        const gh_actor_motion_status status=boss_pair_00293138(s,&random,chosen);
        if(status)return status;
        if(*chosen){
            put(p+0x16e4,0x44610000);
            init->random=random;
            return GH_ACTOR_MOTION_OK;
        }
    }
    const uint16_t gene_state=half(s->player->bytes+0x2f4);
    const int level_height=height<=number(0x3dcccccdu);
    if(em15&&distance>6.25f&&level_height&&(gene_state==0x0501||gene_state==0x0601))
        return commit_attack_0026ee40(s,p,random,heat,0x4b,100,0,chosen);

    uint32_t draw;
    const unsigned delay=word(p+0x16e0);
    if(heat>=2&&distance>9.0f&&distance<25.0f&&!delay){
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x263bb8,GH_ACTOR_MOTION_CALLEE);
        const float dx=sub(gene[0],actor[0]),dz=sub(gene[2],actor[2]);
        if(fesetenv(&env))return stop(s,0x263bb8,GH_ACTOR_MOTION_CALLEE);
        float yaw;
        uint32_t pc=0;
        if(gh_game_angle_001f8180(dx,dz,&yaw,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x1f8180,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_player_target peer;
        const gh_title_player_view *view=&s->enemy->entry.object.view;
        const gh_actor_motion_status status=enemy_peer_query(s,actor,yaw,number(0x3f060a92),view,&peer);
        if(status)return status;
        put(p+0x17a0,peer.handle);
        if(same_class_attack_peer(p,&peer))
            return commit_attack_0026ee40(s,p,random,heat,0x42,0,300,chosen);
    }
    if(distance<(heat<2?4.0f:6.25f)){
        if(gh_game_mt_next_0030f348(&random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        unsigned selector=0x4b;
        if((draw&7u)||!level_height){
            if(gh_game_mt_next_0030f348(&random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            selector=(draw&3u)||(em15&&!level_height)?0x15:em15?0x4b:0x16;
        }
        return commit_attack_0026ee40(s,p,random,heat,selector,100,0,chosen);
    }
    if(!delay&&heat>=2&&distance>9.0f&&distance<16.0f){
        if(gh_game_mt_next_0030f348(&random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16e0,(draw&1u)?30:300);
        if(!(draw&1u)){
            enemy_decision(p,0,0x17,0);
            *chosen=1;
        }
    }
    init->random=random;
    return GH_ACTOR_MOTION_OK;
}

/* dep0250's 264834 close choices and the 2663A8 long-range table. */
static gh_actor_motion_status r104_boss_attack(gh_enemy202_move *s,uint8_t *p,
 float distance,float player_turn,float height,unsigned heat,gh_game_mt random,int *chosen)
{
    gh_game_init_state *init=s->enemy->b.entry.init;
    uint32_t draw;
    if(distance<9.0f){
        if(gh_game_mt_next_0030f348(&random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(draw%3u==0&&!word(p+0x16e0)){
            if(gh_game_mt_next_0030f348(&random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(draw&1u)return commit_attack_0026ee40(s,p,random,heat,0x21,100,300,chosen);
        }
        if(gh_game_mt_next_0030f348(&random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint16_t player_state=half(s->player->bytes+0x2f4);
        int try_throw=!(draw&7u)||player_state==0x0501||player_state==0x0601;
        if(!try_throw&&player_state==0x0700){
            if(gh_game_mt_next_0030f348(&random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            try_throw=(draw&1u)!=0;
        }
        if(try_throw&&height<=number(0x3dcccccdu))
            return commit_attack_0026ee40(s,p,random,heat,0x4b,100,0,chosen);
        if(gh_game_mt_next_0030f348(&random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        return commit_attack_0026ee40(s,p,random,heat,draw%3u==1?0x16:0x15,100,0,chosen);
    }
    const uint32_t route=word(p+0x16d4);
    unsigned next=0;
    if(word(p+0x16e0)){
        init->random=random;
        return GH_ACTOR_MOTION_OK;
    }
    if(heat>1&&heat<6&&distance>12.25f&&distance<225.0f){
        if(gh_game_mt_next_0030f348(&random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if((int32_t)draw%3==0||(route&0x800u)){
            if(distance<20.25f){
                if(gh_game_mt_next_0030f348(&random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if((int32_t)draw%3==0)next=0x1e;
            }
            if(!next&&distance<=49.0f){
                if(gh_game_mt_next_0030f348(&random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(!(draw&1u))next=0x20;
            }
            if(!next)next=0x23;
        }else put(p+0x16e0,30);
    }
    if(!next&&ordered_abs(player_turn)<0.5235988f&&distance>36.0f&&distance<225.0f){
        if(gh_game_mt_next_0030f348(&random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if((int32_t)draw%3==0||(route&0x800u)){
            if(heat!=5){
                if(gh_game_mt_next_0030f348(&random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(draw&3u)next=0x1a;
            }
            if(!next){
                if(gh_game_mt_next_0030f348(&random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                next=(draw&1u)?0x23:0x19;
            }
        }else put(p+0x16e0,30);
    }
    init->random=random;
    if(next){
        put(p+0x16d4,route&~0x800u);
        put(p+0x16e0,300);
        enemy_decision(p,0,next,0);
        *chosen=1;
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status attack_in_range_002630f0(gh_enemy202_move *s,uint8_t *p,
    float distance,int *chosen)
{
    gh_enemy_actor *enemy=s->enemy;
    const uint8_t *work=(const uint8_t *)enemy->b.entry.work;
    float actor[3],player[3],target[3],turn;
    uint32_t pc=0,draw;
    if(!position(&enemy->entry.object.view,actor)||!position(s->player,player))
        return stop(s,0x2db6b0,GH_ACTOR_MOTION_BOUNDS);
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x2630f0,GH_ACTOR_MOTION_CALLEE);
    const float height=ordered_abs(sub(actor[1],player[1]));
    const float remaining=sub(number(word(p+0x171c)),number(word(p+0x5a8)));
    if(fesetenv(&env)||!isfinite(height)||!isfinite(remaining))
        return stop(s,0x2630f0,GH_ACTOR_MOTION_CALLEE);
    if(height>1.5f)return GH_ACTOR_MOTION_OK;
    memcpy(target,p+0x1690,12);
    if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
        number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    const unsigned heat=heat_category(s);
    if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
    const uint32_t route=word(p+0x16d4);
    const float face=number(word(p+0x760));
    if(!isfinite(face))return stop(s,0x268068,GH_ACTOR_MOTION_BOUNDS);
    gh_game_mt random=enemy->b.entry.init->random;
    if(face>=number(0x3fa78d36)&&(route&0x08000000u)){
        if(heat<5&&face>number(0x3fa78d36)){
            if(gh_game_mt_next_0030f348(&random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(draw&1u){
                enemy->b.entry.init->random=random;
                put(p+0x16d4,route|1u);
                enemy_decision(p,0,8,1);
                *chosen=1;
                return GH_ACTOR_MOTION_OK;
            }
        }
        if(p[0x17b9]>=2&&p[0x17b9]<4&&distance<6.25f){
            if(gh_game_mt_next_0030f348(&random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if((draw&3u)==1){
                gh_scene_ray_request ray={.mask=1,.segmented=1};
                float matrix[16];
                memcpy(matrix,p+0x80,sizeof(matrix));
                if(gh_scene_at_apply_world_001508d8(matrix,(const float[3]){0,0.5f,0},ray.from)||
                   gh_scene_at_apply_world_001508d8(matrix,(const float[3]){0,0.5f,-6},ray.to))
                    return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
                int hit;
                if(gh_scene_ray_query_00131e10(enemy->b.actors,&ray,&hit,&pc))
                    return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
                if(!hit){
                    enemy->b.entry.init->random=random;
                    enemy_decision(p,0,10,1);
                    *chosen=1;
                    return GH_ACTOR_MOTION_OK;
                }
            }
        }
    }
    static const float actor_limit[]={0,0.5235988f,1.0471976f,1.2217305f,1.5707964f,2.3561945f};
    static const float player_limit[]={0,1.0471976f,1.5707964f,2.3561945f,2.3561945f,3.1415927f};
    if(ordered_abs(turn)>actor_limit[heat]){
        enemy->b.entry.init->random=random;
        return GH_ACTOR_MOTION_OK;
    }
    if(r103_arena_boss(&enemy->entry))
        return boss_attack_range_00263ad0(s,p,distance,height,actor,player,heat,random,chosen);
    if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,player,actor,
        number(word(s->player->bytes+0x104)),number(0x40490fdb),&turn,&pc))
        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    if(gh_enemy203_is_r104_entry(&enemy->entry.request,enemy->b.entry.work->room))
        return r104_boss_attack(s,p,distance,turn,height,heat,random,chosen);
    if(ordered_abs(turn)>player_limit[heat]){
        enemy->b.entry.init->random=random;
        if(!word(work+0x56c)&&heat<3)return GH_ACTOR_MOTION_OK;
        put(p+0x171c,bits(remaining));
        if(remaining>0)return GH_ACTOR_MOTION_OK;
    }else put(p+0x171c,heat<3?0x44160000u:0x43960000u);

    const uint32_t dependency=word(p+0x564);
    const unsigned weapon_kind=gh_enemy_timers_weapon_kind(&enemy->timers);
    if(word(p+0x6ec)||weapon_kind){
        if(word(p+0x6ec))return stop(s,0x263560,GH_ACTOR_MOTION_UNSUPPORTED);
        const int stamp18=weapon_kind==0x363&&r100_armed_em02(enemy,p);
        const int r101=weapon_kind==0x361&&enemy->b.entry.work->room==0x101&&
            dependency==0x203&&half(p+0x640)==13&&!p[0x1650];
        /* Dependency 0200 underflows the armed table at 263698 and
         * selects the same default range at 2636FC. */
        const int r102=weapon_kind==0x363&&dependency==0x200&&
            gh_enemy_timers_room_weapon(&enemy->timers);
        if(!stamp18&&!r101&&!r102)return stop(s,0x263560,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned delay=stamp18?30:0;
        /* 263560's default armed branch draws only below 4.5 squared.
         * A failed draw still advances the canonical MT state. */
        if(distance<20.25f){
            if(gh_game_mt_next_0030f348(&random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!(draw&7u))return commit_attack_0026ee40(s,p,random,heat,0x16,100,delay,chosen);
        }
        if(distance<9.0f)return commit_attack_0026ee40(s,p,random,heat,0x47,100,delay,chosen);
        enemy->b.entry.init->random=random;
        return GH_ACTOR_MOTION_OK;
    }
    const int em27=dependency==0x227;
    if(dependency==0x200||em27){
        if(heat>1&&distance>9.0f&&distance<25.0f&&!word(p+0x16e0)){
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x263b24,GH_ACTOR_MOTION_CALLEE);
            const float dx=sub(player[0],actor[0]),dz=sub(player[2],actor[2]);
            if(fesetenv(&env))return stop(s,0x263b24,GH_ACTOR_MOTION_CALLEE);
            float yaw;
            if(gh_game_angle_001f8180(dx,dz,&yaw,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
                return stop(s,pc?pc:0x1f8180,GH_ACTOR_MOTION_UNSUPPORTED);
            gh_player_target peer;
            gh_actor_motion_status status=enemy_peer_query(s,actor,yaw,number(0x3f060a92),
                &enemy->entry.object.view,&peer);
            if(status)return status;
            put(p+0x17a0,peer.handle);
            if(same_class_attack_peer(p,&peer))
                return commit_attack_0026ee40(s,p,random,heat,0x42,em27?0:100,300,chosen);
        }
        if(distance<(heat<2?4.0f:6.25f)){
            if(gh_game_mt_next_0030f348(&random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            unsigned substate=0x15;
            if(!(draw&7u)&&height<=number(0x3dcccccdu))substate=0x4b;
            else if(em27){
                /* 263EC8's EM27 family draws again only after the 4B test fails. */
                if(gh_game_mt_next_0030f348(&random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(draw&1u)substate=0x16;
            }
            return commit_attack_0026ee40(s,p,random,heat,substate,100,0,chosen);
        }
        enemy->b.entry.init->random=random;
        return GH_ACTOR_MOTION_OK;
    }
    if(heat>1&&distance<4&&!word(p+0x16e0)){
        if(gh_game_mt_next_0030f348(&random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint32_t class_flags=word(p+0x474);
        if(!(draw&1u)&&(class_flags==1||class_flags==5||class_flags==8||class_flags==9))
            return commit_attack_0026ee40(s,p,random,heat,0x18,0,300,chosen);
    }
    if(distance<(heat<2?4.0f:9.0f)){
        if(gh_game_mt_next_0030f348(&random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned substate=!(draw&7u)&&height<=.1f?0x4b:0x15;
        return commit_attack_0026ee40(s,p,random,heat,substate,100,0,chosen);
    }
    enemy->b.entry.init->random=random;
    return GH_ACTOR_MOTION_OK;
}


static gh_actor_motion_status attack_choice_common(gh_enemy202_move *s,uint8_t *p,
    float distance,int *chosen)
{
    gh_enemy_actor *enemy=s->enemy;
    const uint8_t *work=(const uint8_t *)enemy->b.entry.work;
    const uint8_t *manager=enemy->b.entry.init->enemies;
    const uint32_t behavior=word(p+0x16d0),route=word(p+0x16d4);
    const float escape=number(word(p+0x16c0)),override=number(word(p+0x1734));
    const float drain=number(word(p+0x770)),special=number(word(p+0x17cc));
    if(!isfinite(escape)||!isfinite(override)||!isfinite(drain)||!isfinite(special))
        return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(escape>0)enemy_decision(p,0,0x13,0);
    else if(override>0)enemy_decision(p,0,0x89,0);
    else if((int32_t)word(manager+0x530)>0)
        return stop(s,0x26e258,GH_ACTOR_MOTION_UNSUPPORTED);
    else{
        gh_actor_motion_status status=enemy_home_range_0026e588(s,p,distance,chosen);
        if(status||*chosen)return status;
        if(drain>0){
            enemy_decision(p,0,0x36,0);
            *chosen=1;
            return GH_ACTOR_MOTION_OK;
        }
        if((int16_t)half(s->player->bytes+0x54a)<=0||(word(work+0x5e4)&0x01000000u)||manager[0x5b5])
            return GH_ACTOR_MOTION_OK;
        if(manager[0x53d])enemy_decision(p,0,distance<2.25f?7:0x6c,distance<2.25f?1:0);
        else if(distance<16.0f&&!(route&0x40000000u)&&
            (s->player->size<0x69c||!s->player_aux_698_known||word(s->player->bytes+0x698)))
            return stop(s,0x262cdc,GH_ACTOR_MOTION_UNSUPPORTED);
        else if(distance<16.0f&&!(route&0x40000000u)&&s->player_aux_698&&
            half(s->player_aux_698->bytes+0x2fe)==0x380){
            put(p+0x16d4,route|0x40000000u);
            enemy_decision(p,0,0x6c,1);
        }else{
            if(manager[0x53c])return GH_ACTOR_MOTION_OK;
            if(special>0)return stop(s,0x270c78,GH_ACTOR_MOTION_UNSUPPORTED);
            if(distance<25.0f&&((word(work+0x210)^word(p+0x17d0))&7u)==0&&
                !word(p+0x6ec)&&!gh_enemy_timers_weapon_kind(&enemy->timers)&&!word(p+0x6f0)){
                gh_actor_motion_status status=enemy_periodic_pickup_00272588(s,p,chosen);
                if(status||*chosen)return status;
            }
            if((behavior&2u)&&distance<49.0f&&manager[0x53b])enemy_decision(p,0,0x6c,0);
            else if(p[0x1864]||word(p+0x16dc)||(int32_t)word(manager+0x514)>0||
                (!word(p+0x16ec)&&(int32_t)word(manager+0x510)>0)||!(behavior&0x200000u))
                return GH_ACTOR_MOTION_OK;
            else return attack_in_range_002630f0(s,p,distance,chosen);
        }
    }
    *chosen=1;
    return GH_ACTOR_MOTION_OK;
}

static int em03_approach_range(const gh_enemy202_move *s,const uint8_t *p,float distance)
{
    const unsigned room=s->enemy->b.entry.work->room,stamp=half(p+0x640);
    return word(p+0x564)==0x203&&isfinite(distance)&&distance>=0&&distance<=144.0f&&
        ((room==0x100&&(stamp==19||(stamp==11&&p[0x1650]==5&&word(p+0x16c4)==6)))||
         (room==0x101&&stamp==13&&p[0x1650]==0)||r103_arena_em03(&s->enemy->entry)||
         r102_em03_stamp8(s->enemy->b.entry.work,p));
}

static gh_actor_motion_status em0a_attack_choice_002634d0(gh_enemy202_move *s,
    uint8_t *p,float distance,unsigned heat,int *chosen)
{
    gh_enemy_actor *enemy=s->enemy;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    if(((half(p+0x640)||p[0x1650]!=3)&&!r102_gate_em0a(enemy->b.entry.work,p)&&!r103_arena_em0a(&enemy->entry))||
       word(p+0x6ec)||gh_enemy_timers_weapon_kind(&enemy->timers))
        return stop(s,0x2634d0,GH_ACTOR_MOTION_UNSUPPORTED);
    float from[3],target[3],angle;
    uint32_t pc=0;
    if(!position(s->player,from)||!position(actor,target)||
       gh_player_target_angle_002da038(s->elf,s->elf_bytes,from,target,
        number(word(s->player->bytes+0x104)),number(0x40490fdb),&angle,&pc))
        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    angle=ordered_abs(angle);
    const float tolerance=heat==1?number(0x3f860a92):heat==2?number(0x3fc90fdb):
        heat<5?number(0x4016cbe4):number(0x40490fdb);
    if(angle>tolerance){
        if(!enemy->entry.b.work->language&&heat<3)return GH_ACTOR_MOTION_OK;
        fenv_t env;
        const float timer=number(word(p+0x171c)),rate=number(word(p+0x5a8));
        if(!isfinite(timer)||!isfinite(rate)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x26356c,GH_ACTOR_MOTION_UNSUPPORTED);
        const float next=sub(timer,rate);
        if(fesetenv(&env)||!isfinite(next))return stop(s,0x26356c,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x171c,bits(next));
        if(next>0)return GH_ACTOR_MOTION_OK;
    }else put(p+0x171c,heat<3?0x44160000u:0x43960000u);
    if(distance<(heat<2?4.0f:6.25f)){
        float height;
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x270d00,GH_ACTOR_MOTION_UNSUPPORTED);
        height=ordered_abs(sub(from[1],target[1]));
        if(fesetenv(&env)||!isfinite(height))
            return stop(s,0x270d00,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_game_mt random=enemy->b.entry.init->random;
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned attack=(!(draw&7u)&&height<=number(0x3dcccccdu))?0x4b:0x15;
        return commit_attack_0026ee40(s,p,random,heat,attack,100,0,chosen);
    }
    if(word(p+0x16e0)||angle>=number(0x3f060a92)||distance<=36||distance>=225)
        return GH_ACTOR_MOTION_OK;
    uint32_t draw;
    if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
    int32_t choice;
    memcpy(&choice,&draw,4);
    const uint32_t route=word(p+0x16d4);
    if(choice%3&&!(route&0x800u)){
        put(p+0x16e0,30);
        return GH_ACTOR_MOTION_OK;
    }
    if(heat<5){
        if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
    }else draw=0;
    put(p+0x16d4,route&~0x800u);
    put(p+0x16e0,300);
    enemy_decision(p,0,(draw&3u)?0x1a:0x19,0);
    *chosen=1;
    return GH_ACTOR_MOTION_OK;
}

static int r101_em03_stamp13(unsigned room,const uint8_t *p)
{
    return room==0x101&&word(p+0x564)==0x203&&half(p+0x640)==13&&!p[0x1650];
}

static gh_actor_motion_status attack_choice_00262aa8(gh_enemy202_move *s,uint8_t *p,int *chosen)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_game_init_state *init=enemy->b.entry.init;
    const uint8_t *work=(const uint8_t*)enemy->b.entry.work,*manager=init->enemies;
    const float distance=number(word(p+0x618)),face=number(word(p+0x760));
    const uint32_t route=word(p+0x16d4);
    *chosen=0;
    const unsigned room=enemy->b.entry.work->room;
    const int side_arena=r101_arena_airborne(enemy);
    const int em00=word(p+0x564)==0x200&&(room==0x100||room==0x102||r101_em00_stamp12(room,p)||
        r101_em00_stamp15(room,p)||r103_arena_em00(&enemy->entry)||side_arena);
    const int ordinary=em00&&
        (distance<=144.0f||!(word(p+0x16d0)&0x200000u));
    const int r101_box=r101_scenery_enemy(enemy->b.entry.work,p);
    const int r101_gate=r101_gate_exchanged_enemy(enemy->b.entry.work,p);
    /* The gate actors keep their approach route after the override timer
     * expires. 262AA8 then proceeds to the ordinary attack checks. */
    const uint32_t home_mode=word(p+0x16c4);
    const int area11_em02=(home_mode==2||home_mode==6)&&r102_at0b_em02(enemy->b.entry.work,p);
    const int east_em27=r102_east_em27(enemy->b.entry.work,p);
    const int gate_reinforcement=(home_mode==2||home_mode==6)&&r102_gate_actor(enemy->b.entry.work,p);
    /* The free AT02/AT03 actors share the ordinary 264204 selector. */
    const uint32_t dependency=word(p+0x564);
    const int side_em03=side_arena&&dependency==0x203;
    const gh_boot_work *room_work=enemy->b.entry.work;
    const int free_r100_enemy=r100_at2_enemy(room_work,p)||r100_at3_enemy(room_work,p);
    const int r100_formation=distance<=144.0f&&(r100_at2_formation(room_work,p)||free_r100_enemy);
    const int r104=gh_enemy203_is_r104_entry(&enemy->entry.request,room);
    if(r104||r103_arena_boss(&enemy->entry)||em03_approach_range(s,p,distance)||ordinary||r101_em02_stamp5(room,p)||r101_em03_stamp0(room,p)||r101_box||r100_formation||
       ((area11_em02||gate_reinforcement||east_em27||r101_gate||side_em03)&&distance<=144.0f)||
       (r100_armed_em02(enemy,p)&&distance<=144.0f)||
       r100_stamp18_armed_approach(enemy,p)||r105_opening_enemy(enemy->b.entry.work,p)||
       r105_gate_approach(enemy->b.entry.work,p)){
        if(!isfinite(distance)||distance<0||!isfinite(face))
            return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
        return attack_choice_common(s,p,distance,chosen);
    }
    /* The real R100 EM02 flank uses the common home-range path. Its
     * attack bit is clear at 26314C, before the dependency dispatch. */
    const int far_em02=dependency==0x202&&half(p+0x640)==4&&p[0x1650]==2&&
        distance>144.0f&&!(word(p+0x16d0)&0x200000u);
    if((area11_em02||gate_reinforcement)&&number(word(p+0x16c0))>0.0f){
        /* The original 262AF8 timer decision returns before the dependency selector. */
        enemy_decision(p,0,0x13,0);
        *chosen=1;
        return GH_ACTOR_MOTION_OK;
    }
    /* Original EM02 and EM03 share the 264204 range selector, which
     * returns without an attack outside the highest measured range. */
    const int r102_far=(area11_em02||gate_reinforcement||east_em27||
        r102_route_runner(enemy->b.entry.work,p))&&distance>144.0f;
    const int arena_em03_far=distance>144.0f&&(r103_arena_em03(&enemy->entry)||side_em03);
    if(em00||r102_far||r100_far_stamp18(enemy,p,distance)||arena_em03_far||(r101_gate&&distance>144.0f)||
       r101_em03_stamp13(room,p)||
       (room==0x100&&(dependency==0x203||far_em02||(free_r100_enemy&&distance>144.0f))))
        return attack_choice_ordinary_far(s,p,distance,face,chosen);
    const int gate_em0a=r102_gate_em0a(enemy->b.entry.work,p);
    const int arena_em0a=r103_arena_em0a(&enemy->entry);
    if(dependency!=0x20a||(room!=0x100&&!gate_em0a&&!arena_em0a))
        return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(s->substate67_stage==52||s->substate67_stage==61)return stop(s,0x273c38,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!isfinite(distance)||!isfinite(face))return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(number(word(p+0x16c0))>0||number(word(p+0x1734))>0){
        enemy_decision(p,0,number(word(p+0x16c0))>0?0x13:0x89,0);
        *chosen=1;
        return GH_ACTOR_MOTION_OK;
    }
    if((int32_t)word(manager+0x530)>0)return stop(s,0x26e258,GH_ACTOR_MOTION_UNSUPPORTED);
    if(distance>=16&&!(word(p+0x1644)&0x40000000u)&&(int32_t)word(p+0x16ec)<1){
        if((half(p+0x640)||p[0x1650]!=3)&&!gate_em0a&&!arena_em0a)
            return stop(s,0x26e588,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=enemy_home_range_0026e588(s,p,distance,chosen);
        if(status||*chosen)return status;
    }
    if(number(word(p+0x770))>0)enemy_decision(p,0,0x36,0);
    else{
        if((int16_t)half(s->player->bytes+0x54a)<=0||(word(work+0x5e4)&0x01000000u)||manager[0x5b5])
            return GH_ACTOR_MOTION_OK;
        if(manager[0x53d])enemy_decision(p,0,distance<2.25f?7:0x6c,distance<2.25f?1:0);
        else{
            if(distance<16&&!(route&0x40000000u)){
                if(s->player->size<0x69c||!s->player_aux_698_known||word(s->player->bytes+0x698))
                    return stop(s,0x262cdc,GH_ACTOR_MOTION_UNSUPPORTED);
                if(s->player_aux_698&&half(s->player_aux_698->bytes+0x2fe)==0x380){
                    put(p+0x16d4,route|0x40000000u);
                    enemy_decision(p,0,0x6c,1);
                    *chosen=1;
                    return GH_ACTOR_MOTION_OK;
                }
            }
            if(manager[0x53c])return GH_ACTOR_MOTION_OK;
            if(number(word(p+0x17cc))>0)enemy_decision(p,1,0x0f,0);
            else if((word(p+0x16d0)&2u)&&distance<49&&manager[0x53b])enemy_decision(p,0,0x6c,0);
            else{
                if(p[0x1864]||word(p+0x16dc)||(int32_t)word(manager+0x514)>0||
                   (!word(p+0x16ec)&&(int32_t)word(manager+0x510)>0)||
                   !(word(p+0x16d0)&0x200000u))return GH_ACTOR_MOTION_OK;
                float actor[3],target[3],turn;
                uint32_t pc=0;
                if(!position(&enemy->entry.object.view,actor)||!position(s->player,target))
                    return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
                fenv_t env;
                if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x2630f0,GH_ACTOR_MOTION_CALLEE);
                float height=fabsf(sub(actor[1],target[1]));
                if(fesetenv(&env))return stop(s,0x2630f0,GH_ACTOR_MOTION_CALLEE);
                if(height>1.5f)return GH_ACTOR_MOTION_OK;
                memcpy(target,p+0x1690,12);
                if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                    number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
                const unsigned heat=heat_category(s);
                if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
                if(fabsf(turn)>2.3561945f&&distance<6.25f){
                    enemy_decision(p,0,0x44,0);
                    *chosen=1;
                    return GH_ACTOR_MOTION_OK;
                }
                if(face>=1.3089969f&&(route&0x08000000u)){
                    uint32_t draw=0;
                    if(heat<5&&face>1.3089969f){
                        if(gh_game_mt_next_0030f348(&init->random,&draw))return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                        if(draw&1u){
                            put(p+0x16d4,route|1u);
                            enemy_decision(p,0,8,1);
                            *chosen=1;
                            return GH_ACTOR_MOTION_OK;
                        }
                    }
                    if(p[0x17b9]>=2&&p[0x17b9]<4&&distance<6.25f){
                        if(gh_game_mt_next_0030f348(&init->random,&draw))return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                        if((draw&3u)==1){s->substate67_stage=52;return stop(s,0x273c38,GH_ACTOR_MOTION_UNSUPPORTED);}
                    }
                    if(distance<=4&&!(route&0x10380u)){
                        enemy_decision(p,0,0x45,0);
                        *chosen=1;
                        return GH_ACTOR_MOTION_OK;
                    }
                    /* EM0A's jump-table slot enters 263214. When 268068
                     * returns zero, the original continues at 263464. */
                }
                static const float angular_limits[]={0,0.5235988f,1.0471976f,1.2217305f,1.5707964f,2.3561945f};
                if(fabsf(turn)>angular_limits[heat])return GH_ACTOR_MOTION_OK;
                return em0a_attack_choice_002634d0(s,p,distance,heat,chosen);
            }
        }
    }
    *chosen=1;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status enemy_door_ray_0024bb68(gh_enemy202_move *s,uint8_t *p,int *selected);

static gh_actor_motion_status continue_ordinary_00212670(gh_enemy202_move *s,uint8_t *p,float distance)
{
    gh_enemy_actor *enemy=s->enemy;
    const uint32_t rank=word(p+0x16d8),route=word(p+0x16d4);
    const float heading=number(word(p+0x768));
    if(!isfinite(heading))return stop(s,0x212670,GH_ACTOR_MOTION_UNSUPPORTED);
    if((route&0x08000000u)&&rank<3&&distance<16.0f&&heading>=number(0x3f860a92)){
        const float signed_heading=number(word(p+0x764));
        if(!isfinite(signed_heading))return stop(s,0x2126d4,GH_ACTOR_MOTION_UNSUPPORTED);
        enemy_decision(p,0,9,signed_heading>0?1:0);
        return GH_ACTOR_MOTION_OK;
    }
    const unsigned heat=heat_category(s);
    if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
    const unsigned close_rank=heat==1?1:heat==2?2:3;
    float radius=rank>5?144.0f:rank>=3?64.0f:rank<close_rank?number(0x404f5c28):25.0f;
    if(rank<3){
        put(p+0x5f4,0);
        if(heading>=number(0x3f860a92))radius=0;
    }
    if(distance>radius&&!word(p+0x5f4)){
        float actor[3],target[3],turn;
        uint32_t pc=0;
        memcpy(target,p+0x16a0,12);
        if(!position(&enemy->entry.object.view,actor)||
            gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
            return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(ordered_abs(turn)>number(0x3fc90fdb)){
            put(p+0x16d4,route|1u);
            enemy_decision(p,0,8,0);
        }else{
            const int r104=gh_enemy203_is_r104_entry(&enemy->entry.request,enemy->b.entry.work->room);
            unsigned next=r104&&distance>64.0f?0xb:6;
            if(distance<=36.0f){
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(!(draw&1u)&&(int32_t)word(p+0x16ec)<1&&!(route&0x04000000u))next=5;
            }
            enemy_decision(p,0,next,0);
        }
        return GH_ACTOR_MOTION_OK;
    }
    if(distance>radius)put(p+0x5f4,word(p+0x5f4)-1);
    else{
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x5f4,60u+20u*(draw%10u));
    }
    if(distance<(rank<(heat==5?100u:close_rank)?0.0f:9.0f)){
        enemy_decision(p,0,7,0);
        return GH_ACTOR_MOTION_OK;
    }
    s->substate67_stage=101;
    return GH_ACTOR_MOTION_OK;
}

/* Devil 0220 within the rank >= 3 idle radius. Attack choice at 262AA8
 * consumes the rear reaction and melee or ranged draws before 212670. */
static gh_actor_motion_status devil_near_choice_00262aa8(gh_enemy202_move *s,uint8_t *p,
    int idle_caller,int *chosen)
{
    *chosen=0;
    const gh_enemy_entry *entry=s->runtime.entry;
    const gh_boot_work *work=entry->b.work;
    gh_game_init_state *init=entry->b.init;
    const uint8_t *manager=init->enemies;
    const float distance=number(word(p+0x618));
    const uint32_t route=word(p+0x16d4),behavior=word(p+0x16d0);
    const float behind=number(word(p+0x760)),face=number(word(p+0x768));
    const float guard=number(word(p+0x17cc));
    if(work->room!=0x100||word(p+0x564)!=0x220||
        !isfinite(distance)||distance<0||!isfinite(behind)||!isfinite(face)||!isfinite(guard)||
        word(p+0x6ec)||word(p+0x16c0)||word(p+0x1734)||word(p+0x770)||
        manager[0x53b]||manager[0x53d]||manager[0x5a5]||manager[0x5b5]||
        (int32_t)word(manager+0x530)>0||(word((const uint8_t*)work+0x5e4)&0x01000000u)||
        s->player->size<0x69c||!s->player_aux_698_known||s->player_aux_698||word(s->player->bytes+0x698))
        return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
    /* 262CA4 calls the signed 293100 grab-counter predicate before attack draws. */
    if((int8_t)manager[0x53c]>0)return GH_ACTOR_MOTION_OK;
    if(manager[0x53c])return stop(s,0x293100,GH_ACTOR_MOTION_UNSUPPORTED);
    if(guard>0)return stop(s,0x270c78,GH_ACTOR_MOTION_UNSUPPORTED);
    /* 212670's rank >= 3 branch yields a 64 or 144 squared approach
     * radius. Its retreat threshold is at most 9 in this distance band. */
    const unsigned rank=word(p+0x16d8);
    if(idle_caller&&rank<3&&distance<=144.0f)
        return stop(s,0x212670,GH_ACTOR_MOTION_UNSUPPORTED);

    const int melee_distance=distance<9.0f;
    /* 272588 returns zero for dependency 0220 before searching for a weapon. */

    gh_game_mt random=init->random;
    uint32_t draw,delay=0,next_route=route;
    if((behavior&0x200000u)&&!p[0x1864]&&!word(p+0x16dc)&&
        (int32_t)word(manager+0x514)<=0&&(word(p+0x16ec)||(int32_t)word(manager+0x510)<=0)){
        float actor[3],player[3],target[3],turn;
        if(!position(&entry->object.view,actor)||!position(s->player,player))
            return stop(s,0x2db6b0,GH_ACTOR_MOTION_BOUNDS);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2630f0,GH_ACTOR_MOTION_CALLEE);
        const float height=ordered_abs(sub(actor[1],player[1]));
        if(fesetenv(&env)||!isfinite(height))
            return stop(s,0x2630f0,GH_ACTOR_MOTION_CALLEE);

        if(height<=1.5f){
            const unsigned category=heat_category(s);
            if(!category)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            memcpy(target,p+0x1690,12);
            uint32_t pc=0;
            if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
                return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);

            if((route&0x08000000u)&&behind>=1.3089969f){
                draw=0;
                if(category<5&&gh_game_mt_next_0030f348(&random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                /* 26826C can still consume the second draw. 273C38
                 * rejects EM20's backward evade before making a ray. */
                if(!(draw&1u)&&p[0x17b9]>=2&&p[0x17b9]<4&&distance<6.25f&&
                   gh_game_mt_next_0030f348(&random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            }

            static const float actor_limit[]={0,0.5235988f,1.0471976f,1.2217305f,1.5707964f,2.3561945f};
            if(melee_distance&&ordered_abs(turn)<=actor_limit[category]){
                unsigned selector=0x15;
                if(category>=2){
                    int32_t roll;
                    if(gh_game_mt_next_0030f348(&random,&draw))
                        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                    memcpy(&roll,&draw,sizeof(roll));
                    if(roll%5==3)selector=0x16;
                }
                init->random=random;
                enemy_decision(p,0,selector,0);
                set_attack_timers_0026ee40(init,p,category,100);
                *chosen=1;
                return GH_ACTOR_MOTION_OK;
            }
            if(distance>16&&distance<225&&!word(p+0x16e0)&&ordered_abs(turn)<=actor_limit[category]){
                if(gh_game_mt_next_0030f348(&random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(!(draw&3u)||(route&0x800u)){
                    next_route=route&~0x800u;
                    if(category!=1){
                        init->random=random;
                        put(p+0x16d4,next_route);
                        enemy_decision(p,0,0x23,2);
                        set_attack_timers_0026ee40(init,p,category,100);
                        *chosen=1;
                        return GH_ACTOR_MOTION_OK;
                    }
                }
                delay=30;
            }
        }
    }
    const float idle_radius=rank>5?144.0f:64.0f;
    if(idle_caller&&distance<=idle_radius&&gh_game_mt_next_0030f348(&random,&draw))
        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
    init->random=random;
    if(delay)put(p+0x16e0,delay);
    if(next_route!=route)put(p+0x16d4,next_route);
    if(!idle_caller)return GH_ACTOR_MOTION_OK;

    if(distance>idle_radius){
        /* 212864 decrements only a running clock. Without a clock, 212890
         * starts the Devil approach, variant zero. Neither path draws. */
        const uint32_t timer=word(p+0x5f4);
        if(rank>=3&&timer)put(p+0x5f4,timer-1);
        else{
            if(rank<3)put(p+0x5f4,0);
            enemy_decision(p,0,0x12,0);
            *chosen=1;
        }
        return GH_ACTOR_MOTION_OK;
    }
    int32_t signed_draw;
    memcpy(&signed_draw,&draw,sizeof(signed_draw));
    put(p+0x5f4,60+20*(signed_draw%10));
    if(melee_distance&&(heat_category(s)<5||word(p+0x16d8)>=100)){
        /* Original 212A50 retreats with variant 1 inside the rank's
         * squared 9-unit threshold when the attack choice returned zero. */
        enemy_decision(p,0,0x12,1);
        *chosen=1;
    }
    return GH_ACTOR_MOTION_OK;
}

/* 262AA8's original early decisions for dependency 0220 on the R100
 * far path. 212670 chooses the Devil's approach selector after a zero return. */
static gh_actor_motion_status devil_far_idle_00262aa8(gh_enemy202_move *s,uint8_t *p)
{
    const gh_enemy_entry *entry=s->runtime.entry;
    const gh_boot_work *work=entry->b.work;
    const uint8_t *manager=entry->b.init->enemies;
    const float distance=number(word(p+0x618));
    const float go=number(word(p+0x16c0)),avoid=number(word(p+0x1734));
    const float hold=number(word(p+0x770)),guard=number(word(p+0x17cc));
    if(work->room!=0x100||word(p+0x564)!=0x220||!isfinite(distance)||distance<=144.0f||
        !isfinite(go)||!isfinite(avoid)||!isfinite(hold)||!isfinite(guard))
        return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(go>0)enemy_decision(p,0,0x13,0);
    else if(avoid>0)enemy_decision(p,0,0x89,0);
    else if(hold>0)enemy_decision(p,0,0x36,0);
    else if(!(word((const uint8_t *)work+0x5e4)&0x01000000u)){
        if(manager[0x53d])enemy_decision(p,0,0x6c,0);
        else{
            if(guard>0)return stop(s,0x270c78,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!p[0x1864]&&!word(p+0x16dc)&&(int32_t)word(manager+0x514)<=0&&
                (word(p+0x16ec)||(int32_t)word(manager+0x510)<=0)&&
                (word(p+0x16d0)&0x200000u)){
                int chosen;
                const gh_actor_motion_status status=devil_near_choice_00262aa8(s,p,1,&chosen);
                if(status)return status;
                if(!chosen)s->substate67_stage=54;
                return GH_ACTOR_MOTION_OK;
            }
            const unsigned rank=word(p+0x16d8);
            const uint32_t timer=word(p+0x5f4);
            if(rank>=3&&timer){
                put(p+0x5f4,timer-1);
                s->substate67_stage=54;
                return GH_ACTOR_MOTION_OK;
            }
            if(rank<3)put(p+0x5f4,0);
            enemy_decision(p,0,0x12,0);
        }
    }else{
        /* 262AA8 returns zero while Work inhibits attacks. The movement
         * decision still runs in the original caller. */
        return stop(s,0x212670,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status enemy_door_jump_0024bda0(gh_enemy202_move *s,uint8_t *p)
{
    const gh_boot_work *work=s->enemy->b.entry.work;
    /* Original 24BDA0 excludes R100 before querying the room AT manager. */
    if(work->room==0x100||((word((const uint8_t *)work+0x210)^word(p+0x17d0))&7u)!=0)
        return GH_ACTOR_MOTION_OK;

    float origin[3],target[3],turn;
    memcpy(origin,p+0x490,12);
    memcpy(target,p+0x16a0,12);
    const float heading=number(word(p+0x104));
    uint32_t pc=0;
    if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,origin,target,
        heading,number(0x40490fdb),&turn,&pc))
        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    if(ordered_abs(turn)>number(0x3f060a92))return GH_ACTOR_MOTION_OK;

    const gh_title_player_view *actor=&s->enemy->entry.object.view;
    if(!s->scene_jump_at||!s->scene_jump_room.bytes||!position(actor,origin))
        return stop(s,0x2c2b58,GH_ACTOR_MOTION_UNSUPPORTED);
    float yaw;
    int found;
    if(gh_scene_at_find_jump_002c2b58(s->scene_jump_at,&s->scene_jump_room,(uint16_t)work->room,
        origin,target,&yaw,&found,&pc))return stop(s,pc?pc:0x2c2b58,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!found)return GH_ACTOR_MOTION_OK;

    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x2da1f8,GH_ACTOR_MOTION_CALLEE);
    float delta=sub(yaw,heading);
    const float pi=number(0x40490fdb),tau=number(0x40c90fdb);
    if(delta>pi)delta=sub(delta,tau);
    if(delta<-pi)delta=add(delta,tau);
    if(fesetenv(&env)||!isfinite(delta))return stop(s,0x2da1f8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(ordered_abs(delta)<=number(0x3f490fdb)){
        memcpy(p+0x6d0,target,12);
        put(p+0x6e0,bits(yaw));
        enemy_decision(p,0,0x74,0);
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_substate0_00211c08(gh_enemy202_move *s,uint8_t *p)
{
    const gh_enemy_move_actor *actor_state=&s->runtime;
    gh_actor_motion *motion=actor_state->motion;
    const uint32_t dependency=word(p+0x564);
    const gh_boot_work *room_work=actor_state->entry->b.work;
    const int r102_em00=room_work->room==0x102&&dependency==0x200;
    const int r104_boss=gh_enemy203_is_r104_entry(&actor_state->entry->request,room_work->room);
    const int player_alive=(int16_t)half(s->player->bytes+0x54a)>0;
    const unsigned held=gh_enemy_timers_weapon_kind(actor_state->timers);
    uint32_t pc=0;
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        switch(p[0x2f6]){
        case 0:
            put(p+0x16d0,(word(p+0x16d0)&~0x8000u)|0x402u);
            s->substate67_stage=40;
            break;
        case 1:s->substate67_stage=41;break;
        case 2:s->substate67_stage=47;break;
        case 3:s->substate67_stage=48;break;
        default:s->substate67_stage=46;break;
        }
    }
    if(s->substate67_stage==40||s->substate67_stage==47){
        const int turning=s->substate67_stage==47;
        size_t offset;
        switch(dependency){
        case 0x227:
            if(!r105_exchanged_enemy(room_work,p)&&
               !r101_scenery_enemy(room_work,p)&&!r101_gate_exchanged_enemy(room_work,p)&&!r102_east_em27(room_work,p))
                return stop(s,0x211c08,GH_ACTOR_MOTION_UNSUPPORTED);
            /* EM27 uses the default original clip pair. */
            /* fall through */
        case 0x200:offset=0x98;break;
        case 0x202:case 0x203:offset=0x648;break;
        case 0x20a:offset=0x828;break;
        case 0x214:case 0x215:
            if(!r103_arena_boss(actor_state->entry))return stop(s,0x211c08,GH_ACTOR_MOTION_UNSUPPORTED);
            offset=0x10a8;
            break;
        case 0x220:offset=0x1bc0;break;
        case 0x250:
            if(!r104_boss||held)return stop(s,0x211c08,GH_ACTOR_MOTION_UNSUPPORTED);
            offset=0x18f0;
            break;
        default:return stop(s,0x211c08,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(turning){
            const int em02_turn=dependency==0x202||dependency==0x203;
            const int boss_turn=r103_arena_boss(actor_state->entry);
            if(dependency!=0x20a&&dependency!=0x200&&!em02_turn&&dependency!=0x220&&
               dependency!=0x227&&!boss_turn&&!r104_boss)
                return stop(s,0x2120e8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!s->substate67_motion.active){
                float actor[3],target[3],turn;
                if(!position(&actor_state->entry->object.view,actor)||!position(s->player,target)||
                    gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                        number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            /* This dependency selects the same clip for either turn sign. */
            offset=dependency==0x200||dependency==0x227?0xb0:em02_turn?0x650:dependency==0x220?0x1bc8:
                r104_boss?0x18f8:boss_turn?0x10b0:0x830;
        }
        if(word(p+0x6ec))return stop(s,turning?0x21251c:0x211ee8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(held)offset=turning?0x454:0x44c;
        const gh_object_view *resource=&actor_state->entry->resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            if(!resource->bytes||!resource->file_extent_known||
                resource->file_extent<offset+8||resource->file_extent>resource->size)
                return stop(s,0x211ee8,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+offset),word(resource->bytes+offset+4),
                0,10,p[0x17c3]?2:0,!r105_opening_enemy(actor_state->entry->b.work,p));
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        ++p[0x2f6];
        s->substate67_stage=turning?48:41;
    }
    if(s->substate67_stage>=48&&s->substate67_stage<=50){
        float actor[3],target[3],turn;
        if(!position(&actor_state->entry->object.view,actor)||!position(s->player,target))
            return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(s->substate67_stage==48){
            fenv_t env;
            const float yaw=number(word(p+0x104)),actor_rate=number(word(p+0x5a8));
            const float turn_rate=number(word((const uint8_t*)actor_state->entry->b.work+0x55c));
            if(!isfinite(turn_rate)||turn_rate<0||!isfinite(actor_rate)||actor_rate<0||
                !set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float cap=mul(mul(actor_rate,0.09817477f),turn_rate);
            gh_player_target_status status=gh_player_target_angle_002da038(
                s->elf,s->elf_bytes,actor,target,yaw,cap,&turn,&pc);
            float next=0;
            if(!status)next=add(yaw,turn);
            if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x104,bits(next));
            s->substate67_stage=49;
        }
        if(s->substate67_stage==49){
            if(!actor_state->bindings->canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
            int ended;
            gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
                *actor_state->bindings->canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
            if(status)return stop(s,pc?pc:0x157c18,status);
            s->substate67_stage=50;
        }
        if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
            number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
            return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(fabsf(turn)<0.08726646f)p[0x2f6]=0;
        s->substate67_stage=46;
    }

    if(s->substate67_stage==41){
        int ended=0;
        if(!actor_state->bindings->canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *actor_state->bindings->canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended&&player_alive&&dependency==0x20a?42:43;
    }
    if(s->substate67_stage==42){
        uint32_t draw;
        if(word(p+0x6ec))return stop(s,0x211fb0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_game_mt_next_0030f348(&actor_state->entry->b.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if((draw&1u)&&!held){
            p[0x2f4]=0;p[0x2f5]=0x67;p[0x2f6]=0;p[0x2f7]=0;
        }
        s->substate67_stage=43;
    }
    if(s->substate67_stage==43){
        if(gh_player_root_rotation_0014f9f0(&actor_state->entry->object.view,actor_state->entry->b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=44;
    }
    if(s->substate67_stage==44){
        if(gh_player_root_translation_0014f928(&actor_state->entry->object.view,actor_state->entry->b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=45;
    }
    if(s->substate67_stage==45){
        float actor[3],target[3],turn;
        if(!position(&actor_state->entry->object.view,actor)||!position(s->player,target)||
            gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
            return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(fabsf(turn)>0.34906584f)p[0x2f6]=2;
        else{
            const uint8_t *manager=actor_state->entry->b.init->enemies;
            const float distance=number(word(p+0x618));
            if(manager[0x53d]&&distance>9){
                p[0x2f7]=1;p[0x2f5]=0x6c;p[0x2f4]=0;p[0x2f6]=0;
                s->substate67_stage=5;
            }else if(manager[0x53c]&&distance<16){
                p[0x2f5]=7;p[0x2f7]=1;p[0x2f4]=0;p[0x2f6]=0;
            }
        }
        if(s->substate67_stage==45)s->substate67_stage=46;
    }
    if(s->substate67_stage==53)return stop(s,0x26341c,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->enemy&&(s->substate67_stage==46||s->substate67_stage==52)){
        const uint8_t *manager=actor_state->entry->b.init->enemies;
        if(player_alive&&!manager[0x5b5]&&!manager[0x53c]){
            const float distance=number(word(p+0x618));
            if(distance>=0&&distance<=144){
                int chosen;
                gh_actor_motion_status status=devil_near_choice_00262aa8(s,p,1,&chosen);
                if(status)return status;
                if(!chosen)s->substate67_stage=54;
            }else{
                gh_actor_motion_status status=devil_far_idle_00262aa8(s,p);
                if(status)return status;
            }
        }
        if(s->substate67_stage!=54)s->substate67_stage=5;
    }
    if(!s->enemy&&s->substate67_stage==54){
        gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p,NULL);
        if(status)return status;
        /* 24BDA0's original room=100 exclusion follows the ray. */
        s->substate67_stage=5;
    }
    if(!s->enemy&&s->substate67_stage==5)return advance_state0_tail(s,p);
    if(s->substate67_stage==46||s->substate67_stage==52){
        const uint8_t *manager=actor_state->entry->b.init->enemies;
        if(player_alive&&!manager[0x5b5]&&!manager[0x53c]){
            int chosen;
            gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
            if(status)return status;
            if(!chosen){
                const float distance=number(word(p+0x618));
                const uint32_t rank=word(p+0x16d8);
                const int r100_em0a=dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3&&
                    actor_state->entry->b.work->room==0x100;
                const unsigned stamp=half(p+0x640);
                const int r100_far_em03=room_work->room==0x100&&dependency==0x203&&
                    ((stamp==19&&!p[0x1650])||(stamp==11&&p[0x1650]==5&&word(p+0x16c4)==6))&&
                    isfinite(distance)&&distance>144.0f;
                if(r104_boss||r101_em00_stamp12(room_work->room,p)||r101_em00_stamp15(room_work->room,p)||
                    r101_em02_stamp5(room_work->room,p)||r101_em03_stamp0(room_work->room,p)||r101_em03_stamp13(room_work->room,p)||r102_em00||r103_arena_jumper(actor_state->entry)||r103_arena_boss(actor_state->entry)||
                    r103_arena_em0a(actor_state->entry)||r100_em0a||r102_gate_em0a(room_work,p)||
                    r100_stamp18_armed_approach(s->enemy,p)||r100_far_stamp18(s->enemy,p,distance)||
                    r100_far_em03||em03_approach_range(s,p,distance)||
                    (r101_arena_airborne(s->enemy)&&(dependency==0x200||dependency==0x203))||
                    r100_at2_formation(room_work,p)||r100_at2_enemy(room_work,p)||r100_at3_enemy(room_work,p)||
                    r101_scenery_enemy(room_work,p)||r101_gate_exchanged_enemy(room_work,p)||r102_area11_em02(room_work,p)||
                    r102_gate_reinforcement(room_work,p)||
                    r105_opening_enemy(room_work,p)||r102_east_em27(room_work,p)||
                    r105_gate_approach(actor_state->entry->b.work,p)||
                    (dependency==0x200&&actor_state->entry->b.work->room==0x100&&distance<=144.0f)){
                    status=continue_ordinary_00212670(s,p,distance);
                    if(status)return status;
                    if(s->substate67_stage!=101)s->substate67_stage=5;
                    goto close_tail;
                }
                if(dependency!=0x200||distance<=144.0f)
                    return stop(s,0x212670,GH_ACTOR_MOTION_UNSUPPORTED);
                const uint32_t wait=word(p+0x5f4);
                if(rank>=3&&wait){
                    /* 212864 decrements the active timer. At this distance
                     * the later zero/9 proximity gate cannot pick a reaction. */
                    put(p+0x5f4,wait-1);
                    s->substate67_stage=101;
                    goto close_tail;
                }
                float actor[3],target[3],turn;
                memcpy(target,p+0x16a0,12);
                if(!position(&actor_state->entry->object.view,actor)||
                    gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                        number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
                if(rank<3)put(p+0x5f4,0);
                const unsigned next=fabsf(turn)>1.5707964f?8:6;
                if(next==8)put(p+0x16d4,word(p+0x16d4)|1u);
                enemy_decision(p,0,next,0);
            }
        }
        s->substate67_stage=5;
    }
close_tail:
    if(s->substate67_stage==101){
        const uint8_t *work=(const uint8_t *)actor_state->entry->b.work;
        if((word(p+0x16d4)&0x08000000u)&&((word(work+0x210)^word(p+0x17d0))&7u)==0){
            float matrix[16],origin[4];
            memcpy(matrix,p+0x80,sizeof(matrix));
            if(gh_scene_at_apply_world_001508d8(matrix,(const float[3]){0,0,0.1f},origin))
                return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
            const float yaw=number(word(p+0x104));
            gh_player_target peer;
            gh_actor_motion_status status=enemy_peer_query(s,origin,yaw,number(0x3f860a92),NULL,&peer);
            if(status)return status;
            if(peer.handle){
                float actor[3],candidate[3],angle;
                if(!position(&actor_state->entry->object.view,actor)||!position(&peer.view,candidate))
                    return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,candidate,yaw,
                    number(0x40490fdb),&angle,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
                enemy_decision(p,0,9,angle<0);
                s->substate67_stage=5;
                return advance_state0_tail(s,p);
            }
        }
        int selected;
        gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p,&selected);
        if(status)return status;
        s->substate67_stage=!selected&&(r102_em00||r105_opening_enemy(room_work,p))?102:5;
    }
    if(s->substate67_stage==102){
        gh_actor_motion_status status=enemy_door_jump_0024bda0(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_substate8_00214298(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_boot_work *work=enemy->b.entry.work;
    const int boss=r103_arena_boss(&enemy->entry);
    const int r104=gh_enemy203_is_r104_entry(&enemy->entry.request,work->room);
    uint32_t pc=0;
    if(!s->substate67_stage){
        if(p[0x2f6]==0)s->substate67_stage=54;
        else if(p[0x2f6]==1){
            put(p+0x16d4,word(p+0x16d4)|1u);
            s->substate67_stage=55;
        }else s->substate67_stage=59;
    }
    if(s->substate67_stage==54){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            float actor[3],target[3],turn;
            memcpy(target,p+0x16a0,12);
            if(!position(&enemy->entry.object.view,actor)||
                gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                    number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
                return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x214298,GH_ACTOR_MOTION_CALLEE);
            float scale=mul(fabsf(turn),0.63661975f);
            if(fesetenv(&env))return stop(s,0x214298,GH_ACTOR_MOTION_CALLEE);
            put(p+0x600,bits(scale));
            int positive=p[0x17c3]?turn<=0:turn>0;
            size_t offset;
            switch(word(p+0x564)){
            case 0x227:
                if(!r102_east_em27(work,p))return stop(s,0x214298,GH_ACTOR_MOTION_UNSUPPORTED);
                /* Original 4359DC/435BDC select the default turn rows. */
                /* fall through */
            case 0x200:offset=positive?0xe4:0xdc;break;
            case 0x202:case 0x203:offset=positive?0x684:0x67c;break;
            case 0x20a:offset=positive?0x864:0x85c;break;
            case 0x214:case 0x215:
                if(!boss)return stop(s,0x214298,GH_ACTOR_MOTION_UNSUPPORTED);
                offset=positive?0x10d4:0x10cc;
                break;
            case 0x250:
                if(!r104)return stop(s,0x214298,GH_ACTOR_MOTION_UNSUPPORTED);
                offset=positive?0x192c:0x1924;
                break;
            case 0x20f:
                if(!r102_route_runner(work,p))return stop(s,0x214298,GH_ACTOR_MOTION_UNSUPPORTED);
                offset=p[0x2f7]?0xd40:0xd28;
                if(positive)offset+=8;
                break;
            default:return stop(s,0x214298,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            const gh_object_view *resource=&enemy->entry.resource;
            if(!resource->bytes||!resource->file_extent_known||
                resource->file_extent<offset+8||resource->file_extent>resource->size)
                return stop(s,0x2146a8,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+offset),word(resource->bytes+offset+4),
                0,5,p[0x17c3]?2:0,!boss&&!r104&&!r102_east_em27(work,p));
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        ++p[0x2f6];
        put(p+0x16d4,word(p+0x16d4)|1u);
        s->substate67_stage=55;
    }
    if(s->substate67_stage==55){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?56:57;
    }
    if(s->substate67_stage==56||s->substate67_stage==52){
        int chosen;
        gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
        if(status)return status;
        s->substate67_stage=chosen?5:60;
    }
    if(s->substate67_stage==60){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=57;
    }
    if(s->substate67_stage==57){
        if(gh_player_root_rotation_scaled_0014f9f0(&enemy->entry.object.view,work,
            number(word(p+0x600)),GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=58;
    }
    if(s->substate67_stage==58){
        if(gh_player_root_translation_0014f928(&enemy->entry.object.view,work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=59;
    }
    if(s->substate67_stage==59||s->substate67_stage==61){
        if(!p[0x2f7]){
            const unsigned heat=heat_category(s);
            if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            const uint32_t rank=word(p+0x16d8);
            const uint32_t threshold=heat==2?2:heat==3||heat==4?3:heat==5?100:1;
            if(rank<threshold){
                int chosen;
                gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
                if(status){if(s->substate67_stage==52)s->substate67_stage=61;return status;}
            }
        }
        s->substate67_stage=5;
    }
    if(s->substate67_stage==53)return stop(s,0x26341c,GH_ACTOR_MOTION_UNSUPPORTED);
    return advance_state0_tail(s,p);
}


static gh_actor_motion_status advance_substate9_002147d0(gh_enemy202_move *s,uint8_t *p)
{
    enum { SELECT=102, BIND, TURN, STEP, CHOOSE, ROTATE, TRANSLATE, ATTACK };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    gh_boot_work *work=enemy->b.entry.work;
    uint32_t pc=0;
    const uint32_t dependency=word(p+0x564);
    const int em27=r105_exchanged_enemy(work,p)||r101_scenery_enemy(work,p)||r101_gate_exchanged_enemy(work,p)||r102_east_em27(work,p);
    const int boss=r103_arena_boss(&enemy->entry);
    if(boss&&p[0x2f7]>2)return stop(s,0x21481c,GH_ACTOR_MOTION_UNSUPPORTED);
    const int em0a=(work->room==0x100&&dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3)||
        r102_gate_em0a(work,p)||r103_arena_em0a(&enemy->entry);

    if(!s->substate67_stage){
        if(!boss&&!em27&&!em0a&&dependency!=0x200&&dependency!=0x202&&dependency!=0x203)
            return stop(s,0x2147d0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]==0?SELECT:p[0x2f6]==1?TURN:ATTACK;
    }
    if(s->substate67_stage==SELECT){
        if(word(p+0x6ec))return stop(s,0x214ba0,GH_ACTOR_MOTION_UNSUPPORTED);
        /* EM27 selects the default resource pair at 21487C and the normal tail at 214C94. */
        /* The paired bosses use the original 10E4/10EC resource rows. */
        size_t offset=boss?0x10e4:em0a?0x84c:dependency==0x200||em27?0xcc:0x66c;
        if(gh_enemy_timers_weapon_kind(&enemy->timers))offset=0x464;
        unsigned variant=p[0x2f7];
        if(variant==2){
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            variant=draw&1u;
        }
        if(p[0x17c3])variant^=1u;
        s->reaction_resource_offset=offset+(variant?8:0);
        s->substate67_stage=BIND;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            const size_t offset=s->reaction_resource_offset;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<offset+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x214bd8,GH_ACTOR_MOTION_BOUNDS);
            const uint8_t *row=resource->bytes+offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(row),word(row+4),0,10,p[0x17c3]?2:0,!boss&&!em27&&!em0a);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x604,0x42b40000);
        ++p[0x2f6];
        put(p+0x600,0x42b40000);
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        float origin[3],target[3],turn;
        const float yaw=number(word(p+0x104)),rate=number(word(p+0x5a8));
        const float work_rate=number(word((const uint8_t *)work+0x55c));
        fenv_t env;
        if(!position(actor,origin)||!position(s->player,target)||!isfinite(rate)||!isfinite(work_rate)||
            !set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        const float cap=mul(mul(rate,number(0x3dc90fdb)),work_rate);
        gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
            origin,target,yaw,cap,&turn,&pc);
        float next=status?0:add(yaw,turn);
        if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_id_angle_reference_001f7d48(next,&next))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(next));
        s->substate67_stage=STEP;
    }
    if(s->substate67_stage==STEP){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended&&p[0x2f7]!=2?CHOOSE:ROTATE;
    }
    if(s->substate67_stage==CHOOSE){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ATTACK;
    }
    /* Both slots at 435F60 return from this move before the ordinary chooser. */
    if(s->substate67_stage==ATTACK&&boss)s->substate67_stage=5;
    if(s->substate67_stage==ATTACK){
        const unsigned heat=heat_category(s);
        if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint32_t threshold=heat==2?2:heat==3||heat==4?3:heat==5?100:1;
        if(word(p+0x16d8)<threshold){
            int chosen;
            gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
            if(status)return status;
        }
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_em03_approach_00215d40(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=170, PICK_RUN_VARIANT, TURN, SAMPLE, SELECT_EXIT, ROTATE, TRANSLATE,
           MEASURE_FACING, CHECK_RUN_VARIANT, CHECK_DISTANCE, ATTACK };
    const gh_enemy_move_actor *runtime=&s->runtime;
    const gh_enemy_entry *entry=runtime->entry;
    const gh_boot_work *work=entry->b.work;
    const gh_title_player_view *actor=&entry->object.view;
    gh_actor_motion *motion=runtime->motion;
    const float rate=number(word(p+0x5a8));
    uint32_t pc=0;
    if(!gh_enemy203_is_r104_entry(&entry->request,work->room)||!runtime->bindings->canonical_rate)
        return stop(s,0x215d40,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_phase=p[0x2f6];
        if(!p[0x2f6]){
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&entry->b.init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x17d0,draw%5u);
        }
        s->substate67_stage=p[0x2f6]>5?ATTACK:!(p[0x2f6]&1)?BIND:p[0x2f6]==5?SAMPLE:TURN;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        const gh_object_view *resource=&entry->resource;
        const size_t row=0x1934+4u*s->substate67_phase;
        if(!call->active){
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x215d40,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,10,p[0x17c3]?2:0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        const unsigned phase=++s->substate67_phase;
        if(phase!=3)++p[0x2f6];
        s->substate67_stage=phase==3?PICK_RUN_VARIANT:phase==5?SAMPLE:TURN;
    }
    if(s->substate67_stage==PICK_RUN_VARIANT){
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&entry->b.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        p[0x2f7]=(uint8_t)(draw&1u);
        ++p[0x2f6];
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        float origin[3],target[3],turn;
        memcpy(target,p+0x16a0,sizeof(target));
        const float yaw=number(word(p+0x104));
        const float work_rate=number(word((const uint8_t *)work+0x55c));
        const float turn_rate=number(s->substate67_phase==1?0x3dc90fdb:0x3e490fdb);
        fenv_t env;
        if(!position(actor,origin)||!isfinite(rate)||!isfinite(work_rate)||
           !set_round(&env,FE_TOWARDZERO))return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        const float cap=mul(mul(rate,turn_rate),work_rate);
        const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
            origin,target,yaw,cap,&turn,&pc);
        float forward=status?0:add(yaw,turn);
        if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_id_angle_reference_001f7d48(forward,&forward))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(forward));
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *runtime->bindings->canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(s->substate67_phase==1&&ended)++p[0x2f6];
        if(s->substate67_phase==3){
            const unsigned heat=heat_category(s);
            if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x215f04,GH_ACTOR_MOTION_CALLEE);
            const float stride=mul(rate,heat==5?0.6f:0.3f);
            if(fesetenv(&env)||!isfinite(stride))return stop(s,0x215f04,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x338,bits(stride));
        }
        s->substate67_stage=s->substate67_phase==5&&ended?SELECT_EXIT:ROTATE;
    }
    if(s->substate67_stage==SELECT_EXIT){
        const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=s->substate67_phase==3?MEASURE_FACING:ATTACK;
    }
    if(s->substate67_stage==MEASURE_FACING){
        float origin[3],target[3],angle;
        memcpy(target,p+0x1690,sizeof(target));
        if(!position(actor,origin)||gh_player_target_angle_002da038(s->elf,s->elf_bytes,
            origin,target,number(word(p+0x104)),number(0x40490fdb),&angle,&pc))
            return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        /* The original keeps the pre-selection angle for both distance checks. */
        s->substate67_phase=ordered_abs(angle)>number(0x3fc90fdb);
        s->substate67_stage=CHECK_RUN_VARIANT;
    }
    if(s->substate67_stage==CHECK_RUN_VARIANT){
        if(p[0x2f7]&&number(word(p+0x618))<25.0f){
            const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
            if(status)return status;
        }
        s->substate67_stage=CHECK_DISTANCE;
    }
    if(s->substate67_stage==CHECK_DISTANCE){
        if(s->substate67_phase||number(word(p+0x618))<6.25f){
            const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
            if(status)return status;
        }
        s->substate67_stage=ATTACK;
    }
    if(s->substate67_stage==ATTACK){
        int chosen;
        const gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_substate3_002112f0(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    const gh_title_player_view *view=&enemy->entry.object.view;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_boot_work *work=enemy->b.entry.work;
    const unsigned dependency=word(p+0x564);
    const int periodic=((word((const uint8_t *)work+0x210)^word(p+0x17d0))&7u)==0;
    const float distance=number(word(p+0x618));
    uint32_t pc=0;
    const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&enemy->timers);
    const int r101_armed=word(p+0x16c4)==6&&r101_em03_stamp13(work->room,p)&&
        held&&held->weapon&&held->weapon->kind==0x361;
    const int r101_home=word(p+0x16c4)==6&&
        (r101_em00_stamp12(work->room,p)||r101_em00_stamp15(work->room,p)||r101_em02_stamp5(work->room,p)||r101_armed);
    const int east_em27_home=word(p+0x16c4)==6&&r102_east_em27(work,p);
    const int west_em0a_home=work->room==0x100&&dependency==0x20a&&
        !half(p+0x640)&&p[0x1650]==3&&word(p+0x16c4)==6&&!p[0x2f7];
    if(!s->substate67_stage){
        /* 2112F0 uses the default EM00 98/9C idle row after the opening
         * actor completes the original mode 6 return. */
        const unsigned stamp=half(p+0x640);
        const int r100_opening_home=work->room==0x100&&!p[0x1650]&&
            ((dependency==0x200&&stamp==20)||(dependency==0x203&&stamp==19));
        const int return_home=word(p+0x16c4)==6&&(r100_opening_home||west_em0a_home||
            r102_gate_actor(work,p)||r102_at0b_em02(work,p)||east_em27_home||(dependency==0x203&&work->room==0x100&&p[0x1650]==5));
        if(!r101_home&&!return_home&&(p[0x1650]!=2||(dependency!=0x200&&dependency!=0x202&&dependency!=0x203)))
            return stop(s,0x2112f0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        switch(p[0x2f6]){
        case 0:
        case 2:s->substate67_stage=79;break;
        case 1:s->substate67_stage=80;break;
        case 3:s->substate67_stage=85;break;
        default:s->substate67_stage=88;
        }
    }
    if(s->substate67_stage==79){
        const int turning=p[0x2f6]==2;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            if(word(p+0x6ec)||(!r101_armed&&gh_enemy_timers_weapon_kind(&enemy->timers)))
                return stop(s,turning?0x211978:0x21151c,GH_ACTOR_MOTION_UNSUPPORTED);
            const gh_object_view *resource=&enemy->entry.resource;
            /* Both EM0A turn tables select 830; the idle table selects 828. */
            const unsigned offset=r101_armed?(turning?0x454:0x44c):west_em0a_home?(turning?0x830:0x828):
                (dependency==0x200||east_em27_home)?(turning?0xb0:0x98):(turning?0x650:0x648);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<offset+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2a8578,GH_ACTOR_MOTION_BOUNDS);
            /* The original turn sign branches both select this row for these dependencies. */
            if(turning){
                float actor[3],target[3],angle;
                if(!position(view,actor)||!position(s->player,target)||
                    gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                        number(word(p+0x104)),number(0x40490fdb),&angle,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+offset),word(resource->bytes+offset+4),
                0,10,p[0x17c3]?2:0,!r101_home&&!west_em0a_home);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(!turning)put(p+0x5f0,10);
        p[0x2f6]++;
        s->substate67_stage=turning?85:80;
    }
    if(s->substate67_stage==80){
        const uint32_t cooldown=word(p+0x5f0);
        if(cooldown){
            put(p+0x5f0,cooldown-1u);
            s->substate67_stage=81;
        }else{
            put(p+0x16d0,word(p+0x16d0)|0x8000u);
            s->substate67_stage=number(word(p+0x24c))>0?82:87;
        }
    }
    if(s->substate67_stage==85){
        float actor[3],target[3],turn;
        fenv_t env;
        const float yaw=number(word(p+0x104)),actor_rate=number(word(p+0x5a8));
        const float turn_rate=number(word((const uint8_t *)work+0x55c));
        if(!position(view,actor)||!position(s->player,target)||!isfinite(actor_rate)||actor_rate<0||
            !isfinite(turn_rate)||turn_rate<0||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        const float cap=mul(mul(actor_rate,0.09817477f),turn_rate);
        const gh_player_target_status status=gh_player_target_angle_002da038(
            s->elf,s->elf_bytes,actor,target,yaw,cap,&turn,&pc);
        const float next=status?0:add(yaw,turn);
        if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        float wrapped;
        if(gh_id_angle_reference_001f7d48(next,&wrapped))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(wrapped));
        s->substate67_stage=distance>900&&!periodic?87:86;
    }
    if(s->substate67_stage==81||s->substate67_stage==82||s->substate67_stage==86){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=s->substate67_stage==82?83:87;
    }
    if(s->substate67_stage==83){
        if(gh_player_root_rotation_0014f9f0(view,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=84;
    }
    if(s->substate67_stage==84){
        if(gh_player_root_translation_0014f928(view,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=87;
    }
    if(s->substate67_stage==87){
        float actor[3],target[3],turn;
        if(!position(view,actor)||!position(s->player,target)||
            gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
            return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(p[0x2f6]==1&&fabsf(turn)>0.34906584f)p[0x2f6]=2;
        else if(p[0x2f6]==3&&fabsf(turn)<0.08726646f)p[0x2f6]=0;
        s->substate67_stage=88;
    }
    if(s->substate67_stage==88){
        if(periodic&&(distance<64||word(p+0x16ec))){
            if(!position(s->player,s->path_target))
                return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x16d4,word(p+0x16d4)&~0x08000000u);
            s->route=(gh_enemy_route_call){0};
            s->substate67_stage=89;
        }else s->substate67_stage=number(word(p+0x16c0))>0?91:5;
    }
    if(s->substate67_stage==89){
        float actor[3];
        if(!position(view,actor))return stop(s,0x2a74e0,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=route_clear_002a74e0(s,p,actor);
        if(status)return status;
        s->pending_ai_pc=0x2112f0;
        if(number(word(p+0x510))<8||word(p+0x16ec)){
            put(p+0x16d0,(word(p+0x16d0)&~0x8000u)|2u);
            s->substate67_stage=90;
        }else s->substate67_stage=number(word(p+0x16c0))>0?91:5;
    }
    if(s->substate67_stage==90){
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if((draw&1u)&&number(word(p+0x16c0))<=0&&(int32_t)word(p+0x16ec)<1){
            p[0x2f4]=0;p[0x2f5]=0x6b;p[0x2f6]=p[0x2f7]=0;
            s->substate67_stage=5;
        }else s->substate67_stage=91;
    }
    if(s->substate67_stage==91){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_substate1_0020fbb8(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_title_player_view *actor=&enemy->entry.object.view;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_boot_work *work=enemy->b.entry.work;
    const unsigned dependency=word(p+0x564);
    const unsigned appearance=p[0x1650];
    const int held=gh_enemy_timers_weapon_kind(&enemy->timers)!=0;
    const int periodic=((word((const uint8_t *)work+0x210)^word(p+0x17d0))&7u)==0;
    const float distance=number(word(p+0x618));
    uint32_t pc=0;
    const int r105_exchange=r105_exchanged_enemy(work,p);
    const unsigned stamp=half(p+0x640);
    const int r105_em0a=work->room==0x105&&dependency==0x20a&&appearance==0&&(stamp==3||stamp==5);
    const int r101_em0a=work->room==0x101&&dependency==0x20a&&appearance==1&&stamp==7;
    const int em0a_appearance5=dependency==0x20a&&appearance==5;
    const int em0f_appearance0=dependency==0x20f&&appearance==0;
    const int em27_appearance0=dependency==0x227&&appearance==0;
    const int common_appearance1=appearance==1&&(dependency==0x200||dependency==0x203);
    const int appearance1_em03=dependency==0x203&&appearance==1&&
        ((work->room==0x105&&stamp==6)||(work->room==0x101&&stamp==0));
    const int r101_appearance4=work->room==0x101&&appearance==4&&
        ((stamp==5&&dependency==0x202)||((stamp==9||stamp==10||stamp==11)&&dependency==0x227)||
         (stamp==12&&dependency==0x200));
    const int r105_entry=r105_exchange||r105_em0a||r101_em0a||appearance1_em03||r101_appearance4;
    const int r105_stamp8=work->room==0x105&&stamp==8&&dependency==0x202&&appearance==0;
    const int unarmed_em02=dependency==0x202&&appearance==0&&
        ((work->room==0x100&&(stamp==29||stamp==32))||
         (work->room==0x102&&(stamp==23||stamp==25)));
    const int r102_armed_em00=work->room==0x102&&stamp==0x0a&&dependency==0x200&&appearance==9;
    const int r104_boss=gh_enemy203_is_r104_entry(&enemy->entry.request,work->room);
    const int r103_minion=appearance==24&&stamp>=2&&
        gh_enemy204_is_r103_entry(&enemy->entry.request,work->room);
    if(!s->substate67_stage){
        if((r105_stamp8||unarmed_em02||r103_minion||r104_boss)&&held)return stop(s,0x210590,GH_ACTOR_MOTION_UNSUPPORTED);
        if(r105_entry&&(p[0x2f6]>1||held))
            return stop(s,p[0x2f6]>1?0x210a58:0x210590,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!(((appearance==0||appearance==5)&&(dependency==0x200||dependency==0x203))||
            ((appearance==9&&dependency==0x202)||r102_armed_em00)||em0a_appearance5||em0f_appearance0||em27_appearance0||
            common_appearance1||r105_entry||r105_stamp8||unarmed_em02||r103_minion||r104_boss))
            return stop(s,0x20fbb8,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        switch(p[0x2f6]){
        case 0:
            put(p+0x16d0,word(p+0x16d0)|0x8000u);
            s->substate67_stage=62;
            break;
        case 1:s->substate67_stage=63;break;
        case 2:{
            uint32_t flags=word(p+0x16d0)&~0x8000u;
            if(!(flags&0x20000000u)&&number(word(p+0x16c0))<=0)flags|=2u;
            put(p+0x16d0,flags);
            s->substate67_stage=72;
            break;
        }
        case 3:s->substate67_stage=73;break;
        default:s->substate67_stage=5;
        }
    }
    if(s->substate67_stage==62||s->substate67_stage==72){
        const int enter=s->substate67_stage==62;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            if(enter&&word(p+0x6ec)&&!(r102_armed_em00&&held))
                return stop(s,0x210590,GH_ACTOR_MOTION_UNSUPPORTED);
            if(enter&&!position(actor,s->path_target))
                return stop(s,0x2105e0,GH_ACTOR_MOTION_UNSUPPORTED);
            const gh_object_view *resource=&enemy->entry.resource;
            unsigned offset,sequence;
            uint32_t start=0;
            if(r104_boss){
                offset=0x18f0;
                sequence=0x18f4;
            }else if(!enter){
                if(common_appearance1)offset=0x404;
                else if(em0a_appearance5)offset=0xbe0;
                else if(em0f_appearance0)offset=0xfcc;
                else if(appearance==5)offset=dependency==0x203?0x7bc:0x42c;
                else if(r103_minion)offset=dependency==0x203?0x7b4:0x414;
                else offset=dependency==0x200||em27_appearance0?0x414:0x7b4;
                sequence=offset+4;
            }else if(held){
                offset=appearance==9?0x490:0x44c;
                sequence=offset+4;
                p[0x2f7]=0;
            }else if(appearance==5){
                /* The 20A row selects its own pair in the appearance-5 table. */
                offset=em0a_appearance5?0xbd8:0x424;
                sequence=offset+4;
            }else if(em0f_appearance0){
                /* The initial 20F row uses the ordinary pair unless bit 21
                 * selects the shared-motion branch at 21000C. */
                if(word(p+0x1644)&0x200000u)
                    return stop(s,0x21000c,GH_ACTOR_MOTION_UNSUPPORTED);
                offset=0xcf8;
                sequence=0xcfc;
            }else if(r105_em0a||r101_em0a){
                /* Dependency 20A dispatches to 20FD18. */
                offset=0x828;
                sequence=0x82c;
            }else if(r101_appearance4){
                /* Original 20FBB8 appearance 4 entry rows. */
                offset=0x40c;
                sequence=0x410;
            }else if(common_appearance1){
                /* Dependencies 200 and 203 share the appearance-1 entry pair. */
                offset=0x3fc;
                sequence=0x400;
            }else if(r103_minion){
                offset=dependency==0x20a?0xbcc:0x3f4;
                sequence=offset+4;
                start=bits((float)(7u-(stamp&7u)));
            }else{
                /* Original 227 uses 20FCA8's ordinary entry pair as well. */
                offset=dependency==0x200||em27_appearance0?0x98:0x648;
                sequence=0x9c;
            }
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<offset+4||
                resource->file_extent<sequence+4||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2a8578,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+offset),word(resource->bytes+sequence),
                start,enter?0:10,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(enter){
            memcpy(p+0x5c0,s->path_target,12);
            put(p+0x5f0,30);
        }
        p[0x2f6]++;
        s->substate67_stage=enter?63:73;
    }
    if(s->substate67_stage==63||s->substate67_stage==73){
        const int waiting=s->substate67_stage==63;
        uint32_t flags=word(p+0x16d0);
        if(appearance==5)flags|=0x20000u;
        if(waiting&&(word(p+0x1644)&0x100000u))flags|=0x10000u;
        put(p+0x16d0,flags);
        if(waiting&&number(word(p+0x24c))<=0)s->substate67_stage=67;
        else{
            if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
            int ended;
            gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
                *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
            if(status)return stop(s,pc?pc:0x157c18,status);
            s->substate67_stage=waiting?64:ended?74:75;
        }
    }
    if(s->substate67_stage==74){
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!(draw&1u)||number(word(p+0x16c0))>0||(int32_t)word(p+0x16ec)>0)
            s->substate67_stage=77;
        else{
            p[0x2f4]=0;p[0x2f5]=0x6b;p[0x2f6]=p[0x2f7]=0;
            s->substate67_stage=75;
        }
    }
    if(s->substate67_stage==77){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=75;
    }
    if(s->substate67_stage==64||s->substate67_stage==75){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        ++s->substate67_stage;
    }
    if(s->substate67_stage==65||s->substate67_stage==76){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=s->substate67_stage==65?66:5;
    }
    if(s->substate67_stage==66){
        float current[3];
        if(!position(actor,current))return stop(s,0x2106d0,GH_ACTOR_MOTION_UNSUPPORTED);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x2106d0,GH_ACTOR_MOTION_CALLEE);
        for(unsigned axis=0;axis<=2;axis+=2)
            current[axis]=add(mul(current[axis],0.995f),mul(number(word(p+0x5c0+axis*4)),0.005f));
        if(fesetenv(&env)||!isfinite(current[0])||!isfinite(current[2]))
            return stop(s,0x2106d0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(actor->alias_f0,bits(current[0]));
        put(actor->alias_f0+8,bits(current[2]));
        s->substate67_stage=67;
    }
    if(s->substate67_stage==67){
        if(periodic&&distance<64){
            float origin[3],target[3];
            if(!position(actor,origin)||!position(s->player,target))
                return stop(s,0x26f328,GH_ACTOR_MOTION_UNSUPPORTED);
            gh_actor_motion_status status=target_line_of_sight(s,p,origin,target);
            if(status)return status;
        }
        s->substate67_stage=68;
    }
    if(s->substate67_stage==68){
        const uint32_t spawn_flags=word(p+0x1644);
        const uint32_t cooldown=word(p+0x5f0);
        if(spawn_flags&0x10008000u){
            if(spawn_flags&0x10000000u){
                if(!enemy->b.solids||gh_collision_solid_set_enabled_001351d8(enemy->b.solids,actor,0,&pc))
                    return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
                put(p+0x16d0,word(p+0x16d0)|0x1000u);
            }
            if((appearance==9||(spawn_flags&0x8000u))&&word(p+0x16ec))p[0x2f6]=2;
            s->substate67_stage=71;
        }else if(cooldown){
            put(p+0x5f0,cooldown-1u);
            s->substate67_stage=70;
        }else if(word(p+0x16d0)&0x200000u){
            if(distance<64&&periodic){
                if(!position(s->player,s->path_target))
                    return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
                put(p+0x16d4,word(p+0x16d4)&~0x08000000u);
                s->route=(gh_enemy_route_call){0};
                s->substate67_stage=69;
            }else{
                if(distance<9)p[0x2f6]=2;
                s->substate67_stage=70;
            }
        }else s->substate67_stage=70;
    }
    if(s->substate67_stage==69){
        float origin[3];
        if(!position(actor,origin))return stop(s,0x2a74e0,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=route_clear_002a74e0(s,p,origin);
        if(status)return status;
        s->pending_ai_pc=0x20fbb8;
        if(number(word(p+0x510))<8||distance<9)p[0x2f6]=2;
        s->substate67_stage=70;
    }
    if(s->substate67_stage==70){
        if(periodic&&(word(p+0x16d0)&0x200000u)){
            int near;
            gh_actor_motion_status status=gh_enemy_active_near_002919f8(enemy->b.entry.init,actor,4,&near,&pc);
            if(status)return stop(s,pc?pc:0x2919f8,status);
            if(near)p[0x2f6]=2;
            status=gh_enemy_other_near_00291ac8(enemy->b.entry.init,actor,actor,3,&near,&pc);
            if(status)return stop(s,pc?pc:0x291ac8,status);
            if(near)p[0x2f6]=2;
        }
        if(word(p+0x16ec))p[0x2f6]=2;
        s->substate67_stage=71;
    }
    if(s->substate67_stage==71){
        if((word(p+0x16d0)&0x20000000u)||number(word(p+0x16c0))>0)p[0x2f6]=2;
        if(p[0x2f6]==2){
            s->substate67_stage=78;
        }else s->substate67_stage=5;
    }
    if(s->substate67_stage==78){
        if(!s->room_state)return stop(s,0x295498,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_enemy_clear_set_flags_002736f8(actor,s->room_state,&pc);
        if(status)return stop(s,pc?pc:0x295498,status);
        s->substate67_stage=appearance==5?5:92;
    }
    if(s->substate67_stage==92){
        if(appearance==9){
            if(word(p+0x16d0)&0x20000000u){
                p[0x2f4]=0;p[0x2f5]=held?0x47:0x15;p[0x2f6]=0;p[0x2f7]=(uint8_t)held;
                s->substate67_stage=5;
            }else s->substate67_stage=93;
        }else{
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if((draw&1u)&&number(word(p+0x16c0))<=0&&(int32_t)word(p+0x16ec)<=0){
                p[0x2f4]=0;p[0x2f5]=0x6b;p[0x2f6]=p[0x2f7]=0;
                s->substate67_stage=5;
            }else s->substate67_stage=93;
        }
    }
    if(s->substate67_stage==93){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_substate71_0023f028(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_game_init_state *init=enemy->b.entry.init;
    const gh_boot_work *work=enemy->b.entry.work;
    uint32_t pc=0;
    if(!s->state71_phase){
        init->enemies[0x540]=10;
        switch(p[0x2f6]){
        case 0:
            p[0x1864]=0;
            put_half(p+0x568,0);
            s->state71_phase=1;
            break;
        case 2:
            p[0x1864]=0;
            s->state71_phase=2;
            break;
        case 1:
        case 3:
            s->state71_phase=3;
            break;
        default:
            s->state71_phase=11;
        }
    }
    if(s->state71_phase==1||s->state71_phase==2){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            const unsigned offset=s->state71_phase==1?0x41c:0x1430;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<offset+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x23f028,GH_ACTOR_MOTION_BOUNDS);
            const int remainder=(int32_t)word(p+0x17d0)%3;
            const uint32_t start=s->state71_phase==1?
                (remainder==1?0x41a00000u:remainder==2?0x42700000u:0):0;
            const uint16_t flags=s->state71_phase==1&&p[0x17c3]?2:0;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+offset),word(resource->bytes+offset+4),
                start,10,flags,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5f0,1);
        put(p+0x16d0,word(p+0x16d0)&~0x8000u);
        p[0x2f6]++;
        s->state71_phase=3;
    }
    if(s->state71_phase==3){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state71_exit=(uint8_t)(p[0x2f6]==3);
        if(ended&&p[0x2f6]==1){
            put(p+0x5f0,1);
            s->state71_phase=4;
        }else if(ended&&p[0x2f6]==3)s->state71_phase=13;
        else s->state71_phase=5;
    }
    if(s->state71_phase==4){
        const gh_game_title_player *near=NULL;
        gh_actor_motion_status status=gh_enemy_find_ema2_near_00292dc8(init,
            &enemy->entry.object.view,4,s->elf,s->elf_bytes,&near,&pc);
        if(status)return stop(s,pc?pc:0x292dc8,status);
        s->state71_exit=near?0:2;
        s->state71_phase=5;
    }
    if(s->state71_phase==13){
        put(p+0x16d0,word(p+0x16d0)&~0x8000u);
        uint32_t random=1;
        if((int32_t)word(p+0x16ec)<1&&gh_game_mt_next_0030f348(&init->random,&random))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state71_phase=(random&1u)?14:5;
        if(!(random&1u)){
            p[0x2f4]=0;p[0x2f5]=0x6b;p[0x2f6]=p[0x2f7]=0;
        }
        s->state71_exit=1;
    }
    if(s->state71_phase==14){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->state71_phase=5;
    }
    if(s->state71_phase==5){
        if(gh_player_root_rotation_0014f9f0(&enemy->entry.object.view,work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state71_phase=6;
    }
    if(s->state71_phase==6){
        if(gh_player_root_translation_0014f928(&enemy->entry.object.view,work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        if(s->state71_exit==2){
            p[0x2f4]=0;p[0x2f5]=3;p[0x2f6]=p[0x2f7]=0;
            s->state71_phase=12;
            s->substate67_stage=5;
        }else s->state71_phase=s->state71_exit?11:7;
    }
    const int periodic=((word((const uint8_t *)work+0x210)^word(p+0x17d0))&7u)==0;
    if(s->state71_phase==7){
        if(p[0x2f6]!=1||p[0x2f7])s->state71_phase=10;
        else{
            const float distance=number(word(p+0x618));
            if(!isfinite(distance))return stop(s,0x23f310,GH_ACTOR_MOTION_UNSUPPORTED);
            if(distance<64&&periodic){
                if(!position(s->player,s->path_target))return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
                put(p+0x16d4,word(p+0x16d4)&~0x08000000u);
                s->route=(gh_enemy_route_call){0};
                s->state71_phase=8;
            }else s->state71_phase=9;
        }
    }
    if(s->state71_phase==8){
        float actor[3];
        if(!position(&enemy->entry.object.view,actor))return stop(s,0x2a74e0,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=route_clear_002a74e0(s,p,actor);
        if(status)return status;
        s->pending_ai_pc=0x23f028;
        s->state71_phase=number(word(p+0x510))<8?15:9;
    }
    if(s->state71_phase==9){
        if(number(word(p+0x618))<9){
            if(!half(p+0x568))s->state71_phase=15;
            else{
                put(p+0x16d0,word(p+0x16d0)&~0x8000u);
                p[0x2f6]=2;
                s->state71_phase=periodic?16:10;
            }
        }
        else s->state71_phase=periodic?16:10;
    }
    if(s->state71_phase==16){
        int found;
        gh_actor_motion_status status=gh_enemy_active_near_002919f8(init,
            &enemy->entry.object.view,4,&found,&pc);
        if(status)return stop(s,pc?pc:0x2919f8,status);
        s->state71_phase=18;
        if(found){
            put(p+0x16d0,word(p+0x16d0)&~0x8000u);
            if(half(p+0x568))p[0x2f6]=2;
            else s->state71_phase=15;
        }
    }
    if(s->state71_phase==18){
        int found;
        gh_actor_motion_status status=gh_enemy_other_near_00291ac8(init,
            &enemy->entry.object.view,&enemy->entry.object.view,3,&found,&pc);
        if(status)return stop(s,pc?pc:0x291ac8,status);
        s->state71_phase=10;
        if(found){
            put(p+0x16d0,word(p+0x16d0)&~0x8000u);
            if(half(p+0x568))p[0x2f6]=2;
            else s->state71_phase=15;
        }
    }
    if(s->state71_phase==15){
        put(p+0x16d0,word(p+0x16d0)&~0x8000u);
        uint32_t random;
        if(gh_game_mt_next_0030f348(&init->random,&random))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(random&1u){
            p[0x2f4]=0;p[0x2f5]=0x6b;p[0x2f6]=p[0x2f7]=0;
            s->state71_phase=11;
        }else s->state71_phase=19;
    }
    if(s->state71_phase==19){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->state71_phase=11;
    }
    if(s->state71_phase==10){
        if(number(word(p+0x16c0))>0||(int32_t)word(p+0x16ec)>0){
            put(p+0x16d0,word(p+0x16d0)&~0x8000u);
            if(!half(p+0x568)){
                gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
                if(status)return status;
            }else p[0x2f6]=2;
        }
        s->state71_phase=11;
    }
    if(s->state71_phase==11){
        if(!(half(p+0x3ac)&1u))put(p+0x5f0,1);
        else if(word(p+0x5f0)){
            put(p+0x5f0,0);
            s->state71_phase=17;
        }
        if(s->state71_phase==11){s->state71_phase=12;s->substate67_stage=5;}
    }
    if(s->state71_phase==17){
        const gh_enemy_ema2_damage_bindings bindings={.manager=init,.origin=&enemy->entry.object.view,
            .player=s->player,.work=work,.elf=s->elf,.elf_bytes=s->elf_bytes,
            .sound=s->near_sound,.sound_context=s->near_sound_context};
        gh_actor_motion_status status=gh_enemy_damage_ema2_near_00292d18(&s->near_damage,&bindings,&pc);
        if(status)return stop(s,pc?pc:0x292d18,status);
        s->state71_phase=12;s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status enemy_door_ray_0024bb68(gh_enemy202_move *s,uint8_t *p,int *selected)
{
    if(selected)*selected=0;
    const gh_enemy_move_actor *actor=&s->runtime;
    const gh_boot_work *work=actor->entry->b.work;
    const uint32_t dependency=word(p+0x564);
    /* 24BB68 excludes Devil variants from the staggered ray schedule. */
    if((dependency<0x220||dependency>0x222)&&
       ((word((const uint8_t *)work+0x210)^word(p+0x17d0))&7u)!=0)
        return GH_ACTOR_MOTION_OK;
    float origin[3],target[3],turn;
    memcpy(origin,p+0x490,12);
    memcpy(target,p+0x16a0,12);
    uint32_t pc=0;
    if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,origin,target,
        number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    if(fabsf(turn)>0.5235988f)return GH_ACTOR_MOTION_OK;
    float matrix[16],point[4],normal[4];
    memcpy(matrix,p+0x80,sizeof(matrix));
    const float from[3]={0,0.5f,0},to[3]={0,0.5f,0.8f};
    gh_scene_ray_report report={0};
    gh_scene_ray_request ray={.mask=1,.segmented=1,.report_limit=1,
        .native_reports=&report,.hit_position=point,.hit_normal=normal};
    if(gh_scene_at_apply_world_001508d8(matrix,from,ray.from)||
        gh_scene_at_apply_world_001508d8(matrix,to,ray.to))
        return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
    int hit;
    if(!actor->bindings->actors||gh_scene_ray_query_00131e10(actor->bindings->actors,&ray,&hit,&pc))
        return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!hit)return GH_ACTOR_MOTION_OK;
    if(!report.attributes||report.attribute_bytes<8)
        return stop(s,0x24bcbc,GH_ACTOR_MOTION_BOUNDS);
    if(!(word(report.attributes+4)&0x20u))return GH_ACTOR_MOTION_OK;
    if(work->room!=0x102&&!r103_arena_jumper(actor->entry)&&!r103_arena_em0a(actor->entry)&&
       !r103_arena_boss(actor->entry))
        return stop(s,0x24bcd4,GH_ACTOR_MOTION_UNSUPPORTED);

    /* 24BCD4 takes the horizontal heading opposite the surface normal,
     * transforms (0,0,-0.5) and adds the original ray hit position. */
    float yaw,sine,cosine;
    if(gh_game_angle_001f8180(-normal[0],-normal[2],&yaw,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
        return stop(s,pc?pc:0x1f8180,GH_ACTOR_MOTION_UNSUPPORTED);
    if(gh_sdk_rotation_sincos_reference_0030a5d0(yaw,&sine,&cosine,GH_STATIC_BINARY32_REFERENCE,&pc))
        return stop(s,pc?pc:0x147988,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!position(&actor->entry->object.view,origin))
        return stop(s,0x24bd38,GH_ACTOR_MOTION_ARGUMENT);
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x24bd30,GH_ACTOR_MOTION_CALLEE);
    const float x=add(point[0],mul(sine,-0.5f)),z=add(point[2],mul(cosine,-0.5f));
    if(fesetenv(&env)||!isfinite(x)||!isfinite(z))
        return stop(s,0x24bd30,GH_ACTOR_MOTION_UNSUPPORTED);
    put(p+0x6d0,bits(x));put(p+0x6d4,bits(origin[1]));put(p+0x6d8,bits(z));
    put(p+0x6e0,bits(yaw));
    enemy_decision(p,0,0x75,0);
    if(selected)*selected=1;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_substate5_002130c8(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=180, SET_TIMER, TURN, SAMPLE, ROTATE, TRANSLATE, CHECK_NEAR,
        CHECK_PLAYER, ATTACK, CHECK_DOOR, CHOOSE, CHECK_JUMP };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_game_init_state *init=enemy->b.entry.init;
    gh_boot_work *work=enemy->b.entry.work;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    const uint32_t dependency=word(p+0x564);
    const float distance=number(word(p+0x618));
    uint32_t pc=0;
    /* R102 stamp 11 uses EM00's original unarmed default motion. */
    const int r102=(work->room==0x102&&dependency==0x200&&half(p+0x640)==11&&p[0x1650]==5)||
        r102_east_em27(work,p);
    const int r101=r101_em00_stamp12(work->room,p)||r101_em00_stamp15(work->room,p)||
        r101_em02_stamp5(work->room,p)||r101_em03_stamp0(work->room,p);
    const int r105=r105_opening_enemy(work,p);
    const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&enemy->timers);
    const int r101_armed=work->room==0x101&&dependency==0x203&&half(p+0x640)==13&&
        !p[0x1650]&&!word(p+0x6ec)&&held&&held->weapon&&held->weapon->kind==0x361;
    const int gate=r102_gate_reinforcement(work,p);
    const int r101_scenery=r101_scenery_enemy(work,p);
    const int area11=(r102_area11_obstacle(work,p)&&word(p+0x16c4)==2)||r102_area11_em02(work,p);
    /* EM0A uses resource pair 0x830/0x834 in both rooms. */
    const int em0a=(work->room==0x100&&dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3)||
        r102_gate_em0a(work,p)||r103_arena_em0a(&enemy->entry);
    const int boss=r103_arena_boss(&enemy->entry);
    const int r104=gh_enemy203_is_r104_entry(&enemy->entry.request,work->room);
    const int arena=r103_arena_jumper(&enemy->entry)||boss;
    if((!r101&&!r102&&!r105&&!area11&&!gate&&!em0a&&!r101_armed&&!r101_scenery&&!arena&&!r104&&
        !r100_at2_enemy(work,p)&&((dependency!=0x200&&dependency!=0x203)||work->room!=0x100))||
       !isfinite(distance)||distance<0)
        return stop(s,0x2130c8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]==0?BIND:p[0x2f6]==1?TURN:CHECK_PLAYER;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            const size_t row=r104?0x18f8:r101_armed?0x454:boss?0x10b0:em0a?0x830:
                (dependency==0x202||dependency==0x203)?0x650:0xb0;
            if(!r101_armed&&(word(p+0x6ec)||gh_enemy_timers_weapon_kind(&enemy->timers)))
                return stop(s,0x2133a4,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x213390,GH_ACTOR_MOTION_BOUNDS);
            const uint32_t mtb=word(resource->bytes+row),seq=word(resource->bytes+row+4);
            if(r104&&(mtb!=0x2b6100||seq!=0x2b8800))
                return stop(s,0x213390,GH_ACTOR_MOTION_UNSUPPORTED);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,mtb,seq,0,10,p[0x17c3]?2:0,
                !r101&&!r102&&!r105&&!gate&&!em0a&&!r101_armed&&!arena&&!r104);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        s->substate67_stage=SET_TIMER;
    }
    if(s->substate67_stage==SET_TIMER){
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        ++p[0x2f6];
        put(p+0x5f4,120+draw%60u);
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        float origin[3],target[3],turn;
        memcpy(target,p+0x16a0,12);
        const float yaw=number(word(p+0x104)),rate=number(word(p+0x5a8));
        const float work_rate=number(word((const uint8_t *)work+0x55c));
        fenv_t env;
        if(!position(actor,origin)||!isfinite(rate)||!isfinite(work_rate)||
            !set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        const float cap=mul(mul(rate,number(0x3dc90fdb)),work_rate);
        gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
            origin,target,yaw,cap,&turn,&pc);
        float next=status?0:add(yaw,turn);
        if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_id_angle_reference_001f7d48(next,&next))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(next));
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_phase=(half(p+0x3ac)&0x20u)!=0;
        s->substate67_stage=CHECK_NEAR;
    }
    if(s->substate67_stage==CHECK_NEAR){
        if(!s->substate67_phase){
            const unsigned heat=heat_category(s);
            if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            const uint32_t rank=word(p+0x16d8),near_rank=heat<3?heat:3;
            const float limit=rank>=6?121.0f:rank>=3?49.0f:rank<near_rank?2.25f:16.0f;
            s->substate67_stage=distance<limit?CHOOSE:CHECK_PLAYER;
        }else s->substate67_stage=CHECK_PLAYER;
    }
    if(s->substate67_stage==CHECK_PLAYER){
        const uint8_t *manager=init->enemies;
        if((int16_t)half(s->player->bytes+0x54a)<=0||manager[0x5b5]||manager[0x53c])
            s->substate67_stage=CHOOSE;
        else s->substate67_stage=s->substate67_phase?CHECK_DOOR:ATTACK;
    }
    if(s->substate67_stage==CHOOSE){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    if(s->substate67_stage==ATTACK){
        int chosen;
        gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
        if(status)return status;
        s->substate67_stage=chosen?5:CHECK_DOOR;
    }
    if(s->substate67_stage==CHECK_DOOR){
        if((int32_t)word(p+0x16ec)>0){
            enemy_decision(p,0,r104&&distance>64.0f?0xb:6,0);
            s->substate67_stage=5;
        }else{
            int selected;
            gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p,&selected);
            if(status)return status;
            s->substate67_stage=!selected&&(r101||r102||r105||r101_scenery||area11||arena||r104)?CHECK_JUMP:5;
        }
    }
    if(s->substate67_stage==CHECK_JUMP){
        gh_actor_motion_status status=enemy_door_jump_0024bda0(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status choose_devil_002705d8(gh_enemy202_move *,uint8_t *);

static gh_actor_motion_status advance_substate7_00213ea8(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=200, TURN, SAMPLE, CHOOSE_ENDED, ROTATE, TRANSLATE,
        CHECK_DISTANCE, CHOOSE_FAR, CHECK_LEVEL, ATTACK };
    const gh_enemy_move_actor *runtime=&s->runtime;
    gh_actor_motion *motion=runtime->motion;
    gh_boot_work *work=runtime->entry->b.work;
    const gh_title_player_view *actor=&runtime->entry->object.view;
    const float distance=number(word(p+0x618));
    uint32_t pc=0;
    const uint32_t dependency=word(p+0x564);
    const unsigned stamp=half(p+0x640);
    const int devil=!s->enemy&&work->room==0x100&&dependency==0x220&&
        half(p+0x2fe)==0x220&&stamp==0xff&&p[0x2f7]==1;
    if(!s->enemy&&!devil)return stop(s,0x213ea8,GH_ACTOR_MOTION_UNSUPPORTED);
    const int gate_em0a=r102_gate_em0a(work,p);
    const int arena_em0a=r103_arena_em0a(runtime->entry);
    const int em0a=(dependency==0x20a&&!stamp&&p[0x1650]==3)||gate_em0a;
    const int west_sprint=r100_west_em03_patrol(work,p);
    const int at3_em00=dependency==0x200&&r100_at3_enemy(work,p);
    const int r104=gh_enemy203_is_r104_entry(&runtime->entry->request,work->room);
    const int release_previous_sequence=devil||west_sprint||r100_at2_formation(work,p)||
        r100_at2_enemy(work,p)||at3_em00;
    const int r100=release_previous_sequence||(work->room==0x100&&
        (em0a||(dependency==0x203&&stamp==19)||(dependency==0x200&&
         (stamp==20||stamp==8||(stamp==10&&p[0x1650]==5&&word(p+0x16c4)==6)))));
    const gh_om_object *room_weapon=gh_enemy_timers_room_weapon(runtime->timers);
    const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(runtime->timers);
    const int wave=s->enemy&&r101_arena_airborne(s->enemy)&&
        ((stamp==0x16&&dependency==0x200)||(stamp==0x20&&dependency==0x203));
    const int r101=(r101_em00_stamp12(work->room,p)||r101_em00_stamp15(work->room,p)||
        r101_em02_stamp5(work->room,p)||wave)&&!p[0x2f7];
    const int r101_armed=work->room==0x101&&dependency==0x203&&stamp==13&&!p[0x1650]&&
        !p[0x2f7]&&held&&held->weapon&&held->weapon->kind==0x361;
    const int r102_armed=work->room==0x102&&dependency==0x200&&stamp==3&&p[0x1650]==1&&
        p[0x2f7]==1&&word(p+0x16c4)==8&&room_weapon&&room_weapon->runtime_kind==0x363;
    const int east_unarmed=work->room==0x102&&dependency==0x200&&stamp==3&&p[0x1650]==1&&
        !p[0x2f7]&&!room_weapon;
    const int r102=r102_armed||east_unarmed||(r102_area11_obstacle(work,p)&&word(p+0x16c4)==2)||
        (work->room==0x102&&dependency==0x200&&stamp==11&&p[0x1650]==5)||
        r102_area11_em02(work,p)||gate_em0a||r102_east_em27(work,p);
    const int boss=r103_arena_boss(runtime->entry);
    if((!r100&&!r101&&!r101_armed&&!r102&&!r104&&!r105_opening_enemy(work,p)&&
        !r103_arena_jumper(runtime->entry)&&!arena_em0a&&!boss)||
        !isfinite(distance)||distance<0)
        return stop(s,0x213ea8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]==0?BIND:p[0x2f6]==1?TURN:CHECK_DISTANCE;
    }
    if(s->substate67_stage==BIND){
        const gh_object_view *resource=&runtime->entry->resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            /* Dependency 0250 uses 435858 -> 214058 for its unarmed chase. */
            const size_t row=devil?0x1c00:(r102_armed||r101_armed)?0x45c:r104?0x1900:
                boss?0x10b8:(em0a||arena_em0a)?0x838:
                (dependency==0x202||dependency==0x203)?0x658:0xb8;
            if(word(p+0x6ec)||(!r102_armed&&!r101_armed&&gh_enemy_timers_weapon_kind(runtime->timers)))
                return stop(s,0x2140e8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2140e8,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,10,p[0x17c3]?2:0,release_previous_sequence||r104);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        ++p[0x2f6];
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        float origin[3],target[3],turn;
        const float yaw=number(word(p+0x104)),rate=number(word(p+0x5a8));
        const float work_rate=number(word((const uint8_t *)work+0x55c));
        fenv_t env;
        if(!position(actor,origin)||!position(s->player,target)||!isfinite(rate)||!isfinite(work_rate)||
            !set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        const float cap=mul(mul(rate,number(0x3dc90fdb)),work_rate);
        gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
            origin,target,yaw,cap,&turn,&pc);
        float next=status?0:add(yaw,turn);
        if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_id_angle_reference_001f7d48(next,&next))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(next));
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(!runtime->bindings->canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *runtime->bindings->canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?CHOOSE_ENDED:ROTATE;
    }
    if(s->substate67_stage==CHOOSE_ENDED){
        gh_actor_motion_status status=devil?choose_devil_002705d8(s,p):
            choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=(half(p+0x3ac)&0x20u)?5:CHECK_DISTANCE;
    }
    if(s->substate67_stage==CHECK_DISTANCE)
        s->substate67_stage=!p[0x2f7]&&distance>9.0f?CHOOSE_FAR:CHECK_LEVEL;
    if(s->substate67_stage==CHOOSE_FAR){
        gh_actor_motion_status status=devil?choose_devil_002705d8(s,p):
            choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    if(s->substate67_stage==CHECK_LEVEL){
        const unsigned level=heat_category(s);
        if(!level)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned limit=level==5?100:level>=3?3:level==2?2:1;
        s->substate67_stage=word(p+0x16d8)<limit?ATTACK:5;
    }
    if(s->substate67_stage==ATTACK){
        int chosen;
        gh_actor_motion_status status=devil?devil_near_choice_00262aa8(s,p,0,&chosen):
            attack_choice_00262aa8(s,p,&chosen);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_substate6_00213710(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_boot_work *work=enemy->b.entry.work;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    const float distance=number(word(p+0x618));
    uint32_t pc=0;
    const uint32_t dependency=word(p+0x564);
    const unsigned stamp=half(p+0x640);
    const int r100_em03=dependency==0x203&&
        ((stamp==19&&!p[0x1650])||(stamp==11&&p[0x1650]==5&&word(p+0x16c4)==6));
    const int gate_em0a=r102_gate_em0a(work,p);
    const int arena_em0a=r103_arena_em0a(&enemy->entry);
    const int em0a=arena_em0a||(dependency==0x20a&&!stamp&&p[0x1650]==3)||gate_em0a;
    const int r105=r105_opening_enemy(work,p)||r105_battle_enemy(work,p);
    const int r101=work->room==0x101&&dependency==0x203&&half(p+0x640)==13&&!p[0x1650];
    const int gate=r102_gate_reinforcement(work,p);
    const int r102_em03=r102_em03_stamp8(work,p);
    const int east_em27=r102_east_em27(work,p);
    const int r102=(work->room==0x102&&dependency==0x200)||r102_area11_em02(work,p)||gate||gate_em0a||r102_em03||east_em27;
    const int r101_unarmed=r101_em00_stamp12(work->room,p)||r101_em00_stamp15(work->room,p)||
        r101_em02_stamp5(work->room,p)||r101_gate_exchanged_enemy(work,p)||
        (r101_arena_airborne(enemy)&&(dependency==0x200||dependency==0x203)&&!p[0x2f7]);
    const int r101_scenery=r101_scenery_enemy(work,p);
    const int boss=r103_arena_boss(&enemy->entry);
    const int arena=r103_arena_jumper(&enemy->entry)||boss;
    const int r104=gh_enemy203_is_r104_entry(&enemy->entry.request,work->room);
    const int r100=r100_armed_em02(enemy,p);
    if((!r100&&!r101&&!r101_unarmed&&!r101_scenery&&!r102&&!r105&&!arena&&!arena_em0a&&!r104&&!r100_at2_enemy(work,p)&&
        ((dependency!=0x200&&!r100_em03&&!r100_at2_formation(work,p)&&!em0a)||work->room!=0x100))||
       !isfinite(distance)||distance<0)
        return stop(s,0x213710,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]==0?72:p[0x2f6]==1?74:79;
    }
    if(s->substate67_stage==72){
        const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&enemy->timers);
        if(word(p+0x6ec)||(held&&(!held->weapon||(!r101&&!r100)||
           held->weapon->kind!=(r100?0x363:0x361))))
            return stop(s,0x213a5c,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            /* The original table selects 10C0/10C4 for both middle bosses. */
            const size_t row=held?0x4b8:r104?0x1908:boss?0x10c0:em0a?0x840:
                (dependency==0x202||dependency==0x203)?0x660:0xc0;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x213a58,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,10,p[0x17c3]?2:0,!r105&&!gate&&!arena&&!r102_em03&&!east_em27&&!r101_unarmed);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5f0,90);
        s->substate67_stage=73;
    }
    if(s->substate67_stage==73){
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        ++p[0x2f6];
        put(p+0x5f4,120+draw%60u);
        s->substate67_stage=74;
    }
    if(s->substate67_stage==74){
        float origin[3],target[3],turn;
        memcpy(target,p+0x16a0,12);
        const float yaw=number(word(p+0x104)),rate=number(word(p+0x5a8));
        const float work_rate=number(word((const uint8_t *)work+0x55c));
        fenv_t env;
        if(!position(actor,origin)||!isfinite(rate)||!isfinite(work_rate)||
            !set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        const float cap=mul(mul(rate,number(0x3e860a92)),work_rate);
        gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
            origin,target,yaw,cap,&turn,&pc);
        float next=status?0:add(yaw,turn);
        if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_id_angle_reference_001f7d48(next,&next))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(next));
        s->substate67_stage=75;
    }
    if(s->substate67_stage==75){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=76;
    }
    if(s->substate67_stage==76){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=77;
    }
    if(s->substate67_stage==77){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=78;
    }
    if(s->substate67_stage==78){
        const unsigned heat=heat_category(s);
        if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint32_t rank=word(p+0x16d8),near_rank=heat<3?heat:3;
        const float limit=rank>5?121.0f:rank>=3?49.0f:rank<near_rank?2.25f:16.0f;
        if(distance<limit){
            gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
            if(status)return status;
        }
        const uint32_t remaining=word(p+0x5f0);
        if(remaining)put(p+0x5f0,remaining-1);
        else if(r104)enemy_decision(p,0,0x66,0);
        s->substate67_stage=79;
    }
    if(s->substate67_stage==79){
        const uint8_t *manager=enemy->entry.b.init->enemies;
        if((int16_t)half(s->player->bytes+0x54a)<=0||manager[0x5b5]||manager[0x53c]){
            gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
            if(status)return status;
            s->substate67_stage=5;
        }else{
            int chosen;
            gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
            if(status)return status;
            s->substate67_stage=chosen?5:80;
        }
    }
    if(s->substate67_stage==80){
        int selected;
        gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p,&selected);
        if(status)return status;
        /* Positive 24BB68 skips the original 24BDA0 call. */
        s->substate67_stage=!selected&&(r102||r105||r101_scenery||r101_unarmed||arena||r104)?81:5;
    }
    if(s->substate67_stage==81){
        gh_actor_motion_status status=enemy_door_jump_0024bda0(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Authored R102 EMS row16, stamp18. The last label in 218C48 makes its
 * input appearance4 after mode10; the first routes start in appearance0. */
static int r102_route_runner(const gh_boot_work *work,const uint8_t *p)
{
    return work->room==0x102&&word(p+0x564)==0x20f&&half(p+0x640)==0x12&&
        (p[0x1650]==0||p[0x1650]==4);
}

/* 21A218 and the 218C48 phase10/11 cooldown share the same EM0F clip
 * and turn toward the canonical player at the slower original angle. */
static int r102_mount_passenger(const gh_enemy_actor *actor)
{
    const gh_enemy_request *request=&actor->entry.request;
    return actor->b.entry.work->room==0x102&&request->appearance==6&&
        gh_enemy202_is_room_supported_profile(request,0x102);
}

static int r101_arena_airborne(const gh_enemy_actor *actor)
{
    if(!actor)return 0;
    const gh_enemy_request *request=&actor->entry.request;
    return actor->b.entry.work->room==0x101&&request->appearance==6&&
        gh_enemy202_is_room_supported_profile(request,0x101);
}

static gh_actor_motion_status body_give_00262750(gh_enemy202_move *,uint8_t *,unsigned);

static gh_actor_motion_status advance_airborne_wait_00210ee8(gh_enemy202_move *s,uint8_t *p)
{
    enum {BIND=1,SAMPLE,ROTATE,TRANSLATE,FINISH=5,SET_START=6};
    gh_enemy_actor *enemy=s->enemy;
    gh_boot_work *work=enemy->b.entry.work;
    gh_title_player_view *view=&enemy->entry.object.view;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_enemy_request *request=&enemy->entry.request;
    if(!r102_mount_passenger(enemy)&&!r101_arena_airborne(enemy))
        return stop(s,0x210ee8,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t pc=0;
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        if(!enemy->b.solids||gh_collision_solid_set_enabled_001351d8(enemy->b.solids,view,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=!p[0x2f6]?BIND:p[0x2f6]==1?SAMPLE:FINISH;
    }
    if(s->substate67_stage==BIND){
        const int held=word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers);
        size_t motion_row=0x648,sequence_row=0x9c;
        if(held){motion_row=0x44c;sequence_row=0x450;}
        else switch(request->dependency_kind){
        case 0x200: motion_row=0x98;break;
        case 0x20a: motion_row=0x828;sequence_row=0x82c;break;
        case 0x20f: motion_row=sequence_row=0xfbc;break;
        }
        const gh_object_view *source=&enemy->entry.resource;
        if(!source->bytes||!source->file_extent_known||source->file_extent>source->size||
           source->file_extent<motion_row+4||source->file_extent<sequence_row+4)
            return stop(s,0x21114c,GH_ACTOR_MOTION_BOUNDS);
        put(p+0x16d0,word(p+0x16d0)|0x8000u);
        if(held)p[0x2f7]=0;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(source->bytes+motion_row),word(source->bytes+sequence_row),
                0,0,p[0x17c3]?2:0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(!view->alias_f0)return stop(s,0x211160,GH_ACTOR_MOTION_ARGUMENT);
        memmove(p+0x5c0,view->alias_f0,12);
        ++p[0x2f6];
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(number(word(p+0x24c))>0.0f){
            if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
            int ended;
            gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
                *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
            if(status)return stop(s,pc?pc:0x157c18,status);
            s->substate67_stage=ROTATE;
        }else s->substate67_stage=SET_START;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(view,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(view,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        fenv_t env;
        if(!view->alias_f0||!set_round(&env,FE_TOWARDZERO))return stop(s,0x2111c8,GH_ACTOR_MOTION_CALLEE);
        const float x=add(mul(number(word(view->alias_f0)),0.99f),mul(number(word(p+0x5c0)),0.01f));
        const float z=add(mul(number(word(view->alias_f0+8)),0.99f),mul(number(word(p+0x5c8)),0.01f));
        if(fesetenv(&env)||!isfinite(x)||!isfinite(z))return stop(s,0x2111c8,GH_ACTOR_MOTION_CALLEE);
        put(view->alias_f0,bits(x));
        put(view->alias_f0+8,bits(z));
        s->substate67_stage=SET_START;
    }
    if(s->substate67_stage==SET_START){
        if((word(p+0x16d0)&0x20000000u)||number(word(p+0x16c0))>0.0f){
            if(!view->alias_f0)return stop(s,0x211218,GH_ACTOR_MOTION_ARGUMENT);
            memmove(p+0x6d0,view->alias_f0,12);
            put(p+0x6e0,word(p+0x104));
            enemy_decision(p,0,0x75,0);
        }
        s->substate67_stage=FINISH;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_substate75_00240548(gh_enemy202_move *s,uint8_t *p)
{
    enum {BIND_FALL=60,GIVE,ADVECT,SAMPLE_FALL,ROTATE_FALL,TRANSLATE_FALL,GROUND,
          BIND_LAND,SAMPLE_LAND,DELAY,CHOOSE,ROTATE_LAND,TRANSLATE_LAND};
    gh_enemy_actor *enemy=s->enemy;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->b.entry.work;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    const int arena_em0a=r103_arena_em0a(&enemy->entry);
    if((!r102_area11_obstacle(work,p)&&!r102_mount_passenger(enemy)&&!r101_arena_airborne(enemy)&&
        !r103_arena_jumper(&enemy->entry)&&!arena_em0a&&!r103_arena_boss(&enemy->entry))||
       !actor->alias_f0||!enemy->b.canonical_rate)
        return stop(s,0x240548,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        static const unsigned phases[]={BIND_FALL,GIVE,BIND_LAND,SAMPLE_LAND};
        if(p[0x2f6]>3)return stop(s,0x240548,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=phases[p[0x2f6]];
    }
    if(s->substate67_stage==BIND_FALL||s->substate67_stage==BIND_LAND){
        const int landed=s->substate67_stage==BIND_LAND;
        /* Original 240548 selects the EM0A obstacle-jump pair at 884/88C. */
        const unsigned dependency=word(p+0x564);
        const size_t base=dependency==0x20a?0x884u:dependency==0x20f?0xd60u:0x104u;
        const size_t row=base+(landed?8u:0u);
        const gh_object_view *source=&enemy->entry.resource;
        if(!source->bytes||!source->file_extent_known||source->file_extent>source->size||source->file_extent<row+8)
            return stop(s,0x240548,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            if(landed)memmove(p+0x1660,actor->alias_f0,12);
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x240548,GH_ACTOR_MOTION_CALLEE);
            for(size_t i=0;i<3;++i)put(p+0x580+4*i,bits(sub(number(word(p+0x6d0+4*i)),
                number(word(actor->alias_f0+4*i)))));
            if(fesetenv(&env))return stop(s,0x240548,GH_ACTOR_MOTION_CALLEE);
            put(p+0x104,word(p+0x6e0));
            const gh_actor_motion_status opened=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(source->bytes+row),word(source->bytes+row+4),0,3,p[0x17c3]?2:0,0);
            if(opened)return stop(s,call->frontier?call->frontier:0x2a8578,opened);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(!landed)put(p+0x600,0x41f00000u);
        ++p[0x2f6];
        s->substate67_stage=landed?SAMPLE_LAND:GIVE;
    }
    if(s->substate67_stage==GIVE){
        put(p+0x16d0,word(p+0x16d0)|0x1000u);
        const gh_actor_motion_status status=body_give_00262750(s,p,2);
        if(status)return status;
        s->substate67_stage=ADVECT;
    }
    if(s->substate67_stage==ADVECT){
        const float rate=number(word(p+0x5a8));
        const float time=number(word(p+0x600));
        fenv_t env;
        if(!isfinite(rate)||!isfinite(time)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x240878,GH_ACTOR_MOTION_UNSUPPORTED);
        float position[3],remaining[3];
        for(size_t i=0;i<3;++i){
            const float residual=number(word(p+0x580+4*i));
            const float delta=mul(mul(residual,0.1f),rate);
            position[i]=add(number(word(actor->alias_f0+4*i)),delta);
            remaining[i]=sub(residual,delta);
        }
        const float next_time=sub(time,rate);
        if(fesetenv(&env)||!isfinite(position[0])||!isfinite(position[1])||!isfinite(position[2]))
            return stop(s,0x240878,GH_ACTOR_MOTION_UNSUPPORTED);
        if(time>0.0f){p[0x617]=1;put(p+0x600,bits(next_time));}
        put(p+0x16d0,word(p+0x16d0)|0x30000u);
        memcpy(actor->alias_f0,position,sizeof(position));
        memcpy(p+0x580,remaining,sizeof(remaining));
        s->substate67_stage=SAMPLE_FALL;
    }
    if(s->substate67_stage==SAMPLE_FALL||s->substate67_stage==SAMPLE_LAND){
        const int landed=s->substate67_stage==SAMPLE_LAND;
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(!landed&&ended)p[0x2f6]=2;
        s->substate67_stage=landed?(ended?DELAY:ROTATE_LAND):ROTATE_FALL;
    }
    if(s->substate67_stage==DELAY){
        fenv_t env;
        uint32_t draw;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x31d000,GH_ACTOR_MOTION_CALLEE);
        const gh_init_status result=gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw);
        float delay=0;
        if(!result){
            const volatile float number31=(float)(draw>>1);
            const float fraction=mul(number31,0x1p-31f);
            delay=add(mul(mul(fraction,5.0f),30.0f),150.0f);
        }
        if(fesetenv(&env)||result)return stop(s,0x31d000,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16e4,bits(delay));
        s->substate67_stage=CHOOSE;
    }
    if(s->substate67_stage==CHOOSE){
        const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE_LAND;
    }
    if(s->substate67_stage==ROTATE_FALL||s->substate67_stage==ROTATE_LAND){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        ++s->substate67_stage;
    }
    if(s->substate67_stage==TRANSLATE_FALL||s->substate67_stage==TRANSLATE_LAND){
        const int landed=s->substate67_stage==TRANSLATE_LAND;
        if(gh_player_root_translation_scaled_0014f928(actor,work,landed?1.5f:1.0f,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=landed?5:GROUND;
    }
    if(s->substate67_stage==GROUND){
        if(half(p+0x3ac)&0x10u){
            if(!enemy->b.actors)return stop(s,0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
            float surface[4]={0,0,0,1};
            gh_scene_ray_request ray={.mask=2,.hit_position=surface,.segmented=1};
            memcpy(ray.from,p+0x490,12);
            memcpy(ray.to,actor->alias_f0,12);
            ray.from[3]=ray.to[3]=1;
            ray.from[1]=add(ray.from[1],0.5f);
            int hit;
            if(gh_scene_ray_query_00131e10(enemy->b.actors,&ray,&hit,&pc))
                return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit){put(actor->alias_f0+4,bits(surface[1]));p[0x2f6]=2;}
        }
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_r102_route_wait(gh_enemy202_move *s,uint8_t *p,int cooldown)
{
    enum { BIND=110, FACE, SAMPLE, ROTATE, TRANSLATE, CHECK_TIMER, CHOOSE };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_boot_work *work=enemy->b.entry.work;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    const uint32_t old_flags=word(p+0x16d0);
    uint32_t pc=0;
    if(!s->substate67_stage){
        put(p+0x16d0,old_flags|0x400u);
        if(p[0x2f6]==(cooldown?10:0))s->substate67_stage=BIND;
        else s->substate67_stage=p[0x2f6]==(cooldown?11:1)?FACE:CHECK_TIMER;
    }
    if(s->substate67_stage==BIND){
        const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&enemy->timers);
        const gh_object_view *resource=&enemy->entry.resource;
        const size_t row=word(p+0x6ec)||held?0x44c:0xcf8;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
            resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,cooldown?0x219d4c:0x21a434,GH_ACTOR_MOTION_BOUNDS);
        put(p+0x16d0,(word(p+0x16d0)&~0x8000u)|2u);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,10,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(cooldown)put_half(p+0x568,150);
        ++p[0x2f6];
        s->substate67_stage=FACE;
    }
    if(s->substate67_stage==FACE){
        float target[3],origin[3],turn;
        const float rate=number(word(p+0x5a8));
        const float work_rate=number(word((const uint8_t *)work+0x55c));
        const float yaw=number(word(p+0x104));
        fenv_t env;
        if(!position(actor,origin)||!position(s->player,target)||!isfinite(rate)||!isfinite(work_rate)||
           !set_round(&env,FE_TOWARDZERO))return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        const float cap=mul(mul(rate,number(0x3dc90fdb)),work_rate);
        const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
            origin,target,yaw,cap,&turn,&pc);
        float result=status?yaw:add(yaw,turn);
        if(fesetenv(&env)||status||gh_id_angle_reference_001f7d48(result,&result))
            return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(result));
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=CHECK_TIMER;
    }
    if(s->substate67_stage==CHECK_TIMER){
        const float remaining=number(word(p+0x16c0));
        if(!isfinite(remaining))return stop(s,0x26ea28,GH_ACTOR_MOTION_UNSUPPORTED);
        if(cooldown){
            const uint16_t frames=(uint16_t)(half(p+0x568)-1u);
            put_half(p+0x568,frames);
            if(frames>INT16_MAX){
                put(p+0x16d4,word(p+0x16d4)|1u);
                s->substate67_stage=CHOOSE;
            }else if(remaining>0)p[0x2f6]=0;
        }else if(remaining>0)enemy_decision(p,0,0x13,0);
        else if(word(p+0x16d0)&0x20000000u)s->substate67_stage=CHOOSE;
        if(s->substate67_stage==CHECK_TIMER)s->substate67_stage=5;
    }
    if(s->substate67_stage==CHOOSE){
        const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_substate13_00218c48(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_boot_work *work=enemy->b.entry.work;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    const uint32_t dependency=word(p+0x564);
    const unsigned stamp=half(p+0x640);
    const uint32_t mode=word(p+0x16c4);
    const int r105=mode==2&&r105_battle_enemy(work,p);
    const int area11=work->room==0x102&&mode==2&&
        ((stamp==0x17&&dependency==0x202)||(stamp==7&&dependency==0x200));
    const int gate=r102_gate_reinforcement(work,p);
    const int arena=gh_enemy204_is_r103_entry(&enemy->entry.request,work->room);
    const int arena_boss=arena&&stamp<2&&mode==3;
    const int arena_reinforcement=arena&&stamp>=2&&mode==2;
    /* AT02 uses its original approach and both west waves can walk to their camera destination. */
    const int at2=r100_at2_enemy(work,p);
    const int camera_walk=(at2||r100_at3_enemy(work,p))&&mode==0;
    const int approach=r105||area11||gate||arena_boss||arena_reinforcement||(at2&&mode==2);
    const int r102=r102_route_runner(work,p)&&(mode==0||mode==10||mode==11||mode==12);
    if(r102&&(s->substate67_stage>=110||(s->substate67_stage==0&&(p[0x2f6]==10||p[0x2f6]==11))))
        return advance_r102_route_wait(s,p,1);
    const int em00=dependency==0x200&&((stamp==10&&p[0x1650]==5)||
        (stamp==3&&p[0x1650]==2)||(stamp==20&&!p[0x1650]));
    const int em02_home=dependency==0x202&&((stamp==4&&p[0x1650]==2)||at2);
    const int em0a=dependency==0x20a&&stamp==0&&p[0x1650]==3;
    const int em0a_formation=em0a&&mode==9;
    const int r102_em00=work->room==0x102&&dependency==0x200;
    const int east_em27_home=mode==6&&r102_east_em27(work,p);
    const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&enemy->timers);
    const int r101_armed=mode==6&&r101_em03_stamp13(work->room,p)&&
        held&&held->weapon&&held->weapon->kind==0x361;
    const int r101_home=mode==6&&(r101_em00_stamp12(work->room,p)||r101_em00_stamp15(work->room,p)||r101_em02_stamp5(work->room,p)||r101_armed);
    const int r102_home=mode==6&&(r102_em00||east_em27_home||r102_gate_actor(work,p)||r102_at0b_em02(work,p));
    const int r102_pickup=r102_em00&&mode==8;
    if(!r102&&!approach&&!camera_walk&&!r101_home&&!r102_home&&!r102_pickup&&(work->room!=0x100||
       (!em0a_formation&&(mode!=6||(dependency!=0x203&&!em00&&!em02_home&&!em0a)))))
        return stop(s,0x218c48,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,(word(p+0x16d0)&~0x8000u)|0x400u);
        if(p[0x2f6]>1||p[0x2f7])
            return stop(s,0x218ca8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=p[0x2f6]?95:94;
    }
    if(s->substate67_stage==94){
        if(!r102&&(word(p+0x6ec)||(!r101_armed&&gh_enemy_timers_weapon_kind(&enemy->timers))))
            return stop(s,0x219364,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            /* Modes 0, 2, 6 and 8 use the same dependency table. Mode 3
             * uses 10B0/10B4 for the EM14 and EM15 walking animation. */
            size_t row=dependency==0x200?0xc0:(em0a||(arena_reinforcement&&dependency==0x20a))?0x840:0x660;
            if(arena_boss)row=0x10b0;
            if(r101_armed)row=0x4b8;
            if(r102)row=held?(held->weapon->kind==0x3b7?0x4f4:0x4b8):word(p+0x6ec)?0x4b8:0xd10;
            /* 436774 dispatches EM27 to 218D5C and its C0/C8 pair. */
            if(east_em27_home)row=0xc0;
            const size_t candidate=row+(east_em27_home?8:4);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<candidate+4||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x219358,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+candidate),
                0,10,p[0x17c3]?2:0,!r101_home);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5f0,90);
        p[0x2f6]=1;
        s->substate67_stage=95;
    }
    if(s->substate67_stage==95){
        float origin[3],target[3],angle;
        memcpy(target,p+(mode==12?0x16b0:0x16a0),12);
        const float yaw=number(word(p+0x104));
        if(!position(actor,origin)||
            gh_player_target_angle_002da038(s->elf,s->elf_bytes,origin,target,yaw,
                number(0x40490fdb),&angle,&pc))
            return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(ordered_abs(angle)>number(0x3fc90fdb)){
            enemy_decision(p,0,8,1);
            put(p+0x16d4,word(p+0x16d4)|1u);
        }else{
            const float rate=number(word(p+0x5a8));
            const float work_rate=number(word((const uint8_t *)work+0x55c));
            fenv_t env;
            if(!isfinite(rate)||!isfinite(work_rate)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float cap=mul(mul(rate,number(0x3ec90fdb)),work_rate);
            gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
                origin,target,yaw,cap,&angle,&pc);
            float next=status?0:add(yaw,angle);
            if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x104,bits(next));
        }
        s->substate67_stage=96;
    }
    if(s->substate67_stage==96){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=97;
    }
    if(s->substate67_stage==97){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=98;
    }
    if(s->substate67_stage==98){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=99;
    }
    if(s->substate67_stage==99){
        float origin[3],home[3];
        memcpy(home,p+0x16b0,12);
        const float timer=number(word(p+0x16c0));
        if(!position(actor,origin)||!isfinite(home[0])||!isfinite(home[1])||
            !isfinite(home[2])||!isfinite(timer))
            return stop(s,0x1f7eb0,GH_ACTOR_MOTION_UNSUPPORTED);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x1f7eb0,GH_ACTOR_MOTION_UNSUPPORTED);
        const int arrived=distance_xz(home,origin)<1.0f&&ordered_abs(sub(home[1],origin[1]))<1.0f;
        if(fesetenv(&env))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(timer<=0||arrived){
            put(p+0x16c0,0);
            if(r102_pickup&&timer>0){
                enemy_decision(p,0,0x73,0);
                s->substate67_stage=5;
            }else if(approach||camera_walk||r102_pickup||(r102&&mode==0)){
                put(p+0x16d4,word(p+0x16d4)|1u);
                s->substate67_stage=104;
            }else if(r102){
                if(mode==12){
                    p[0x2f4]=0;
                    p[0x2f5]=0x13;
                    p[0x2f6]=10;
                }else{
                    put(p+0x16d0,word(p+0x16d0)&~0x20000000u);
                    if(mode==10)p[0x1650]=4;
                    enemy_decision(p,0,mode==10?1:0x14,0);
                }
                s->substate67_stage=5;
            }else{
                if(em0a_formation)enemy_decision(p,0,0x66,2);
                else{
                    enemy_decision(p,0,3,0);
                    put(p+0x16d0,word(p+0x16d0)&~2u);
                }
                s->substate67_stage=5;
            }
        }else s->substate67_stage=approach?101:100;
    }
    if(s->substate67_stage==101){
        const float distance=number(word(p+0x618));
        if(!isfinite(distance)||distance<0)return stop(s,0x219eac,GH_ACTOR_MOTION_UNSUPPORTED);
        if(distance<25){
            gh_scene_ray_request ray={.mask=7,.segmented=1};
            if(!position(actor,ray.from)||!position(s->player,ray.to))
                return stop(s,0x219ed0,GH_ACTOR_MOTION_UNSUPPORTED);
            ray.from[3]=ray.to[3]=1;
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x131e10,GH_ACTOR_MOTION_CALLEE);
            ray.from[1]=add(ray.from[1],0.5f);
            ray.to[1]=add(ray.to[1],0.5f);
            if(fesetenv(&env))return stop(s,0x131e10,GH_ACTOR_MOTION_CALLEE);
            int hit;
            if(!enemy->b.actors||gh_scene_ray_query_00131e10(enemy->b.actors,&ray,&hit,&pc))
                return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!hit){
                put(p+0x16c0,0);
                put(p+0x16d0,word(p+0x16d0)|2u);
                put(p+0x16d4,word(p+0x16d4)|1u);
                s->substate67_stage=102;
            }
        }
        if(s->substate67_stage==101)s->substate67_stage=100;
    }
    if(s->substate67_stage==102||s->substate67_stage==104){
        const int arrived=s->substate67_stage==104;
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=arrived?5:100;
    }
    if(s->substate67_stage==100){
        int selected;
        gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p,&selected);
        if(status)return status;
        s->substate67_stage=!selected&&(r102_home||r102_pickup||approach||camera_walk||r102)?103:5;
    }
    if(s->substate67_stage==103){
        gh_actor_motion_status status=enemy_door_jump_0024bda0(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original 23F970, phase 0 selects the pickup motion then falls through
 * to its first sample. A completed clip still applies both root deltas. */
static gh_actor_motion_status advance_ground_pickup_0023f970(gh_enemy202_move *s,uint8_t *p)
{
    enum {OPEN=110,SAMPLE,DECIDE,ROTATE,TRANSLATE,PICKUP,ATTACH};
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_boot_work *work=enemy->b.entry.work;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    if(work->room!=0x102||word(p+0x564)!=0x200)
        return stop(s,0x23f970,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage)
        s->substate67_stage=p[0x2f6]==0?OPEN:p[0x2f6]==1?SAMPLE:PICKUP;
    if(s->substate67_stage==OPEN){
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const size_t row=0x488;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x23f98c,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,3,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        p[0x2f6]=1;
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?DECIDE:ROTATE;
    }
    if(s->substate67_stage==DECIDE){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=PICKUP;
    }
    if(s->substate67_stage==PICKUP){
        if(!word(p+0x6ec)&&!gh_enemy_timers_weapon_kind(&enemy->timers)&&(half(p+0x3ac)&1u)){
            float target[3];
            gh_player_target found;
            gh_actor_motion_status status=find_ground_weapon_002ddab0(s,2.25f,1,&found,target);
            if(status)return status;
            s->pickup_weapon_handle=found.handle;
        }
        s->substate67_stage=s->pickup_weapon_handle?ATTACH:5;
    }
    if(s->substate67_stage==ATTACH){
        gh_actor_motion_status status=gh_enemy_timers_pickup_room_weapon_0026ab20(&enemy->timers,
            &enemy->entry,s->projectile_factory,s->pickup_weapon_handle,enemy->b.actors,enemy->b.weapon_scroll);
        if(status)return stop(s,enemy->timers.frontier?enemy->timers.frontier:0x26ab20,status);
        s->pickup_weapon_handle=0;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original 214DE0 rear evade, default EM00 EC/F0 row. */
static gh_actor_motion_status advance_rear_evade_00214de0(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=240, TURN, SAMPLE, CHOOSE, ATTACK, RESET, ROTATE, TRANSLATE };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->b.entry.work;
    const unsigned stamp=half(p+0x640);
    uint32_t pc=0;
    const int opening=work->room==0x100&&word(p+0x564)==0x200&&(stamp==8||stamp==20||r100_at2_formation(work,p)||r100_at3_enemy(work,p));
    const int arena=r103_arena_em00(&enemy->entry);
    const int boss=r103_arena_boss(&enemy->entry);
    const int r104=gh_enemy203_is_r104_entry(&enemy->entry.request,work->room);
    if((!opening&&!arena&&!boss&&!r104)||p[0x2f7]!=1)
        return stop(s,0x214de0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        if(p[0x2f6]>1)return advance_state0_tail(s,p);
        s->substate67_stage=p[0x2f6]?TURN:BIND;
    }
    if(s->substate67_stage==BIND){
        if(word(p+0x6e4)||word(p+0x6e8))return stop(s,0x26a938,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x6f0))return stop(s,0x1d04d0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x708))return stop(s,0x1d05c0,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            const size_t row=r104?0x194cu:0xecu;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2151e4,GH_ACTOR_MOTION_BOUNDS);
            const uint32_t mtb=word(resource->bytes+row),seq=word(resource->bytes+row+4);
            if(r104&&(mtb!=0x2c8540||seq!=0x2c9900))
                return stop(s,0x2151e4,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x604,r104?0x3f800000:0x3f19999a);
            put(p+0x600,r104?0x40e00000:0x40a00000);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,mtb,seq,0,3,0x40u|(!r104&&p[0x17c3]?2u:0u),1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x16e0,0);
        p[0x2f6]=1;
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        float from[3],to[3],delta;
        const float rate=number(word(p+0x5a8)),yaw=number(word(p+0x104));
        const float frame_rate=number(word((const uint8_t *)work+0x55c)),timer=number(word(p+0x600));
        fenv_t env;
        if(!position(actor,from)||!position(s->player,to)||!isfinite(rate)||!isfinite(frame_rate)||
           !isfinite(timer)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        const float cap=mul(mul(rate,number(0x3e490fdb)),frame_rate);
        gh_player_target_status status=gh_player_target_angle_002da038(
            s->elf,s->elf_bytes,from,to,yaw,cap,&delta,&pc);
        float next=status?yaw:add(yaw,delta);
        const float remaining=timer>0?sub(timer,rate):timer;
        if(fesetenv(&env)||status||!isfinite(remaining))
            return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_id_angle_reference_001f7d48(next,&next))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(next));
        if(timer>0){
            put(p+0x600,bits(remaining));
            if(remaining<=0)put(p+0x54c,0x41700000);
        }
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        const float resistance=number(word(p+0x17cc));
        int ended;
        if(!isfinite(resistance))return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended||(resistance<=0&&(half(p+0x3ac)&0x10u))?CHOOSE:ROTATE;
    }
    if(s->substate67_stage==CHOOSE&&r104){
        /* The dep0250 tail at 215960 draws only beyond the original radius. */
        const float distance=number(word(p+0x618));
        if(!isfinite(distance))return stop(s,0x215960,GH_ACTOR_MOTION_BOUNDS);
        if(distance>9.0f){
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            enemy_decision(p,0,(draw&1u)?0x20:0x19,0);
        }
        s->substate67_stage=RESET;
    }
    if(s->substate67_stage==CHOOSE){
        if(boss){
            /* 294970 reads the native enemy manager's original pairing timer. */
            const float paired=number(word(enemy->entry.b.init->enemies+0x544));
            const float distance=number(word(p+0x618));
            if(!isfinite(paired)||!isfinite(distance))
                return stop(s,0x294970,GH_ACTOR_MOTION_BOUNDS);
            if(paired>0&&distance<36.0f){
                enemy_decision(p,0,distance<9.0f?0xa:0x17,1);
                s->substate67_stage=5;
                return advance_state0_tail(s,p);
            }
        }
        uint32_t random;
        if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&random))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(random&1u){
            enemy_decision(p,0,0x16,0);
            s->substate67_stage=RESET;
        }else s->substate67_stage=ATTACK;
    }
    if(s->substate67_stage==ATTACK){
        int chosen;
        gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
        if(status)return status;
        s->substate67_stage=RESET;
    }
    if(s->substate67_stage==RESET){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_scaled_0014f928(actor,work,number(word(p+0x604)),
              GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original 23AFD8, ordinary R100 stamp19 recovery on the shared EM02 model. */

/* R100 AT1's EM0A formation selects variant two, a fixed motion/SEQ pair. */
static gh_actor_motion_status advance_em0a_substate66_0023a950(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=201, SAMPLE, DECIDE, ROTATE, TRANSLATE, GIVE };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    gh_boot_work *work=enemy->b.entry.work;
    uint32_t pc=0;
    if(work->room!=0x100||word(p+0x564)!=0x20a||half(p+0x640)!=0||
       p[0x1650]!=3||(!s->substate67_stage&&(p[0x2f7]!=2||p[0x2f6]>1)))
        return stop(s,0x23a950,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]?SAMPLE:BIND;
    }
    if(s->substate67_stage==BIND){
        const gh_object_view *resource=&enemy->entry.resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0xc2c||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x23a950,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+0xc20),word(resource->bytes+0xc28),
                0,3,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        p[0x2f6]=1;
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?DECIDE:ROTATE;
    }
    if(s->substate67_stage==DECIDE){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=GIVE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=GIVE;
    }
    if(half(p+0x3ac)&1u){
        gh_actor_motion_status status=body_give_00262750(s,p,0);
        if(status)return status;
    }
    s->substate67_stage=5;
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_substate69_0023afd8(gh_enemy202_move *s,uint8_t *p)
{
    enum { OPEN=101, OPEN_RANDOM, TURN, TICK, IMPULSE, ROTATE, TRANSLATE,
           TIMER, DECIDE, OVERRIDE, FINISH, TICK_EXIT };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    gh_boot_work *work=enemy->b.entry.work;
    uint32_t pc=0;
    const unsigned dependency=word(p+0x564),stamp=half(p+0x640);
    const int r100=work->room==0x100&&
        ((dependency==0x203&&stamp==19)||(dependency==0x200&&(stamp==8||stamp==20)));
    const int r102=work->room==0x102&&dependency==0x200&&
        ((stamp==0x0b&&p[0x1650]==5)||(stamp==0x10&&!p[0x1650]));
    const int west_counter=r100_at2_enemy(work,p)||r100_at3_enemy(work,p)||
        ((dependency==0x202||dependency==0x203)&&r100_at2_formation(work,p));
    const int r101=work->room==0x101&&p[0x1650]==6&&
        ((dependency==0x200&&stamp==0x16)||(dependency==0x203&&stamp==0x20));
    const int em0a=r102_gate_em0a(work,p);
    const int em03=r102_em03_stamp8(work,p);
    const int arena_em0a=r103_arena_em0a(&enemy->entry);
    const int boss=r103_arena_boss(&enemy->entry);
    const int arena=r103_arena_em00(&enemy->entry)||r103_arena_em03(&enemy->entry)||arena_em0a||boss;
    const int east_em00=work->room==0x102&&dependency==0x200&&stamp==3&&p[0x1650]==1;
    const int at0b_em02=r102_area11_em02(work,p);
    if((!r100&&!r101&&!r102&&!em0a&&!arena&&!em03&&!east_em00&&!at0b_em02&&!west_counter)||
       p[0x2f7]>(unsigned)(r101||em0a||em03||arena_em0a||east_em00||at0b_em02||boss||west_counter))
        return stop(s,0x23afd8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        if(p[0x2f6]>3)return stop(s,0x23afd8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!p[0x2f6])p[0x1864]=0;
        s->substate67_stage=p[0x2f6]==1?TURN:p[0x2f6]==3?TICK_EXIT:OPEN;
    }
    if(s->substate67_stage==OPEN){
        /* 020A uses the original 43A660/43A854 opening and exit rows. */
        const size_t offset=boss?(p[0x2f6]?0x1b88:0x1b80):
            (em0a||arena_em0a)?(p[0x2f6]?0xb7c:0xb74):
            (p[0x2f6]?0x3a4:(dependency==0x200?0x39c:0x7c4));
        const gh_object_view *resource=&enemy->entry.resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<offset+8||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x23afd8,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=resource->bytes+offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(row),word(row+4),0,p[0x2f6]?10:3,p[0x17c3]?2:0,!(r101||east_em00));
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(p[0x2f6]){
            put(p+0x600,bits(10.0f));
            p[0x2f6]=3;
            s->substate67_stage=TICK_EXIT;
        }else s->substate67_stage=OPEN_RANDOM;
    }
    if(s->substate67_stage==OPEN_RANDOM){
        uint32_t first,second;
        gh_game_mt random=enemy->b.entry.init->random;
        if(gh_game_mt_next_0030f348(&random,&first)||gh_game_mt_next_0030f348(&random,&second))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        enemy->b.entry.init->random=random;
        put(p+0x5f0,(second&3u)?20u:first%30u+30u);
        put(p+0x604,p[0x2f7]?0xbe99999au:0);
        s->substate67_stage=FINISH;
    }
    if(s->substate67_stage==FINISH){
        if(word(p+0x6f0))return stop(s,0x1d04d0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x708))return stop(s,0x1d05c0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x5f0,5);
        put(p+0x600,bits(15.0f));
        p[0x2f6]=1;
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        put(p+0x16d0,word(p+0x16d0)|0x2000u);
        const uint32_t turns=word(p+0x5f0);
        if(turns){
            float from[3],to[3],turn;
            const float yaw=number(word(p+0x104)),rate=number(word(p+0x5a8));
            const float frame_rate=number(word((const uint8_t *)work+0x55c));
            fenv_t env;
            if(!position(actor,from)||!position(s->player,to)||!isfinite(rate)||!isfinite(frame_rate)||
                !set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float cap=mul(mul(rate,number(0x3ec90fdb)),frame_rate);
            gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
                from,to,yaw,cap,&turn,&pc);
            float next=status?0:add(yaw,turn);
            if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x104,bits(next));
            put(p+0x5f0,turns-1);
        }
        s->substate67_stage=TICK;
    }
    if(s->substate67_stage==TICK_EXIT){
        const float timer=number(word(p+0x600)),rate=number(word(p+0x5a8));
        if(!isfinite(timer)||!isfinite(rate))return stop(s,0x23afd8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(timer>0){
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x23afd8,GH_ACTOR_MOTION_CALLEE);
            const float remaining=sub(timer,rate);
            if(fesetenv(&env)||!isfinite(remaining))
                return stop(s,0x23afd8,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x600,bits(remaining));
            put(p+0x16d0,word(p+0x16d0)|0x2000u);
        }
        s->substate67_stage=TICK;
    }
    if(s->substate67_stage==TICK){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=p[0x2f6]==1?IMPULSE:ended?DECIDE:ROTATE;
    }
    if(s->substate67_stage==IMPULSE){
        const float offset=number(word(p+0x338)),speed=number(word(p+0x604));
        const float rate=number(word(p+0x5a8));
        fenv_t env;
        if(!isfinite(offset)||!isfinite(speed)||!isfinite(rate)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x23b350,GH_ACTOR_MOTION_UNSUPPORTED);
        const float next=add(offset,mul(speed,rate));
        const float decay=mul(speed,sub(1.0f,mul(rate,0.3f)));
        if(fesetenv(&env)||!isfinite(next)||!isfinite(decay))
            return stop(s,0x23b350,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x338,bits(next));
        put(p+0x604,bits(decay));
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=p[0x2f6]==1?TIMER:5;
    }
    if(s->substate67_stage==TIMER){
        const float gate=number(word(p+0x173c)),timer=number(word(p+0x600));
        const float rate=number(word(p+0x5a8));
        if(!isfinite(gate)||!isfinite(timer)||!isfinite(rate))
            return stop(s,0x23b39c,GH_ACTOR_MOTION_UNSUPPORTED);
        unsigned next=OVERRIDE;
        if(gate<=0){
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x23b39c,GH_ACTOR_MOTION_CALLEE);
            const float remaining=sub(timer,rate);
            if(fesetenv(&env)||!isfinite(remaining))return stop(s,0x23b39c,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x600,bits(remaining));
            if(remaining<=0)next=DECIDE;
        }
        s->substate67_stage=next;
    }
    if(s->substate67_stage==DECIDE){
        if(p[0x2f6]==3){
            put(p+0x16dc,0);
            gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
            if(status)return status;
            s->substate67_stage=5;
        }else{
            int chosen;
            gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
            if(status)return status;
            if(!chosen)p[0x2f6]=2;
            s->substate67_stage=chosen?5:OVERRIDE;
        }
    }
    if(s->substate67_stage==OVERRIDE){
        const float timer=number(word(p+0x1734));
        if(!isfinite(timer))return stop(s,0x274150,GH_ACTOR_MOTION_UNSUPPORTED);
        /* 274150 returns zero for both rooms using this profile. */
        if(timer>0)enemy_decision(p,0,0x89,0);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status choose_devil_002705d8(gh_enemy202_move *,uint8_t *);

static gh_actor_motion_status triangle_rear_clear_00273c38(gh_enemy202_move *s,
    const uint8_t *p,int *clear);

/* Original 23D7D0 after EM15's frontal recovery guard. */
static gh_actor_motion_status advance_boss_deflect_0023d7d0(gh_enemy202_move *s,uint8_t *p)
{
    enum { OPEN=101, IMPACT, SOUND, GIVE, TURN, SAMPLE, DECIDE, ROTATE, TRANSLATE };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->entry.b.work;
    uint32_t pc=0;
    if(!r103_arena_boss(&enemy->entry)||p[0x2f7]||!enemy->b.canonical_rate)
        return stop(s,0x23d7d0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        if(p[0x2f6]>1)s->substate67_stage=5;
        else if(p[0x2f6])s->substate67_stage=TURN;
        else{
            p[0x1864]=0;
            s->substate67_stage=OPEN;
        }
    }
    if(s->substate67_stage==OPEN){
        const gh_object_view *resource=&enemy->entry.resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x1b98||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x23da08,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+0x1b90),word(resource->bytes+0x1b94),
                0,1,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        s->substate67_stage=IMPACT;
    }
    if(s->substate67_stage==IMPACT){
        if(word(p+0x6f0))return stop(s,0x1d04d0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x708))return stop(s,0x1d05c0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(p[0x2b4]>=5){
            if(!s->impact_create.active){
                const gh_actor_part *part;
                uint32_t xyz[3];
                if(gh_actor_model_part_at(motion->b.model,4,&part)||!part->alias_f0)
                    return stop(s,0x14bfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                memcpy(xyz,part->alias_f0,12);
                if(gh_effect_create_enemy_deflect_begin_00306838(&s->impact_create,xyz))
                    return stop(s,0x306838,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            gh_effect_status effect=gh_game_effect_create_00306528(&s->impact_create,&s->impact_bindings);
            if(effect||!s->impact_create.complete)
                return stop(s,s->impact_create.frontier?s->impact_create.frontier:0x306838,
                    GH_ACTOR_MOTION_UNSUPPORTED);
            s->impact_create=(gh_effect_create_call){0};
        }
        s->substate67_stage=SOUND;
    }
    if(s->substate67_stage==SOUND){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request cue={.model=motion->b.model,.actor=actor,.bank=0,.program=0xf6};
        uint32_t handle;
        gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&cue,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->substate67_stage=GIVE;
    }
    if(s->substate67_stage==GIVE){
        gh_hitarea_object *give=&enemy->hitareas.units[3];
        const gh_game_math_globals *math=enemy->hitareas.b.math;
        if(!enemy->hitareas.created[3]||give->fallback||!math||!math->initialized)
            return stop(s,0x262980,GH_ACTOR_MOTION_UNSUPPORTED);
        put_half(give->bytes+0x46,0x49);
        if(gh_hitarea_object_set_node_scalar_001fd7b0(give,(int16_t)half(p+0x684),0x40000000u,&pc))
            return stop(s,pc?pc:0x1fd7b0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(give->bytes+0x4c,0);
        if(gh_hitarea_object_set_active_001fd668(give,1,&pc))
            return stop(s,pc?pc:0x1fd668,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(give->bytes+0x10,math->zero_vector,12);
        put(give->bytes+0x40,UINT32_MAX);
        put_half(give->bytes+0x44,0);
        const gh_title_player_view *source=NULL;
        float next[3];
        if(gh_hitarea_object_last_hit_actor(&enemy->hitareas.units[0],&source))
            return stop(s,0x23db24,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x5f0,0);
        if(source){
            if(!position(source,next))return stop(s,0x23db24,GH_ACTOR_MOTION_UNSUPPORTED);
            memcpy(p+0x5d0,next,12);
            put(p+0x5f0,10);
        }
        p[0x2f6]=1;
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        const uint32_t remaining=word(p+0x5f0);
        if(remaining){
            float from[3],to[3],angle,next;
            const float yaw=number(word(p+0x104));
            const float limit=mul(number(word(p+0x5a8)),0.3926991f);
            memcpy(to,p+0x5d0,12);
            if(!position(actor,from)||gh_player_target_angle_002da038(
                s->elf,s->elf_bytes,from,to,yaw,limit,&angle,&pc))
                return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(add(yaw,angle),&next))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x104,bits(next));
            put(p+0x5f0,remaining-1);
        }
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended||(half(p+0x3ac)&0x10u)?DECIDE:ROTATE;
    }
    if(s->substate67_stage==DECIDE){
        if(heat_category(s)!=1||word(p+0x6ec))
            return stop(s,0x23e5e8,GH_ACTOR_MOTION_UNSUPPORTED);
        put_half(p+0x17ca,30);
        put(p+0x16dc,0);
        gh_game_mt random=enemy->entry.b.init->random;
        uint32_t dodge,grab;
        if(gh_game_mt_next_0030f348(&random,&dodge))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        unsigned selector=0x15,variant=0;
        const uint32_t route=word(p+0x16d4);
        if(!(dodge&3u)&&(word(p+0x16d0)&0x200000u)&&(route&0x8000000u)){
            /* 268068 admits two rear evasion choices for 214/215 at rank 1. */
            const float face=number(word(p+0x760));
            const float distance=number(word(p+0x618));
            if(!isfinite(face)||!isfinite(distance))return stop(s,0x268068,GH_ACTOR_MOTION_BOUNDS);
            if(face>1.3089969f){
                if(gh_game_mt_next_0030f348(&random,&dodge))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(dodge&1u){
                    selector=8;variant=1;
                    put(p+0x16d4,route|1u);
                }
            }
            if(selector==0x15&&p[0x17b9]>=2&&p[0x17b9]<4&&distance<6.25f){
                if(gh_game_mt_next_0030f348(&random,&dodge))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if((dodge&3u)==1){
                    int clear;
                    gh_actor_motion_status status=triangle_rear_clear_00273c38(s,p,&clear);
                    if(status)return status;
                    if(clear){selector=10;variant=1;}
                }
            }
        }
        if(selector==0x15){
            if(gh_game_mt_next_0030f348(&random,&grab))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!(grab&7u)){
                float a[3],b[3];
                if(!position(actor,a)||!position(s->player,b))return stop(s,0x270d00,GH_ACTOR_MOTION_UNSUPPORTED);
                if(fabsf(sub(a[1],b[1]))<=0.1f)selector=0x4b;
            }
        }
        enemy->entry.b.init->random=random;
        enemy_decision(p,0,selector,variant);
        s->substate67_stage=selector==0x15?ROTATE:5;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Shared 23B968/23C620 recovery dispatch, native phase 0/1. */
static gh_actor_motion_status advance_substate6b_or_6c(gh_enemy202_move *s,uint8_t *p)
{
    enum { SELECT_MOTION=101, START_MOTION, TURN_TO_PLAYER, STEP_MOTION,
           SELECT_AFTER, ROOT_ROTATION, ROOT_TRANSLATION, CUE, CUE_SOUND };
    const gh_enemy_actor *enemy=s->enemy;
    const gh_enemy_move_actor *runtime=&s->runtime;
    gh_actor_motion *motion=runtime->motion;
    gh_boot_work *work=runtime->entry->b.work;
    gh_title_player_view *actor=&runtime->entry->object.view;
    uint32_t pc=0;
    const uint32_t dependency=word(p+0x564);
    const unsigned stamp=half(p+0x640);
    const int opening_actor=(dependency==0x203&&stamp==19)||(dependency==0x200&&stamp==20);
    const int substate6c=s->pending_ai_pc==0x23c620;
    const int devil=!enemy&&!substate6c&&work->room==0x100&&dependency==0x220&&
        half(p+0x2fe)==0x220&&stamp==0xff&&!p[0x2f7];
    if(!enemy&&!devil)return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
    /* Patched SLUS 23C680 underflows dep0200 into the same default table.
     * The stamp8 unarmed bind reaches 23D134 with t1 zero. */
    const int stamp8_6c=substate6c&&dependency==0x200&&stamp==8&&p[0x1650]==5;
    /* The west EM00 follows the same 23C6A4 default and releases its
     * previous attack SEQ at the original 23D134 bind. */
    const int formation_em00_6c=substate6c&&dependency==0x200&&r100_at2_formation(work,p);
    const int r102_em00_6c=substate6c&&dependency==0x200&&work->room==0x102&&
        stamp==11&&p[0x1650]==5;
    const int common_6b=!substate6c&&p[0x1650]==2&&((dependency==0x200&&stamp==3)||(dependency==0x202&&stamp==4)||(dependency==0x203&&stamp==5));
    /* EM00 underflows the phase 0 dependency table at 23B9C0 and
     * takes the original unarmed default, including 23C340 with t1 zero. */
    const int r101_em00_recovery=!p[0x2f7]&&(r101_em00_stamp12(work->room,p)||
        (!substate6c&&r101_em00_stamp15(work->room,p)));
    const int r101_em02_6b=!substate6c&&r101_em02_stamp5(work->room,p)&&!p[0x2f7];
    const int r101_em03_recovery=!p[0x2f7]&&
        ((!substate6c&&(r101_em03_stamp0(work->room,p)||r101_gate_exchanged_enemy(work,p)))||
         (substate6c&&r101_arena_airborne(enemy)&&dependency==0x203&&stamp==0x20));
    const int r102_em00_6b=!substate6c&&work->room==0x102&&
        dependency==0x200&&stamp==3&&p[0x1650]==1;
    /* Original dependency0203 slot 43AC54 selects the same unarmed table. */
    const int r102_em03_6b=!substate6c&&r102_em03_stamp8(work,p)&&!p[0x2f7];
    const int r104_em03_recovery=!p[0x2f7]&&
        gh_enemy203_is_r104_entry(&runtime->entry->request,work->room);
    /* Original EM27 uses shared 6B/6C clips via 43ACE4 and 43AF74. */
    const int r102_em27_recovery=r102_east_em27(work,p)&&!p[0x2f7];
    /* The AT0F EM0A guard uses the 23BC30 and 23C9F8 motion tables. */
    const int gate_em0a=r102_gate_em0a(work,p)&&!p[0x2f7];
    const int west_em0a=substate6c&&work->room==0x100&&dependency==0x20a&&!stamp&&p[0x1650]==3&&!p[0x2f7];
    /* AT0B stamp 7 also underflows dependency 0200 to 23C6A4. */
    const int r102_area11_6c=substate6c&&
        (r102_area11_em02(work,p)||(r102_area11_obstacle(work,p)&&word(p+0x16c4)==2));
    /* Original 6C dispatch uses default 23C6A4 for the R105 trio:
     * EM00 unsigned underflow, EM03 entry 43AEE4, EM27 entry 43AF74. */
    const int r105=r105_opening_enemy(work,p);
    const int r101_held=work->room==0x101&&dependency==0x203&&stamp==13&&!p[0x1650];
    const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(runtime->timers);
    if(r101_held&&(!held||!held->weapon||held->weapon->kind!=0x361||word(p+0x6ec)))
        return stop(s,0x23c308,GH_ACTOR_MOTION_UNSUPPORTED);
    const gh_om_object *room_weapon=gh_enemy_timers_room_weapon(runtime->timers);
    const int r102_armed_6c=substate6c&&work->room==0x102&&dependency==0x200&&
        stamp==3&&p[0x1650]==1&&room_weapon&&room_weapon->runtime_kind==0x363;
    /* The original EM02 6C with OM363 starts resource row 3E4. */
    const int r100_armed_6c=substate6c&&r100_armed_em02(enemy,p)&&held&&held->weapon&&
        held->weapon->kind==0x363;
    const int arena_em0a=substate6c&&r103_arena_em0a(runtime->entry)&&!p[0x2f7];
    const int boss=substate6c&&r103_arena_boss(runtime->entry)&&!p[0x2f7];
    const int arena=substate6c&&(r103_arena_jumper(runtime->entry)||arena_em0a||boss);
    const int west_patrol_em00=work->room==0x100&&dependency==0x200&&stamp==10&&
        p[0x1650]==5&&word(p+0x16c4)==6&&!p[0x2f7];
    const int west_step=substate6c&&(r100_at2_enemy(work,p)||r100_at3_enemy(work,p)||west_patrol_em00);
    if(!devil&&!r101_held&&!r101_em00_recovery&&!r101_em02_6b&&!r101_em03_recovery&&!r105&&!r102_em00_6b&&!r102_em03_6b&&!r104_em03_recovery&&!r102_em27_recovery&&!r102_em00_6c&&!r102_armed_6c&&!r102_area11_6c&&
       !r100_armed_6c&&!gate_em0a&&!west_em0a&&!arena&&!west_step&&!formation_em00_6c&&
       (work->room!=0x100||((!opening_actor||p[0x1650])&&!common_6b&&!stamp8_6c)))
        return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        if(p[0x2f6]>1)return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!p[0x2f6])p[0x1864]=0;
        s->substate67_stage=p[0x2f6]?TURN_TO_PLAYER:SELECT_MOTION;
    }
    if(s->substate67_stage==SELECT_MOTION){
        const gh_object_view *resource=&runtime->entry->resource;
        const size_t required=devil?0x1d78:boss?0x1aa0:r104_em03_recovery?0x1a48:
            (gate_em0a||arena_em0a||west_em0a)?(substate6c?0xbb4:0xbcc):0x3f4;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<required||
            resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,gate_em0a?(substate6c?0x23c9f8:0x23bc70):substate6c?0x23c6a4:0x23ba2c,
                        GH_ACTOR_MOTION_BOUNDS);
        uint32_t random=0;
        if(!boss&&gh_game_mt_next_0030f348(&runtime->entry->b.init->random,&random))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        static const uint16_t rows_6b[]={0x3c4,0x3cc,0x3d4,0x3dc,0x3ec};
        if(boss){
            /* EM14 and EM15 share the fixed unarmed 23C92C table entry. */
            s->reaction_resource_offset=0x1a98;
        }else if(r104_em03_recovery){
            /* Dependency 0250 uses the same unsigned three-way draw
             * at 23BF50 for 6B and 23CDB0 for 6C. */
            s->reaction_resource_offset=(uint16_t)(0x1a30+8*(random%3u));
        }else if(west_em0a){
            /* 23C9F8 divides the original signed MT word. Negative remainders select B84. */
            int32_t roll;
            memcpy(&roll,&random,sizeof(roll));
            const int choice=roll%6;
            s->reaction_resource_offset=(uint16_t)(0xb84+8*(choice>0?choice:0));
        }else if(gate_em0a||arena_em0a){
            const unsigned count=substate6c?6u:9u;
            s->reaction_resource_offset=(uint16_t)(0xb84+8*(random%count));
        }else if(substate6c){
            s->reaction_resource_offset=(uint16_t)(0x3c4+8*(random%6u));
            /* 23D100 selects the weapon row after the default draw. */
            if(p[0x2f7]||r101_held||r102_armed_6c||r100_armed_6c)s->reaction_resource_offset=0x3e4;
        }else if(devil){
            s->reaction_resource_offset=(uint16_t)((random&1u)?0x1d68:0x1d70);
        }else{
            /* 23C308 overrides the default MT-selected resource when the
             * actor has a weapon. The default dispatch still draws once. */
            s->reaction_resource_offset=r101_held?0x3e4:rows_6b[random%5u];
        }
        s->substate67_stage=START_MOTION;
    }
    if(s->substate67_stage==START_MOTION){
        if((!r101_held&&!r100_armed_6c&&(word(p+0x6ec)||held))||
           (room_weapon&&!r102_armed_6c))
            return stop(s,substate6c?0x23d0f4:0x23c308,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=runtime->entry->resource.bytes+s->reaction_resource_offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(row),word(row+4),0,10,p[0x17c3]?2:0,
                devil||!(common_6b||r101_em00_recovery||r101_em02_6b||r101_em03_recovery||r102_em00_6b||r102_em03_6b||r104_em03_recovery||r102_em27_recovery||gate_em0a||r105||stamp8_6c||r102_em00_6c||
                  r102_armed_6c||r102_area11_6c||arena||west_step));
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5f0,1);
        p[0x2f6]=1;
        s->substate67_stage=TURN_TO_PLAYER;
    }
    if(s->substate67_stage==TURN_TO_PLAYER){
        float from[3],to[3],turn;
        const float yaw=number(word(p+0x104)),actor_rate=number(word(p+0x5a8));
        const float frame_rate=number(word((const uint8_t *)work+0x55c));
        fenv_t env;
        if(!position(actor,from)||!position(s->player,to)||!isfinite(actor_rate)||
            !isfinite(frame_rate)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        const float cap=mul(mul(actor_rate,number(0x3e490fdb)),frame_rate);
        gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
            from,to,yaw,cap,&turn,&pc);
        float next=status?0:add(yaw,turn);
        if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_id_angle_reference_001f7d48(next,&next))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(next));
        s->substate67_stage=STEP_MOTION;
    }
    if(s->substate67_stage==STEP_MOTION){
        if(!runtime->bindings->canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *runtime->bindings->canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?SELECT_AFTER:ROOT_ROTATION;
    }
    if(s->substate67_stage==SELECT_AFTER){
        gh_actor_motion_status status=devil?choose_devil_002705d8(s,p):
            choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROOT_ROTATION;
    }
    if(s->substate67_stage==ROOT_ROTATION){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ROOT_TRANSLATION;
    }
    if(s->substate67_stage==ROOT_TRANSLATION){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=CUE;
    }
    if(s->substate67_stage==CUE){
        if((half(p+0x3ac)&3u)&&word(p+0x5f0)){
            put(p+0x5f0,0);
            s->substate67_stage=CUE_SOUND;
        }else s->substate67_stage=5;
    }
    if(s->substate67_stage==CUE_SOUND){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_motion_sound_request *request=&s->reaction_sound;
        if(!request->model){
            uint32_t random;
            if(gh_game_mt_next_0030f348(&runtime->entry->b.init->random,&random))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            /* 4474B8 -> 274750 draws once from the five original EM50 cues. */
            static const uint8_t em03_cues[]={0x0a,0x0b,0x0e,0x0f,0x10};
            unsigned cue=r104_em03_recovery?em03_cues[random%5u]:(random&1u?10u:11u);
            const uint16_t program=(uint16_t)(word(p+0x424)+cue);
            *request=(gh_motion_sound_request){.model=motion->b.model,.actor=actor,.bank=1};
            memcpy(&request->program,&program,sizeof(program));
        }
        uint32_t handle;
        gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status enemy_attack_record_contact_00262a10(gh_enemy202_move *s,uint8_t *p)
{
    const gh_enemy_hitareas *hitareas=s->runtime.hitareas;
    for(unsigned i=1;i<5;i++){
        const gh_hitarea_object *give=&hitareas->units[i];
        const gh_title_player_view *hit;
        if(!hitareas->created[i]||
           (!give->fallback&&gh_hitarea_object_last_hit_actor(give,&hit)))
            return stop(s,0x262a10,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!give->fallback&&hit&&hit->bytes&&hit->size>=0x300&&half(hit->bytes+0x2fe)==0x100){
            p[0x1864]=1;
            break;
        }
    }
    return GH_ACTOR_MOTION_OK;
}

static void stamp19_attack_parameters(uint8_t *p,unsigned power,unsigned shape,unsigned kind)
{
    for(unsigned offset=0x1870;offset<=0x1884;offset+=0x14){
        put(p+offset,shape);
        put(p+offset+4,power);
        put(p+offset+8,kind);
        put(p+offset+12,UINT32_MAX);
        put_half(p+offset+16,0);
    }
}

static int r102_armed_em00(const gh_enemy_actor *enemy,const uint8_t *p)
{
    return enemy->b.entry.work->room==0x102&&word(p+0x564)==0x200&&
        half(p+0x640)==3&&p[0x1650]==1;
}

static gh_actor_motion_status enemy_attack_area_00260b30(gh_enemy202_move *s,uint8_t *p)
{
    const unsigned hit_flags=half(p+0x3ac)&3u;
    if(!hit_flags)return GH_ACTOR_MOTION_OK;
    const unsigned params=hit_flags&2u?0x1884:0x1870;
    const unsigned profile=word(p+params);
    gh_enemy_actor *enemy=s->enemy;
    gh_enemy_move_actor *runtime=&s->runtime;
    const int r101_weapon_swing=(profile==0x12||profile==0x13)&&
        (s->pending_ai_pc==0x233738||s->pending_ai_pc==0x21fd50)&&
        runtime->entry->b.work->room==0x101&&word(p+0x564)==0x203&&half(p+0x640)==13;
    const int r102_weapon_swing=enemy&&r102_armed_em00(enemy,p)&&
        ((profile==0x12&&s->pending_ai_pc==0x233738)||
         ((profile==0x12||profile==0x13)&&s->pending_ai_pc==0x21fd50));
    const uint16_t cue=half(p+params+16);
    const int r100_weapon_swing=enemy&&r100_armed_em02(enemy,p)&&
        (((profile==0x12||profile==0x13)&&s->pending_ai_pc==0x21fd50&&cue==0x11d)||
         (profile==0x12&&s->pending_ai_pc==0x233738&&cue==0x142));
    const int weapon_swing=r100_weapon_swing||r101_weapon_swing||r102_weapon_swing;
    const int opening_em0a=runtime->entry->b.work->room==0x100&&word(p+0x564)==0x20a&&
        !half(p+0x640)&&p[0x1650]==3;
    const int em0a_long_kick=(opening_em0a||r102_gate_em0a(runtime->entry->b.work,p)||
        r103_arena_em0a(runtime->entry))&&p[0x2f5]==0x15&&word(p+params+4)==15&&word(p+params+8)==0x37;
    const int boss_long_kick=s->pending_ai_pc==0x22fae0&&r103_arena_boss(runtime->entry)&&
        p[0x2f5]==0x17&&p[0x2f7]==1&&word(p+params+4)==20&&word(p+params+8)==0x3e&&!cue;
    const int long_kick=profile==12&&!p[0x2f4]&&p[0x2f6]==1&&
        word(p+params+12)==UINT32_MAX&&(em0a_long_kick||boss_long_kick);
    const int reverse_kick=profile==0x14&&
        (opening_em0a||r102_gate_em0a(runtime->entry->b.work,p)||r103_arena_em0a(runtime->entry))&&
        s->pending_ai_pc==0x231ac0&&!word(p+params+4)&&
        word(p+params+8)==0x3e&&word(p+params+12)==UINT32_MAX;
    const int arena_slide=profile==10&&s->pending_ai_pc==0x2236e8&&
        r103_arena_em0a(runtime->entry)&&word(p+params+4)==40&&word(p+params+8)==0x41&&
        !word(p+params+12)&&half(p+params+16)==0xf5;
    const int r104_charge=profile==10&&s->pending_ai_pc==0x224090&&
        gh_enemy203_is_r104_entry(&runtime->entry->request,runtime->entry->b.work->room)&&
        word(p+params+4)==40&&word(p+params+8)==0x42&&
        !word(p+params+12)&&cue==0xf5;
    const int randomized_charge=arena_slide||r104_charge;
    const int r104_wide=profile==0x1b&&s->pending_ai_pc==0x232500&&
        gh_enemy203_is_r104_entry(&runtime->entry->request,runtime->entry->b.work->room)&&
        word(p+params+4)==50&&word(p+params+8)==0x45&&word(p+params+12)==UINT32_MAX&&!cue;
    const int wide_kick=profile==0x29&&s->pending_ai_pc==0x232500&&
        (r102_gate_em0a(runtime->entry->b.work,p)||r103_arena_em0a(runtime->entry))&&word(p+params+4)==20&&
        word(p+params+8)==0x42&&!word(p+params+12)&&half(p+params+16)==0xf6;
    const int boss_attack=s->pending_ai_pc==0x21a538&&r103_arena_boss(runtime->entry);
    const int em14_special=profile==0x11&&boss_attack&&word(p+0x564)==0x214&&
        word(p+params+4)==20&&word(p+params+8)==0x4e&&word(p+params+12)==UINT32_MAX&&!cue;
    const int formation_capture=profile==0xf&&s->pending_ai_pc==0x223470&&
        ((word(p+0x564)==0x202&&
          (r100_at2_formation(runtime->entry->b.work,p)||r100_at2_enemy(runtime->entry->b.work,p)||
           r100_at3_enemy(runtime->entry->b.work,p)))||
         (runtime->entry->b.work->room==0x100&&word(p+0x564)==0x203&&half(p+0x640)==19&&!p[0x1650]))&&
        !word(p+params+4)&&
        word(p+params+8)==0x4a&&word(p+params+12)==UINT32_MAX&&!cue;
    const int devil_melee=profile==0xd&&s->pending_ai_pc==0x21a538&&
        runtime->entry->b.work->room==0x100&&word(p+0x564)==0x220&&half(p+0x2fe)==0x220&&
        half(p+0x640)==0xff&&word(p+params+4)==40&&word(p+params+8)==0x3e&&
        word(p+params+12)==UINT32_MAX&&!cue;
    const int r104_brace=profile==0x10&&s->pending_ai_pc==0x226450&&
        gh_enemy203_is_r104_entry(&runtime->entry->request,runtime->entry->b.work->room)&&
        !word(p+params+4)&&word(p+params+8)==0x4b&&word(p+params+12)==UINT32_MAX&&!cue;
    if((profile>2&&!weapon_swing&&!long_kick&&!reverse_kick&&!wide_kick&&!randomized_charge&&!em14_special&&!formation_capture&&!devil_melee&&!r104_brace&&!r104_wide)||
       (profile==1&&word(p+0x564)!=0x200&&!r105_exchanged_enemy(runtime->entry->b.work,p)&&
        !r102_east_em27(runtime->entry->b.work,p)&&!boss_attack))
        return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    gh_hitarea_object *give=&runtime->hitareas->units[1];
    const uint8_t *node;
    gh_collision_pair_shape shapes[4];
    for(unsigned i=0;i<4;i++){
        gh_title_player_view *actor;
        if(!runtime->hitareas->created[i+1]||give[i].fallback||
           gh_hitarea_object_actor(&give[i],&actor)||actor!=&runtime->entry->object.view||
           gh_hitarea_node_at(&give[i],0,&node,&shapes[i]))
            return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    if(word(shapes[3].record+0xe0)!=4||word(shapes[3].record+0x1e0)!=0x44bb20)
        return stop(s,0x1fd820,GH_ACTOR_MOTION_UNSUPPORTED);
    float euler[3],rotation[16];
    uint32_t pc=0;
    if(profile!=1&&!long_kick&&!wide_kick){
        memcpy(euler,p+0x100,sizeof(euler));
        if(randomized_charge){
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            const float angle=number(0x3e860a92);
            const float yaw=draw&1u?add(euler[1],angle):sub(euler[1],angle);
            if(gh_id_angle_reference_001f7d48(yaw,euler+1))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(gh_actor_matrix_rotation_00147728(euler,0,rotation,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x1fd338,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    fenv_t env;
    const float scale=number(word(p+0x114)),power_rate=number(word(p+0x76c));
    if((profile!=1&&(!isfinite(scale)||scale==0))||!isfinite(power_rate)||!set_round(&env,FE_TOWARDZERO))
        return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    int32_t power=(int32_t)word(p+params+4);
    if(r104_charge&&(word(p+0x16d4)&0x10u))power+=power/2;
    if(word(p+0x16ec))power+=power/2;
    const float scaled_power=mul((float)power,power_rate);
    volatile float reciprocal=profile==1?1.0f:1.0f/scale;
    const float height_scale=profile==0x12?0.5f:reverse_kick?0.0f:long_kick?0.6f:
        profile==2?0.7f:1.0f;
    const float height=mul(reciprocal,height_scale);
    const uint8_t *work=(const uint8_t *)runtime->entry->b.work;
    if(!isfinite(scaled_power)||scaled_power < -2147483648.0f||scaled_power >= 2147483648.0f){
        (void)fesetenv(&env);
        return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    power=(int32_t)scaled_power;
    if(word(work+0x594)&0x800u)power=(int32_t)mul((float)power,0.8f);
    if(fesetenv(&env)||!isfinite(height))return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    const float front[]={0,1.3f,0.75f},side[]={0.75f,1.3f,0},base[]={0,1.3f,0};
    const float box[]={0,randomized_charge?0.0f:height,
        devil_melee?2.0f:reverse_kick?1.5f:(long_kick||wide_kick)?0.0f:0.75f};
    if(gh_collision_pair_shape_offset_002bb258(&shapes[3],base,&pc)||
       gh_collision_pair_shape_offset_002bb258(&shapes[0],front,&pc)||
       gh_collision_pair_shape_offset_002bb258(&shapes[1],wide_kick?side:front,&pc)||
       (profile!=1&&!formation_capture&&gh_collision_pair_shape_offset_002bb258(&shapes[3],box,&pc)))
        return stop(s,pc?pc:0x1fd888,GH_ACTOR_MOTION_UNSUPPORTED);
    put(p+0x16d0,word(p+0x16d0)|0x100000u);
    for(unsigned i=0;i<4;i++){
        put(give[i].bytes+0x40,word(p+params+12));
        put_half(give[i].bytes+0x44,half(p+params+16));
    }
    if(r104_wide){
        /* Profile 1B activates the 680 sphere using its existing local offset. */
        const gh_game_math_globals *math=runtime->hitareas->b.math;
        if(!math||!math->initialized)return stop(s,0x1fd308,GH_ACTOR_MOTION_UNSUPPORTED);
        put_half(give[2].bytes+0x46,0x45);
        if(gh_hitarea_object_set_node_scalar_001fd7b0(&give[2],(int16_t)half(p+0x684),0x40000000u,&pc))
            return stop(s,pc?pc:0x1fd7b0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(give[2].bytes+0x4c,(uint32_t)power);
        if(gh_hitarea_object_set_active_001fd668(&give[2],1,&pc))
            return stop(s,pc?pc:0x1fd668,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(give[2].bytes+0x10,math->zero_vector,12);
        return GH_ACTOR_MOTION_OK;
    }
    if(formation_capture){
        const float capture[]={0,0.3f,0.75f};
        gh_hitarea_object *cylinder=&give[1];
        if(gh_collision_pair_shape_offset_002bb258(&shapes[1],capture,&pc))
            return stop(s,pc?pc:0x1fd888,GH_ACTOR_MOTION_UNSUPPORTED);
        put_half(cylinder->bytes+0x46,0x4a);
        if(gh_hitarea_object_set_node_scalar_001fd7b0(cylinder,
            (int16_t)half(p+0x67c),0x3f333333u,&pc))
            return stop(s,pc?pc:0x1fd7b0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(cylinder->bytes+0x4c,0);
        if(gh_hitarea_object_set_active_001fd668(cylinder,1,&pc))
            return stop(s,pc?pc:0x1fd668,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(cylinder->bytes+0x10,rotation+8,12);
        return GH_ACTOR_MOTION_OK;
    }
    if(em14_special||r104_brace){
        /* Original profiles 11 and 10 use the 670 and 678 spheres respectively. */
        const unsigned slot=r104_brace?1u:0u;
        const float offset[]={0,0.3f,0.75f};
        if(gh_collision_pair_shape_offset_002bb258(&shapes[slot],offset,&pc))
            return stop(s,pc?pc:0x1fd888,GH_ACTOR_MOTION_UNSUPPORTED);
        put_half(give[slot].bytes+0x46,(uint16_t)word(p+params+8));
        if(gh_hitarea_object_set_node_scalar_001fd7b0(&give[slot],(int16_t)half(p+0x674+8*slot),
            r104_brace?0x3f800000u:0x3f000000u,&pc))
            return stop(s,pc?pc:0x1fd7b0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(give[slot].bytes+0x4c,(uint32_t)power);
        if(gh_hitarea_object_set_active_001fd668(&give[slot],1,&pc))
            return stop(s,pc?pc:0x1fd668,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(give[slot].bytes+0x10,rotation+8,12);
        return GH_ACTOR_MOTION_OK;
    }
    if(gh_hitarea_object_set_active_001fd668(&give[3],1,&pc))
        return stop(s,pc?pc:0x1fd668,GH_ACTOR_MOTION_UNSUPPORTED);
    put_half(give[3].bytes+0x46,(uint16_t)word(p+params+8));
    put(give[3].bytes+0x4c,(uint32_t)power);
    if(profile==1||long_kick||wide_kick){
        const gh_game_math_globals *math=runtime->hitareas->b.math;
        if(!math||!math->initialized)return stop(s,0x1fd308,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(give[3].bytes+0x10,math->zero_vector,12);
    }else memcpy(give[3].bytes+0x10,rotation+8,12);
    const int short_box=profile==1||long_kick;
    const float size[]={
        wide_kick?2.2f:profile==0x13?2.0f:weapon_swing?0.3f:(reverse_kick||devil_melee)?0.5f:short_box?1.0f:0.7f,
        profile==0x13?0.3f:(reverse_kick||randomized_charge)?2.0f:short_box?0.5f:1.0f,
        wide_kick?2.2f:(reverse_kick||weapon_swing||devil_melee)?2.0f:1.0f
    };
    if(gh_collision_pair_box_size_002baaa8(&shapes[3],size,&pc))
        return stop(s,pc?pc:0x1fd820,GH_ACTOR_MOTION_UNSUPPORTED);
    return GH_ACTOR_MOTION_OK;
}

/* Original EM00 selector 16 with an attached OM363, phases 2 through 7. */
static gh_actor_motion_status advance_r102_armed16_0021fd50(gh_enemy202_move *s,uint8_t *p)
{
    enum { MOTION=206, EFFECT, TRACK, STEP, ROTATE, TRANSLATE, FINISH };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->entry.b.work;
    const gh_om_object *weapon=gh_enemy_timers_room_weapon(&enemy->timers);
    if(!r102_armed_em00(enemy,p)||!weapon||weapon->runtime_kind!=0x363||word(p+0x6ec))
        return stop(s,0x21fd50,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        if(!p[0x2f6])p[0x2f6]=2;
        for(unsigned i=1;i<5;++i){
            const gh_hitarea_object *give=&enemy->hitareas.units[i];
            const gh_title_player_view *hit=NULL;
            if(!enemy->hitareas.created[i]||
               (!give->fallback&&gh_hitarea_object_last_hit_actor(give,&hit)))
                return stop(s,0x262a10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit&&hit->bytes&&hit->size>=0x300&&half(hit->bytes+0x2fe)==0x100){
                p[0x1864]=1;
                break;
            }
        }
        s->substate67_phase=p[0x2f6];
        if(s->substate67_phase==2){
            p[0x1864]=0;
            put(p+0x604,0);
            put(p+0x5f4,1);
            /* 221E10 replaces both Give rows after the generic setup. */
            put(p+0x1870,0x13);
            put(p+0x1874,20);
            put(p+0x1878,0x3a);
            put(p+0x187c,0);
            put_half(p+0x1880,0x11d);
            put(p+0x1884,0x12);
            put(p+0x1888,40);
            put(p+0x188c,0x3b);
            put(p+0x1890,0);
            put_half(p+0x1894,0x11d);
            put(p+0x5fc,0);
            s->substate67_stage=MOTION;
        }else if(s->substate67_phase==4||s->substate67_phase==6)s->substate67_stage=MOTION;
        else if(s->substate67_phase>=3&&s->substate67_phase<=7)s->substate67_stage=TRACK;
        else return stop(s,0x21fed0,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    uint32_t pc=0;
    const unsigned phase=s->substate67_phase;
    if(s->substate67_stage==MOTION){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const unsigned heat=heat_category(s);
            const gh_object_view *resource=&enemy->entry.resource;
            if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            const size_t row=phase==2?0x4c0:phase==4?0x4cc:0x4e0;
            const size_t sequence=row+(phase==2&&heat==5?8:4);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<sequence+4||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2a8578,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+sequence),
                0,phase==2?5:2,p[0x17c3]?2:0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(phase==2){
            put(p+0x600,word(p+0x104));
            p[0x2f6]=3;
            s->substate67_stage=TRACK;
        }else s->substate67_stage=EFFECT;
    }
    if(s->substate67_stage==EFFECT){
        gh_effect_create_bindings bindings=s->impact_bindings;
        if(!bindings.storage.effects||bindings.work!=work||
           bindings.random!=&enemy->entry.b.init->random)
            return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
        bindings.model=&weapon->initialization->model;
        gh_effect_create_call *effect=&s->death_effect_create;
        const unsigned heat=heat_category(s);
        if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!effect->active){
            const unsigned index=heat==5?0x81+(phase==6):0x7c+(phase==6);
            put(p+0x574,2);
            put_half(p+0x570,0);
            if(gh_effect_create_r101_weapon_002a8608(effect,&weapon->view,actor,
                index,word(p+0x114)))return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(gh_game_effect_create_00306528(effect,&bindings)||!effect->complete)
            return stop(s,effect->frontier?effect->frontier:0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        if(phase==4){
            unsigned swings=1;
            if(heat>=3){
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                swings+=draw&1u;
            }
            put_half(p+0x568,(uint16_t)swings);
        }else{
            put(p+0x5f0,10);
            p[0x1864]=0;
        }
        ++p[0x2f6];
        s->substate67_stage=TRACK;
    }
    if(s->substate67_stage==TRACK&&p[0x2f6]==7&&!word(p+0x5f0))
        s->substate67_stage=STEP;
    if(s->substate67_stage==TRACK){
        float from[3],to[3],turn;
        const float tracking_yaw=number(word(p+0x600)),actor_yaw=number(word(p+0x104));
        const float delay=number(word(p+0x604)),rate=number(word(p+0x5a8));
        fenv_t env;
        if(!position(actor,from)||!position(s->player,to)||!isfinite(delay)||!isfinite(rate)||
           !set_round(&env,FE_TOWARDZERO))
            return stop(s,0x221f38,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
            from,to,tracking_yaw,mul(rate,number(0x3dc90fdb)),&turn,&pc);
        float next_tracking=status?0:add(tracking_yaw,turn);
        float next_actor=status?0:add(actor_yaw,turn);
        const float remaining=delay>0?sub(delay,rate):delay;
        if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_id_angle_reference_001f7d48(next_tracking,&next_tracking)||
           gh_id_angle_reference_001f7d48(next_actor,&next_actor))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x600,bits(next_tracking));
        put(p+0x104,bits(next_actor));
        if(p[0x2f6]==3&&delay>0){
            put(p+0x16d0,word(p+0x16d0)|0x800000u);
            put(p+0x604,bits(remaining));
        }
        if(p[0x2f6]==7)put(p+0x5f0,word(p+0x5f0)-1);
        s->substate67_stage=STEP;
    }
    if(s->substate67_stage==STEP){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        motion->sequence_held_weapon=&weapon->view;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        motion->sequence_held_weapon=NULL;
        if(status)return stop(s,pc?pc:0x157c18,status);
        const uint8_t *manager=enemy->entry.b.init->enemies;
        const int interrupted=manager[0x53d]||manager[0x53c];
        s->state71_exit=(uint8_t)(ended&&p[0x2f6]==7);
        s->substate67_stage=ROTATE;
        if(ended&&p[0x2f6]==3){
            p[0x2f6]=interrupted?6:4;
            if(interrupted)s->substate67_stage=FINISH;
        }else if(ended&&p[0x2f6]==5){
            const uint16_t swings=(uint16_t)(half(p+0x568)-1u);
            put_half(p+0x568,swings);
            const float distance=number(word(p+0x618));
            if((int16_t)swings<=0||distance>16.0f||p[0x1864]||interrupted)
                p[0x2f6]=6;
        }
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=FINISH;
    }
    if(s->substate67_stage==FINISH&&s->state71_exit){
        const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    if(s->substate67_stage==FINISH){
        if(p[0x2f6]==7&&p[0x1864])put(p+0x5f4,0);
        if(word(p+0x5f4)){
            const gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
            if(status)return status;
        }
        if(heat_category(s)>=3)return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(half(p+0x3ac)&3u)put(p+0x5fc,1);
        if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* The unarmed EM20 selector 16 runs the authored opening, follow-up,
 * and finishing kick rows, then returns directly to selector 12. */
static gh_actor_motion_status advance_devil_combo_0021fd50(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=235, TRACK, SAMPLE, ROTATE, TRANSLATE, GIVE };
    const gh_enemy_move_actor *runtime=&s->runtime;
    const gh_enemy_entry *entry=runtime->entry;
    const gh_title_player_view *actor=&entry->object.view;
    gh_actor_motion *motion=runtime->motion;
    const uint8_t *manager=entry->b.init->enemies;
    if(entry->b.work->room!=0x100||word(p+0x564)!=0x220||half(p+0x2fe)!=0x220||
       half(p+0x640)!=0xff||word(p+0x6ec)||gh_enemy_timers_weapon_kind(runtime->timers)||
       !runtime->bindings->canonical_rate)
        return stop(s,0x21fd50,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t pc=0;
    if(!s->substate67_stage){
        if(p[0x2f6]==1||p[0x2f6]>7)return stop(s,0x21fd50,GH_ACTOR_MOTION_UNSUPPORTED);
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        if(!p[0x2f6])p[0x2f6]=2;
        put(p+0x250,word(p+0x250)&~0x40000u);
        gh_actor_motion_status status=enemy_attack_record_contact_00262a10(s,p);
        if(status)return status;
        if(p[0x2f6]==2){
            p[0x1864]=0;
            put(p+0x604,0);
            put(p+0x5f4,1);
            put(p+0x5fc,0);
            stamp19_attack_parameters(p,5,0,0x3d);
            put(p+0x1888,10);
            put(p+0x188c,0x3e);
        }
        s->substate67_stage=(p[0x2f6]&1u)?TRACK:BIND;
    }
    if(s->substate67_stage==BIND){
        const unsigned phase=p[0x2f6];
        const unsigned heat=heat_category(s);
        if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&entry->resource;
            const size_t row=phase==2?0x1c7c:phase==4?0x1c90:0x1ca4;
            const size_t sequence=row+(heat==5?8:4);
            if(!resource->bytes||!resource->file_extent_known||
               resource->file_extent>resource->size||resource->file_extent<sequence+4)
                return stop(s,0x21fd50,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+row),word(resource->bytes+sequence),
                0,phase==2?5:2,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(phase==2)put(p+0x600,word(p+0x104));
        if(phase==4){
            uint32_t random=0;
            if(heat>=3&&gh_game_mt_next_0030f348(&entry->b.init->random,&random))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            put_half(p+0x568,(uint16_t)(1u+(random&1u)));
        }
        if(phase==6){
            put(p+0x5f0,10);
            p[0x1864]=0;
        }
        ++p[0x2f6];
        s->substate67_stage=TRACK;
    }
    if(s->substate67_stage==TRACK){
        const unsigned phase=p[0x2f6];
        const uint32_t remaining=word(p+0x5f0);
        if(phase!=7||remaining){
            const float rate=number(word(p+0x5a8)),delay=number(word(p+0x604));
            const float shadow=number(word(p+0x600)),yaw=number(word(p+0x104));
            float from[3],to[3],turn;
            fenv_t env;
            if(!position(actor,from)||!position(s->player,to)||!isfinite(rate)||!isfinite(delay)||
               !set_round(&env,FE_TOWARDZERO))
                return stop(s,0x221f38,GH_ACTOR_MOTION_UNSUPPORTED);
            const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
                from,to,shadow,mul(rate,number(0x3dc90fdb)),&turn,&pc);
            float next_shadow=status?0:add(shadow,turn),next_yaw=status?0:add(yaw,turn);
            const float next_delay=delay>0?sub(delay,rate):delay;
            if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(next_shadow,&next_shadow)||
               gh_id_angle_reference_001f7d48(next_yaw,&next_yaw))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x600,bits(next_shadow));
            put(p+0x104,bits(next_yaw));
            if(phase==3&&delay>0){
                put(p+0x16d0,word(p+0x16d0)|0x800000u);
                put(p+0x604,bits(next_delay));
            }
            if(phase==7)put(p+0x5f0,remaining-1);
        }
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        const unsigned phase=p[0x2f6];
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *runtime->bindings->canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        const int interrupted=manager[0x53c]||manager[0x53d];
        s->substate67_stage=ROTATE;
        if(ended&&phase==3){
            p[0x2f6]=(uint8_t)(interrupted?6:4);
            if(interrupted)s->substate67_stage=GIVE;
        }else if(ended&&phase==5){
            const uint16_t swings=(uint16_t)(half(p+0x568)-1u);
            put_half(p+0x568,swings);
            if((int16_t)swings<=0||number(word(p+0x618))>16.0f||p[0x1864]||interrupted)
                p[0x2f6]=6;
        }else if(ended&&phase==7){
            enemy_decision(p,0,0x12,1);
            s->substate67_stage=5;
        }
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,entry->b.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,entry->b.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=GIVE;
    }
    if(s->substate67_stage==GIVE){
        if(p[0x2f6]==7&&p[0x1864])put(p+0x5f4,0);
        if(word(p+0x5f4)){
            gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
            if(status)return status;
        }
        const unsigned flags=half(p+0x3ac);
        if(heat_category(s)>=3&&number(word(p+0x17cc))<=0&&!p[0x1864]&&(flags&0x10u))
            return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(flags&3u)put(p+0x5fc,1);
        if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* 21FD50's original unarmed phase0 and phase1 use the bound dependency
 * resource, two Give rows, and the yaw shadow at actor+600. */
static gh_actor_motion_status advance_unarmed16_0021fd50(gh_enemy202_move *s,uint8_t *p)
{
    enum { MOTION=200, TRACK, STEP, ROTATE, TRANSLATE, FINISH, FOLLOWUP };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->entry.b.work;
    const uint32_t dependency=word(p+0x564);
    const int west_em03=dependency==0x203&&work->room==0x100&&
        half(p+0x640)==19&&!p[0x1650];
    const int west_6c4=(dependency==0x202&&r100_at3_enemy(work,p))||west_em03;
    const int boss=r103_arena_boss(&enemy->entry)&&word(p+0x564)==0x214;
    const int r104=gh_enemy203_is_r104_entry(&enemy->entry.request,work->room)&&!p[0x2f7];
    if((!r104&&!boss&&!r101_gate_exchanged_enemy(work,p)&&!r105_exchanged_enemy(work,p)&&
        !r102_east_em27(work,p)&&!west_6c4)||word(p+0x6ec)||
       gh_enemy_timers_weapon_kind(&enemy->timers)||p[0x2f6]>1)
        return stop(s,0x21fd50,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t pc=0;
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        for(unsigned i=1;i<5;++i){
            const gh_hitarea_object *give=&enemy->hitareas.units[i];
            const gh_title_player_view *hit=NULL;
            if(!enemy->hitareas.created[i]||
               (!give->fallback&&gh_hitarea_object_last_hit_actor(give,&hit)))
                return stop(s,0x262a10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit&&hit->bytes&&hit->size>=0x300&&half(hit->bytes+0x2fe)==0x100){
                p[0x1864]=1;
                break;
            }
        }
        if(p[0x2f6])s->substate67_stage=TRACK;
        else{
            p[0x1864]=0;
            put(p+0x604,0);
            put(p+0x5f4,1);
            put(p+0x5f0,r104?70:50);
            stamp19_attack_parameters(p,r104?30:5,0,0x3d);
            put(p+0x1888,r104?40:10);
            put(p+0x188c,(r104||west_6c4)?0x3e:0x37);
            if(r104){
                const unsigned heat=heat_category(s);
                uint32_t draw=0;
                if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
                if(heat>1&&gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                /* Original dep0250 selects 19C0 on bit one, otherwise 19C4. */
                s->substate67_phase=(draw&1u)?4:8;
                put(p+0x187c,1);
                put_half(p+0x1880,0x28);
                put(p+0x1890,1);
                put_half(p+0x1894,0x28);
            }
            if(boss){
                put(p+0x1890,0);
                put_half(p+0x1894,0xf6);
            }
            put(p+0x5fc,0);
            s->substate67_stage=MOTION;
        }
    }
    if(s->substate67_stage==MOTION){
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const unsigned heat=heat_category(s);
            if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            const size_t row=r104?0x19bc:boss?0x1b24:west_6c4?0x6c4:0x138;
            const size_t sequence=row+(r104?s->substate67_phase:heat==5?8:4);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+12||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x21ff40,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+sequence),
                0,10,p[0x17c3]?2:0,west_6c4);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x600,word(p+0x104));
        p[0x2f6]=1;
        s->substate67_stage=TRACK;
    }
    if(s->substate67_stage==TRACK){
        const uint32_t remaining=word(p+0x5f0);
        const float turn_delay=number(word(p+0x604));
        const float rate=number(word(p+0x5a8));
        if(!isfinite(turn_delay)||!isfinite(rate))
            return stop(s,0x221758,GH_ACTOR_MOTION_UNSUPPORTED);
        if(remaining||turn_delay>0){
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x221758,GH_ACTOR_MOTION_CALLEE);
            if(turn_delay>0){
                put(p+0x16d0,word(p+0x16d0)|0x800000u);
                put(p+0x604,bits(sub(turn_delay,rate)));
            }
            if(remaining){
                float from[3],to[3],turn;
                const float tracking_yaw=number(word(p+0x600)),actor_yaw=number(word(p+0x104));
                put(p+0x5f0,remaining-1);
                if(!position(actor,from)||!position(s->player,to)||
                   gh_player_target_angle_002da038(s->elf,s->elf_bytes,from,to,
                    tracking_yaw,mul(rate,number(0x3dc90fdb)),&turn,&pc)){
                    (void)fesetenv(&env);
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
                }
                float next_tracking=add(tracking_yaw,turn),next_actor=add(actor_yaw,turn);
                if(gh_id_angle_reference_001f7d48(next_tracking,&next_tracking)||
                   gh_id_angle_reference_001f7d48(next_actor,&next_actor)){
                    (void)fesetenv(&env);
                    return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
                }
                put(p+0x600,bits(next_tracking));
                put(p+0x104,bits(next_actor));
            }
            if(fesetenv(&env))return stop(s,0x221758,GH_ACTOR_MOTION_CALLEE);
        }
        s->substate67_stage=STEP;
    }
    if(s->substate67_stage==STEP){
        int ended=0;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state71_exit=(uint8_t)ended;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=FINISH;
    }
    if(s->substate67_stage==FINISH){
        if(s->state71_exit){
            gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
            if(status)return status;
        }else{
            if(word(p+0x5f4)){
                gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
                if(status)return status;
            }
            s->substate67_stage=FOLLOWUP;
        }
        if(s->state71_exit)s->substate67_stage=5;
    }
    if(s->substate67_stage==FOLLOWUP){
        const unsigned flags=half(p+0x3ac);
        int chosen=0;
        if(heat_category(s)>=3){
            if(!west_em03&&!r104)return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
            /* 2233D0 calls the guard-timer predicate, then the original
             * contact and SEQ conditions gate the next attack. */
            if(!(number(word(p+0x17cc))>0)&&!p[0x1864]&&(flags&0x10u)){
                const gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
                if(status)return status;
            }
        }
        if(!chosen){
            if(flags&3u)put(p+0x5fc,1);
            if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        }
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original EM14/EM15 paired lunge, selected by rear evade at range. */
static gh_actor_motion_status advance_boss_paired_attack_0022fae0(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=201, TURN, SAMPLE, CHOOSE, ROTATE, TRANSLATE, GIVE };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->entry.b.work;
    gh_game_init_state *init=enemy->entry.b.init;
    uint32_t pc=0;
    if(!r103_arena_boss(&enemy->entry)||p[0x2f7]!=1||p[0x2f6]>1||word(p+0x6ec))
        return stop(s,0x22fae0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        for(unsigned i=1;i<5;++i){
            const gh_hitarea_object *give=&enemy->hitareas.units[i];
            const gh_title_player_view *hit=NULL;
            if(!enemy->hitareas.created[i]||(!give->fallback&&gh_hitarea_object_last_hit_actor(give,&hit)))
                return stop(s,0x262a10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit&&hit->bytes&&hit->size>=0x300&&half(hit->bytes+0x2fe)==0x100){
                p[0x1864]=1;
                break;
            }
        }
        if(p[0x2f6])s->substate67_stage=TURN;
        else{
            const unsigned rank=heat_category(s);
            const gh_object_view *resource=&enemy->entry.resource;
            if(!rank||rank>5)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x1b4c||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x230ce0,GH_ACTOR_MOTION_BOUNDS);
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&init->random,&draw))
                return stop(s,0x31d000,GH_ACTOR_MOTION_UNSUPPORTED);
            const int thrust=number(gh_frame_unit_bits_0030f518_word(draw))>=0.75f;
            const unsigned row=thrust?0x1b40:rank==5?0x1b30:0x1b38;
            s->reaction_resource_offset=(uint16_t)row;
            s->substate67_phase=(uint8_t)(thrust&&rank==5?8:4);
            const unsigned wait=thrust?20:25;
            static const unsigned percent[]={0,75,80,90,90,100};
            put(p+0x5f0,wait*percent[rank]/100);
            stamp19_attack_parameters(p,20,thrust?12:2,0x3e);
            put(p+0x604,0);
            p[0x1864]=0;
            s->substate67_stage=BIND;
        }
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=enemy->entry.resource.bytes+s->reaction_resource_offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(row),word(row+s->substate67_phase),0,3,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5fc,0);
        put(p+0x600,word(p+0x104));
        p[0x2f6]=1;
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        const uint32_t remaining=word(p+0x5f0);
        const float delay=number(word(p+0x604)),rate=number(word(p+0x5a8));
        fenv_t env;
        if(!isfinite(delay)||!isfinite(rate)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x230dcc,GH_ACTOR_MOTION_UNSUPPORTED);
        if(delay>0){
            put(p+0x16d0,word(p+0x16d0)|0x800000u);
            put(p+0x604,bits(sub(delay,rate)));
        }
        if(remaining){
            float a[3],b[3],turn;
            const float tracking=number(word(p+0x600)),yaw=number(word(p+0x104));
            if(!position(actor,a)||!position(s->player,b)||gh_player_target_angle_002da038(
               s->elf,s->elf_bytes,a,b,tracking,mul(rate,0.19634955f),&turn,&pc)){
                (void)fesetenv(&env);
                return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            float next_tracking,next_yaw;
            if(gh_id_angle_reference_001f7d48(add(tracking,turn),&next_tracking)||
               gh_id_angle_reference_001f7d48(add(yaw,turn),&next_yaw)){
                (void)fesetenv(&env);
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            put(p+0x600,bits(next_tracking));put(p+0x104,bits(next_yaw));
            put(p+0x5f0,remaining-1);
        }
        if(fesetenv(&env))return stop(s,0x230dcc,GH_ACTOR_MOTION_CALLEE);
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?CHOOSE:ROTATE;
    }
    if(s->substate67_stage==CHOOSE){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=GIVE;
    }
    if(s->substate67_stage==GIVE){
        if(!p[0x1864]){
            gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
            if(status)return status;
        }
        if(half(p+0x3ac)&3u)put(p+0x5fc,1);
        if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_armed_attack(gh_enemy202_move *s,uint8_t *p)
{
    enum { ARMED_MOTION=180, ARMED_EFFECT, ARMED_TRACK, ARMED_STEP,
        ARMED_ROTATION, ARMED_TRANSLATION, ARMED_FINISH };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    const int combination=s->pending_ai_pc==0x21fd50;
    const unsigned phase=p[0x2f6];
    const int r102=!combination&&r102_armed_em00(enemy,p);
    const int r101=enemy->b.entry.work->room==0x101&&word(p+0x564)==0x203&&
        half(p+0x640)==13&&!p[0x1650];
    const int r100=r100_armed_em02(enemy,p);
    const gh_title_player_view *weapon=NULL;
    const gh_actor_model *weapon_model=NULL;
    if(r102){
        const gh_om_object *held=gh_enemy_timers_room_weapon(&enemy->timers);
        if(held&&held->runtime_kind==0x363){
            weapon=&held->view;
            weapon_model=&held->initialization->model;
        }
    }else if(r101||r100){
        const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&enemy->timers);
        if(held&&held->weapon&&held->weapon->kind==(r100?0x363:0x361)){
            weapon=&held->weapon->object.view;
            weapon_model=&held->model;
        }
    }
    if(!weapon||word(p+0x6ec)||p[0x2f7]||(combination?(phase==1||phase>7):phase>1))
        return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t pc=0;
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        if(combination&&!phase)p[0x2f6]=2;
        for(unsigned i=1;i<5;i++){
            const gh_hitarea_object *give=&enemy->hitareas.units[i];
            const gh_title_player_view *hit=NULL;
            if(!enemy->hitareas.created[i]||
               (!give->fallback&&gh_hitarea_object_last_hit_actor(give,&hit)))
                return stop(s,0x262a10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit&&hit->bytes&&hit->size>=0x300&&half(hit->bytes+0x2fe)==0x100){
                p[0x1864]=1;
                break;
            }
        }
        if(combination&&p[0x2f6]==2){
            p[0x1864]=0;
            put(p+0x604,0);
            put(p+0x5f4,1);
            /* 221E10 and 221E94 give the two weapons different impact cues. */
            const uint16_t cue=r100?0x11d:0x111;
            put(p+0x1870,0x13);put(p+0x1874,20);put(p+0x1878,0x3a);
            put(p+0x187c,0);put_half(p+0x1880,cue);
            put(p+0x1884,0x12);put(p+0x1888,40);put(p+0x188c,0x3b);
            put(p+0x1890,0);put_half(p+0x1894,cue);
            put(p+0x5fc,0);
            s->reaction_resource_offset=0x4c0;
            s->substate67_stage=ARMED_MOTION;
        }else if(combination&&p[0x2f6]==4){
            s->reaction_resource_offset=0x4cc;
            s->substate67_stage=ARMED_MOTION;
        }else if(combination&&p[0x2f6]==6){
            s->reaction_resource_offset=0x4e0;
            s->substate67_stage=ARMED_MOTION;
        }else if(p[0x2f6])s->substate67_stage=ARMED_TRACK;
        else{
            p[0x1864]=0;
            put(p+0x5f0,20);
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            s->reaction_resource_offset=(draw&1u)?0x4a4:0x474;
            s->substate67_stage=ARMED_MOTION;
        }
    }
    if(s->substate67_stage==ARMED_MOTION){
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        const size_t row=s->reaction_resource_offset;
        if(!call->active){
            const unsigned heat=heat_category(s);
            if(combination&&!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            const size_t seq=row+(row==0x4c0&&heat==5?8:4);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<seq+4||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,combination?0x221f04:0x23380c,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+seq),
                0,(row==0x4cc||row==0x4e0)?2:combination?5:3,p[0x17c3]?2:0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(row==0x4c0){
            put(p+0x600,word(p+0x104));
            p[0x2f6]=3;
            s->substate67_stage=ARMED_TRACK;
        }else if(row==0x4cc||row==0x4e0)s->substate67_stage=ARMED_EFFECT;
        else{
            put(p+0x5f0,20);
            for(unsigned offset=0x1870;offset<=0x1884;offset+=0x14){
                put(p+offset,0x12);
                put(p+offset+4,row==0x474?60:40);
                put(p+offset+8,row==0x474?0x3b:0x3a);
                put(p+offset+12,0);
                put_half(p+offset+16,(r100||r102)?0x142:0x140);
            }
            s->substate67_stage=ARMED_EFFECT;
        }
    }
    if(s->substate67_stage==ARMED_EFFECT){
        gh_effect_create_bindings bindings=s->impact_bindings;
        if(!bindings.storage.effects||bindings.work!=enemy->entry.b.work||
           bindings.random!=&enemy->entry.b.init->random)
            return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
        bindings.model=weapon_model;
        gh_effect_create_call *effect=&s->death_effect_create;
        const unsigned heat=heat_category(s);
        if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!effect->active){
            const int sweep=s->reaction_resource_offset==0x474;
            const uint32_t index=combination?
                (s->reaction_resource_offset==0x4e0?(heat==5?0x82:0x7d):(heat==5?0x81:0x7c)):
                heat==5?(sweep?0x80:0x7f):(sweep?0x7b:0x7a);
            put(p+0x574,2);
            put_half(p+0x570,0);
            if(gh_effect_create_r101_weapon_002a8608(effect,weapon,
                &enemy->entry.object.view,index,word(p+0x114)))
                return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(gh_game_effect_create_00306528(effect,&bindings)||!effect->complete)
            return stop(s,effect->frontier?effect->frontier:0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        if(combination&&s->reaction_resource_offset==0x4e0){
            put(p+0x5f0,10);
            p[0x1864]=0;
        }else if(combination){
            unsigned repetitions=1;
            if(heat>=3){
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                repetitions+=draw&1u;
            }
            put_half(p+0x568,(uint16_t)repetitions);
        }else{
            static const uint8_t tracking_ticks[]={0,15,16,18,18,20};
            put(p+0x5f0,tracking_ticks[heat]);
            put(p+0x5fc,0);
        }
        ++p[0x2f6];
        s->substate67_stage=ARMED_TRACK;
    }
    if(s->substate67_stage==ARMED_TRACK){
        const uint32_t tracking=word(p+0x5f0);
        const int final_combination=combination&&p[0x2f6]==7;
        if(combination&&!final_combination&&word(p+0x604))
            return stop(s,0x221f9c,GH_ACTOR_MOTION_UNSUPPORTED);
        if((combination&&!final_combination)||tracking){
            float from[3],to[3],turn;
            const float yaw=number(word(p+0x104)),rate=number(word(p+0x5a8));
            const float frame_rate=number(word((const uint8_t *)enemy->entry.b.work+0x55c));
            fenv_t env;
            if(!position(&enemy->entry.object.view,from)||!position(s->player,to)||
               !isfinite(rate)||!isfinite(frame_rate)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float previous=combination?number(word(p+0x600)):yaw;
            const float cap=combination?mul(rate,number(0x3dc90fdb)):
                mul(mul(rate,number(0x3e490fdb)),frame_rate);
            const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
                from,to,previous,cap,&turn,&pc);
            float next=status?0:add(yaw,turn),track=status?0:add(previous,turn);
            if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(next,&next)||
               (combination&&gh_id_angle_reference_001f7d48(track,&track)))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            if(combination)put(p+0x600,bits(track));
            if(!combination||final_combination)put(p+0x5f0,tracking-1);
            put(p+0x104,bits(next));
        }
        s->substate67_stage=ARMED_STEP;
    }
    if(s->substate67_stage==ARMED_STEP){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        motion->sequence_held_weapon=weapon;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        motion->sequence_held_weapon=NULL;
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state71_exit=(uint8_t)ended;
        s->substate67_stage=ARMED_ROTATION;
    }
    if(s->substate67_stage==ARMED_ROTATION){
        if(combination&&p[0x2f6]!=7&&s->state71_exit){
            const uint8_t *manager=enemy->entry.b.init->enemies;
            const int no_followup=manager[0x53d]||manager[0x53c];
            if(p[0x2f6]==3)p[0x2f6]=(uint8_t)(no_followup?6:4);
            else{
                const uint16_t repetitions=(uint16_t)(half(p+0x568)-1u);
                put_half(p+0x568,repetitions);
                if((int16_t)repetitions<1||number(word(p+0x618))>16.0f||p[0x1864]||no_followup)
                    p[0x2f6]=6;
            }
            if(p[0x2f6]==6&&phase==3&&no_followup)s->substate67_stage=ARMED_FINISH;
        }
        if(s->substate67_stage==ARMED_ROTATION&&gh_player_root_rotation_0014f9f0(&enemy->entry.object.view,enemy->entry.b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(s->substate67_stage==ARMED_ROTATION)s->substate67_stage=ARMED_TRANSLATION;
    }
    if(s->substate67_stage==ARMED_TRANSLATION){
        if(gh_player_root_translation_0014f928(&enemy->entry.object.view,enemy->entry.b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ARMED_FINISH;
    }
    if(s->substate67_stage==ARMED_FINISH){
        if(s->state71_exit&&(!combination||p[0x2f6]==7)){
            gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
            if(status)return status;
        }else{
            const unsigned hit_flags=half(p+0x3ac);
            if(combination&&p[0x2f6]==7&&p[0x1864])put(p+0x5f4,0);
            if(!combination||word(p+0x5f4)){
                gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
                if(status)return status;
            }
            if(heat_category(s)>=3)return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit_flags&3u)put(p+0x5fc,1);
            if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        }
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status choose_after_knockdown_00274fe8(gh_enemy202_move *,uint8_t *);

static gh_actor_motion_status advance_em0a_slide_002236e8(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=201, TURN, SAMPLE, RECOVER, ROTATE, TRANSLATE, CHECK_WALL, BODY, GIVE };
    gh_enemy_actor *enemy=s->enemy;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_boot_work *work=enemy->entry.b.work;
    gh_game_init_state *init=enemy->b.entry.init;
    uint32_t pc=0;
    if(!r103_arena_em0a(&enemy->entry)||p[0x2f6]>7||p[0x2f7]||p[0x17c3])
        return stop(s,0x2236e8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        for(unsigned i=1;i<5;++i){
            const gh_hitarea_object *give=&enemy->hitareas.units[i];
            const gh_title_player_view *hit=NULL;
            if(!enemy->hitareas.created[i]||
               (!give->fallback&&gh_hitarea_object_last_hit_actor(give,&hit)))
                return stop(s,0x262a10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit&&hit->bytes&&hit->size>=0x300&&half(hit->bytes+0x2fe)==0x100){
                p[0x1864]=1;
                break;
            }
        }
        s->substate67_phase=p[0x2f6];
        s->substate67_stage=(p[0x2f6]&1u)?(p[0x2f6]==1?TURN:SAMPLE):BIND;
    }
    if(s->substate67_stage==BIND){
        const unsigned phase=p[0x2f6];
        const unsigned rank=heat_category(s);
        if(!rank||rank>5)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_object_view *res=&enemy->entry.resource;
        static const uint16_t rows[]={0x98c,0x998,0x9a0,0x9a8};
        const unsigned row=rows[phase/2],sequence=row+(!phase&&rank>1?8:4);
        if(!res->bytes||!res->file_extent_known||res->file_extent>res->size||
           res->file_extent<sequence+4)
            return stop(s,0x2236e8,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            if(!phase){
                p[0x1864]=0;
                stamp19_attack_parameters(p,40,10,0x41);
                for(unsigned offset=0x1870;offset<=0x1884;offset+=0x14){
                    put(p+offset+12,0);
                    put_half(p+offset+16,0xf5);
                }
                put(p+0x5c0,0);put(p+0x5c4,0);
                put(p+0x5c8,rank==5?0x3f800000u:rank==1?0x3f000000u:0x3f400000u);
                put(p+0x5cc,0x3f800000u);
                static const uint16_t turns[]={30,35,40,40,999};
                put(p+0x5f0,turns[rank-1]);
            }
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(res->bytes+row),word(res->bytes+sequence),
                0,phase?2:10,0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(!phase)put(p+0x5fc,0);
        else if(phase==2){
            put(p+0x5f0,4);put(p+0x5f4,0);
            for(unsigned offset=0x5d0;offset<=0x5d8;offset+=4)put(p+offset,0x47c34ff3u);
        }else put(p+0x5f0,5);
        s->substate67_phase=++p[0x2f6];
        s->substate67_stage=phase?SAMPLE:TURN;
    }
    if(s->substate67_stage==TURN){
        const uint32_t turns=word(p+0x5f0);
        if(turns){
            float from[3],to[3],delta;
            const float rate=number(word(p+0x5a8)),yaw=number(word(p+0x104));
            if(!position(actor,from)||!position(s->player,to)||!isfinite(rate)||
               gh_player_target_angle_002da038(s->elf,s->elf_bytes,from,to,yaw,
                  mul(rate,number(0x3e490fdb)),&delta,&pc))
                return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            float next=add(yaw,delta);
            if(gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x104,bits(next));put(p+0x5f0,turns-1);
        }
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        const unsigned phase=s->substate67_phase;
        s->substate67_stage=ROTATE;
        if(phase==1&&ended)p[0x2f6]=2;
        if(phase==3){
            if(ended){
                const uint32_t remaining=word(p+0x5f0)-1u;
                put(p+0x5f0,remaining);
                if((int32_t)remaining<1)p[0x2f6]=4;
            }
            const float rate=number(word(p+0x5a8));
            if(!isfinite(rate))return stop(s,0x223be8,GH_ACTOR_MOTION_UNSUPPORTED);
            for(unsigned i=0;i<3;++i)put(p+0x330+4*i,bits(mul(number(word(p+0x5c0+4*i)),rate)));
        }
        if(phase==5){
            if(ended)s->substate67_stage=RECOVER;
            else {
                const gh_title_player_view *hit=NULL;
                if(gh_hitarea_object_last_hit_actor(&enemy->hitareas.units[1],&hit))
                    return stop(s,0x223d4c,GH_ACTOR_MOTION_UNSUPPORTED);
                if(hit)p[0x2f6]=6;
                put(p+0x5c8,bits(mul(number(word(p+0x5c8)),0.95f)));
                memcpy(p+0x330,p+0x5c0,12);
            }
        }
        if(phase==7&&(ended||((half(p+0x3ac)&0x10u)&&heat_category(s)>1)))
            s->substate67_stage=RECOVER;
    }
    if(s->substate67_stage==RECOVER){
        gh_actor_motion_status status=choose_after_knockdown_00274fe8(s,p);
        if(status)return status;
        if(s->substate67_phase==5){
            s->substate67_stage=5;
            return advance_state0_tail(s,p);
        }
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=s->substate67_phase==3?CHECK_WALL:GIVE;
    }
    if(s->substate67_stage==CHECK_WALL){
        const gh_title_player_view *hit_actor=NULL;
        if(gh_hitarea_object_last_hit_actor(&enemy->hitareas.units[1],&hit_actor))
            return stop(s,0x223c38,GH_ACTOR_MOTION_UNSUPPORTED);
        float from[3],sine,cosine;
        if(!position(actor,from)||gh_sdk_rotation_sincos_reference_0030a5d0(
            number(word(p+0x104)),&sine,&cosine,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x275888,GH_ACTOR_MOTION_UNSUPPORTED);
        const float height=add(from[1],0.5f);
        gh_scene_ray_request ray={.mask=1,.flags=0x124,.segmented=1,
          .from={from[0],height,from[2],1},
          .to={add(from[0],mul(2,sine)),height,add(from[2],mul(2,cosine)),1}};
        int wall;
        if(!enemy->b.actors||gh_scene_ray_query_00131e10(enemy->b.actors,&ray,&wall,&pc))
            return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
        if(hit_actor||p[0x1864]||init->enemies[0x53d]||init->enemies[0x53c]||wall)
            p[0x2f6]=6;
        s->substate67_stage=BODY;
    }
    if(s->substate67_stage==BODY){
        const gh_actor_motion_status status=body_give_00262750(s,p,1);
        if(status)return status;
        s->substate67_stage=GIVE;
    }
    if(s->substate67_stage==GIVE){
        const gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
        if(status)return status;
        if(half(p+0x3ac)&3u){
            put(p+0x16f8,10);put(p+0x5fc,1);
            put(p+0x16d0,word(p+0x16d0)|0x1000u);
        }
        if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* 2705D8 handles recorded EM20 contact before the ordinary distance gate. */
static gh_actor_motion_status choose_devil_002705d8(gh_enemy202_move *s,uint8_t *p)
{
    gh_game_init_state *init=s->runtime.entry->b.init;
    const float escape=number(word(p+0x16c0)),override=number(word(p+0x1734));
    const float distance=number(word(p+0x618)),rate=number(word(p+0x5a8));
    const float angle=number(word(p+0x760));
    if(word(p+0x738))return stop(s,0x1d0408,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!isfinite(escape)||!isfinite(override)||!isfinite(distance)||!isfinite(rate)||!isfinite(angle))
        return stop(s,0x2705d8,GH_ACTOR_MOTION_UNSUPPORTED);
    unsigned next=0,variant=0;
    if(escape>0)next=0x13;
    else if(override>0)next=0x89;
    else{
        const unsigned heat=heat_category(s);
        if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        if(p[0x1864]){
            if(heat>=3){
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&init->random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(draw&1u)p[0x1864]=0;
            }
            if(word(p+0x16ec)||angle>1.5707964f)p[0x1864]=0;
        }
        if(p[0x1864]){next=0x6b;p[0x1864]=0;}
        else if(distance<9.0f){
            if(init->heat[0x10]||rate<1.0f)next=0x6b;
            else if(heat>=3){next=0x12;variant=1;}
        }
    }
    enemy_decision(p,0,(uint8_t)next,(uint8_t)variant);
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_attack_0021a538(gh_enemy202_move *s,uint8_t *p)
{
    enum { SELECT_ATTACK=160, OPEN_ATTACK, TRACK_ATTACK, STEP_ATTACK,
        ATTACK_ROTATION, ATTACK_TRANSLATION, FINISH_ATTACK, CHOOSE_REVERSE, ATTACK_EFFECT,
        ATTACK_FOLLOWUP };
    const gh_enemy_move_actor *runtime=&s->runtime;
    gh_actor_motion *motion=runtime->motion;
    gh_game_init_state *init=runtime->entry->b.init;
    const gh_boot_work *work=runtime->entry->b.work;
    const uint32_t dependency=word(p+0x564);
    const int gate_em0a=r102_gate_em0a(work,p);
    const int east_em27=r102_east_em27(work,p);
    const int r102=(work->room==0x102&&dependency==0x200)||r102_area11_em02(work,p)||
        r102_gate_reinforcement(work,p)||gate_em0a||r102_em03_stamp8(work,p)||east_em27;
    const int r101_wave=s->enemy&&r101_arena_airborne(s->enemy)&&
        ((half(p+0x640)==0x16&&dependency==0x200)||(half(p+0x640)==0x20&&dependency==0x203));
    const int r101=s->pending_ai_pc==0x21a538&&
        (r101_em00_stamp12(work->room,p)||r101_em00_stamp15(work->room,p)||
         r101_em02_stamp5(work->room,p)||r101_em03_stamp0(work->room,p)||r101_scenery_enemy(work,p)||r101_wave);
    const int r105=r105_opening_enemy(work,p);
    const int arena=r103_arena_jumper(runtime->entry);
    const int em27=dependency==0x227&&(r105||east_em27||r101_scenery_enemy(work,p));
    const int em00_family=dependency==0x200||em27;
    const int boss=r103_arena_boss(runtime->entry);
    const int em14=boss&&dependency==0x214;
    const int r104=s->pending_ai_pc==0x21a538&&p[0x2f5]==0x15&&!p[0x2f7]&&
        gh_enemy203_is_r104_entry(&runtime->entry->request,work->room);
    const int default_attack=em00_family||boss;
    const int arena_em0a=r103_arena_em0a(runtime->entry);
    const int em0a=(dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3)||gate_em0a||arena_em0a;
    const int reverse=(work->room==0x100||gate_em0a||arena_em0a)&&em0a&&s->pending_ai_pc==0x231ac0;
    const int r104_wide=s->pending_ai_pc==0x232500&&dependency==0x250&&p[0x2f5]==0x45&&
        !p[0x2f7]&&gh_enemy203_is_r104_entry(&runtime->entry->request,work->room);
    const int wide_kick=(gate_em0a||arena_em0a)&&s->pending_ai_pc==0x232500;
    const int west_patrol=r100_west_em03_patrol(work,p);
    const int devil=work->room==0x100&&dependency==0x220&&half(p+0x2fe)==0x220&&
        half(p+0x640)==0xff&&s->pending_ai_pc==0x21a538&&!p[0x2f7];
    uint32_t pc=0;
    if((s->pending_ai_pc==0x231ac0&&!reverse)||(s->pending_ai_pc==0x232500&&!wide_kick&&!r104_wide)||
       (!r101&&!r102&&!r105&&!arena&&!arena_em0a&&!boss&&!devil&&!r104&&!r104_wide&&!west_patrol&&!r100_at2_formation(work,p)&&
        !r100_at2_enemy(work,p)&&!r100_at3_enemy(work,p)&&
        (work->room!=0x100||(!em00_family&&!em0a&&(dependency!=0x203||half(p+0x640)!=19))))||
        word(p+0x6ec)||gh_enemy_timers_weapon_kind(runtime->timers))
        return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        if(p[0x2f6]>1||(em0a&&p[0x2f7]))
            return stop(s,0x21f8f8,GH_ACTOR_MOTION_UNSUPPORTED);
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        gh_actor_motion_status status=enemy_attack_record_contact_00262a10(s,p);
        if(status)return status;
        s->substate67_stage=p[0x2f6]?TRACK_ATTACK:SELECT_ATTACK;
    }
    if(s->substate67_stage==SELECT_ATTACK&&(wide_kick||r104_wide)){
        /* 232500 selects the dependency-specific Give without drawing RNG. */
        const unsigned heat=heat_category(s);
        static const float factors[]={0,.75f,.8f,.9f,.9f,1.0f};
        fenv_t env;
        if(!heat||!set_round(&env,FE_TOWARDZERO))return stop(s,0x232ef0,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint32_t tracking=(uint32_t)mul(15.0f,factors[heat]);
        if(fesetenv(&env))return stop(s,0x232f80,GH_ACTOR_MOTION_CALLEE);
        put(p+0x5f0,tracking);
        put(p+0x5f4,0);
        put(p+0x5f8,1);
        put(p+0x5fc,0);
        put(p+0x604,r104_wide?0x420c0000u:0);
        put(p+0x608,0x3f800000u);
        put(p+0x60c,0);
        p[0x1864]=0;
        if(r104_wide){
            /* Dependency 0250 uses Give45 and a 35-frame brace timer. */
            stamp19_attack_parameters(p,50,0x1b,0x45);
            s->reaction_resource_offset=0x19e8;
            s->substate67_phase=heat==5?8:4;
        }else{
            stamp19_attack_parameters(p,20,0x29,0x42);
            for(unsigned offset=0x1870;offset<=0x1884;offset+=0x14){
                put(p+offset+12,0);
                put_half(p+offset+16,0xf6);
            }
            s->reaction_resource_offset=0x8f0;
        }
        s->substate67_stage=OPEN_ATTACK;
    }
    if(s->substate67_stage==SELECT_ATTACK&&devil){
        const unsigned heat=heat_category(s);
        const float distance=number(word(p+0x618));
        static const uint8_t tracking[]={0,11,12,13,13,15};
        if(!heat||heat>=sizeof(tracking)||!isfinite(distance))
            return stop(s,0x21f648,GH_ACTOR_MOTION_UNSUPPORTED);
        const int close=distance<=4.0f;
        /* Both authored 0220 branches select a fixed motion without an MT draw. */
        p[0x1864]=0;
        put(p+0x5f0,tracking[heat]);put(p+0x5f4,0);put(p+0x5fc,0);
        put(p+0x604,0);put(p+0x608,0x3f800000u);put(p+0x60c,0);
        stamp19_attack_parameters(p,40,close?0:0xd,0x3e);
        s->reaction_resource_offset=close?0x1c30:0x1c44;
        s->substate67_stage=OPEN_ATTACK;
    }
    if(s->substate67_stage==SELECT_ATTACK&&reverse){
        float facing;
        if(gh_id_angle_reference_001f7d48(add(number(word(p+0x104)),3.1415927f),&facing))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x600,bits(facing));
        put(p+0x604,0);
        put(p+0x608,0);
        put(p+0x5fc,0);
        p[0x1864]=0;
        const int west_em0a=work->room==0x100&&dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3;
        /* The West dependency table targets 231BB0: power20, shape0, Give3E. */
        stamp19_attack_parameters(p,west_em0a?20:0,west_em0a?0:0x14,0x3e);
        s->reaction_resource_offset=0x8fc;
        s->substate67_stage=OPEN_ATTACK;
    }
    if(s->substate67_stage==SELECT_ATTACK){
        const unsigned heat=heat_category(s);
        const float distance=number(word(p+0x618));
        if(!heat||!isfinite(distance))return stop(s,0x21b1c8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(r104&&heat!=1)return stop(s,0x21def4,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t random;
        if(gh_game_mt_next_0030f348(&init->random,&random))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned choice=random%3;
        unsigned row,power=20,shape=0,kind=0x37,tracking=15,timed=0;
        uint32_t target=UINT32_MAX;
        uint16_t cue=0;
        uint32_t speed=0x3f800000u;
        if(r104){
            static const uint16_t rows[]={0x1974,0x19d0,0x19e0,0x924};
            const unsigned variant=random&3u;
            unsigned sequence_delta=4;
            row=rows[variant];
            power=60;
            kind=variant==3?0x3c:0x3e;
            target=1;
            cue=0x28;
            if(variant<2){
                if(gh_game_mt_next_0030f348(&init->random,&random))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                sequence_delta=random&1u?(variant?4u:8u):12u;
            }
            s->substate67_phase=(uint8_t)sequence_delta;
        }else if(em0a){
            const float face=number(word(p+0x760));
            if(!isfinite(face))return stop(s,0x21d0a0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(distance<2.89f){
                static const uint16_t rows[]={0x93c,0x8d8,0x918,0x8cc};
                const unsigned variant=random&3u;
                row=rows[variant];
                power=(variant==0||variant==2)?15:20;
                shape=(variant==0||variant==2)?2:0;
                kind=variant==0?0x3e:variant==1?0x3c:0x37;
                if(variant&1u){target=0;cue=0xf6;}
            }else{
                static const uint16_t rows[]={0x8cc,0x8cc,0x924,0x930};
                const unsigned variant=random&3u;
                row=rows[variant];
                if(variant<2){target=0;cue=0xf6;}
                if(variant==3){power=15;shape=12;}
            }
            if(face>number(0x3f060a92)){
                if(gh_game_mt_next_0030f348(&init->random,&random))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                int32_t signed_roll;
                memcpy(&signed_roll,&random,4);
                if(!(signed_roll%3)){
                    row=0x8f0;power=20;shape=0;kind=0x3e;target=0;cue=0xf6;
                }
            }
            if(heat>1){
                if(gh_game_mt_next_0030f348(&init->random,&random))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(random&1u){
                    row=0x8e4;power=20;shape=0;kind=0x37;target=0;cue=0xf6;timed=1;
                }
            }
        }else if(default_attack){
            /* 21A654 uses the shared resource rows, then applies
             * the independent alternate and distance decisions at 21A860. */
            if(heat==1||(!boss&&!r105&&heat==5))row=(random&1u)?0x1ac:0x150;
            else if(choice==0){row=0x12c;power=15;}
            else row=choice==1?0x150:0x1ac;
            if(em14){
                uint32_t special;
                if(gh_game_mt_next_0030f348(&init->random,&special))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(special&1u){row=0x1b18;shape=0x11;kind=0x4e;power=20;}
            }
            uint32_t alternate;
            if(gh_game_mt_next_0030f348(&init->random,&alternate))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(alternate&1u){
                tracking=30;
                if(!boss&&p[0x186b]>=5){row=0x21c;power=20;}
            }
            if(distance>number(0x4038f5c3u)){row=0x144;shape=1;kind=0x37;power=20;}
            if(distance>4.0f){
                uint32_t far_draw;
                if(gh_game_mt_next_0030f348(&init->random,&far_draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                row=(far_draw&1u)?0x18c:0x19c;
                timed=!(far_draw&1u);
                shape=0;power=timed?15:20;
                if(em27){
                    uint32_t variant;
                    if(gh_game_mt_next_0030f348(&init->random,&variant))
                        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                    if(variant&1u){
                        row=0x6b8;power=40;kind=0x3f;tracking=50;speed=0x40000000u;
                    }
                }
            }
        }else if(distance<2.89f){
            shape=2;
            if(choice==2){row=0x6b0;power=60;kind=0x3e;tracking=50;}
            else{row=heat>1&&choice!=1?0x6f4:0x6a4;power=20;kind=0x37;}
        }else{
            shape=0;
            if(heat>1&&choice==2){row=0x6dc;power=20;kind=0x37;}
            else{row=0x69c;power=30;kind=0x3e;tracking=50;}
        }
        static const float factors[]={0,.75f,.8f,.9f,.9f,1};
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x21f648,GH_ACTOR_MOTION_CALLEE);
        const uint32_t track_frames=(uint32_t)mul((float)tracking,factors[heat]);
        if(fesetenv(&env))return stop(s,0x21f648,GH_ACTOR_MOTION_CALLEE);
        put(p+0x5f0,track_frames);
        put(p+0x5f4,timed);put(p+0x5fc,0);put(p+0x604,0);put(p+0x608,speed);put(p+0x60c,0);
        p[0x1864]=0;
        stamp19_attack_parameters(p,power,shape,kind);
        if(em0a||r104){
            put(p+0x187c,target);put_half(p+0x1880,cue);
            put(p+0x1890,target);put_half(p+0x1894,cue);
        }
        s->reaction_resource_offset=(uint16_t)row;
        s->substate67_stage=OPEN_ATTACK;
    }
    if(s->substate67_stage==OPEN_ATTACK){
        const gh_object_view *resource=&runtime->entry->resource;
        const size_t row=s->reaction_resource_offset;
        if(!s->substate67_motion.active){
            const unsigned heat=heat_category(s);
            if(reverse&&!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            const int long_sequence=(em27&&row==0x6b8)||
                (heat==5&&(default_attack||em0a||row==0x6a4||row==0x6dc||row==0x6f4||row==0x1c30));
            const size_t seq=row+((r104||r104_wide)?s->substate67_phase:long_sequence?8:4);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent>resource->size||
               resource->file_extent<seq+4)
                return stop(s,0x21f5a4,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                &s->substate67_motion,motion,&motion->b,word(resource->bytes+row),
                word(resource->bytes+seq),0,reverse?3:10,p[0x17c3]?2:0,
                wide_kick||devil||west_patrol||(em00_family&&!r105&&!arena&&!r101));
            if(status)return stop(s,s->substate67_motion.frontier?s->substate67_motion.frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(&s->substate67_motion);
        if(status)return stop(s,s->substate67_motion.frontier?s->substate67_motion.frontier:0x2a8578,status);
        s->substate67_stage=ATTACK_EFFECT;
    }
    if(s->substate67_stage==ATTACK_EFFECT){
        if(devil&&s->reaction_resource_offset==0x1c30){
            gh_effect_create_call *effect=&s->death_effect_create;
            const gh_title_player_view *actor=&runtime->entry->object.view;
            gh_effect_create_bindings bindings=s->impact_bindings;
            if(!bindings.storage.effects||bindings.work!=work||bindings.random!=&init->random)
                return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
            bindings.model=motion->b.model;
            if(!effect->active){
                const unsigned heat=heat_category(s);
                if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
                const gh_effect_create_request request={
                    .bank=0x58,.index=heat==5?0x4d:0x4c,.actor=actor,.mode=0,.selector=-1};
                const uint32_t scale=(half(p+0x434)&2u)?word(p+0x114):0x3f800000u;
                put(p+0x574,0);
                put_half(p+0x570,0);
                if(gh_effect_create_actor_sequence_0014fb48(effect,&request,scale))
                    return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            if(gh_game_effect_create_00306528(effect,&bindings)||!effect->complete)
                return stop(s,effect->frontier?effect->frontier:0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(reverse){
            const unsigned heat=heat_category(s);
            static const uint8_t tracking[]={0,22,24,27,27,30};
            if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f0,tracking[heat]);
        }else put(p+0x600,word(p+0x104));
        ++p[0x2f6];
        s->substate67_stage=TRACK_ATTACK;
    }
    if(s->substate67_stage==TRACK_ATTACK){
        const float rate=number(word(p+0x5a8));
        if((!r104_wide&&word(p+0x604))||!isfinite(rate))
            return stop(s,0x21f7d8,GH_ACTOR_MOTION_UNSUPPORTED);
        const float brace=number(word(p+0x604));
        if(r104_wide&&brace>0.0f){
            put(p+0x16d0,word(p+0x16d0)|0x00800000u);
            put(p+0x604,bits(sub(brace,rate)));
        }
        const uint32_t tracking=word(p+0x5f0);
        if(tracking){
            float origin[3],player[3],delta;
            if(!position(&runtime->entry->object.view,origin)||!position(s->player,player))
                return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
            const float previous=number(word(p+0x600)),yaw=number(word(p+0x104));
            if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,origin,player,previous,
                mul(rate,0.19634955f),&delta,&pc))
                return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            float track_angle,actor_angle;
            if(gh_id_angle_reference_001f7d48(add(previous,delta),&track_angle)||
               gh_id_angle_reference_001f7d48(add(yaw,delta),&actor_angle))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f0,tracking-1);
            put(p+0x600,bits(track_angle));
            put(p+0x104,bits(actor_angle));
        }
        s->substate67_stage=STEP_ATTACK;
    }
    if(s->substate67_stage==STEP_ATTACK){
        int ended;
        if(!runtime->bindings->canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*runtime->bindings->canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state71_exit=(uint8_t)ended;
        if(em27&&!ended&&word(p+0x608)==0x40000000u)
            put(p+0x338,bits(mul(number(word(p+0x338)),2.0f)));
        s->substate67_stage=reverse&&ended?CHOOSE_REVERSE:ATTACK_ROTATION;
    }
    if(s->substate67_stage==CHOOSE_REVERSE){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ATTACK_ROTATION;
    }
    if(s->substate67_stage==ATTACK_ROTATION){
        if(!reverse&&word(p+0x608)!=0x3f800000u&&(!em27||word(p+0x608)!=0x40000000u))
            return stop(s,0x21f8d0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_player_root_rotation_0014f9f0(&runtime->entry->object.view,runtime->entry->b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ATTACK_TRANSLATION;
    }
    if(s->substate67_stage==ATTACK_TRANSLATION){
        if(gh_player_root_translation_0014f928(&runtime->entry->object.view,runtime->entry->b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=FINISH_ATTACK;
    }
    if(s->substate67_stage==FINISH_ATTACK){
        if(reverse){
            if(!p[0x1864]){
                gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
                if(status)return status;
            }
            if(half(p+0x3ac)&3u)put(p+0x5fc,1);
            if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        }else if(s->state71_exit){
            if(!wide_kick&&!r104_wide&&(word(s->player->bytes+0x15f4)&0x1000u))enemy_decision(p,0,0x6c,0);
            else{
                gh_actor_motion_status status=devil?choose_devil_002705d8(s,p):
                    choose_after_substate_002705d8(s,p);
                if(status)return status;
            }
        }else if(devil){
            const unsigned flags=half(p+0x3ac);
            if(word(p+0x5f4))stamp19_attack_parameters(p,flags&2u?20:5,0,flags&2u?0x3f:0x3d);
            gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
            if(status)return status;
            s->substate67_stage=ATTACK_FOLLOWUP;
        }else{
            const unsigned hit_flags=half(p+0x3ac);
            const uint32_t timed=word(p+0x5f4);
            const unsigned stamp=half(p+0x640);
            const int opening=work->room==0x100&&!p[0x1650]&&
                ((dependency==0x203&&stamp==19)||(dependency==0x200&&stamp==20));
            const int west_em0a=work->room==0x100&&em0a&&dependency==0x20a&&stamp==0&&
                p[0x1650]==3&&!p[0x2f7];
            const int west_patrol_em00=work->room==0x100&&dependency==0x200&&stamp==10&&
                p[0x1650]==5&&word(p+0x16c4)==6&&!p[0x2f7];
            const int west_followup=r100_at3_enemy(work,p)||opening||west_em0a||west_patrol_em00;
            if((timed&&((!default_attack&&!em0a)||timed!=1))||(!default_attack&&!west_em0a&&word(p+0x5fc))||
               (heat_category(s)>=3&&(hit_flags&0x10u)&&!west_followup))
                return stop(s,0x21f8f8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(timed){
                /* 21FB34 default timed row19C rewrites both Give parameter
                 * sets each nonterminal frame before the hit-area test. */
                stamp19_attack_parameters(p,hit_flags&2u?20:15,0,hit_flags&2u?0x37:0x3d);
            }
            gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
            if(status)return status;
            if(west_followup)s->substate67_stage=ATTACK_FOLLOWUP;
            else if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        }
        if(s->substate67_stage!=ATTACK_FOLLOWUP)s->substate67_stage=5;
    }
    if(s->substate67_stage==ATTACK_FOLLOWUP){
        /* 21FC4C queries the authored post-Give gate. 12C530 reads Gene's
         * Devil activation flag before 262AA8 can choose a follow-up. */
        int left_attack=0;
        if(heat_category(s)>=3&&(half(p+0x3ac)&0x10u)){
            if(word(s->player->bytes+0x15f4)&0x1000u){
                enemy_decision(p,0,0x6c,0);
                left_attack=1;
            }else if(!p[0x1864]){
                const gh_actor_motion_status status=devil?devil_near_choice_00262aa8(s,p,0,&left_attack):
                    attack_choice_00262aa8(s,p,&left_attack);
                if(status)return status;
            }
        }
        if(!left_attack&&word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* The EM02/EM03 branch of the original Take counter opens the common
 * dodge clip at sample two, then chooses a fresh attack at its SEQ gate. */
static gh_actor_motion_status advance_wave_counter_0023e6b0(gh_enemy202_move *s,uint8_t *p)
{
    enum { OPEN=110, RETARGET, TURN, SAMPLE, ROTATE, TRANSLATE, GIVE, FINISH, BIND };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_boot_work *work=enemy->b.entry.work;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    const int opening_em03=work->room==0x100&&word(p+0x564)==0x203&&
        half(p+0x640)==19&&!p[0x1650];
    if((!r100_at2_enemy(work,p)&&!r100_at3_enemy(work,p)&&!opening_em03)||p[0x2f7]||
       word(p+0x6ec)||gh_enemy_timers_weapon_kind(&enemy->timers))
        return stop(s,0x23e6b0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        put(p+0x16d4,word(p+0x16d4)|0x40000u);
        s->substate67_stage=p[0x2f6]==0?OPEN:p[0x2f6]==1?TURN:GIVE;
    }
    if(s->substate67_stage==OPEN){
        p[0x1864]=0;
        put(p+0x54c,0x40a00000u);
        stamp19_attack_parameters(p,20,0,0x37);
        s->reaction_resource_offset=0x3ac;
        if(r100_at3_enemy(work,p)&&word(p+0x564)==0x200){
            /* 23E784 draws once for unarmed EM00. At heat category three
             * or higher the even draw selects the alternate and cooldown ten. */
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!(draw&1u)&&heat_category(s)>=3){
                s->reaction_resource_offset=0x3b4;
                put(p+0x54c,0x41200000u);
            }
        }
        s->substate67_stage=BIND;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        const gh_object_view *resource=&enemy->entry.resource;
        if(!call->active){
            const size_t row=s->reaction_resource_offset;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x23eca4,GH_ACTOR_MOTION_BOUNDS);
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0x40000000u,2,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        s->substate67_stage=RETARGET;
    }
    if(s->substate67_stage==RETARGET){
        if(word(p+0x6f0)||word(p+0x708))
            return stop(s,word(p+0x6f0)?0x1d04d0:0x1d05c0,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_hitarea_object *body=&enemy->hitareas.units[0];
        const gh_title_player_view *source=NULL;
        if(!enemy->hitareas.created[0]||word(p+0x698)||
            (!body->fallback&&(gh_hitarea_object_last_hit_actor(body,&source)||
              (!source&&word(body->bytes+0x34)))))
            return stop(s,0x23ed28,GH_ACTOR_MOTION_UNSUPPORTED);
        if(source){
            float target[3];
            if(!position(source,target))return stop(s,0x23ed40,GH_ACTOR_MOTION_UNSUPPORTED);
            memcpy(p+0x5d0,target,12);
        }
        put(p+0x5f0,source?10:0);
        p[0x2f6]=1;
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        const uint32_t remaining=word(p+0x5f0);
        if(remaining){
            float origin[3],target[3],delta;
            memcpy(target,p+0x5d0,12);
            const float rate=number(word(p+0x5a8));
            const float work_rate=number(word((const uint8_t *)work+0x55c));
            const float yaw=number(word(p+0x104));
            fenv_t env;
            if(!position(actor,origin)||!isfinite(rate)||!isfinite(work_rate)||
                !set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float cap=mul(mul(rate,number(0x3ec90fdb)),work_rate);
            const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
                origin,target,yaw,cap,&delta,&pc);
            float next=status?yaw:add(yaw,delta);
            if(fesetenv(&env)||status||gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f0,remaining-1u);
            put(p+0x104,bits(next));
        }
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended||(half(p+0x3ac)&0x10u)?FINISH:ROTATE;
    }
    if(s->substate67_stage==FINISH){
        const unsigned rank=heat_category(s);
        if(!rank||rank>5)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t draw=1;
        if(rank>=2&&rank<5&&gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        put_half(p+0x17ca,30);
        put(p+0x16dc,0);
        const int follow_up=rank==5||!(draw&3u);
        enemy_decision(p,0,follow_up?0x16:0x15,0);
        s->substate67_stage=follow_up?5:ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=GIVE;
    }
    if(s->substate67_stage==GIVE){
        const gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status formation_attack_peer(gh_enemy202_move *s,uint32_t handle,
    gh_title_player_view *peer,uint32_t pc)
{
    if(!s->targets.resolve)return stop(s,pc,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t frontier=0;
    const gh_player_target_status status=s->targets.resolve(s->targets.context,handle,peer,&frontier);
    if(status)return stop(s,frontier?frontier:pc,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!peer->bytes||peer->size<0x550)return stop(s,pc,GH_ACTOR_MOTION_BOUNDS);
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status turn_formation_target_002a7498(gh_enemy202_move *s,
    uint8_t *p,const gh_title_player_view *target_actor)
{
    const gh_title_player_view *actor=&s->enemy->entry.object.view;
    const gh_boot_work *work=s->enemy->b.entry.work;
    uint32_t pc=0;
    float origin[3],target[3],turn;
    const float yaw=number(word(p+0x104)),rate=number(word(p+0x5a8));
    const float work_rate=number(word((const uint8_t *)work+0x55c));
    fenv_t env;
    if(!position(actor,origin)||!position(target_actor,target)||!isfinite(rate)||
        !isfinite(work_rate)||!set_round(&env,FE_TOWARDZERO))
        return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
    const float cap=mul(mul(rate,0.19634955f),work_rate);
    gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
        origin,target,yaw,cap,&turn,&pc);
    float next=status?0:add(yaw,turn);
    if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    if(gh_id_angle_reference_001f7d48(next,&next))
        return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
    put(p+0x104,bits(next));
    return GH_ACTOR_MOTION_OK;
}

/* EM02 selector18 binds its own kick/capture clip and original Give 4A. */
static gh_actor_motion_status advance_formation_em02_00223470(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=245, TRACK, STEP, CHOOSE, ROTATE, TRANSLATE, EFFECT, GIVE, RECHECK, FINISH };
    gh_enemy_actor *enemy=s->enemy;
    const gh_boot_work *work=enemy->entry.b.work;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    const uint32_t dependency=word(p+0x564);
    const int west_em02=dependency==0x202&&
        (r100_at2_formation(work,p)||r100_at2_enemy(work,p)||r100_at3_enemy(work,p));
    const int opening_em03=work->room==0x100&&dependency==0x203&&half(p+0x640)==19&&!p[0x1650];
    if((!west_em02&&!opening_em03)||p[0x2f7])
        return stop(s,0x223470,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        s->substate67_stage=p[0x2f6]==0?BIND:p[0x2f6]==1?TRACK:GIVE;
    }
    if(s->substate67_stage==BIND){
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const unsigned heat=heat_category(s);
            const size_t sequence=heat==5?0x6f0:0x6ec;
            if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<sequence+4||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2234e0,GH_ACTOR_MOTION_BOUNDS);
            p[0x1864]=0;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+0x6e8),word(resource->bytes+sequence),
                0,3,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5f4,0);put(p+0x5fc,0);put(p+0x5f0,15);put(p+0x16dc,0);
        ++p[0x2f6];
        s->substate67_stage=TRACK;
    }
    if(s->substate67_stage==TRACK){
        const uint32_t tracking=word(p+0x5f0);
        if(tracking){
            gh_actor_motion_status status=turn_formation_target_002a7498(s,p,s->player);
            if(status)return status;
            put(p+0x5f0,tracking-1);
        }
        s->substate67_stage=STEP;
    }
    if(s->substate67_stage==STEP){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(ended)put(p+0x16dc,0);
        s->substate67_stage=ended?CHOOSE:ROTATE;
    }
    if(s->substate67_stage==CHOOSE){
        int chosen;
        gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
        if(status)return status;
        if(chosen){s->substate67_stage=5;return advance_state0_tail(s,p);}
        status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=EFFECT;
    }
    if(s->substate67_stage==EFFECT){
        if(!word(p+0x5f4)&&(half(p+0x3ac)&2u)){
            gh_effect_create_bindings bindings=s->impact_bindings;
            if(!bindings.storage.effects||bindings.work!=work)
                return stop(s,0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
            bindings.model=motion->b.model;
            gh_effect_create_call *effect=&s->death_effect_create;
            if(!effect->active||effect->complete){
                const gh_effect_create_request request={.bank=0x58,.index=0x29,
                    .actor=actor,.mode=0,.selector=-1};
                if(gh_effect_create_begin(effect,&request))
                    return stop(s,0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            if(gh_game_effect_create_00306528(effect,&bindings)||!effect->complete)
                return stop(s,effect->frontier?effect->frontier:0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f4,1);
        }
        s->substate67_stage=GIVE;
    }
    if(s->substate67_stage==GIVE){
        stamp19_attack_parameters(p,0,0xf,0x4a);
        if(half(p+0x3ac)&1u){
            gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
            if(status)return status;
        }
        s->substate67_stage=RECHECK;
    }
    if(s->substate67_stage==RECHECK){
        const unsigned heat=heat_category(s);
        if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        if(heat>=3&&(half(p+0x3ac)&0x10u)&&!(number(word(p+0x17cc))>0)){
            int chosen;
            gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
            if(status)return status;
            if(chosen){s->substate67_stage=5;return advance_state0_tail(s,p);}
        }
        s->substate67_stage=FINISH;
    }
    if(s->substate67_stage==FINISH){
        if(half(p+0x3ac)&3u)put(p+0x5fc,1);
        if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* The original west EM00's 42 uses the ordinary 1B8/1BC clip row. */
static gh_actor_motion_status advance_formation_00230f20(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=230, TRACK, STEP, CHOOSE, ROTATE, TRANSLATE, MARKERS, SOLID, GIVE };
    gh_enemy_actor *enemy=s->enemy;
    const gh_boot_work *work=enemy->entry.b.work;
    gh_title_player_view *actor=&enemy->entry.object.view;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    if(word(p+0x564)!=0x200||!r100_at2_formation(work,p)||p[0x2f7])
        return stop(s,0x230f20,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        gh_actor_motion_status status=enemy_attack_record_contact_00262a10(s,p);
        if(status)return status;
        s->substate67_stage=p[0x2f6]==0?BIND:p[0x2f6]==1?TRACK:MARKERS;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        const gh_object_view *resource=&enemy->entry.resource;
        if(!call->active){
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x1c0||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x230fb8,GH_ACTOR_MOTION_BOUNDS);
            p[0x1864]=0;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+0x1b8),word(resource->bytes+0x1bc),
                0,5,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5f0,22);put(p+0x5f8,15);put(p+0x5f4,0);put(p+0x5fc,0);
        ++p[0x2f6];
        s->substate67_stage=TRACK;
    }
    if(s->substate67_stage==TRACK){
        const uint32_t delay=word(p+0x5f8),tracking=word(p+0x5f0);
        const uint32_t handle=word(p+0x17a0);
        if(tracking&&(handle||delay<=1)){
            const gh_title_player_view *target_actor=s->player;
            gh_title_player_view peer;
            if(handle){
                gh_actor_motion_status status=formation_attack_peer(s,handle,&peer,0x231060);
                if(status)return status;
                target_actor=&peer;
            }
            gh_actor_motion_status status=turn_formation_target_002a7498(s,p,target_actor);
            if(status)return status;
        }
        if(delay)put(p+0x5f8,delay-1);
        if(tracking)put(p+0x5f0,tracking-1);
        s->substate67_stage=STEP;
    }
    if(s->substate67_stage==STEP){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?CHOOSE:ROTATE;
    }
    if(s->substate67_stage==CHOOSE){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=MARKERS;
    }
    if(s->substate67_stage==MARKERS){
        if((half(p+0x3ac)&2u)&&!word(p+0x5f4)){
            const uint32_t handle=word(p+0x17a0);
            if(handle){
                gh_title_player_view peer;
                gh_actor_motion_status status=formation_attack_peer(s,handle,&peer,0x26ec60);
                if(status)return status;
                uint8_t *chosen=peer.bytes;
                if(half(chosen+0x2fe)==half(p+0x2fe)&&!chosen[0x2f4]&&
                   number(word(chosen+0x54c))<=0){
                    chosen[0x2f4]=chosen[0x2f6]=chosen[0x2f7]=0;
                    chosen[0x2f5]=0x65;
                }
            }
            put(p+0x5f4,1);
            put(p+0x17a0,0);
        }
        s->substate67_stage=SOLID;
    }
    if(s->substate67_stage==SOLID){
        if((half(p+0x3ac)&0x20u)&&
           (!enemy->b.solids||gh_collision_solid_set_enabled_001351d8(enemy->b.solids,actor,0,&pc)))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=GIVE;
    }
    if(s->substate67_stage==GIVE){
        const unsigned flags=half(p+0x3ac);
        if(flags&1u){
            stamp19_attack_parameters(p,30,2,0x3e);
            const gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
            if(status)return status;
        }
        if(flags&3u)put(p+0x5fc,1);
        if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* 26EC60's selected cEm00 partner uses the original 1C0/1C4 response. */
static gh_actor_motion_status advance_formation_partner_002311a8(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=239, VOICE, STEP, CHOOSE, ROTATE, TRANSLATE };
    gh_enemy_actor *enemy=s->enemy;
    const gh_boot_work *work=enemy->entry.b.work;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    if(!r100_at2_formation(work,p)||p[0x2f7])
        return stop(s,0x2311a8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage)
        s->substate67_stage=p[0x2f6]==0?BIND:p[0x2f6]==1?STEP:5;
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        const gh_object_view *resource=&enemy->entry.resource;
        if(!call->active){
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x1c8||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x231268,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+0x1c0),word(resource->bytes+0x1c4),
                0,10,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        s->substate67_stage=VOICE;
    }
    if(s->substate67_stage==VOICE){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request cue={.model=motion->b.model,.actor=actor,
            .bank=1,.program=(int16_t)half(p+0x424)};
        uint32_t handle;
        gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&cue,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        put(p+0x54c,0x40000000u);
        ++p[0x2f6];
        s->substate67_stage=STEP;
    }
    if(s->substate67_stage==STEP){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?CHOOSE:ROTATE;
    }
    if(s->substate67_stage==CHOOSE){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original 270DB0 reach test for the default EM00/EM03 size. The
 * short ray consults the canonical scene owner with the original mask. */
static gh_actor_motion_status grab_reach_00270db0(gh_enemy202_move *s,uint8_t *p,int *reached)
{
    gh_enemy_actor *enemy=s->enemy;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_title_player_view *player=s->player;
    const gh_boot_work *work=enemy->b.entry.work;
    *reached=0;
    if(!player||!player->bytes||player->size<0x15f8)
        return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
    const float cooldown=number(word(player->bytes+0x54c));
    if(!isfinite(cooldown))return stop(s,0x270db0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(cooldown>0||(work->system_flags&8u)||!positive_hp(player->bytes)||
       (word(player->bytes+0x15f4)&0x1000u))
        return GH_ACTOR_MOTION_OK;

    float origin[3],target[3],sn,cs,inverse[16],local[4];
    uint32_t pc=0;
    if(!position(actor,origin)||!position(player,target))
        return stop(s,0x147c40,GH_ACTOR_MOTION_UNSUPPORTED);
    if(gh_sdk_rotation_sincos_reference_0030a5d0(number(word(p+0x104)),&sn,&cs,
         GH_STATIC_BINARY32_REFERENCE,&pc))
        return stop(s,pc?pc:0x147988,GH_ACTOR_MOTION_UNSUPPORTED);
    const float basis[16]={cs,0,-sn,0, 0,1,0,0, sn,0,cs,0,
         origin[0],origin[1],origin[2],1};
    if(gh_actor_matrix_inverse_00147568(basis,inverse,GH_STATIC_BINARY32_REFERENCE,&pc))
        return stop(s,pc?pc:0x147568,GH_ACTOR_MOTION_UNSUPPORTED);
    if(gh_scene_at_apply_world_001508d8(inverse,target,local))
        return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
    const float half_width=number(0x3f333333);
    const float depth=word(p+0x564)==0x20a?number(0x3fe66666):1.5f;
    if(local[0]<-half_width||local[0]>half_width||local[1]<-0.5f||local[1]>0.5f||
       local[2]<0||local[2]>depth)
        return GH_ACTOR_MOTION_OK;

    gh_scene_ray_request ray={.mask=1,.flags=0x10,.segmented=1};
    memcpy(ray.from,origin,12);
    memcpy(ray.to,target,12);
    ray.from[3]=ray.to[3]=1;
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x2710b8,GH_ACTOR_MOTION_CALLEE);
    ray.from[1]=add(ray.from[1],0.5f);
    ray.to[1]=add(ray.to[1],0.5f);
    if(fesetenv(&env))return stop(s,0x2710b8,GH_ACTOR_MOTION_CALLEE);
    int hit;
    if(!enemy->b.actors||gh_scene_ray_query_00131e10(enemy->b.actors,&ray,&hit,&pc))
        return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    *reached=!hit;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_move_bind_player_grab(gh_enemy202_move *s,
    gh_enemy202_player_grab grab,void *context,gh_game_action_buttons *buttons)
{
    if(!s||!grab||!buttons)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->stage&&(s->player_grab!=grab||s->player_grab_context!=context||s->grab_buttons!=buttons))
        return GH_ACTOR_MOTION_ARGUMENT;
    s->player_grab=grab;
    s->player_grab_context=context;
    s->grab_buttons=buttons;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_substate4b_00234d30(gh_enemy202_move *s,uint8_t *p)
{
    enum { OPEN_MOTION=170, ADVANCE_OFFSET, TURN, SAMPLE, CHOOSE, ROTATE, TRANSLATE, FINISH, GRAB_SOLID, GRAB_PLAYER, TAIL };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_boot_work *work=enemy->b.entry.work;
    gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    const uint32_t dependency=word(p+0x564);
    const int r105=r105_opening_enemy(work,p);
    const int r101_key_carrier=r101_scenery_enemy(work,p)&&!p[0x2f7];
    const int em00=dependency==0x200||r105_exchanged_enemy(work,p)||r101_key_carrier;
    const unsigned stamp=half(p+0x640);
    const int east_unarmed=stamp==3&&p[0x1650]==1&&
        !gh_enemy_timers_room_weapon(&enemy->timers)&&!word(p+0x6ec);
    const int r102_em00=work->room==0x102&&dependency==0x200&&!p[0x2f7]&&
        ((stamp==11&&p[0x1650]==5)||east_unarmed);
    /* The AT0B EM00 dispatches to the unarmed 234DE0 resource rows. */
    const int area11_em00=r102_area11_obstacle(work,p)&&word(p+0x16c4)==2&&!p[0x2f7];
    /* The AT0F gate guard also takes the original 234DE0 unarmed rows. */
    const int gate_guard=work->room==0x102&&dependency==0x200&&half(p+0x640)==0x10&&
        !p[0x1650]&&!word(p+0x16c4)&&!p[0x2f7];
    const int wave_em02=dependency==0x202&&(r100_at2_enemy(work,p)||r100_at3_enemy(work,p))&&!p[0x2f7];
    const int gate_em0a=r102_gate_em0a(work,p)&&!p[0x2f7];
    const int arena_em0a=r103_arena_em0a(&enemy->entry)&&!p[0x2f7];
    const int em0a=arena_em0a||(dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3)||gate_em0a;
    const int arena=r103_arena_jumper(&enemy->entry)&&!p[0x2f7];
    /* Both original table rows take 235338; only EM15's success goes to 235A9C. */
    const int boss_grab=r103_arena_boss(&enemy->entry)&&!p[0x2f7];
    const int boss_em15=boss_grab&&dependency==0x215;
    const int side_em00=r101_arena_airborne(enemy)&&dependency==0x200&&stamp==0x16;
    const int r101=(r101_em00_stamp12(work->room,p)||r101_key_carrier||side_em00)&&!p[0x2f7];
    const int r104=gh_enemy203_is_r104_entry(&enemy->entry.request,work->room)&&!p[0x2f7];
    if(!r101&&!r104&&!r102_em00&&!area11_em00&&!gate_guard&&!gate_em0a&&!r102_area11_em02(work,p)&&
       !r102_gate_reinforcement(work,p)&&!r105&&!arena&&!arena_em0a&&!boss_grab&&!wave_em02&&
       (work->room!=0x100||(!em00&&!em0a&&(dependency!=0x203||half(p+0x640)!=19))))
        return stop(s,0x234d30,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        if(p[0x2f6]>1)return stop(s,0x235850,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=p[0x2f6]?ADVANCE_OFFSET:OPEN_MOTION;
    }
    if(s->substate67_stage==OPEN_MOTION){
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const unsigned rank=heat_category(s);
            /* 439E08/0C dispatch the two bosses to the same rank-dependent motion. */
            const size_t row=r104?0x1a58:boss_grab?0x1b4c:em0a?0xc5c:em00?0x50c:0x7cc;
            const size_t sequence=row+(rank==5?8:4);
            if(!rank)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<sequence+4||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x234e24,GH_ACTOR_MOTION_BOUNDS);
            static const float factor[]={0,.75f,.8f,.9f,.9f,1};
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x235648,GH_ACTOR_MOTION_CALLEE);
            const uint32_t tracking=(uint32_t)mul((r104||em0a)?40.0f:boss_grab?30.0f:20.0f,factor[rank]);
            if(fesetenv(&env))return stop(s,0x235648,GH_ACTOR_MOTION_CALLEE);
            p[0x1864]=0;
            put(p+0x5f8,0);
            put(p+0x5f0,tracking);
            put(p+0x5f4,r104||em0a?30:0);
            memset(p+0x580,0,12);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+sequence),
                0,10,p[0x17c3]?2:0,wave_em02||(em00&&!r101&&!r102_em00&&!area11_em00&&!gate_guard&&!r105&&!arena));
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x60c,0);
        put(p+0x5fc,0);
        p[0x2f6]=1;
        s->substate67_stage=ADVANCE_OFFSET;
    }
    if(s->substate67_stage==ADVANCE_OFFSET){
        float origin[3],offset[3];
        const float rate=number(word(p+0x5a8));
        fenv_t env;
        if(!position(actor,origin)||!isfinite(rate)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x23568c,GH_ACTOR_MOTION_UNSUPPORTED);
        for(unsigned i=0;i<3;++i){
            const float old=number(word(p+0x580+4*i));
            const float delta=mul(mul(old,rate),0.1f);
            origin[i]=add(origin[i],delta);
            offset[i]=sub(old,delta);
        }
        if(fesetenv(&env)||!isfinite(origin[0])||!isfinite(origin[1])||!isfinite(origin[2])||
           !isfinite(offset[0])||!isfinite(offset[1])||!isfinite(offset[2]))
            return stop(s,0x235718,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(actor->alias_f0,origin,12);
        memcpy(p+0x580,offset,12);
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        const uint32_t tracking=word(p+0x5f0);
        if(tracking){
            float origin[3],target[3],angle;
            const float rate=number(word(p+0x5a8));
            const float game_rate=number(word((const uint8_t *)work+0x55c));
            const float yaw=number(word(p+0x104));
            fenv_t env;
            if(!position(actor,origin)||!position(s->player,target)||!isfinite(rate)||
                !isfinite(game_rate)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float cap=mul(mul(rate,number(0x3e860a92)),game_rate);
            const gh_player_target_status status=gh_player_target_angle_002da038(
                s->elf,s->elf_bytes,origin,target,yaw,cap,&angle,&pc);
            const float turned=status?yaw:add(yaw,angle);
            if(fesetenv(&env)||status||gh_id_angle_reference_001f7d48(turned,&angle))
                return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x104,bits(angle));
            put(p+0x5f0,tracking-1);
        }
        const uint32_t timed=word(p+0x5f4);
        if(timed){
            put(p+0x5f4,timed-1);
            put(p+0x16d0,word(p+0x16d0)|0x800000u);
        }
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state71_exit=(uint8_t)ended;
        s->substate67_stage=CHOOSE;
    }
    if(s->substate67_stage==CHOOSE){
        if(s->state71_exit){
            const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
            if(status)return status;
        }
        const float distance=number(word(p+0x618));
        if(!isfinite(distance))return stop(s,0x2357f0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(distance<number(0x3fd851ea)){
            const float speed=number(word(p+0x338));
            fenv_t env;
            if(!isfinite(speed)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x235824,GH_ACTOR_MOTION_UNSUPPORTED);
            const float slowed=mul(speed,0.2f);
            if(fesetenv(&env))return stop(s,0x235824,GH_ACTOR_MOTION_CALLEE);
            put(p+0x338,bits(slowed));
        }
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=FINISH;
    }
    if(s->substate67_stage==FINISH){
        const unsigned flags=half(p+0x3ac);
        if(flags&3u){
            put(p+0x5f4,0);
            int reached;
            gh_actor_motion_status status=grab_reach_00270db0(s,p,&reached);
            if(status)return status;
            if(reached&&boss_em15){
                /* 43A02C selects 235A9C, which starts the original paired throw. */
                enemy_decision(p,0,0x52,0);
                s->substate67_stage=GRAB_PLAYER;
            }else if(reached&&boss_grab){
                /* EM14's 43A028 dispatches to 2358D4 and the standard player hold. */
                enemy_decision(p,0,0x4e,0);
                s->substate67_stage=GRAB_PLAYER;
            }else if(reached&&(em0a||r104)){
                enemy_decision(p,0,0x4c,0);
                s->substate67_stage=GRAB_PLAYER;
            }else if(reached){
                put(p+0x54c,0x41f00000);
                s->substate67_stage=GRAB_SOLID;
            }
        }
        if(s->substate67_stage==FINISH)s->substate67_stage=TAIL;
    }
    if(s->substate67_stage==GRAB_SOLID){
        if(!enemy->b.solids||gh_collision_solid_set_enabled_001351d8(enemy->b.solids,actor,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        enemy_decision(p,0,em00?0x4d:0x4e,0);
        s->substate67_stage=GRAB_PLAYER;
    }
    if(s->substate67_stage==GRAB_PLAYER){
        if(!s->player_grab)return stop(s,0x129718,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=s->player_grab(s->player_grab_context,enemy,boss_em15?7u:r104?1u:em0a?0u:em00?2u:3u,&pc);
        if(status)return stop(s,pc?pc:0x129718,status);
        s->substate67_stage=boss_em15?TAIL:5;
    }
    if(s->substate67_stage==TAIL){
        const unsigned flags=half(p+0x3ac);
        const int at3_em00=dependency==0x200&&r100_at3_enemy(work,p);
        const unsigned stamp=half(p+0x640);
        const int opening=work->room==0x100&&!p[0x1650]&&
            ((dependency==0x203&&stamp==19)||(dependency==0x200&&stamp==20));
        const int west_em0a=work->room==0x100&&dependency==0x20a&&stamp==0&&
            p[0x1650]==3&&!p[0x2f7];
        const int followup=at3_em00||opening||west_em0a;
        const unsigned heat=heat_category(s);
        const int choose=heat>=3&&!(number(word(p+0x17cc))>0)&&(flags&0x10u);
        if(heat>=3&&!followup&&(!wave_em02||choose))
            return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
        int chosen=0;
        if(followup&&choose){
            /* Original 235D04 compares the guard timer before the
             * authored SEQ bit gates 262AA8's follow-up selection. */
            const gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
            if(status)return status;
        }
        if(!chosen){
            if(flags&3u)put(p+0x5fc,1);
            if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        }
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original R104 EM03's non-damaging Give4B attack at 226450. */
static gh_actor_motion_status advance_r104_selector21_00226450(gh_enemy202_move *s,uint8_t *p)
{
    enum { OPEN=230, TRACK, SAMPLE, CHOOSE, ROTATE, TRANSLATE, GIVE, DECIDE, FINISH };
    gh_enemy_actor *enemy=s->enemy;
    gh_enemy_entry *entry=&enemy->entry;
    gh_title_player_view *actor=&entry->object.view;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_boot_work *work=entry->b.work;
    uint32_t pc=0;
    if(!gh_enemy203_is_r104_entry(&entry->request,work->room)||p[0x2f7]||word(p+0x6ec)||
       gh_enemy_timers_weapon_kind(&enemy->timers))
        return stop(s,0x226450,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        s->substate67_stage=p[0x2f6]>1?GIVE:p[0x2f6]?TRACK:OPEN;
    }
    if(s->substate67_stage==OPEN){
        gh_actor_motion_call *call=&s->substate67_motion;
        const gh_object_view *resource=&entry->resource;
        const unsigned rank=heat_category(s);
        static const unsigned tracking[]={0,11,12,13,13,15};
        if(!rank||rank>5)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!call->active){
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x19bc||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes||
               word(resource->bytes+0x19b4)!=0x2d9b40||word(resource->bytes+0x19b8)!=0x2dbbc0)
                return stop(s,0x2264d4,GH_ACTOR_MOTION_UNSUPPORTED);
            p[0x1864]=0;
            stamp19_attack_parameters(p,0,0x10,0x4b);
            put(p+0x5f0,15);
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,0x2d9b40,0x2dbbc0,0,10,p[0x17c3]?2:0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5f0,tracking[rank]);
        put(p+0x5fc,0);
        put(p+0x5f4,100);
        put(p+0x16dc,0);
        p[0x2f6]=1;
        s->substate67_stage=TRACK;
    }
    if(s->substate67_stage==TRACK){
        const unsigned track=word(p+0x5f0);
        if(track){
            float origin[3],target[3],delta,angle;
            const float rate=number(word(p+0x5a8));
            const float frame_rate=number(word((const uint8_t *)work+0x55c));
            const float yaw=number(word(p+0x104));
            fenv_t env;
            if(!position(actor,origin)||!position(s->player,target)||!isfinite(rate)||
               !isfinite(frame_rate)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float cap=mul(mul(rate,number(0x3e490fdb)),frame_rate);
            const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
                origin,target,yaw,cap,&delta,&pc);
            const float next=status?yaw:add(yaw,delta);
            if(fesetenv(&env)||status||gh_id_angle_reference_001f7d48(next,&angle))
                return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f0,track-1);
            put(p+0x104,bits(angle));
        }
        unsigned timer=word(p+0x5f4);
        if((half(p+0x3ac)&1u)&&timer>10)timer=10;
        if(timer){
            put(p+0x5f4,timer-1);
            put(p+0x16d0,word(p+0x16d0)|0x800000u);
        }
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?CHOOSE:ROTATE;
    }
    if(s->substate67_stage==CHOOSE){
        const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=GIVE;
    }
    if(s->substate67_stage==GIVE){
        stamp19_attack_parameters(p,0,0x10,0x4b);
        const gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
        if(status)return status;
        s->substate67_stage=(half(p+0x3ac)&0x10u)?DECIDE:FINISH;
    }
    if(s->substate67_stage==DECIDE){
        int chosen;
        const gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
        if(status)return status;
        s->substate67_stage=chosen?5:FINISH;
    }
    if(s->substate67_stage==FINISH){
        if(half(p+0x3ac)&3u)put(p+0x5fc,1);
        if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original EM03 charge and recovery at 224090. */
static gh_actor_motion_status advance_r104_projectile_windup_00224090(gh_enemy202_move *s,uint8_t *p)
{
    enum {
        OPEN=230, EFFECT, TRACK, SAMPLE, ROTATE, TRANSLATE,
        CHARGE_OPEN, CHARGE_TRACK, CHARGE_SAMPLE, CHARGE_ROTATE, CHARGE_TRANSLATE,
        CHECK_WALL, BODY, GIVE, RECOVERY_EFFECT, RECOVERY_OPEN, RECOVERY_SAMPLE, RECOVERY_CHOOSE
    };
    gh_enemy_actor *enemy=s->enemy;
    gh_enemy_entry *entry=&enemy->entry;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_title_player_view *actor=&entry->object.view;
    const gh_boot_work *work=entry->b.work;
    const gh_game_init_state *init=entry->b.init;
    uint32_t pc=0;
    if(!gh_enemy203_is_r104_entry(&entry->request,work->room)||p[0x2f7]||p[0x2f6]>5)
        return stop(s,0x224090,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        for(unsigned i=1;i<5;++i){
            const gh_hitarea_object *give=&enemy->hitareas.units[i];
            const gh_title_player_view *hit=NULL;
            if(!enemy->hitareas.created[i]||
               (!give->fallback&&gh_hitarea_object_last_hit_actor(give,&hit)))
                return stop(s,0x262a10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit&&hit->bytes&&hit->size>=0x300&&half(hit->bytes+0x2fe)==0x100){
                p[0x1864]=1;
                break;
            }
        }
        if(p[0x2f6]==5)s->substate67_stage=RECOVERY_SAMPLE;
        else if(p[0x2f6]==4)s->substate67_stage=RECOVERY_EFFECT;
        else if(p[0x2f6]==3)s->substate67_stage=CHARGE_TRACK;
        else if(p[0x2f6]==2)s->substate67_stage=CHARGE_OPEN;
        else if(p[0x2f6]==1)s->substate67_stage=TRACK;
        else{
            p[0x1864]=0;
            /* 2609A8 sets the same charge Give row in both banks. */
            for(unsigned offset=0x1870;offset<=0x1884;offset+=0x14){
                put(p+offset,10);
                put(p+offset+4,40);
                put(p+offset+8,0x42);
                put(p+offset+12,0);
                put_half(p+offset+16,0xf5);
            }
            s->substate67_stage=OPEN;
        }
    }
    if(s->substate67_stage==CHARGE_OPEN){
        gh_actor_motion_call *call=&s->substate67_motion;
        const gh_object_view *resource=&entry->resource;
        if(!call->active){
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x96c||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2a8578,GH_ACTOR_MOTION_BOUNDS);
            /* Original dep0250 always selects the 0968 sequence for the charge. */
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+0x95c),word(resource->bytes+0x968),
                0,3,p[0x17c3]?2:0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5f0,8);
        put(p+0x5f4,0);
        for(unsigned offset=0x5d0;offset<=0x5d8;offset+=4)
            put(p+offset,0x47c34ff3);
        p[0x2f6]=3;
        s->substate67_stage=CHARGE_TRACK;
    }
    if(s->substate67_stage==OPEN){
        gh_actor_motion_call *call=&s->substate67_motion;
        const gh_object_view *resource=&entry->resource;
        if(!call->active){
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x95c||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2a8578,GH_ACTOR_MOTION_BOUNDS);
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+0x950),word(resource->bytes+0x958),
                0,10,p[0x17c3]?2:0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        s->substate67_stage=EFFECT;
    }
    if(s->substate67_stage==EFFECT){
        gh_effect_create_call *call=&s->death_effect_create;
        gh_effect_create_bindings bindings=s->impact_bindings;
        bindings.model=&entry->initialize.model.model;
        if(!call->active){
            const gh_effect_create_request request={.bank=0x58,.index=0x1a,
                .actor=actor,.mode=-1,.selector=-1};
            if(gh_effect_create_actor_sequence_0014fb48(call,&request,word(p+0x114)))
                return stop(s,0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(gh_game_effect_create_00306528(call,&bindings)||!call->complete)
            return stop(s,call->frontier?call->frontier:0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x5fc,0);
        p[0x2f6]=1;
        s->substate67_stage=TRACK;
    }
    if(s->substate67_stage==CHARGE_TRACK){
        float origin[3],target[3],angle;
        if(!init||!position(actor,origin)||!position(s->player,target))
            return stop(s,0x2244d0,GH_ACTOR_MOTION_UNSUPPORTED);
        const float yaw=number(word(p+0x104));
        const float range_sq=number(word(p+0x618));
        if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,origin,target,
                yaw,number(0x40490fdb),&angle,&pc))
            return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(fabsf(angle)<number(0x3f060a92)&&range_sq>16.0f){
            const unsigned rank=heat_category(s);
            static const uint32_t caps[]={0x3c490fdb,0x3cc90fdb,0x3d490fdb,0x3dc90fdb,0x3dc90fdb};
            if(rank<1||rank>5)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float cap=mul(mul(number(caps[rank-1]),number(word(p+0x5a8))),
                number(word((const uint8_t *)work+0x55c)));
            float turn;
            const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
                origin,target,yaw,cap,&turn,&pc);
            float next=status?0:add(yaw,turn);
            if(fesetenv(&env)||status||gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x104,bits(next));
        }
        if(range_sq<16.0f&&(int32_t)word(p+0x5f0)>=3)put(p+0x5f0,2);
        const gh_hitarea_object *give=&enemy->hitareas.units[1];
        if(!enemy->hitareas.created[1]||give->fallback||!give->bytes||give->size<0x38)
            return stop(s,0x224700,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(give->bytes+0x34)||init->enemies[0x53d]||init->enemies[0x53c])p[0x2f6]=4;
        s->substate67_stage=CHARGE_SAMPLE;
    }
    if(s->substate67_stage==CHARGE_SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_ARGUMENT);
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(ended){
            const uint32_t remaining=word(p+0x5f0)-1u;
            put(p+0x5f0,remaining);
            if((int32_t)remaining<1)p[0x2f6]=4;
        }
        s->substate67_stage=CHARGE_ROTATE;
    }
    if(s->substate67_stage==TRACK){
        float origin[3],target[3],turn;
        fenv_t env;
        if(!position(actor,origin)||!position(s->player,target)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        const float yaw=number(word(p+0x104));
        const float cap=mul(mul(number(word(p+0x5a8)),0.09817477f),number(word((const uint8_t *)work+0x55c)));
        const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
            origin,target,yaw,cap,&turn,&pc);
        float next=status?0:add(yaw,turn);
        if(fesetenv(&env)||status||gh_id_angle_reference_001f7d48(next,&next))
            return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(next));
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_ARGUMENT);
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(ended)++p[0x2f6];
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==RECOVERY_EFFECT){
        gh_effect_create_call *call=&s->death_effect_create;
        gh_effect_create_bindings bindings=s->impact_bindings;
        bindings.model=&entry->initialize.model.model;
        if(!call->active){
            const gh_effect_create_request request={.bank=0x58,.index=0x1c,
                .actor=actor,.mode=-1,.selector=-1};
            if(gh_effect_create_actor_sequence_0014fb48(call,&request,word(p+0x114)))
                return stop(s,0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(gh_game_effect_create_00306528(call,&bindings)||!call->complete)
            return stop(s,call->frontier?call->frontier:0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=RECOVERY_OPEN;
    }
    if(s->substate67_stage==RECOVERY_OPEN){
        gh_actor_motion_call *call=&s->substate67_motion;
        const gh_object_view *resource=&entry->resource;
        if(!call->active){
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x974||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2a8578,GH_ACTOR_MOTION_BOUNDS);
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+0x96c),word(resource->bytes+0x970),
                0,3,p[0x17c3]?2:0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5f0,5);
        p[0x2f6]=5;
        s->substate67_stage=RECOVERY_SAMPLE;
    }
    if(s->substate67_stage==RECOVERY_SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_ARGUMENT);
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?RECOVERY_CHOOSE:ROTATE;
    }
    if(s->substate67_stage==RECOVERY_CHOOSE){
        const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE||s->substate67_stage==CHARGE_ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=s->substate67_stage==CHARGE_ROTATE?CHARGE_TRANSLATE:TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE||s->substate67_stage==CHARGE_TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=s->substate67_stage==CHARGE_TRANSLATE?CHECK_WALL:GIVE;
    }
    if(s->substate67_stage==CHECK_WALL){
        float from[3],sine,cosine;
        if(!position(actor,from)||gh_sdk_rotation_sincos_reference_0030a5d0(
            number(word(p+0x104)),&sine,&cosine,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x275888,GH_ACTOR_MOTION_UNSUPPORTED);
        const float height=add(from[1],0.5f);
        gh_scene_ray_request ray={.mask=1,.flags=0x124,.segmented=1,
          .from={from[0],height,from[2],1},
          .to={add(from[0],mul(2,sine)),height,add(from[2],mul(2,cosine)),1}};
        int wall;
        if(!enemy->b.actors||gh_scene_ray_query_00131e10(enemy->b.actors,&ray,&wall,&pc))
            return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
        if(wall)p[0x2f6]=4;
        s->substate67_stage=BODY;
    }
    if(s->substate67_stage==BODY){
        const gh_actor_motion_status status=body_give_00262750(s,p,1);
        if(status)return status;
        s->substate67_stage=GIVE;
    }
    if(s->substate67_stage==GIVE){
        const gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
        if(status)return status;
        if(half(p+0x3ac)&3u){
            put(p+0x16f8,10);put(p+0x5fc,1);
            put(p+0x16d0,word(p+0x16d0)|0x1000u);
        }
        if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* EM15 throws from phase 0 while EM14 joins through phase 2. */
static gh_actor_motion_status advance_boss_throw_00236ed0(gh_enemy202_move *s,uint8_t *p)
{
    enum { PREPARE=230, BIND, SAMPLE, ROTATE, TRANSLATE };
    gh_enemy_actor *enemy=s->enemy;
    gh_title_player_view *actor=&enemy->entry.object.view;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_boot_work *work=enemy->entry.b.work;
    uint32_t pc=0;
    if(!r103_arena_boss(&enemy->entry)||p[0x2f6]>3||p[0x2f7])
        return stop(s,0x236ed0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage)s->substate67_stage=PREPARE;
    if(s->substate67_stage==PREPARE){
        if(!enemy->b.solids||gh_collision_solid_set_enabled_001351d8(enemy->b.solids,actor,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)|0x20000u);
        put(p+0x250,word(p+0x250)|0x10000u);
        put(p+0x54c,0x40400000u);
        s->substate67_stage=(p[0x2f6]&1u)?SAMPLE:BIND;
    }
    if(s->substate67_stage==BIND){
        const size_t row=p[0x2f6]==2?0x1ba8:0x1ba0;
        const gh_object_view *resource=&enemy->entry.resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x2a8578,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            p[0x1864]=0;
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),0,0,0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        ++p[0x2f6];
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_ARGUMENT);
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(ended)enemy_decision(p,0,0x6c,0);
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_grab_substate4c_00235db8(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=230, SOLID, FOLLOW, SAMPLE, ROTATE, ROTATE_HOLD, MOVE, MOVE_HOLD, MASH, DECIDE };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->b.entry.work;
    uint32_t pc=0;
    const int r100=work->room==0x100&&word(p+0x564)==0x20a&&!half(p+0x640)&&p[0x1650]==3;
    const int r104=gh_enemy203_is_r104_entry(&enemy->entry.request,work->room)&&!p[0x2f7];
    if((!r100&&!r102_gate_em0a(work,p)&&!r103_arena_em0a(&enemy->entry)&&!r104)||
        !s->player||!s->player->bytes||s->player->size<0x55c)
        return stop(s,0x235db8,GH_ACTOR_MOTION_UNSUPPORTED);
    uint8_t *player=s->player->bytes;
    if(!s->substate67_stage){
        if(p[0x2f6]>5)return stop(s,0x235db8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=(p[0x2f6]&1u)?SOLID:BIND;
    }
    if(s->substate67_stage==BIND){
        const size_t row=(r104?0x1a64u:0xc68u)+4u*p[0x2f6];
        const gh_object_view *resource=&enemy->entry.resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x2a8578,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            p[0x1864]=0;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,p[0x2f6]?3:0,0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(!p[0x2f6]){
            const unsigned heat=heat_category(s);
            if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            const unsigned resistance=(int16_t)half(player+0x54a)<2?60:heat==5?30:heat>=3?25:heat==2?20:15;
            put_half(p+0x568,(uint16_t)resistance);
            put_half(p+0x56a,30);
            put_half(p+0x56e,15);
        }else if(p[0x2f6]==4)put_half(p+0x568,30);
        ++p[0x2f6];
        s->substate67_stage=SOLID;
    }
    if(s->substate67_stage==SOLID){
        const unsigned remaining=half(p+0x568);
        if(p[0x2f6]!=5||remaining){
            if(!enemy->b.solids||gh_collision_solid_set_enabled_001351d8(enemy->b.solids,actor,0,&pc))
                return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(p[0x2f6]==5)put_half(p+0x568,(uint16_t)(remaining-1));
            if(p[0x2f6]!=1)put(p+0x54c,0x40400000);
        }
        s->substate67_stage=p[0x2f6]==1?FOLLOW:SAMPLE;
    }
    if(s->substate67_stage==FOLLOW){
        const unsigned frames=half(p+0x56e);
        if(frames){
            float xyz[3],delta[3];
            if(!position(actor,xyz))return stop(s,0x235f00,GH_ACTOR_MOTION_UNSUPPORTED);
            memcpy(delta,player+0x550,12);
            for(unsigned i=0;i<3;i++){
                xyz[i]=add(xyz[i],delta[i]);
                if(!isfinite(xyz[i]))return stop(s,0x235f00,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            put_half(p+0x56e,(uint16_t)(frames-1));
            memcpy(actor->alias_f0,xyz,12);
        }
        put(p+0x54c,0x40400000);
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        const unsigned phase=p[0x2f6];
        if(ended&&phase==1)p[0x2f6]=2;
        if(ended&&phase==3)enemy_decision(p,0,0x6c,0);
        s->substate67_stage=phase==1?ROTATE_HOLD:(ended&&phase==5)?DECIDE:ROTATE;
    }
    if(s->substate67_stage==DECIDE){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    if(s->substate67_stage==ROTATE||s->substate67_stage==ROTATE_HOLD){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=s->substate67_stage==ROTATE_HOLD?MOVE_HOLD:MOVE;
    }
    if(s->substate67_stage==MOVE||s->substate67_stage==MOVE_HOLD){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=s->substate67_stage==MOVE_HOLD?MASH:5;
    }
    if(s->substate67_stage==MASH){
        const unsigned frames=half(p+0x56a);
        if((int16_t)frames>0){
            const gh_action_set_request request={.priority=11,.life=19,.unknown20=16};
            int original;
            unsigned slot;
            if(!s->grab_buttons)return stop(s,0x2db770,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_action_button_set_001f7858(s->grab_buttons,&request,&original,&slot,&pc))
                return stop(s,pc?pc:0x1f7858,GH_ACTOR_MOTION_UNSUPPORTED);
            put_half(p+0x56a,(uint16_t)(frames-1));
            const uint32_t pressed=word((const uint8_t *)work+8);
            const int16_t resistance=(int16_t)(half(p+0x568)-
                ((pressed&0xf0u)?1:0)-((pressed&0xf00000u)?4:0));
            put_half(p+0x568,(uint16_t)resistance);
            if(resistance<1)p[0x2f6]=player[0x2f6]=4;
        }
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original 236190 short catch and escape counters. */
static gh_actor_motion_status advance_em00_substate4d_00236190(gh_enemy202_move *s,uint8_t *p)
{
    enum { ENTER=230, BIND, INPUT, SAMPLE, ROTATE, TRANSLATE, PROMPT, FINISH, RESET, ESCAPE_SOLID, ESCAPE_PLAYER, ESCAPE_PROMPT };
    gh_enemy_actor *enemy=s->enemy;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->b.entry.work;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    const int em27=r105_exchanged_enemy(work,p);
    const int r102_em00=r102_em00_stamp11(work,p)||r102_em00_stamp3(work,p)||r102_em00_stamp7_grab(work,p);
    const int arena=r103_arena_em00(&enemy->entry);
    if((!em27&&!r102_em00&&!arena&&(work->room!=0x100||word(p+0x564)!=0x200||half(p+0x640)!=20))||
       !s->player||!s->player->bytes||s->player->size<0x55c)
        return stop(s,0x236190,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage)s->substate67_stage=ENTER;
    if(s->substate67_stage==ENTER){
        if(!enemy->b.solids||gh_collision_solid_set_enabled_001351d8(enemy->b.solids,actor,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=p[0x2f6]==0?BIND:p[0x2f6]==1?INPUT:FINISH;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x520||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2361e0,GH_ACTOR_MOTION_BOUNDS);
            p[0x1864]=0;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+0x518),word(resource->bytes+0x51c),0,0,0,!em27&&!r102_em00&&!arena);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        const unsigned rank=heat_category(s);
        put_half(p+0x568,rank==5?20:rank>=3?15:rank==2?10:5);
        put_half(p+0x56a,4);
        put_half(p+0x56e,15);
        put_half(p+0x56c,1);
        p[0x2f6]=1;
        s->substate67_stage=INPUT;
    }
    if(s->substate67_stage==INPUT){
        const unsigned remaining=half(p+0x56e);
        if(remaining){
            float next[3],delta[3];
            if(!position(actor,next))return stop(s,0x236280,GH_ACTOR_MOTION_UNSUPPORTED);
            memcpy(delta,s->player->bytes+0x550,12);
            for(unsigned i=0;i<3;i++){
                next[i]=add(next[i],delta[i]);
                if(!isfinite(next[i]))return stop(s,0x236280,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            put_half(p+0x56e,(uint16_t)(remaining-1));
            memcpy(actor->alias_f0,next,12);
        }
        const uint32_t pressed=word((const uint8_t *)work+8);
        const unsigned timer=half(p+0x568);
        put_half(p+0x568,(uint16_t)(timer-((pressed&0xf0u)?1u:0u)-((pressed&0xf00000u)?4u:0u)));
        put(p+0x54c,0x40400000);
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?RESET:ROTATE;
    }
    if(s->substate67_stage==RESET){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=FINISH;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=PROMPT;
    }
    if(s->substate67_stage==PROMPT){
        const int escape=(half(p+0x3ac)&0x10u)!=0;
        if(escape&&(int16_t)half(p+0x568)<1){
            enemy_decision(p,1,0x1d,(uint8_t)(p[0x2f7]+1));
            s->substate67_stage=ESCAPE_SOLID;
        }else if(escape||half(p+0x56c)){
            const gh_action_set_request request={.priority=11,.life=19,.unknown20=16};
            int original;
            unsigned slot;
            if(!s->grab_buttons)return stop(s,0x2db770,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_action_button_set_001f7858(s->grab_buttons,&request,&original,&slot,&pc))
                return stop(s,pc?pc:0x1f7858,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(s->substate67_stage==PROMPT){
            if(escape)put_half(p+0x56c,0);
            s->substate67_stage=FINISH;
        }
    }
    if(s->substate67_stage==ESCAPE_SOLID){
        if(gh_collision_solid_set_enabled_001351d8(enemy->b.solids,actor,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ESCAPE_PLAYER;
    }
    if(s->substate67_stage==ESCAPE_PLAYER){
        if(!s->player_grab)return stop(s,0x129718,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=s->player_grab(s->player_grab_context,enemy,0x1d,&pc);
        if(status)return stop(s,pc?pc:0x129718,status);
        s->substate67_stage=ESCAPE_PROMPT;
    }
    if(s->substate67_stage==ESCAPE_PROMPT){
        const gh_action_set_request request={.priority=11,.life=19,.unknown20=16};
        int original;
        unsigned slot;
        if(!s->grab_buttons)return stop(s,0x2db770,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_action_button_set_001f7858(s->grab_buttons,&request,&original,&slot,&pc))
            return stop(s,pc?pc:0x1f7858,GH_ACTOR_MOTION_UNSUPPORTED);
        put_half(p+0x56c,0);
        s->substate67_stage=FINISH;
    }
    if(s->substate67_stage==FINISH)s->substate67_stage=5;
    return advance_state0_tail(s,p);
}

/* Original paired catch shared by EM02, EM03 and the first arena boss. */
static gh_actor_motion_status advance_substate4e_002363e8(gh_enemy202_move *s,uint8_t *p)
{
    enum { ENTER=210, BIND, PROMPT, INPUT, SAMPLE, CHOOSE, ROTATE, TRANSLATE };
    gh_enemy_actor *enemy=s->enemy;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->b.entry.work;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    const int wave_em02=word(p+0x564)==0x202&&
        (r100_at2_enemy(work,p)||r100_at3_enemy(work,p));
    const int opening=work->room==0x100&&word(p+0x564)==0x203&&half(p+0x640)==19;
    const int arena_boss=word(p+0x564)==0x214&&r103_arena_boss(&enemy->entry);
    if((!wave_em02&&!r102_area11_em02(work,p)&&!opening&&!r103_arena_em03(&enemy->entry)&&!arena_boss)||
       !s->player||s->player->size<0x55c||s->player->bytes[0x2f4]!=1||
       s->player->bytes[0x2f5]!=0xb)
        return stop(s,0x2363e8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage)s->substate67_stage=ENTER;
    if(s->substate67_stage==ENTER){
        if(!enemy->b.solids||gh_collision_solid_set_enabled_001351d8(enemy->b.solids,actor,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned phase=p[0x2f6];
        if((!arena_boss&&!wave_em02&&(phase==4||phase==5))||phase>7)
            return stop(s,0x2363e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=phase==5||phase==7?INPUT:(phase&1u)?PROMPT:BIND;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            const unsigned phase=p[0x2f6];
            const unsigned row=phase==4&&!positive_hp(s->player->bytes)?0x7f0:
                phase>=4?0x7e0:phase==2?0x7d8:0x7e8;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x236420,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),0,phase>=4?3:0,0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
            p[0x1864]=0;
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(!p[0x2f6]){
            const unsigned rank=heat_category(s);
            if(!rank)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            put_half(p+0x568,rank==1?20:rank==2?30:rank==5?50:40);
            put_half(p+0x56a,4);
            put_half(p+0x56e,15);
        }
        p[0x2f6]++;
        s->substate67_stage=p[0x2f6]>=5?INPUT:PROMPT;
    }
    if(s->substate67_stage==PROMPT){
        const gh_action_set_request request={.priority=11,.life=19,.unknown20=16};
        int original;
        unsigned slot;
        if(!s->grab_buttons)return stop(s,0x2db770,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_action_button_set_001f7858(s->grab_buttons,&request,&original,&slot,&pc))
            return stop(s,pc?pc:0x1f7858,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=INPUT;
    }
    if(s->substate67_stage==INPUT){
        unsigned remaining=half(p+0x56e);
        if(p[0x2f6]==1&&remaining){
            float next[3],delta[3];
            if(!position(actor,next))return stop(s,0x2364b4,GH_ACTOR_MOTION_UNSUPPORTED);
            memcpy(delta,s->player->bytes+0x550,12);
            for(unsigned i=0;i<3;i++){
                next[i]=add(next[i],delta[i]);
                if(!isfinite(next[i]))return stop(s,0x2364b4,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            put_half(p+0x56e,(uint16_t)(remaining-1));
            memcpy(actor->alias_f0,next,12);
        }
        if(p[0x2f6]<5){
            const uint32_t pressed=word((const uint8_t *)work+8);
            const unsigned timer=half(p+0x568);
            put_half(p+0x568,(uint16_t)(timer-((pressed&0xf0u)?1u:0u)-((pressed&0xf00000u)?4u:0u)));
        }
        put(p+0x54c,0x40400000);
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state71_exit=(uint8_t)ended;
        s->substate67_stage=CHOOSE;
    }
    if(s->substate67_stage==CHOOSE){
        if(s->state71_exit){
            if(p[0x2f6]==1)p[0x2f6]=2;
            else if(p[0x2f6]==5)enemy_decision(p,0,0x6c,0);
            else if(p[0x2f6]==7){
                gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
                if(status)return status;
                s->substate67_stage=5;
                return advance_state0_tail(s,p);
            }else{
                const uint16_t loops=(uint16_t)(half(p+0x56a)-1u);
                if((int16_t)half(p+0x568)<1||(int16_t)loops<=0)
                    p[0x2f6]=s->player->bytes[0x2f6]=6;
                put_half(p+0x56a,loops);
                if(!positive_hp(s->player->bytes))
                    p[0x2f6]=s->player->bytes[0x2f6]=4;
            }
        }
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original launch selectors 2 and 3 choose the mirrored hand and aim
 * within 45 degrees of the actor. Heat 5 adds the original lateral speed. */
static gh_actor_motion_status launch_devil_projectile_0026bef0(gh_enemy202_move *s,
    const uint8_t *actor_bytes,unsigned side,gh_om_object *projectile)
{
    gh_actor_model *model=&s->runtime.entry->initialize.model.model;
    const gh_title_player_view *actor=&s->runtime.entry->object.view;
    const unsigned hand=(side^(actor_bytes[0x17c3]!=0))?16:10;
    const unsigned heat=heat_category(s);
    float origin[3],target[3];
    if(!heat||!position(s->player,target))return stop(s,0x26bef0,GH_ACTOR_MOTION_UNSUPPORTED);
    const gh_actor_part *part=NULL;
    if(hand<model->part_count&&gh_actor_model_part_at(model,hand,&part))
        return stop(s,0x26c060,GH_ACTOR_MOTION_BOUNDS);
    if(part&&part->alias_f0)memcpy(origin,part->alias_f0,12);
    else{
        float matrix[16],world[4];
        const float local[3]={0,1.3f,1.2f};
        memcpy(matrix,actor_bytes+0x80,sizeof(matrix));
        if(gh_scene_at_apply_world_001508d8(matrix,local,world))
            return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(origin,world,12);
    }
    for(unsigned i=0;i<3;i++)if(!isfinite(origin[i]))return stop(s,0x26bef0,GH_ACTOR_MOTION_UNSUPPORTED);
    const float base=number(word(actor_bytes+0x104));
    const float pi=number(0x40490fdb),limit=number(0x3f490fdb);
    uint32_t pc=0;
    float aim;
    if(gh_game_angle_001f8180(sub(target[0],origin[0]),sub(target[2],origin[2]),
        &aim,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
        return stop(s,pc?pc:0x1f8180,GH_ACTOR_MOTION_UNSUPPORTED);
    float delta=sub(aim,base);
    if(delta>pi)delta=sub(delta,2*pi);
    if(delta<-pi)delta=add(delta,2*pi);
    if(delta>limit)delta=limit;
    if(delta<-limit)delta=-limit;
    float yaw;
    if(gh_id_angle_reference_001f7d48(add(base,delta),&yaw))
        return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
    float sine,cosine;
    if(gh_sdk_rotation_sincos_reference_0030a5d0(yaw,&sine,&cosine,GH_STATIC_BINARY32_REFERENCE,&pc))
        return stop(s,pc?pc:0x147988,GH_ACTOR_MOTION_UNSUPPORTED);
    const float lateral=heat==5?.8f:0;
    const float velocity[3]={add(mul(cosine,lateral),mul(sine,.5f)),
        0,sub(mul(cosine,.5f),mul(sine,lateral))};
    gh_om3be_state *state=(gh_om3be_state *)projectile->initialization;
    const gh_scene_placement_status status=gh_om3be_launch_001bb548(state,actor,origin,velocity,heat,3,&s->impact_bindings);
    return status?stop(s,state->common.frontier?state->common.frontier:0x1bb548,GH_ACTOR_MOTION_UNSUPPORTED):GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status launch_em03_projectile_0026bef0(gh_enemy202_move *s,uint8_t *p)
{
    gh_om_factory *factory=s->projectile_factory;
    if(!factory||p[0x2f7])return stop(s,0x26bef0,GH_ACTOR_MOTION_UNSUPPORTED);
    const unsigned heat=heat_category(s);
    if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
    put(p+0x5f8,0);
    gh_om_object *object=NULL;
    gh_scene_placement_status placement=gh_om_factory_create_0030fa98(factory,
        &s->projectile_create[0],0x3be,0xffff,&object);
    if(placement)return stop(s,factory->frontier?factory->frontier:0x30fa98,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!object)return GH_ACTOR_MOTION_OK;

    float matrix[16],origin[4],sine,cosine;
    const float local[3]={0,1.3f,.3f};
    memcpy(matrix,p+0x80,sizeof(matrix));
    if(gh_scene_at_apply_world_001508d8(matrix,local,origin))
        return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t pc=0;
    if(gh_sdk_rotation_sincos_reference_0030a5d0(number(word(p+0x104)),&sine,&cosine,
        GH_STATIC_BINARY32_REFERENCE,&pc))return stop(s,pc?pc:0x147988,GH_ACTOR_MOTION_UNSUPPORTED);
    const float speed=heat==5?1.2f:.8f;
    const float velocity[3]={mul(sine,speed),0,mul(cosine,speed)};
    placement=gh_om_factory_initialize_runtime_3be(factory,object);
    if(placement)return stop(s,factory->frontier?factory->frontier:0x1b90d0,GH_ACTOR_MOTION_UNSUPPORTED);
    const gh_title_player_view *shooter=&s->runtime.entry->object.view;
    gh_om3be_state *state=(gh_om3be_state *)object->initialization;
    placement=gh_om3be_launch_001bb548(state,shooter,origin,velocity,heat,0,&s->impact_bindings);
    return placement?stop(s,state->common.frontier?state->common.frontier:0x1bb548,
        GH_ACTOR_MOTION_UNSUPPORTED):GH_ACTOR_MOTION_OK;
}

/* 2705D8 rank-independent far decision for EM20 in R100.
 * The three room-specific selectors preceding the range check return zero. */
gh_actor_motion_status gh_enemy_devil_choose_far_002705d8(const gh_title_player_view *actor,
    const gh_boot_work *work,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0x2705d8;
    if(!actor||!actor->bytes||actor->size<=0x1864||!work||
       actor->original_vtable214!=0x4479c8||word(actor->bytes+0x564)!=0x220)
        return GH_ACTOR_MOTION_ARGUMENT;
    uint8_t *p=actor->bytes;
    if(word(p+0x738)){*frontier=0x1d0408;return GH_ACTOR_MOTION_UNSUPPORTED;}
    const float escape=number(word(p+0x16c0)),override=number(word(p+0x1734));
    const float distance=number(word(p+0x618));
    if(work->room!=0x100||p[0x1864]||!isfinite(escape)||!isfinite(override)||
       !isfinite(distance)||escape>0.0f||override>0.0f||distance<9.0f)
        return GH_ACTOR_MOTION_UNSUPPORTED;
    memset(p+0x2f4,0,4);
    *frontier=0;
    return GH_ACTOR_MOTION_OK;
}

/* 217D90 selects a six-unit forward leap, with original player-side
 * jitter and the optional authored/player-relative destination. */
static gh_actor_motion_status devil_forward_leap_00217d90(gh_enemy202_move *s,
    const uint8_t *p,const float origin[3],const float target[3],float sine,float cosine,
    gh_game_mt *random,float destination[3],uint16_t *tracking)
{
    uint32_t pc=0,draw;
    const float distance=number(word(p+0x618));
    if(!s->player->bytes||s->player->size<0x108||!isfinite(distance)||distance<0)
        return stop(s,0x217d90,GH_ACTOR_MOTION_UNSUPPORTED);
    float angle;
    if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,target,origin,
        number(word(s->player->bytes+0x104)),number(0x40490fdb),&angle,&pc))
        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    if(gh_game_mt_next_0030f348(random,&draw))
        return stop(s,0x31d000,GH_ACTOR_MOTION_UNSUPPORTED);
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))
        return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
    const volatile float number31=(float)(draw>>1);
    const float fraction=mul(number31,0x1p-31f);
    const float side=distance<100.0f?add(mul(fraction,0.5f),1.0f):add(fraction,1.5f);
    const float x=angle<0?-side:side;
    destination[0]=add(origin[0],add(mul(cosine,x),mul(sine,6.0f)));
    destination[1]=origin[1];
    destination[2]=add(origin[2],add(mul(-sine,x),mul(cosine,6.0f)));
    if(fesetenv(&env))return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
    *tracking=0;
    if(distance>=49.0f){
        if(gh_game_mt_next_0030f348(random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!(draw&1u))return GH_ACTOR_MOTION_OK;
    }
    memcpy(destination,p+0x16a0,12);
    if(gh_game_mt_next_0030f348(random,&draw))
        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!(draw&3u)){
        const float local[3]={0,0,-2};
        float matrix[16],world[4];
        memcpy(matrix,s->player->bytes+0x80,sizeof(matrix));
        if(gh_scene_at_apply_world_001508d8(matrix,local,world))
            return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(destination,world,12);
        *tracking=1;
    }
    return GH_ACTOR_MOTION_OK;
}

/* EM20 selector 12 plans the original approach or the retreat through
 * three clearance rays, then binds and samples the take-off sequence. */
static gh_actor_motion_status advance_devil_retreat_00217980(gh_enemy202_move *s,uint8_t *p)
{
    enum { PLAN=210, BIND, ANNOUNCE, LEAP_SOUND, LEAP_INIT, LEAP_MOVE,
        LAND_BIND, HOLD_BIND, HOLD_TIMER, TURN, SAMPLE, LEAP_TIMER, LAND_END,
        ROTATE, TRANSLATE, HOLD_END, CHOOSE_ATTACK, CHOOSE_RETREAT, CHOOSE_IDLE,
        LAND_GRAB_ROTATE, LAND_GRAB_TRANSLATE };
    const gh_enemy_move_actor *runtime=&s->runtime;
    const gh_enemy_entry *entry=runtime->entry;
    const gh_boot_work *work=entry->b.work;
    const gh_title_player_view *actor=&entry->object.view;
    gh_actor_motion *motion=runtime->motion;
    uint32_t pc=0;
    if(work->room!=0x100||word(p+0x564)!=0x220||half(p+0x640)!=0xff||
       !runtime->bindings->canonical_rate)
        return stop(s,0x217980,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        if(p[0x2f6]>7)return stop(s,0x217980,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!p[0x2f6]&&p[0x2f7]>1)
            return stop(s,0x217aa4,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)|0x08000400u);
        if(p[0x2f6]==6)s->substate67_stage=HOLD_BIND;
        else if(p[0x2f6]==4){
            unsigned variant=p[0x2f7];
            const unsigned previous_mirror=p[0x17c3]?2u:0u;
            if(variant<2){
                uint32_t random;
                if(gh_game_mt_next_0030f348(&entry->b.init->random,&random))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                p[0x17c3]=(uint8_t)((random&1u)==0);
            }else if(variant>3)return stop(s,0x218548,GH_ACTOR_MOTION_UNSUPPORTED);
            s->reaction_resource_offset=variant==0?0x1bf0:variant==1?0x1c00:
                ((variant==2)==(p[0x17c3]==0)?0x1c10:0x1c20);
            s->substate67_phase=(uint8_t)(previous_mirror|(p[0x17c3]?2u:0u));
            s->substate67_stage=LAND_BIND;
        }else if(p[0x2f6]==2){
            put(p+0x24c,0);
            put(p+0x250,word(p+0x250)|0x10u);
            s->substate67_stage=LEAP_SOUND;
        }else s->substate67_stage=p[0x2f6]==3?LEAP_MOVE:p[0x2f6]?TURN:PLAN;
    }
    if(s->substate67_stage==PLAN){
        float origin[3],player[3],target[3],yaw,turn,sine,cosine;
        if(!position(actor,origin)||!position(s->player,player))
            return stop(s,0x2db6b0,GH_ACTOR_MOTION_BOUNDS);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2179e4,GH_ACTOR_MOTION_CALLEE);
        const float height=ordered_abs(sub(player[1],origin[1]));
        if(fesetenv(&env)||!isfinite(height))return stop(s,0x2180b0,GH_ACTOR_MOTION_UNSUPPORTED);
        const int guided=height<1.0f&&(word(p+0x16d0)&0x200000u);
        const int approaching=p[0x2f7]==0;
        gh_game_mt random=entry->b.init->random;
        float destination[3];
        uint16_t tracking=0;
        if(!guided){
            /* 2179C4 uses the original route's current target without searching rays. */
            memcpy(target,p+0x16a0,sizeof(target));
            memcpy(destination,target,sizeof(destination));
        }else{
            memcpy(target,p+0x1690,sizeof(target));
            yaw=number(word(p+0x104));
            if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,origin,target,yaw,
                number(0x40490fdb),&turn,&pc))
                return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x217a88,GH_ACTOR_MOTION_CALLEE);
            yaw=add(yaw,turn);
            if(fesetenv(&env)||gh_id_angle_reference_001f7d48(yaw,&yaw))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_sdk_rotation_sincos_reference_0030a5d0(yaw,&sine,&cosine,
                GH_STATIC_BINARY32_REFERENCE,&pc))
                return stop(s,pc?pc:0x147988,GH_ACTOR_MOTION_UNSUPPORTED);

            if(approaching){
                gh_actor_motion_status status=devil_forward_leap_00217d90(s,p,origin,target,
                    sine,cosine,&random,destination,&tracking);
                if(status)return status;
            }else{
                static const float rays[3][2]={{0,-4},{-4,-2},{4,-2}};
                unsigned index=0;
                for(;index<3;++index){
                    const float x=rays[index][0],z=rays[index][1];
                    if(!set_round(&env,FE_TOWARDZERO))
                        return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
                    const float y=add(origin[1],0.5f);
                    const gh_scene_ray_request ray={.mask=7,.segmented=1,
                        .from={origin[0],y,origin[2],1},
                        .to={add(origin[0],add(mul(cosine,x),mul(sine,z))),y,
                            add(origin[2],add(mul(-sine,x),mul(cosine,z))),1}};
                    if(fesetenv(&env))return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
                    int hit;
                    if(gh_scene_ray_query_00131e10(runtime->bindings->actors,&ray,&hit,&pc))
                        return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
                    if(!hit)break;
                }

                float x=0,z=index==3?7.0f:-7.0f;
                if(index==0){
                    uint32_t draw;
                    if(gh_game_mt_next_0030f348(&random,&draw))
                        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                    x=number(gh_frame_jitter_bits_0031d020_word(draw))*2.0f;
                }else if(index<3)x=index==1?-5.0f:5.0f;
                if(!set_round(&env,FE_TOWARDZERO))
                    return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
                const float delta[3]={add(mul(cosine,x),mul(sine,z)),0,
                    add(mul(-sine,x),mul(cosine,z))};
                destination[0]=add(origin[0],delta[0]);
                destination[1]=origin[1];
                destination[2]=add(origin[2],delta[2]);
                if(fesetenv(&env))return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
            }
        }
        if(!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
        const float difference[3]={sub(destination[0],origin[0]),sub(destination[1],origin[1]),
            sub(destination[2],origin[2])};
        const float player_x=sub(player[0],origin[0]),player_z=sub(player[2],origin[2]);
        if(fesetenv(&env))return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
        float heading,heading_player;
        if(gh_game_angle_001f8180(difference[0],difference[2],&heading,
            GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc)||
           gh_game_angle_001f8180(player_x,player_z,&heading_player,
            GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x1f8180,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2da1f8,GH_ACTOR_MOTION_CALLEE);
        turn=sub(heading,heading_player);
        if(turn>number(0x40490fdb))turn=sub(turn,number(0x40c90fdb));
        if(turn<-number(0x40490fdb))turn=add(turn,number(0x40c90fdb));
        if(fesetenv(&env))return stop(s,0x2da1f8,GH_ACTOR_MOTION_CALLEE);
        const float angle=ordered_abs(turn);
        p[0x2f7]=angle<0.7853982f?0:angle>2.3561945f?1:turn<0?2:3;
        memcpy(p+0x5d0,destination,12);
        memcpy(p+0x5e0,target,12);
        memcpy(p+0x5c0,difference,12);
        const int retreat=guided&&!approaching;
        if(retreat)put(p+0x54c,0x41200000);
        put_half(p+0x568,tracking);
        s->substate67_phase=retreat?2:0;
        put(p+0x250,word(p+0x250)&~0x10u);
        put(p+0x24c,0x3f800000);
        entry->b.init->random=random;
        s->substate67_stage=BIND;
    }
    if(s->substate67_stage==BIND||s->substate67_stage==LAND_BIND||s->substate67_stage==HOLD_BIND){
        const int landing=s->substate67_stage==LAND_BIND;
        const int holding=s->substate67_stage==HOLD_BIND;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&entry->resource;
            const unsigned variant=p[0x2f7];
            const size_t row=holding?0x1bc0:landing?s->reaction_resource_offset:variant==0?0x1be8:variant==1?0x1bf8:
                ((variant==2)==(p[0x17c3]==0)?0x1c08:0x1c18);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2181b4,GH_ACTOR_MOTION_BOUNDS);
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                !landing&&!holding&&s->substate67_phase?0x40000000u:0u,holding?10u:2u,
                landing?s->substate67_phase:p[0x17c3]?2:0,!landing&&!holding);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(landing){
            p[0x2f6]=5;
            s->substate67_stage=TURN;
        }else s->substate67_stage=holding?HOLD_TIMER:ANNOUNCE;
    }
    if(s->substate67_stage==HOLD_TIMER){
        const unsigned heat=heat_category(s);
        if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        static const uint8_t idle_wait[]={0,45,35,10,10,5};
        static const uint8_t close_wait[]={0,15,10,5,5,0};
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&entry->b.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        const float distance=number(word(p+0x618));
        put_half(p+0x568,distance<4.0f?close_wait[heat]:(draw&1u)?30u:idle_wait[heat]);
        put_half(p+0x56a,0);
        p[0x2f6]=7;
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==ANNOUNCE||s->substate67_stage==LEAP_SOUND){
        const int takeoff=s->substate67_stage==LEAP_SOUND;
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request cue={.model=motion->b.model,.actor=actor,.bank=1,
            .program=(int16_t)(half(p+0x424)+(takeoff?0x21u:10u))};
        uint32_t handle;
        const gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&cue,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        if(takeoff)s->substate67_stage=LEAP_INIT;
        else{
            p[0x2f6]=1;
            s->substate67_stage=TURN;
        }
    }
    if(s->substate67_stage==LEAP_INIT){
        float origin[3];
        if(!position(actor,origin))return stop(s,0x2db6b0,GH_ACTOR_MOTION_BOUNDS);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x1f7e28,GH_ACTOR_MOTION_CALLEE);
        const float x=sub(number(word(p+0x5d0)),origin[0]);
        const float z=sub(number(word(p+0x5d8)),origin[2]);
        const float length=sqrtf(add(mul(x,x),mul(z,z)));
        const float distance=half(p+0x568)?fmaxf(sub(length,1.0f),0.0f):fminf(length,10.0f);
        volatile float reciprocal=length>0?1.0f/length:0.0f;
        const float one_third=number(0x3eaaaaabu);
        const float delta_x=mul(mul(mul(x,reciprocal),distance),one_third);
        const float delta_z=mul(mul(mul(z,reciprocal),distance),one_third);
        const float delta_w=mul(mul(length>0?number(word(p+0x5cc)):1.0f,distance),one_third);
        if(fesetenv(&env)||!isfinite(length)||!isfinite(delta_x)||!isfinite(delta_z)||!isfinite(delta_w))
            return stop(s,0x1f7e28,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x5c0,bits(delta_x));
        put(p+0x5c4,0);
        put(p+0x5c8,bits(delta_z));
        put(p+0x5cc,bits(delta_w));
        put(p+0x600,0x40400000u);
        p[0x2f6]=3;
        s->substate67_stage=LEAP_MOVE;
    }
    if(s->substate67_stage==LEAP_MOVE){
        float origin[3],next[4];
        const float rate=number(word(p+0x5a8));
        if(!position(actor,origin)||!isfinite(rate))
            return stop(s,0x2184a8,GH_ACTOR_MOTION_BOUNDS);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2184a8,GH_ACTOR_MOTION_CALLEE);
        for(unsigned i=0;i<4;++i)
            next[i]=add(number(word(actor->alias_f0+4*i)),mul(number(word(p+0x5c0+4*i)),rate));
        if(fesetenv(&env))return stop(s,0x2184a8,GH_ACTOR_MOTION_CALLEE);
        for(unsigned i=0;i<4;++i)
            if(!isfinite(next[i]))return stop(s,0x2184a8,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(actor->alias_f0,next,sizeof(next));
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        float origin[3],player[3],turn;
        const float yaw=number(word(p+0x104));
        const float rate=number(word(p+0x5a8)),work_rate=number(word((const uint8_t *)work+0x55c));
        fenv_t env;
        if(!position(actor,origin)||!position(s->player,player)||!isfinite(yaw)||
           !isfinite(rate)||!isfinite(work_rate)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
        const float cap=p[0x2f6]==3?number(0x40490fdb):mul(rate,p[0x2f6]==7?0.19634955f:0.3926991f);
        if((p[0x2f6]==5||p[0x2f6]==7)&&(!(word(p+0x16d0)&0x200000u)||
           ordered_abs(sub(player[1],origin[1]))>=1.0f))
            memcpy(player,p+0x16a0,12);
        const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
            origin,player,yaw,mul(cap,work_rate),&turn,&pc);
        const float next=status?yaw:add(yaw,turn);
        const float alpha=p[0x2f6]==5?add(number(word(p+0x24c)),mul(rate,0.2f)):0.0f;
        if(fesetenv(&env)||status||!isfinite(alpha))
            return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        float adjusted;
        if(gh_id_angle_reference_001f7d48(next,&adjusted))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(adjusted));
        if(p[0x2f6]==5){
            put(p+0x24c,bits(fminf(alpha,1.0f)));
            const uint32_t flags=word(p+0x250);
            put(p+0x250,alpha>1.0f?flags&~0x10u:flags|0x10u);
        }
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *runtime->bindings->canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(p[0x2f6]==3)s->substate67_stage=LEAP_TIMER;
        else if(p[0x2f6]==5&&ended)s->substate67_stage=LAND_END;
        else{
            if(ended&&p[0x2f6]==1)p[0x2f6]=2;
            s->substate67_stage=ROTATE;
        }
    }
    if(s->substate67_stage==LAND_END){
        int selected;
        const gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p,&selected);
        if(status)return status;
        if(selected)s->substate67_stage=5;
        else{
            /* Original 24BDA0 excludes R100. Near landing can switch to the grab. */
            float origin[3],player[3];
            if(!position(actor,origin)||!position(s->player,player))
                return stop(s,0x2db6b0,GH_ACTOR_MOTION_BOUNDS);
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2187e0,GH_ACTOR_MOTION_CALLEE);
            const float height=ordered_abs(sub(player[1],origin[1]));
            if(fesetenv(&env)||!isfinite(height))
                return stop(s,0x2187e0,GH_ACTOR_MOTION_UNSUPPORTED);
            float angle=0;
            /* 270D00's EM20 branch tests the same height using the live positions.
             * The following angle is measured from the player's facing. */
            if(number(word(p+0x618))<6.25f&&height<0.1f&&
               gh_player_target_angle_002da038(s->elf,s->elf_bytes,player,origin,
                    number(word(s->player->bytes+0x104)),number(0x40490fdb),&angle,&pc))
                return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(ordered_abs(angle)>2.3561945f){
                enemy_decision(p,0,0x4b,0);
                s->substate67_stage=LAND_GRAB_ROTATE;
            }else{
                p[0x2f6]=6;
                s->substate67_stage=ROTATE;
            }
        }
    }
    if(s->substate67_stage==LAND_GRAB_ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=LAND_GRAB_TRANSLATE;
    }
    if(s->substate67_stage==LAND_GRAB_TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=5;
    }
    if(s->substate67_stage==LEAP_TIMER){
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x21851c,GH_ACTOR_MOTION_CALLEE);
        const float remaining=sub(number(word(p+0x600)),number(word(p+0x5a8)));
        if(fesetenv(&env)||!isfinite(remaining))
            return stop(s,0x21851c,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x600,bits(remaining));
        if(remaining<=0)p[0x2f6]=4;
        s->substate67_stage=5;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_scaled_0014f928(actor,work,p[0x2f6]==7?1.0f:0.75f,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=p[0x2f6]==7?HOLD_END:5;
    }
    if(s->substate67_stage==HOLD_END){
        int selected;
        const gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p,&selected);
        if(status)return status;
        if(!selected){
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x218a7c,GH_ACTOR_MOTION_CALLEE);
            const float alpha=add(number(word(p+0x24c)),mul(number(word(p+0x5a8)),0.2f));
            if(fesetenv(&env)||!isfinite(alpha))
                return stop(s,0x218a7c,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x24c,bits(fminf(alpha,1.0f)));
            const uint32_t flags=word(p+0x250);
            put(p+0x250,alpha>1.0f?flags&~0x10u:flags|0x10u);
            const uint16_t remaining=(uint16_t)(half(p+0x568)-1);
            put_half(p+0x568,remaining);
            s->substate67_stage=(int16_t)remaining<=0?CHOOSE_ATTACK:5;
        }else s->substate67_stage=5;
    }
    if(s->substate67_stage==CHOOSE_ATTACK){
        /* The scheduled 272588 weapon probe returns zero for dependency 0220. */
        int chosen;
        const gh_actor_motion_status status=devil_near_choice_00262aa8(s,p,0,&chosen);
        if(status)return status;
        s->substate67_stage=chosen?5:CHOOSE_RETREAT;
    }
    if(s->substate67_stage==CHOOSE_RETREAT){
        const float distance=number(word(p+0x618));
        const float rate=number(word(p+0x5a8));
        if(distance<25.0f&&half(p+0x56a)){
            put_half(p+0x56a,half(p+0x56a)-1);
            s->substate67_stage=5;
        }else{
            gh_game_mt random=entry->b.init->random;
            uint32_t draw;
            unsigned selector=0;
            if(!isfinite(rate)||!isfinite(distance))
                return stop(s,0x218b18,GH_ACTOR_MOTION_UNSUPPORTED);
            if(p[0x2f7]==1){
                if(distance<100.0f){
                    if(gh_game_mt_next_0030f348(&random,&draw))
                        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                    if((draw&1u)&&!entry->b.init->heat[0x10]&&rate>=1.0f)selector=0x12;
                }
                if(!selector&&distance>49.0f){
                    if(gh_game_mt_next_0030f348(&random,&draw))
                        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                    if(draw&1u)selector=0x6b;
                }
            }
            entry->b.init->random=random;
            if(selector){
                enemy_decision(p,0,selector,selector==0x12);
                s->substate67_stage=5;
            }else s->substate67_stage=CHOOSE_IDLE;
        }
    }
    if(s->substate67_stage==CHOOSE_IDLE){
        const gh_actor_motion_status status=choose_devil_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Selector 23 shares its turning and motion update between EM20 and EM03. */
static gh_actor_motion_status advance_ranged_002269f0(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=200, TURN, SAMPLE, CHOOSE, ROTATE, TRANSLATE, MARKERS };
    const gh_enemy_move_actor *runtime=&s->runtime;
    const gh_enemy_entry *entry=runtime->entry;
    const gh_boot_work *work=entry->b.work;
    const gh_title_player_view *actor=&entry->object.view;
    gh_actor_motion *motion=runtime->motion;
    uint32_t pc=0;

    const int devil=!s->enemy&&work->room==0x100&&word(p+0x564)==0x220&&p[0x2f7]==2;
    const int em03=s->enemy&&p[0x2f7]<=1&&gh_enemy203_is_r104_entry(&entry->request,work->room);
    if((!devil&&!em03)||!runtime->bindings->canonical_rate)
        return stop(s,0x2269f0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        s->substate67_stage=p[0x2f6]==0?BIND:p[0x2f6]==1?TURN:MARKERS;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        const gh_object_view *resource=&entry->resource;
        const unsigned heat=heat_category(s);
        if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!call->active){
            const int maximum=(int16_t)half(p+0x548);
            const int current=(int16_t)half(p+0x54a);
            const size_t row=em03?(p[0x2f7]?0x19a4:0x19c8):
                (current>maximum/2?0x1cd0:0x1cdc);
            const size_t sequence=row+(devil&&heat==5?8u:4u);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<sequence+4||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x226b10,GH_ACTOR_MOTION_BOUNDS);

            p[0x1864]=0;
            put(p+0x5f4,em03?60u:0u);
            put(p+0x5f0,em03?50u:25u);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+sequence),
                0,10,p[0x17c3]?2:0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        static const uint32_t devil_turns[]={0,18,20,22,22,25};
        if(em03){
            /* The original rank table scales the 50-tick turning period. */
            static const uint32_t multiplier[]={0,0x3f400000,0x3f4ccccd,0x3f666666,
                                               0x3f666666,0x3f800000};
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x226f84,GH_ACTOR_MOTION_UNSUPPORTED);
            const float turns=mul(50.0f,number(multiplier[heat]));
            if(fesetenv(&env))return stop(s,0x226f84,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f0,(uint32_t)turns);
        }else put(p+0x5f0,devil_turns[heat]);
        put(p+0x5f8,1);
        put(p+0x600,word(p+0x104));
        p[0x2f6]=1;
        put(p+0x5fc,0);
        put(p+0x604,0);
        put_half(p+0x568,0);
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        const uint32_t remaining=word(p+0x5f0);
        if(remaining){
            float origin[3],target[3],turn;
            const float rate=number(word(p+0x5a8)),base=number(word(p+0x600));
            const float yaw=number(word(p+0x104));
            fenv_t env;
            if(!position(actor,origin)||!position(s->player,target)||
               !isfinite(rate)||!isfinite(base)||!isfinite(yaw)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x227014,GH_ACTOR_MOTION_UNSUPPORTED);
            const gh_player_target_status status=gh_player_target_angle_002da038(
                s->elf,s->elf_bytes,origin,target,base,mul(rate,number(0x3e490fdb)),&turn,&pc);
            float next_base=status?base:add(base,turn);
            float next_yaw=status?yaw:add(yaw,turn);
            if(fesetenv(&env)||status)
                return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(next_base,&next_base)||
               gh_id_angle_reference_001f7d48(next_yaw,&next_yaw))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f0,remaining-1);
            put(p+0x600,bits(next_base));
            put(p+0x104,bits(next_yaw));
        }
        uint32_t counter=word(p+0x5f4);
        if((half(p+0x3ac)&1u)&&(int32_t)counter>=11)counter=10;
        if(counter){
            put(p+0x5f4,counter-1);
            put(p+0x16d0,word(p+0x16d0)|0x800000u);
        }
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *runtime->bindings->canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?CHOOSE:ROTATE;
    }
    if(s->substate67_stage==CHOOSE){
        const gh_actor_motion_status status=em03?choose_after_substate_002705d8(s,p):
            choose_devil_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=em03?ROTATE:5;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=MARKERS;
    }
    if(s->substate67_stage==MARKERS){
        const unsigned marks=half(p+0x3ac)&3u;
        if(em03){
            /* 227130/227158 consume the shared arming bit before 26BEF0. */
            if(((marks&1u)&&word(p+0x5f8))||s->projectile_create[0].bound){
                gh_actor_motion_status launch=launch_em03_projectile_0026bef0(s,p);
                if(launch)return launch;
            }
        }else if(marks&&word(p+0x5f8)){
            gh_om_factory *factory=s->projectile_factory;
            if(!factory)return stop(s,0x26bef0,GH_ACTOR_MOTION_UNSUPPORTED);
            for(unsigned side=0;side<2;side++){
                if(!(marks&(1u<<side)))continue;
                gh_om_object *projectile=NULL;
                gh_scene_placement_status status=gh_om_factory_create_0030fa98(factory,
                    &s->projectile_create[side],0x3be,0xffff,&projectile);
                if(status)return stop(s,factory->frontier?factory->frontier:0x30fa98,GH_ACTOR_MOTION_UNSUPPORTED);
                if(projectile){
                    status=gh_om_factory_initialize_runtime_3be(factory,projectile);
                    if(status)return stop(s,factory->frontier?factory->frontier:0x1b90d0,GH_ACTOR_MOTION_UNSUPPORTED);
                    gh_actor_motion_status launch=launch_devil_projectile_0026bef0(s,p,side,projectile);
                    if(launch)return launch;
                }
            }
            put(p+0x5f8,0);
        }else put(p+0x5f8,1);
        if(marks)put(p+0x5fc,1);
        if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* 2426A0 preserves the script-controlled 7D phase, including the original
 * standby clip and the bit29-triggered alternate clip. */
static gh_actor_motion_status advance_r103_boss_idle_002426a0(gh_enemy202_move *s,uint8_t *p)
{
    enum {BIND_CLIP=40,STEP_CLIP,ROOT_ROTATION,ROOT_TRANSLATION,FINISH};
    gh_enemy_actor *enemy=s->enemy;
    gh_enemy_entry *entry=&enemy->entry;
    if(!gh_enemy204_is_r103_entry(&entry->request,entry->b.work->room)||entry->request.entry_number>=2)
        return stop(s,0x2426a0,GH_ACTOR_MOTION_UNSUPPORTED);
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    if(!s->substate67_stage){
        if(gh_collision_solid_set_enabled_001351d8(enemy->b.solids,&entry->object.view,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)|0x31000u);
        s->substate67_phase=p[0x2f6];
        switch(s->substate67_phase){
        case 0: case 2: s->substate67_stage=BIND_CLIP;break;
        case 1: case 3: s->substate67_stage=STEP_CLIP;break;
        default: s->substate67_stage=5;break;
        }
    }
    if(s->substate67_stage==BIND_CLIP){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const int alternate=s->substate67_phase==2;
            const size_t directory=alternate?0x1b58:0x3f4;
            const gh_object_view *resource=&entry->resource;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent>resource->size||
               resource->file_extent<directory+8||motion->b.resource!=resource->bytes)
                return stop(s,alternate?0x242814:0x242740,GH_ACTOR_MOTION_BOUNDS);
            const uint32_t start=!alternate&&entry->request.dependency_kind==0x215?0x41c80000:0;
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+directory),word(resource->bytes+directory+4),
                start,alternate?3:10,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(!s->substate67_phase)put(p+0x16d0,word(p+0x16d0)&~0x20000000u);
        ++p[0x2f6];
        s->substate67_stage=STEP_CLIP;
    }
    if(s->substate67_stage==STEP_CLIP){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(s->substate67_phase>=2&&ended)p[0x2f6]=0;
        s->substate67_stage=ROOT_ROTATION;
    }
    if(s->substate67_stage==ROOT_ROTATION){
        if(gh_player_root_rotation_0014f9f0(&entry->object.view,entry->b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ROOT_TRANSLATION;
    }
    if(s->substate67_stage==ROOT_TRANSLATION){
        if(gh_player_root_translation_0014f928(&entry->object.view,entry->b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=FINISH;
    }
    if(s->substate67_stage==FINISH){
        if(s->substate67_phase<2){
            if(number(word(p+0x16c0))>0){
                const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
                if(status)return status;
            }
            if(word(p+0x16d0)&0x20000000u)p[0x2f6]=2;
        }
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* 22D618 and 22D784 apply the flank's saved steering and displacement. */
static gh_actor_motion_status pincer_flank_step(gh_enemy202_move *s,uint8_t *p,
    gh_title_player_view *actor)
{
    float origin[3],next[3];
    const float turn=number(word(p+0x600)),rate=number(word(p+0x5a8));
    const float yaw=number(word(p+0x104));
    fenv_t env;
    if(!position(actor,origin)||!isfinite(turn)||!isfinite(rate)||!isfinite(yaw)||
       !set_round(&env,FE_TOWARDZERO))
        return stop(s,0x22d628,GH_ACTOR_MOTION_UNSUPPORTED);
    const float step=mul(mul(turn,number(0x3e99999au)),rate);
    const float remaining=sub(turn,step),next_yaw=add(yaw,step);
    for(unsigned i=0;i<3;++i)next[i]=add(origin[i],mul(number(word(p+0x590+4*i)),rate));
    if(fesetenv(&env)||!isfinite(remaining)||!isfinite(next_yaw)||
       !isfinite(next[0])||!isfinite(next[1])||!isfinite(next[2]))
        return stop(s,0x22d654,GH_ACTOR_MOTION_CALLEE);
    put(p+0x600,bits(remaining));put(p+0x104,bits(next_yaw));
    memcpy(actor->alias_f0,next,sizeof(next));
    return GH_ACTOR_MOTION_OK;
}

/* 22C3F8 and 22CBC8 share the approach and wait phases of the paired attack. */
static gh_actor_motion_status advance_boss_pincer(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=190, CUE, TURN, SAMPLE, ROTATE, TRANSLATE, TIMERS, CANCEL, CHOOSE, COLLISION,
           ATTACK_BIND, ATTACK_OFFSET_START, ATTACK_OFFSET_STEP, ATTACK_SAMPLE,
           ATTACK_CHOOSE, ATTACK_ROTATE, ATTACK_TRANSLATE, ATTACK_SIGNAL,
           IMPACT_ALIGN, IMPACT_GIVE, IMPACT_PEER, CHARGE_RATE, CHARGE_UPDATE, CHARGE_EFFECT,
           RECOVERY_HEADING, RECOVERY_TARGET, RECOVERY_STEP, FLANK_ADVANCE, FLANK_WALL };
    gh_enemy_actor *enemy=s->enemy;
    gh_enemy_entry *entry=&enemy->entry;
    const int flank=s->pending_ai_pc==0x22cbc8;
    if(!r103_arena_boss(entry))
        return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
    gh_game_init_state *init=entry->b.init;
    const unsigned stamp=entry->request.entry_number;
    const gh_game_title_player *peer_owner=init->enemy_r103_boss[stamp^1u];
    const gh_title_player_view *peer=peer_owner?&peer_owner->view:NULL;
    gh_title_player_view *actor=&entry->object.view;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    if(!s->substate67_stage){
        put(p+0x16d4,word(p+0x16d4)|4u);
        if(flank){
            /* 262A10 checks whether any of the four Give objects hit Gene. */
            for(unsigned i=1;i<5;++i){
                const gh_hitarea_object *give=&enemy->hitareas.units[i];
                const gh_title_player_view *hit=NULL;
                if(!enemy->hitareas.created[i]||
                   (!give->fallback&&gh_hitarea_object_last_hit_actor(give,&hit)))
                    return stop(s,0x262a10,GH_ACTOR_MOTION_UNSUPPORTED);
                if(hit&&hit->bytes&&hit->size>=0x300&&half(hit->bytes+0x2fe)==0x100){
                    p[0x1864]=1;
                    break;
                }
            }
        }
        if(p[0x2f6]==11&&init->enemies[0x5a2])++p[0x2f6];
        const unsigned phase=p[0x2f6];
        if(phase>(unsigned)(flank?17:13))return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_phase=phase;
        if(phase==12)s->substate67_stage=RECOVERY_HEADING;
        else if(phase==13)s->substate67_stage=flank?RECOVERY_STEP:ATTACK_SAMPLE;
        else if(phase==14||phase==16)s->substate67_stage=ATTACK_BIND;
        else if(phase==15||phase==17)s->substate67_stage=ATTACK_SAMPLE;
        else if(phase==6||phase==8||phase==10){
            if(phase==6&&!flank)init->enemies[0x5a4]|=(uint8_t)(1u<<stamp);
            s->substate67_stage=ATTACK_BIND;
        }else if(phase==11){
            s->substate67_stage=CHARGE_RATE;
        }else if(phase==7||phase==9){
            s->substate67_stage=phase==7&&flank?ATTACK_OFFSET_STEP:ATTACK_SAMPLE;
        }else if(phase==0||phase==2){
            if(!phase)p[0x1864]=0;
            s->substate67_stage=BIND;
        }else{
            if(phase==4){
                const unsigned rank=heat_category(s);
                if(!rank)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
                put(p+0x600,rank==5?0x41800000u:0x42340000u);
                ++p[0x2f6];
            }
            s->substate67_stage=TURN;
        }
    }
    const int impact=s->substate67_phase>=8;
    const int recovery=s->substate67_phase>=12;
    const int charging=s->substate67_phase>=10&&!recovery;
    const int ending=s->substate67_phase>=16;
    const int flank_advance=s->substate67_phase>=14&&!ending;
    if(s->substate67_stage==RECOVERY_HEADING){
        const float shared_yaw=number(word(init->enemies+0x59c));
        float yaw=shared_yaw;
        if(!isfinite(yaw))return stop(s,flank?0x22d4b0:0x22cab4,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!flank){
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x22cab4,GH_ACTOR_MOTION_CALLEE);
            const float reversed=add(shared_yaw,number(0x40490fdbu));
            if(fesetenv(&env)||gh_id_angle_reference_001f7d48(reversed,&yaw))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        put(p+0x104,bits(yaw));
        s->substate67_stage=ATTACK_BIND;
    }
    if(s->substate67_stage==ATTACK_BIND){
        const size_t row=ending?0x1ae8:flank_advance?0x1ae0:
            (recovery?0x1ad0:charging?0x1ac0:impact?0x1ab0:0x1aa0)+(flank?8:0);
        const gh_object_view *resource=&entry->resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x2a8578,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            if(flank_advance)p[0x1864]=0;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,ending||!impact?3:0,0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(ending||flank_advance){
            if(flank_advance)put(p+0x604,0x41c80000u);
            ++p[0x2f6];
            s->substate67_stage=ATTACK_SAMPLE;
        }else if(charging){
            put(p+0x604,0x3f800000u);
            if(flank)put(p+0x608,0x41f00000u);
            ++p[0x2f6];
            s->substate67_stage=CHARGE_RATE;
        }else if(flank)s->substate67_stage=recovery?RECOVERY_TARGET:impact?IMPACT_ALIGN:ATTACK_OFFSET_START;
        else{
            ++p[0x2f6];
            s->substate67_stage=ATTACK_SAMPLE;
        }
    }
    if(s->substate67_stage==RECOVERY_TARGET){
        /* 22D4EC steers the flank follow-up toward Gene, up to pi/4. */
        float origin[3],target[3],turn,heading,sine,cosine;
        const float yaw=number(word(p+0x104));
        if(!position(actor,origin)||!position(s->player,target)||
           gh_player_target_angle_002da038(s->elf,s->elf_bytes,origin,target,yaw,
                number(0x3f490fdbu),&turn,&pc))
            return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x22d51c,GH_ACTOR_MOTION_CALLEE);
        const float total=add(yaw,turn);
        if(fesetenv(&env)||gh_id_angle_reference_001f7d48(total,&heading)||
           gh_sdk_rotation_sincos_reference_0030a5d0(heading,&sine,&cosine,
                GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x147988,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned rank=heat_category(s);
        if(!rank)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
        const float distance=rank==5?2.0f:1.0f;
        const float x=mul(distance,sine),z=mul(distance,cosine);
        if(fesetenv(&env)||!isfinite(x)||!isfinite(z))
            return stop(s,0x30a2b0,GH_ACTOR_MOTION_CALLEE);
        put(p+0x600,bits(turn));
        put(p+0x590,bits(x));put(p+0x594,0);put(p+0x598,bits(z));
        ++p[0x2f6];
        s->substate67_stage=RECOVERY_STEP;
    }
    if(s->substate67_stage==RECOVERY_STEP){
        if(half(p+0x3ac)&0x10u){
            const gh_actor_motion_status status=pincer_flank_step(s,p,actor);
            if(status)return status;
        }
        s->substate67_stage=ATTACK_SAMPLE;
    }
    if(s->substate67_stage==CHARGE_RATE){
        if(!peer||peer->size<0x5ac)
            return stop(s,0x158288,GH_ACTOR_MOTION_UNSUPPORTED);
        const float own_rate=number(word(p+0x5a8)),peer_rate=number(word(peer->bytes+0x5a8));
        const float current=number(word(p+0x604));
        fenv_t env;
        if(!isfinite(own_rate)||!isfinite(peer_rate)||!isfinite(current)||
           !set_round(&env,FE_TOWARDZERO))return stop(s,0x158288,GH_ACTOR_MOTION_UNSUPPORTED);
        const float next=add(current,mul(own_rate,number(0x3ca3d70au)));
        const float factor=next>2.0f?2.0f:next;
        const float speed=mul(peer_rate,factor);
        if(fesetenv(&env)||!isfinite(speed))return stop(s,0x158288,GH_ACTOR_MOTION_CALLEE);
        put(p+0x604,bits(factor));
        put(p+0x444,bits((half(p+0x434)&4u)?-speed:speed));
        s->substate67_stage=ATTACK_SAMPLE;
    }
    if(s->substate67_stage==IMPACT_ALIGN){
        /* The flank's 22D21C bind uses the peer's origin and a zero offset. */
        if(peer){
            float origin[3],previous[3],yaw;
            if(!position(peer,origin)||!position(actor,previous)||
               gh_id_angle_reference_001f7d48(number(word(p+0x104)),&yaw))
                return stop(s,0x22d24c,GH_ACTOR_MOTION_UNSUPPORTED);
            memcpy(actor->alias_f0,origin,sizeof(origin));
            put(p+0x104,bits(yaw));
        }
        ++p[0x2f6];
        s->substate67_stage=ATTACK_SAMPLE;
    }
    if(s->substate67_stage==ATTACK_OFFSET_START){
        /* 22CFA4 stores the horizontal displacement to an authored slot
         * in the front actor's space. 22D0DC consumes a fraction per tick. */
        float displacement[3]={0};
        if(peer){
            float from[3],to[3],sine,cosine,yaw;
            if(!position(actor,from)||!position(peer,to)||peer->size<0x108)
                return stop(s,0x22cfe4,GH_ACTOR_MOTION_BOUNDS);
            if(gh_sdk_rotation_sincos_reference_0030a5d0(number(word(peer->bytes+0x104)),
                &sine,&cosine,GH_STATIC_BINARY32_REFERENCE,&pc))
                return stop(s,pc?pc:0x147988,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(number(word(p+0x104)),&yaw))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x22d020,GH_ACTOR_MOTION_CALLEE);
            const float x=mul(number(word(p+0x110)),number(0x3db8bac7u));
            const float z=mul(number(word(p+0x118)),number(0x3fe9ff2eu));
            displacement[0]=sub(add(to[0],add(mul(x,cosine),mul(z,sine))),from[0]);
            displacement[2]=sub(add(to[2],sub(mul(z,cosine),mul(x,sine))),from[2]);
            if(fesetenv(&env)||!isfinite(displacement[0])||!isfinite(displacement[2]))
                return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x104,bits(yaw));
        }
        memcpy(p+0x590,displacement,sizeof(displacement));
        ++p[0x2f6];
        s->substate67_stage=ATTACK_OFFSET_STEP;
    }
    if(s->substate67_stage==ATTACK_OFFSET_STEP){
        float from[3],displacement[3],next[3];
        const float rate=number(word(p+0x5a8));
        fenv_t env;
        if(!position(actor,from)||!isfinite(rate)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x22d0dc,GH_ACTOR_MOTION_UNSUPPORTED);
        int valid=1;
        for(unsigned i=0;i<3;++i){
            const float remaining=number(word(p+0x590+4*i));
            const float step=mul(mul(remaining,number(0x3dcccccdu)),rate);
            next[i]=add(from[i],step);
            displacement[i]=sub(remaining,step);
            valid=valid&&isfinite(next[i])&&isfinite(displacement[i]);
        }
        if(fesetenv(&env)||!valid)return stop(s,0x22d0dc,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(actor->alias_f0,next,sizeof(next));
        memcpy(p+0x590,displacement,sizeof(displacement));
        s->substate67_stage=ATTACK_SAMPLE;
    }
    if(s->substate67_stage==ATTACK_SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(ended&&impact&&!charging&&(!recovery||(flank&&!flank_advance&&!ending)))++p[0x2f6];
        s->substate67_stage=ended&&(!impact||(recovery&&!flank)||ending)?ATTACK_CHOOSE:ATTACK_ROTATE;
    }
    if(s->substate67_stage==ATTACK_CHOOSE){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ATTACK_ROTATE;
    }
    if(s->substate67_stage==ATTACK_ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,entry->b.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ATTACK_TRANSLATE;
    }
    if(s->substate67_stage==ATTACK_TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,entry->b.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ending?COLLISION:flank_advance?FLANK_ADVANCE:
            charging?CHARGE_UPDATE:impact?IMPACT_GIVE:ATTACK_SIGNAL;
    }
    if(s->substate67_stage==FLANK_ADVANCE){
        const float timer=number(word(p+0x604)),rate=number(word(p+0x5a8));
        fenv_t env;
        if(!isfinite(timer)||!isfinite(rate)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x22d7f8,GH_ACTOR_MOTION_UNSUPPORTED);
        const float remaining=sub(timer,rate);
        if(fesetenv(&env)||!isfinite(remaining))return stop(s,0x22d804,GH_ACTOR_MOTION_CALLEE);
        const gh_actor_motion_status status=pincer_flank_step(s,p,actor);
        if(status)return status;
        put(p+0x604,bits(remaining));
        if(remaining<=0||p[0x1864])p[0x2f6]=16;
        s->substate67_stage=IMPACT_GIVE;
    }
    if(s->substate67_stage==CHARGE_UPDATE){
        if(flank){
            const float budget=number(word(p+0x608));
            const float rate=number(word(p+0x5a8));
            if(!isfinite(budget)||!isfinite(rate))return stop(s,0x22d424,GH_ACTOR_MOTION_UNSUPPORTED);
            if(budget>0){
                fenv_t env;
                if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x22d424,GH_ACTOR_MOTION_CALLEE);
                const float remaining=sub(budget,rate);
                if(fesetenv(&env)||!isfinite(remaining))return stop(s,0x22d424,GH_ACTOR_MOTION_CALLEE);
                put(p+0x608,bits(remaining));
                s->substate67_stage=remaining<=0?CHARGE_EFFECT:IMPACT_GIVE;
            }else s->substate67_stage=IMPACT_GIVE;
        }else{
            if(number(word(p+0x604))>=2.0f){
                float self[3],player[3],reverse,angle;
                const float yaw=number(word(p+0x104));
                fenv_t env;
                if(!position(actor,self)||!position(s->player,player)||!isfinite(yaw)||
                   !set_round(&env,FE_TOWARDZERO))return stop(s,0x22c9c0,GH_ACTOR_MOTION_UNSUPPORTED);
                reverse=add(yaw,number(0x40490fdbu));
                if(fesetenv(&env)||gh_id_angle_reference_001f7d48(reverse,&reverse))
                    return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
                if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,self,player,reverse,
                    number(0x40490fdbu),&angle,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&init->random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if((draw&1u)&&angle>number(0xbf490fdbu)&&angle<number(0xbe860a92u)){
                    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x22ca54,GH_ACTOR_MOTION_CALLEE);
                    const float turn=add(reverse,angle);
                    if(fesetenv(&env)||gh_id_angle_reference_001f7d48(turn,&reverse))
                        return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
                    init->enemies[0x5a5]|=1u;
                    put(init->enemies+0x59c,bits(reverse));
                }
            }
            s->substate67_stage=IMPACT_GIVE;
        }
    }
    if(s->substate67_stage==CHARGE_EFFECT){
        gh_effect_create_call *call=&s->death_effect_create;
        gh_effect_create_bindings bindings=s->impact_bindings;
        bindings.model=&entry->initialize.model.model;
        if(!call->active||call->complete){
            const gh_effect_create_request request={.bank=0x58,.index=0x28,.actor=actor,.mode=0,.selector=-1};
            const int scaled=(half(p+0x434)&2u)!=0;
            put_half(p+0x570,0);
            put(p+0x574,0);
            gh_effect_status status=gh_effect_create_actor_sequence_0014fb48(call,&request,
                scaled?word(p+0x114):0x3f800000u);
            if(status)return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        gh_effect_status status=gh_game_effect_create_00306528(call,&bindings);
        if(status||!call->complete)return stop(s,call->frontier?call->frontier:0x306528,
            status==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE);
        s->substate67_stage=IMPACT_GIVE;
    }
    if(s->substate67_stage==IMPACT_GIVE){
        if(half(p+0x3ac)&1u){
            /* 22D338 uses power30 for the opening flank. 22D464 uses 40
             * once the charge begins, as do the original recovery rows. */
            const unsigned power=flank&&!charging&&!recovery?30:40;
            stamp19_attack_parameters(p,power,0,flank?0x3e:0x3f);
            const gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
            if(status)return status;
            if(flank&&recovery&&p[0x1864])p[0x2f6]=16;
        }else if(flank&&recovery&&!flank_advance)p[0x1864]=0;
        s->substate67_stage=flank_advance?FLANK_WALL:recovery?COLLISION:IMPACT_PEER;
    }
    if(s->substate67_stage==FLANK_WALL){
        float origin[3],sine,cosine;
        if(!position(actor,origin)||gh_sdk_rotation_sincos_reference_0030a5d0(
            number(word(p+0x104)),&sine,&cosine,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x275888,GH_ACTOR_MOTION_UNSUPPORTED);
        const float height=add(origin[1],0.5f);
        const gh_scene_ray_request ray={.mask=1,.flags=0x124,.segmented=1,
            .from={origin[0],height,origin[2],1},
            .to={add(origin[0],mul(2.5f,sine)),height,add(origin[2],mul(2.5f,cosine)),1}};
        int blocked;
        if(!enemy->b.actors||gh_scene_ray_query_00131e10(enemy->b.actors,&ray,&blocked,&pc))
            return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
        if(blocked)p[0x2f6]=16;
        s->substate67_stage=COLLISION;
    }
    if(s->substate67_stage==IMPACT_PEER){
        if(flank)p[0x617]=1;
        const unsigned peer_state=flank?0x3b00:0x3c00;
        const int matched=peer&&peer->size>=0x2f6&&half(peer->bytes+0x2f4)==peer_state;
        if(!matched)enemy_decision(p,1,flank?3:0x11,flank?1:0);
        s->substate67_stage=matched||flank?COLLISION:5;
    }
    if(s->substate67_stage==ATTACK_SIGNAL){
        if(init->enemies[0x5a1]){
            if(flank)p[0x617]=1;
            ++p[0x2f6];
        }else if(flank&&(half(p+0x3ac)&1u))init->enemies[0x5a4]|=(uint8_t)(1u<<stamp);
        s->substate67_stage=COLLISION;
    }
    if(s->substate67_stage==BIND){
        const int approach=p[0x2f6]==0;
        const size_t row=approach?0x10c0:0x1a98;
        /* The flank wait passes the motion itself as the optional SEQ candidate. */
        const size_t sequence=approach?0x10c8:row+4;
        const gh_object_view *resource=&entry->resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<sequence+4||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x2a8578,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint32_t offset=word(resource->bytes+row);
            const uint32_t seq=!approach&&flank?offset:word(resource->bytes+sequence);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,offset,seq,0,10,0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(approach)s->substate67_stage=CUE;
        else{
            /* 293718 ORs actor identity, independent of the front/flank role. */
            init->enemies[0x5a3]|=(uint8_t)(1u<<stamp);
            put(p+0x600,0x42b40000u);
            ++p[0x2f6];
            s->substate67_stage=TURN;
        }
    }
    if(s->substate67_stage==CUE){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint16_t program=(uint16_t)(word(p+0x424)+10u);
        gh_motion_sound_request request={.model=motion->b.model,.actor=actor,.bank=1};
        memcpy(&request.program,&program,sizeof(program));
        uint32_t handle=0;
        gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        put(p+0x600,0x43160000u);
        ++p[0x2f6];
        s->substate67_stage=TURN;
    }
    if(s->substate67_stage==TURN){
        const int approach=p[0x2f6]==1;
        if(approach||peer){
            float from[3],to[3],turn;
            if(!position(actor,from)||(!approach&&!position(peer,to)))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_BOUNDS);
            if(approach)memcpy(to,p+0x580,sizeof(to));
            const float yaw=number(word(p+0x104)),rate=number(word(p+0x5a8));
            const float work_rate=number(word((const uint8_t *)entry->b.work+0x55c));
            fenv_t env;
            if(!isfinite(yaw)||!isfinite(rate)||!isfinite(work_rate)||
               !isfinite(to[0])||!isfinite(to[2])||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float cap=mul(mul(rate,number(0x3e490fdb)),work_rate);
            const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
                from,to,yaw,cap,&turn,&pc);
            float next=status?0:add(yaw,turn);
            if(fesetenv(&env)||status)return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x104,bits(next));
        }
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,entry->b.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,entry->b.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TIMERS;
    }
    if(s->substate67_stage==TIMERS){
        const unsigned phase=p[0x2f6];
        if(!peer&&phase==5)s->substate67_stage=CANCEL;
        else{
            const float remaining=number(word(p+0x600)),rate=number(word(p+0x5a8));
            float self[3];
            fenv_t env;
            if(!position(actor,self)||!isfinite(remaining)||!isfinite(rate)||
               !set_round(&env,FE_TOWARDZERO))
                return stop(s,0x1f7eb0,GH_ACTOR_MOTION_UNSUPPORTED);
            const float dx=sub(number(word(p+0x580)),self[0]),dz=sub(number(word(p+0x588)),self[2]);
            const float distance=add(mul(dx,dx),mul(dz,dz)),next=sub(remaining,rate);
            if(fesetenv(&env)||!isfinite(next)||(phase==1&&!isfinite(distance)))
                return stop(s,0x1f7eb0,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x600,bits(next));
            if(phase==1&&distance<1.0f)++p[0x2f6];
            if(!peer||(phase!=5&&next<=0.0f))s->substate67_stage=CANCEL;
            else{
                if((phase==3&&init->enemies[0x5a0])||(phase==5&&next<=0.0f))++p[0x2f6];
                s->substate67_stage=COLLISION;
            }
        }
    }
    if(s->substate67_stage==CANCEL){
        init->enemies[0x5a0]=0;
        init->enemies[0x5a3]=0;
        s->substate67_stage=CHOOSE;
    }
    if(s->substate67_stage==CHOOSE){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=COLLISION;
    }
    if(s->substate67_stage==COLLISION){
        if((half(p+0x3ac)&0x20u)&&
           gh_collision_solid_set_enabled_001351d8(enemy->b.solids,actor,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_target(gh_enemy202_move *s,uint8_t *p,
    const float actor[3],const float player[3])
{
    if(s->pending_ai_pc==0x26f328){
        if(!s->visibility_done){
            gh_actor_motion_status status=target_line_of_sight(s,p,actor,player);
            if(status)return status;
        }
        gh_actor_motion_status status=choose_path_target(s,p,actor,player);
        if(status)return status;
        put(p+0x16d4,word(p+0x16d4)&~0x08000000u);
        s->path_target_ready=1;
        s->pending_ai_pc=0x2a74e0;
    }
    if(s->pending_ai_pc==0x2a74e0){
        gh_actor_motion_status status=route_clear_002a74e0(s,p,actor);
        if(status)return status;
    }
    if(s->pending_ai_pc==0x20fa64){
        if(p[0x2f5]>0xad)p[0x2f5]=0;
        const uint8_t *descriptor=elf_span(s->elf,s->elf_bytes,0x3bf1c8+8u*p[0x2f5],8);
        if(descriptor&&half(descriptor)==0&&half(descriptor+2)==0xffff&&word(descriptor+4))
            s->pending_ai_pc=word(descriptor+4);
        else s->pending_ai_pc=0x20fa68;
    }
    if(!s->enemy){
        if(s->pending_ai_pc==0x211c08)
            return advance_substate0_00211c08(s,p);
        if(s->pending_ai_pc==0x213ea8)
            return advance_substate7_00213ea8(s,p);
        if(s->pending_ai_pc==0x217980)
            return advance_devil_retreat_00217980(s,p);
        if(s->pending_ai_pc==0x2269f0)
            return advance_ranged_002269f0(s,p);
        if(s->pending_ai_pc==0x21a538)
            return advance_attack_0021a538(s,p);
        if(s->pending_ai_pc==0x21fd50)
            return advance_devil_combo_0021fd50(s,p);
        if(s->pending_ai_pc==0x23b968)
            return advance_substate6b_or_6c(s,p);
        if(s->pending_ai_pc!=0x212c08)return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!s->substate67_stage){
            uint32_t frontier=0;
            const gh_actor_motion_status status=s->devil_selector(s->devil_selector_context,&frontier);
            if(status)return stop(s,frontier?frontier:0x212c08,status);
            s->substate67_stage=5;
        }
        return advance_state0_tail(s,p);
    }
    if(s->pending_ai_pc==0x22c3f8||s->pending_ai_pc==0x22cbc8)return advance_boss_pincer(s,p);
    if(s->pending_ai_pc==0x2426a0)return advance_r103_boss_idle_002426a0(s,p);
    if(s->pending_ai_pc==0x23f028)return advance_substate71_0023f028(s,p);
    if(s->pending_ai_pc==0x20fbb8)return advance_substate1_0020fbb8(s,p);
    if(s->pending_ai_pc==0x210ee8)return advance_airborne_wait_00210ee8(s,p);
    if(s->pending_ai_pc==0x240548)return advance_substate75_00240548(s,p);
    if(s->pending_ai_pc==0x2112f0)return advance_substate3_002112f0(s,p);
    if(s->pending_ai_pc==0x23aad0)return advance_substate67(s,p);
    if(s->pending_ai_pc==0x211c08)return advance_substate0_00211c08(s,p);
    if(s->pending_ai_pc==0x2130c8)return advance_substate5_002130c8(s,p);
    if(s->pending_ai_pc==0x213ea8)return advance_substate7_00213ea8(s,p);
    if(s->pending_ai_pc==0x213710)return advance_substate6_00213710(s,p);
    if(s->pending_ai_pc==0x218c48)return advance_substate13_00218c48(s,p);
    if(s->pending_ai_pc==0x23f970)return advance_ground_pickup_0023f970(s,p);
    if(s->pending_ai_pc==0x21a218&&r102_route_runner(s->enemy->b.entry.work,p))
        return advance_r102_route_wait(s,p,0);
    if(s->pending_ai_pc==0x214298)return advance_substate8_00214298(s,p);
    if(s->pending_ai_pc==0x2147d0)return advance_substate9_002147d0(s,p);
    if(s->pending_ai_pc==0x215d40)return advance_em03_approach_00215d40(s,p);
    if(s->pending_ai_pc==0x214de0)return advance_rear_evade_00214de0(s,p);
    if(s->pending_ai_pc==0x23a950)return advance_em0a_substate66_0023a950(s,p);
    if(s->pending_ai_pc==0x23afd8)return advance_substate69_0023afd8(s,p);
    if(s->pending_ai_pc==0x23d7d0)return advance_boss_deflect_0023d7d0(s,p);
    if(s->pending_ai_pc==0x23b968||s->pending_ai_pc==0x23c620)
        return advance_substate6b_or_6c(s,p);
    if(s->pending_ai_pc==0x23e6b0)return advance_wave_counter_0023e6b0(s,p);
    if(s->pending_ai_pc==0x233738||(s->pending_ai_pc==0x21fd50&&
       r101_em03_stamp13(s->enemy->b.entry.work->room,p)))
        return advance_armed_attack(s,p);
    if(s->pending_ai_pc==0x22fae0)return advance_boss_paired_attack_0022fae0(s,p);
    if(s->pending_ai_pc==0x21fd50){
        if(r100_armed_em02(s->enemy,p))
            return advance_armed_attack(s,p);
        if(r102_armed_em00(s->enemy,p)&&gh_enemy_timers_room_weapon(&s->enemy->timers))
            return advance_r102_armed16_0021fd50(s,p);
        return advance_unarmed16_0021fd50(s,p);
    }
    if(s->pending_ai_pc==0x223470)return advance_formation_em02_00223470(s,p);
    if(s->pending_ai_pc==0x230f20)return advance_formation_00230f20(s,p);
    if(s->pending_ai_pc==0x2311a8)return advance_formation_partner_002311a8(s,p);
    if(s->pending_ai_pc==0x234d30)return advance_substate4b_00234d30(s,p);
    if(s->pending_ai_pc==0x235db8)return advance_grab_substate4c_00235db8(s,p);
    if(s->pending_ai_pc==0x236190)return advance_em00_substate4d_00236190(s,p);
    if(s->pending_ai_pc==0x2363e8)return advance_substate4e_002363e8(s,p);
    if(s->pending_ai_pc==0x224090)return advance_r104_projectile_windup_00224090(s,p);
    if(s->pending_ai_pc==0x2269f0)return advance_ranged_002269f0(s,p);
    if(s->pending_ai_pc==0x226450)return advance_r104_selector21_00226450(s,p);
    if(s->pending_ai_pc==0x236ed0)return advance_boss_throw_00236ed0(s,p);
    if(s->pending_ai_pc==0x2236e8)return advance_em0a_slide_002236e8(s,p);
    if(s->pending_ai_pc==0x21a538||s->pending_ai_pc==0x231ac0||s->pending_ai_pc==0x232500)
        return advance_attack_0021a538(s,p);
    return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
}


static gh_actor_motion_status triangle_recovery_00274238(gh_enemy202_move *s,uint8_t *p)
{
    gh_game_init_state *init=s->enemy->b.entry.init;
    if(!(word(p+0x16d0)&0x2000u)){
        static const float duration[]={0,40,30,20,20,15};
        const unsigned category=heat_category(s);
        if(category<1||category>5)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        float timer=duration[category];
        if(category<=4){
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x274238,GH_ACTOR_MOTION_CALLEE);
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&init->random,&draw)){
                (void)fesetenv(&env);
                return stop(s,0x31d000,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            timer=add(timer,mul(number(gh_frame_unit_bits_0030f518_word(draw)),4.0f));
            if(fesetenv(&env))return stop(s,0x274238,GH_ACTOR_MOTION_CALLEE);
        }
        put(p+0x173c,bits(timer));
    }
    enemy_decision(p,0,0x69,0);
    s->state1_phase=17;
    return GH_ACTOR_MOTION_OK;
}

/* 24DA80 tests the zero wait before root delta. Keep committed MT
 * decisions across a paused 262AA8, and preserve its original early return. */
/* Original 273C38 checks a six-unit rearward segment in the actor's world matrix. */
static gh_actor_motion_status triangle_rear_clear_00273c38(gh_enemy202_move *s,
    const uint8_t *p,int *clear)
{
    const uint32_t dependency=word(p+0x564);
    *clear=0;
    if(dependency>=0x220&&dependency<=0x222)return GH_ACTOR_MOTION_OK;
    float matrix[16];
    memcpy(matrix,p+0x80,sizeof matrix);
    const float local_from[3]={0,0.5f,0},local_to[3]={0,0.5f,-6};
    gh_scene_ray_request ray={.segmented=1};
    uint32_t pc=0;
    int hit;
    if(gh_scene_at_apply_world_001508d8(matrix,local_from,ray.from)||
       gh_scene_at_apply_world_001508d8(matrix,local_to,ray.to))
        return stop(s,0x30a2b0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->enemy->b.actors||gh_scene_ray_query_00131e10(s->enemy->b.actors,&ray,&hit,&pc))
        return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    *clear=hit!=1;
    return GH_ACTOR_MOTION_OK;
}

/* The 24CEC8 and 24E438 calls both pass forced=1 to the original 268068. */
static gh_actor_motion_status r104_forced_recovery_00268068(gh_enemy202_move *s,uint8_t *p,
    gh_game_mt *random,int *chosen)
{
    const float distance=number(word(p+0x618)),face=number(word(p+0x760));
    *chosen=0;
    if(!isfinite(distance)||!isfinite(face))return stop(s,0x268068,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!(word(p+0x16d0)&0x200000u)||!(word(p+0x16d4)&0x08000000u))
        return GH_ACTOR_MOTION_OK;
    uint32_t draw;
    const unsigned heat=heat_category(s);
    if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
    unsigned selector=0,variant=0;
    if(heat<5&&face>number(0x3fa78d36)){
        if(gh_game_mt_next_0030f348(random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(draw&1u){
            put(p+0x16d4,word(p+0x16d4)|1u);
            selector=8;
            variant=1;
        }
    }
    if(!selector&&p[0x17b9]>=2&&p[0x17b9]<4&&distance<6.25f){
        if(gh_game_mt_next_0030f348(random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if((draw&3u)==1){
            int clear;
            const gh_actor_motion_status status=triangle_rear_clear_00273c38(s,p,&clear);
            if(status)return status;
            if(clear){selector=10;variant=1;}
        }
    }
    if(!selector&&distance<=4.0f&&!(word(p+0x16d4)&0x10380u))
        selector=0x45;
    if(selector){
        enemy_decision(p,0,selector,variant);
        *chosen=1;
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status r104_frontal_recovery_0024cec8(gh_enemy202_move *s,uint8_t *p)
{
    gh_game_init_state *init=s->enemy->b.entry.init;
    const float distance=number(word(p+0x618)),cooldown=number(word(p+0x1740));
    if(!isfinite(distance)||!isfinite(cooldown)||!isfinite(number(word(p+0x760))))
        return stop(s,0x24cec8,GH_ACTOR_MOTION_UNSUPPORTED);
    gh_game_mt random=init->random;
    uint32_t draw;
    if(gh_game_mt_next_0030f348(&random,&draw))
        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!(draw&3u)&&distance<25.0f&&!init->heat[0x10]){
        int chosen;
        const gh_actor_motion_status status=r104_forced_recovery_00268068(s,p,&random,&chosen);
        if(status)return status;
        if(chosen){
            init->random=random;
            s->state1_phase=17;
            return GH_ACTOR_MOTION_OK;
        }
    }
    if(gh_game_mt_next_0030f348(&random,&draw))
        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
    init->random=random;
    s->state1_phase=(draw%12u==0&&distance<9.0f&&!init->heat[0x10]&&
        p[0x17bb]&&cooldown<=0)?27:26;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status triangle_zero_wait(gh_enemy202_move *s,uint8_t *p)
{
    gh_game_init_state *init=s->runtime.entry->b.init;
    if(s->state1_phase==23)return s->enemy?triangle_recovery_00274238(s,p):stop(s,0x274238,GH_ACTOR_MOTION_UNSUPPORTED);
    if(s->state1_phase==20){
        const float distance=number(word(p+0x618)),cooldown=number(word(p+0x1740));
        if(!isfinite(distance)||!isfinite(cooldown))
            return stop(s,0x24e340,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t draw;
        if(p[0x17bb]&&cooldown<=0&&half(p+0x568)){
            /* dep0250 takes the additional forced-choice draw at 24E3F8. */
            if(gh_enemy203_is_r104_entry(&s->enemy->entry.request,s->enemy->entry.b.work->room)&&
               distance<25.0f&&!init->heat[0x10]){
                gh_game_mt random=init->random;
                if(gh_game_mt_next_0030f348(&random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(draw&1u){
                    int chosen;
                    const gh_actor_motion_status status=r104_forced_recovery_00268068(s,p,&random,&chosen);
                    if(status)return status;
                    if(chosen){
                        init->random=random;
                        s->state1_phase=17;
                        return GH_ACTOR_MOTION_OK;
                    }
                }
                init->random=random;
            }
            if(gh_game_mt_next_0030f348(&init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(draw%12==0&&distance<9&&!init->heat[0x10]){
                s->state1_phase=23;
                return s->enemy?triangle_recovery_00274238(s,p):stop(s,0x274238,GH_ACTOR_MOTION_UNSUPPORTED);
            }
        }
        s->state1_phase=21;
    }
    if(s->state1_phase==21){
        const float distance=number(word(p+0x618)),cooldown=number(word(p+0x1740));
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(draw%12==0&&distance<9&&!init->heat[0x10]&&p[0x17bb]&&cooldown<=0){
            s->state1_phase=23;
            return s->enemy?triangle_recovery_00274238(s,p):stop(s,0x274238,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(p[0x17b9]){
            if(gh_game_mt_next_0030f348(&init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            s->state1_phase=(draw&3u)?22:24;
        }else s->state1_phase=22;
    }
    if(s->state1_phase==24){
        int clear;
        gh_actor_motion_status status=triangle_rear_clear_00273c38(s,p,&clear);
        if(status)return status;
        if(clear){
            enemy_decision(p,0,10,1);
            s->state1_phase=17;
            return GH_ACTOR_MOTION_OK;
        }
        s->state1_phase=22;
    }
    int chosen;
    gh_actor_motion_status status=s->enemy?attack_choice_00262aa8(s,p,&chosen):
        devil_near_choice_00262aa8(s,p,0,&chosen);
    if(status)return status;
    s->state1_phase=chosen?17:14;
    return GH_ACTOR_MOTION_OK;
}

/* The 24D0F0 Square and 24DA80 Triangle rear reactions use distinct clips
 * and wait draws, followed by the same stun, attack-choice and root steps. */
static gh_actor_motion_status advance_short_rear_reaction(gh_enemy202_move *s,uint8_t *p)
{
    enum { PREPARE=30, BIND, VOICE, RECOVERY_CUE, DELAY, STEP_EFFECT, SAMPLE,
        CHOOSE, RESISTANCE, WAIT, ATTACK, ROTATE, TRANSLATE, RECOVERY_DECIDE, OPEN_VOICE, OPEN_OFFSET };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_game_init_state *init=enemy->entry.b.init;
    gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    const int triangle=s->pending_ai_pc==0x24da80;
    const int r104=gh_enemy203_is_r104_entry(&enemy->entry.request,enemy->entry.b.work->room);
    const size_t row_offset=r104?0x9f0u:triangle?0x1268u:0x540u;
    if(s->state1_phase==PREPARE){
        const gh_object_view *resource=&enemy->entry.resource;
        const uint32_t dependency=word(p+0x564);
        const uint32_t selection=triangle?0x43e2f0+(dependency-0x200)*4u:r104?0x43dec0:0x43dd80;
        const uint32_t entry=triangle?(r104?0x24e76c:0x24e938):r104?0x24d16c:0x24d12c;
        const uint8_t *leaf=elf_span(s->elf,s->elf_bytes,selection,4);
        if(!leaf||word(leaf)!=entry||word(p+0x6ec)||
            !resource->bytes||!resource->file_extent_known||resource->file_extent<row_offset+8||
            resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,entry,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!triangle&&!r104){
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x608,(draw&3u)?0x41200000u:0);
        }
        s->state1_phase=r104?OPEN_VOICE:BIND;
    }
    if(s->state1_phase==OPEN_VOICE){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request request={.model=motion->b.model,.actor=actor,
            .bank=1,.program=(int16_t)(half(p+0x424)+0x23u)};
        uint32_t handle;
        const gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->state1_phase=triangle?BIND:OPEN_OFFSET;
    }
    if(s->state1_phase==OPEN_OFFSET){
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x608,(draw&3u)?0x41200000u:0);
        s->state1_phase=BIND;
    }
    if(s->state1_phase==BIND){
        const uint8_t *row=enemy->entry.resource.bytes+row_offset;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(row),word(row+4),triangle?0:word(p+0x608),2,
                p[0x17c3]?0x42:0x40,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        s->state1_phase=VOICE;
    }
    if(s->state1_phase==VOICE){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request request={.model=motion->b.model,.actor=actor,
            .bank=1,.program=(int16_t)half(p+0x424)};
        uint32_t handle;
        const gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->state1_phase=DELAY;
        if((int32_t)word(p+0x5b4)>=200){
            put(p+0x5b4,0);
            put(p+0x17cc,0x43160000u);
            s->state1_phase=RECOVERY_CUE;
        }
    }
    if(s->state1_phase==RECOVERY_CUE){
        const gh_motion_sound_request request={.model=motion->b.model,.actor=actor,
            .bank=0,.program=0x8c};
        uint32_t handle;
        const gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        p[0x621]=0;
        s->state1_phase=DELAY;
    }
    if(s->state1_phase==DELAY){
        const unsigned heat=heat_category(s);
        if(heat<1||heat>5)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_game_mt next=init->random;
        uint32_t delay,jitter;
        if(gh_game_mt_next_0030f348(&next,&delay))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        unsigned wait=delay%45u+60u-15u*(heat>4?3:heat-1);
        if(triangle&&heat<=2){
            uint32_t short_wait;
            if(gh_game_mt_next_0030f348(&next,&short_wait))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(short_wait%(heat==1?8u:6u)==0)wait=15;
        }
        if(gh_game_mt_next_0030f348(&next,&jitter))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        const float yaw=number(word(p+0x104));
        fenv_t env;
        if(!isfinite(yaw)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x24d4fc,GH_ACTOR_MOTION_UNSUPPORTED);
        float turned=add(yaw,mul(number(gh_frame_jitter_bits_0031d020_word(jitter)),number(0x3f060a92)));
        if(fesetenv(&env)||gh_id_angle_reference_001f7d48(turned,&turned))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        init->random=next;
        put(p+0x104,bits(turned));
        put(p+0x5f4,wait);
        put(p+0x600,0x41200000u);
        put(p+0x604,0x40000000u);
        p[0x2f6]=3;
        s->state1_phase=triangle?SAMPLE:STEP_EFFECT;
    }
    const float rate=number(word(p+0x5a8));
    if(!isfinite(rate))return stop(s,0x24d550,GH_ACTOR_MOTION_UNSUPPORTED);
    if(s->state1_phase==STEP_EFFECT){
        const float active=number(word(p+0x608));
        if(!isfinite(active))return stop(s,0x24d550,GH_ACTOR_MOTION_UNSUPPORTED);
        if(active>0){
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x24d55c,GH_ACTOR_MOTION_CALLEE);
            const float remaining=sub(active,rate);
            if(fesetenv(&env))return stop(s,0x24d55c,GH_ACTOR_MOTION_CALLEE);
            put(p+0x16d0,word(p+0x16d0)|0x00800000u);
            put(p+0x608,bits(remaining));
        }
        s->state1_phase=SAMPLE;
    }
    if(s->state1_phase==SAMPLE){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(ended&&number(word(p+0x17cc))>0){
            enemy_decision(p,1,0x0f,0);
            s->state1_phase=RESISTANCE;
        }else s->state1_phase=ended?CHOOSE:RESISTANCE;
        if(triangle&&s->state1_phase==RESISTANCE&&p[0x2f7])s->state1_phase=17;
    }
    if(s->state1_phase==CHOOSE){
        const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->state1_phase=RESISTANCE;
    }
    if(s->state1_phase==RESISTANCE){
        if(!p[0x2f7]&&number(word(p+0x17cc))>0){
            const float timer=number(word(p+0x600));
            fenv_t env;
            if(!isfinite(timer)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x24d604,GH_ACTOR_MOTION_UNSUPPORTED);
            const float remaining=sub(timer,rate);
            if(fesetenv(&env))return stop(s,0x24d604,GH_ACTOR_MOTION_CALLEE);
            put(p+0x600,bits(remaining));
            if(remaining<=0)enemy_decision(p,1,0x0f,0);
        }
        s->state1_phase=WAIT;
    }
    if(s->state1_phase==WAIT){
        const unsigned wait=(half(p+0x3ac)&0x10)?0:word(p+0x5f4);
        if(wait)put(p+0x5f4,wait-1);
        else put(p+0x5f4,0);
        s->state1_phase=ROTATE;
        if(!wait&&!(number(word(p+0x17cc))>0)){
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(draw%12u==0&&number(word(p+0x618))<9&&!init->heat[0x10]&&p[0x17bb]&&
                number(word(p+0x1740))<=0){
                s->state1_phase=RECOVERY_DECIDE;
            }else s->state1_phase=ATTACK;
        }
    }
    if(s->state1_phase==RECOVERY_DECIDE)return triangle_recovery_00274238(s,p);
    if(s->state1_phase==ATTACK){
        int chosen;
        const gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
        if(status)return status;
        s->state1_phase=chosen?17:ROTATE;
    }
    if(s->state1_phase==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=TRANSLATE;
    }
    if(s->state1_phase==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        const float timer=number(word(p+0x604));
        if(timer>0&&!init->heat[0x10]){
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x24d770,GH_ACTOR_MOTION_CALLEE);
            const float remaining=sub(timer,rate);
            if(fesetenv(&env))return stop(s,0x24d770,GH_ACTOR_MOTION_CALLEE);
            put(p+0x604,bits(remaining));
        }
        s->state1_phase=17;
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_triangle_rear_0024e95c(gh_enemy202_move *s,uint8_t *p)
{
    const gh_enemy_move_actor *runtime=&s->runtime;
    const gh_enemy_entry *owner=runtime->entry;
    gh_game_init_state *init=owner->b.init;
    gh_actor_motion *motion=runtime->motion;
    uint32_t pc=0;
    if(s->state1_phase==30){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            /* EM03 uses the live EM02 motion directory after the TXE exchange. */
            const gh_object_view *resource=&owner->resource;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x548||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x24e95c,GH_ACTOR_MOTION_BOUNDS);
            const uint8_t *row=resource->bytes+0x540;
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(row),word(row+4),0,2,0x40u|(p[0x17c3]?2u:0u),1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        s->state1_phase=31;
    }
    if(s->state1_phase==31){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request request={.model=motion->b.model,.actor=&owner->object.view,
            .bank=1,.program=(int16_t)half(p+0x424)};
        uint32_t handle;
        const gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->state1_phase=32;
    }
    if(s->state1_phase==32){
        if((int32_t)word(p+0x5b4)>199)return stop(s,0x26f120,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned rank=heat_category(s);
        if(!rank)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t delay,short_wait,yaw_draw;
        gh_game_mt next=init->random;
        if(gh_game_mt_next_0030f348(&next,&delay)||gh_game_mt_next_0030f348(&next,&short_wait)||
           gh_game_mt_next_0030f348(&next,&yaw_draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned divisor=rank==1?8u:rank==2?6u:4u;
        const unsigned wait=short_wait%divisor?delay%45u+60u-15u*(rank>4?3u:rank-1u):15u;
        fenv_t env;
        const float yaw=number(word(p+0x104));
        if(!isfinite(yaw)||!set_round(&env,FE_TOWARDZERO))
            return stop(s,0x24eb08,GH_ACTOR_MOTION_UNSUPPORTED);
        const float jitter=number(gh_frame_jitter_bits_0031d020_word(yaw_draw));
        float turned=add(yaw,mul(jitter,0.5235988f));
        if(fesetenv(&env)||gh_id_angle_reference_001f7d48(turned,&turned))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        init->random=next;
        put(p+0x5f4,wait);
        put(p+0x604,0x40000000);
        put(p+0x600,0x41200000);
        put(p+0x104,bits(turned));
        p[0x2f6]=3;
        s->state1_phase=33;
    }
    if(s->state1_phase==33){
        if(!runtime->bindings->canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *runtime->bindings->canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state1_phase=ended?34:35;
    }
    if(s->state1_phase==34){
        if(number(word(p+0x17cc))>0)return stop(s,0x270c78,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->state1_phase=35;
    }
    if(s->state1_phase==35){
        if(p[0x2f7]){s->state1_phase=17;return GH_ACTOR_MOTION_OK;}
        const float resistance=number(word(p+0x17cc));
        if(!isfinite(resistance)||resistance>0)return stop(s,0x24ebd0,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint32_t wait=(half(p+0x3ac)&0x10u)?0:word(p+0x5f4);
        if(wait){
            put(p+0x5f4,wait-1);
            s->state1_phase=38;
        }else{
            const float distance=number(word(p+0x618)),cooldown=number(word(p+0x1740));
            uint32_t draw;
            if(!isfinite(distance)||!isfinite(cooldown))return stop(s,0x24ec60,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_game_mt_next_0030f348(&init->random,&draw))return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f4,0);
            s->state1_phase=draw%12u==0&&distance<9&&!init->heat[0x10]&&p[0x17bb]&&cooldown<=0?36:37;
        }
    }
    if(s->state1_phase==36){
        const gh_actor_motion_status status=triangle_recovery_00274238(s,p);
        if(status)return status;
        s->state1_phase=17;
        return GH_ACTOR_MOTION_OK;
    }
    if(s->state1_phase==37){
        int chosen;
        const gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
        if(status)return status;
        s->state1_phase=chosen?17:38;
    }
    if(s->state1_phase==38){
        if(gh_player_root_rotation_0014f9f0(&owner->object.view,owner->b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=39;
    }
    if(s->state1_phase==39){
        if(gh_player_root_translation_0014f928(&owner->object.view,owner->b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=40;
    }
    if(s->state1_phase==40){
        const float timer=number(word(p+0x604)),rate=number(word(p+0x5a8));
        if(!isfinite(timer)||!isfinite(rate))return stop(s,0x24ed38,GH_ACTOR_MOTION_UNSUPPORTED);
        if(timer>0&&!init->heat[0x10]){
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x24ed38,GH_ACTOR_MOTION_CALLEE);
            const float remaining=sub(timer,rate);
            if(fesetenv(&env)||!isfinite(remaining))return stop(s,0x24ed38,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x604,bits(remaining));
        }
        s->state1_phase=17;
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_first_reaction_0024c3c8(gh_enemy202_move *s,uint8_t *p)
{
    const gh_enemy_move_actor *runtime=&s->runtime;
    const gh_enemy_entry *owner=runtime->entry;
    const gh_boot_work *work=owner->b.work;
    gh_game_init_state *init=owner->b.init;
    gh_actor_motion *motion=runtime->motion;
    const gh_hitarea_object *body=&runtime->hitareas->units[0];
    uint32_t pc=0;
    const int triangle=s->pending_ai_pc==0x24da80;
    const uint32_t entry=triangle?0x24da80:0x24c3c8;
    const unsigned stamp=half(p+0x640),dependency=word(p+0x564);
    const int devil=!s->enemy&&dependency==0x220&&stamp==0xff;
    const int arena_boss=r103_arena_boss(owner);
    const int boss_square=arena_boss&&!triangle;
    const int boss_triangle=arena_boss&&triangle;
    const int r104=gh_enemy203_is_r104_entry(&owner->request,work->room);
    const int opening_triangle=work->room==0x100&&
        ((stamp==19&&dependency==0x203)||((stamp==8||stamp==20)&&dependency==0x200)||
         r100_at2_formation(work,p)||r100_at2_enemy(work,p)||r100_at3_enemy(work,p)||
         r100_west_em03_patrol(work,p)||devil);
    const int arena_em0a=r103_arena_em0a(owner);
    const int gate_em0a_reaction=triangle&&r102_gate_em0a(work,p);
    const int em0a_reaction=gate_em0a_reaction||(triangle&&arena_em0a);
    const int triangle_wait=em0a_reaction||boss_triangle||
        (triangle&&(opening_triangle||r104||r103_arena_jumper(owner)));
    const int gate_guard=(dependency==0x200&&stamp==0x10&&
        work->room==0x102&&!p[0x1650])||gate_em0a_reaction;
    const int east_em00=!triangle&&work->room==0x102&&
        dependency==0x200&&stamp==3&&p[0x1650]==1;
    if(triangle&&s->state1_phase==19)
        return stop(s,entry,GH_ACTOR_MOTION_UNSUPPORTED);
    const int rear=triangle&&dependency==0x203&&r100_at2_enemy(work,p);
    if(s->state1_phase==6){
        if((boss_triangle||r104)&&!p[0x2f7]&&(p[0x2f6]==2||p[0x2f6]==3))
            s->state1_phase=p[0x2f6]==2?30:triangle?36:35;
        else if(east_em00&&!p[0x2f7]&&(p[0x2f6]==2||p[0x2f6]==3))
            s->state1_phase=p[0x2f6]==2?30:35;
        else if(p[0x2f6]==1)s->state1_phase=11;
        else if(rear&&p[0x2f6]==2)s->state1_phase=30;
        else if(rear&&p[0x2f6]==3)s->state1_phase=33;
        else if(p[0x2f6]!=0||p[0x2f7]||word(p+0x6ec))
            return stop(s,entry,GH_ACTOR_MOTION_UNSUPPORTED);
        else{
            const gh_title_player_view *source=NULL;
            if(gh_hitarea_object_last_hit_actor(body,&source))
                return stop(s,entry,GH_ACTOR_MOTION_UNSUPPORTED);
            if(source){
                float actor[3],target[3],angle;
                if(!position(&owner->object.view,actor)||!position(source,target)||
                    gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                        number(word(p+0x104)),number(0x40490fdb),&angle,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
                if(ordered_abs(angle)>2.3561945f){
                    p[0x2f6]=2;
                    if(!rear&&!east_em00&&!boss_triangle&&!r104){
                        s->state1_phase=19;
                        return stop(s,triangle?entry:0x24d0f0,GH_ACTOR_MOTION_UNSUPPORTED);
                    }
                    s->state1_phase=30;
                    goto rear_reaction;
                }
            }
        }
        if(s->state1_phase==6){
            const gh_object_view *resource=&owner->resource;
            const size_t minimum=devil?0x1d10u:r104?(triangle?0x9e0u:0x9c8u):
                arena_boss?(triangle?0x1258u:0x1248u):
                em0a_reaction?0x9e0u:triangle?0x794u:0x784u;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<minimum||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,entry,GH_ACTOR_MOTION_BOUNDS);
            uint32_t random=0;
            if(!devil&&gh_game_mt_next_0030f348(&init->random,&random))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            /* 24C3C8 selects 2D0/2D8 for dependency 0200. */
            if(devil)s->reaction_resource_offset=0x1d08;
            else if(r104&&!triangle){
                /* 24C3C8 chooses one of the original three frontal hurt rows. */
                s->reaction_resource_offset=0x9b0u+(random%3u)*8u;
            }else if(r104||em0a_reaction){
                /* dep0250 at 43E1F0 and EM0A at 43E0D8 select matching rows. */
                s->reaction_resource_offset=(random&1u)?0x9d0:0x9d8;
            }else if(boss_triangle){
                /* 24DA80 selects the two original EM14/EM15 short-hurt rows. */
                s->reaction_resource_offset=(random&1u)?0x1248:0x1250;
            }else if(triangle) {
                const uint32_t offset=(dependency==0x200||dependency==0x227)?0x2e0u:0x784u;
                s->reaction_resource_offset=offset+(random&1u)*8u;
            }else if(boss_square)s->reaction_resource_offset=(random&1u)?0x1238:0x1240;
            else if(dependency==0x200||r102_east_em27(work,p)||r101_scenery_enemy(work,p))
                s->reaction_resource_offset=(random&1u)?0x2d0:0x2d8;
            else s->reaction_resource_offset=(random&1u)?0x774:0x77c;
            s->state1_phase=r104?29:7;
        }
    }
rear_reaction:
    if(s->state1_phase>=30){
        const gh_actor_motion_status status=rear?advance_triangle_rear_0024e95c(s,p):
            advance_short_rear_reaction(s,p);
        if(status)return status;
    }
    if(s->state1_phase==29){
        /* 24DD9C selects the original second-bank hurt cue before the clip. */
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request request={.model=motion->b.model,.actor=&owner->object.view,
            .bank=1,.program=(int16_t)(half(p+0x424)+0x23u)};
        uint32_t handle;
        const gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->state1_phase=7;
    }
    if(s->state1_phase==7){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=owner->resource.bytes+s->reaction_resource_offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(row),word(row+4),0,2,p[0x17c3]?2:0,
                !r104&&dependency!=0x227&&!r103_arena_em00(owner));
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        s->state1_phase=8;
    }
    if(s->state1_phase==8){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request request={.model=motion->b.model,.actor=&owner->object.view,
            .bank=1,.program=(int16_t)half(p+0x424)};
        uint32_t handle;
        gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->state1_phase=9;
    }
    if(s->state1_phase==9&&(int32_t)word(p+0x5b4)>199){
        if(!gate_guard&&!(triangle&&arena_em0a))return stop(s,0x26f120,GH_ACTOR_MOTION_UNSUPPORTED);
        /* 26F120 sets 150 frames before the original CORE 8C call. */
        put(p+0x5b4,0);
        put(p+0x17cc,0x43160000u);
        s->state1_phase=10;
    }
    if(s->state1_phase==10){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!s->reaction_sound.actor)s->reaction_sound=(gh_motion_sound_request){
            .model=motion->b.model,.actor=&owner->object.view,.bank=0,.program=0x8c};
        uint32_t handle;
        const gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&s->reaction_sound,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        p[0x621]=0;
        s->state1_phase=9;
    }
    if(s->state1_phase==9){
        const gh_title_player_view *source=NULL;
        float target[3];
        if(gh_hitarea_object_last_hit_actor(body,&source)||!position(source?source:s->player,target))
            return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
        unsigned rank=heat_category(s);
        if(!rank)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x5f0,source?10:0);
        memcpy(p+0x5d0,target,12);
        if(triangle){
            uint32_t delay,short_wait;
            gh_game_mt next=init->random;
            if(gh_game_mt_next_0030f348(&next,&delay)||gh_game_mt_next_0030f348(&next,&short_wait))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            init->random=next;
            const int short_delay=short_wait%3u==0&&!init->heat[0x10]&&
                !(number(word(p+0x17cc))>0);
            put_half(p+0x568,short_delay?1:0);
            put(p+0x5f4,short_delay?20:delay%45+60-15*(rank>4?3:rank-1));
            put(p+0x600,0x41700000);
        }else{
            put(p+0x5f4,rank<3?15:10);
            put(p+0x600,0x40a00000);
            put(p+0x608,0x41a00000);
        }
        put(p+0x604,0x40000000);
        p[0x2f6]=1;
        s->state1_phase=11;
    }
    if(s->state1_phase==11){
        uint32_t turn_frames=word(p+0x5f0);
        if(turn_frames){
            float actor[3],target[3],turn;
            memcpy(target,p+0x5d0,12);
            fenv_t env;
            const float yaw=number(word(p+0x104)),actor_rate=number(word(p+0x5a8));
            const float turn_rate=number(word((const uint8_t*)work+0x55c));
            if(!position(&owner->object.view,actor)||!isfinite(actor_rate)||!isfinite(turn_rate)||
                !set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float base_cap=mul(actor_rate,0.3926991f);
            const float cap=triangle?base_cap:mul(base_cap,turn_rate);
            gh_player_target_status result=gh_player_target_angle_002da038(
                s->elf,s->elf_bytes,actor,target,yaw,cap,&turn,&pc);
            float next=result?0:add(yaw,turn);
            if(fesetenv(&env)||result)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f0,turn_frames-1);
            put(p+0x104,bits(next));
        }
        if(!triangle)put_half(p+0x434,half(p+0x434)|8u);
        s->state1_phase=12;
    }
    if(s->state1_phase==12){
        if(!runtime->bindings->canonical_rate||p[0x2f7])
            return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *runtime->bindings->canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state1_phase=ended?13:14;
    }
    if(s->state1_phase==13){
        if(number(word(p+0x17cc))>0){
            if(!gate_guard&&!(triangle&&arena_em0a))return stop(s,0x270c78,GH_ACTOR_MOTION_UNSUPPORTED);
            enemy_decision(p,1,0x0f,0);
            s->state1_phase=triangle&&gate_guard?14:17;
        }else{
            const gh_actor_motion_status status=devil?choose_devil_002705d8(s,p):
                choose_after_substate_002705d8(s,p);
            if(status)return status;
            s->state1_phase=devil&&!p[0x2f7]?14:17;
        }
    }
    const int stunned=triangle&&arena_em0a&&number(word(p+0x17cc))>0;
    if(triangle_wait&&!stunned&&s->state1_phase==14&&!word(p+0x5f4)){
        const float resistance=number(word(p+0x17cc));
        if(!isfinite(resistance)||resistance>0)
            return stop(s,0x270c78,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=20;
    }
    if(triangle&&s->state1_phase>=20&&s->state1_phase<=24){
        gh_actor_motion_status status=triangle_zero_wait(s,p);
        if(status)return status;
    }
    if(s->state1_phase==14&&number(word(p+0x17cc))>0){
        if(!gate_guard&&!stunned)return stop(s,0x24ccf0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(p[0x2f5]!=0x0f){
            const float timer=number(word(p+0x600)),rate=number(word(p+0x5a8));
            fenv_t env;
            if(!isfinite(timer)||!isfinite(rate)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x24cd88,GH_ACTOR_MOTION_UNSUPPORTED);
            const float next=sub(timer,rate);
            if(fesetenv(&env)||!isfinite(next))return stop(s,0x24cd88,GH_ACTOR_MOTION_CALLEE);
            put(p+0x600,bits(next));
            if(next<=0)enemy_decision(p,1,0x0f,0);
        }
        s->state1_phase=25;
    }
    if(s->state1_phase==14||s->state1_phase==25){
        if(gh_player_root_rotation_0014f9f0(&owner->object.view,work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=15;
    }
    if(s->state1_phase==15){
        if(gh_player_root_translation_0014f928(&owner->object.view,work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=16;
    }
    if(boss_square&&s->state1_phase==16&&!word(p+0x5f4)){
        /* 24CF10 tests 26F1D8, then makes one common recovery draw for 214/215. */
        const float resistance=number(word(p+0x17cc)),distance=number(word(p+0x618));
        const float cooldown=number(word(p+0x1740));
        if(!isfinite(resistance)||!isfinite(distance)||!isfinite(cooldown))
            return stop(s,0x24cf10,GH_ACTOR_MOTION_UNSUPPORTED);
        if(resistance<=0){
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            s->state1_phase=(draw%12==0&&distance<9&&!init->heat[0x10]&&
                p[0x17bb]&&cooldown<=0)?27:26;
        }
    }
    if(r104&&!triangle&&s->state1_phase==16&&!word(p+0x5f4)&&
       number(word(p+0x17cc))<=0){
        const gh_actor_motion_status status=r104_frontal_recovery_0024cec8(s,p);
        if(status)return status;
    }
    if(s->state1_phase==27){
        gh_actor_motion_status status=triangle_recovery_00274238(s,p);
        if(status)return status;
    }
    if(s->state1_phase==26){
        int chosen;
        gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
        if(status)return status;
        s->state1_phase=chosen?17:16;
    }
    if(s->state1_phase==16){
        const uint32_t wait=word(p+0x5f4);
        if(!wait&&!triangle_wait&&!boss_square&&!r104)
            return stop(s,triangle?0x24e334:r104?0x24cec8:0x24cf10,GH_ACTOR_MOTION_UNSUPPORTED);
        const float timer=number(word(p+0x604)),rate=number(word(p+0x5a8));
        if(!isfinite(timer)||!isfinite(rate))return stop(s,0x24cfa4,GH_ACTOR_MOTION_UNSUPPORTED);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x24cfa4,GH_ACTOR_MOTION_CALLEE);
        const float remaining=timer>0&&!init->heat[0x10]?sub(timer,rate):timer;
        if(fesetenv(&env)||!isfinite(remaining))return stop(s,0x24cfa4,GH_ACTOR_MOTION_UNSUPPORTED);
        if(wait)put(p+0x5f4,wait-1);
        put(p+0x604,bits(remaining));
        s->state1_phase=17;
    }
    if(rear&&s->state1_phase>=30){
        const gh_actor_motion_status status=advance_triangle_rear_0024e95c(s,p);
        if(status)return status;
    }
    if(s->state1_phase==17){
        if(number(word(p+0x1704))>0)return stop(s,0x26ea50,GH_ACTOR_MOTION_UNSUPPORTED);
        if(s->opacity&&(s->opacity->init!=init||s->opacity->current_player!=s->player))
            return GH_ACTOR_MOTION_ARGUMENT;
        if(gh_game_object_opacity_002a87e8(&owner->object.view,NULL,work,
            s->opacity,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x2a87e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=7;
        s->state1_phase=18;
    }
    if(s->state1_phase==19)return stop(s,0x24d0f0,GH_ACTOR_MOTION_UNSUPPORTED);
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_scenery_reaction_00258d80(gh_enemy202_move *s,uint8_t *p)
{
    const gh_enemy_move_actor *runtime=&s->runtime;
    gh_enemy_entry *entry=runtime->entry;
    gh_title_player_view *actor=&entry->object.view;
    gh_boot_work *work=entry->b.work;
    gh_actor_motion *motion=runtime->motion;
    uint32_t pc=0;
    if(s->state1_phase==6){
        if(p[0x2f6]){
            if(p[0x2f6]==1)s->state1_phase=10;
            else {s->state1_phase=15;s->substate67_stage=5;}
        }else{
            const gh_object_view *resource=&entry->resource;
            const uint32_t impact_power=word(p+0x17b4);
            const int west_em0a=work->room==0x100&&word(p+0x564)==0x20a&&!half(p+0x640)&&
                p[0x1650]==3&&(impact_power==0||impact_power==20);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<(west_em0a?0xb04u:0x608u)||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x258d80,GH_ACTOR_MOTION_BOUNDS);
            uint32_t random;
            if(gh_game_mt_next_0030f348(&entry->b.init->random,&random))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            /* Original dependency020A dispatches to the AF4/AFC resource pairs. */
            s->reaction_resource_offset=west_em0a?((random&1u)?0xaf4:0xafc):((random&1u)?0x5f8:0x600);
            s->state1_phase=7;
        }
    }
    if(s->state1_phase==7){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=entry->resource.bytes+s->reaction_resource_offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(row),word(row+4),0,3,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        s->reaction_sound=(gh_motion_sound_request){.model=motion->b.model,.actor=actor,
            .bank=1,.program=(int16_t)half(p+0x424)};
        s->state1_phase=8;
    }
    if(s->state1_phase==8){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t handle;
        gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&s->reaction_sound,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->state1_phase=9;
    }
    if(s->state1_phase==9){
        const gh_hitarea_object *body=&runtime->hitareas->units[0];
        const gh_title_player_view *source=NULL;
        if(!runtime->hitareas->created[0]||word(p+0x698)||
            (!body->fallback&&(gh_hitarea_object_last_hit_actor(body,&source)||
              (!source&&word(body->bytes+0x34)))))
            return stop(s,0x2590c8,GH_ACTOR_MOTION_UNSUPPORTED);
        /* The native body owner and hit ledger represent +698 and body+34. */
        if(source){
            float target[3];
            if(!position(source,target))return stop(s,0x2590f4,GH_ACTOR_MOTION_UNSUPPORTED);
            memcpy(p+0x5d0,target,12);
        }
        put(p+0x5f0,source?10:0);
        ++p[0x2f6];
        s->state1_phase=10;
    }
    if(s->state1_phase==10){
        const uint32_t turn_frames=word(p+0x5f0);
        if(turn_frames){
            float origin[3],target[3],turn;
            memcpy(target,p+0x5d0,12);
            const float yaw=number(word(p+0x104)),rate=number(word(p+0x5a8));
            const float turn_rate=number(word((const uint8_t *)work+0x55c));
            fenv_t env;
            if(!position(actor,origin)||!isfinite(rate)||!isfinite(turn_rate)||
                !set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float cap=mul(mul(rate,number(0x3ec90fdb)),turn_rate);
            const gh_player_target_status status=gh_player_target_angle_002da038(
                s->elf,s->elf_bytes,origin,target,yaw,cap,&turn,&pc);
            float next=status?0:add(yaw,turn);
            if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f0,turn_frames-1);
            put(p+0x104,bits(next));
        }
        s->state1_phase=11;
    }
    if(s->state1_phase==11){
        if(!runtime->bindings->canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *runtime->bindings->canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state1_phase=ended?12:13;
    }
    if(s->state1_phase==12){
        gh_actor_motion_status status=s->enemy?choose_after_substate_002705d8(s,p):
            choose_devil_002705d8(s,p);
        if(status)return status;
        s->state1_phase=13;
    }
    if(s->state1_phase==13){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=14;
    }
    if(s->state1_phase==14){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=15;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original 253DE8 and 254450 use the same lethal landing selection. */
static gh_actor_motion_status select_death_after_landing(gh_enemy202_move *s,uint8_t *p)
{
    unsigned substate=2;
    if(!(word(p+0x16d4)&0x100000u)){
        if(word(p+0x1644)&0x800000u)substate=3;
        else {
            uint8_t *manager=s->enemy->entry.b.init->enemies;
            const gh_boot_work *work=s->enemy->entry.b.work;
            const int west=r100_at2_formation(work,p)||r100_at2_enemy(work,p);
            if((word(p+0x16d4)&0x1000000u)||(word(p+0x16d0)&0x200u)||(manager[0x53e]&&!west))
                return stop(s,0x25fe30,GH_ACTOR_MOTION_UNSUPPORTED);
            /* 294998 calls 2949C8 before the ordinary no-transform bit test. */
            if(manager[0x53e])--manager[0x53e];
            substate=1;
        }
    }
    enemy_decision(p,2,substate,0);
    return GH_ACTOR_MOTION_OK;
}

/* 274E98 and 274150 both return zero in the rooms using this profile. */
static gh_actor_motion_status choose_after_knockdown_00274fe8(gh_enemy202_move *s,uint8_t *p)
{
    const float override=number(word(p+0x1734)),timer=number(word(p+0x17cc));
    const float distance=number(word(p+0x618));
    if(!isfinite(override)||!isfinite(timer)||!isfinite(distance))
        return stop(s,0x274fe8,GH_ACTOR_MOTION_UNSUPPORTED);
    int flip=override>0;
    if(!flip&&p[0x17c1]&&timer<=0&&distance<6.25f){
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&s->enemy->entry.b.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        flip=(draw&3u)!=0;
    }
    enemy_decision(p,1,flip?0x14:0xd,0);
    if(flip)put(p+0x54c,0x40a00000);
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_knockdown_reaction_00253de8(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    if(s->state1_phase==6){
        const int arena_em0a=r103_arena_em0a(&enemy->entry);
        const int arena=r103_arena_em00(&enemy->entry)||r103_arena_em03(&enemy->entry)||arena_em0a;
        if((positive_hp(p)&&!arena)||p[0x2f6]>1||p[0x2f7]>1||p[0x17c3]||
           word(p+0x6ec)||gh_enemy_timers_weapon_kind(&enemy->timers))
            return stop(s,0x253de8,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)|0x1000u);
        if(p[0x2f6])s->state1_phase=10;
        else {
            const gh_title_player_view *source=NULL;
            if(gh_hitarea_object_last_hit_actor(&enemy->hitareas.units[0],&source))
                return stop(s,0x253e8c,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x1710,0);
            if(source){
                float self[3],target[3],turn,angle;
                const float yaw=number(word(p+0x104));
                if(!position(actor,self)||!position(source,target)||
                   gh_player_target_angle_002da038(s->elf,s->elf_bytes,self,target,yaw,
                     number(0x40490fdb),&turn,&pc))
                    return stop(s,pc?pc:0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
                fenv_t fp;
                if(!set_round(&fp,FE_TOWARDZERO))
                    return stop(s,0x2a7498,GH_ACTOR_MOTION_CALLEE);
                angle=add(yaw,turn);
                if(fesetenv(&fp)||gh_id_angle_reference_001f7d48(angle,&angle))
                    return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
                put(p+0x104,bits(angle));
            }
            s->reaction_resource_offset=arena_em0a?(p[0x2f7]?0xac4:0xabc):
                (p[0x2f7]?0x5e8:0x5e0);
            s->state1_phase=7;
        }
    }
    if(s->state1_phase==7){
        const gh_object_view *resource=&enemy->entry.resource;
        if(!resource->bytes||!resource->file_extent_known||
           resource->file_extent<s->reaction_resource_offset+8u||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x253de8,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=resource->bytes+s->reaction_resource_offset;
            const gh_actor_motion_status result=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(row),word(row+4),0,5,0,1);
            if(result)return stop(s,call->frontier?call->frontier:0x2a8578,result);
        }
        const gh_actor_motion_status result=gh_actor_motion_call_step_002a8578(call);
        if(result)return stop(s,call->frontier?call->frontier:0x2a8578,result);
        s->state1_phase=8;
    }
    if(s->state1_phase==8){
        gh_effect_create_bindings b=s->impact_bindings;
        if(!b.storage.effects||b.work!=enemy->entry.b.work||b.random!=&enemy->entry.b.init->random)
            return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
        b.model=motion->b.model;
        gh_effect_create_call *effect=&s->death_effect_create;
        if(!effect->active){
            put(p+0x574,2);
            put_half(p+0x570,0);
            if(gh_effect_create_enemy_knockdown_002a8608(effect,actor,p[0x2f7]?0x30:0x2f))
                return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(gh_game_effect_create_00306528(effect,&b)||!effect->complete)
            return stop(s,effect->frontier?effect->frontier:0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=9;
    }
    if(s->state1_phase==9){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request request={.model=motion->b.model,.actor=actor,
            .bank=1,.program=(int16_t)half(p+0x424)};
        uint32_t handle;
        const gh_actor_motion_status result=motion->b.sound(motion->b.sound_context,&request,&handle,&pc);
        if(result)return stop(s,pc?pc:0x2cba48,result);
        put(p+0x5f0,1);
        p[0x2f6]=1;
        s->state1_phase=10;
    }
    if(s->state1_phase==10){
        int ended;
        const gh_actor_motion_status result=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(result)return stop(s,pc?pc:0x157c18,result);
        s->state1_phase=!ended?11:positive_hp(p)?15:13;
    }
    if(s->state1_phase==15){
        const gh_actor_motion_status status=choose_after_knockdown_00274fe8(s,p);
        if(status)return status;
        s->state1_phase=11;
    }
    if(s->state1_phase==11){
        if(gh_player_root_rotation_0014f9f0(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=12;
    }
    if(s->state1_phase==12){
        if(gh_player_root_translation_0014f928(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=(half(p+0x3ac)&0x10u)&&!positive_hp(p)?13:16;
    }
    if(s->state1_phase==16){
        const float timer=number(word(p+0x17cc));
        if(!isfinite(timer))return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
        if((half(p+0x3ac)&8u)&&word(p+0x5f0)&&timer<=0&&positive_hp(p)){
            put(p+0x5f0,0);
            if(heat_category(s)>=2&&(p[0x17b8]==1||p[0x17b8]==2)){
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                const int flip=p[0x17b8]==1?(draw&1u)!=0:(int32_t)draw%3!=0;
                if(flip){
                    enemy_decision(p,1,0x14,0);
                    put(p+0x54c,0x40a00000);
                }
            }
        }
        s->state1_phase=14;
    }
    if(s->state1_phase==13){
        const gh_actor_motion_status result=select_death_after_landing(s,p);
        if(result)return result;
        s->state1_phase=14;
    }
    s->substate67_stage=s->substate67_stage?s->substate67_stage:7;
    return advance_state0_tail(s,p);
}

/* Original virtual AC damage for the paired counter. It uses the live
 * player borrow and a direct amount 50, with no hit-area ledger input. */
static gh_actor_motion_status counter_damage_0028fb88(gh_enemy202_move *s,uint8_t *p)
{
    const gh_title_player_view *player=s->player;
    const gh_boot_work *work=s->enemy->b.entry.work;
    const uint32_t vtable=s->enemy->entry.object.view.original_vtable214;
    const uint8_t *method=elf_span(s->elf,s->elf_bytes,vtable+0xa8,8);
    if(vtable!=0x4479c8||word(p+0x214)!=vtable||!method||word(method)!=0||
        word(method+4)!=0x28fb88||!player||!player->bytes||player->size<0x600||
        half(player->bytes+0x2fe)!=0x100)
        return stop(s,0x28fb88,GH_ACTOR_MOTION_UNSUPPORTED);
    static const float gains[]={1,1,1.3f,1.6f,2,2.3f,2.6f,3,3.3f};
    const unsigned stage=((const uint8_t *)work)[0x5b1];
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x28fcf8,GH_ACTOR_MOTION_CALLEE);
    float multiplier=stage<sizeof(gains)/sizeof(*gains)?gains[stage]:1;
    if(word(player->bytes+0x5fc))multiplier=mul(multiplier,1.5f);
    if(s->enemy->b.entry.init->heat[0x10])
        multiplier=p[0x622]==1?mul(multiplier,1.5f):add(multiplier,multiplier);
    const float adjusted=add(mul(50,multiplier),0.5f);
    if(fesetenv(&env)||!isfinite(adjusted)||adjusted<0||adjusted>32000)
        return stop(s,0x28fd14,GH_ACTOR_MOTION_UNSUPPORTED);
    const int hp=(int16_t)(uint16_t)(half(p+0x54a)-(unsigned)adjusted);
    put_half(p+0x54a,(uint16_t)(hp>0?hp:0));
    return GH_ACTOR_MOTION_OK;
}

/* The counter uses the existing Give unit registered at original +690. */
static gh_actor_motion_status body_give_00262750(gh_enemy202_move *s, uint8_t *p,unsigned mode)
{
    gh_enemy_hitareas *areas=&s->enemy->hitareas;
    const gh_title_player_view *actor=&s->enemy->entry.object.view;
    const gh_hitarea_object *give=&areas->units[5];
    gh_title_player_view *owner=NULL;
    const uint8_t *record;
    gh_collision_pair_shape shape;
    float position_now[3],delta[3];
    const float offset[]={0,mode==2?-2.0f:mode==4?-0.5f:0,0};
    const float size[]={mode==2?0.3f:mode==4?1.0f:2.0f,
                        mode==2?1.0f:mode==4?1.5f:2.0f,
                        mode==2?0.3f:mode==4?1.0f:2.0f};
    uint32_t pc=0;
    if(!areas->created[5]||gh_hitarea_object_actor(give,&owner)||!owner||
       owner->bytes!=actor->bytes||give->size<0x50||!position(actor,position_now))
        return stop(s,0x262750,GH_ACTOR_MOTION_UNSUPPORTED);
    for(unsigned i=0;i<3;i++){
        delta[i]=sub(position_now[i],number(word(p+0x490+4*i)));
        if(!isfinite(delta[i]))return stop(s,0x1fd308,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    if(!give->fallback){
        if(gh_hitarea_node_at(give,0,&record,&shape))
            return stop(s,0x1fd820,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint8_t *next;
        gh_collision_pair_shape other;
        if(gh_hitarea_node_at(give,1,&next,&other)!=GH_HITAREA_BOUNDS)
            return stop(s,0x1fd820,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    const int r104_charge=mode==1&&s->pending_ai_pc==0x224090&&
        gh_enemy203_is_r104_entry(&s->enemy->entry.request,s->enemy->entry.b.work->room);
    put_half(give->bytes+0x46,r104_charge?0x2f:0x2e);
    put(give->bytes+0x4c,r104_charge?20:mode==2||mode==4?1:10);
    if(gh_hitarea_object_set_active_001fd668(give,1,&pc))
        return stop(s,pc?pc:0x1fd668,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!give->fallback){
        if(gh_collision_pair_shape_offset_002bb258(&shape,offset,&pc))
            return stop(s,pc?pc:0x1fd888,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_collision_pair_box_size_002baaa8(&shape,size,&pc))
            return stop(s,pc?pc:0x1fd820,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    memcpy(give->bytes+0x10,delta,12);
    put(give->bytes+0x40,UINT32_MAX);
    put_half(give->bytes+0x44,0);
    return GH_ACTOR_MOTION_OK;
}
static gh_actor_motion_status advance_em00_escape_counter_0025bcc0(gh_enemy202_move *s,uint8_t *p)
{
    enum { ENTER=230, BIND, FOLLOW, HIT, HEAT, SAMPLE, CHOOSE, ROTATE, TRANSLATE, TAIL, GIVE };
    gh_enemy_actor *enemy=s->enemy;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->b.entry.work;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    const int opening=work->room==0x100&&word(p+0x564)==0x200&&half(p+0x640)==20;
    if((!opening&&!r103_arena_em00(&enemy->entry)&&!r102_grab_escape_counter(work,p))||
       !s->player||!s->player->bytes||s->player->size<0x55c)
        return stop(s,0x25bcc0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage)s->substate67_stage=ENTER;
    if(s->substate67_stage==ENTER){
        if(!enemy->b.solids||gh_collision_solid_set_enabled_001351d8(enemy->b.solids,actor,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x54c,0x40400000);
        if(p[0x2f6]>1)return stop(s,0x25bfe4,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=p[0x2f6]?FOLLOW:BIND;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            if(word(p+0x6ec)||gh_enemy_timers_weapon_kind(&enemy->timers))
                return stop(s,0x26b1e8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x538||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x25bd38,GH_ACTOR_MOTION_BOUNDS);
            const uint16_t hp=half(p+0x54a);
            gh_actor_motion_status status=counter_damage_0028fb88(s,p);
            if(status)return status;
            const int lethal=!positive_hp(p);
            put_half(p+0x54a,hp);
            const unsigned row=lethal?0x530:0x528;
            status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),0,0,0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
            put_half(p+0x568,(uint16_t)lethal);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5f0,42);put(p+0x5f4,1);
        put_half(p+0x56e,30);put_half(p+0x56a,15);
        p[0x2f6]=1;
        put(p+0x16ec,0);
        s->substate67_stage=FOLLOW;
    }
    if(s->substate67_stage==FOLLOW){
        const unsigned frames=half(p+0x56e);
        const int grounded=(half(p+0x3ac)&8u)==0;
        if(grounded||frames){
            float next[3],player[3],delta[3];
            if(!position(actor,next)||!position(s->player,player))
                return stop(s,0x25be48,GH_ACTOR_MOTION_UNSUPPORTED);
            memcpy(delta,s->player->bytes+0x550,12);
            if(grounded)next[1]=sub(player[1],mul(sub(number(word(p+0x114)),1),0.1f));
            for(unsigned i=0;i<3;i++){
                if(frames)next[i]=add(next[i],delta[i]);
                if(!isfinite(next[i]))return stop(s,0x25be48,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            if(grounded)put(p+0x16d0,word(p+0x16d0)|0x10000u);
            if(frames)put_half(p+0x56e,(uint16_t)(frames-1));
            memcpy(actor->alias_f0,next,12);
        }
        s->substate67_stage=(half(p+0x3ac)&2u)&&word(p+0x5f4)?HIT:SAMPLE;
    }
    if(s->substate67_stage==HIT){
        const gh_actor_motion_status status=counter_damage_0028fb88(s,p);
        if(status)return status;
        put(p+0x5f4,0);
        if(half(p+0x568))put_half(p+0x54a,0);
        s->substate67_stage=HEAT;
    }
    if(s->substate67_stage==HEAT){
        const gh_actor_motion_status status=gh_enemy_counter_heat_002a9978(enemy,&pc);
        if(status)return stop(s,pc?pc:0x2a9978,status);
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?CHOOSE:ROTATE;
    }
    if(s->substate67_stage==CHOOSE){
        if(!positive_hp(p)){
            if(word(p+0x1644)&0x800000u){
                enemy_decision(p,2,3,0);
                s->substate67_stage=5;
                return advance_state0_tail(s,p);
            }
            /* Ordinary 25FE30 returns before any Devil spawn or RNG draw. */
            if((word(p+0x16d4)&0x1000000u)||(word(p+0x16d0)&0x200u)||
               enemy->entry.b.init->enemies[0x53e])
                return stop(s,0x25fe30,GH_ACTOR_MOTION_UNSUPPORTED);
            enemy_decision(p,2,2,0);
            s->substate67_stage=ROTATE;
        }else{
            const gh_actor_motion_status status=choose_after_knockdown_00274fe8(s,p);
            if(status)return status;
            s->substate67_stage=ROTATE;
        }
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TAIL;
    }
    if(s->substate67_stage==TAIL){
        const int16_t remaining=(int16_t)half(p+0x56a);
        if(remaining)put_half(p+0x56a,(uint16_t)(remaining-1));
        else {
            if(p[0x186b]>4&&p[0x2f7]==1)return stop(s,0x25bf80,GH_ACTOR_MOTION_UNSUPPORTED);
            put_half(p+0x56a,999);
        }
        s->substate67_stage=(half(p+0x3ac)&1u)?GIVE:5;
    }
    if(s->substate67_stage==GIVE){
        const gh_actor_motion_status status=body_give_00262750(s,p,0);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_em00_getup_00257948(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=230, WAKE, SAMPLE, CHOOSE, ROTATE, TRANSLATE, ATTACK };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    gh_boot_work *work=enemy->b.entry.work;
    uint32_t pc=0;
    const int opening=work->room==0x100&&word(p+0x564)==0x200&&half(p+0x640)==20;
    const int em0a=(r102_gate_em0a(work,p)&&p[0x2f6]<=1&&!p[0x2f7])||
        r103_arena_em0a(&enemy->entry);
    const int boss_getup=r103_heavy_recoil_getup(enemy,p);
    if(!opening&&!r103_arena_em00(&enemy->entry)&&!r102_em00_stamp11(work,p)&&!r102_em00_stamp7_grab(work,p)&&!em0a&&!boss_getup)
        return stop(s,0x257948,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        if(p[0x2f6]>1)s->substate67_stage=ATTACK;
        else s->substate67_stage=p[0x2f6]?SAMPLE:BIND;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            if(word(p+0x6ec)||gh_enemy_timers_weapon_kind(&enemy->timers))
                return stop(s,0x257d34,GH_ACTOR_MOTION_UNSUPPORTED);
            const float timer=number(word(p+0x17cc));
            if(!isfinite(timer))return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
            unsigned row=!(half(p+0x3ac)&0x200u)?0x360:timer>0?0x358:0x368;
            if(em0a){
                row=0xb0c;
                if(half(p+0x3ac)&0x200u){
                    uint32_t random;
                    if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&random))
                        return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                    row=random&1u?0xb04:0xb14;
                }
            }
            const gh_object_view *res=&enemy->entry.resource;
            if(!res->bytes||!res->file_extent_known||res->file_extent<row+8||
               res->file_extent>res->size||motion->b.resource!=res->bytes)
                return stop(s,0x257d34,GH_ACTOR_MOTION_UNSUPPORTED);
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(res->bytes+row),word(res->bytes+row+4),0,10,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        s->substate67_stage=WAKE;
    }
    if(s->substate67_stage==WAKE){
        if((int32_t)word(p+0x5b4)>199)return stop(s,0x26f120,GH_ACTOR_MOTION_UNSUPPORTED);
        p[0x2f6]=1;
        s->substate67_stage=SAMPLE;
    }
    if(s->substate67_stage==SAMPLE){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?CHOOSE:ROTATE;
    }
    if(s->substate67_stage==CHOOSE){
        if(number(word(p+0x17cc))>0)return stop(s,0x270c78,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->substate67_stage=ROTATE;
    }
    if(s->substate67_stage==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=TRANSLATE;
    }
    if(s->substate67_stage==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ATTACK;
    }
    if(s->substate67_stage==ATTACK){
        if(half(p+0x3ac)&0x10u){
            int chosen;
            const gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
            if(status)return status;
        }
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}
/* EM00's original 2580B8 stun motions, followed by the ordinary 258834 recovery. */
static gh_actor_motion_status advance_gate_guard_stun_002580b8(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=6, SAMPLE, ROTATE, TRANSLATE, TAIL, CHOOSE, RETURNED };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_boot_work *work=enemy->b.entry.work;
    gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    if(s->state1_phase==BIND){
        const unsigned phase=p[0x2f6];
        if(p[0x2f7]||(phase>1&&phase!=8&&phase!=9))
            return stop(s,0x2581ac,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!(phase&1u)){
            /* 258114 preserves phase 0 for dependency 0200. The 2581AC
             * table then selects 2581D0 and 2581FC, with row 5F0. */
            const size_t row=r103_arena_em0a(&enemy->entry)?
                (phase==8?0x828u:0xadcu):(phase==8?0x98u:0x5f0u);
            const gh_object_view *res=&enemy->entry.resource;
            if(!res->bytes||!res->file_extent_known||res->file_extent<row+8||
               res->file_extent>res->size||motion->b.resource!=res->bytes)
                return stop(s,phase==8?0x258a10:0x258288,GH_ACTOR_MOTION_BOUNDS);
            if(word(p+0x6ec)||gh_enemy_timers_weapon_kind(&enemy->timers))
                return stop(s,0x26b1e8,GH_ACTOR_MOTION_UNSUPPORTED);
            gh_actor_motion_call *call=&s->substate67_motion;
            gh_actor_motion_status status;
            if(!call->active){
                status=gh_actor_motion_call_open_parameters_002a8578(call,motion,&motion->b,
                    word(res->bytes+row),word(res->bytes+row+4),0,phase==8?10:3,p[0x17c3]?2:0,1);
                if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
            }
            status=gh_actor_motion_call_step_002a8578(call);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
            if(phase==8)put(p+0x600,0x41700000u);
            else{
                put(p+0x5f0,1);
                put(p+0x16ec,0);
            }
            ++p[0x2f6];
        }
        s->state1_phase=SAMPLE;
    }
    if(s->state1_phase==SAMPLE){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        (void)ended;
        s->state1_phase=ROTATE;
    }
    if(s->state1_phase==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=TRANSLATE;
    }
    if(s->state1_phase==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=TAIL;
    }
    if(s->state1_phase==TAIL){
        if(p[0x2f6]==1&&number(word(p+0x17cc))<=0.0f)p[0x2f6]=8;
        if(p[0x2f6]==9){
            const float remaining=number(word(p+0x600)),rate=number(word(p+0x5a8));
            fenv_t env;
            if(!isfinite(remaining)||!isfinite(rate)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x258a7c,GH_ACTOR_MOTION_UNSUPPORTED);
            const float next=sub(remaining,rate);
            if(fesetenv(&env))return stop(s,0x258a7c,GH_ACTOR_MOTION_CALLEE);
            put(p+0x600,bits(next));
            if(next<=0){
                put(p+0x16dc,0);
                s->state1_phase=CHOOSE;
            }
        }
        if(s->state1_phase==TAIL)s->state1_phase=RETURNED;
    }
    if(s->state1_phase==CHOOSE){
        const gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->state1_phase=RETURNED;
    }
    if(s->state1_phase==RETURNED&&s->substate67_stage<5){
        if(number(word(p+0x1734))>0)enemy_decision(p,0,0x89,0);
        s->substate67_stage=5;
        s->state1_phase=RETURNED;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status enemy_recoil_screen_00292e80(gh_enemy202_move *s)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_game_init_state *init=enemy->entry.b.init;
    const gh_boot_work *work=enemy->entry.b.work;
    gh_effect_create_call *effect=&s->blowaway_effect;
    if((word((const uint8_t *)work+0x590)&0x800u)||init->enemies[0x538])
        return GH_ACTOR_MOTION_OK;
    const gh_effect_create_request request={.bank=0,.index=0xf,.mode=-1,.selector=-1};
    if(!effect->active&&gh_effect_create_begin(effect,&request))
        return stop(s,0x306528,GH_ACTOR_MOTION_ARGUMENT);
    gh_effect_status result=gh_game_effect_create_00306528(effect,&s->impact_bindings);
    if(result||!effect->complete)
        return stop(s,effect->frontier?effect->frontier:0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
    init->enemies[0x538]=2;
    *effect=(gh_effect_create_call){0};
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status enemy_recoil_impact_002717d0(gh_enemy202_move *s)
{
    gh_effect_create_call *effect=&s->blowaway_effect;
    if(!effect->active){
        uint32_t pc=0,position_bits[3];
        float xyz[4];
        gh_actor_motion_status result=gh_enemy202_impact_position_00271518(s->enemy,xyz,&pc);
        if(result)return stop(s,pc?pc:0x271518,result);
        memcpy(position_bits,xyz,sizeof(position_bits));
        if(gh_effect_create_enemy_blowaway_begin_00306838(effect,position_bits))
            return stop(s,0x306838,GH_ACTOR_MOTION_ARGUMENT);
    }
    gh_effect_status result=gh_game_effect_create_00306528(effect,&s->impact_bindings);
    return result||!effect->complete?
        stop(s,effect->frontier?effect->frontier:0x306838,GH_ACTOR_MOTION_UNSUPPORTED):
        GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_em0a_blowaway_00254450(gh_enemy202_move *s,uint8_t *p)
{
    enum { START=6, SCREEN_EFFECT, IMPACT, OPEN_MOTION, VOICE, LAUNCH,
           AIRBORNE, SAMPLE_AIRBORNE, ROTATE_AIRBORNE, TRANSLATE_AIRBORNE, FINISH_AIRBORNE,
           FLOOR_SELECT, FLOOR_BIND, FLOOR_SAMPLE, FLOOR_ENDED, FLOOR_ROTATE, FLOOR_TRANSLATE, FLOOR_DEATH, FLOOR_DONE };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=s->runtime.motion;
    const gh_boot_work *work=enemy->entry.b.work;
    gh_game_init_state *init=enemy->entry.b.init;
    uint32_t pc=0;
    gh_title_player_view *actor=&enemy->entry.object.view;
    if(s->state1_phase==START){
        if(p[0x2f7]!=2||p[0x2f6]>5)return stop(s,0x254ca8,GH_ACTOR_MOTION_UNSUPPORTED);
        switch(p[0x2f6]){
        case 1: s->state1_phase=AIRBORNE; break;
        case 2: case 4: s->state1_phase=FLOOR_SELECT; break;
        case 3: case 5: s->state1_phase=FLOOR_SAMPLE; break;
        }
    }
    if(s->state1_phase==START){
        put(p+0x1710,0);
        put(p+0x5c4,0x3e6f9db2u);
        s->state1_phase=SCREEN_EFFECT;
    }
    if(s->state1_phase==SCREEN_EFFECT){
        const gh_actor_motion_status result=enemy_recoil_screen_00292e80(s);
        if(result)return result;
        s->state1_phase=IMPACT;
    }
    if(s->state1_phase==IMPACT){
        const gh_actor_motion_status result=enemy_recoil_impact_002717d0(s);
        if(result)return result;
        if(p[0x1867]<4)p[0x1867]++;
        if(!init->heat[0x10])put(p+0x5c4,bits(mul(number(word(p+0x5c4)),.7f)));
        if(!word(p+0x6ec)&&!word(p+0x6f0)&&!word(p+0x700)&&!word(p+0x704)){
            uint32_t random;
            if(gh_game_mt_next_0030f348(&init->random,&random))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            p[0x17c3]=(uint8_t)(random&1u);
        }
        put(p+0x5f4,0);
        s->state1_phase=OPEN_MOTION;
    }
    if(s->state1_phase==OPEN_MOTION){
        gh_actor_motion_call *call=&s->substate67_motion;
        const gh_object_view *resource=&enemy->entry.resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0xa70||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x25482c,GH_ACTOR_MOTION_BOUNDS);
        if(!call->active){
            const uint8_t *row=resource->bytes+0xa68;
            gh_actor_motion_status result=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(row),word(row+4),0,3,p[0x17c3]?2:0,1);
            if(result)return stop(s,call->frontier?call->frontier:0x2a8578,result);
        }
        gh_actor_motion_status result=gh_actor_motion_call_step_002a8578(call);
        if(result)return stop(s,call->frontier?call->frontier:0x2a8578,result);
        s->state1_phase=VOICE;
    }
    if(s->state1_phase==VOICE){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request cue={.model=motion->b.model,.actor=&enemy->entry.object.view,
            .bank=1,.program=(int16_t)half(p+0x424)};
        uint32_t handle;
        gh_actor_motion_status result=motion->b.sound(motion->b.sound_context,&cue,&handle,&pc);
        if(result)return stop(s,pc?pc:0x2cba48,result);
        s->state1_phase=LAUNCH;
    }
    if(s->state1_phase==LAUNCH){
        const gh_title_player_view *source=NULL;
        const gh_hitarea_object *body=&enemy->hitareas.units[0];
        float target[3];
        if(gh_hitarea_object_last_hit_actor(body,&source)||
           (source&&!position(source,target)))return stop(s,0x254964,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x5f0,source?10u:0u);
        if(source)memcpy(p+0x5d0,target,sizeof(target));
        if((int32_t)word(p+0x5b4)>199)return stop(s,0x26f120,GH_ACTOR_MOTION_UNSUPPORTED);
        p[0x2f6]=1;
        s->state1_phase=AIRBORNE;
    }
    if(s->state1_phase==AIRBORNE){
        float from[3],target[3],turn,point[4]={0};
        const float old_speed=number(word(p+0x5c4)),actor_rate=number(word(p+0x5a8));
        const float turn_rate=number(word((const uint8_t *)work+0x55c));
        if(!position(actor,from)||!isfinite(old_speed)||!isfinite(actor_rate)||!isfinite(turn_rate))
            return stop(s,0x2549b8,GH_ACTOR_MOTION_UNSUPPORTED);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x2549b8,GH_ACTOR_MOTION_CALLEE);
        const uint32_t turn_frames=word(p+0x5f0);
        float yaw=number(word(p+0x104));
        if(turn_frames){
            memcpy(target,p+0x5d0,12);
            gh_player_target_status result=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
                from,target,yaw,mul(mul(actor_rate,0.3926991f),turn_rate),&turn,&pc);
            if(result){(void)fesetenv(&env);return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);}
            yaw=add(yaw,turn);
        }
        const float speed=sub(old_speed,mul(actor_rate,old_speed<=0?0.0078f:0.013f));
        float y=add(from[1],mul(old_speed,actor_rate));
        const int down=speed<=0;
        gh_scene_ray_request ray={
            .from={from[0],down?add(from[1],.5f):from[1],from[2],1},
            .to={from[0],add(y,down?-10.0f:10.0f),from[2],1},
            .mask=down?2u:7u,.segmented=1,.hit_position=point};
        if(fesetenv(&env))return stop(s,0x2549b8,GH_ACTOR_MOTION_CALLEE);
        int hit=0;
        if(!enemy->b.actors||gh_scene_ray_query_00131e10(enemy->b.actors,&ray,&hit,&pc))
            return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
        if(turn_frames&&gh_id_angle_reference_001f7d48(yaw,&yaw))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x254c70,GH_ACTOR_MOTION_CALLEE);
        const int landed=hit==1&&down&&y<=add(point[1],.01f);
        const int ceiling=hit==1&&!down&&sub(point[1],1.5f)<y;
        if(landed)y=point[1];
        if(ceiling)y=sub(point[1],1.5f);
        if(fesetenv(&env))return stop(s,0x254c70,GH_ACTOR_MOTION_CALLEE);
        unsigned next=landed?2u:1u;
        if(landed&&positive_hp(p)&&number(word(p+0x17cc))<=0){
            const unsigned rank=heat_category(s);
            if(rank>=2){
                uint32_t random;
                if(gh_game_mt_next_0030f348(&init->random,&random))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(!(random&(rank==2?7u:rank==5?1u:3u)))next=4;
            }
        }
        put(p+0x16d0,word(p+0x16d0)|0x10000u);
        if(turn_frames){put(p+0x104,bits(yaw));put(p+0x5f0,turn_frames-1);}
        memcpy(actor->alias_f0+4,&y,4);
        put(p+0x5c4,ceiling?0:bits(speed));
        p[0x2f6]=(uint8_t)next;
        if(!word(p+0x5f4))put_half(p+0x434,half(p+0x434)|8u);
        s->state1_phase=SAMPLE_AIRBORNE;
    }
    if(s->state1_phase==SAMPLE_AIRBORNE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_ARGUMENT);
        int ended;
        gh_actor_motion_status result=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(result)return stop(s,pc?pc:0x157c18,result);
        s->state1_phase=ROTATE_AIRBORNE;
    }
    if(s->state1_phase==ROTATE_AIRBORNE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=TRANSLATE_AIRBORNE;
    }
    if(s->state1_phase==TRANSLATE_AIRBORNE){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=5;
        s->state1_phase=FINISH_AIRBORNE;
    }
    if(s->state1_phase==FLOOR_SELECT){
        if(!word(p+0x6ec)&&!word(p+0x6f0)&&!word(p+0x700)&&!word(p+0x704)){
            uint32_t random;
            if(gh_game_mt_next_0030f348(&init->random,&random))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            p[0x17c3]=(uint8_t)(random&1u);
        }
        s->reaction_resource_offset=p[0x2f6]==2?0xa70:0xa78;
        if(p[0x2f6]==2)put(p+0x600,0x3fc00000u);
        s->state1_phase=FLOOR_BIND;
    }
    if(s->state1_phase==FLOOR_BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        const gh_object_view *resource=&enemy->entry.resource;
        const size_t row=s->reaction_resource_offset;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x254e34,GH_ACTOR_MOTION_BOUNDS);
        if(!call->active){
            gh_actor_motion_status result=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),0,3,p[0x17c3]?2:0,1);
            if(result)return stop(s,call->frontier?call->frontier:0x2a8578,result);
        }
        gh_actor_motion_status result=gh_actor_motion_call_step_002a8578(call);
        if(result)return stop(s,call->frontier?call->frontier:0x2a8578,result);
        if(p[0x2f6]==4)put(p+0x54c,0x41200000u);
        p[0x2f6]++;
        s->state1_phase=FLOOR_SAMPLE;
    }
    if(s->state1_phase==FLOOR_SAMPLE){
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_ARGUMENT);
        int ended;
        gh_actor_motion_status result=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(result)return stop(s,pc?pc:0x157c18,result);
        s->state1_phase=ended?FLOOR_ENDED:FLOOR_ROTATE;
    }
    if(s->state1_phase==FLOOR_ENDED){
        if(p[0x2f6]==3&&!positive_hp(p))s->state1_phase=FLOOR_DEATH;
        else if(p[0x2f6]==3){
            const float armor=number(word(p+0x1734)),resistance=number(word(p+0x17cc)),dist=number(word(p+0x618));
            if(!isfinite(armor)||!isfinite(resistance)||!isfinite(dist))return stop(s,0x274fe8,GH_ACTOR_MOTION_UNSUPPORTED);
            int counter=armor>0;
            if(!counter&&p[0x17c1]&&resistance<=0&&dist<6.25f){
                uint32_t random;
                if(gh_game_mt_next_0030f348(&init->random,&random))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                counter=(random&3u)!=0;
            }
            enemy_decision(p,1,counter?0x14:0xd,0);
            if(counter)put(p+0x54c,0x40a00000u);
            s->substate67_stage=5;
            s->state1_phase=FLOOR_DONE;
        }else{
            const gh_actor_motion_status result=choose_after_substate_002705d8(s,p);
            if(result)return result;
            s->state1_phase=FLOOR_ROTATE;
        }
    }
    if(s->state1_phase==FLOOR_ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=FLOOR_TRANSLATE;
    }
    if(s->state1_phase==FLOOR_TRANSLATE){
        const float scale=p[0x2f6]==3?number(word(p+0x600)):1.0f;
        if(gh_player_root_translation_scaled_0014f928(actor,work,scale,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=p[0x2f6]==3&&!positive_hp(p)&&(half(p+0x3ac)&0x10)?FLOOR_DEATH:FLOOR_DONE;
        s->substate67_stage=5;
    }
    if(s->state1_phase==FLOOR_DEATH){
        const gh_actor_motion_status result=select_death_after_landing(s,p);
        if(result)return result;
        s->substate67_stage=5;
        s->state1_phase=FLOOR_DONE;
    }
    return advance_state0_tail(s,p);
}

/* The room-0103 recoil path uses the two ordinary 271EB0 wall rays.
 * A hit still needs the authored wall-specific reaction. */
static gh_actor_motion_status enemy_recoil_clear_wall_00271eb0(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    float current[3],previous[3],yaw,sine,cosine;
    uint32_t pc=0;
    if(!enemy->b.actors||!position(&enemy->entry.object.view,current))
        return stop(s,0x271eb0,GH_ACTOR_MOTION_UNSUPPORTED);
    memcpy(previous,p+0x490,12);
    if(gh_game_angle_001f8180(sub(current[0],previous[0]),sub(current[2],previous[2]),
          &yaw,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc)||
       gh_sdk_rotation_sincos_reference_0030a5d0(yaw,&sine,&cosine,GH_STATIC_BINARY32_REFERENCE,&pc))
        return stop(s,pc?pc:0x271eb0,GH_ACTOR_MOTION_UNSUPPORTED);
    const float reach=mul(.7f,number(word(p+0x118)));
    gh_scene_ray_report report={0};
    gh_scene_ray_request ray={.mask=1,.flags=0x124,.segmented=1,.report_limit=1,.native_reports=&report};
    memcpy(ray.from,previous,12);
    ray.from[1]=add(previous[1],.8f);ray.from[3]=ray.to[3]=1;
    ray.to[1]=add(current[1],.8f);
    for(unsigned i=0;i<2;++i){
        const float side=i?-.3f:.3f;
        ray.to[0]=add(current[0],add(mul(side,cosine),mul(reach,sine)));
        ray.to[2]=add(current[2],sub(mul(reach,cosine),mul(side,sine)));
        int hit;
        if(gh_scene_ray_query_00131e10(enemy->b.actors,&ray,&hit,&pc))
            return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
        if(hit){
            if(!r103_arena_boss(&enemy->entry)||!report.attributes||report.attribute_bytes<8||
               (word(report.attributes+4)&2u))
                return stop(s,0x2722a0,GH_ACTOR_MOTION_UNSUPPORTED);
            float facing;
            if(gh_id_angle_reference_001f7d48(sub(yaw,number(word(p+0x104))),&facing))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            /* 2724EC's ordinary wall applies VT+AC(10,NULL,1,0).
             * The original stage multiplier is 1 in R103. */
            const unsigned hp=half(p+0x54a);
            put_half(p+0x54a,(uint16_t)(hp>10?hp-10:0));
            p[0x2f4]=1;
            p[0x2f5]=4;
            p[0x2f6]=2;
            p[0x2f7]=(uint8_t)(fabsf(facing)<1.5707964f);
            return GH_ACTOR_MOTION_OK;
        }
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_boss_heavy_recoil_0024f638(gh_enemy202_move *s,uint8_t *p)
{
    enum { START=6, SCREEN_EFFECT, IMPACT, CORE_SWING, CORE_RECOIL,
           CHOOSE_MOTION, BIND_MOTION, VOICE, ATTACH_DUST, ATTACH_TRAIL, ADVANCE,
           ROOT_ROTATE, ROOT_TRANSLATE, BODY_GIVE, WALL_CHECK, TAIL };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=s->runtime.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    put(p+0x16d0,word(p+0x16d0)|0x1000u);
    if(s->state1_phase==START&&p[0x2f6]){
        if(p[0x2f7]||p[0x2f6]>3||!positive_hp(p)||
           !(word(p+0x16d0)&0x4000000u))return stop(s,0x24f638,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=p[0x2f6]==2?CHOOSE_MOTION:ADVANCE;
    }
    if(s->state1_phase==START){
        if(p[0x2f7]||!positive_hp(p)||word(p+0x6ec))
            return stop(s,0x24f638,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_hitarea_object *body=&enemy->hitareas.units[0];
        const gh_title_player_view *source=NULL;
        if(!body->bytes||body->size<0x50||
           (half(body->bytes+0x46)!=0x0f&&
            !(half(body->bytes+0x46)==0x27&&s->damage.effect==0x27))||
           gh_hitarea_object_last_hit_actor(body,&source)||!source||!source->bytes)
            return stop(s,0x24f788,GH_ACTOR_MOTION_UNSUPPORTED);
        float direction[3],yaw;
        memcpy(direction,body->bytes+0x10,sizeof(direction));
        if(gh_game_angle_001f8180(direction[0],direction[2],&yaw,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x1f8180,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_id_angle_reference_001f7d48(add(yaw,3.1415927f),&yaw))
            return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x104,bits(yaw));
        put(p+0x1710,0);
        put(p+0x5c0,0);
        put(p+0x5c4,0);
        put(p+0x5c8,0xbf000000u);
        put(p+0x5cc,0x3f800000u);
        s->state1_phase=SCREEN_EFFECT;
    }
    if(s->state1_phase==SCREEN_EFFECT){
        const gh_actor_motion_status status=enemy_recoil_screen_00292e80(s);
        if(status)return status;
        s->state1_phase=IMPACT;
    }
    if(s->state1_phase==IMPACT){
        const gh_actor_motion_status status=enemy_recoil_impact_002717d0(s);
        if(status)return status;
        s->state1_phase=CORE_SWING;
    }
    while(s->state1_phase==CORE_SWING||s->state1_phase==CORE_RECOIL){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const int16_t program=s->state1_phase==CORE_SWING?0x2b:0x2e;
        const gh_motion_sound_request cue={.model=motion->b.model,.actor=actor,.bank=0,.program=program};
        uint32_t handle;
        const gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&cue,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        ++s->state1_phase;
    }
    if(s->state1_phase==CHOOSE_MOTION){
        uint32_t draw;
        if(!p[0x2f6]){
            if(word(p+0x6f0)||word(p+0x700)||word(p+0x704))
                return stop(s,0x2740d8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            p[0x17c3]=(uint8_t)(draw&1);
        }
        if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->reaction_resource_offset=p[0x2f6]?(draw%3u==2?0x568:0x560):0x548+8u*(draw%3u);
        s->state1_phase=BIND_MOTION;
    }
    if(s->state1_phase==BIND_MOTION){
        const gh_object_view *resource=&enemy->entry.resource;
        const size_t offset=s->reaction_resource_offset;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<offset+8||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x24f638,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=resource->bytes+offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,motion,
                &motion->b,word(row),word(row+4),0,p[0x2f6]?5:2,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(p[0x2f6]){
            p[0x2f6]=3;
            s->state1_phase=ADVANCE;
        }else s->state1_phase=VOICE;
    }
    if(s->state1_phase==VOICE){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request cue={.model=motion->b.model,.actor=actor,
            .bank=1,.program=(int16_t)(word(p+0x424)+0x0c)};
        uint32_t handle;
        const gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&cue,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->blowaway_effect=(gh_effect_create_call){0};
        s->state1_phase=ATTACH_DUST;
    }
    while(s->state1_phase==ATTACH_DUST||s->state1_phase==ATTACH_TRAIL){
        gh_effect_create_bindings bindings=s->impact_bindings;
        bindings.model=motion->b.model;
        gh_effect_create_call *effect=&s->blowaway_effect;
        if(!effect->active){
            put(p+0x574,2);
            put_half(p+0x570,0);
            if(gh_effect_create_enemy_knockdown_002a8608(effect,actor,s->state1_phase==ATTACH_DUST?4:8))
                return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        const gh_effect_status result=gh_game_effect_create_00306528(effect,&bindings);
        if(result||!effect->complete)
            return stop(s,effect->frontier?effect->frontier:0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        *effect=(gh_effect_create_call){0};
        ++s->state1_phase;
    }
    if(s->state1_phase==ADVANCE){
        if(!p[0x2f6])p[0x2f6]=1;
        if(p[0x2f6]==1)put_half(p+0x434,half(p+0x434)|8u);
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(p[0x2f6]==3&&ended){
            const gh_actor_motion_status recovery=choose_after_knockdown_00274fe8(s,p);
            if(recovery)return recovery;
            s->state1_phase=TAIL;
            return advance_state0_tail(s,p);
        }
        if(p[0x2f6]==1&&ended)p[0x2f6]=2;
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x25002c,GH_ACTOR_MOTION_CALLEE);
        if(p[0x2f6]==3){
            put(p+0x5c8,bits(mul(number(word(p+0x5c8)),.92f)));
            memcpy(p+0x330,p+0x5c0,12);
        }else{
            const float speed=number(word(p+0x5a8));
            for(unsigned i=0;i<3;i++)
                put(p+0x330+i*4,bits(mul(number(word(p+0x5c0+i*4)),speed)));
        }
        if(fesetenv(&env))return stop(s,0x25002c,GH_ACTOR_MOTION_CALLEE);
        s->state1_phase=ROOT_ROTATE;
    }
    if(s->state1_phase==ROOT_ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=ROOT_TRANSLATE;
    }
    if(s->state1_phase==ROOT_TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=p[0x2f6]==3?TAIL:BODY_GIVE;
    }
    if(s->state1_phase==BODY_GIVE){
        put(p+0x54c,0x40400000u);
        const gh_actor_motion_status status=body_give_00262750(s,p,0);
        if(status)return status;
        s->state1_phase=WALL_CHECK;
    }
    if(s->state1_phase==WALL_CHECK){
        const gh_actor_motion_status status=enemy_recoil_clear_wall_00271eb0(s,p);
        if(status)return status;
        s->state1_phase=TAIL;
    }
    return advance_state0_tail(s,p);
}

/* 2508D8 selector04 phase 2 binds the wall recoil clip; 251614
 * advances it until both the original motion and ground tests allow recovery. */
static gh_actor_motion_status advance_wall_recoil_002508d8(gh_enemy202_move *s,uint8_t *p)
{
    enum { ENTER=6, BIND, SAMPLE, ROTATE, TRANSLATE, GIVE, TAIL,
           RECOVER_BIND, RECOVER_SAMPLE, RECOVER_ROTATE, RECOVER_TRANSLATE, RECOVER_FINISH };
    gh_enemy_actor *enemy=s->enemy;
    gh_title_player_view *actor=&enemy->entry.object.view;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    if(s->state1_phase==ENTER){
        if(!r103_arena_boss(&enemy->entry)||p[0x2f7]||!positive_hp(p)||
           p[0x2f6]<2||p[0x2f6]>5)
            return stop(s,0x2508d8,GH_ACTOR_MOTION_UNSUPPORTED);
        switch(p[0x2f6]){
            case 2:s->state1_phase=BIND;break;
            case 3:s->state1_phase=SAMPLE;break;
            case 4:s->state1_phase=RECOVER_BIND;break;
            case 5:s->state1_phase=RECOVER_SAMPLE;break;
        }
    }
    if(s->state1_phase==BIND){
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(word(p+0x6f4))return stop(s,0x251570,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x580||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x251564,GH_ACTOR_MOTION_BOUNDS);
        if(!call->active){
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+0x578),word(resource->bytes+0x57c),
                0,3,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        if(number(word(p+0x54c))<8.0f)put(p+0x54c,0x41000000u);
        put(p+0x600,0);
        put(p+0x5fc,0);
        p[0x2f6]=3;
        s->state1_phase=SAMPLE;
    }
    if(s->state1_phase==SAMPLE){
        int ended;
        put_half(p+0x434,half(p+0x434)|8u);
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(ended||(half(p+0x3ac)&0x10u))put(p+0x5fc,1);
        if(word(p+0x5fc)&&(word(p+0x16d0)&0x4000000u))p[0x2f6]=4;
        s->state1_phase=ROTATE;
    }
    if(s->state1_phase==ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=TRANSLATE;
    }
    if(s->state1_phase==TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x251614,GH_ACTOR_MOTION_CALLEE);
        const float elapsed=add(number(word(p+0x600)),number(word(p+0x5a8)));
        if(fesetenv(&env))return stop(s,0x251614,GH_ACTOR_MOTION_CALLEE);
        put(p+0x600,bits(elapsed));
        if(elapsed>60.0f){
            p[0x2f4]=2;
            p[0x2f5]=2;
            p[0x2f6]=0;
            p[0x2f7]=1;
        }
        s->state1_phase=GIVE;
    }
    if(s->state1_phase==GIVE){
        const gh_actor_motion_status status=body_give_00262750(s,p,4);
        if(status)return status;
        s->state1_phase=TAIL;
    }
    if(s->state1_phase==RECOVER_BIND){
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x58c||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x2519bc,GH_ACTOR_MOTION_BOUNDS);
        if(!call->active){
            const gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+0x584),word(resource->bytes+0x588),
                0,3,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        const gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put_half(p+0x56e,1);
        p[0x2f6]=5;
        s->state1_phase=RECOVER_SAMPLE;
    }
    if(s->state1_phase==RECOVER_SAMPLE){
        int ended;
        const gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state1_phase=ended?RECOVER_FINISH:RECOVER_ROTATE;
    }
    if(s->state1_phase==RECOVER_ROTATE){
        if(gh_player_root_rotation_0014f9f0(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=RECOVER_TRANSLATE;
    }
    if(s->state1_phase==RECOVER_TRANSLATE){
        if(gh_player_root_translation_0014f928(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        if((half(p+0x3ac)&0x10u)&&p[0x17c1]&&half(p+0x56e)&&number(word(p+0x17cc))<=0){
            if(number(word(p+0x618))<4.0f)
                return stop(s,0x251bf8,GH_ACTOR_MOTION_UNSUPPORTED);
            put_half(p+0x56e,0);
        }
        s->state1_phase=TAIL;
    }
    if(s->state1_phase==RECOVER_FINISH){
        /* 274E98 can redirect only in room 0700. */
        const gh_actor_motion_status status=choose_after_knockdown_00274fe8(s,p);
        if(status)return status;
        s->state1_phase=TAIL;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_state1_0024c0f0(gh_enemy202_move *s,uint8_t *p)
{
    const gh_enemy_entry *entry=s->runtime.entry;
    const gh_boot_work *work=entry->b.work;
    const uint32_t dependency=word(p+0x564);
    const int devil=!s->enemy&&work->room==0x100&&dependency==0x220&&
        half(p+0x2fe)==0x220&&half(p+0x640)==0xff;
    if(!s->enemy&&(!devil||(p[0x2f5]!=1&&p[0x2f5]!=0x11)))
        return stop(s,0x24c0f0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->state1_phase){
        /* The same original 24C2E8 table reaches 24C3C8 for stamp19's
         * state 01/00. Its dependency 203 takes the ordinary 774/77C rows. */
        const int stamp19=dependency==0x203&&half(p+0x640)==19&&
            (uint16_t)work->room==0x100;
        const int west=r100_at2_enemy(work,p)||r100_at2_formation(work,p)||r100_at3_enemy(work,p);
        const int stamp20=dependency==0x200&&half(p+0x640)==20&&
            (uint16_t)work->room==0x100;
        const int stamp8=dependency==0x200&&half(p+0x640)==8&&
            (uint16_t)work->room==0x100;
        const int r102_em00=dependency==0x200&&
            (uint16_t)work->room==0x102;
        const int arena=r103_arena_em00(entry);
        const int r102_em03=r102_em03_stamp8(work,p);
        const int gate_guard_stun=r102_em00&&half(p+0x640)==0x10&&!p[0x1650]&&p[0x2f5]==0x0f;
        const int arena_em03=r103_arena_em03(entry);
        const int arena_em0a=r103_arena_em0a(entry);
        const int arena_boss=r103_arena_boss(entry)&&
            (p[0x2f5]<=1||p[0x2f5]==3||p[0x2f5]==4||(p[0x2f5]==7&&!positive_hp(p)));
        const int gate_em0a_reaction=r102_gate_em0a(work,p)&&p[0x2f5]==1&&
            p[0x2f6]<=1&&!p[0x2f7];
        const int gate_em0a_blowaway=r102_gate_em0a(work,p)&&p[0x2f5]==8&&p[0x2f7]==2;
        const int r101_box=r101_scenery_enemy(work,p)&&p[0x2f5]<=1;
        const int r101_wave=r101_arena_airborne(s->enemy)&&p[0x2f5]<=1&&
            ((half(p+0x640)==0x16&&dependency==0x200)||(half(p+0x640)==0x20&&dependency==0x203));
        /* EM14's original 2E/body recoil Take enters the same 258D80 row. */
        const int twin_collateral=r103_arena_boss(entry)&&dependency==0x214&&
            half(p+0x640)==0&&positive_hp(p)&&word(p+0x17b4)==10&&!p[0x2f7];
        const uint32_t impact_power=word(p+0x17b4);
        const int west_em0a_scenery=work->room==0x100&&dependency==0x20a&&!half(p+0x640)&&
            p[0x1650]==3&&!p[0x2f7]&&(impact_power==0||impact_power==20);
        const int scenery=p[0x2f5]==0x11&&(twin_collateral||r101_scenery_enemy(work,p)||
            r102_east_em27(work,p)||west_em0a_scenery);
        const int boss_getup=p[0x2f5]==0x0d&&r103_heavy_recoil_getup(s->enemy,p);
        const int r104=gh_enemy203_is_r104_entry(&entry->request,work->room)&&
            p[0x2f5]<=1&&p[0x2f6]<=3&&!p[0x2f7]&&positive_hp(p);
        if(!(devil||r104||r101_box||r101_wave||scenery||gate_guard_stun||gate_em0a_reaction||gate_em0a_blowaway||arena_boss||boss_getup||
            ((dependency==0x20a||stamp19||stamp20||r102_em00||r102_em03||arena||arena_em03||
              r102_east_em27(work,p)||r102_area11_em02(work,p))&&p[0x2f5]==0)||
            (r102_em00&&half(p+0x640)==0x10&&!p[0x1650]&&p[0x2f5]==1&&
             (p[0x2f6]==1||number(word(p+0x17cc))>0))||
            (stamp8&&(p[0x2f5]==1||(p[0x2f5]==7&&!positive_hp(p))))||
            ((stamp19||stamp20||west)&&(p[0x2f5]==1||(p[0x2f5]==7&&!positive_hp(p))))||
            (r100_west_em03_patrol(work,p)&&(p[0x2f5]==1||(p[0x2f5]==7&&!positive_hp(p))))||
            ((arena||arena_em03)&&p[0x2f5]==1)||((arena||arena_em03)&&p[0x2f5]==7)||
            ((stamp20||arena)&&(p[0x2f5]==0x1d||p[0x2f5]==0xd))||
            (p[0x2f5]==0x1d&&r102_grab_escape_counter(work,p))||
            (p[0x2f5]==0xd&&(r102_em00_stamp11(work,p)||
             r102_em00_stamp7_grab(work,p)||
             (r102_gate_em0a(work,p)&&p[0x2f6]<=1&&!p[0x2f7])))||
            (arena_em0a&&(p[0x2f5]==1||p[0x2f5]==7||p[0x2f5]==0xd||p[0x2f5]==0xf))||
            ((dependency==0x200||dependency==0x202||dependency==0x203)&&p[0x2f5]==0x11)))
            return stop(s,0x24c0f0,GH_ACTOR_MOTION_UNSUPPORTED);
        p[0x1864]=0;
        if(p[0x2f6]==0){
            put(p+0x16d0,(word(p+0x16d0)&~0x8000u)|2u);
            s->state1_phase=1;
        }else s->state1_phase=5;
    }
    if(s->state1_phase==1){
        /* 26A938 drops the selected held object only when its slot exists. */
        if(word(p+0x6e4)||word(p+0x6e8))return stop(s,0x26a938,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=2;
    }
    if(s->state1_phase==2){
        if(word(p+0x70c))return stop(s,0x1c2318,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x730))return stop(s,0x1d0108,GH_ACTOR_MOTION_UNSUPPORTED);
        /* 26BB70 only visits held accessories for dependency 211. */
        if(word(p+0x738))return stop(s,0x1d0408,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x16d4)&0x20u)return stop(s,0x306200,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=3;
    }
    if(s->state1_phase==3){
        /* 24C1E8 clears this byte only for selector 08. */
        if(p[0x2f5]!=1||!r102_gate_em0a(work,p))p[0x1867]=0;
        s->state1_phase=4;
    }
    if(s->state1_phase==4){
        if(word(p+0x6f0))return stop(s,0x1d04d0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x708))return stop(s,0x1d05c0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=5;
    }
    if(s->state1_phase==5){
        uint32_t expected=0x24c3c8;
        if(p[0x2f5]==1)expected=0x24da80;
        if(p[0x2f5]==3)expected=0x24f638;
        if(p[0x2f5]==4)expected=0x2508d8;
        if(p[0x2f5]==7)expected=0x253de8;
        if(p[0x2f5]==8)expected=0x254450;
        if(p[0x2f5]==0x11)expected=0x258d80;
        if(p[0x2f5]==0x1d)expected=0x25bcc0;
        if(p[0x2f5]==0xd)expected=0x257948;
        if(p[0x2f5]==0x0f)expected=0x2580b8;
        const uint8_t *descriptor=elf_span(s->elf,s->elf_bytes,0x3bf738+8u*p[0x2f5],8);
        if(!descriptor||half(descriptor)||half(descriptor+2)!=0xffff||word(descriptor+4)!=expected)
            return stop(s,0x24c2e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->pending_ai_pc=word(descriptor+4);
        s->state1_phase=6;
    }
    if(s->pending_ai_pc==0x24f638)return advance_boss_heavy_recoil_0024f638(s,p);
    if(s->pending_ai_pc==0x2508d8)return advance_wall_recoil_002508d8(s,p);
    if(s->pending_ai_pc==0x258d80)return advance_scenery_reaction_00258d80(s,p);
    if(s->pending_ai_pc==0x253de8)return advance_knockdown_reaction_00253de8(s,p);
    if(s->pending_ai_pc==0x254450)return advance_em0a_blowaway_00254450(s,p);
    if(s->pending_ai_pc==0x25bcc0)return advance_em00_escape_counter_0025bcc0(s,p);
    if(s->pending_ai_pc==0x257948)return advance_em00_getup_00257948(s,p);
    if(s->pending_ai_pc==0x2580b8)return advance_gate_guard_stun_002580b8(s,p);
    return advance_first_reaction_0024c3c8(s,p);
}

static gh_actor_motion_status death_effect(gh_enemy202_move *s,int32_t effect,uint32_t mode)
{
    gh_actor_motion *motion=&s->enemy->motion.motion;
    if(!motion->b.effect)return stop(s,0x306200,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t pc=0;
    gh_actor_motion_status status=motion->b.effect(motion->b.context,&motion->b.model->root,effect,mode,&pc);
    return status?stop(s,pc?pc:0x306200,status):GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_move_bind_devil_spawn(gh_enemy202_move *s,
 gh_enemy202_devil_spawn spawn,void *context)
{
    if(!s||!spawn)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->devil_spawn&&(s->devil_spawn!=spawn||s->devil_spawn_context!=context))
        return GH_ACTOR_MOTION_BUSY;
    s->devil_spawn=spawn;
    s->devil_spawn_context=context;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_move_bind_death_release(gh_enemy202_move *s,
    gh_enemy202_death_release release,void *context)
{
    if(!s||!release)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->death_release&&(s->death_release!=release||s->death_release_context!=context))
        return GH_ACTOR_MOTION_BUSY;
    s->death_release=release;s->death_release_context=context;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status count_death_and_mark_room(gh_enemy202_move *s,const uint8_t *p)
{
    const unsigned stamp=half(p+0x640);
    if(stamp==0xff)return GH_ACTOR_MOTION_OK;
    const gh_room_state *room=s->room_state;
    if(!room||!room->record||room->record->room!=s->enemy->b.entry.work->room||
       !room->active||room->size<12||room->active[8]>64||
       (size_t)room->active[8]>(room->size-12)/24)
        return stop(s,0x2bef60,GH_ACTOR_MOTION_UNSUPPORTED);
    for(unsigned i=0;i<room->active[8];++i){
        uint8_t *row=room->active+12+24*i;
        if(row[0x13]!=stamp)continue;
        if(row[0x14]!=1){
            const unsigned rank=heat_category(s);
            const gh_core_save_view save=s->enemy->b.entry.save;
            if(!rank||rank>5||!save.bytes||save.size<GH_CORE_SAVE_BYTES)
                return stop(s,0x1fbe40,GH_ACTOR_MOTION_UNSUPPORTED);
            uint8_t *local=save.bytes+0x8c+2*(rank-1);
            uint8_t *total=save.bytes+0xbac+2*(rank-1);
            put_half(local,(uint16_t)(half(local)+1));
            put_half(total,(uint16_t)(half(total)+1));
            row[0x14]=1;
        }
        break;
    }
    return GH_ACTOR_MOTION_OK;
}

enum {
    DEATH_FADE_PRIMARY_EFFECT = 1,
    DEATH_FADE_SECONDARY_EFFECT = 2,
    DEATH_FADE_RELEASE_MOTION = 3,
    DEATH_DROP_BEGIN = 4,
    DEATH_DROP_PITY = 5,
    DEATH_DROP_CURRENCY_DRAW = 6,
    DEATH_DROP_CURRENCY = 7,
    DEATH_DROP_COMPLETE = 8,
    DEATH_FADE_ADVANCE = 9,
    DEATH_FADE_RELEASE_ACTOR = 10,
    DEATH_FADE_TAIL = 11,
    DEATH_DROP_FALLBACK_DRAW = 12,
    DEATH_DROP_FALLBACK_SPAWN = 13,
    DEATH_DROP_FALLBACK_NEARBY = 14,
    DEATH_DROP_FALLBACK_CONTINUES = 15,
    DEATH_DROP_AUTHORED = 16,
};

/* 292650 chooses the original item after the pity threshold succeeds. */
static gh_actor_motion_status fallback_death_item_00292650(gh_enemy202_move *s)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_game_init_state *init=enemy->b.entry.init;
    const gh_core_save_view save=enemy->b.entry.save;
    if(init->random.index>624||!save.bytes||save.size<GH_CORE_SAVE_BYTES)
        return stop(s,0x31d000,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t draw;
    if(gh_game_mt_next_0030f348(&init->random,&draw))
        return stop(s,0x31d000,GH_ACTOR_MOTION_UNSUPPORTED);
    float probability=number(gh_frame_unit_bits_0030f518_word(draw));
    unsigned kind=probability>=.9f?0x3df:probability>=.85f?0x3e2:probability>=.5f?0x3e0:0x3c1;
    if(kind==0x3c1||kind==0x3e0){
        if(gh_game_mt_next_0030f348(&init->random,&draw))
            return stop(s,0x31d000,GH_ACTOR_MOTION_UNSUPPORTED);
        probability=number(gh_frame_unit_bits_0030f518_word(draw));
        if(kind==0x3e0)kind=probability>=.95f?0x3de:probability>=.75f?0x3e1:0x3e0;
        else {
            kind=probability>=.95f?0x3d0:probability>=.85f?0x3d1:probability>=.55f?0x3c8:0x3c1;
            if(kind==0x3d0&&heat_category(s)>2){
                if(gh_game_mt_next_0030f348(&init->random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                if(draw&1u)kind=0x3d1;
            }
        }
    }
    s->death_item=(uint16_t)kind;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status finish_death_item_00292804(gh_enemy202_move *s,unsigned kind)
{
    gh_game_init_state *init=s->enemy->b.entry.init;
    const gh_core_save_view save=s->enemy->b.entry.save;
    uint32_t draw;
    if(init->continue_count>=4&&save.bytes[0x1f]!=2){
        s->death_fade_phase=DEATH_DROP_FALLBACK_CONTINUES;return stop(s,0x292828,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    if(gh_game_mt_next_0030f348(&init->random,&draw))
        return stop(s,0x31d000,GH_ACTOR_MOTION_UNSUPPORTED);
    if(number(gh_frame_unit_bits_0030f518_word(draw))>=.75f)kind=0x3c4;
    if(word(init->enemies+0x5a8)==kind){
        if(gh_game_mt_next_0030f348(&init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        kind=(draw&1u)?0x3c1:0x3c8;
    }
    s->death_item=(uint16_t)kind;
    s->death_fade_phase=DEATH_DROP_FALLBACK_SPAWN;
    return GH_ACTOR_MOTION_OK;
}

static int r100_opening_death_drop(const gh_boot_work *work,const uint8_t *actor)
{
    const unsigned stamp=half(actor+0x640);
    const uint32_t dependency=word(actor+0x564);
    return work->room==0x100&&!actor[0x1650]&&
        ((stamp==19&&dependency==0x203)||(stamp==20&&dependency==0x200));
}

/* Keep the original RNG choice while the item factory works through its
 * typed construction and placement. */
static gh_actor_motion_status death_drop_002923e8(gh_enemy202_move *s,uint8_t *p)
{
    gh_game_init_state *init=s->enemy->b.entry.init;
    if(s->death_fade_phase==DEATH_DROP_BEGIN){
        if(word(p+0x16d4)&2u){s->death_fade_phase=DEATH_DROP_COMPLETE;return GH_ACTOR_MOTION_OK;}
        put(p+0x16d4,word(p+0x16d4)|2u);
        const unsigned stamp=half(p+0x640);
        const uint32_t item=word(p+0x560);
        /* REL0E54 gives both bosses explicit drops in the 292048 identity range. */
        const int r101_key_card=item==0x3e3&&s->enemy->b.entry.work->room==0x101&&
            ((stamp==9&&word(p+0x564)==0x227&&p[0x1650]==4)||
             (stamp==7&&word(p+0x564)==0x20a&&p[0x1650]==1));
        if(r101_key_card||(r103_arena_boss(&s->enemy->entry)&&item==(stamp==0?0x3d1u:0x3d7u))){
            s->death_item=(uint16_t)item;
            s->death_fade_phase=DEATH_DROP_AUTHORED;
        }else s->death_fade_phase=DEATH_DROP_PITY;
    }
    if(s->death_fade_phase==DEATH_DROP_PITY){
        /* Actual dependency203 uses the default branch and original
         * item-code FFFF stays FFFF through 292048. */
        if(word(p+0x560)!=0xffff)return stop(s,0x292048,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_core_save_view save=s->enemy->b.entry.save;
        const unsigned rank=heat_category(s);
        const uint32_t pity=word(init->enemies+0x534);
        if(!save.bytes||save.size<GH_CORE_SAVE_BYTES||!rank||rank>5||pity>INT32_MAX/10)
            return stop(s,0x292498,GH_ACTOR_MOTION_UNSUPPORTED);
        static const float factors[]={1,.9f,.8f,.8f,.65f};
        const unsigned difficulty=save.bytes[0x1f];
        const unsigned modulus=difficulty!=2&&(difficulty==0||init->continue_count>=4)?40:50;
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x292498,GH_ACTOR_MOTION_CALLEE);
        volatile float scaled=mul((float)(pity*10u),factors[rank-1]);
        if(fesetenv(&env)||!isfinite(scaled)||scaled<0||scaled>=2147483648.0f)
            return stop(s,0x292498,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if((uint32_t)scaled<draw%modulus){
            put(init->enemies+0x534,pity+1);
            s->death_fade_phase=DEATH_DROP_CURRENCY_DRAW;
        }else{
            put(init->enemies+0x534,0);
            const gh_enemy_entry *entry=&s->enemy->entry;
            const int arena=r103_arena_em00(entry)||r103_arena_em03(entry)||r103_arena_em0a(entry);
            const gh_boot_work *work=s->enemy->b.entry.work;
            const int opening=r100_opening_death_drop(work,p);
            const int west=r100_at2_formation(work,p)||r100_at2_enemy(work,p)||opening;
            s->death_fade_phase=(arena||west||r102_em00_stamp3(work,p))?
                DEATH_DROP_FALLBACK_DRAW:DEATH_DROP_CURRENCY;
        }
    }
    if(s->death_fade_phase==DEATH_DROP_CURRENCY_DRAW){
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(draw&3u)s->death_fade_phase=DEATH_DROP_COMPLETE;
        else{s->death_item=0x3c4;s->death_fade_phase=DEATH_DROP_CURRENCY;}
    }
    if(s->death_fade_phase==DEATH_DROP_FALLBACK_DRAW){
        const gh_actor_motion_status status=fallback_death_item_00292650(s);
        if(status)return status;
        s->death_fade_phase=DEATH_DROP_FALLBACK_NEARBY;
    }
    if(s->death_fade_phase==DEATH_DROP_FALLBACK_NEARBY){
        unsigned kind=s->death_item;
        if(kind==0x3df){
            if(!r102_em00_stamp3(s->enemy->b.entry.work,p))
                return stop(s,0x291010,GH_ACTOR_MOTION_UNSUPPORTED);
            float origin[3];
            uint32_t pc=0;
            gh_player_target nearby={0};
            const gh_enemy_actor *enemy=s->enemy;
            if(s->targets.pools!=enemy->b.entry.pools||s->targets.actors!=enemy->b.actors||
               !position(&enemy->entry.object.view,origin))
                return stop(s,0x291010,GH_ACTOR_MOTION_UNSUPPORTED);
            const gh_player_target_status status=gh_death_drop_target_00291010(&s->targets,init,origin,&nearby,&pc);
            if(status)return stop(s,pc?pc:0x291010,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!nearby.handle){
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&init->random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                kind=(draw&1u)?0x3e0:0x3c8;
                s->death_item=(uint16_t)kind;
            }
        }
        const gh_actor_motion_status status=finish_death_item_00292804(s,kind);
        if(status)return status;
    }
    if(s->death_fade_phase==DEATH_DROP_FALLBACK_CONTINUES)return stop(s,0x292828,GH_ACTOR_MOTION_UNSUPPORTED);
    const int authored=s->death_fade_phase==DEATH_DROP_AUTHORED;
    if(authored||s->death_fade_phase==DEATH_DROP_CURRENCY||s->death_fade_phase==DEATH_DROP_FALLBACK_SPAWN){
        if(!s->death_item)return stop(s,0x31d000,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_boot_work *work=s->enemy->entry.b.work;
        const int opening=r100_opening_death_drop(work,p);
        if(!authored&&!r102_em00_stamp3(work,p)&&
           s->death_item!=0x3c4&&s->death_item!=0x3d0&&!(opening&&(s->death_item==0x3e0||s->death_item==0x3e1))&&
           (s->death_item!=0x3c8||!r103_arena_em0a(&s->enemy->entry)))
            return stop(s,0x30fa98,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!s->death_item_spawn)return stop(s,0x30fa98,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t pc=0;
        gh_actor_motion_status status=s->death_item_spawn(s->death_item_context,&s->death_drop,
            s->enemy,s->death_item,&pc);
        if(status)return stop(s,pc?pc:0x30fa98,status);
        if(s->death_fade_phase==DEATH_DROP_FALLBACK_SPAWN)put(init->enemies+0x5a8,s->death_item);
        s->death_fade_phase=DEATH_DROP_COMPLETE;
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status death_fade_0025f1f0(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_game_init_state *init=enemy->b.entry.init;
    uint32_t pc=0;
    const uint32_t flags=word(p+0x16d4);
    const int retained_boss=(flags&0x1000u)&&r103_arena_boss(&enemy->entry);
    if(!s->death_fade_phase){
        put(p+0x54c,0x40400000);
        put(p+0x16d0,word(p+0x16d0)|0x10000u);
        if(gh_collision_solid_set_enabled_001351d8(enemy->b.solids,&enemy->entry.object.view,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)|0x20000u);
        put(p+0x250,word(p+0x250)|0x10000u);
        p[0x617]=1;
        if(p[0x2f7]||(flags&0x100000u)||number(word(p+0x770))!=0)
            return stop(s,0x25f1f0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(retained_boss){
            /* 25F900 records the defeat and leaves the body for the room outro. */
            if(p[0x2f6]==6)s->death_fade_phase=DEATH_FADE_TAIL;
            else if(p[0x2f6]==0||p[0x2f6]==5){
                p[0x2f6]=5;
                put_half(p+0x54a,0);
                put(p+0x608,0);
                s->death_fade_phase=DEATH_DROP_BEGIN;
            }else return stop(s,0x25f1f0,GH_ACTOR_MOTION_UNSUPPORTED);
        }else{
            if(flags&0x1000u)return stop(s,0x25f1f0,GH_ACTOR_MOTION_UNSUPPORTED);
            s->death_fade_phase=p[0x2f6]?DEATH_FADE_ADVANCE:DEATH_FADE_PRIMARY_EFFECT;
        }
    }
    if(s->death_fade_phase==DEATH_FADE_PRIMARY_EFFECT){
        gh_actor_motion_status status=death_effect(s,1,2);
        if(status)return status;
        s->death_fade_phase=DEATH_FADE_SECONDARY_EFFECT;
    }
    if(s->death_fade_phase==DEATH_FADE_SECONDARY_EFFECT){
        gh_actor_motion_status status=death_effect(s,9,2);
        if(status)return status;
        s->death_fade_phase=DEATH_FADE_RELEASE_MOTION;
    }
    if(s->death_fade_phase==DEATH_FADE_RELEASE_MOTION){
        gh_actor_motion *motion=&enemy->motion.motion;
        if(!s->substate67_motion.active){
            gh_actor_motion_status status=gh_actor_motion_call_open_release_002a84a8(
                &s->substate67_motion,motion,motion->b.effect,motion->b.context);
            if(status)return stop(s,0x2a8718,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_release_step_002a84a8(&s->substate67_motion);
        if(status)return stop(s,s->substate67_motion.frontier?s->substate67_motion.frontier:0x2a8718,status);
        put_half(p+0x54a,0);p[0x531]=0xff;
        put(p+0x604,0);put(p+0x600,0);put_half(p+0x62c,half(p+0x62c)|4u);
        s->death_fade_phase=DEATH_DROP_BEGIN;
    }
    if((s->death_fade_phase>=DEATH_DROP_BEGIN&&s->death_fade_phase<=DEATH_DROP_COMPLETE)||
       s->death_fade_phase>=DEATH_DROP_FALLBACK_DRAW){
        gh_actor_motion_status status=death_drop_002923e8(s,p);
        if(status)return status;
        if(half(p+0x640)==0xff)return stop(s,0x1fbe40,GH_ACTOR_MOTION_UNSUPPORTED);
        status=count_death_and_mark_room(s,p);
        if(status)return status;
        if(retained_boss){
            p[0x531]=0xff;
            ++p[0x2f6];
            s->death_fade_phase=DEATH_FADE_TAIL;
        }else{
            if(word(p+0x6f4)||word(p+0x704))return stop(s,0x1cf4a8,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x608,0);
            ++p[0x2f6];
            s->death_fade_phase=DEATH_FADE_ADVANCE;
        }
    }
    if(s->death_fade_phase==DEATH_FADE_ADVANCE){
        const unsigned phase=p[0x2f6];
        fenv_t env;
        if(phase==1){
            const float alpha=number(word(p+0x24c));
            if(!isfinite(alpha)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x25f1f0,GH_ACTOR_MOTION_UNSUPPORTED);
            const float next=sub(alpha,.1f);
            if(fesetenv(&env)||!isfinite(next))return stop(s,0x25f1f0,GH_ACTOR_MOTION_CALLEE);
            put(p+0x24c,next<=0?0:bits(next));
            if(next<=0)p[0x2f6]=2;
        }else if(phase==2||phase==3){
            if(phase==2){put(p+0x600,0x41f00000);p[0x2f6]=3;}
            const float timer=number(word(p+0x600)),rate=number(word(p+0x5a8));
            if(!isfinite(timer)||!isfinite(rate)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x25f1f0,GH_ACTOR_MOTION_UNSUPPORTED);
            const float next=sub(timer,rate);
            if(fesetenv(&env)||!isfinite(next))return stop(s,0x25f1f0,GH_ACTOR_MOTION_CALLEE);
            if(timer>0)put(p+0x600,bits(next));
            else{
                gh_actor_motion_status status=count_death_and_mark_room(s,p);
                if(status)return status;
                if(init->enemies[0x53e])--init->enemies[0x53e];
                s->death_fade_phase=DEATH_FADE_RELEASE_ACTOR;
            }
        }else if(phase!=4)return stop(s,0x25f1f0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(s->death_fade_phase==DEATH_FADE_ADVANCE)s->death_fade_phase=DEATH_FADE_TAIL;
    }
    if(s->death_fade_phase==DEATH_FADE_RELEASE_ACTOR){
        if(!s->death_release)return stop(s,0x290770,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=s->death_release(s->death_release_context,enemy,&pc);
        if(status)return stop(s,pc?pc:0x290770,status);
        ++p[0x2f6];
        s->death_fade_phase=DEATH_FADE_TAIL;
    }
    /* 208BE0 resumes with body nodes after the mode2 handler. */
    if(!s->substate67_stage)s->substate67_stage=7;
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_knockdown_death_0025ed00(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->entry.b.work;
    uint32_t pc=0;
    const int em0a=r102_gate_em0a(work,p)||r103_arena_em0a(&enemy->entry);
    if(s->death_phase==1){
        if(p[0x2f6]>1||positive_hp(p)||(!em0a&&p[0x17c3]))
            return stop(s,0x25ed00,GH_ACTOR_MOTION_UNSUPPORTED);
        if(p[0x2f6])s->death_phase=5;
        else{
            const gh_object_view *resource=&enemy->entry.resource;
            const size_t required=em0a?0xb4c:0x390;
            if(!resource->file_extent_known||resource->file_extent<required||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x25ed00,GH_ACTOR_MOTION_BOUNDS);
            if(em0a){
                /* Dependency 020A selects 25EE6C, with no random row selection. */
                s->reaction_resource_offset=half(p+0x3ac)&0x200u?0xb44:0xb3c;
            }else{
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                const uint32_t offset=half(p+0x3ac)&0x200u?0x380u:0x370u;
                s->reaction_resource_offset=offset+((draw&1u)?0u:8u);
            }
            s->death_phase=2;
        }
    }
    if(s->death_phase==2){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=enemy->entry.resource.bytes+s->reaction_resource_offset;
            const gh_actor_motion_status result=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(row),word(row+4),0,5,p[0x17c3]?2:0,1);
            if(result)return stop(s,call->frontier?call->frontier:0x2a8578,result);
        }
        const gh_actor_motion_status result=gh_actor_motion_call_step_002a8578(call);
        if(result)return stop(s,call->frontier?call->frontier:0x2a8578,result);
        s->death_phase=4;
    }
    if(s->death_phase==4){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request request={.model=motion->b.model,.actor=actor,
            .bank=1,.program=(int16_t)(half(p+0x424)+8u)};
        uint32_t handle;
        const gh_actor_motion_status result=motion->b.sound(motion->b.sound_context,&request,&handle,&pc);
        if(result)return stop(s,pc?pc:0x2cba48,result);
        p[0x2f6]=1;
        s->death_phase=5;
    }
    if(s->death_phase==5){
        int ended;
        const gh_actor_motion_status result=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(result)return stop(s,pc?pc:0x157c18,result);
        if(ended){
            p[0x2f5]=(word(p+0x1644)&0x800000u)?3:2;
            p[0x2f6]=p[0x2f7]=0;
            s->death_phase=9;
        }else s->death_phase=6;
    }
    if(s->death_phase==6){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->death_phase=7;
    }
    if(s->death_phase==7){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->death_phase=9;
    }
    if(!s->substate67_stage)s->substate67_stage=5;
    return advance_state0_tail(s,p);
}

/* R100 stamp8 25FA18, including the forced child creation and caller's
 * second Take disable. The child spawn callback owns 2905D0's native result. */
static gh_actor_motion_status stamp8_transform_death_0025fa18(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_enemy_entry *entry=&enemy->entry;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint8_t *manager=enemy->b.entry.init->enemies;
    if(s->death_phase==1){
        if(!s->player)return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x54c,0x40400000);
        if(p[0x2f6]==1)s->death_phase=6;
        else if(p[0x2f6])return stop(s,0x25fa18,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    if(s->death_phase==1){
        const gh_object_view *resource=&entry->resource;
        if(!resource->file_extent_known||resource->file_extent<0x44c||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x25fa18,GH_ACTOR_MOTION_BOUNDS);
        const uint8_t *row=resource->bytes+0x444;
        if(!s->substate67_motion.active){
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                &s->substate67_motion,motion,&motion->b,word(row),word(row+4),0,0,0,1);
            if(status)return stop(s,s->substate67_motion.frontier?s->substate67_motion.frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(&s->substate67_motion);
        if(status)return stop(s,s->substate67_motion.frontier?s->substate67_motion.frontier:0x2a8578,status);
        p[0x617]=1;
        put_half(p+0x3ac,half(p+0x3ac)&~0x200u);
        s->death_phase=2;
    }
    if(s->death_phase==2){
        if(!(word(p+0x16d4)&0x1000000u)&&manager[0x53e])
            return stop(s,0x294998,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_object_view other;
        if(gh_game_objects_find(entry->b.room_objects,0x209,&other)!=GH_OBJECT_MISSING||
           gh_game_objects_find(entry->b.room_objects,0x223,&other)!=GH_OBJECT_MISSING)
            return stop(s,0x28fed8,GH_ACTOR_MOTION_UNSUPPORTED);
        int ready=0;
        uint32_t frontier=0;
        if(gh_game_objects_check_ready_001ff238(entry->b.room_objects,0x220,
            &entry->b.resources,&ready,&frontier)||!ready)
            return stop(s,frontier?frontier:0x28fed8,GH_ACTOR_MOTION_UNSUPPORTED);
        float xyz[3];
        if(!position(&entry->object.view,xyz))return stop(s,0x25fe30,GH_ACTOR_MOTION_BOUNDS);
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        /* 25FE30 consumes the variant draw before R100 forces 0220. */
        s->death_transform_request=(gh_enemy_request){
            .object_kind=0x220,.dependency_kind=0x220,.yaw_bits=word(p+0x104),
            .flags=0x400000,.entry_number=0xff
        };
        memcpy(s->death_transform_request.position_xyz,xyz,sizeof(xyz));
        manager[0x53e]=7;
        s->death_phase=3;
    }
    uint32_t pc=0;
    if(s->death_phase==3){
        if(!s->devil_spawn)return stop(s,0x2905d0,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=s->devil_spawn(s->devil_spawn_context,enemy,
            &s->death_transform_request,&pc);
        if(status)return stop(s,pc?pc:0x2905d0,status);
        s->death_phase=4;
    }
    if(s->death_phase==4){
        if(!enemy->hitareas.created[0]||
           gh_hitarea_object_set_active_001fd668(&enemy->hitareas.units[0],0,&pc))
            return stop(s,pc?pc:0x1fd668,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)&~0x200u);
        s->death_phase=5;
    }
    if(s->death_phase==5){
        if(gh_hitarea_object_set_active_001fd668(&enemy->hitareas.units[0],0,&pc))
            return stop(s,pc?pc:0x1fd668,GH_ACTOR_MOTION_UNSUPPORTED);
        put_half(p+0x568,0xdd);
        p[0x2f6]=1;
        s->death_phase=6;
    }
    if(s->death_phase==6){
        put_half(p+0x434,half(p+0x434)|8u);
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->death_phase=7;
    }
    if(s->death_phase==7){
        if(gh_player_root_rotation_0014f9f0(&entry->object.view,entry->b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->death_phase=8;
    }
    if(s->death_phase==8){
        if(gh_player_root_translation_0014f928(&entry->object.view,entry->b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned timer=half(p+0x568);
        if(timer)put_half(p+0x568,(uint16_t)(timer-1));
        else enemy_decision(p,2,2,0);
        s->substate67_stage=5;
        s->death_phase=9;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_death_0025e600(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->b.entry.work;
    uint32_t pc=0;
    if(!s->death_phase){
        const uint32_t dependency=word(p+0x564);
        const unsigned stamp=half(p+0x640);
        const int opening=work->room==0x100&&((dependency==0x203&&stamp==19)||
            (dependency==0x200&&(stamp==20||(stamp==8&&(p[0x2f5]==3||p[0x2f5]==2)))));
        const int guard=stamp==0x10&&!p[0x1650]&&(p[0x2f5]==0||p[0x2f5]==2);
        const int stamp11=r102_em00_stamp11(work,p)&&(p[0x2f5]==0||p[0x2f5]==2);
        const int story_counter=p[0x2f5]==2&&
            (r102_em00_stamp3(work,p)||r102_em00_stamp7_grab(work,p));
        const int gate_em0a=r102_gate_em0a(work,p)&&(p[0x2f5]==1||p[0x2f5]==2);
        const int em03=r102_em03_stamp8(work,p)&&(p[0x2f5]==0||p[0x2f5]==2);
        const int r102=work->room==0x102&&((dependency==0x200&&(guard||stamp11))||
            story_counter||gate_em0a||em03)&&
            !positive_hp(p)&&!p[0x2f7];
        const int arena=!positive_hp(p)&&(p[0x2f5]==1||p[0x2f5]==2)&&
            (r103_arena_em00(&enemy->entry)||r103_arena_em03(&enemy->entry)||
             r103_arena_em0a(&enemy->entry)||r103_arena_boss(&enemy->entry));
        const int at3=r100_at3_enemy(work,p);
        const int west=(r100_at2_formation(work,p)||r100_at2_enemy(work,p)||at3||
            r100_west_em03_patrol(work,p))&&!positive_hp(p)&&
            (p[0x2f5]==1||p[0x2f5]==2)&&!p[0x2f7];
        if(!opening&&!r102&&!arena&&!west)return stop(s,0x25e600,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x6e4)||word(p+0x6e8))return stop(s,0x26a938,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x6ec)||gh_enemy_timers_weapon_kind(&enemy->timers))
            return stop(s,0x26b1e8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x70c))return stop(s,0x1c2318,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x730))return stop(s,0x1d0108,GH_ACTOR_MOTION_UNSUPPORTED);
        /* 26BB70 acts only on dependency211. */
        if(word(p+0x738))return stop(s,0x1d0408,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x6f0))return stop(s,0x1d04d0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x708))return stop(s,0x1d05c0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x16d4)&0x20u){
            gh_actor_motion_status status=death_effect(s,1,2);
            if(status)return status;
            put(p+0x16d4,word(p+0x16d4)&~0x20u);
        }
        p[0x1864]=0;
        if(p[0x2f5]>=5)p[0x2f5]=0;
        const uint8_t *descriptor=elf_span(s->elf,s->elf_bytes,0x3bf878+8u*p[0x2f5],8);
        if(!descriptor||half(descriptor)||half(descriptor+2)!=0xffff)
            return stop(s,0x25e600,GH_ACTOR_MOTION_UNSUPPORTED);
        s->pending_ai_pc=word(descriptor+4);
        s->death_phase=1;
    }
    if(s->pending_ai_pc==0x25fa18&&half(p+0x640)==8)
        return stamp8_transform_death_0025fa18(s,p);
    if(s->pending_ai_pc==0x25f1f0)return death_fade_0025f1f0(s,p);
    if(s->pending_ai_pc==0x25ed00)return advance_knockdown_death_0025ed00(s,p);
    if(s->pending_ai_pc!=0x25e790)return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
    if(s->death_phase==1){
        if(p[0x2f6]>1){s->substate67_stage=5;s->death_phase=9;return advance_state0_tail(s,p);}
        if(p[0x2f6])s->death_phase=5;
        else{
            const gh_object_view *resource=&enemy->entry.resource;
            if(!resource->file_extent_known||resource->file_extent<0x348||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x25e790,GH_ACTOR_MOTION_BOUNDS);
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            s->reaction_resource_offset=(draw&1u)?0x340:0x338;
            s->death_phase=2;
        }
    }
    if(s->death_phase==2){
        if(!s->substate67_motion.active){
            const uint8_t *row=enemy->entry.resource.bytes+s->reaction_resource_offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                &s->substate67_motion,motion,&motion->b,word(row),word(row+4),0,5,p[0x17c3]?2:0,1);
            if(status)return stop(s,s->substate67_motion.frontier?s->substate67_motion.frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(&s->substate67_motion);
        if(status)return stop(s,s->substate67_motion.frontier?s->substate67_motion.frontier:0x2a8578,status);
        s->death_phase=3;
    }
    if(s->death_phase==3){
        /* 25FE30's first valid bounded branch: ordinary spawn flags and
         * dependency203 without the transform-on-death bit. 294998 has
         * no decrement when the shared manager byte is zero. */
        if(!(word(p+0x1644)&0x800000u)&&
           ((word(p+0x16d4)&0x1000000u)||(word(p+0x16d0)&0x200u)||
            enemy->b.entry.init->enemies[0x53e]))
            return stop(s,0x25fe30,GH_ACTOR_MOTION_UNSUPPORTED);
        s->death_phase=4;
    }
    if(s->death_phase==4){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!s->reaction_sound.actor)s->reaction_sound=(gh_motion_sound_request){
            .model=motion->b.model,.actor=actor,.bank=1,.program=(int16_t)(half(p+0x424)+8u)};
        uint32_t handle;
        gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&s->reaction_sound,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        ++p[0x2f6];
        s->death_phase=5;
    }
    if(s->death_phase==5){
        put(p+0x54c,0x40400000);
        put_half(p+0x434,half(p+0x434)|8u);
        int ended=0;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        if(ended){
            p[0x2f5]=(word(p+0x1644)&0x800000u)?3:2;
            p[0x2f6]=p[0x2f7]=0;
            s->substate67_stage=5;
            s->death_phase=9;
        }else s->death_phase=6;
    }
    if(s->death_phase==6){
        if(gh_player_root_rotation_0014f9f0(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->death_phase=7;
    }
    if(s->death_phase==7){
        if(gh_player_root_translation_0014f928(actor,work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=5;
        s->death_phase=9;
    }
    return advance_state0_tail(s,p);
}

gh_actor_motion_status gh_enemy202_move_bind_impact(gh_enemy202_move *move,
    const gh_effect_create_bindings *bindings)
{
    if(!move||!bindings||!bindings->storage.effects||!bindings->work||
        !bindings->random||!bindings->atan_coeff)return GH_ACTOR_MOTION_ARGUMENT;
    if(move->impact_bindings.storage.effects&&
        (move->impact_bindings.storage.effects!=bindings->storage.effects||
         move->impact_bindings.work!=bindings->work||move->impact_bindings.random!=bindings->random||
         move->impact_bindings.atan_coeff!=bindings->atan_coeff))return GH_ACTOR_MOTION_BUSY;
    move->impact_bindings=*bindings;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_move_bind_near_sound(gh_enemy202_move *s,
    gh_enemy_ema2_hit_sound sound,void *context)
{
    if(!s||!sound)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->near_sound&&(s->near_sound!=sound||s->near_sound_context!=context))
        return GH_ACTOR_MOTION_BUSY;
    s->near_sound=sound;
    s->near_sound_context=context;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_move_bind_scene_jump(gh_enemy202_move *s,
    gh_scene_at_manager *at,const gh_bundle *room)
{
    if(!s||!at||!at->ready||!room||!room->bytes||!room->count)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->scene_jump_at&&(s->scene_jump_at!=at||s->scene_jump_room.bytes!=room->bytes||
        s->scene_jump_room.size!=room->size||s->scene_jump_room.count!=room->count))
        return GH_ACTOR_MOTION_BUSY;
    s->scene_jump_at=at;
    s->scene_jump_room=*room;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_move_bind_room_state(gh_enemy202_move *s,
    const gh_room_state *room)
{
    if(!s||!room)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->room_state&&s->room_state!=room)return GH_ACTOR_MOTION_BUSY;
    s->room_state=room;
    return GH_ACTOR_MOTION_OK;
}

/* Original 208F84 chooses the audible hit from the canonical Take ledger.
 * An extra descriptor sound replaces ledger programs 10/100 when both are
 * on the core-bank branch. Dual-sound profiles remain at this boundary. */
static gh_actor_motion_status create_hit_impact(gh_enemy202_move *s,const gh_enemy_move_actor *rt)
{
    if(!s->damage.impact.index)return GH_ACTOR_MOTION_OK;
    if(!s->impact_bindings.storage.effects)return stop(s,0x306838,GH_ACTOR_MOTION_UNSUPPORTED);
    if(s->impact_bindings.work!=rt->entry->b.work||
       s->impact_bindings.random!=&rt->entry->b.init->random)
        return stop(s,0x306838,GH_ACTOR_MOTION_ARGUMENT);
    if(!s->impact_create.active){
        uint32_t position[3];
        memcpy(position,s->damage.impact.position,sizeof(position));
        gh_effect_status begin;
        if(s->damage.impact.bank==0&&s->damage.impact.index==0x40)
            begin=gh_effect_create_enemy_boosted_impact_begin_00306838(&s->impact_create,position);
        else if(s->damage.impact.bank==0)
            begin=gh_effect_create_enemy_counter_impact_begin_00306838(&s->impact_create,position);
        else if(s->damage.impact.index==0x12)
            begin=gh_effect_create_enemy_amplified_impact_begin_00306838(&s->impact_create,position);
        else
            begin=gh_effect_create_enemy_impact_begin_00306838(&s->impact_create,position);
        if(begin)return stop(s,0x306838,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    gh_effect_status status=gh_game_effect_create_00306528(&s->impact_create,&s->impact_bindings);
    if(status||!s->impact_create.complete)
        return stop(s,s->impact_create.frontier?s->impact_create.frontier:0x306528,
            status==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE);
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status submit_hit_sound(gh_enemy202_move *s,const gh_motion_sound_request *request)
{
    const gh_actor_motion *motion=s->runtime.motion;
    if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t handle,pc=0;
    gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,request,&handle,&pc);
    return status?stop(s,pc?pc:0x2cba48,status):GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status play_hit_sound(gh_enemy202_move *s, gh_actor_model *model)
{
    const gh_hitarea_object *body=&s->runtime.hitareas->units[0];
    gh_title_player_view *view=&s->runtime.entry->object.view;
    if(!s->hit_sound.model){
        if(!body->bytes||body->size<0x46)return stop(s,0x208f84,GH_ACTOR_MOTION_ARGUMENT);
        uint32_t bank=word(body->bytes+0x40);
        unsigned program=half(body->bytes+0x44);
        const uint32_t extra=word(view->bytes+0x17c4);
        if(bank==0&&extra!=UINT32_MAX){
            if(program!=10&&program!=100)return stop(s,0x208f84,GH_ACTOR_MOTION_UNSUPPORTED);
            bank=extra;
            program=half(view->bytes+0x17c8);
        }else if(bank==UINT32_MAX){
            bank=0;
            program=10;
        }
        if(bank>=52||program>INT16_MAX)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        s->hit_sound=(gh_motion_sound_request){.model=model,.actor=view,
            .bank=(int16_t)bank,.program=(int16_t)program};
    }
    return submit_hit_sound(s,&s->hit_sound);
}

static gh_actor_motion_status enemy_move(gh_enemy202_move *s,gh_enemy_actor *enemy,
    const gh_enemy_move_actor *rt,const gh_title_player_view *player,const void *elf,size_t elf_bytes)
{
    if(!s||!rt->entry||!rt->entry->published||!rt->entry->b.init||!rt->entry->b.work||
        !rt->entry->object.view.bytes)return GH_ACTOR_MOTION_ARGUMENT;
    gh_title_player_view *view=&rt->entry->object.view;
    uint8_t *p=view->bytes;
    if(view->size<0x1774||view->original_vtable214!=0x4479c8)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->runtime.entry&&(s->runtime.entry!=rt->entry||s->enemy!=enemy||s->player!=player||
        s->elf!=elf||s->elf_bytes!=elf_bytes))return GH_ACTOR_MOTION_BUSY;
    s->runtime=*rt;s->enemy=enemy;s->player=player;s->elf=elf;s->elf_bytes=elf_bytes;
    s->frontier=0;
    if(!s->stage){
        if((word((const uint8_t*)rt->entry->b.work+0x5d8)&0x40000000u)||
            word(rt->entry->b.init->object_pool+0xacd0)){
            s->stage=RETURNED;s->original_returned=1;
            return GH_ACTOR_MOTION_OK;
        }
        s->stage=HEADINGS;
    }
    if(s->stage==RETURNED)return GH_ACTOR_MOTION_OK;
    float actor_xyz[3],player_xyz[3];
    if(!position(view,actor_xyz)||!position(player,player_xyz)||player->size<0x63c)
        return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
    gh_actor_model *model=&rt->entry->initialize.model.model;
    if(s->stage==HEADINGS){
        /* 208C38 distinguishes only dependency 23B before the shared headings. */
        if(word(p+0x564)==0x23b)put_half(p+0x54a,half(p+0x548));
        uint32_t pc=0;
        float angle;
        gh_player_target_status status=gh_player_target_angle_002da038(elf,elf_bytes,
            actor_xyz,player_xyz,number(word(p+0x104)),number(0x40490fdb),&angle,&pc);
        if(status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x75c,bits(angle));put(p+0x760,bits(ordered_abs(angle)));
        status=gh_player_target_angle_002da038(elf,elf_bytes,player_xyz,actor_xyz,
            number(word(player->bytes+0x104)),number(0x40490fdb),&angle,&pc);
        if(status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x764,bits(angle));put(p+0x768,bits(ordered_abs(angle)));
        put_half(p+0x17c8,0);
        put(p+0x17c4,UINT32_MAX);
        put(p+0x16d4,word(p+0x16d4)&~0x4000u);
        s->stage=HIT_CHECK;
    }
    if(s->stage==HIT_CHECK){
        int no_hit=s->damage.phase?0:body_allows_no_hit(&s->runtime);
        if(no_hit<0)return stop(s,0x20a178,GH_ACTOR_MOTION_BUSY);
        if(!no_hit){
            uint32_t pc;
            const unsigned rank=heat_category(s);
            gh_actor_motion_status status=enemy?
                gh_enemy202_hit_0020a178(&s->damage,enemy,player,rank,&pc):
                gh_enemy_devil_hit_0020a178(&s->damage,rt->entry,rt->hitareas,rt->bindings->actors,player,rank,&pc);
            if(status)return stop(s,pc?pc:0x20a178,status);
            if(s->damage.reacted){
                if(!enemy&&s->damage.phase<10){
                    const uint8_t *thresholds=elf_span(elf,elf_bytes,0x3bf160,20);
                    if(!thresholds)return stop(s,0x1fa280,GH_ACTOR_MOTION_UNSUPPORTED);
                    status=gh_enemy_devil_take_post_00208be0(&s->damage,rt->entry,rt->hitareas,player,thresholds,&pc);
                    if(status)return stop(s,pc?pc:0x208f34,status);
                }
                if(s->damage.phase==5){
                    p[0x17b0]=15;
                    if(word(p+0x16c4)==6)put(p+0x16c0,0);
                    if(half(p+0x3ac)&0x100u){
                        const uint8_t *thresholds=elf_span(elf,elf_bytes,0x3bf160,20);
                        if(!thresholds)return stop(s,0x1fa280,GH_ACTOR_MOTION_UNSUPPORTED);
                        status=gh_enemy202_hit_rank_points_001fa280(enemy,thresholds,15,&pc);
                        if(status)return stop(s,pc?pc:0x1fa280,status);
                    }
                    s->damage.phase=7;
                }
                if(s->damage.phase==7){
                    const uint8_t *thresholds=elf_span(elf,elf_bytes,0x3bf160,20);
                    if(!thresholds)return stop(s,0x1fa280,GH_ACTOR_MOTION_UNSUPPORTED);
                    const uint16_t effect=s->damage.effect;
                    /* Original 208DC0 awards 24 points on Give0F. */
                    const unsigned points=effect==0x0f?24u:
                        (effect==0x2e||effect==0x12||effect==0x2a||effect==0x27||effect==0x36||effect==0x52)?16u:4u;
                    status=gh_enemy202_hit_rank_points_001fa280(enemy,thresholds,points,&pc);
                    if(status)return stop(s,pc?pc:0x1fa280,status);
                    s->damage.phase=8;
                }
                if(s->damage.phase==8){
                    if(s->damage.effect!=0x2e&&s->damage.effect!=0x36){
                        status=gh_enemy202_hit_stun_0028ed28(enemy,&pc);
                        if(status)return stop(s,pc?pc:0x28ed28,status);
                    }
                    s->damage.phase=9;
                }
                if(s->damage.phase<=10){
                    if(enemy){
                        status=gh_enemy202_impact_00271810(&s->damage,enemy,player,&pc);
                        if(status)return stop(s,pc?pc:0x271810,status);
                    }
                    status=create_hit_impact(s,rt);
                    if(status)return status;
                    if(s->damage.impact.bank==0&&s->damage.impact.index==0x40)s->damage.phase=61;
                    else s->damage.phase=(half(p+0x3ac)&0x100u)?17:11;
                }
                if(s->damage.phase==61){
                    /* 271DF4 follows the boosted world impact and precedes the Take's own sound. */
                    const gh_motion_sound_request cue={.model=model,.actor=view,.bank=0,.program=0x121};
                    status=submit_hit_sound(s,&cue);
                    if(status)return status;
                    s->damage.phase=11;
                }
                if(s->damage.phase==17){
                    /* 271D5C is a separate original cue before the ledger hit sound. */
                    const gh_motion_sound_request cue={.model=model,.actor=view,.bank=0,.program=1};
                    status=submit_hit_sound(s,&cue);
                    if(status)return status;
                    if(s->damage.impact.heat_after_critical){
                        s->impact_create=(gh_effect_create_call){0};
                        s->damage.impact.bank=0;
                        s->damage.impact.index=0x40;
                        s->damage.phase=62;
                    }else s->damage.phase=11;
                }
                if(s->damage.phase==62){
                    status=create_hit_impact(s,rt);
                    if(status)return status;
                    const gh_motion_sound_request cue={.model=model,.actor=view,.bank=0,.program=0x121};
                    status=submit_hit_sound(s,&cue);
                    if(status)return status;
                    s->damage.phase=11;
                }
                if(s->damage.phase==11){
                    status=play_hit_sound(s,model);
                    if(status)return status;
                    s->damage.phase=12;
                }
            }
        }
        s->stage=TIMERS;
    }
    if(s->stage==TIMERS){
        gh_actor_motion_status status=tick_timers(s,p);
        if(status)return status;
        s->stage=TEXTURES;
    }
    if(s->stage==TEXTURES){
        uint32_t pc;
        gh_actor_model_status status=gh_game_model_texture_scroll_0014e4d0(model,
            positive_hp(p)?number(word(p+0x5a8)):0,&s->texture_node,&pc);
        if(status)return stop(s,pc?pc:0x14e4d0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=SOLIDS;
    }
    if(s->stage==SOLIDS){
        uint32_t pc;
        if(!rt->bindings->solids||gh_collision_solid_set_enabled_001351d8(rt->bindings->solids,view,1,&pc))
            return stop(s,rt->bindings->solids&&pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x250,word(p+0x250)&~0x10000u);
        s->stage=ATTACK_AREAS;
    }
    if(s->stage==ATTACK_AREAS){
        for(;s->hit_index<5;s->hit_index++){
            unsigned i=s->hit_index+1;
            gh_hitarea_object *hit=&rt->hitareas->units[i];
            gh_title_player_view *owner;
            uint32_t pc=0;
            if(!rt->hitareas->created[i]||gh_hitarea_object_actor(hit,&owner)||
                (!hit->fallback&&owner!=view))return stop(s,0x20f310,GH_ACTOR_MOTION_UNSUPPORTED);
            put_half(hit->bytes+0x46,0);put(hit->bytes+0x4c,0);
            if(gh_hitarea_object_set_active_001fd668(hit,0,&pc))
                return stop(s,pc?pc:0x1fd668,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        put_half(p+0x434,half(p+0x434)&0xfff7);
        s->stage=PREDICT_TARGET;
    }
    if(s->stage==PREDICT_TARGET){
        put_half(p+0x434,half(p+0x434)&~8u);
        float old[3],target[3];
        memcpy(old,player->bytes+0x630,12);
        fenv_t env;
        if(!set_round(&env,FE_TONEAREST))return stop(s,0x12a578,GH_ACTOR_MOTION_CALLEE);
        int valid=1;
        for(unsigned i=0;i<3;i++){target[i]=add(mul(sub(player_xyz[i],old[i]),3),player_xyz[i]);valid&=isfinite(target[i]);}
        float distance=distance_xz(target,actor_xyz);
        if(fesetenv(&env)||!valid||!isfinite(distance))return stop(s,0x12a578,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(p+0x1690,target,12);
        put(p+0x618,bits(distance));
        put(p+0x258,(word(p+0x258)&0x7fffffffu)|((half(p+0x3ac)&0x424)?0x80000000u:0));
        put(p+0x250,word(p+0x250)&~0x40000u);
        s->stage=SELECT_AI;
    }
    if(s->stage==SELECT_AI){
        /* 3BF1A0 contains member-function descriptors, never native pointers. */
        if(!s->dispatch_pc){
            if(p[0x2f4]>=4)return stop(s,0x209654,GH_ACTOR_MOTION_UNSUPPORTED);
            const uint8_t *descriptor=elf_span(elf,elf_bytes,0x3bf1a0+8u*p[0x2f4],8);
            if(!descriptor||half(descriptor)||half(descriptor+2)!=0xffff)
                return stop(s,0x209654,GH_ACTOR_MOTION_UNSUPPORTED);
            s->dispatch_pc=word(descriptor+4);
        }
        if(s->dispatch_pc==0x24c0f0)return advance_state1_0024c0f0(s,p);
        if(s->dispatch_pc==0x25e600&&enemy)return advance_death_0025e600(s,p);
        if(s->dispatch_pc!=0x20f3c0)return stop(s,s->dispatch_pc?s->dispatch_pc:0x209654,GH_ACTOR_MOTION_UNSUPPORTED);
        s->stage=RANK_AI;
    }
    if(s->stage==RANK_AI){
        gh_actor_motion_status status=rank(s,p,player_xyz,actor_xyz);
        if(status)return status;
        s->stage=AI_CONTINUATION;
    }
    if(!s->pending_ai_pc){
        gh_actor_motion_status status=state0_schedule(s,p);
        if(status)return status;
    }
    return advance_target(s,p,actor_xyz,player_xyz);
}

gh_actor_motion_status gh_enemy202_move_00208be0(gh_enemy202_move *s,
    gh_enemy_actor *enemy,const gh_title_player_view *player,const void *elf,size_t elf_bytes)
{
    if(!enemy||enemy->stage!=13)return GH_ACTOR_MOTION_ARGUMENT;
    const gh_enemy_move_actor rt={&enemy->entry,&enemy->b,&enemy->nodes,&enemy->hitareas,
        &enemy->motion.motion,&enemy->timers};
    return enemy_move(s,enemy,&rt,player,elf,elf_bytes);
}

gh_actor_motion_status gh_enemy_devil_move_00208be0(gh_enemy202_move *s,
    const gh_enemy_move_actor *rt,const gh_title_player_view *player,const void *elf,size_t elf_bytes)
{
    if(!s||!rt||!rt->entry||!rt->bindings||!rt->nodes||!rt->hitareas||!rt->motion||!rt->timers||
        !rt->entry->object.view.bytes||rt->entry->object.view.size<0x1869||
        word(rt->entry->object.view.bytes+0x564)!=0x220||!s->devil_selector)
        return GH_ACTOR_MOTION_ARGUMENT;
    return enemy_move(s,NULL,rt,player,elf,elf_bytes);
}
