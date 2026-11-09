#ifndef GH_GAME_SCENE_PLACEMENTS_H
#define GH_GAME_SCENE_PLACEMENTS_H
#include "game_room_state.h"
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
/* stop_flag borrows canonical00586AB0 byte previouslyusedby2C33D8. */
typedef struct {uint8_t *stop_flag;uint32_t room04;gh_bundle_entry file,source_entries;
 uint8_t scratch10[0x30];uint32_t frontier;} gh_em_scene_placements;
typedef struct {gh_bundle_entry file,entries;uint32_t frontier;} gh_om_scene_placements;
typedef struct {const uint8_t *records;size_t record_bytes;uint8_t scalar04[0x2c];uint32_t field30,frontier;} gh_acd_scene_binding;
typedef enum {GH_PLACE_OK,GH_PLACE_ARGUMENT,GH_PLACE_BOUNDS,GH_PLACE_UNSUPPORTED} gh_scene_placement_status;
/* Actual294B38/295600; resetsfiles, conditionally48scratchbytes/roomFFFF. */
gh_scene_placement_status gh_em_scene_room_init_00294b38(gh_em_scene_placements *,const gh_boot_work *,uint8_t scenario14);
/* Actual294C68/294B98 emptyEMS path using canonicalroom.active. Positive
 * entries/alreadyloadedsetEmAll remain preciseunsupported, never fakeactors. */
gh_scene_placement_status gh_em_scene_bind_00294c68(gh_em_scene_placements *,gh_room_state *,const gh_bundle_entry *ems,int *original_load_return);
void gh_om_scene_construct_002a7308(gh_om_scene_placements *);
/* Actual1C3CB8/1C3C58 missing/invalid/emptyOMS; originalreturn0 is explicit. */
gh_scene_placement_status gh_om_scene_bind_001c3cb8(gh_om_scene_placements *,const gh_bundle_entry *oms,int *original_return);
/* Actual137AE8/137AA0, scalarheadercopy andboundednativeACDrecordview. */
gh_scene_placement_status gh_acd_scene_bind_00137ae8(gh_acd_scene_binding *,const gh_bundle_entry *acd,int *original_return);
#ifdef __cplusplus
}
#endif
#endif
