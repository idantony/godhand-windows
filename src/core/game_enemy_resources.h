#ifndef GH_GAME_ENEMY_RESOURCES_H
#define GH_GAME_ENEMY_RESOURCES_H
#include "game_scene_placements.h"
#include "game_object_sounds.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_em_scene_placements *em;gh_room_state *room;gh_game_objects *room_objects;const gh_boot_work *work;const uint8_t *scenario14;
 gh_enemy_se_banks *banks;gh_object_resource_bindings resources;} gh_em_resource_bindings;
/* Caller programcounter only; all page/model/sound state remains canonical. */
typedef struct {const gh_em_resource_bindings *bindings;const uint8_t *page;uint32_t stage,cursor,entry_stage,live_count,aux_kind,spawn_cursor,frontier;int detail;
 gh_object_resource_load load;gh_object_sound_load sound;gh_object_view view;uint8_t prefix_complete;} gh_em_resource_prefix;
/* Actual294CA0 stop gate/resource loop/295860 inactive-scratch domain/294F70
 * preload. R100 default branch performs realEM202/2A2/220 loads and sounds.
 * First positive enemyfactory2905D0 remains preciseUNSUPPORTED; no actor ack.
 * If nativewhole294C68 already returnedOK after stop1clear, do NOT call again:
 * route here only at its294CA0 frontier, or call afterloadprimitive294B98. */
gh_scene_placement_status gh_em_resources_00294ca0(gh_em_resource_prefix *,const gh_em_resource_bindings *);
/* Pure original294F70 room/cachedkind selector. Room>=500 randomfallback
 * remains31CEF8; does not consume a shadow RNG or choose a default. */
gh_scene_placement_status gh_em_auxiliary_kind_00294f70(uint16_t room,uint32_t cached_kind,uint32_t *kind,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
