#ifndef GH_GAME_CAMERA_SHAKE_H
#define GH_GAME_CAMERA_SHAKE_H
#include "game_camera.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual138558 parameter progression only: amplitude decay, phase addition
 * and1F7D48 wrap, then byte counter decrement, vertical before horizontal.
 * Does not apply view offset or complete138740 projection. Active shake
 * remains a strict frontier in the existing projector until separately ported.
 * Finite binary32 reference; restores caller FEnv, no libc fmod substitution. */
gh_camera_status gh_game_camera_shake_step_00138558(gh_game_pl_camera *,uint32_t *frontier);
/* Decode actual138740 post-view offsets without mutating camera matrices.
 * Output (horizontal view+170 delta,vertical view+174 delta). Original count
 * AFTER138558 gates each contribution, including genuine finalcount0 skip. */
gh_camera_status gh_game_camera_shake_offsets_00138740(const gh_game_pl_camera *,float out_xy[2],uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
