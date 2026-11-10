#ifndef GH_GAME_PLAYER_OPACITY_H
#define GH_GAME_PLAYER_OPACITY_H
#include "game_camera.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_OPACITY_OK,GH_OPACITY_ARGUMENT,GH_OPACITY_BOUNDS,GH_OPACITY_BUSY,GH_OPACITY_NUMERIC,GH_OPACITY_UNSUPPORTED} gh_player_opacity_status;
/* Mutable original .data3BD490..49F; initialize once, retain across rooms. */
typedef struct {uint8_t bytes[16];uint8_t loaded;} gh_player_opacity_globals;
gh_player_opacity_status gh_player_opacity_globals_from_elf(gh_player_opacity_globals *,const void *,size_t);
typedef struct {const gh_game_init_state *init;const gh_game_pl_camera *camera;
 const gh_player_opacity_globals *globals;const gh_title_player_view *aux_68c;
 uint8_t auxiliary_binding_known;gh_camera_numeric_policy policy;} gh_player_opacity_bindings;
/* One original invocation. Keep across a strict auxiliary-binding frontier;
 * clear only for the next original call. Same parent fields commit once. */
typedef struct {const uint8_t *player_bytes;uint32_t opacity_bits,frontier;
 uint8_t stage,transparent;} gh_player_opacity_call;
/* Actual123160: mode-mask138F88, canonical PlCamera520/528/210, XZ fade,
 * then identical opacity/flag10 on typed68C. Explicit nearest binary32
 * reference, not EE/VU hardware equivalence. Unknown typed68C is strict
 * after the original parent prefix, even if its removed guest word is zero. */
gh_player_opacity_status gh_player_opacity_00123160(gh_player_opacity_call *,
 const gh_title_player_view *,const gh_player_opacity_bindings *);
#ifdef __cplusplus
}
#endif
#endif
