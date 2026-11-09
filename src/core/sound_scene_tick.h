#ifndef GH_SOUND_SCENE_TICK_H
#define GH_SOUND_SCENE_TICK_H
#include "sound_scene.h"
#include "game_children.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2CAF28, beforeBGM2D0938/SEflush. SameRootcSnd scalarowner already
 * initialized fromactualboot BSS/constructors. It runs beforeanyroomready.
 * Work5E4bit08000000 requires actualcanonicalchildren+EE8. Otherwise children
 * may beNULL because originalbranch neverreads it. Orderednativebinary32
 * nearest reference withfullFEnvrestore, not EEhardware roundingproof. */
gh_sound_scene_status gh_sound_scene_tick_002caf28(gh_sound_scene *,const gh_boot_work *,const gh_game_children *);
#ifdef __cplusplus
}
#endif
#endif
