#ifndef GH_GAME_PLAYER_TARGETS_H
#define GH_GAME_PLAYER_TARGETS_H
#include "game_room_pools.h"
#include "game_ray_query.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PLAYER_TARGET_OK,GH_PLAYER_TARGET_ARGUMENT,GH_PLAYER_TARGET_NUMERIC,GH_PLAYER_TARGET_UNSUPPORTED,GH_PLAYER_TARGET_CALLEE} gh_player_target_status;
typedef gh_player_target_status (*gh_player_target_resolve)(void *,uint32_t handle,gh_title_player_view *,uint32_t *frontier);
typedef struct {gh_room_pools *pools;gh_game_scene_tables *actors;const gh_title_player_view *player;
 gh_player_target_resolve resolve;void *context;const void *elf;size_t elf_bytes;
} gh_player_target_bindings;
typedef struct {uint32_t handle;gh_title_player_view view;} gh_player_target;
/* Same754C58 registry, source flags/ID filters BEFORE class-specific fields.
 * Pure target selection; no pickup, prompt, animation or gameplay success. */
gh_player_target_status gh_player_pickup_target_0012a610(const gh_player_target_bindings *,gh_player_target *,uint32_t *frontier);
gh_player_target_status gh_player_interact_target_0012b328(const gh_player_target_bindings *,gh_player_target *,uint32_t *frontier);
gh_player_target_status gh_player_small_object_gate_00129cc8(const gh_player_target_bindings *,uint32_t *frontier);
/* Original2DA038 uses SDK3A1648 atan polynomial (ELF45EA08 tables), NOT
 *1F8180's VU approximation or hostlibc atan. Finite native nearest reference. */
gh_player_target_status gh_player_target_angle_002da038(const void *elf,size_t,const float from[3],const float to[3],float heading,float limit,float *out,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
