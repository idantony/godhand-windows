#include "game_enemy_route_graph.h"
#include "game_ray_query.h"
#include "game_init.h"

#include <fenv.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static uint16_t half(const uint8_t *bytes)
{
    uint16_t value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static uint32_t word(const uint8_t *bytes)
{
    uint32_t value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static void put(uint8_t *p,uint32_t v){memcpy(p,&v,4);}
static void put_half(uint8_t *p,uint16_t v){memcpy(p,&v,2);}
static float number(uint32_t bits){float v;memcpy(&v,&bits,4);return v;}
static uint32_t bits(float value){uint32_t v;memcpy(&v,&value,4);return v;}

static int set_round(fenv_t *env,int mode)
{
    if(feholdexcept(env))return 0;
    if(!fesetround(mode))return 1;
    (void)fesetenv(env);
    return 0;
}

static float add(float a, float b) { volatile float value = a + b; return value; }
static float sub(float a, float b) { volatile float value = a - b; return value; }
static float mul(float a, float b) { volatile float value = a * b; return value; }
static float div32(float a, float b) { volatile float value = a / b; return value; }

static float dot(const float a[3], const float b[3])
{
    return add(add(mul(a[0], b[0]), mul(a[1], b[1])), mul(a[2], b[2]));
}

static void cross(const float a[3], const float b[3], float result[3])
{
    result[0] = sub(mul(a[1], b[2]), mul(a[2], b[1]));
    result[1] = sub(mul(a[2], b[0]), mul(a[0], b[2]));
    result[2] = sub(mul(a[0], b[1]), mul(a[1], b[0]));
}

static void normalize(float xyz[3])
{
    const float length = sqrtf(dot(xyz, xyz));
    if (length > 0) {
        const float inverse = div32(1.0f, length);
        for (unsigned i = 0; i < 3; ++i) xyz[i] = mul(xyz[i], inverse);
    } else {
        memset(xyz, 0, 3 * sizeof(*xyz));
    }
}

/* 1566C0 picks an endpoint if the point projects beyond the line segment.
 * The interior distance uses the 1565A0 double cross and normalization. */
static float edge_distance(const float origin[4], const float a[4], const float b[4],
                           float normal[3], unsigned *region)
{
    float from_a[3], from_b[3], direction[3], crossed[3];
    for (unsigned i = 0; i < 3; ++i) {
        from_a[i] = sub(a[i], origin[i]);
        from_b[i] = sub(b[i], origin[i]);
        direction[i] = sub(b[i], a[i]);
        crossed[i] = -from_a[i];
    }
    float first_cross[3];
    cross(direction, crossed, first_cross);
    cross(direction, first_cross, normal);
    const float distance_a = sqrtf(dot(from_a, from_a));
    const float distance_b = sqrtf(dot(from_b, from_b));
    const unsigned closest = distance_b <= distance_a;
    const float distance = closest ? distance_b : distance_a;
    const float endpoint_limit = sqrtf(add(dot(direction, direction), mul(distance, distance)));
    if (endpoint_limit <= distance_a || endpoint_limit <= distance_b) {
        *region = closest;
        return distance;
    }
    float unit[3];
    memcpy(unit, normal, sizeof(unit));
    normalize(unit);
    *region = 2;
    return dot(unit, from_a);
}

/* 156BE8 projects through the normal point computed by 1569A0. The
 * point addition and subtraction are separate original binary32 steps. */
static void edge_projection(const float origin[4], const float a[4],
                            const float normal[3], float point[4])
{
    float axis[3], unit[3], from_a[3];
    for (unsigned i = 0; i < 3; ++i) {
        from_a[i] = sub(a[i], origin[i]);
        axis[i] = sub(add(origin[i], normal[i]), origin[i]);
        unit[i] = normal[i];
    }
    normalize(unit);
    const float distance = dot(unit, from_a);
    normalize(axis);
    for (unsigned i = 0; i < 3; ++i) point[i] = add(origin[i], mul(axis[i], distance));
    point[3] = 1.0f;
}

static gh_actor_motion_status collect_edges(const gh_scene_nearcheck *nav,
                                            const float origin[4],
                                            gh_enemy_route_edge_candidate candidates[4])
{
    for (unsigned i = 0; i < 4; ++i) {
        candidates[i].edge.pair = UINT32_MAX;
        candidates[i].distance = FLT_MAX;
    }
    unsigned worst = 0;
    for (uint32_t i = 0; i < nav->pair_count; ++i) {
        const uint8_t *pair = nav->pairs.bytes + (size_t)i * 12;
        if ((word(pair + 8) & 0xc0000000u) == 0x80000000u) continue;
        const uint16_t a = half(pair), b = half(pair + 2);
        if (a >= nav->vertex_count || b >= nav->vertex_count) return GH_ACTOR_MOTION_BOUNDS;
        float start[4], end[4], normal[3];
        memcpy(start, nav->vertices.bytes + (size_t)a * 32, sizeof(start));
        memcpy(end, nav->vertices.bytes + (size_t)b * 32, sizeof(end));
        unsigned region;
        const float distance = edge_distance(origin, start, end, normal, &region);
        if (!isfinite(distance)) return GH_ACTOR_MOTION_UNSUPPORTED;
        if (distance >= candidates[worst].distance) continue;
        gh_enemy_route_edge_candidate *candidate = &candidates[worst];
        candidate->distance = distance;
        candidate->edge.pair = i;
        candidate->edge.vertices[0] = a;
        candidate->edge.vertices[1] = b;
        edge_projection(origin, start, normal, candidate->edge.projection);
        const float *target = region == 2 ? candidate->edge.projection : region == 0 ? start : end;
        memcpy(candidate->ray_target, target, sizeof(candidate->ray_target));
        worst = 0;
        for (unsigned slot = 1; slot < 4; ++slot)
            if (candidates[worst].distance < candidates[slot].distance) worst = slot;
    }
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_route_edge_002a8038(
    gh_enemy_route_edge_query *query, const gh_scene_nearcheck *nav,
    gh_game_scene_tables *actors, const float origin[4],
    gh_enemy_route_edge *edge, uint32_t *frontier)
{
    if (!query || !nav || !actors || !origin || !edge || !frontier) return GH_ACTOR_MOTION_ARGUMENT;
    *frontier = 0x2a8038;
    if ((nav->pair_count && !nav->pairs.bytes) || (nav->vertex_count && !nav->vertices.bytes) ||
        nav->pair_count > nav->pairs.size / 12 || nav->vertex_count > nav->vertices.size / 32)
        return GH_ACTOR_MOTION_BOUNDS;
    if (query->navigation) {
        if (query->navigation != nav || query->actors != actors || memcmp(query->origin, origin, 16))
            return GH_ACTOR_MOTION_BUSY;
    } else {
        fenv_t env;
        if (feholdexcept(&env)) return GH_ACTOR_MOTION_CALLEE;
        if (fesetround(FE_TOWARDZERO)) {
            (void)fesetenv(&env);
            return GH_ACTOR_MOTION_CALLEE;
        }
        const gh_actor_motion_status status = collect_edges(nav, origin, query->candidates);
        if (fesetenv(&env)) return GH_ACTOR_MOTION_CALLEE;
        if (status) return status;
        query->navigation = nav;
        query->actors = actors;
        memcpy(query->origin, origin, sizeof(query->origin));
        query->ray_cursor = 0;
    }
    for (; query->ray_cursor < 4; ++query->ray_cursor) {
        gh_enemy_route_edge_candidate *candidate = &query->candidates[query->ray_cursor];
        if (candidate->edge.pair == UINT32_MAX) continue;
        gh_scene_ray_request ray = {.mask=7, .flags=0x60, .segmented=1};
        memcpy(ray.from, origin, sizeof(ray.from));
        memcpy(ray.to, candidate->ray_target, sizeof(ray.to));
        int hit;
        if (gh_scene_ray_query_00131e10(actors, &ray, &hit, frontier)) {
            if (!*frontier) *frontier = 0x131e10;
            return GH_ACTOR_MOTION_UNSUPPORTED;
        }
        if (hit) {
            candidate->edge.pair = UINT32_MAX;
            candidate->distance = FLT_MAX;
        }
    }
    unsigned best = 0;
    for (unsigned i = 1; i < 4; ++i)
        if (query->candidates[i].distance < query->candidates[best].distance) best = i;
    *edge = query->candidates[best].edge;
    *frontier = 0;
    return GH_ACTOR_MOTION_OK;
}

typedef struct {
    uint32_t vertex;
    unsigned next;
    float cost;
} route_level;

gh_actor_motion_status gh_enemy_route_graph_0031be88(
    const gh_scene_nearcheck *nav, uint32_t origin, uint32_t destination,
    unsigned max_depth, uint32_t *output, unsigned capacity, float *cost)
{
    if (!nav || !cost || (capacity && !output)) return GH_ACTOR_MOTION_ARGUMENT;
    if (origin >= nav->vertex_count || destination >= nav->vertex_count)
        return GH_ACTOR_MOTION_BOUNDS;
    if (origin == destination) {
        if (capacity) output[0] = destination;
        if (capacity > 1) output[1] = 0xffff;
        *cost = 0;
        return GH_ACTOR_MOTION_OK;
    }
    /* 2A74E0 computes a maximum of 64, with height and room multipliers. */
    if (max_depth < 2 || max_depth > 120 || nav->vertex_count > 0xffff ||
        !nav->vertices.bytes || !nav->pairs.bytes || !nav->indices.bytes ||
        nav->vertex_count > nav->vertices.size / 32 ||
        nav->pair_count > nav->pairs.size / 12 ||
        nav->index_count > nav->indices.size / 2)
        return GH_ACTOR_MOTION_BOUNDS;

    float *best_cost = malloc((size_t)nav->vertex_count * sizeof(*best_cost));
    uint8_t *active = calloc(nav->vertex_count, 1);
    if (!best_cost || !active) {
        free(active);
        free(best_cost);
        return GH_ACTOR_MOTION_CALLEE;
    }
    for (unsigned i = 0; i < nav->vertex_count; ++i) best_cost[i] = FLT_MAX;
    route_level stack[120] = {{origin, 0, 0}};
    uint32_t path[120];
    unsigned depth = 1, path_count = 0;
    float path_cost = FLT_MAX;
    gh_actor_motion_status status = GH_ACTOR_MOTION_OK;
    fenv_t original;
    if (fegetenv(&original)) {
        status = GH_ACTOR_MOTION_CALLEE;
        goto release;
    }
    if (fesetround(FE_TOWARDZERO)) {
        (void)fesetenv(&original);
        status = GH_ACTOR_MOTION_CALLEE;
        goto release;
    }
    while (depth) {
        route_level *level = &stack[depth - 1];
        uint32_t node = level->vertex;
        active[node] = 1;
        best_cost[node] = level->cost;
        if (node == destination) {
            path_count = depth + 1;
            for (unsigned i = 0; i < depth; ++i) path[i] = stack[i].vertex;
            path[depth] = 0xffff;
            path_cost = level->cost;
            active[node] = 0;
            --depth;
            continue;
        }
        const uint8_t *vertex = nav->vertices.bytes + (size_t)node * 32;
        uint32_t offset = word(vertex + 16), count = word(vertex + 20);
        if ((offset & 1) || offset / 2 > nav->index_count ||
            count > nav->index_count - offset / 2) {
            status = GH_ACTOR_MOTION_BOUNDS;
            break;
        }
        if (level->next == count) {
            active[node] = 0;
            --depth;
            continue;
        }
        uint16_t index = half(nav->indices.bytes + offset + (size_t)level->next++ * 2);
        if (index >= nav->pair_count) {
            status = GH_ACTOR_MOTION_BOUNDS;
            break;
        }
        const uint8_t *pair = nav->pairs.bytes + (size_t)index * 12;
        uint32_t flags = word(pair + 8);
        if ((flags & 0xc0000000u) == 0x80000000u) continue;
        uint16_t a = half(pair), b = half(pair + 2);
        if ((flags & 0x20000000u) && b == node) continue;
        uint16_t next = a == node ? b : a;
        if ((a != node && b != node) || next >= nav->vertex_count) {
            status = GH_ACTOR_MOTION_BOUNDS;
            break;
        }
        if (active[next]) continue;
        float distance;
        memcpy(&distance, pair + 4, sizeof(distance));
        volatile float next_cost = level->cost + distance;
        if (!isfinite(distance) || distance < 0 || !isfinite(next_cost)) {
            status = GH_ACTOR_MOTION_UNSUPPORTED;
            break;
        }
        if (next_cost <= best_cost[next] && depth < max_depth - 1)
            stack[depth++] = (route_level){next, 0, next_cost};
    }
    if (fesetenv(&original)) status = GH_ACTOR_MOTION_CALLEE;
    if (!status) {
        unsigned copied = capacity < path_count ? capacity : path_count;
        if (copied) memcpy(output, path, copied * sizeof(*output));
        *cost = path_cost;
    }
release:
    free(active);
    free(best_cost);
    return status;
}

static gh_actor_motion_status route_stop(gh_enemy_route_call *s,uint32_t frontier,
    gh_actor_motion_status status)
{
    s->frontier=frontier;
    return status;
}

static gh_actor_motion_status navigation_lookup(gh_enemy_route_call *s,gh_enemy_route_query *q,
    const float point[4],uint32_t *vertex)
{
    const gh_scene_nearcheck *nav=s->navigation;
    if(!nav)return route_stop(s,0x2a7cd0,GH_ACTOR_MOTION_UNSUPPORTED);
    if(nav->vertex_count&&(!nav->source||!nav->vertices.bytes||
        nav->vertex_count>nav->vertices.size/32))return route_stop(s,0x2a7d94,GH_ACTOR_MOTION_BOUNDS);
    if(q->initialized&&(q->owner!=nav||memcmp(q->point,point,16)))return GH_ACTOR_MOTION_BUSY;
    if(!q->initialized){
        fenv_t env;
        if(!set_round(&env,FE_TONEAREST))return route_stop(s,0x1f7e28,GH_ACTOR_MOTION_CALLEE);
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
        if(fesetenv(&env)||!valid)return route_stop(s,0x2a7db8,GH_ACTOR_MOTION_UNSUPPORTED);
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
            if(index>=nav->vertex_count)return route_stop(s,0x2a7ed0,GH_ACTOR_MOTION_BUSY);
            gh_scene_ray_request ray={.mask=7,.flags=0x60,.segmented=1};
            memcpy(ray.from,q->point,16);
            memcpy(ray.to,nav->vertices.bytes+(size_t)index*32,16);
            int hit;
            uint32_t pc;
            if(gh_scene_ray_query_00131e10(s->actors,&ray,&hit,&pc))
                return route_stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
            if(hit)q->vertices[q->index]=UINT32_MAX;
            else q->visibility=1;
        }
        q->complete=1;
    }
    *vertex=UINT32_MAX;
    for(unsigned i=0;i<8;i++)if(q->vertices[i]!=UINT32_MAX){*vertex=q->vertices[i];break;}
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status route_waypoint_ray(gh_enemy_route_call *s,
    const float origin[4],uint32_t vertex,int *hit)
{
    gh_scene_ray_request ray={.mask=7,.flags=0x60,.segmented=1};
    memcpy(ray.from,origin,16);
    memcpy(ray.to,s->navigation->vertices.bytes+(size_t)vertex*32,16);
    uint32_t pc;
    if(gh_scene_ray_query_00131e10(s->actors,&ray,hit,&pc))
        return route_stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status route_max_depth(gh_enemy_route_call *s,float length,
    float target_y,float actor_y,uint16_t room,unsigned *depth)
{
    fenv_t env;
    if(!set_round(&env,FE_TOWARDZERO))return route_stop(s,0x2a7650,GH_ACTOR_MOTION_CALLEE);
    unsigned result=16;
    if(length>=30){
        float scaled=mul(sub(length,30),0.25f);
        result=scaled>=48?64:16+(unsigned)scaled;
    }
    if(fabsf(sub(target_y,actor_y))>3)result=result*24/16;
    if(room==0x300)result=result*20/16;
    if(room==0x700)result/=2;
    if(fesetenv(&env))return route_stop(s,0x2a7650,GH_ACTOR_MOTION_CALLEE);
    *depth=result;
    return GH_ACTOR_MOTION_OK;
}

/* 2A79D0 finds a visible origin edge when the ordinary vertex ray found
 * none. 2A7AC4 removes the first path entry if the path follows that edge. */
static gh_actor_motion_status route_from_origin_edge(gh_enemy_route_call *s,uint8_t *p,
    const float origin[4],float length,float target_y,float actor_y,uint16_t room)
{
    gh_enemy_route_edge edge;
    uint32_t pc;
    gh_actor_motion_status status=gh_enemy_route_edge_002a8038(&s->origin_edge_query,
        s->navigation,s->actors,origin,&edge,&pc);
    if(status)return route_stop(s,pc?pc:0x2a8038,status);
    if(edge.pair==UINT32_MAX){
        s->route_stage=8;
        return GH_ACTOR_MOTION_OK;
    }

    put_half(p+0x5a4,(uint16_t)edge.pair);
    unsigned max_depth;
    status=route_max_depth(s,length,target_y,actor_y,room,&max_depth);
    if(status)return status;
    uint32_t nodes[16];
    float cost;
    memcpy(nodes,p+0x4d0,sizeof(nodes));
    status=gh_enemy_route_graph_0031be88(s->navigation,edge.vertices[0],s->target_vertex,
        max_depth,nodes,16,&cost);
    if(status)return route_stop(s,0x31be88,status);
    if(nodes[1]==edge.vertices[1])memmove(nodes,nodes+1,sizeof(nodes)-sizeof(*nodes));
    memcpy(p+0x4d0,nodes,sizeof(nodes));
    put(p+0x510,bits(cost));
    memcpy(p+0x520,edge.projection,12);
    p[0x530]=0;
    s->route_stage=9;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_route_clear_002a74e0(gh_enemy_route_call *s,uint8_t *p,
    const float actor[3],const float target[3],const gh_scene_nearcheck *navigation,
    gh_game_scene_tables *actors,const gh_game_init_state *init,uint16_t room)
{
    if(!s||!p||!actor||!target||!actors||!init)return GH_ACTOR_MOTION_ARGUMENT;
    if(s->route_stage&&((s->navigation&&s->navigation!=navigation)||s->actors!=actors))
        return route_stop(s,0x2a74e0,GH_ACTOR_MOTION_BUSY);
    s->navigation=navigation;
    s->actors=actors;
    s->frontier=0;
    if(!init->navigation_height_initialized)
        return route_stop(s,0x2a7530,GH_ACTOR_MOTION_UNSUPPORTED);
    float lift=number(init->navigation_height_bits);
    if(!isfinite(lift))return route_stop(s,0x2a7530,GH_ACTOR_MOTION_UNSUPPORTED);
    gh_scene_ray_request ray={.mask=7,.flags=0x60,.segmented=1};
    memcpy(ray.from,actor,12);memcpy(ray.to,target,12);
    ray.from[3]=ray.to[3]=1;
    fenv_t env;
    if(!set_round(&env,FE_TONEAREST))return route_stop(s,0x2a7554,GH_ACTOR_MOTION_CALLEE);
    for(unsigned i=0;i<3;i++){
        float offset=i==1?lift:0;
        ray.from[i]=add(ray.from[i],offset);
        ray.to[i]=add(ray.to[i],offset);
    }
    float x=sub(ray.from[0],ray.to[0]),y=sub(ray.from[1],ray.to[1]),z=sub(ray.from[2],ray.to[2]);
    float square=add(add(mul(x,x),mul(y,y)),mul(z,z));
    if(!isfinite(square)||square<0){
        (void)fesetenv(&env);
        return route_stop(s,0x1f7e28,GH_ACTOR_MOTION_UNSUPPORTED);
    }
    /* Explicit positive finite EE ChopZero reference for SQRT.S.
     * A binary32 candidate's squared value is exact in binary64. */
    float length=(float)sqrt((double)square);
    if((double)length*length>(double)square)length=nextafterf(length,0);
    if(fesetenv(&env)||!isfinite(length))
        return route_stop(s,0x2a7604,GH_ACTOR_MOTION_UNSUPPORTED);
    if(!s->route_stage){
        memcpy(p+0x520,ray.to,12);
        p[0x530]=1;
        s->origin_edge_query=(gh_enemy_route_edge_query){0};
        s->route_stage=1;
    }
    if(s->route_stage==1){
        int hit;
        uint32_t pc;
        gh_scene_ray_status status=gh_scene_ray_query_00131e10(s->actors,&ray,&hit,&pc);
        if(status)return route_stop(s,pc?pc:0x131e10,GH_ACTOR_MOTION_UNSUPPORTED);
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
            status=route_from_origin_edge(s,p,ray.from,length,target[1],actor[1],room);
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
        gh_actor_motion_status status=route_max_depth(s,length,target[1],actor[1],room,&max_depth);
        if(status)return status;
        uint32_t nodes[16];
        memcpy(nodes,p+0x4d0,sizeof(nodes));
        float cost;
        status=gh_enemy_route_graph_0031be88(s->navigation,
            s->origin_vertex,s->target_vertex,max_depth,nodes,16,&cost);
        if(status)return route_stop(s,0x31be88,status);
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
    return GH_ACTOR_MOTION_OK;
}

uint8_t gh_enemy_route_waypoint_002a7ca0(const uint8_t *p,uint8_t *destination)
{
    /* The original copies XYZ, preserves W, and returns the direct-route byte. */
    memcpy(destination,p+0x520,12);
    return p[0x530];
}
