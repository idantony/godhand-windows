#include "id_layout_rotation.h"
#include "game_player_root_delta.h"
#include "game_enemy_route_graph.h"
#include "game_enemy_move.h"
#include "game_enemy_request.h"
#include "game_model_texture_scroll.h"
#include "game_player_targets.h"
#include "game_ray_query.h"
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

static int body_allows_no_hit(gh_enemy_actor *enemy)
{
    const gh_hitarea_object *body=&enemy->hitareas.units[0];
    const uint8_t *p=enemy->entry.object.view.bytes;
    if(enemy->hitareas.created[0]){
        gh_title_player_view *owner;
        if(gh_hitarea_object_actor(body,&owner)||(!body->fallback&&owner!=&enemy->entry.object.view)||
            body->size<0x64)return -1;
    }
    float drain=number(word(p+0x770)),cooldown=number(word(p+0x54c));
    if(!isfinite(drain)||!isfinite(cooldown)||(drain>0&&positive_hp(p)))return 0;
    if(!enemy->hitareas.created[0]||!(word(body->bytes+0x60)&1u)||cooldown>0)return 1;
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

static gh_actor_motion_status rank(gh_enemy202_move *s,uint8_t *p,
    const float player[3],const float self[3])
{
    const gh_game_init_state *manager=s->enemy->b.entry.init;
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
        if(!candidate||candidate==&s->enemy->entry.object)continue;
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
    const gh_core_save_view save=s->enemy->b.entry.save;
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
        uint32_t frame=word((const uint8_t*)s->enemy->b.entry.work+0x210);
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
    if(!s->enemy->b.actors)return stop(s,0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    int hit;
    uint32_t pc;
    gh_scene_ray_status status=gh_scene_ray_query_00131e10(s->enemy->b.actors,&ray,&hit,&pc);
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
    gh_scene_ray_status status=gh_scene_ray_query_00131e10(s->enemy->b.actors,&ray,&hit,&pc);
    if(status)return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    if(hit){
        /* Actual hit branch normalizes from hit toward ray origin via 101E28.
         * Keep its result stable across retries before that continuation. */
        memcpy(s->path_target,intersection,12);
        s->pending_ai_pc=0x101e28;
        return GH_ACTOR_MOTION_UNSUPPORTED;
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
            if(gh_scene_ray_query_00131e10(s->enemy->b.actors,&ray,&hit,&pc))
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
    if(gh_scene_ray_query_00131e10(s->enemy->b.actors,&ray,hit,&pc))
        return stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status route_clear_002a74e0(gh_enemy202_move *s,uint8_t *p,
    const float actor[3])
{
    const gh_game_init_state *init=s->enemy->b.entry.init;
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
        s->route_stage=1;
    }
    if(s->route_stage==1){
        int hit;
        uint32_t pc;
        gh_scene_ray_status status=gh_scene_ray_query_00131e10(s->enemy->b.actors,&ray,&hit,&pc);
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
        if(s->origin_vertex==UINT32_MAX)return stop(s,0x2a79d0,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint8_t *origin=s->navigation->vertices.bytes+(size_t)s->origin_vertex*32;
        memcpy(p+0x520,origin,12);
        put(p+0x510,bits(length));
        p[0x530]=0;
        s->route_stage=5;
    }
    if(s->route_stage==5){
        unsigned max_depth=16;
        fenv_t depth_env;
        if(!set_round(&depth_env,FE_TOWARDZERO))return stop(s,0x2a7650,GH_ACTOR_MOTION_CALLEE);
        if(length>=30){
            float scaled=mul(sub(length,30),0.25f);
            max_depth=scaled>=48?64:16+(unsigned)scaled;
        }
        if(fabsf(sub(s->path_target[1],actor[1]))>3)max_depth=max_depth*24/16;
        if(s->enemy->b.entry.work->room==0x300)max_depth=max_depth*20/16;
        if(s->enemy->b.entry.work->room==0x700)max_depth/=2;
        if(fesetenv(&depth_env))return stop(s,0x2a7650,GH_ACTOR_MOTION_CALLEE);
        uint32_t nodes[16];
        memcpy(nodes,p+0x4d0,sizeof(nodes));
        float cost;
        gh_actor_motion_status status=gh_enemy_route_graph_0031be88(s->navigation,
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

static gh_actor_motion_status camera_update_gate(gh_enemy202_move *s,const uint8_t *p,int *update)
{
    const gh_object_opacity_environment *env=s->opacity;
    if(!env||!env->camera||!env->camera->constructed||
        env->init!=s->enemy->b.entry.init||env->init->active_camera!=env->camera->identity)
        return stop(s,0x26ef60,GH_ACTOR_MOTION_UNSUPPORTED);
    float target[3],eye[3],self[3],previous[3];
    memcpy(target,env->camera->fields+0x200,12);
    memcpy(eye,env->camera->fields+0x210,12);
    memcpy(previous,p+0x490,12);
    if(!position(&s->enemy->entry.object.view,self))
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
    if(gh_actor_model_part_at(&s->enemy->entry.initialize.model.model,0,&root_part)||
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
    gh_actor_model *model=&s->enemy->entry.initialize.model.model;
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
    if(gh_actor_model_find_mesh_0014b730(&s->enemy->entry.initialize.model.model,
        name,8,&node,&exists))return stop(s,0x14b730,GH_ACTOR_MOTION_UNSUPPORTED);
    return exists?hide_cached_node(s,&node,0,0x268dc8):GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status turn_head_00269b50(gh_enemy202_move *s,uint8_t *p)
{
    gh_actor_model *model=&s->enemy->entry.initialize.model.model;
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
        if(!position(&s->enemy->entry.object.view,actor)||!position(s->player,player)||
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

static gh_actor_motion_status advance_terrain_tail(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    const gh_title_player_view *actor=&enemy->entry.object.view;
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
        if(gh_collision_solid_step_resume_001348e8(enemy->b.solids,actor,
            GH_STATIC_BINARY32_REFERENCE,&s->post_solid,&hit,&pc))
            return stop(s,pc?pc:0x1348e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=26;
    }
    gh_player_terrain_bindings binding={enemy->b.actors,actor,gh_terrain_sphere_query_001321d8,&s->sphere_call};
    if(s->substate67_stage==26){
        if(!(word(p+0x16d0)&0x20000u)){
            s->sphere_call.work=enemy->b.entry.work;
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
            if(gh_scene_ray_query_00131e10(enemy->b.actors,&ray,&hit,&pc))
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
    if(half(p+0x3ac)&0x800u)return stop(s,0x292f70,GH_ACTOR_MOTION_UNSUPPORTED);
    if(flags&2u)enemy->b.entry.init->enemies[0x53a]=2;
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
    gh_enemy_actor *enemy=s->enemy;
    if(s->substate67_stage==17){
        /* 3130E0 zeroes all 0x1980 bytes, and 271410's unsigned
         * dependency-214 gate cannot allocate +1860 for the verified kinds. */
        uint32_t dependency=word(p+0x564);
        if(enemy->entry.object.view.initial_memory!=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO||
            dependency>=0x214||word(p+0x1860))return stop(s,0x2714e8,GH_ACTOR_MOTION_UNSUPPORTED);
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
        int ambient=enemy->b.entry.init->enemies[0x518]!=0;
        if(ambient!=!!(flags&0x100u))
            return stop(s,ambient?0x306528:0x306200,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=20;
    }
    if(s->substate67_stage==20){
        if(enemy->nodes.stage<2)return stop(s,0x268dc8,GH_ACTOR_MOTION_UNSUPPORTED);
        for(;s->post_node_index<6;s->post_node_index++){
            gh_actor_motion_status status=hide_cached_node(s,&enemy->nodes.nodes[s->post_node_index],1,0x268dc8);
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
                hide_cached_node(s,&enemy->nodes.nodes[bank*3+index],0,0x268dc8);
            if(status)return status;
        }
        ++s->substate67_stage;
    }
    if(s->substate67_stage==23){
        if(enemy->nodes.nodes[9].bytes){
            int hidden=!(word(p+0x16d0)&0x400000u);
            for(;s->post_node_index<8;s->post_node_index++){
                gh_actor_motion_status status=hide_cached_node(s,&enemy->nodes.nodes[s->post_node_index+3],
                    hidden,0x269ac0);
                if(status)return status;
            }
        }
        s->substate67_stage=24;
    }
    return advance_terrain_tail(s,p);
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
        if(!special&&half(p+0x3ae))return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
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
            if(dependency!=0x200&&dependency!=0x202&&dependency!=0x203&&dependency!=0x20a)
                return stop(s,0x26a3d8,GH_ACTOR_MOTION_UNSUPPORTED);
            gh_actor_model *model=&s->enemy->entry.initialize.model.model;
            const gh_actor_part *parts[2]={NULL,NULL};
            for(unsigned i=0;i<2;i++)
                if(p[0x2b4]>i+1&&gh_actor_model_part_at(model,i+1,&parts[i]))
                    return stop(s,0x26a3d8,GH_ACTOR_MOTION_UNSUPPORTED);
            float roll=number(word(p+0x16fc));
            if(!isfinite(roll))return stop(s,0x26a508,GH_ACTOR_MOTION_UNSUPPORTED);
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x26a508,GH_ACTOR_MOTION_CALLEE);
            float next=mul(roll,0.6f);
            if(fesetenv(&env)||!isfinite(next))
                return stop(s,0x26a508,GH_ACTOR_MOTION_UNSUPPORTED);
            for(unsigned i=0;i<2;i++)if(parts[i]){
                uint8_t *part=parts[i]->bytes;
                put(part+0x150,roll==0?0:bits(roll));
                uint32_t flags=word(part+0x154);
                put(part+0x154,roll==0?flags&~0x800u:flags|0x800u);
            }
            put(p+0x16fc,ordered_abs(next)<=0.017453292f?0:bits(next));
            /* Actual 26A558 and 273F98 gate out on these dependency IDs. */
            s->substate67_stage=11;
        }
    }
    if(s->substate67_stage==11){
        if(half(p+0x3ac)&8u)return stop(s,0x26eb20,GH_ACTOR_MOTION_UNSUPPORTED);
        float roll=number(word(p+0x1708));
        if(!isfinite(roll))return stop(s,0x26eb20,GH_ACTOR_MOTION_UNSUPPORTED);
        /* In the actual flag-clear branch the desired angle is zero. */
        put(p+0x1708,0);
        s->substate67_stage=12;
    }
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_model *model=&enemy->entry.initialize.model.model;
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
        gh_motion_solve_bindings b={.model=model,.work=enemy->b.entry.work,.actors=enemy->b.actors,
            .ray=enemy->b.ray,.context=enemy->b.ray_context,.ik_globals=enemy->b.ik,.atan_coeff=enemy->b.atan_coeff};
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
    if(dependency!=0x200&&dependency!=0x202&&dependency!=0x203&&dependency!=0x20a)
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
    gh_enemy_actor *enemy=s->enemy;
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
                if(gh_collision_solid_set_enabled_001351d8(enemy->b.solids,&enemy->entry.object.view,0,&pc))
                    return stop(s,pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
                put(p+0x1704,bits(remaining));put(p+0x24c,bits(alpha>=1?1:alpha));
                uint32_t flags=word(p+0x250);
                put(p+0x250,alpha>=1?flags&~16u:flags|16u);
                s->substate67_stage=38;
            }else s->substate67_stage=37;
        }
    }
    if(s->substate67_stage==37){
        if(s->opacity&&(s->opacity->init!=enemy->b.entry.init||
            s->opacity->current_player!=s->player))return GH_ACTOR_MOTION_ARGUMENT;
        if(gh_game_object_opacity_002a87e8(&enemy->entry.object.view,NULL,enemy->b.entry.work,
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
        gh_hitarea_object *body=&enemy->hitareas.units[0];
        gh_title_player_view *owner=NULL;
        if(!enemy->hitareas.created[0]||gh_hitarea_object_actor(body,&owner)||
           (!body->fallback&&owner!=&enemy->entry.object.view))
            return stop(s,0x1fd610,GH_ACTOR_MOTION_UNSUPPORTED);
        /* The constructor owns body+698 even though its native raw slot is zero. */
        if(!body->fallback){
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

static gh_actor_motion_status attack_choice_00262aa8(gh_enemy202_move *s,uint8_t *p,int *chosen)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_game_init_state *init=enemy->b.entry.init;
    const uint8_t *work=(const uint8_t*)enemy->b.entry.work,*manager=init->enemies;
    const float distance=number(word(p+0x618)),face=number(word(p+0x760));
    const uint32_t route=word(p+0x16d4);
    *chosen=0;
    if(word(p+0x564)!=0x20a||enemy->b.entry.work->room!=0x100)
        return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(s->substate67_stage==52||s->substate67_stage==61)return stop(s,0x273c38,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!isfinite(distance)||!isfinite(face))return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(number(word(p+0x16c0))>0)enemy_decision(p,0,0x13,0);
    else if(number(word(p+0x1734))>0)enemy_decision(p,0,0x89,0);
    else if((int32_t)word(manager+0x530)>0)return stop(s,0x26e258,GH_ACTOR_MOTION_UNSUPPORTED);
    else if(distance>=16&&!(word(p+0x1644)&0x40000000u)&&(int32_t)word(p+0x16ec)<1)
        return stop(s,0x26e588,GH_ACTOR_MOTION_UNSUPPORTED);
    else if(number(word(p+0x770))>0)enemy_decision(p,0,0x36,0);
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
                if(fabsf(turn)>2.3561945f&&distance<6.25f)enemy_decision(p,0,0x44,0);
                else if(face>=1.3089969f&&(route&0x08000000u)){
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
                    if(distance<=4&&!(route&0x10380u))enemy_decision(p,0,0x45,0);
                    else{s->substate67_stage=53;return stop(s,0x26341c,GH_ACTOR_MOTION_UNSUPPORTED);}
                }else{
                    static const float angular_limits[]={0,0.5235988f,1.0471976f,1.2217305f,1.5707964f,2.3561945f};
                    if(fabsf(turn)>angular_limits[heat])return GH_ACTOR_MOTION_OK;
                    /* The following 262AA8 player-facing switch is not yet recovered. */
                    return stop(s,0x2634d0,GH_ACTOR_MOTION_UNSUPPORTED);
                }
            }
        }
    }
    *chosen=1;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_substate0_00211c08(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    const uint32_t dependency=word(p+0x564);
    const int player_alive=(int16_t)half(s->player->bytes+0x54a)>0;
    const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&enemy->timers);
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
        case 0x200:offset=0x98;break;
        case 0x202:case 0x203:offset=0x648;break;
        case 0x20a:offset=0x828;break;
        default:return stop(s,0x211c08,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(turning){
            if(dependency!=0x20a)return stop(s,0x2120e8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!s->substate67_motion.active){
                float actor[3],target[3],turn;
                if(!position(&enemy->entry.object.view,actor)||!position(s->player,target)||
                    gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                        number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            /* This dependency selects the same clip for either turn sign. */
            offset=0x830;
        }
        if(word(p+0x6ec))return stop(s,turning?0x21251c:0x211ee8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(held)offset=turning?0x454:0x44c;
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            if(!resource->bytes||!resource->file_extent_known||
                resource->file_extent<offset+8||resource->file_extent>resource->size)
                return stop(s,0x211ee8,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+offset),word(resource->bytes+offset+4),
                0,10,p[0x17c3]?2:0,1);
            if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(call);
        if(status)return stop(s,call->frontier?call->frontier:0x2a8578,status);
        ++p[0x2f6];
        s->substate67_stage=turning?48:41;
    }
    if(s->substate67_stage>=48&&s->substate67_stage<=50){
        float actor[3],target[3],turn;
        if(!position(&enemy->entry.object.view,actor)||!position(s->player,target))
            return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(s->substate67_stage==48){
            fenv_t env;
            const float yaw=number(word(p+0x104)),actor_rate=number(word(p+0x5a8));
            const float turn_rate=number(word((const uint8_t*)enemy->b.entry.work+0x55c));
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
            if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
            int ended;
            gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
                *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
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
        if(!enemy->b.canonical_rate)return stop(s,0x157c18,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(motion,
            *enemy->b.canonical_rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
        if(status)return stop(s,pc?pc:0x157c18,status);
        s->substate67_stage=ended&&player_alive&&dependency==0x20a?42:43;
    }
    if(s->substate67_stage==42){
        uint32_t draw;
        if(word(p+0x6ec))return stop(s,0x211fb0,GH_ACTOR_MOTION_UNSUPPORTED);
        if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&draw))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        if((draw&1u)&&!held){
            p[0x2f4]=0;p[0x2f5]=0x67;p[0x2f6]=0;p[0x2f7]=0;
        }
        s->substate67_stage=43;
    }
    if(s->substate67_stage==43){
        if(gh_player_root_rotation_0014f9f0(&enemy->entry.object.view,enemy->b.entry.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f9f0,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=44;
    }
    if(s->substate67_stage==44){
        if(gh_player_root_translation_0014f928(&enemy->entry.object.view,enemy->b.entry.work,
            GH_STATIC_BINARY32_REFERENCE,&pc))
            return stop(s,pc?pc:0x14f928,GH_ACTOR_MOTION_UNSUPPORTED);
        s->substate67_stage=45;
    }
    if(s->substate67_stage==45){
        float actor[3],target[3],turn;
        if(!position(&enemy->entry.object.view,actor)||!position(s->player,target)||
            gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
            return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
        if(fabsf(turn)>0.34906584f)p[0x2f6]=2;
        else{
            const uint8_t *manager=enemy->b.entry.init->enemies;
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
    if(s->substate67_stage==46||s->substate67_stage==52){
        const uint8_t *manager=enemy->b.entry.init->enemies;
        if(player_alive&&!manager[0x5b5]&&!manager[0x53c]){
            int chosen;
            gh_actor_motion_status status=attack_choice_00262aa8(s,p,&chosen);
            if(status)return status;
            if(!chosen)return stop(s,0x212670,GH_ACTOR_MOTION_UNSUPPORTED);
        }
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
    if(!s->substate67_stage){
        if(!(((appearance==0||appearance==5)&&(dependency==0x200||dependency==0x203))||
            (appearance==9&&dependency==0x202)))
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
                if(appearance==5)offset=dependency==0x203?0x7bc:0x42c;
                else offset=dependency==0x200?0x414:0x7b4;
                sequence=offset+4;
            }else if(held){
                offset=appearance==9?0x490:0x44c;
                sequence=offset+4;
                p[0x2f7]=0;
            }else if(appearance==5){
                offset=0x424;
                sequence=0x428;
            }else{
                offset=dependency==0x200?0x98:0x648;
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

static gh_actor_motion_status advance_target(gh_enemy202_move *s,uint8_t *p,
    const float actor[3],const float player[3])
{
    if(s->pending_ai_pc==0x26f328){
        if(!s->visibility_done){
            gh_actor_motion_status status=target_line_of_sight(s,p,actor,player);
            if(status)return status;
        }
        gh_actor_motion_status status=choose_path_target(s,p,actor,player);
        if(status)return s->pending_ai_pc==0x101e28?stop(s,0x101e28,status):status;
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
    if(s->pending_ai_pc==0x23f028)return advance_substate71_0023f028(s,p);
    if(s->pending_ai_pc==0x20fbb8)return advance_substate1_0020fbb8(s,p);
    if(s->pending_ai_pc==0x23aad0)return advance_substate67(s,p);
    if(s->pending_ai_pc==0x211c08)return advance_substate0_00211c08(s,p);
    if(s->pending_ai_pc==0x214298)return advance_substate8_00214298(s,p);
    return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
}


static gh_actor_motion_status advance_first_reaction_0024c3c8(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_hitarea_object *body=&enemy->hitareas.units[0];
    uint32_t pc=0;
    if(s->state1_phase==6){
        if(p[0x2f6]==1)s->state1_phase=11;
        else if(p[0x2f6]!=0||p[0x2f7]||word(p+0x6ec))
            return stop(s,0x24c3c8,GH_ACTOR_MOTION_UNSUPPORTED);
        else{
            const gh_title_player_view *source=NULL;
            if(gh_hitarea_object_last_hit_actor(body,&source))
                return stop(s,0x24c3c8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(source){
                float actor[3],target[3],angle;
                if(!position(&enemy->entry.object.view,actor)||!position(source,target)||
                    gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                        number(word(p+0x104)),number(0x40490fdb),&angle,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
                if(ordered_abs(angle)>2.3561945f){
                    p[0x2f6]=2;
                    s->state1_phase=19;
                    return stop(s,0x24d0f0,GH_ACTOR_MOTION_UNSUPPORTED);
                }
            }
            const gh_object_view *resource=&enemy->entry.resource;
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x784||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x24c3c8,GH_ACTOR_MOTION_BOUNDS);
            uint32_t random;
            if(gh_game_mt_next_0030f348(&enemy->b.entry.init->random,&random))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            s->reaction_resource_offset=(random&1u)?0x774:0x77c;
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
        put(p+0x5f4,rank<3?15:10);
        put(p+0x600,0x40a00000);put(p+0x604,0x40000000);put(p+0x608,0x41a00000);
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
            const float cap=mul(mul(actor_rate,0.3926991f),turn_rate);
            gh_player_target_status result=gh_player_target_angle_002da038(
                s->elf,s->elf_bytes,actor,target,yaw,cap,&turn,&pc);
            float next=result?0:add(yaw,turn);
            if(fesetenv(&env)||result)return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            if(gh_id_angle_reference_001f7d48(next,&next))
                return stop(s,0x1f7d48,GH_ACTOR_MOTION_UNSUPPORTED);
            put(p+0x5f0,turn_frames-1);
            put(p+0x104,bits(next));
        }
        put_half(p+0x434,half(p+0x434)|8u);
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
        if(!wait)return stop(s,0x24cf10,GH_ACTOR_MOTION_UNSUPPORTED);
        const float timer=number(word(p+0x604)),rate=number(word(p+0x5a8));
        if(!isfinite(timer)||!isfinite(rate))return stop(s,0x24cfa4,GH_ACTOR_MOTION_UNSUPPORTED);
        fenv_t env;
        if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x24cfa4,GH_ACTOR_MOTION_CALLEE);
        const float remaining=timer>0&&!enemy->entry.b.init->heat[0x10]?sub(timer,rate):timer;
        if(fesetenv(&env)||!isfinite(remaining))return stop(s,0x24cfa4,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x5f4,wait-1);
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

static gh_actor_motion_status advance_state1_0024c0f0(gh_enemy202_move *s,uint8_t *p)
{
    if(word(p+0x564)!=0x20a||p[0x2f5]!=0)
        return stop(s,0x24c0f0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->state1_phase){
        p[0x1864]=0;
        if(p[0x2f6]==0){
            put(p+0x16d0,(word(p+0x16d0)&~0x8000u)|2u);
            s->state1_phase=1;
        }else s->state1_phase=5;
    }
    if(s->state1_phase==1){
        /* 26A938 drops the selected held object only when its slot exists.
         * This first 20A domain enters with both original slots clear. */
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
        const uint8_t *descriptor=elf_span(s->elf,s->elf_bytes,0x3bf738,8);
        if(!descriptor||half(descriptor)||half(descriptor+2)!=0xffff||word(descriptor+4)!=0x24c3c8)
            return stop(s,0x24c2e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->pending_ai_pc=word(descriptor+4);
        s->state1_phase=6;
    }
    return advance_first_reaction_0024c3c8(s,p);
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

gh_actor_motion_status gh_enemy202_move_bind_room_state(gh_enemy202_move *s,
    const gh_room_state *room)
{
    if(!s||!room)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->room_state&&s->room_state!=room)return GH_ACTOR_MOTION_BUSY;
    s->room_state=room;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy202_move_00208be0(gh_enemy202_move *s,
    gh_enemy_actor *enemy,const gh_title_player_view *player,const void *elf,size_t elf_bytes)
{
    if(!s||!enemy||enemy->stage!=13||!enemy->entry.published||!enemy->b.entry.init||
        !enemy->b.entry.work||!enemy->entry.object.view.bytes)return GH_ACTOR_MOTION_ARGUMENT;
    gh_title_player_view *view=&enemy->entry.object.view;
    uint8_t *p=view->bytes;
    if(view->size<0x1774||view->original_vtable214!=0x4479c8)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->enemy&&(s->enemy!=enemy||s->player!=player||s->elf!=elf||s->elf_bytes!=elf_bytes))
        return GH_ACTOR_MOTION_BUSY;
    s->enemy=enemy;s->player=player;s->elf=elf;s->elf_bytes=elf_bytes;
    s->frontier=0;
    if(!s->stage){
        if((word((const uint8_t*)enemy->b.entry.work+0x5d8)&0x40000000u)||
            word(enemy->b.entry.init->object_pool+0xacd0)){
            s->stage=RETURNED;s->original_returned=1;
            return GH_ACTOR_MOTION_OK;
        }
        s->stage=HEADINGS;
    }
    if(s->stage==RETURNED)return GH_ACTOR_MOTION_OK;
    unsigned dep=word(p+0x564);
    if(dep!=0x200&&dep!=0x202&&dep!=0x203&&dep!=0x20a)
        return stop(s,0x208c38,GH_ACTOR_MOTION_UNSUPPORTED);
    float actor_xyz[3],player_xyz[3];
    if(!position(view,actor_xyz)||!position(player,player_xyz)||player->size<0x63c)
        return stop(s,0x2db6b0,GH_ACTOR_MOTION_UNSUPPORTED);
    gh_actor_model *model=&enemy->entry.initialize.model.model;
    if(s->stage==HEADINGS){
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
        int no_hit=s->damage.phase?0:body_allows_no_hit(enemy);
        if(no_hit<0)return stop(s,0x20a178,GH_ACTOR_MOTION_BUSY);
        if(!no_hit){
            uint32_t pc;
            gh_actor_motion_status status=gh_enemy202_hit_0020a178(&s->damage,enemy,player,heat_category(s),&pc);
            if(status)return stop(s,pc?pc:0x20a178,status);
            if(s->damage.reacted){
                if(s->damage.phase==5){
                    p[0x17b0]=15;
                    if(word(p+0x16c4)==6)put(p+0x16c0,0);
                    s->damage.phase=7;
                }
                if(s->damage.phase==7){
                    const uint8_t *thresholds=elf_span(elf,elf_bytes,0x3bf160,20);
                    if(!thresholds)return stop(s,0x1fa280,GH_ACTOR_MOTION_UNSUPPORTED);
                    status=gh_enemy202_hit_rank_001fa280(enemy,thresholds,&pc);
                    if(status)return stop(s,pc?pc:0x1fa280,status);
                    s->damage.phase=8;
                }
                if(s->damage.phase==8){
                    status=gh_enemy202_hit_stun_0028ed28(enemy,&pc);
                    if(status)return stop(s,pc?pc:0x28ed28,status);
                    s->damage.phase=9;
                }
                if(s->damage.phase<=10){
                    status=gh_enemy202_impact_00271810(&s->damage,enemy,player,&pc);
                    if(status)return stop(s,pc?pc:0x271810,status);
                    if(!s->impact_bindings.storage.effects)return stop(s,0x306838,GH_ACTOR_MOTION_UNSUPPORTED);
                    if(s->impact_bindings.work!=enemy->entry.b.work||
                        s->impact_bindings.random!=&enemy->entry.b.init->random)
                        return stop(s,0x306838,GH_ACTOR_MOTION_ARGUMENT);
                    if(!s->impact_create.active){
                        uint32_t position[3];
                        memcpy(position,s->damage.impact.position,sizeof(position));
                        if(gh_effect_create_enemy_impact_begin_00306838(&s->impact_create,position))
                            return stop(s,0x306838,GH_ACTOR_MOTION_UNSUPPORTED);
                    }
                    gh_effect_status effect=gh_game_effect_create_00306528(&s->impact_create,&s->impact_bindings);
                    if(effect||!s->impact_create.complete)
                        return stop(s,s->impact_create.frontier?s->impact_create.frontier:0x306528,
                            effect==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE);
                    s->damage.phase=11;
                }
                if(s->damage.phase==11){
                    const gh_actor_motion *motion=&enemy->motion.motion;
                    if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
                    const gh_motion_sound_request request={.model=model,.actor=view,.bank=1,.program=0x2a};
                    uint32_t handle;
                    pc=0;
                    status=motion->b.sound(motion->b.sound_context,&request,&handle,&pc);
                    if(status)return stop(s,pc?pc:0x2cba48,status);
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
        if(!enemy->b.solids||gh_collision_solid_set_enabled_001351d8(enemy->b.solids,view,1,&pc))
            return stop(s,enemy->b.solids&&pc?pc:0x1351d8,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x250,word(p+0x250)&~0x10000u);
        s->stage=ATTACK_AREAS;
    }
    if(s->stage==ATTACK_AREAS){
        for(;s->hit_index<5;s->hit_index++){
            unsigned i=s->hit_index+1;
            gh_hitarea_object *hit=&enemy->hitareas.units[i];
            gh_title_player_view *owner;
            uint32_t pc=0;
            if(!enemy->hitareas.created[i]||gh_hitarea_object_actor(hit,&owner)||
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
        if(p[0x2f4]>=4)return stop(s,0x209654,GH_ACTOR_MOTION_UNSUPPORTED);
        const uint8_t *descriptor=elf_span(elf,elf_bytes,0x3bf1a0+8u*p[0x2f4],8);
        if(!descriptor||half(descriptor)||half(descriptor+2)!=0xffff)
            return stop(s,0x209654,GH_ACTOR_MOTION_UNSUPPORTED);
        s->dispatch_pc=word(descriptor+4);
        if(s->dispatch_pc==0x24c0f0)return advance_state1_0024c0f0(s,p);
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
