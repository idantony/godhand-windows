#include "game_collision_pair_shapes.h"
#include "game_player_attack_hitarea.h"
#include <fenv.h>
#include <math.h>
#include <string.h>

static uint32_t word(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static gh_hitarea_status stop(uint32_t *frontier, uint32_t entry, gh_hitarea_status status) {
    *frontier = entry;
    return status;
}

gh_hitarea_status gh_player_attack_hitarea_0010a438(
    const gh_title_player_view *player, const gh_hitarea_object give[5], uint32_t *frontier) {
    if (!frontier) return GH_HITAREA_ARGUMENT;
    *frontier = 0;
    if (!player || !player->bytes || player->size < 0x1684)
        return stop(frontier, 0x10a438, GH_HITAREA_ARGUMENT);
    const uint8_t *p = player->bytes;
    const unsigned flags = p[0x3ac] & 3;
    if (!flags) return GH_HITAREA_OK;

    const unsigned side = (flags & 1) ? 0 : 1;
    const unsigned shape_class = word(p + 0x1660 + side * 4);
    if (shape_class > 1)
        return stop(frontier, 0x10a608, GH_HITAREA_UNSUPPORTED);
    if (!give) return stop(frontier, 0x10a438, GH_HITAREA_ARGUMENT);

    const unsigned order[] = {1, 2, 0, 3, 4};
    gh_collision_pair_shape shapes[5];
    for (unsigned i = 0; i < 5; ++i) {
        gh_title_player_view *owner;
        const uint8_t *node;
        if (give[i].fallback || give[i].group != 1 || give[i].size < 0x50 ||
            gh_hitarea_object_actor(&give[i], &owner) || owner != player ||
            gh_hitarea_node_at(&give[i], 0, &node, &shapes[i]) ||
            word(shapes[i].record + 0xe0) != (i == 4 ? 4u : 2u))
            return stop(frontier, 0x10a438, GH_HITAREA_ARGUMENT);
        for (unsigned j = 0; j < i; ++j) {
            if (give[i].bytes == give[j].bytes)
                return stop(frontier, 0x10a438, GH_HITAREA_ARGUMENT);
        }
    }

    float euler[3], rotation[16], multiplier;
    memcpy(euler, p + 0x100, sizeof(euler));
    if (gh_actor_matrix_rotation_00147728(euler, 0, rotation, GH_STATIC_BINARY32_REFERENCE, frontier))
        return GH_HITAREA_UNSUPPORTED;
    int32_t power;
    memcpy(&power, p + 0x1668 + side * 4, sizeof(power));
    memcpy(&multiplier, p + 0x1610, sizeof(multiplier));
    fenv_t saved;
    if (feholdexcept(&saved)) return stop(frontier, 0x10a5d4, GH_HITAREA_UNSUPPORTED);
    if (fesetround(FE_TONEAREST)) {
        (void)fesetenv(&saved);
        return stop(frontier, 0x10a5d4, GH_HITAREA_UNSUPPORTED);
    }
    volatile float converted = (float)power;
    volatile float scaled = converted * multiplier;
    if (fesetenv(&saved) || !isfinite(scaled) ||
        scaled < -2147483648.0f || scaled >= 2147483648.0f)
        return stop(frontier, 0x10a5e4, GH_HITAREA_UNSUPPORTED);
    power = (int32_t)scaled;

    const float offset1[3] = {0.0f, 1.3f, 0.2f};
    const float offset2[3] = {0.0f, 0.8f, 0.2f};
    const float base_box[3] = {0.0f, 1.3f, 0.0f};
    const float active_box[3] = {0.0f, shape_class ? 0.5f : 0.9f, 0.7f};
    const float box_size[3] = {
        shape_class ? 1.8f : 0.7f, shape_class ? 1.5f : 1.0f, shape_class ? 1.8f : 1.2f
    };
    for (unsigned i = 0; i < 5; ++i) {
        uint8_t *object = give[order[i]].bytes;
        memcpy(object + 0x44, p + 0x1680 + side * 2, 2);
        memcpy(object + 0x40, p + 0x1678 + side * 4, 4);
    }
    if (gh_collision_pair_shape_offset_002bb258(&shapes[1], offset1, frontier) ||
        gh_collision_pair_shape_offset_002bb258(&shapes[2], offset2, frontier) ||
        gh_collision_pair_shape_offset_002bb258(&shapes[4], base_box, frontier) ||
        gh_collision_pair_shape_offset_002bb258(&shapes[4], active_box, frontier))
        return GH_HITAREA_UNSUPPORTED;

    uint8_t *box = give[4].bytes;
    memcpy(box + 0x46, p + 0x1670 + side * 4, 2);
    gh_hitarea_status status = gh_hitarea_object_set_active_001fd668(&give[4], 1, frontier);
    if (status) return status;
    memcpy(box + 0x4c, &power, sizeof(power));
    memcpy(box + 0x10, rotation + 8, 12);
    /* 1FD820 dispatches to box VT+3C = 2BAAA8 (SetSize). */
    return gh_collision_pair_box_size_002baaa8(&shapes[4],box_size,frontier)?
        GH_HITAREA_UNSUPPORTED:GH_HITAREA_OK;
}
