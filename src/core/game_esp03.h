#ifndef GH_GAME_ESP03_H
#define GH_GAME_ESP03_H
#include "game_room_pools.h"
#include "game_effects.h"
#include "game_actor_model.h"
#include "game_effects_trans.h"
#ifdef __cplusplus
extern "C" {
#endif

enum {
    GH_ESP03_VTABLE = 0x44e300,
    GH_ESP03_ARM_POINTS = 18,
    GH_ESP03_ARM_COUNT = 2,
    GH_ESP03_SHELL_COUNT = 3,
    GH_ESP03_SHELL_ROWS = 6,
    GH_ESP03_SHELL_COLUMNS = 6,
    GH_ESP03_SHELL_CELLS = GH_ESP03_SHELL_ROWS * GH_ESP03_SHELL_COLUMNS,
    GH_ESP03_SHELL_LOCAL_POINTS = GH_ESP03_SHELL_COUNT * GH_ESP03_SHELL_CELLS * 4,
    GH_ESP03_SHELL_WORLD_POINTS = GH_ESP03_SHELL_COUNT * GH_ESP03_SHELL_CELLS * 6
};
typedef float gh_esp03_point[4];

/* Native arena handles retain the original allocations without truncating pointers
 * into the PS2 ESP layout. The two parity buffers belong to the same instance. */
typedef struct gh_esp03_active_state {
    gh_room_pools *pools;
    gh_room_arena_allocation packet[2], arm_points, local_rings, local_shells, world_shells;
    uint8_t moved;
} gh_esp03_active_state;

gh_effect_status gh_esp03_initialize_002e35c0(
    gh_esp03_active_state *, gh_room_pools *, uint32_t work594, uint8_t *esp,
    const uint8_t *est, uint32_t *frontier);
gh_effect_status gh_esp03_destroy_002e3500(gh_esp03_active_state *);
gh_effect_status gh_esp03_move_002e4280(gh_esp03_active_state *,const gh_actor_model *,
    gh_static_transform_policy,uint32_t *frontier);
gh_effect_status gh_esp03_draw_002e4e70(const gh_esp03_active_state *,
    const gh_effects_trans_bindings *,unsigned,const gh_effect_esp_view *,
    gh_effect_trans_packet *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
