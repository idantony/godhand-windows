#ifndef GH_GAME_OBJECT_STEP_H
#define GH_GAME_OBJECT_STEP_H
#include "game_init.h"
#include "game_room_pools.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Existing init.side_counter[16] is the canonical original754C38+0..F
 * scalar/opaque span. Registries belong to room_pools, never this companion.
 * This owns only missing754C48 ratio and independent profiling752CF8. */
typedef struct {gh_game_init_state *init;uint32_t ratio10_bits,profile_752cf8;uint8_t constructed;} gh_game_object_step;
typedef enum {GH_OBJSTEP_OK,GH_OBJSTEP_ARGUMENT,GH_OBJSTEP_POOLS,GH_OBJSTEP_UNSUPPORTED} gh_object_step_status;
typedef uint32_t (*gh_object_profile_counter)(void *);
/* Actual2C53A8 constructor precedes roompool construction. Clearsdiagnostic
 * pointertoken/on/side, setsratio1; preservescounter/progress/padding/profile.
 * Must not be reused to reset a live typed registry/list owner. */
gh_object_step_status gh_game_object_step_construct_002c53a8(gh_game_object_step *,gh_game_init_state *);
/* Actualentry2C53C8;2C53E0 is an interiorblock. Modulo32 arithmetic andsigned
 * count/progress comparisons, no traversal of the separate754C38 registry. */
gh_object_step_status gh_game_scroll_counter_move_002c53c8(gh_game_object_step *);
/* Actual30F090 empty/nativegated path. Timerreads alwaysrequired andprofiling
 * remains observable. Positive unskipped objects needtypedactors; precise
 * frontier beforeopaqueguestobjectaccess. No objectmove callback is faked. */
gh_object_step_status gh_game_move_objects_0030f090(gh_game_object_step *,
 const gh_room_pools *,const gh_boot_work *,gh_object_profile_counter,void *,uint32_t *frontier);
/* Actual30F650 registry0 empty earlyreturn, not a graphicsacknowledgment. */
gh_object_step_status gh_game_objects_trans_0030f650(const gh_room_pools *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
