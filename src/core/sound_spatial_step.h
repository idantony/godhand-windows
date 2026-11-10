#ifndef GH_SOUND_SPATIAL_STEP_H
#define GH_SOUND_SPATIAL_STEP_H
#include "sound_scene.h"
#include "game_camera.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2CC150 listenerselector andposition/viewmatrix. Ownsno soundstate:
 * writesSAMEcanonicalSoundControl C0/D0/110. PlayerNULL usesoriginalcamera
 * fallbackwithoutchangingthestoredselector. Nativebinary32 policyrequired
 * forplayer-basedmatrixmath; cameracopy itselfisexactbitcopy. */
gh_sound_scene_status gh_sound_listener_update_002cc150(gh_sound_control *,const gh_boot_work *,
 const gh_game_init_state *,const gh_game_pl_camera *,const gh_title_player_view *,gh_camera_numeric_policy,uint32_t *frontier);
/* Actual2CA518: listener→2D22F0 unconditionalmask/ratiohistory→BMHgate→
 * 2CB128TSlistgate→verified2CB200JRRA. Nooriginvoicepumpoccurs here.
 * PresentBMHzones/activeTSentries requiretheirrealconsumers, notemptystubs. */
gh_sound_scene_status gh_sound_spatial_step_002ca518(gh_sound_scene *,const gh_boot_work *,
 const gh_game_init_state *,const gh_game_pl_camera *,const gh_title_player_view *,gh_camera_numeric_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
