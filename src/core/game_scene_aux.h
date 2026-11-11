#ifndef GH_GAME_SCENE_AUX_H
#define GH_GAME_SCENE_AUX_H
#include "boot.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_AUX_OK,GH_AUX_ARGUMENT,GH_AUX_UNSUPPORTED} gh_scene_aux_status;
typedef uint32_t (*gh_scene_aux_clock)(void *);
typedef gh_scene_aux_status (*gh_scene_start_load)(void *);
typedef struct {
 uint32_t effect_profile_752d00,effect_count_3c3bb8,frontier;
 uint8_t effect_stage,door_stage,load_flags_saved;
 uint8_t save_load[0x80]; /* Original cSaveLoad at 005F38F0, from *003C2648. */
} gh_game_scene_aux;
/* Native provenance: actual game-scene-aux-ghidra.json. Known diagnostic
 * profiling globals, not a shadow particle/door owner. Canonical wait is lent
 * by services. Unrecovered active consumers retain their original frontier. */
gh_scene_aux_status gh_scene_door_step_001ffe10(gh_game_scene_aux *,uint16_t *canonical_wait,const gh_boot_work *);
void gh_scene_background_mode_0030e5c0(gh_boot_work *,int mode,uint32_t a,uint32_t b,uint32_t c);
gh_scene_aux_status gh_scene_cursor_open_load_002bf268(gh_game_scene_aux *,gh_boot_work *);
gh_scene_aux_status gh_scene_cursor_step_002bf2d0(gh_game_scene_aux *,gh_boot_work *,gh_scene_start_load,void *);
gh_scene_aux_status gh_scene_effects_step_0030f228(gh_game_scene_aux *,const gh_boot_work *,gh_scene_aux_clock,void *);
#ifdef __cplusplus
}
#endif
#endif
