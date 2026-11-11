#include "game_scene_at_geometry.h"
#include "id_layout_rotation.h"
#include "game_player_root_delta.h"
#include "game_enemy_route_graph.h"
#include "game_player_targets.h"
#include "game_enemy_move.h"
#include "game_enemy_request.h"
#include "game_model_texture_scroll.h"
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

/* 292F70 updates one of the EnemyM directional threat timers. */
static gh_actor_motion_status attack_direction_00292f70(gh_enemy202_move *s)
{
    float player[3],actor[3],angle;
    uint32_t pc=0;
    if(!position(s->player,player)||!position(&s->enemy->entry.object.view,actor))
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
        put(s->enemy->b.entry.init->enemies+offset,2);
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
    if(half(p+0x3ac)&0x800u){
        gh_actor_motion_status status=attack_direction_00292f70(s);
        if(status)return status;
    }
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

static gh_actor_motion_status advance_sequence_effects_0014fb48(gh_enemy202_move *s,uint8_t *p)
{
    const uint32_t dependency=word(p+0x564);
    if(s->pending_ai_pc!=0x21a538||
       (dependency!=0x200&&(dependency!=0x203||half(p+0x640)!=19))||
       (uint16_t)s->enemy->b.entry.work->room!=0x100)
        return stop(s,0x14fb48,GH_ACTOR_MOTION_UNSUPPORTED);
    const unsigned count=half(p+0x3ae);
    if(count>8)return stop(s,0x14fb48,GH_ACTOR_MOTION_BOUNDS);
    const gh_title_player_view *actor=&s->enemy->entry.object.view;
    gh_effect_create_call *call=&s->death_effect_create;
    gh_effect_create_bindings bindings=s->impact_bindings;
    bindings.model=&s->enemy->entry.initialize.model.model;
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

static gh_actor_motion_status death_sequence_0014fb48(gh_enemy202_move *s,const uint8_t *p)
{
    const unsigned count=half(p+0x3ae);
    if(count>8)return stop(s,0x14fb48,GH_ACTOR_MOTION_BOUNDS);
    if(!((s->dispatch_pc==0x25e600&&(s->pending_ai_pc==0x25e790||s->pending_ai_pc==0x25ed00))||
         (s->dispatch_pc==0x24c0f0&&s->pending_ai_pc==0x253de8))||
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
    if(!((s->dispatch_pc==0x25e600&&(s->pending_ai_pc==0x25e790||s->pending_ai_pc==0x25ed00||
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
            gh_actor_motion_status status=(s->dispatch_pc==0x25e600||s->pending_ai_pc==0x253de8)?
                death_sequence_0014fb48(s,p):advance_sequence_effects_0014fb48(s,p);
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
        const int retired_body=s->dispatch_pc==0x25e600&&s->pending_ai_pc==0x25f1f0&&
            p[0x2f6]==4&&(word(p+0x250)&0x80000000u)&&!enemy->hitareas.created[0]&&!word(p+0x698);
        if(!retired_body&&(!enemy->hitareas.created[0]||gh_hitarea_object_actor(body,&owner)||
           (!body->fallback&&owner!=&enemy->entry.object.view)))
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

/* Original 26E588 home radius for R100 dependencies 0200 and 0203. */
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

/* Original 2630F0 facing and flank gates shared by R100 0200 and 0203. */
static gh_actor_motion_status r100_attack_in_range_002630f0(gh_enemy202_move *s,uint8_t *p,
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

    if(word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
        return stop(s,0x263560,GH_ACTOR_MOTION_UNSUPPORTED);
    if(word(p+0x564)==0x200){
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
                return commit_attack_0026ee40(s,p,random,heat,0x42,100,300,chosen);
        }
        if(distance<(heat<2?4.0f:6.25f)){
            if(gh_game_mt_next_0030f348(&random,&draw))
                return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
            /* EM00 uses the default case at 263EC8 and the 270D00 height gate. */
            const unsigned substate=!(draw&7u)&&height<=number(0x3dcccccdu)?0x4b:0x15;
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


static gh_actor_motion_status attack_choice_r100_common(gh_enemy202_move *s,uint8_t *p,
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
            else return r100_attack_in_range_002630f0(s,p,distance,chosen);
        }
    }
    *chosen=1;
    return GH_ACTOR_MOTION_OK;
}

static int stamp19_close(const gh_enemy202_move *s,const uint8_t *p,float distance)
{
    return s->enemy->b.entry.work->room==0x100&&word(p+0x564)==0x203&&
        half(p+0x640)==19&&isfinite(distance)&&distance>=0&&distance<16.0f;
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
    if(stamp19_close(s,p,distance)||ordinary){
        if(!isfinite(distance)||distance<0||!isfinite(face))
            return stop(s,0x262aa8,GH_ACTOR_MOTION_UNSUPPORTED);
        return attack_choice_r100_common(s,p,distance,chosen);
    }
    if((word(p+0x564)==0x200||word(p+0x564)==0x203)&&enemy->b.entry.work->room==0x100)
        return attack_choice_ordinary_far(s,p,distance,face,chosen);
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
            if(dependency!=0x20a&&dependency!=0x200&&dependency!=0x203)return stop(s,0x2120e8,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!s->substate67_motion.active){
                float actor[3],target[3],turn;
                if(!position(&enemy->entry.object.view,actor)||!position(s->player,target)||
                    gh_player_target_angle_002da038(s->elf,s->elf_bytes,actor,target,
                        number(word(p+0x104)),number(0x40490fdb),&turn,&pc))
                    return stop(s,pc?pc:0x2da038,GH_ACTOR_MOTION_UNSUPPORTED);
            }
            /* This dependency selects the same clip for either turn sign. */
            offset=dependency==0x200?0xb0:dependency==0x203?0x650:0x830;
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
            if(!chosen){
                const float distance=number(word(p+0x618));
                const uint32_t rank=word(p+0x16d8);
                if(stamp19_close(s,p,distance)||(dependency==0x200&&
                    enemy->b.entry.work->room==0x100&&distance<=144.0f)){
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
                if(!position(&enemy->entry.object.view,actor)||
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
        const uint8_t *work=(const uint8_t *)enemy->b.entry.work;
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
                if(!position(&enemy->entry.object.view,actor)||!position(&peer.view,candidate))
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

    if(!s->substate67_stage){
        const uint32_t dependency=word(p+0x564);
        if(dependency!=0x200&&dependency!=0x202&&dependency!=0x203)
            return stop(s,0x2147d0,GH_ACTOR_MOTION_UNSUPPORTED);
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]==0?SELECT:p[0x2f6]==1?TURN:ATTACK;
    }
    if(s->substate67_stage==SELECT){
        if(word(p+0x6ec))return stop(s,0x214ba0,GH_ACTOR_MOTION_UNSUPPORTED);
        size_t offset=word(p+0x564)==0x200?0xcc:0x66c;
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
                motion,&motion->b,word(row),word(row+4),0,10,p[0x17c3]?2:0,1);
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
        CHECK_PLAYER, ATTACK, CHECK_DOOR, CHOOSE };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_game_init_state *init=enemy->b.entry.init;
    gh_boot_work *work=enemy->b.entry.work;
    const gh_title_player_view *actor=&enemy->entry.object.view;
    const uint32_t dependency=word(p+0x564);
    const float distance=number(word(p+0x618));
    uint32_t pc=0;
    if((dependency!=0x200&&dependency!=0x203)||work->room!=0x100||!isfinite(distance)||distance<0)
        return stop(s,0x2130c8,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]==0?BIND:p[0x2f6]==1?TURN:CHECK_PLAYER;
    }
    if(s->substate67_stage==BIND){
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const gh_object_view *resource=&enemy->entry.resource;
            const size_t row=dependency==0x200?0xb0:0x650;
            if(word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
                return stop(s,0x2133a4,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<row+8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x213390,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+row+4),
                0,10,p[0x17c3]?2:0,1);
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
        if((int32_t)word(p+0x16ec)>0)enemy_decision(p,0,6,0);
        else{
            gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p);
            if(status)return status;
            /* Original 24BDA0 returns zero in room 0100. */
        }
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
    if(word(p+0x564)!=0x200||work->room!=0x100||!isfinite(distance)||distance<0)
        return stop(s,0x213710,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        s->substate67_stage=p[0x2f6]==0?72:p[0x2f6]==1?74:79;
    }
    if(s->substate67_stage==72){
        if(word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
            return stop(s,0x213a5c,GH_ACTOR_MOTION_UNSUPPORTED);
        const gh_object_view *resource=&enemy->entry.resource;
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0xc8||
                resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x213a58,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+0xc0),word(resource->bytes+0xc4),
                0,10,p[0x17c3]?2:0,1);
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
        /* Original 24BDA0 excludes room 0100 before querying its AT owner. */
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
    const int stamp10=dependency==0x200&&half(p+0x640)==10&&p[0x1650]==5;
    if((dependency!=0x203&&!stamp10)||work->room!=0x100||word(p+0x16c4)!=6)
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
            /* 218D08's dependency table begins at 0202. EM00 uses the
             * original 218D30 fallback row while returning home. */
            const size_t row=stamp10?0xc0:0x660;
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
            enemy_decision(p,0,3,0);
            put(p+0x16d0,word(p+0x16d0)&~2u);
            s->substate67_stage=5;
        }else s->substate67_stage=100;
    }
    if(s->substate67_stage==100){
        gh_actor_motion_status status=enemy_door_ray_0024bb68(s,p);
        if(status)return status;
        /* Original 24BDA0 excludes R100. */
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

/* Original 23B968 dep0200, R100 stamp20's no-held-weapon phase0/1. */
static gh_actor_motion_status advance_substate6b_0023b968(gh_enemy202_move *s,uint8_t *p)
{
    enum { SELECT_MOTION=101, START_MOTION, TURN_TO_PLAYER, STEP_MOTION,
           SELECT_AFTER, ROOT_ROTATION, ROOT_TRANSLATION, CUE, CUE_SOUND };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_boot_work *work=enemy->b.entry.work;
    gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    if(word(p+0x564)!=0x200||half(p+0x640)!=20||p[0x1650]||work->room!=0x100)
        return stop(s,0x23b968,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        put(p+0x16d0,word(p+0x16d0)|0x400u);
        if(p[0x2f6]>1)return stop(s,0x23b968,GH_ACTOR_MOTION_UNSUPPORTED);
        if(!p[0x2f6])p[0x1864]=0;
        s->substate67_stage=p[0x2f6]?TURN_TO_PLAYER:SELECT_MOTION;
    }
    if(s->substate67_stage==SELECT_MOTION){
        const gh_object_view *resource=&enemy->entry.resource;
        if(!resource->bytes||!resource->file_extent_known||resource->file_extent<0x3f4||
            resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
            return stop(s,0x23ba2c,GH_ACTOR_MOTION_BOUNDS);
        uint32_t random;
        if(gh_game_mt_next_0030f348(&enemy->entry.b.init->random,&random))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        static const uint16_t rows[]={0x3c4,0x3cc,0x3d4,0x3dc,0x3ec};
        s->reaction_resource_offset=rows[random%5u];
        s->substate67_stage=START_MOTION;
    }
    if(s->substate67_stage==START_MOTION){
        if(word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
            return stop(s,0x23c308,GH_ACTOR_MOTION_UNSUPPORTED);
        gh_actor_motion_call *call=&s->substate67_motion;
        if(!call->active){
            const uint8_t *row=enemy->entry.resource.bytes+s->reaction_resource_offset;
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                call,motion,&motion->b,word(row),word(row+4),0,10,p[0x17c3]?2:0,1);
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

static gh_actor_motion_status stamp19_attack_area_00260b30(gh_enemy202_move *s,uint8_t *p)
{
    const unsigned hit_flags=half(p+0x3ac)&3u;
    if(!hit_flags)return GH_ACTOR_MOTION_OK;
    const unsigned params=hit_flags&2u?0x1884:0x1870;
    const unsigned profile=word(p+params);
    if(profile!=0&&profile!=2)return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    gh_enemy_actor *enemy=s->enemy;
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
    memcpy(euler,p+0x100,sizeof(euler));
    if(gh_actor_matrix_rotation_00147728(euler,0,rotation,GH_STATIC_BINARY32_REFERENCE,&pc))
        return stop(s,pc?pc:0x1fd338,GH_ACTOR_MOTION_UNSUPPORTED);
    fenv_t env;
    const float scale=number(word(p+0x114)),power_rate=number(word(p+0x76c));
    if(!isfinite(scale)||scale==0||!isfinite(power_rate)||!set_round(&env,FE_TOWARDZERO))
        return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    int32_t power=(int32_t)word(p+params+4);
    if(word(p+0x16ec))power+=power/2;
    const float scaled_power=mul((float)power,power_rate);
    volatile float reciprocal=1.0f/scale;
    const float height=profile==2?mul(reciprocal,0.7f):reciprocal;
    const uint8_t *work=(const uint8_t *)enemy->b.entry.work;
    if(!isfinite(scaled_power)||scaled_power < -2147483648.0f||scaled_power >= 2147483648.0f){
        (void)fesetenv(&env);
        return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    power=(int32_t)scaled_power;
    if(word(work+0x594)&0x800u)power=(int32_t)mul((float)power,0.8f);
    if(fesetenv(&env)||!isfinite(height))return stop(s,0x260b30,GH_ACTOR_MOTION_UNSUPPORTED);
    const float front[]={0,1.3f,0.75f},base[]={0,1.3f,0};
    const float box[]={0,height,0.75f};
    if(gh_collision_pair_shape_offset_002bb258(&shapes[3],base,&pc)||
       gh_collision_pair_shape_offset_002bb258(&shapes[0],front,&pc)||
       gh_collision_pair_shape_offset_002bb258(&shapes[1],front,&pc)||
       gh_collision_pair_shape_offset_002bb258(&shapes[3],box,&pc))
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
    memcpy(give[3].bytes+0x10,rotation+8,12);
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status advance_r100_attack_0021a538(gh_enemy202_move *s,uint8_t *p)
{
    enum { SELECT_ATTACK=160, OPEN_ATTACK, TRACK_ATTACK, STEP_ATTACK,
        ATTACK_ROTATION, ATTACK_TRANSLATION, FINISH_ATTACK };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_game_init_state *init=enemy->b.entry.init;
    const int em00=word(p+0x564)==0x200;
    uint32_t pc=0;
    if(enemy->b.entry.work->room!=0x100||(!em00&&(word(p+0x564)!=0x203||half(p+0x640)!=19))||
        word(p+0x6ec)||gh_enemy_timers_held_weapon(&enemy->timers))
        return stop(s,0x21a538,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->substate67_stage){
        if(p[0x2f6]>1)return stop(s,0x21f8f8,GH_ACTOR_MOTION_UNSUPPORTED);
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
    if(s->substate67_stage==SELECT_ATTACK){
        const unsigned heat=heat_category(s);
        const float distance=number(word(p+0x618));
        if(!heat||!isfinite(distance))return stop(s,0x21b1c8,GH_ACTOR_MOTION_UNSUPPORTED);
        uint32_t random;
        if(gh_game_mt_next_0030f348(&init->random,&random))
            return stop(s,0x31cfe0,GH_ACTOR_MOTION_UNSUPPORTED);
        const unsigned choice=random%3;
        unsigned row,power=20,shape=0,kind=0x37,tracking=15,timed=0;
        if(em00){
            /* 21A654 default starts with an EM00 resource row, then applies
             * the independent alternate and distance decisions at 21A860. */
            if(heat==1||heat==5)row=(random&1u)?0x1ac:0x150;
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
        put(p+0x5f0,(uint32_t)((float)tracking*factors[heat]));
        put(p+0x5f4,timed);put(p+0x5fc,0);put(p+0x604,0);put(p+0x608,0x3f800000);put(p+0x60c,0);
        p[0x1864]=0;
        stamp19_attack_parameters(p,power,shape,kind);
        s->reaction_resource_offset=(uint16_t)row;
        s->substate67_stage=OPEN_ATTACK;
    }
    if(s->substate67_stage==OPEN_ATTACK){
        const gh_object_view *resource=&enemy->entry.resource;
        const size_t row=s->reaction_resource_offset;
        if(!s->substate67_motion.active){
            const unsigned heat=heat_category(s);
            const size_t seq=row+(heat==5&&(em00||row==0x6a4||row==0x6dc||row==0x6f4)?8:4);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent>resource->size||
               resource->file_extent<seq+4)
                return stop(s,0x21f5a4,GH_ACTOR_MOTION_BOUNDS);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(
                &s->substate67_motion,motion,&motion->b,word(resource->bytes+row),
                word(resource->bytes+seq),0,10,p[0x17c3]?2:0,em00);
            if(status)return stop(s,s->substate67_motion.frontier?s->substate67_motion.frontier:0x2a8578,status);
        }
        gh_actor_motion_status status=gh_actor_motion_call_step_002a8578(&s->substate67_motion);
        if(status)return stop(s,s->substate67_motion.frontier?s->substate67_motion.frontier:0x2a8578,status);
        put(p+0x600,word(p+0x104));
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
        s->substate67_stage=ATTACK_ROTATION;
    }
    if(s->substate67_stage==ATTACK_ROTATION){
        if(word(p+0x608)!=0x3f800000u)return stop(s,0x21f8d0,GH_ACTOR_MOTION_UNSUPPORTED);
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
        if(s->state71_exit){
            if(word(s->player->bytes+0x15f4)&0x1000u)enemy_decision(p,0,0x6c,0);
            else{
                gh_actor_motion_status status=choose_after_substate_002705d8(s,p);
                if(status)return status;
            }
        }else{
            if(word(p+0x5f4)||word(p+0x5fc)||(heat_category(s)>=3&&(half(p+0x3ac)&0x10)))
                return stop(s,0x21f8f8,GH_ACTOR_MOTION_UNSUPPORTED);
            gh_actor_motion_status status=stamp19_attack_area_00260b30(s,p);
            if(status)return status;
        }
        s->substate67_stage=5;
    }
    return advance_state0_tail(s,p);
}

static gh_actor_motion_status advance_r100_substate4b_00234d30(gh_enemy202_move *s,uint8_t *p)
{
    enum { OPEN_MOTION=170, ADVANCE_OFFSET, TURN, SAMPLE, CHOOSE, ROTATE, TRANSLATE, FINISH };
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    gh_boot_work *work=enemy->b.entry.work;
    gh_title_player_view *actor=&enemy->entry.object.view;
    uint32_t pc=0;
    const int em00=word(p+0x564)==0x200;
    if(work->room!=0x100||(!em00&&(word(p+0x564)!=0x203||half(p+0x640)!=19)))
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
            const size_t row=em00?0x50c:0x7cc;
            const size_t sequence=row+(rank==5?8:4);
            if(!rank)return stop(s,0x1fa010,GH_ACTOR_MOTION_UNSUPPORTED);
            if(!resource->bytes||!resource->file_extent_known||resource->file_extent<sequence+4||
               resource->file_extent>resource->size||motion->b.resource!=resource->bytes)
                return stop(s,0x234e24,GH_ACTOR_MOTION_BOUNDS);
            static const float factor[]={0,.75f,.8f,.9f,.9f,1};
            fenv_t env;
            if(!set_round(&env,FE_TOWARDZERO))return stop(s,0x235648,GH_ACTOR_MOTION_CALLEE);
            const uint32_t tracking=(uint32_t)mul(20.0f,factor[rank]);
            if(fesetenv(&env))return stop(s,0x235648,GH_ACTOR_MOTION_CALLEE);
            p[0x1864]=0;
            put(p+0x5f8,0);
            put(p+0x5f0,tracking);
            put(p+0x5f4,0);
            memset(p+0x580,0,12);
            gh_actor_motion_status status=gh_actor_motion_call_open_parameters_002a8578(call,
                motion,&motion->b,word(resource->bytes+row),word(resource->bytes+sequence),
                0,10,p[0x17c3]?2:0,em00);
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
            const float cooldown=number(word(s->player->bytes+0x54c));
            if(!(cooldown>0)&&!(work->system_flags&8u)&&
               (int16_t)half(s->player->bytes+0x54a)>0)
                return stop(s,0x270db0,GH_ACTOR_MOTION_UNSUPPORTED);
        }
        if(heat_category(s)>=3)return stop(s,0x26f1d8,GH_ACTOR_MOTION_UNSUPPORTED);
        if(flags&3u)put(p+0x5fc,1);
        if(word(p+0x5fc))put(p+0x16d4,word(p+0x16d4)&~0x400u);
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
    if(s->pending_ai_pc==0x2112f0)return advance_substate3_002112f0(s,p);
    if(s->pending_ai_pc==0x23aad0)return advance_substate67(s,p);
    if(s->pending_ai_pc==0x211c08)return advance_substate0_00211c08(s,p);
    if(s->pending_ai_pc==0x2130c8)return advance_substate5_002130c8(s,p);
    if(s->pending_ai_pc==0x213710)return advance_substate6_00213710(s,p);
    if(s->pending_ai_pc==0x218c48)return advance_substate13_00218c48(s,p);
    if(s->pending_ai_pc==0x214298)return advance_substate8_00214298(s,p);
    if(s->pending_ai_pc==0x2147d0)return advance_substate9_002147d0(s,p);
    if(s->pending_ai_pc==0x23b968)return advance_substate6b_0023b968(s,p);
    if(s->pending_ai_pc==0x234d30)return advance_r100_substate4b_00234d30(s,p);
    if(s->pending_ai_pc==0x21a538)return advance_r100_attack_0021a538(s,p);
    return stop(s,s->pending_ai_pc,GH_ACTOR_MOTION_UNSUPPORTED);
}


static gh_actor_motion_status advance_first_reaction_0024c3c8(gh_enemy202_move *s,uint8_t *p)
{
    gh_enemy_actor *enemy=s->enemy;
    gh_actor_motion *motion=&enemy->motion.motion;
    const gh_hitarea_object *body=&enemy->hitareas.units[0];
    uint32_t pc=0;
    const int triangle=s->pending_ai_pc==0x24da80;
    const uint32_t entry=triangle?0x24da80:0x24c3c8;
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
        if(!wait)return stop(s,triangle?0x24e334:0x24cf10,GH_ACTOR_MOTION_UNSUPPORTED);
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
        if(!(((dependency==0x20a||stamp19||stamp20)&&p[0x2f5]==0)||
            ((stamp19||stamp20)&&(p[0x2f5]==1||(p[0x2f5]==7&&!positive_hp(p))))||
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
        const uint8_t *descriptor=elf_span(s->elf,s->elf_bytes,0x3bf738+8u*p[0x2f5],8);
        if(!descriptor||half(descriptor)||half(descriptor+2)!=0xffff||word(descriptor+4)!=expected)
            return stop(s,0x24c2e8,GH_ACTOR_MOTION_UNSUPPORTED);
        s->pending_ai_pc=word(descriptor+4);
        s->state1_phase=6;
    }
    if(s->pending_ai_pc==0x258d80)return advance_scenery_reaction_00258d80(s,p);
    if(s->pending_ai_pc==0x253de8)return advance_lethal_reaction_00253de8(s,p);
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
           !((dependency==0x203&&stamp==19)||(dependency==0x200&&stamp==20)))
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
static gh_actor_motion_status play_hit_sound(gh_enemy202_move *s, gh_actor_model *model)
{
    gh_enemy_actor *enemy=s->enemy;
    const gh_actor_motion *motion=&enemy->motion.motion;
    const gh_hitarea_object *body=&enemy->hitareas.units[0];
    gh_title_player_view *view=&enemy->entry.object.view;
    if(!motion->b.sound)return stop(s,0x2cba48,GH_ACTOR_MOTION_UNSUPPORTED);
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
    uint32_t handle,pc=0;
    gh_actor_motion_status status=motion->b.sound(motion->b.sound_context,&s->hit_sound,&handle,&pc);
    return status?stop(s,pc?pc:0x2cba48,status):GH_ACTOR_MOTION_OK;
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
                    if(s->damage.effect==0x2e && (half(p+0x3ac)&0x100u)){
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
        if(!s->dispatch_pc){
            if(p[0x2f4]>=4)return stop(s,0x209654,GH_ACTOR_MOTION_UNSUPPORTED);
            const uint8_t *descriptor=elf_span(elf,elf_bytes,0x3bf1a0+8u*p[0x2f4],8);
            if(!descriptor||half(descriptor)||half(descriptor+2)!=0xffff)
                return stop(s,0x209654,GH_ACTOR_MOTION_UNSUPPORTED);
            s->dispatch_pc=word(descriptor+4);
        }
        if(s->dispatch_pc==0x24c0f0)return advance_state1_0024c0f0(s,p);
        if(s->dispatch_pc==0x25e600)return advance_death_0025e600(s,p);
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
