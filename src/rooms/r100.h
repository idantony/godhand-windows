#ifndef GH_R100_H
#define GH_R100_H
#include "../core/game_room_state.h"
#include "../core/game_scene_setup.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual pinnedR100 PreInit1A0: three gates and stopEmSet1Time2955F0.
 * Same canonical persistent RoomSave and enemy stop byte, no new room state. */
gh_scene_status gh_r100_preinit_000001a0(const gh_boot_work *,const gh_room_state *,uint8_t *enemy_stop);
/* ActualR100Move740 is jrra/nop, verified in the pinned module. */
gh_scene_status gh_r100_move_00000740(void);
#ifdef __cplusplus
}
#endif
#endif
