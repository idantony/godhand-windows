#ifndef GH_GAME_GATE_H
#define GH_GAME_GATE_H
#include "game_start.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint8_t state;int32_t timer;} gh_game_disc_gate;
typedef struct {int changed,media_ready,queue_idle,identity_matches;} gh_game_disc_inputs;
/* Original201960 recovery: disc-change cover countdown, not DVDjob busy. */
uint8_t gh_game_disc_gate_step(gh_game_disc_gate *,gh_game_disc_inputs);
typedef enum {GH_GATE_COVER_QUAD,GH_GATE_MESSAGE_BEGIN,GH_GATE_MESSAGE_UPDATE,
 GH_GATE_MESSAGE_DRAW,GH_GATE_MESSAGE_FINISH} gh_game_gate_operation;
typedef struct gh_game_gate gh_game_gate;
typedef int (*gh_game_gate_call)(void *,gh_game_gate *,gh_game_gate_operation);
struct gh_game_gate {
 gh_boot_work *work;gh_dvd_jobs *dvd;gh_task_threads *threads;
 const uint8_t *game_state;gh_game_disc_gate disc_gate;
 void *context;gh_game_gate_call call;
 uint32_t failed_entry;gh_game_gate_operation failed_operation;int detail;
};
/* Stable open read-only ISO replaces optical media change/device identity.
 * Actual native read errors fail explicitly; no fabricated device-recovery
 * success. Cover callback required only if real gate.state reaches1.
 * Cover quad: width/height from original video globals, z65535,w1,ctx.mode20,
 * prim30000,color80000000,args1/6/5. Renderer must implement, not ignore. */
gh_game_status gh_game_gate_002a6ca8(gh_game_gate *);
/* Exact Work flags,room,game state,pause/input branch. Message operations:
 * BEGIN13F238(256,220,messageObj+1520,3007,0); UPDATE140E58;
 * DRAW1410A8; FINISH actual message object's word+15BC=FFFF.
 * No-draw branch succeeds without these callbacks because original does none.
 * Pending draw/input branch without an implementation reports exact frontier. */
gh_game_status gh_game_gate_002a6da8(gh_game_gate *);
#ifdef __cplusplus
}
#endif
#endif
