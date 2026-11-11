#include "game_enemy_route_graph.h"

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
