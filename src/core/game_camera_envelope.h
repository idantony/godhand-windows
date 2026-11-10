#ifndef GH_GAME_CAMERA_ENVELOPE_H
#define GH_GAME_CAMERA_ENVELOPE_H
#include "game_camera_follow.h"
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One canonical mutable3BD540 mode plus immutable actualELF tuning tables.
 * Does not own or copy a second camera/player. Initialize ONCE per host;
 * original139DB8 room binding does not reset global3BD540. */
typedef struct {uint32_t mode;float offset[14][3],angle[14][3],distance[14];
 float smoothing,high_offset,low_offset,atan_coeff[8];uint8_t loaded,stage;
 gh_game_pl_camera *pending;const gh_actor_model *player_model;} gh_camera_envelope;
/* Borrow SAME services player_model; no part/model creation or ready ack. */
gh_camera_status gh_game_camera_envelope_bind_model(gh_camera_envelope *,const gh_actor_model *);
gh_camera_status gh_game_camera_envelope_from_elf(gh_camera_envelope *,const void *,size_t,uint32_t *frontier);
/* Continue ONLY after real139FF8 terrain/eye prefix reaches stage6.
 * Supported current ordinary domain: no target640 and same typed model parts (2B4=partNum);
 * flags/state-driven preset blends are real, positive unbound target pointers
 * retain exact frontiers. Dead local envelope-ray scratch has no observable
 * consumer in this domain (no further131E10 calls in original139FF8).
 * Finishes actual139C88 tail138358 aim angles too. Does not project/draw.
 * Successful return resets prefix stage for next original camera update. */
gh_camera_status gh_game_camera_finish_follow_00139c88(gh_camera_envelope *,gh_camera_follow *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif

