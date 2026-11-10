#ifndef GH_GAME_SYSTEM_MENU_H
#define GH_GAME_SYSTEM_MENU_H
#include "game_cockpit.h"
#include "id_animation.h"
#include "../platform/disc.h"
#include "../platform/task_threads.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_SYSTEM_MENU_OK,GH_SYSTEM_MENU_ARGUMENT,GH_SYSTEM_MENU_BOUNDS,
 GH_SYSTEM_MENU_BUSY,GH_SYSTEM_MENU_THREAD,GH_SYSTEM_MENU_FULL,GH_SYSTEM_MENU_UNSUPPORTED,
 GH_SYSTEM_MENU_ID} gh_system_menu_status;
typedef struct {
 gh_game_cockpit *cockpit;gh_id_runtime *runtime;gh_game_mt *random;
 gh_task_threads *global_threads;uint8_t *enabled_3c2555;
 gh_cockpit_frame_callbacks frame;
 gh_id_animation_profile animation_profile;int binary32_reference;
} gh_system_menu_bindings;
/* Canonical previously-unrepresented3C2554 byte plus native continuation.
 * Borrowed bindings/owners must outlive the real registered fiber task. */
typedef struct {
 uint8_t active_3c2554,initialized,task_live;int32_t task_slot;
 gh_system_menu_bindings b;gh_cockpit_system_menu_view widget;
 gh_system_menu_status error;uint32_t frontier;uint64_t iterations;
} gh_game_system_menu;
gh_system_menu_status gh_system_menu_from_disc(gh_game_system_menu*,gh_disc*);
/* Actual2B65B0 ->GlobalTask register2B6600. Does not execute the new task early,
 * acknowledge a UI draw, or replace the original task traversal order. */
gh_system_menu_status gh_system_menu_start_002b65b0(gh_game_system_menu*,const gh_system_menu_bindings*);
/* Concrete original SystemMenu widget methods, always the SAME Cockpit ID2. */
gh_system_menu_status gh_system_menu_initialize_0013ed28(gh_game_system_menu*);
gh_system_menu_status gh_system_menu_move_0013ee58(gh_game_system_menu*);
gh_system_menu_status gh_system_menu_trans_0013f070(gh_game_system_menu*);
/* Native abort only; no invented original flag writes or wholeCockpit teardown. */
gh_system_menu_status gh_system_menu_cancel(gh_game_system_menu*);
#ifdef __cplusplus
}
#endif
#endif
