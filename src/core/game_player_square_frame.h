#ifndef GH_GAME_PLAYER_SQUARE_FRAME_H
#define GH_GAME_PLAYER_SQUARE_FRAME_H
#include "game_player_square_attack.h"
#include "game_player_targets.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef gh_player_frame_status (*gh_square_frame_hit_area)(
 void *,const gh_title_player_view *,gh_hitarea_object[5],uint32_t *);
typedef gh_player_frame_status (*gh_square_frame_input)(
 void *,const gh_player_frame_bindings *,int *,uint32_t *);
typedef gh_player_frame_status (*gh_square_frame_camera)(
 void *,const gh_player_frame_bindings *,uint32_t *);

typedef struct {
 const gh_player_frame_bindings *frame;
 const gh_player_target_bindings *target_bindings;
 gh_player_target *target_640;
 gh_square_frame_hit_area hit_area_0010a438;
 gh_square_frame_input input_mode1_00123938;
 gh_square_frame_camera camera_exit_00124f60;
 void *context;
} gh_player_square_frame_bindings;

typedef struct {
 const gh_player_square_frame_bindings *bindings;
 gh_player_square_attack begin;
 gh_player_motion_request motion;
 uint32_t frontier;
 int ended,input_result;
 uint8_t stage,turn_pending,camera_pending;
} gh_player_square_frame;

/* Each real call starts with a zero cursor, or after a prior complete call
 * with reset. Canonical target +640 and motion are borrowed throughout. */
gh_player_frame_status gh_game_player_square_frame_0010d3a8(
 gh_player_square_frame *,const gh_player_square_frame_bindings *);
gh_player_frame_status gh_player_square_frame_reset(gh_player_square_frame *);
#ifdef __cplusplus
}
#endif
#endif
