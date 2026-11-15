#include "game_scene_at_geometry.h"
#include "game_scene_at_jump.h"
#include "id_layout_rotation.h"
#include "game_player_root_delta.h"
#include "game_enemy_route_graph.h"
#include "game_player_targets.h"
#include "game_enemy_move.h"
#include "game_enemy_request.h"
#include "game_model_texture_scroll.h"
#include "game_ray_query.h"
#include "game_actor_inverse.h"
#include "sdk_rotation_trig.h"
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

gh_actor_motion_status gh_enemy202_move_bind_navigation(gh_enemy202_move *s,
    const gh_scene_nearcheck *navigation)
{
    if(!s||!navigation)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->navigation&&s->navigation!=navigation)return GH_ACTOR_MOTION_BUSY;
    s->navigation=navigation;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status navigation_lookup(gh_enemy202_move *s,gh_enemy_route_query *q,
    const float point[4],uint32_t *vertex)
{
    const gh_scene_nearcheck *nav=s->navigation;
    if(!nav)return stop(s,0x2a7cd0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(nav->vertex_count&&(!nav->source||!nav->vertices.bytes||
        nav->vertex_count>nav->vertices.size/32))return stop(s,0x2a7d94,GH_ACTOR_MOTION_BOUNDS);
    if(q->initialized&&(q->owner!=nav||memcmp(q->point,point,16)))return GH_ACTOR_MOTION_BUSY;
    if(!q->initialized){
        fenv_t env;
        if(!set_round(&env,FE_TONEAREST))return stop(s,0x1f7e28,GH_ACTOR_MOTION_CALLEE);
        unsigned worst=0;
        int valid=1;
        for(unsigned i=0;i<8;i++){q->vertices[i]=UINT32_MAX;q->distance[i]=360000;}
        for(unsigned i=0;i<nav->vertex_count;i++){
            float v[4];
            memcpy(v,nav->vertices.bytes+(size_t)i*32,16);
            float x=sub(v[0],point[0]),y=sub(v[1],point[1]),z=sub(v[2],point[2]);
            float distance=add(add(mul(x,x),mul(y,y)),mul(z,z));
            if(!isfinite(distance)||!isfinite(y)){valid=0;break;}
            if(distance<q->distance[worst]&&fabsf(y)<1){
                q->vertices[worst]=i;q->distance[worst]=distance;
                worst=0;
                for(unsigned j=1;j<8;j++)if(q->distance[worst]<q->distance[j])worst=j;
            }
        }
        if(fesetenv(&env)||!valid)return stop(s,0x2a7db8,GH_ACTOR_MOTION_UNSUPPORTED);
        /* 2A8EF0 skips <=16 elements; 2A9100 inserts using strict <. */
        for(unsigned i=1;i<8;i++){
            uint32_t index=q->vertices[i];float distance=q->distance[i];
            unsigned j=i;
            while(j&&distance<q->distance[j-1]){
                q->vertices[j]=q->vertices[j-1];q->distance[j]=q->distance[j-1];j--;
            }
            q->vertices[j]=index;q->distance[j]=distance;
        }
        q->owner=nav;memcpy(q->point,point,16);q->initialized=1;
    }
    if(!q->complete){
        for(;q->index<8;q->index++){
            if(q->index==4&&q->visibility){
                for(unsigned j=4;j<8;j++)q->vertices[j]=UINT32_MAX;
                break;
            }
            uint32_t index=q->vertices[q->index];
            if(index==UINT32_MAX)continue;
            if(index>=nav->vertex_count)return stop(s,0x2a7ed0,GH_ACTOR_MOTION_BUSY);
            gh_scene_ray_request ray={.mask=7,.flags=0x60,.segmented=1};
            memcpy(ray.from,q->point,16);
            memcpy(ray.to,nav->vertices.bytes+(size_t)index*32,16);
            int hit;
            uint32_t pc;
            if(gh_scene_ray_query_00131e10(s->runtime.bindings->actors,&ray,&hit,&pc))
                return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit)q->vertices[q->index]=UINT32_MAX;
            else q->visibility=1;
        }
        q->complete=1;
    }
    *vertex=UINT32_MAX;
    for(unsigned i=0;i<8;i++)if(q->vertices[i]!=UINT32_MAX){*vertex=q->vertices[i];break;}
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status route_waypoint_ray(gh_enemy202_move *s,
    const float origin[4],uint32_t vertex,int *hit)
{
    gh_scene_ray_request ray={.mask=7,.flags=0x60,.segmented=1};
    memcpy(ray.from,origin,16);
    memcpy(ray.to,s->navigation->vertices.bytes+(size_t)vertex*32,16);
    uint32_t pc;
    if(gh_scene_ray_query_00131e10(s->runtime.bindings->actors,&ray,hit,&pc))
        return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status route_max_depth(gh_enemy202_move *s,float length,
    float actor_y,unsigned *depth)
{
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x2a7650,GH_ACTOR_MOTION_CALLEE);
    unsigned result=16;
    if(length>=30){
        float scaled=mul(sub(length,30),0.25f);
        result=scaled>=48?64:16+(unsigned)scaled;
    }
    if(fabsf(sub(s->path_target[1],actor_y))>3)result=result*24/16;
    const uint16_t room=s->runtime.entry->b.work->room;
    if(room==0x300)result=result*20/16;
    if(room==0x700)result/=2;
    if(fesetenv(&env))return stop(s,0x2a7650,GH_ACTOR_MOTION_CALLEE);
    *depth=result;
    return GH_ACTOR_MOTION_OK;
}

/* 2A79D0 finds a visible origin edge when the ordinary vertex ray found
 * none. 2A7AC4 removes the first path entry if the path follows that edge. */
static gh_actor_motion_status route_from_origin_edge(gh_enemy202_move *s,uint8_t *p,
    const float origin[4],float length,float actor_y)
{
    gh_enemy_route_edge edge;
    uint32_t pc;
    gh_actor_motion_status status=gh_enemy_route_edge_002a8038(&s->origin_edge_query,
        s->navigation,s->runtime.bindings->actors,origin,&edge,&pc);
    if(status)return stop(s,pc?pc:0x2a8038,status);
    if(edge.pair==UINT32_MAX){
        s->route_stage=8;
        return GH_ACTOR_MOTION_OK;
    }

    put_half(p+0x5a4,(uint16_t)edge.pair);
    unsigned max_depth;
    status=route_max_depth(s,length,actor_y,&max_depth);
    if(status)return status;
    uint32_t nodes[16];
    float cost;
    memcpy(nodes,p+0x4d0,sizeof(nodes));
    status=gh_enemy_route_graph_0031be88(s->navigation,edge.vertices[0],s->target_vertex,
        max_depth,nodes,16,&cost);
    if(status)return stop(s,0x31be88,status);
    if(nodes[1]==edge.vertices[1])memmove(nodes,nodes+1,sizeof(nodes)-sizeof(*nodes));
    memcpy(p+0x4d0,nodes,sizeof(nodes));
    put(p+0x510,bits(cost));
    memcpy(p+0x520,edge.projection,12);
    p[0x530]=0;
    s->route_stage=9;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status route_clear_002a74e0(gh_enemy202_move *s,uint8_t *p,
    const float actor[3])
{
    const gh_game_init_state *init=s->runtime.entry->b.init;
    if(!init->navigation_height_initialized)
        return stop(s,0x2a7530,GH_ACTOR_MOTION_UNSUPPORTED);
    float lift=number(init->navigation_height_bits);
    if(!isfinite(lift))return stop(s,0x2a7530,GH_ACTOR_MOTION_UNSUPPORTED);
    gh_scene_ray_request ray={.mask=7,.flags=0x60,.segmented=1};
    memcpy(ray.from,actor,12);memcpy(ray.to,s->path_target,12);
    ray.from[3]=ray.to[3]=1;
    fenv_t env;
    if(!set_round(&env,FE_TONEAREST))return stop(s,0x2a7554,GH_ACTOR_MOTION_CALLEE);
    for(unsigned i=0;i<3;i++){
        float offset=i==1?lift:0;
        ray.from[i]=add(ray.from[i],offset);
        ray.to[i]=add(ray.to[i],offset);
    }
    float x=sub(ray.from[0],ray.to[0]),y=sub(ray.from[1],ray.to[1]),z=sub(ray.from[2],ray.to[2]);
    float square=add(add(mul(x,x),mul(y,y)),mul(z,z));
    if(!isfinite(square)||square<0){
        (void)fesetenv(&env);
        return stop(s,0x1f7e28,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    /* Explicit positive finite EE ChopZero reference for SQRT.S.
     * A binary32 candidate's squared value is exact in binary64. */
    float length=(float)sqrt((double)square);
    if((double)length*length>(double)square)length=nextafterf(length,0);
    if(fesetenv(&env)||!isfinite(length))
        return stop(s,0x2a7604,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->route_stage){
        memcpy(p+0x520,ray.to,12);
        p[0x530]=1;
        s->origin_edge_query=(gh_enemy_route_edge_query){0};
        s->route_stage=1;
    }
    if(s->route_stage==1){
        int hit;
        uint32_t pc;
        gh_scene_ray_status status=gh_scene_ray_query_00131e10(s->runtime.bindings->actors,&ray,&hit,&pc);
        if(status)return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
        s->route_stage=(uint8_t)(hit?2:4);
    }
    if(s->route_stage==2){
        gh_actor_motion_status status=navigation_lookup(s,&s->target_query,ray.to,&s->target_vertex);
        if(status)return status;
        s->route_stage=(uint8_t)(s->target_vertex==UINT32_MAX?4:3);
    }
    if(s->route_stage==3){
        gh_actor_motion_status status=navigation_lookup(s,&s->origin_query,ray.from,&s->origin_vertex);
        if(status)return status;
        if(s->origin_vertex==UINT32_MAX){
            status=route_from_origin_edge(s,p,ray.from,length,actor[1]);
            if(status)return status;
        }else{
            const uint8_t *origin=s->navigation->vertices.bytes+(size_t)s->origin_vertex*32;
            memcpy(p+0x520,origin,12);
            put(p+0x510,bits(length));
            p[0x530]=0;
            s->route_stage=5;
        }
    }
    if(s->route_stage==5){
        unsigned max_depth;
        gh_actor_motion_status status=route_max_depth(s,length,actor[1],&max_depth);
        if(status)return status;
        uint32_t nodes[16];
        memcpy(nodes,p+0x4d0,sizeof(nodes));
        float cost;
        status=gh_enemy_route_graph_0031be88(s->navigation,
            s->origin_vertex,s->target_vertex,max_depth,nodes,16,&cost);
        if(status)return stop(s,0x31be88,status);
        memcpy(p+0x4d0,nodes,sizeof(nodes));
        put(p+0x510,bits(cost));
        if(nodes[0]>=s->navigation->vertex_count||nodes[0]==s->target_vertex){
            memcpy(p+0x520,ray.to,12);
            p[0x530]=1;
            s->route_stage=8;
        }else{
            memcpy(p+0x520,s->navigation->vertices.bytes+(size_t)nodes[0]*32,12);
            s->route_stage=6;
        }
    }
    if(s->route_stage==6){
        uint32_t second=word(p+0x4d4),third=word(p+0x4d8);
        if(second!=s->target_vertex&&third<s->navigation->vertex_count){
            int hit;
            gh_actor_motion_status status=route_waypoint_ray(s,ray.from,third,&hit);
            if(status)return status;
            if(!hit){
                memcpy(p+0x520,s->navigation->vertices.bytes+(size_t)third*32,12);
                s->route_stage=8;
            }
        }
        if(s->route_stage==6)s->route_stage=7;
    }
    if(s->route_stage==7){
        uint32_t second=word(p+0x4d4);
        if(second<s->navigation->vertex_count){
            int hit;
            gh_actor_motion_status status=route_waypoint_ray(s,ray.from,second,&hit);
            if(status)return status;
            if(!hit)memcpy(p+0x520,s->navigation->vertices.bytes+(size_t)second*32,12);
        }
        s->route_stage=8;
    }
    if(s->route_stage==9){
        const uint32_t first=word(p+0x4d0);
        if(first<s->navigation->vertex_count){
            int hit;
            gh_actor_motion_status status=route_waypoint_ray(s,ray.from,first,&hit);
            if(status)return status;
            if(!hit){
                memcpy(p+0x520,s->navigation->vertices.bytes+(size_t)first*32,12);
                p[0x530]=1;
            }
        }
        s->route_stage=8;
    }
    if(s->route_stage==4){
        p[0x530]=1;
        put(p+0x510,bits(length));
    }
    /* Original 2A7CA0 copies the route XYZ, leaves W, returns byte530. */
    memcpy(p+0x16a0,p+0x520,12);
    if(p[0x530])put(p+0x16d4,word(p+0x16d4)|0x08000000u);
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
    if((flags&0x40000u)||(number(word(p+0x17cc))>0&&positive_hp(p)&&!(half(p+0x3ac)&0xcu)))
        return stop(s,0x2dcd50,GH_ACTOR_MOTION_UNSUPPORTED);
    p[0x620]=0;
    if(word(p+0x16c4)==0x10&&number(word(p+0x16c0))>0)
        return stop(s,0x209f50,GH_ACTOR_MOTION_UNSUPPORTED);
    if((word(p+0x16d4)&0x20000000u)&&positive_hp(p))
        return stop(s,0x209fa0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(flags&0x1000000u)return stop(s,0x209fe4,GH_ACTOR_MOTION_UNSUPPORTED);
    s->stage=RETURNED;
    s->original_returned=1;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_post_pose(gh_enemy202_move *s,uint8_t *p)
{
    const gh_enemy_move_actor *actor=&s->runtime;
    if(s->substate67_stage==17){
        /* 3130E0 zeroes the actor. The verified dependency paths, including
         * 227's table entry at 44621C, return from 271410 without using 1860. */
        uint32_t dependency=word(p+0x564);
        if(actor->entry->object.view.initial_memory!=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO||
            (dependency>=0x214&&dependency!=0x220&&dependency!=0x227)||
            word(p+0x1860))return stop(s,0x2714e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=18;
    }
    if(s->substate67_stage==18){
        float timer=number(word(p+0x17cc));
        if(!isfinite(timer))return stop(s,0x268508,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t flags=word(p+0x16d0);
        if((timer>0)!=!!(flags&0x80u))
            return stop(s,timer>0?0x306900:0x306200,GH_ACTOR_MOTION_UNSUPPORTED);
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

static gh_actor_motion_status advance_sequence_effects_0014fb48(gh_enemy202_move *s,uint8_t *p)
{
    const uint32_t dependency=word(p+0x564);
    const int devil=s->pending_ai_pc==0x212c08&&dependency==0x220;
    const gh_boot_work *work=s->runtime.entry->b.work;
    const int r105=s->pending_ai_pc==0x21a538&&r105_opening_enemy(work,p);
    const int escape=s->pending_ai_pc==0x25bcc0&&dependency==0x200&&half(p+0x640)==20;
    const int em0a=s->pending_ai_pc==0x21a538&&dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3;
    if(!r105&&((!devil&&!escape&&!em0a&&(s->pending_ai_pc!=0x21a538||
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
        if(entry[8]!=0xff)return stop(s,0x306900,GH_ACTOR_MOTION_UNSUPPORTED);
        if(flags&8u)return stop(s,0x306838,GH_ACTOR_MOTION_UNSUPPORTED);
        if(escape&&(entry[0]!=0x58||(entry[1]!=0x27&&entry[1]!=0x59)||mode||flags!=7u))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(em0a&&(entry[0]!=0x58||entry[1]!=0x1e||mode||flags!=7u))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if(devil&&(entry[0]!=0x58||entry[1]!=0x3f||mode||flags!=7u))
            return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
        if((half(p+0x434)&2u)||!bindings.storage.effects)
            return stop(s,0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!call->active||call->complete){
            const gh_effect_create_request request={.bank=entry[0],.index=entry[1],
                .actor=(flags&1u)?actor:NULL,.mode=mode,.selector=-1};
            status=gh_effect_create_actor_sequence_0014fb48(call,&request,word(p+0x114));
            if(status)return stop(s,0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        status=gh_game_effect_create_00306528(call,&bindings);
        if(status||!call->complete)return stop(s,call->frontier?call->frontier:0x306528,
            status==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE);
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status ground_sequence_0014fb48(gh_enemy202_move *s,const uint8_t *p)
{
    const unsigned count=half(p+0x3ae);
    if(count>8)return stop(s,0x14fb48,GH_ACTOR_MOTION_BOUNDS);
    const int em0a_formation=s->pending_ai_pc==0x23a950&&word(p+0x564)==0x20a&&
        !half(p+0x640)&&p[0x1650]==3&&s->enemy->b.entry.work->room==0x100;
    if(!(em0a_formation||(s->dispatch_pc==0x25e600&&(s->pending_ai_pc==0x25e790||s->pending_ai_pc==0x25ed00))||
         (s->dispatch_pc==0x24c0f0&&(s->pending_ai_pc==0x253de8||
           (s->pending_ai_pc==0x257948&&word(p+0x564)==0x200&&half(p+0x640)==20&&
            s->enemy->b.entry.work->room==0x100))))||
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
    const int counter=s->dispatch_pc==0x24c0f0&&
        (s->pending_ai_pc==0x25bcc0||s->pending_ai_pc==0x257948)&&
        word(p+0x564)==0x200&&half(p+0x640)==20&&s->enemy->b.entry.work->room==0x100;
    const int em0a_formation=s->pending_ai_pc==0x23a950&&word(p+0x564)==0x20a&&
        !half(p+0x640)&&p[0x1650]==3&&s->enemy->b.entry.work->room==0x100;
    if(!counter&&!em0a_formation&&!((s->dispatch_pc==0x25e600&&(s->pending_ai_pc==0x25e790||s->pending_ai_pc==0x25ed00||
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
    const float pitch=mul(mul(mul(number(word(p+0x1708)),.9f),-angle),.1f);
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
                s->pending_ai_pc==0x257948||s->pending_ai_pc==0x23a950)?
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
    if(dependency!=0x200&&dependency!=0x202&&dependency!=0x203&&dependency!=0x20a&&
        !r105_exchanged_enemy(work,p))
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
            s->route_stage=0;
            s->target_query=(gh_enemy_route_query){0};
            s->origin_query=(gh_enemy_route_query){0};
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

/* Original 2DDAB0 mode zero, then 272588's player-distance gate and SetGoto. */
static gh_actor_motion_status enemy_periodic_pickup_00272588(gh_enemy202_move *s,uint8_t *p,int *chosen)
{
    const gh_enemy_actor *enemy=s->enemy;
    const gh_player_target_bindings *b=&s->targets;
    const gh_room_vector *registry;
    if(b->pools!=enemy->b.entry.pools||b->actors!=enemy->b.actors||b->player!=s->player||
        !b->resolve||gh_room_registry_view(b->pools,0x754c58,&registry)||
        registry->count>registry->capacity||(registry->count&&!registry->allocation.pointer))
        return stop(s,0x2ddab0,GH_ACTOR_MOTION_UNSUPPORTED);

    float origin[3],player[3];
    if(!position(&enemy->entry.object.view,origin)||!position(s->player,player))
        return stop(s,0x2ddab0,GH_ACTOR_MOTION_BOUNDS);
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
        const float timer=number(word(object+0x66c));
        if(!isfinite(timer))return stop(s,0x1d0ae0,GH_ACTOR_MOTION_BOUNDS);
        if(timer>0)continue;

        float target[3];
        if(!position(&view,target))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_BOUNDS);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
        const float range=distance_xz(origin,target),height=sub(origin[1],target[1]);
        const float player_range=distance_xz(player,target);
        gh_scene_ray_request ray={.mask=7,.segmented=1,
            .from={origin[0],add(origin[1],0.5f),origin[2],1},
            .to={target[0],add(target[1],0.5f),target[2],1}};
        if(fesetenv(&env)||!isfinite(range)||!isfinite(height)||!isfinite(player_range))
            return stop(s,0x1f7eb0,GH_ACTOR_MOTION_CALLEE);
        if(range>100||height<-.5f||height>1.7f)continue;
        int hit;
        if(gh_scene_ray_query_00131e10(b->actors,&ray,&hit,&pc))
            return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
        if(hit)continue;

        /* A found weapon close to the player rejects this invocation. The
         * search does not resume at a later registry entry in that case. */
        if(player_range<6.25f)return GH_ACTOR_MOTION_OK;
        if(word(p+0x738))return stop(s,0x1d0408,GH_ACTOR_MOTION_UNSUPPORTED);
        put(object+0x66c,0x43e10000);
        memcpy(p+0x16b0,target,12);
        put(p+0x16c0,0x43960000);
        put(p+0x16c4,8);
        enemy_decision(p,0,0x13,0);
        *chosen=1;
        return GH_ACTOR_MOTION_OK;
    }
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status commit_attack_0026ee40(gh_enemy202_move *s,uint8_t *p,
    gh_game_mt random,unsigned heat,unsigned substate,unsigned lock,unsigned delay,int *chosen)
{
    gh_game_init_state *init=s->enemy->b.entry.init;
    uint8_t *manager=init->enemies;
    const unsigned divisor=heat==5?0:heat>=3?4:2;
    const unsigned shared=divisor?30/divisor:0;
    if(shared&&(int32_t)word(manager+0x510)<(int32_t)shared)put(manager+0x510,shared);
    put(p+0x16dc,divisor?lock/divisor:0);
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
static int r100_stamp18_armed_approach(const gh_enemy_actor *enemy,const uint8_t *p)
{
    return enemy->b.entry.work->room==0x100&&word(p+0x564)==0x202&&
        half(p+0x640)==18&&p[0x1650]==9&&
        (!(word(p+0x16d0)&0x200000u)||(p[0x2f4]==0&&p[0x2f5]==9&&p[0x2f6]==1))&&
        gh_enemy_timers_held_weapon(&enemy->timers);
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
    if(gh_player_target_angle_002da038(s->elf,s->elf_bytes,player,actor,
        number(word(s->player->bytes+0x104)),number(0x40490fdb),&turn,&pc))
        return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
    if(ordered_abs(turn)>player_limit[heat]){
        enemy->b.entry.init->random=random;
        if(!word(work+0x56c)&&heat<3)return GH_ACTOR_MOTION_OK;
        put(p+0x171c,bits(remaining));
        if(remaining>0)return GH_ACTOR_MOTION_OK;
    }else put(p+0x171c,heat<3?0x44160000u:0x43960000u);

    const uint32_t dependency=word(p+0x564);
    const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&enemy->timers);
    if(word(p+0x6ec)||held){
        if(word(p+0x6ec)||!held->weapon)
            return stop(s,0x263560,GH_ACTOR_MOTION_UNSUPPORTED);
        const int stamp18=r100_stamp18_armed_approach(enemy,p);
        const int r101=held->weapon->kind==0x361&&enemy->b.entry.work->room==0x101&&
            dependency==0x203&&half(p+0x640)==13&&!p[0x1650];
        if(!stamp18&&!r101)return stop(s,0x263560,GH_ACTOR_MOTION_UNSUPPORTED);
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
                !word(p+0x6ec)&&!gh_enemy_timers_held_weapon(&enemy->timers)&&!word(p+0x6f0)){
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
        ((room==0x100&&stamp==19)||(room==0x101&&stamp==13&&p[0x1650]==0));
}

static gh_actor_motion_status em0a_attack_choice_002634d0(gh_enemy202_move *s,
    uint8_t *p,float distance,unsigned heat,int *chosen)
{
    gh_enemy_actor *enemy=s->enemy;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    if(half(p+0x640)||p[0x1650]!=3||word(p+0x6ec)||
       gh_enemy_timers_held_weapon(&enemy->timers))
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

static gh_actor_motion_status attack_choice_00262aa8(gh_enemy202_move *s,uint8_t *p,int *chosen)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_game_init_state *init=enemy->b.entry.init;
    const uint8_t *work=(const uint8_t*)enemy->b.entry.work,*manager=init->enemies;
    const float distance=number(word(p+0x618)),face=number(word(p+0x760));
    const uint32_t route=word(p+0x16d4);
    *chosen=0;
    const int ordinary=word(p+0x564)==0x200&&enemy->b.entry.work->room==0x100&&
        (distance<=144.0f||!(word(p+0x16d0)&0x200000u));
    /* The gate actors keep their approach route after the override timer
     * expires. 262AA8 then proceeds to the ordinary attack checks. */
    if(em03_approach_range(s,p,distance)||ordinary||r100_stamp18_armed_approach(enemy,p)||
       r105_opening_enemy(enemy->b.entry.work,p)||r105_gate_approach(enemy->b.entry.work,p)){
        if(!isfinite(distance)||distance<0||!isfinite(face))
            return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
        return attack_choice_common(s,p,distance,chosen);
    }
    const uint32_t dependency=word(p+0x564);
    /* The real R100 EM02 flank uses the common home-range path. Its
     * attack bit is clear at 26314C, before the dependency dispatch. */
    const int far_em02=dependency==0x202&&half(p+0x640)==4&&p[0x1650]==2&&
        distance>144.0f&&!(word(p+0x16d0)&0x200000u);
    if(enemy->b.entry.work->room==0x100&&(dependency==0x200||dependency==0x203||far_em02))
        return attack_choice_ordinary_far(s,p,distance,face,chosen);
    if(word(p+0x564)!=0x20a||enemy->b.entry.work->room!=0x100)
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
        if(half(p+0x640)||p[0x1650]!=3)
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

static gh_actor_motion_status enemy_door_ray_0024bb68(gh_enemy202_move *s,uint8_t *p);

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
            unsigned next=6;
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
                (word(p+0x16d0)&0x200000u))
                return stop(s,0x2630f0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(word(p+0x16d8)>=3&&word(p+0x5f4))
                return stop(s,0x212864,GH_ACTOR_MOTION_UNSUPPORTED);
            if(word(p+0x16d8)<3)put(p+0x5f4,0);
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
    if(((word((const uint8_t *)work+0x210)^word(p+0x17d0))&7u)!=0)return GH_ACTOR_MOTION_OK;

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
    const int player_alive=(int16_t)half(s->player->bytes+0x54a)>0;
    const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(actor_state->timers);
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
            if(!r105_exchanged_enemy(actor_state->entry->b.work,p))
                return stop(s,0x211c08,GH_ACTOR_MOTION_UNSUPPORTED);
            /* The original exchange uses the default EM00 clip pair. */
            /* fall through */
        case 0x200:offset=0x98;break;
        case 0x202:case 0x203:offset=0x648;break;
        case 0x20a:offset=0x828;break;
        case 0x220:offset=0x1bc0;break;
        default:return stop(s,0x211c08,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(turning){
            if(dependency!=0x20a&&dependency!=0x200&&dependency!=0x203&&dependency!=0x220&&dependency!=0x227)
                return stop(s,0x2120e8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!s->substate67_motion.active){
                float actor[3],target[3],turn;
                if(!position(&actor_state->entry->object.view,actor)||!position(s->player,target)||
                    gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                        number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            /* This dependency selects the same clip for either turn sign. */
            offset=dependency==0x200||dependency==0x227?0xb0:dependency==0x203?0x650:dependency==0x220?0x1bc8:0x830;
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
            gh_actor_motion_status status=devil_far_idle_00262aa8(s,p);
            if(status)return status;
        }
        s->substate67_stage=5;
        return advance_state0_tail(s,p);
    }
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
                if(r100_em0a||r100_stamp18_armed_approach(s->enemy,p)||em03_approach_range(s,p,distance)||
                    r105_opening_enemy(actor_state->entry->b.work,p)||
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
        gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p);
        if(status)return status;
        s->substate67_stage=r105_opening_enemy(actor_state->entry->b.work,p)?102:5;
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
            case 0x200:offset=positive?0xe4:0xdc;break;
            case 0x202:case 0x203:offset=positive?0x684:0x67c;break;
            case 0x20a:offset=positive?0x864:0x85c;break;
            default:return stop(s,0x214298,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            const gh_object_view *resource=&enemy->entry.resource;
            if(!resource->bytes||!resource->file_extent_known||
                resource->file_extent<offset+8||resource->file_extent>resource->size)
                return stop(s,0x2146a8,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+offset),word(resource->bytes+offset+4),
                0,5,p[0x17c3]?2:0,1);
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
    const int em27=r105_exchanged_enemy(work,p);
    const int em0a=work->room==0x100&&dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3;

    if(!s->substate67_stage){
        if(!em27&&!em0a&&dependency!=0x200&&dependency!=0x202&&dependency!=0x203)
            return stop(s,0x2147d0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]==0?SELECT:p[0x2f6]==1?TURN:ATTACK;
    }
    if(s->substate67_stage==SELECT){
        if(word(p+0x6ec))return stop(s,0x214ba0,GH_ACTOR_MOTION_UNSUPPORTED);
        /* EM27 selects the default resource pair at 21487C and the normal tail at 214C94. */
        size_t offset=em0a?0x84c:dependency==0x200||em27?0xcc:0x66c;
        if(gh_enemy_timers_held_weapon(&enemy->timers))offset=0x464;
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
                motion,&motion->b,word(row),word(row+4),0,10,p[0x17c3]?2:0,!em27&&!em0a);
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
    if(!s->substate67_stage){
        const int return_home=dependency==0x203&&work->room==0x100&&
            p[0x1650]==5&&word(p+0x16c4)==6;
        if(!return_home&&(p[0x1650]!=2||(dependency!=0x200&&dependency!=0x202&&dependency!=0x203)))
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
            if(word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
                return stop(s,turning?0x211978:0x21151c,GH_ACTOR_MOTION_UNSUPPORTED);
            const gh_object_view *resource=&enemy->entry.resource;
            const unsigned offset=dependency==0x200?(turning?0xb0:0x98):(turning?0x650:0x648);
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
                0,10,p[0x17c3]?2:0,1);
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
            s->route_stage=0;
            s->target_query=(gh_enemy_route_query){0};s->origin_query=(gh_enemy_route_query){0};
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
    const int held=gh_enemy_timers_held_weapon(&enemy->timers)!=NULL;
    const int periodic=((word((const uint8_t *)work+0x210)^word(p+0x17d0))&7u)==0;
    const float distance=number(word(p+0x618));
    uint32_t pc=0;
    const int r105_exchange=r105_exchanged_enemy(work,p);
    const unsigned stamp=half(p+0x640);
    const int r105_em0a=work->room==0x105&&dependency==0x20a&&appearance==0&&(stamp==3||stamp==5);
    const int em0a_appearance5=dependency==0x20a&&appearance==5;
    const int em0f_appearance0=dependency==0x20f&&appearance==0;
    const int em27_appearance0=dependency==0x227&&appearance==0;
    const int common_appearance1=appearance==1&&(dependency==0x200||dependency==0x203);
    const int appearance1_em03=dependency==0x203&&appearance==1&&
        ((work->room==0x105&&stamp==6)||(work->room==0x101&&stamp==0));
    const int r101_appearance4=work->room==0x101&&appearance==4&&
        ((stamp==5&&dependency==0x202)||(stamp==9&&dependency==0x227)||
         (stamp==12&&dependency==0x200));
    const int r105_entry=r105_exchange||r105_em0a||appearance1_em03||r101_appearance4;
    const int r105_stamp8=work->room==0x105&&stamp==8&&dependency==0x202&&appearance==0;
    if(!s->substate67_stage){
        if(r105_stamp8&&held)return stop(s,0x210590,GH_ACTOR_MOTION_UNSUPPORTED);
        if(r105_entry&&(p[0x2f6]>1||held))
            return stop(s,p[0x2f6]>1?0x210a58:0x210590,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!(((appearance==0||appearance==5)&&(dependency==0x200||dependency==0x203))||
            (appearance==9&&dependency==0x202)||em0a_appearance5||em0f_appearance0||em27_appearance0||
            common_appearance1||r105_entry||r105_stamp8))
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
            if(enter&&word(p+0x6ec))return stop(s,0x210590,GH_ACTOR_MOTION_UNSUPPORTED);
            if(enter&&!position(actor,s->path_target))
                return stop(s,0x2105e0,GH_ACTOR_MOTION_UNSUPPORTED);
            const gh_object_view *resource=&enemy->entry.resource;
            unsigned offset,sequence;
            if(!enter){
                if(common_appearance1)offset=0x404;
                else if(em0a_appearance5)offset=0xbe0;
                else if(em0f_appearance0)offset=0xfcc;
                else if(appearance==5)offset=dependency==0x203?0x7bc:0x42c;
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
            }else if(r105_em0a){
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
                0,enter?0:10,p[0x17c3]?2:0,1);
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
                s->route_stage=0;
                s->target_query=(gh_enemy_route_query){0};
                s->origin_query=(gh_enemy_route_query){0};
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
                s->route_stage=0;
                s->target_query=(gh_enemy_route_query){0};s->origin_query=(gh_enemy_route_query){0};
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

static gh_actor_motion_status enemy_door_ray_0024bb68(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    const gh_boot_work *work=enemy->b.entry.work;
    if(((word((const uint8_t *)work+0x210)^word(p+0x17d0))&7u)!=0)
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
    if(!enemy->b.actors||gh_scene_ray_query_00131e10(enemy->b.actors,&ray,&hit,&pc))
        return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!hit)return GH_ACTOR_MOTION_OK;
    if(!report.attributes||report.attribute_bytes<8)
        return stop(s,0x24bcbc,GH_ACTOR_MOTION_BOUNDS);
    if(word(report.attributes+4)&0x20u)
        return stop(s,0x24bcd4,GH_ACTOR_MOTION_UNSUPPORTED);
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
    const int r105=r105_opening_enemy(work,p);
    const int em0a=work->room==0x100&&dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3;
    if((!r105&&!em0a&&((dependency!=0x200&&dependency!=0x203)||work->room!=0x100))||!isfinite(distance)||distance<0)
        return stop(s,0x2130c8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]==0?BIND:p[0x2f6]==1?TURN:CHECK_PLAYER;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            const size_t row=em0a?0x830:dependency==0x203?0x650:0xb0;
            if(word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
                return stop(s,0x2133a4,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x213390,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,10,p[0x17c3]?2:0,!r105&&!em0a);
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
            enemy_decision(p,0,6,0);
            s->substate67_stage=5;
        }else{
            gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p);
            if(status)return status;
            s->substate67_stage=r105?CHECK_JUMP:5;
        }
    }
    if(s->substate67_stage==CHECK_JUMP){
        gh_actor_motion_status status=enemy_door_jump_0024bda0(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_substate7_00213ea8(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=200, TURN, SAMPLE, CHOOSE_ENDED, ROTATE, TRANSLATE,
        CHECK_DISTANCE, CHOOSE_FAR, CHECK_LEVEL, ATTACK };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_boot_work *work=enemy->b.entry.work;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    const float distance=number(word(p+0x618));
    uint32_t pc=0;
    const uint32_t dependency=word(p+0x564);
    const unsigned stamp=half(p+0x640);
    const int em0a=dependency==0x20a&&!stamp&&p[0x1650]==3;
    const int r100=work->room==0x100&&
        (em0a||(dependency==0x203&&stamp==19)||(dependency==0x200&&(stamp==20||stamp==8)));
    if((!r100&&!r105_opening_enemy(work,p))||
        !isfinite(distance)||distance<0)
        return stop(s,0x213ea8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]==0?BIND:p[0x2f6]==1?TURN:CHECK_DISTANCE;
    }
    if(s->substate67_stage==BIND){
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            /* The 435720 jump table sends EM27 to the same default as EM00. */
            const size_t row=em0a?0x838:dependency==0x203?0x658:0xb8;
            if(word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
                return stop(s,0x2140e8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2140e8,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,10,p[0x17c3]?2:0,0);
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
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?CHOOSE_ENDED:ROTATE;
    }
    if(s->substate67_stage==CHOOSE_ENDED){
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
        s->substate67_stage=(half(p+0x3ac)&0x20u)?5:CHECK_DISTANCE;
    }
    if(s->substate67_stage==CHECK_DISTANCE)
        s->substate67_stage=!p[0x2f7]&&distance>9.0f?CHOOSE_FAR:CHECK_LEVEL;
    if(s->substate67_stage==CHOOSE_FAR){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
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
        gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
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
    const int stamp19=dependency==0x203&&half(p+0x640)==19&&!p[0x1650];
    const int em0a=dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3;
    const int r105=r105_opening_enemy(work,p)||r105_battle_enemy(work,p);
    const int r101=work->room==0x101&&dependency==0x203&&half(p+0x640)==13&&!p[0x1650];
    if((!r101&&!r105&&((dependency!=0x200&&!stamp19&&!em0a)||work->room!=0x100))||!isfinite(distance)||distance<0)
        return stop(s,0x213710,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]==0?72:p[0x2f6]==1?74:79;
    }
    if(s->substate67_stage==72){
        const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&enemy->timers);
        if(word(p+0x6ec)||(held&&(!r101||!held->weapon||held->weapon->kind!=0x361)))
            return stop(s,0x213a5c,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            /* Table 435504 selects the EM02 and EM0A rows. EM27 uses the default. */
            const size_t row=held?0x4b8:em0a?0x840:(dependency==0x202||dependency==0x203)?0x660:0xc0;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x213a58,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,10,p[0x17c3]?2:0,!r105);
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
        gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p);
        if(status)return status;
        /* 24BDA0's periodic Jump probe is enabled outside R100. */
        s->substate67_stage=r105?81:5;
    }
    if(s->substate67_stage==81){
        gh_actor_motion_status status=enemy_door_jump_0024bda0(s,p);
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
    const int em00=dependency==0x200&&((stamp==10&&p[0x1650]==5)||(stamp==3&&p[0x1650]==2));
    const int em02_home=dependency==0x202&&stamp==4&&p[0x1650]==2;
    const int em0a=dependency==0x20a&&stamp==0&&p[0x1650]==3;
    const int em0a_formation=em0a&&mode==9;
    if(!r105&&(work->room!=0x100||
       (!em0a_formation&&(mode!=6||(dependency!=0x203&&!em00&&!em02_home&&!em0a)))))
        return stop(s,0x218c48,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,(word(p+0x16d0)&~0x8000u)|0x400u);
        if(p[0x2f6]>1||p[0x2f7])
            return stop(s,0x218ca8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=p[0x2f6]?95:94;
    }
    if(s->substate67_stage==94){
        if(word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
            return stop(s,0x219364,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            /* Original mode2 and mode6 use the same dependency table.
             * EM00 reaches default 218D30, EM02 reaches 218DA4. */
            const size_t row=dependency==0x200?0xc0:em0a?0x840:0x660;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x219358,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,10,p[0x17c3]?2:0,1);
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
        memcpy(target,p+0x16a0,12);
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
            if(r105){
                put(p+0x16d4,word(p+0x16d4)|1u);
                s->substate67_stage=104;
            }else{
                if(em0a_formation)enemy_decision(p,0,0x66,2);
                else{
                    enemy_decision(p,0,3,0);
                    put(p+0x16d0,word(p+0x16d0)&~2u);
                }
                s->substate67_stage=5;
            }
        }else s->substate67_stage=r105?101:100;
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
        gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p);
        if(status)return status;
        s->substate67_stage=r105?103:5;
    }
    if(s->substate67_stage==103){
        gh_actor_motion_status status=enemy_door_jump_0024bda0(s,p);
        if(status)return status;
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original 214DE0 rear evade, default EM00 EC/F0 row. */
static gh_actor_motion_status advance_em00_evade_00214de0(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=240, TURN, SAMPLE, CHOOSE, ATTACK, RESET, ROTATE, TRANSLATE };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->b.entry.work;
    const unsigned stamp=half(p+0x640);
    uint32_t pc=0;
    if(work->room!=0x100||word(p+0x564)!=0x200||(stamp!=8&&stamp!=20)||p[0x2f7]!=1)
        return stop(s,0x214de0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        if(p[0x2f6]>1)return advance_state0_tail(s,p);
        s->substate67_stage=p[0x2f6]?TURN:BIND;
    }
    if(s->substate67_stage==BIND){
        if(word(p+0x6e4)||word(p+0x6e8))return stop(s,0x26a938,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x6f0))return stop(s,0x1d04d0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x708))return stop(s,0x1d05c0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(p[0x17c3])return stop(s,0x276468,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0xf4||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x2151e4,GH_ACTOR_MOTION_BOUNDS);
            put(p+0x604,0x3f19999a);
            put(p+0x600,0x40a00000);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+0xec),word(resource->bytes+0xf0),0,3,0x40,1);
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
    if(s->substate67_stage==CHOOSE){
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
static gh_actor_motion_status body_give_00262750(gh_enemy202_move *,uint8_t *);

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
        gh_actor_motion_status status=body_give_00262750(s,p);
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
    if(work->room!=0x100||p[0x2f7]||
       !((dependency==0x203&&stamp==19)||(dependency==0x200&&(stamp==8||stamp==20))))
        return stop(s,0x23afd8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        if(p[0x2f6]>3)return stop(s,0x23afd8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!p[0x2f6])p[0x1864]=0;
        s->substate67_stage=p[0x2f6]==1?TURN:p[0x2f6]==3?TICK_EXIT:OPEN;
    }
    if(s->substate67_stage==OPEN){
        /* 23B050 underflows for dependency 0200 into the 39C/3A0 pair. */
        const size_t offset=p[0x2f6]?0x3a4:(dependency==0x200?0x39c:0x7c4);
        const gh_object_view *resource=&enemy->entry.resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<offset+8||
           resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x23afd8,GH_ACTOR_MOTION_BOUNDS);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=resource->bytes+offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(row),word(row+4),0,p[0x2f6]?10:3,p[0x17c3]?2:0,1);
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
        put(p+0x604,0);
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
        /* 274150 returns zero in room 0100. */
        if(timer>0)enemy_decision(p,0,0x89,0);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Shared 23B968/23C620 default EM02 motion tables, no-held-weapon phase0/1. */
static gh_actor_motion_status advance_substate6b_or_6c(gh_enemy202_move *s,uint8_t *p)
{
    enum { SELECT_MOTION=101, START_MOTION, TURN_TO_PLAYER, STEP_MOTION,
           SELECT_AFTER, ROOT_ROTATION, ROOT_TRANSLATION, CUE, CUE_SOUND };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_boot_work *work=enemy->b.entry.work;
    gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    const uint32_t dependency=word(p+0x564);
    const unsigned stamp=half(p+0x640);
    const int opening_actor=(dependency==0x203&&stamp==19)||(dependency==0x200&&stamp==20);
    const int substate6c=s->pending_ai_pc==0x23c620;
    /* Patched SLUS 23C680 underflows dep0200 into the same default table.
     * The stamp8 unarmed bind reaches 23D134 with t1 zero. */
    const int stamp8_6c=substate6c&&dependency==0x200&&stamp==8&&p[0x1650]==5;
    const int common_6b=!substate6c&&p[0x1650]==2&&((dependency==0x200&&stamp==3)||(dependency==0x202&&stamp==4)||(dependency==0x203&&stamp==5));
    /* Original 6C dispatch uses default 23C6A4 for the R105 trio:
     * EM00 unsigned underflow, EM03 entry 43AEE4, EM27 entry 43AF74. */
    const int r105=r105_opening_enemy(work,p);
    const int r101_held=!substate6c&&work->room==0x101&&dependency==0x203&&stamp==13&&!p[0x1650];
    const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&enemy->timers);
    if(r101_held&&(!held||!held->weapon||held->weapon->kind!=0x361||word(p+0x6ec)))
        return stop(s,0x23c308,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!r101_held&&!r105&&(work->room!=0x100||((!opening_actor||p[0x1650])&&!common_6b&&!stamp8_6c)))
        return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        if(p[0x2f6]>1)return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!p[0x2f6])p[0x1864]=0;
        s->substate67_stage=p[0x2f6]?TURN_TO_PLAYER:SELECT_MOTION;
    }
    if(s->substate67_stage==SELECT_MOTION){
        const gh_object_view *resource=&enemy->entry.resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x3f4||
            resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,substate6c?0x23c6a4:0x23ba2c,GH_ACTOR_MOTION_BOUNDS);
        uint32_t random;
        if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&random))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        static const uint16_t rows_6b[]={0x3c4,0x3cc,0x3d4,0x3dc,0x3ec};
        if(substate6c){
            s->reaction_resource_offset=(uint16_t)(0x3c4+8*(random%6u));
            if(p[0x2f7])s->reaction_resource_offset=0x3e4;
        }else{
            /* 23C308 overrides the default MT-selected resource when the
             * actor has a weapon. The default dispatch still draws once. */
            s->reaction_resource_offset=r101_held?0x3e4:rows_6b[random%5u];
        }
        s->substate67_stage=START_MOTION;
    }
    if(s->substate67_stage==START_MOTION){
        if(!r101_held&&(word(p+0x6ec)||held))
            return stop(s,substate6c?0x23d0f4:0x23c308,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=enemy->entry.resource.bytes+s->reaction_resource_offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(row),word(row+4),0,10,p[0x17c3]?2:0,!(common_6b||r105||stamp8_6c));
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
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended?SELECT_AFTER:ROOT_ROTATION;
    }
    if(s->substate67_stage==SELECT_AFTER){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
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
            if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&random))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            const uint16_t program=(uint16_t)(word(p+0x424)+(random&1u?10u:11u));
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

static gh_actor_motion_status enemy_attack_area_00260b30(gh_enemy202_move *s,uint8_t *p)
{
    const unsigned hit_flags=half(p+0x3ac)&3u;
    if(!hit_flags)return GH_ACTOR_MOTION_OK;
    const unsigned params=hit_flags&2u?0x1884:0x1870;
    const unsigned profile=word(p+params);
    gh_enemy_actor *enemy=s->enemy;
    const int weapon_swing=profile==0x12&&s->pending_ai_pc==0x233738&&
        enemy->b.entry.work->room==0x101&&word(p+0x564)==0x203&&half(p+0x640)==13;
    const int long_kick=profile==12&&enemy->b.entry.work->room==0x100&&
        word(p+0x564)==0x20a&&!half(p+0x640)&&p[0x1650]==3&&
        !p[0x2f4]&&p[0x2f5]==0x15&&p[0x2f6]==1&&word(p+params+4)==15&&
        word(p+params+8)==0x37&&word(p+params+12)==UINT32_MAX;
    const int reverse_kick=profile==0x14&&enemy->b.entry.work->room==0x100&&
        word(p+0x564)==0x20a&&!half(p+0x640)&&p[0x1650]==3&&
        s->pending_ai_pc==0x231ac0&&!word(p+params+4)&&
        word(p+params+8)==0x3e&&word(p+params+12)==UINT32_MAX;
    if((profile>2&&!weapon_swing&&!long_kick&&!reverse_kick)||
       (profile==1&&word(p+0x564)!=0x200&&!r105_exchanged_enemy(enemy->b.entry.work,p)))
        return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    gh_hitarea_object *give=&enemy->hitareas.units[1];
    const uint8_t *node;
    gh_collision_pair_shape shapes[4];
    for(unsigned i=0;i<4;i++){
        gh_title_player_view *actor;
        if(!enemy->hitareas.created[i+1]||give[i].fallback||
           gh_hitarea_object_actor(&give[i],&actor)||actor!=&enemy->entry.object.view||
           gh_hitarea_node_at(&give[i],0,&node,&shapes[i]))
            return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    if(word(shapes[3].record+0xe0)!=4||word(shapes[3].record+0x1e0)!=0x44bb20)
        return stop(s,0x1fd820,GH_ACTOR_MOTION_UNSUPPORTED);
    float euler[3],rotation[16];
    uint32_t pc=0;
    if(profile!=1&&!long_kick){
        memcpy(euler,p+0x100,sizeof(euler));
        if(gh_actor_matrix_rotation_00147728(euler,0,rotation,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x1fd338,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    fenv_t env;
    const float scale=number(word(p+0x114)),power_rate=number(word(p+0x76c));
    if((profile!=1&&(!isfinite(scale)||scale==0))||!isfinite(power_rate)||!set_round(&env,FE_TOWARDZERO))
        return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    int32_t power=(int32_t)word(p+params+4);
    if(word(p+0x16ec))power+=power/2;
    const float scaled_power=mul((float)power,power_rate);
    volatile float reciprocal=profile==1?1.0f:1.0f/scale;
    const float height_scale=weapon_swing?0.5f:reverse_kick?0.0f:long_kick?0.6f:
        profile==2?0.7f:1.0f;
    const float height=mul(reciprocal,height_scale);
    const uint8_t *work=(const uint8_t *)enemy->b.entry.work;
    if(!isfinite(scaled_power)||scaled_power < -2147483648.0f||scaled_power >= 2147483648.0f){
        (void)fesetenv(&env);
        return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    power=(int32_t)scaled_power;
    if(word(work+0x594)&0x800u)power=(int32_t)mul((float)power,0.8f);
    if(fesetenv(&env)||!isfinite(height))return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    const float front[]={0,1.3f,0.75f},base[]={0,1.3f,0};
    const float box[]={0,height,reverse_kick?1.5f:long_kick?0.0f:0.75f};
    if(gh_collision_pair_shape_offset_002bb258(&shapes[3],base,&pc)||
       gh_collision_pair_shape_offset_002bb258(&shapes[0],front,&pc)||
       gh_collision_pair_shape_offset_002bb258(&shapes[1],front,&pc)||
       (profile!=1&&gh_collision_pair_shape_offset_002bb258(&shapes[3],box,&pc)))
        return stop(s,pc?pc:0x1fd888,GH_ACTOR_MOTION_UNSUPPORTED);
    if(gh_hitarea_object_set_active_001fd668(&give[3],1,&pc))
        return stop(s,pc?pc:0x1fd668,GH_ACTOR_MOTION_UNSUPPORTED);
    put(p+0x16d0,word(p+0x16d0)|0x100000u);
    for(unsigned i=0;i<4;i++){
        put(give[i].bytes+0x40,word(p+params+12));
        put_half(give[i].bytes+0x44,half(p+params+16));
    }
    put_half(give[3].bytes+0x46,(uint16_t)word(p+params+8));
    put(give[3].bytes+0x4c,(uint32_t)power);
    if(profile==1||long_kick){
        const gh_game_math_globals *math=enemy->hitareas.b.math;
        if(!math||!math->initialized)return stop(s,0x1fd308,GH_ACTOR_MOTION_UNSUPPORTED);
        memcpy(give[3].bytes+0x10,math->zero_vector,12);
    }else memcpy(give[3].bytes+0x10,rotation+8,12);
    const int short_box=profile==1||long_kick;
    const float size[]={
        weapon_swing?0.3f:reverse_kick?0.5f:short_box?1.0f:0.7f,
        reverse_kick?2.0f:short_box?0.5f:1.0f,
        (reverse_kick||weapon_swing)?2.0f:1.0f
    };
    if(gh_collision_pair_box_size_002baaa8(&shapes[3],size,&pc))
        return stop(s,pc?pc:0x1fd820,GH_ACTOR_MOTION_UNSUPPORTED);
    return GH_ACTOR_MOTION_OK;
}

/* 21FD50's original unarmed phase0 and phase1 use the bound dependency
 * resource, two Give rows, and the yaw shadow at actor+600. */
static gh_actor_motion_status advance_r105_unarmed16_0021fd50(gh_enemy202_move *s,uint8_t *p)
{
    enum { MOTION=200, TRACK, STEP, ROTATE, TRANSLATE, FINISH };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->entry.b.work;
    if(!r105_exchanged_enemy(work,p)||word(p+0x6ec)||
       gh_enemy_timers_held_weapon(&enemy->timers)||p[0x2f6]>1)
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
            put(p+0x5f0,50);
            stamp19_attack_parameters(p,5,0,0x3d);
            put(p+0x1888,10);
            put(p+0x188c,0x37);
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
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x144||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x21ff40,GH_ACTOR_MOTION_BOUNDS);
            const size_t sequence=heat==5?0x140:0x13c;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+0x138),word(resource->bytes+sequence),
                0,10,p[0x17c3]?2:0,0);
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
            if(heat_category(s)>=3)return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(half(p+0x3ac)&3u)put(p+0x5fc,1);
            if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        }
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_r101_armed47_00233738(gh_enemy202_move *s,uint8_t *p)
{
    enum { ARMED_MOTION=180, ARMED_EFFECT, ARMED_TRACK, ARMED_STEP,
        ARMED_ROTATION, ARMED_TRANSLATION, ARMED_FINISH };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&enemy->timers);
    if(enemy->b.entry.work->room!=0x101||word(p+0x564)!=0x203||half(p+0x640)!=13||
       p[0x1650]||word(p+0x6ec)||!held||!held->weapon||held->weapon->kind!=0x361||
       p[0x2f6]>1||p[0x2f7])
        return stop(s,0x233738,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t pc=0;
    if(!s->substate67_stage){
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
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
        if(p[0x2f6])s->substate67_stage=ARMED_TRACK;
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
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x23380c,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,3,p[0x17c3]?2:0,0);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        put(p+0x5f0,20);
        for(unsigned offset=0x1870;offset<=0x1884;offset+=0x14){
            put(p+offset,0x12);
            put(p+offset+4,row==0x474?60:40);
            put(p+offset+8,row==0x474?0x3b:0x3a);
            put(p+offset+12,0);
            put_half(p+offset+16,0x140);
        }
        s->substate67_stage=ARMED_EFFECT;
    }
    if(s->substate67_stage==ARMED_EFFECT){
        gh_effect_create_bindings bindings=s->impact_bindings;
        if(!bindings.storage.effects||bindings.work!=enemy->entry.b.work||
           bindings.random!=&enemy->entry.b.init->random)
            return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
        bindings.model=&held->model;
        gh_effect_create_call *effect=&s->death_effect_create;
        const unsigned heat=heat_category(s);
        if(!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!effect->active){
            const int sweep=s->reaction_resource_offset==0x474;
            const uint32_t index=heat==5?(sweep?0x80:0x7f):(sweep?0x7b:0x7a);
            put(p+0x574,2);
            put_half(p+0x570,0);
            if(gh_effect_create_r101_weapon_002a8608(effect,&held->weapon->object.view,
                &enemy->entry.object.view,index,word(p+0x114)))
                return stop(s,0x2a8608,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(gh_game_effect_create_00306528(effect,&bindings)||!effect->complete)
            return stop(s,effect->frontier?effect->frontier:0x306528,GH_ACTOR_MOTION_UNSUPPORTED);
        static const uint8_t tracking_ticks[]={0,15,16,18,18,20};
        put(p+0x5f0,tracking_ticks[heat]);
        put(p+0x5fc,0);
        ++p[0x2f6];
        s->substate67_stage=ARMED_TRACK;
    }
    if(s->substate67_stage==ARMED_TRACK){
        const uint32_t tracking=word(p+0x5f0);
        if(tracking){
            float from[3],to[3],turn;
            const float yaw=number(word(p+0x104)),rate=number(word(p+0x5a8));
            const float frame_rate=number(word((const uint8_t *)enemy->entry.b.work+0x55c));
            fenv_t env;
            if(!position(&enemy->entry.object.view,from)||!position(s->player,to)||
               !isfinite(rate)||!isfinite(frame_rate)||!set_round(&env,FE_TOWARDZERO))
                return stop(s,0x2a7498,GH_ACTOR_MOTION_UNSUPPORTED);
            const float cap=mul(mul(rate,number(0x3e490fdb)),frame_rate);
            const gh_player_target_status status=gh_player_target_angle_002da038(s->elf,s->elf_bytes,
                from,to,yaw,cap,&turn,&pc);
            float next=status?0:add(yaw,turn);
            if(fesetenv(&env)||status)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f0,tracking-1);
            put(p+0x104,bits(next));
        }
        s->substate67_stage=ARMED_STEP;
    }
    if(s->substate67_stage==ARMED_STEP){
        int ended;
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        motion->sequence_held_weapon=&held->weapon->object.view;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
            GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        motion->sequence_held_weapon=NULL;
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state71_exit=(uint8_t)ended;
        s->substate67_stage=ARMED_ROTATION;
    }
    if(s->substate67_stage==ARMED_ROTATION){
        if(gh_player_root_rotation_0014f9f0(&enemy->entry.object.view,enemy->entry.b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ARMED_TRANSLATION;
    }
    if(s->substate67_stage==ARMED_TRANSLATION){
        if(gh_player_root_translation_0014f928(&enemy->entry.object.view,enemy->entry.b.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ARMED_FINISH;
    }
    if(s->substate67_stage==ARMED_FINISH){
        if(s->state71_exit){
            gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
            if(status)return status;
        }else{
            const unsigned hit_flags=half(p+0x3ac);
            gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
            if(status)return status;
            if(heat_category(s)>=3)return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit_flags&3u)put(p+0x5fc,1);
            if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        }
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_attack_0021a538(gh_enemy202_move *s,uint8_t *p)
{
    enum { SELECT_ATTACK=160, OPEN_ATTACK, TRACK_ATTACK, STEP_ATTACK,
        ATTACK_ROTATION, ATTACK_TRANSLATION, FINISH_ATTACK, CHOOSE_REVERSE };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_game_init_state *init=enemy->b.entry.init;
    const gh_boot_work *work=enemy->b.entry.work;
    const uint32_t dependency=word(p+0x564);
    const int r105=r105_opening_enemy(work,p);
    const int em27=r105&&dependency==0x227;
    const int em00_family=dependency==0x200||em27;
    const int em0a=dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3;
    const int reverse=work->room==0x100&&em0a&&s->pending_ai_pc==0x231ac0;
    uint32_t pc=0;
    if((s->pending_ai_pc==0x231ac0&&!reverse)||(!r105&&(work->room!=0x100||(!em00_family&&!em0a&&(dependency!=0x203||half(p+0x640)!=19))))||
        word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
        return stop(s,0x21a538,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        if(p[0x2f6]>1||(em0a&&p[0x2f7]))
            return stop(s,0x21f8f8,GH_ACTOR_MOTION_UNSUPPORTED);
        p[0x186a]=2;
        put(p+0x16d4,word(p+0x16d4)|0x400u);
        for(unsigned i=1;i<5;i++){
            const gh_hitarea_object *give=&enemy->hitareas.units[i];
            const gh_title_player_view *hit;
            if(!enemy->hitareas.created[i]||
               (!give->fallback&&gh_hitarea_object_last_hit_actor(give,&hit)))
                return stop(s,0x262a10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!give->fallback&&hit&&hit->bytes&&hit->size>=0x300&&half(hit->bytes+0x2fe)==0x100){
                p[0x1864]=1;
                break;
            }
        }
        s->substate67_stage=p[0x2f6]?TRACK_ATTACK:SELECT_ATTACK;
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
        stamp19_attack_parameters(p,0,0x14,0x3e);
        s->reaction_resource_offset=0x8fc;
        s->substate67_stage=OPEN_ATTACK;
    }
    if(s->substate67_stage==SELECT_ATTACK){
        const unsigned heat=heat_category(s);
        const float distance=number(word(p+0x618));
        if(!heat||!isfinite(distance))return stop(s,0x21b1c8,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t random;
        if(gh_game_mt_next_0030f348(&init->random,&random))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned choice=random%3;
        unsigned row,power=20,shape=0,kind=0x37,tracking=15,timed=0;
        uint32_t target=UINT32_MAX;
        uint16_t cue=0;
        uint32_t speed=0x3f800000u;
        if(em0a){
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
        }else if(em00_family){
            /* 21A654 default starts with an EM00 resource row, then applies
             * the independent alternate and distance decisions at 21A860. */
            if(heat==1||(!r105&&heat==5))row=(random&1u)?0x1ac:0x150;
            else if(choice==0){row=0x12c;power=15;}
            else row=choice==1?0x150:0x1ac;
            uint32_t alternate;
            if(gh_game_mt_next_0030f348(&init->random,&alternate))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(alternate&1u){
                tracking=30;
                if(p[0x186b]>=5){row=0x21c;power=20;}
            }
            if(distance>number(0x4038f5c3u)){row=0x144;shape=1;power=20;}
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
        if(em0a){
            put(p+0x187c,target);put_half(p+0x1880,cue);
            put(p+0x1890,target);put_half(p+0x1894,cue);
        }
        s->reaction_resource_offset=(uint16_t)row;
        s->substate67_stage=OPEN_ATTACK;
    }
    if(s->substate67_stage==OPEN_ATTACK){
        const gh_object_view *resource=&enemy->entry.resource;
        const size_t row=s->reaction_resource_offset;
        if(!s->substate67_motion.active){
            const unsigned heat=heat_category(s);
            if(reverse&&!heat)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            const int long_sequence=(em27&&row==0x6b8)||
                (heat==5&&(em00_family||em0a||row==0x6a4||row==0x6dc||row==0x6f4));
            const size_t seq=row+(long_sequence?8:4);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent>resource->size||
               resource->file_extent<seq+4)
                return stop(s,0x21f5a4,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                &s->substate67_motion,motion,&motion->b,word(resource->bytes+row),
                word(resource->bytes+seq),0,reverse?3:10,p[0x17c3]?2:0,em00_family&&!r105);
            if(status)return stop(s,s->substate67_motion.frontier?s->substate67_motion.frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(&s->substate67_motion);
        if(status)return stop(s,s->substate67_motion.frontier?s->substate67_motion.frontier:0x2a8578,status);
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
        if(word(p+0x604)||!isfinite(rate))return stop(s,0x21f7d8,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint32_t tracking=word(p+0x5f0);
        if(tracking){
            float origin[3],player[3],delta;
            if(!position(&enemy->entry.object.view,origin)||!position(s->player,player))
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
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,*enemy->b.canonical_rate,
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
        if(gh_player_root_rotation_0014f9f0(&enemy->entry.object.view,enemy->b.entry.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=ATTACK_TRANSLATION;
    }
    if(s->substate67_stage==ATTACK_TRANSLATION){
        if(gh_player_root_translation_0014f928(&enemy->entry.object.view,enemy->b.entry.work,
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
            if(word(s->player->bytes+0x15f4)&0x1000u)enemy_decision(p,0,0x6c,0);
            else{
                gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
                if(status)return status;
            }
        }else{
            const unsigned hit_flags=half(p+0x3ac);
            const uint32_t timed=word(p+0x5f4);
            if((timed&&((!em00_family&&!em0a)||timed!=1))||(!em00_family&&word(p+0x5fc))||
               (heat_category(s)>=3&&(hit_flags&0x10u)))
                return stop(s,0x21f8f8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(timed){
                /* 21FB34 default timed row19C rewrites both Give parameter
                 * sets each nonterminal frame before the hit-area test. */
                stamp19_attack_parameters(p,hit_flags&2u?20:15,0,hit_flags&2u?0x37:0x3d);
            }
            gh_actor_motion_status status=enemy_attack_area_00260b30(s,p);
            if(status)return status;
            if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        }
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
    const int em00=dependency==0x200||r105_exchanged_enemy(work,p);
    const int em0a=dependency==0x20a&&!half(p+0x640)&&p[0x1650]==3;
    if(!r105&&(work->room!=0x100||(!em00&&!em0a&&(dependency!=0x203||half(p+0x640)!=19))))
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
            const size_t row=em0a?0xc5c:em00?0x50c:0x7cc;
            const size_t sequence=row+(rank==5?8:4);
            if(!rank)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<sequence+4||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x234e24,GH_ACTOR_MOTION_BOUNDS);
            static const float factor[]={0,.75f,.8f,.9f,.9f,1};
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x235648,GH_ACTOR_MOTION_CALLEE);
            const uint32_t tracking=(uint32_t)mul(em0a?40.0f:20.0f,factor[rank]);
            if(fesetenv(&env))return stop(s,0x235648,GH_ACTOR_MOTION_CALLEE);
            p[0x1864]=0;
            put(p+0x5f8,0);
            put(p+0x5f0,tracking);
            put(p+0x5f4,em0a?30:0);
            memset(p+0x580,0,12);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+sequence),
                0,10,p[0x17c3]?2:0,em00&&!r105);
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
            if(reached&&em0a){
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
        gh_actor_motion_status status=s->player_grab(s->player_grab_context,enemy,em0a?0u:em00?2u:3u,&pc);
        if(status)return stop(s,pc?pc:0x129718,status);
        s->substate67_stage=5;
    }
    if(s->substate67_stage==TAIL){
        const unsigned flags=half(p+0x3ac);
        if(heat_category(s)>=3)return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(flags&3u)put(p+0x5fc,1);
        if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_em0a_substate4c_00235db8(gh_enemy202_move *s,uint8_t *p)
{
    enum { BIND=230, SOLID, FOLLOW, SAMPLE, ROTATE, ROTATE_HOLD, MOVE, MOVE_HOLD, MASH, DECIDE };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->b.entry.work;
    uint32_t pc=0;
    if(work->room!=0x100||word(p+0x564)!=0x20a||half(p+0x640)||p[0x1650]!=3||
       !s->player||!s->player->bytes||s->player->size<0x55c)
        return stop(s,0x235db8,GH_ACTOR_MOTION_UNSUPPORTED);
    uint8_t *player=s->player->bytes;
    if(!s->substate67_stage){
        if(p[0x2f6]>5)return stop(s,0x235db8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=(p[0x2f6]&1u)?SOLID:BIND;
    }
    if(s->substate67_stage==BIND){
        const size_t row=0xc68u+4u*p[0x2f6];
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
static gh_actor_motion_status advance_r100_substate4d_00236190(gh_enemy202_move *s,uint8_t *p)
{
    enum { ENTER=230, BIND, INPUT, SAMPLE, ROTATE, TRANSLATE, PROMPT, FINISH, RESET, ESCAPE_SOLID, ESCAPE_PLAYER, ESCAPE_PROMPT };
    gh_enemy_actor *enemy=s->enemy;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->b.entry.work;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    const int em27=r105_exchanged_enemy(work,p);
    if((!em27&&(work->room!=0x100||word(p+0x564)!=0x200||half(p+0x640)!=20))||
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
                call,motion,&motion->b,word(resource->bytes+0x518),word(resource->bytes+0x51c),0,0,0,!em27);
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

/* 2363E8's initial catch animation for original R100 stamp19. */
static gh_actor_motion_status advance_r100_substate4e_002363e8(gh_enemy202_move *s,uint8_t *p)
{
    enum { ENTER=210, BIND, PROMPT, INPUT, SAMPLE, CHOOSE, ROTATE, TRANSLATE };
    gh_enemy_actor *enemy=s->enemy;
    gh_title_player_view *actor=&enemy->entry.object.view;
    const gh_boot_work *work=enemy->b.entry.work;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    if(work->room!=0x100||word(p+0x564)!=0x203||half(p+0x640)!=19||
       !s->player||s->player->size<0x55c||s->player->bytes[0x2f4]!=1||
       s->player->bytes[0x2f5]!=0xb)
        return stop(s,0x2363e8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage)s->substate67_stage=ENTER;
    if(s->substate67_stage==ENTER){
        if(!enemy->b.solids||gh_collision_solid_set_enabled_001351d8(enemy->b.solids,actor,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(p[0x2f6]==4||p[0x2f6]==5||p[0x2f6]>7)return stop(s,0x2363e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=p[0x2f6]==7?INPUT:(p[0x2f6]&1u)?PROMPT:BIND;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x7f0||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x236420,GH_ACTOR_MOTION_BOUNDS);
            const unsigned row=p[0x2f6]==6?0x7e0:p[0x2f6]==2?0x7d8:0x7e8;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),0,p[0x2f6]==6?3:0,0,1);
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
        s->substate67_stage=p[0x2f6]==7?INPUT:PROMPT;
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
        if(p[0x2f6]!=7){
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
        if(s->pending_ai_pc!=0x212c08)return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!s->substate67_stage){
            uint32_t frontier=0;
            const gh_actor_motion_status status=s->devil_selector(s->devil_selector_context,&frontier);
            if(status)return stop(s,frontier?frontier:0x212c08,status);
            s->substate67_stage=5;
        }
        return advance_state0_tail(s,p);
    }
    if(s->pending_ai_pc==0x23f028)return advance_substate71_0023f028(s,p);
    if(s->pending_ai_pc==0x20fbb8)return advance_substate1_0020fbb8(s,p);
    if(s->pending_ai_pc==0x2112f0)return advance_substate3_002112f0(s,p);
    if(s->pending_ai_pc==0x23aad0)return advance_substate67(s,p);
    if(s->pending_ai_pc==0x211c08)return advance_substate0_00211c08(s,p);
    if(s->pending_ai_pc==0x2130c8)return advance_substate5_002130c8(s,p);
    if(s->pending_ai_pc==0x213ea8)return advance_substate7_00213ea8(s,p);
    if(s->pending_ai_pc==0x213710)return advance_substate6_00213710(s,p);
    if(s->pending_ai_pc==0x218c48)return advance_substate13_00218c48(s,p);
    if(s->pending_ai_pc==0x214298)return advance_substate8_00214298(s,p);
    if(s->pending_ai_pc==0x2147d0)return advance_substate9_002147d0(s,p);
    if(s->pending_ai_pc==0x214de0)return advance_em00_evade_00214de0(s,p);
    if(s->pending_ai_pc==0x23a950)return advance_em0a_substate66_0023a950(s,p);
    if(s->pending_ai_pc==0x23afd8)return advance_substate69_0023afd8(s,p);
    if(s->pending_ai_pc==0x23b968||s->pending_ai_pc==0x23c620)
        return advance_substate6b_or_6c(s,p);
    if(s->pending_ai_pc==0x233738)return advance_r101_armed47_00233738(s,p);
    if(s->pending_ai_pc==0x21fd50)return advance_r105_unarmed16_0021fd50(s,p);
    if(s->pending_ai_pc==0x234d30)return advance_substate4b_00234d30(s,p);
    if(s->pending_ai_pc==0x235db8)return advance_em0a_substate4c_00235db8(s,p);
    if(s->pending_ai_pc==0x236190)return advance_r100_substate4d_00236190(s,p);
    if(s->pending_ai_pc==0x2363e8)return advance_r100_substate4e_002363e8(s,p);
    if(s->pending_ai_pc==0x21a538||s->pending_ai_pc==0x231ac0)return advance_attack_0021a538(s,p);
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

static gh_actor_motion_status triangle_zero_wait(gh_enemy202_move *s,uint8_t *p)
{
    gh_game_init_state *init=s->enemy->b.entry.init;
    if(s->state1_phase==23)return triangle_recovery_00274238(s,p);
    if(s->state1_phase==20){
        const float distance=number(word(p+0x618)),cooldown=number(word(p+0x1740));
        if(!isfinite(distance)||!isfinite(cooldown))
            return stop(s,0x24e340,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t draw;
        if(p[0x17bb]&&cooldown<=0&&half(p+0x568)){
            if(gh_game_mt_next_0030f348(&init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            if(draw%12==0&&distance<9&&!init->heat[0x10]){
                s->state1_phase=23;
                return triangle_recovery_00274238(s,p);
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
            return triangle_recovery_00274238(s,p);
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
    gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
    if(status)return status;
    s->state1_phase=chosen?17:14;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_first_reaction_0024c3c8(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_hitarea_object *body=&enemy->hitareas.units[0];
    uint32_t pc=0;
    const int triangle=s->pending_ai_pc==0x24da80;
    const uint32_t entry=triangle?0x24da80:0x24c3c8;
    const unsigned stamp=half(p+0x640),dependency=word(p+0x564);
    const int triangle_wait=triangle&&enemy->b.entry.work->room==0x100&&
        ((stamp==19&&dependency==0x203)||((stamp==8||stamp==20)&&dependency==0x200));
    if(triangle&&s->state1_phase==19)
        return stop(s,entry,GH_ACTOR_MOTION_UNSUPPORTED);
    if(s->state1_phase==6){
        if(p[0x2f6]==1)s->state1_phase=11;
        else if(p[0x2f6]!=0||p[0x2f7]||word(p+0x6ec))
            return stop(s,entry,GH_ACTOR_MOTION_UNSUPPORTED);
        else{
            const gh_title_player_view *source=NULL;
            if(gh_hitarea_object_last_hit_actor(body,&source))
                return stop(s,entry,GH_ACTOR_MOTION_UNSUPPORTED);
            if(source){
                float actor[3],target[3],angle;
                if(!position(&enemy->entry.object.view,actor)||!position(source,target)||
                    gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                        number(word(p+0x104)),number(0x40490fdb),&angle,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
                if(ordered_abs(angle)>2.3561945f){
                    p[0x2f6]=2;
                    s->state1_phase=19;
                    return stop(s,triangle?entry:0x24d0f0,GH_ACTOR_MOTION_UNSUPPORTED);
                }
            }
            const gh_object_view *resource=&enemy->entry.resource;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<(triangle?0x794u:0x784u)||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,entry,GH_ACTOR_MOTION_BOUNDS);
            uint32_t random;
            if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&random))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            /* 24C3C8 selects 2D0/2D8 for dependency 0200. */
            if(triangle) {
                const uint32_t offset=word(p+0x564)==0x200?0x2e0u:0x784u;
                s->reaction_resource_offset=offset+(random&1u)*8u;
            }else if(word(p+0x564)==0x200)s->reaction_resource_offset=(random&1u)?0x2d0:0x2d8;
            else s->reaction_resource_offset=(random&1u)?0x774:0x77c;
            s->state1_phase=7;
        }
    }
    if(s->state1_phase==7){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=enemy->entry.resource.bytes+s->reaction_resource_offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(row),word(row+4),0,2,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        s->state1_phase=8;
    }
    if(s->state1_phase==8){
        if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_motion_sound_request request={.model=motion->b.model,.actor=&enemy->entry.object.view,
            .bank=1,.program=(int16_t)half(p+0x424)};
        uint32_t handle;
        gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&request,&handle,&pc);
        if(status)return stop(s,pc?pc:0x2cba48,status);
        s->state1_phase=9;
    }
    if(s->state1_phase==9){
        if((int32_t)word(p+0x5b4)>199)return stop(s,0x26f120,GH_ACTOR_MOTION_UNSUPPORTED);
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
            gh_game_mt next=enemy->b.entry.init->random;
            if(gh_game_mt_next_0030f348(&next,&delay)||gh_game_mt_next_0030f348(&next,&short_wait))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            enemy->b.entry.init->random=next;
            const int short_delay=short_wait%3u==0&&!enemy->b.entry.init->heat[0x10]&&
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
            const float turn_rate=number(word((const uint8_t*)enemy->b.entry.work+0x55c));
            if(!position(&enemy->entry.object.view,actor)||!isfinite(actor_rate)||!isfinite(turn_rate)||
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
        if(!enemy->b.canonical_rate||p[0x2f7])
            return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state1_phase=ended?13:14;
    }
    if(s->state1_phase==13){
        if(number(word(p+0x17cc))>0)return stop(s,0x270c78,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
        if(status)return status;
        s->state1_phase=17;
    }
    if(triangle_wait&&s->state1_phase==14&&!word(p+0x5f4)){
        const float resistance=number(word(p+0x17cc));
        if(!isfinite(resistance)||resistance>0)
            return stop(s,0x270c78,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=20;
    }
    if(triangle&&s->state1_phase>=20&&s->state1_phase<=24){
        gh_actor_motion_status status=triangle_zero_wait(s,p);
        if(status)return status;
    }
    if(s->state1_phase==14){
        if(number(word(p+0x17cc))>0)return stop(s,0x24ccf0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_player_root_rotation_0014f9f0(&enemy->entry.object.view,enemy->b.entry.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=15;
    }
    if(s->state1_phase==15){
        if(gh_player_root_translation_0014f928(&enemy->entry.object.view,enemy->b.entry.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=16;
    }
    if(s->state1_phase==16){
        const uint32_t wait=word(p+0x5f4);
        if(!wait&&!triangle_wait)
            return stop(s,triangle?0x24e334:0x24cf10,GH_ACTOR_MOTION_UNSUPPORTED);
        const float timer=number(word(p+0x604)),rate=number(word(p+0x5a8));
        if(!isfinite(timer)||!isfinite(rate))return stop(s,0x24cfa4,GH_ACTOR_MOTION_UNSUPPORTED);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x24cfa4,GH_ACTOR_MOTION_CALLEE);
        const float remaining=timer>0&&!enemy->entry.b.init->heat[0x10]?sub(timer,rate):timer;
        if(fesetenv(&env)||!isfinite(remaining))return stop(s,0x24cfa4,GH_ACTOR_MOTION_UNSUPPORTED);
        if(wait)put(p+0x5f4,wait-1);
        put(p+0x604,bits(remaining));
        s->state1_phase=17;
    }
    if(s->state1_phase==17){
        if(number(word(p+0x1704))>0)return stop(s,0x26ea50,GH_ACTOR_MOTION_UNSUPPORTED);
        if(s->opacity&&(s->opacity->init!=enemy->b.entry.init||s->opacity->current_player!=s->player))
            return GH_ACTOR_MOTION_ARGUMENT;
        if(gh_game_object_opacity_002a87e8(&enemy->entry.object.view,NULL,enemy->b.entry.work,
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
    gh_enemy_actor *enemy=s->enemy;
    gh_title_player_view *actor=&enemy->entry.object.view;
    gh_boot_work *work=enemy->b.entry.work;
    gh_actor_motion *motion=&enemy->motion.motion;
    uint32_t pc=0;
    if(s->state1_phase==6){
        if(p[0x2f6]){
            if(p[0x2f6]==1)s->state1_phase=10;
            else {s->state1_phase=15;s->substate67_stage=5;}
        }else{
            const gh_object_view *resource=&enemy->entry.resource;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x608||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x258d80,GH_ACTOR_MOTION_BOUNDS);
            uint32_t random;
            if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&random))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            s->reaction_resource_offset=(random&1u)?0x5f8:0x600;
            s->state1_phase=7;
        }
    }
    if(s->state1_phase==7){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=enemy->entry.resource.bytes+s->reaction_resource_offset;
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
        const gh_hitarea_object *body=&enemy->hitareas.units[0];
        const gh_title_player_view *source=NULL;
        if(!enemy->hitareas.created[0]||word(p+0x698)||
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
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->state1_phase=ended?12:13;
    }
    if(s->state1_phase==12){
        gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
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

static gh_actor_motion_status advance_lethal_reaction_00253de8(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    if(s->state1_phase==6){
        if(positive_hp(p)||p[0x2f6]>1||p[0x2f7]>1||p[0x17c3]||
           word(p+0x6ec)||gh_enemy_timers_held_weapon_runtime(&enemy->timers))
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
            s->reaction_resource_offset=p[0x2f7]?0x5e8:0x5e0;
            s->state1_phase=7;
        }
    }
    if(s->state1_phase==7){
        const gh_object_view *resource=&enemy->entry.resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x5f0||
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
        s->state1_phase=ended?13:11;
    }
    if(s->state1_phase==11){
        if(gh_player_root_rotation_0014f9f0(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=12;
    }
    if(s->state1_phase==12){
        if(gh_player_root_translation_0014f928(actor,enemy->b.entry.work,GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->state1_phase=(half(p+0x3ac)&0x10u)?13:14;
    }
    if(s->state1_phase==13){
        /* 25FE30 returns false without a transformation when its enable bit is clear. */
        unsigned substate=2;
        if(!(word(p+0x16d4)&0x100000u)){
            if(word(p+0x1644)&0x800000u)substate=3;
            else {
                if((word(p+0x16d4)&0x1000000u)||(word(p+0x16d0)&0x200u)||
                   enemy->entry.b.init->enemies[0x53e])
                    return stop(s,0x25fe30,GH_ACTOR_MOTION_UNSUPPORTED);
                substate=1;
            }
        }
        p[0x2f4]=2;
        p[0x2f5]=(uint8_t)substate;
        p[0x2f6]=p[0x2f7]=0;
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
static gh_actor_motion_status body_give_00262750(gh_enemy202_move *s, uint8_t *p)
{
    gh_enemy_hitareas *areas=&s->enemy->hitareas;
    const gh_title_player_view *actor=&s->enemy->entry.object.view;
    const gh_hitarea_object *give=&areas->units[5];
    gh_title_player_view *owner=NULL;
    const uint8_t *record;
    gh_collision_pair_shape shape;
    float position_now[3],delta[3];
    const float offset[]={0,0,0},size[]={2,2,2};
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
    put_half(give->bytes+0x46,0x2e);
    put(give->bytes+0x4c,10);
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
    if(work->room!=0x100||word(p+0x564)!=0x200||half(p+0x640)!=20||
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
            if(word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
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
            /* 274E98 and 274150 have no nonzero branch in R100. */
            float override=number(word(p+0x1734)),timer=number(word(p+0x17cc));
            float distance=number(word(p+0x618));
            if(!isfinite(override)||!isfinite(timer)||!isfinite(distance))
                return stop(s,0x274fe8,GH_ACTOR_MOTION_UNSUPPORTED);
            int flip=override>0;
            if(!flip&&p[0x17c1]&&timer<=0&&distance<6.25f){
                uint32_t draw;
                if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&draw))
                    return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
                flip=(draw&3u)!=0;
            }
            enemy_decision(p,1,flip?0x14:0xd,0);
            if(flip)put(p+0x54c,0x40a00000);
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
        const gh_actor_motion_status status=body_give_00262750(s,p);
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
    if(work->room!=0x100||word(p+0x564)!=0x200||half(p+0x640)!=20)
        return stop(s,0x257948,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        if(p[0x2f6]>1)s->substate67_stage=ATTACK;
        else s->substate67_stage=p[0x2f6]?SAMPLE:BIND;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            if(word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
                return stop(s,0x257d34,GH_ACTOR_MOTION_UNSUPPORTED);
            const float timer=number(word(p+0x17cc));
            if(!isfinite(timer))return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
            const unsigned row=!(half(p+0x3ac)&0x200u)?0x360:timer>0?0x358:0x368;
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
static gh_actor_motion_status advance_state1_0024c0f0(gh_enemy202_move *s,uint8_t *p)
{
    if(!s->state1_phase){
        const uint32_t dependency=word(p+0x564);
        /* The same original 24C2E8 table reaches 24C3C8 for stamp19's
         * state 01/00. Its dependency 203 takes the ordinary 774/77C rows. */
        const int stamp19=dependency==0x203&&half(p+0x640)==19&&
            (uint16_t)s->enemy->b.entry.work->room==0x100;
        const int stamp20=dependency==0x200&&half(p+0x640)==20&&
            (uint16_t)s->enemy->b.entry.work->room==0x100;
        const int stamp8=dependency==0x200&&half(p+0x640)==8&&
            (uint16_t)s->enemy->b.entry.work->room==0x100;
        if(!(((dependency==0x20a||stamp19||stamp20)&&p[0x2f5]==0)||
            (stamp8&&(p[0x2f5]==1||(p[0x2f5]==7&&!positive_hp(p))))||
            ((stamp19||stamp20)&&(p[0x2f5]==1||(p[0x2f5]==7&&!positive_hp(p))))||
            (stamp20&&(p[0x2f5]==0x1d||p[0x2f5]==0xd))||
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
        p[0x1867]=0;
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
        if(p[0x2f5]==7)expected=0x253de8;
        if(p[0x2f5]==0x11)expected=0x258d80;
        if(p[0x2f5]==0x1d)expected=0x25bcc0;
        if(p[0x2f5]==0xd)expected=0x257948;
        const uint8_t *descriptor=elf_span(s->elf,s->elf_bytes,0x3bf738+8u*p[0x2f5],8);
        if(!descriptor||half(descriptor)||half(descriptor+2)!=0xffff||word(descriptor+4)!=expected)
            return stop(s,0x24c2e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->pending_ai_pc=word(descriptor+4);
        s->state1_phase=6;
    }
    if(s->pending_ai_pc==0x258d80)return advance_scenery_reaction_00258d80(s,p);
    if(s->pending_ai_pc==0x253de8)return advance_lethal_reaction_00253de8(s,p);
    if(s->pending_ai_pc==0x25bcc0)return advance_em00_escape_counter_0025bcc0(s,p);
    if(s->pending_ai_pc==0x257948)return advance_em00_getup_00257948(s,p);
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

/* 273D30 -> 2923E8's original no-item probability branch. Keep the selected
 * positive item and cursor on 30FA98 rather than drawing again on retry. */
static gh_actor_motion_status death_drop_002923e8(gh_enemy202_move *s,uint8_t *p)
{
    gh_game_init_state *init=s->enemy->b.entry.init;
    if(s->death_fade_phase==4){
        if(word(p+0x16d4)&2u){s->death_fade_phase=8;return GH_ACTOR_MOTION_OK;}
        put(p+0x16d4,word(p+0x16d4)|2u);
        s->death_fade_phase=5;
    }
    if(s->death_fade_phase==5){
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
            s->death_fade_phase=6;
        }else{
            put(init->enemies+0x534,0);
            s->death_fade_phase=7;
        }
    }
    if(s->death_fade_phase==6){
        uint32_t draw;
        if(gh_game_mt_next_0030f348(&init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(draw&3u)s->death_fade_phase=8;
        else{s->death_item=0x3c4;s->death_fade_phase=7;}
    }
    if(s->death_fade_phase==7)
        return stop(s,s->death_item?0x30fa98:0x31d000,GH_ACTOR_MOTION_UNSUPPORTED);
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status death_fade_0025f1f0(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_game_init_state *init=enemy->b.entry.init;
    uint32_t pc=0;
    if(!s->death_fade_phase){
        put(p+0x54c,0x40400000);
        put(p+0x16d0,word(p+0x16d0)|0x10000u);
        if(gh_collision_solid_set_enabled_001351d8(enemy->b.solids,&enemy->entry.object.view,0,&pc))
            return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)|0x20000u);
        put(p+0x250,word(p+0x250)|0x10000u);
        p[0x617]=1;
        if(p[0x2f7]||(word(p+0x16d4)&0x101000u)||number(word(p+0x770))!=0)
            return stop(s,0x25f1f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->death_fade_phase=p[0x2f6]?9:1;
    }
    if(s->death_fade_phase==1){
        gh_actor_motion_status status=death_effect(s,1,2);
        if(status)return status;
        s->death_fade_phase=2;
    }
    if(s->death_fade_phase==2){
        gh_actor_motion_status status=death_effect(s,9,2);
        if(status)return status;
        s->death_fade_phase=3;
    }
    if(s->death_fade_phase==3){
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
        s->death_fade_phase=4;
    }
    if(s->death_fade_phase>=4&&s->death_fade_phase<=8){
        gh_actor_motion_status status=death_drop_002923e8(s,p);
        if(status)return status;
        if(half(p+0x640)==0xff)return stop(s,0x1fbe40,GH_ACTOR_MOTION_UNSUPPORTED);
        status=count_death_and_mark_room(s,p);
        if(status)return status;
        if(word(p+0x6f4)||word(p+0x704))return stop(s,0x1cf4a8,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x608,0);++p[0x2f6];
        s->death_fade_phase=9;
    }
    if(s->death_fade_phase==9){
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
                s->death_fade_phase=10;
            }
        }else if(phase!=4)return stop(s,0x25f1f0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(s->death_fade_phase==9)s->death_fade_phase=11;
    }
    if(s->death_fade_phase==10){
        if(!s->death_release)return stop(s,0x290770,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=s->death_release(s->death_release_context,enemy,&pc);
        if(status)return stop(s,pc?pc:0x290770,status);
        ++p[0x2f6];
        s->death_fade_phase=11;
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
    if(s->death_phase==1){
        if(p[0x2f6]>1||positive_hp(p)||p[0x17c3])
            return stop(s,0x25ed00,GH_ACTOR_MOTION_UNSUPPORTED);
        if(p[0x2f6])s->death_phase=5;
        else{
            const gh_object_view *resource=&enemy->entry.resource;
            if(!resource->file_extent_known||resource->file_extent<0x390||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x25ed00,GH_ACTOR_MOTION_BOUNDS);
            uint32_t draw;
            if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            const uint32_t offset=half(p+0x3ac)&0x200u?0x380u:0x370u;
            s->reaction_resource_offset=offset+((draw&1u)?0u:8u);
            s->death_phase=2;
        }
    }
    if(s->death_phase==2){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=enemy->entry.resource.bytes+s->reaction_resource_offset;
            const gh_actor_motion_status result=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(row),word(row+4),0,5,0,1);
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
        if((uint16_t)work->room!=0x100||
           !((dependency==0x203&&stamp==19)||(dependency==0x200&&(stamp==20||(stamp==8&&(p[0x2f5]==3||p[0x2f5]==2))))))
            return stop(s,0x25e600,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x6e4)||word(p+0x6e8))return stop(s,0x26a938,GH_ACTOR_MOTION_UNSUPPORTED);
        if(word(p+0x6ec)||gh_enemy_timers_held_weapon_runtime(&enemy->timers))
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
static gh_actor_motion_status submit_hit_sound(gh_enemy202_move *s,const gh_motion_sound_request *request)
{
    const gh_actor_motion *motion=&s->enemy->motion.motion;
    if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
    uint32_t handle,pc=0;
    gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,request,&handle,&pc);
    return status?stop(s,pc?pc:0x2cba48,status):GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status play_hit_sound(gh_enemy202_move *s, gh_actor_model *model)
{
    gh_enemy_actor *enemy=s->enemy;
    const gh_hitarea_object *body=&enemy->hitareas.units[0];
    gh_title_player_view *view=&enemy->entry.object.view;
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
            if(!enemy)return stop(s,0x20a178,GH_ACTOR_MOTION_UNSUPPORTED);
            uint32_t pc;
            gh_actor_motion_status status=gh_enemy202_hit_0020a178(&s->damage,enemy,player,heat_category(s),&pc);
            if(status)return stop(s,pc?pc:0x20a178,status);
            if(s->damage.reacted){
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
                    status=gh_enemy202_hit_rank_points_001fa280(enemy,thresholds,(s->damage.effect==0x2e||s->damage.effect==0x12)?16u:4u,&pc);
                    if(status)return stop(s,pc?pc:0x1fa280,status);
                    s->damage.phase=8;
                }
                if(s->damage.phase==8){
                    if(s->damage.effect!=0x2e){
                        status=gh_enemy202_hit_stun_0028ed28(enemy,&pc);
                        if(status)return stop(s,pc?pc:0x28ed28,status);
                    }
                    s->damage.phase=9;
                }
                if(s->damage.phase<=10){
                    status=gh_enemy202_impact_00271810(&s->damage,enemy,player,&pc);
                    if(status)return stop(s,pc?pc:0x271810,status);
                    if(s->damage.impact.index){
                        if(!s->impact_bindings.storage.effects)return stop(s,0x306838,GH_ACTOR_MOTION_UNSUPPORTED);
                        if(s->impact_bindings.work!=rt->entry->b.work||
                            s->impact_bindings.random!=&rt->entry->b.init->random)
                            return stop(s,0x306838,GH_ACTOR_MOTION_ARGUMENT);
                        if(!s->impact_create.active){
                            uint32_t position[3];
                            memcpy(position,s->damage.impact.position,sizeof(position));
                            gh_effect_status begin;
                            if(s->damage.impact.bank==0)
                                begin=gh_effect_create_enemy_counter_impact_begin_00306838(&s->impact_create,position);
                            else if(s->damage.impact.index==0x12)
                                begin=gh_effect_create_enemy_amplified_impact_begin_00306838(&s->impact_create,position);
                            else
                                begin=gh_effect_create_enemy_impact_begin_00306838(&s->impact_create,position);
                            if(begin)return stop(s,0x306838,GH_ACTOR_MOTION_UNSUPPORTED);
                        }
                        gh_effect_status effect=gh_game_effect_create_00306528(&s->impact_create,&s->impact_bindings);
                        if(effect||!s->impact_create.complete)
                            return stop(s,s->impact_create.frontier?s->impact_create.frontier:0x306528,
                                effect==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE);
                    }
                    s->damage.phase=(half(p+0x3ac)&0x100u)?17:11;
                }
                if(s->damage.phase==17){
                    /* 271D5C is a separate original cue before the ledger hit sound. */
                    const gh_motion_sound_request cue={.model=model,.actor=view,.bank=0,.program=1};
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
        if(s->dispatch_pc==0x24c0f0&&enemy)return advance_state1_0024c0f0(s,p);
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
