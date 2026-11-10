#ifndef GH_GAME_HITAREA_FRAME_H
#define GH_GAME_HITAREA_FRAME_H
#include "game_hitarea_shapes.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Persist SAME cursor on late failure; never copy active cursor or reset its
 * borrowed canonical pool. Success resets cursor for the next original frame. */
typedef struct {const void *cursor_identity;const gh_game_init_state *identity;
 uint32_t generation,phase,slot,node,visited,slot_serial;uint8_t active,slot_started,query_done;} gh_hitarea_frame;
/* Explicit rate is SAME original Work574 semantic owner, not another clock.
 * Binary32 reference preserves FEnv. Positive hit list remains strict1FC810. */
gh_hitarea_status gh_hitarea_frame_resume_001fdfd8(gh_game_init_state *,const gh_boot_work *,float,
 gh_static_transform_policy,gh_hitarea_frame *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
