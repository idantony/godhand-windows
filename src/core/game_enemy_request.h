#ifndef GH_GAME_ENEMY_REQUEST_H
#define GH_GAME_ENEMY_REQUEST_H
#include "game_enemy_actor.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual294CA0 RoomSave24B row->SET_EM_DATA. Skips saved/dead andsignflag
 * rows before conversions. ExplicitPCSX2ChopZero reference; no hostFEnv. */
gh_scene_placement_status gh_enemy_request_from_room_00294ca0(const gh_room_state *,unsigned index,gh_em_numeric_policy,gh_enemy_request *,int *original_should_spawn,int16_t *saved_vitality,uint32_t *frontier);
/* Actual294CA0 after successfulEntryEm: positiveRoomSave.vitality clamps
 * current54A tooriginalmax548; nonpositive leavesinitializedHP unchanged. */
gh_actor_motion_status gh_enemy_apply_saved_vitality_00294f34(gh_game_title_player *,int16_t saved_vitality);
#ifdef __cplusplus
}
#endif
#endif
