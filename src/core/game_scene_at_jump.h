#ifndef GH_GAME_SCENE_AT_JUMP_H
#define GH_GAME_SCENE_AT_JUMP_H
#include "game_scene_at_geometry.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2C2B58: first containing kind1/7A1 unit in SAME tagged order,
 * then SAME room JMP[index]. False leaves outputs untouched. Unattached
 *2C1D68 copies original shape; positive object attachment stays strict. */
gh_scene_at_status gh_scene_at_find_jump_002c2b58(gh_scene_at_manager *,const gh_bundle *room,
 uint16_t current_room,const float point[3],float out_position[3],float *out_yaw,
 int *original_return,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
