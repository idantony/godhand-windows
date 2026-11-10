#ifndef GH_GAME_PLAYER_GROUND_POSE_H
#define GH_GAME_PLAYER_GROUND_POSE_H
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_GROUND_OK,GH_GROUND_ARGUMENT,GH_GROUND_BOUNDS,GH_GROUND_BUSY,GH_GROUND_NUMERIC,GH_GROUND_UNSUPPORTED} gh_player_ground_status;
/* ONE mutable original.data3BD470. Load once at host startup, never per room
 * or player; loaded zero comes from the actual ELF, not a guessed reset. */
typedef struct {float tilt;uint8_t loaded;
 /* Same original.data neighborhood474/478/47C/480. Mutable shared tuning,
  * not four forever-constant copies read afresh by idle/walk callers. */
 float idle_turn_cap,walk_turn_cap,idle_turn_speed,walk_turn_speed;
 float run_turn_speed; /* same original.data484, originalrun target */
 uint8_t tuning_loaded;
} gh_player_ground_globals;
typedef struct {const uint8_t *actor;gh_actor_model *model;gh_player_ground_globals *globals;
 uint8_t stage,active,complete;float target;uint32_t frontier;} gh_player_ground_pose_call;
gh_player_ground_status gh_player_ground_globals_from_elf(gh_player_ground_globals *,const void *,size_t);
/* Inline108AB0 range1095F4..1097B8, after14FB48 and before149EB8.
 * Real258 flags, normalized480XYZ, shared tilt filter and first-part D0.Y.
 * Original zero2B4 skips model/global/normal reads. Finite binary32-nearest
 * reference, preserving caller FEnv. Zero cursor only for a new invocation. */
gh_player_ground_status gh_player_ground_pose_00108ab0(gh_player_ground_pose_call *,const gh_title_player_view *,gh_actor_model *,gh_player_ground_globals *);
#ifdef __cplusplus
}
#endif
#endif
