#include "game_model_texture_scroll.h"
#include "game_actor_model_nodes.h"
#include "game_om_frame.h"

#include <math.h>
#include <string.h>

static uint32_t word(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static float number(const uint8_t *p) {
    float value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static float half_float(const uint8_t *p) {
    const uint16_t half = (uint16_t)(p[0] | (unsigned)p[1] << 8);
    const uint32_t bits = gh_om_half_bits_00156488(half);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static float accumulate(float offset, float increment, float rate) {
    const volatile float product = increment * rate;
    const volatile float total = offset + product;
    return total;
}

gh_actor_model_status gh_game_model_texture_scroll_0014e4d0(const gh_actor_model *model,
    float rate, unsigned *cursor, uint32_t *frontier) {
    if (!frontier) return GH_ACTOR_MODEL_ARGUMENT;
    *frontier = 0x14e4d0;
    if (!model || !model->ready || !model->root.bytes || model->root.size <= 0x2b1 || !cursor)
        return GH_ACTOR_MODEL_ARGUMENT;
    if (model->root.bytes[0x2b1] != model->script.mesh_count || *cursor > model->script.mesh_count) {
        *frontier = 0x14b6f8;
        return GH_ACTOR_MODEL_UNSUPPORTED;
    }
    for (; *cursor < model->script.mesh_count; ++*cursor) {
        const gh_actor_node_packets *packet;
        *frontier = 0x14b6f8;
        if (gh_actor_model_packet_at(model, *cursor, &packet) || !packet || !packet->ready || !packet->node)
            return GH_ACTOR_MODEL_BOUNDS;
        uint8_t *node = packet->node;
        if (word(node + 0x410) & 0x80000000u) continue;
        *frontier = 0x149244;
        if (word(node + 0x414)) return GH_ACTOR_MODEL_UNSUPPORTED;
        gh_model_mesh mesh;
        if (gh_model_mesh_at(&packet->script, packet->mesh_index, &mesh) ||
            mesh.descriptor_offset > packet->script.size || packet->script.size - mesh.descriptor_offset < 0x42)
            return GH_ACTOR_MODEL_BOUNDS;
        const uint8_t *descriptor = packet->script.bytes + mesh.descriptor_offset;
        float u = accumulate(number(node + 0x38c), half_float(descriptor + 0x3e), rate);
        float v = accumulate(number(node + 0x390), half_float(descriptor + 0x40), rate);
        *frontier = 0x14927c;
        if (!isfinite(u) || !isfinite(v) || fabsf(u) >= 16777216.0f || fabsf(v) >= 16777216.0f)
            return GH_ACTOR_MODEL_UNSUPPORTED;
        u -= truncf(u);
        v -= truncf(v);
        memcpy(node + 0x38c, &u, sizeof(u));
        memcpy(node + 0x390, &v, sizeof(v));
    }
    *frontier = 0;
    return GH_ACTOR_MODEL_OK;
}
