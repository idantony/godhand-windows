#include "game_enemy_route_graph.h"
#include "game_ray_query.h"

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
