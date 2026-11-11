#ifndef GH_GAME_DOOR_H
#define GH_GAME_DOOR_H
#include "game_start.h"
#include "game_scene_ui.h"
#include "game_scene_step.h"
#include "display_state.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One scalar owner for original Door00583EC0 +00..3B. No guest pointers. */
typedef struct {
 uint16_t wait,unknown02;
 uint32_t position_bits[3],yaw_bits;int16_t target_room;
 uint8_t point,kind;uint32_t flags;uint8_t backup[24];uint32_t unknown34;
 uint8_t phase38,verify_phase39,force3a,unknown3b;
 uint8_t initialized,step_stage,begin_stage;uint32_t frontier;
} gh_game_door;
typedef enum {GH_DOOR_OK,GH_DOOR_ARGUMENT,GH_DOOR_BOUNDS,GH_DOOR_FORMAT,
 GH_DOOR_BUSY,GH_DOOR_UNSUPPORTED,GH_DOOR_CALLEE} gh_door_status;
/* Required original operations beyond the verified R020->newroom branch.
 * No callback is needed or invoked on a genuine original branch which skips it. */
typedef gh_door_status (*gh_door_external)(void*,uint32_t address,uint32_t *frontier);
typedef struct {
 gh_boot_work *work;gh_game_loop *loop;gh_scene_scenario_view scenario;
 gh_game_scene_ui *messages;gh_game_prepare *prepare;
 gh_game_title_player *player; /*borrowed, required only on original active-player branches*/
 gh_game_scene_step *scene_step; /*canonical Main+1B0 cooldown*/
 gh_screen_fade *fade;gh_display_state *display;
 gh_door_external external;void *context;
} gh_game_door_bindings;
/* Zero-initialize native owner first. These load the exact cold ELF file/BSS
 * scalar values; they do not claim the whole original static constructor chain. */
gh_door_status gh_game_door_from_elf(gh_game_door*,const uint8_t*,size_t);
gh_door_status gh_game_door_from_disc(gh_game_door*,gh_disc*);
/* Actual scene reset writes only Door+00, preserving every other field. */
gh_door_status gh_game_door_reset_001ffe08(gh_game_door*);
/* Actual2C4548 wrapper ->1FFEB8. Request fields are stored even when an existing
 * transition flag makes1FFF80 return early, as in the original. */
gh_door_status gh_game_door_request_001ffeb8(gh_game_door*,const gh_game_door_bindings*,
 uint16_t room,const float position[3],float yaw,uint8_t kind,uint32_t flags,uint8_t force);
/* Direct original 1FFF30 uses the supplied destination room verbatim. */
gh_door_status gh_game_door_request_001fff30(gh_game_door *,const gh_game_door_bindings *,
 uint16_t room,const uint32_t position_bits[3],uint32_t yaw_bits,uint8_t kind,uint32_t flags,uint8_t force);
/* Original per-frame1FFE10: wait decrement, Fade readiness,2000F8. No hidden
 * Fade stepping, no forced frame waits, and no room asset loading shortcut.
 * R020 kind1/force1 changes canonical Main state to5 and Work's next-room data;
 * later original Main state5 still must execute its real teardown/load chain. */
gh_door_status gh_game_door_step_001ffe10(gh_game_door*,const gh_game_door_bindings*);
#ifdef __cplusplus
}
#endif
#endif
