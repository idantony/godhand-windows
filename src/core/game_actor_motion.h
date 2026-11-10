#ifndef GH_GAME_ACTOR_MOTION_H
#define GH_GAME_ACTOR_MOTION_H
#include "game_actor_model.h"
#include "../assets/motion.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_ACTOR_MOTION_OK,GH_ACTOR_MOTION_ARGUMENT,GH_ACTOR_MOTION_BOUNDS,GH_ACTOR_MOTION_BUSY,GH_ACTOR_MOTION_UNSUPPORTED,GH_ACTOR_MOTION_CALLEE} gh_actor_motion_status;
typedef gh_actor_motion_status (*gh_motion_effect_consumer)(void *,const gh_title_player_view *,int32_t kind,uint32_t mode,uint32_t *frontier);
struct gh_motion_ik_globals;
typedef struct {gh_actor_model *model;const uint8_t *resource;size_t resource_bytes;gh_boot_work *work;
 gh_motion_effect_consumer effect;void *context;const struct gh_motion_ik_globals *ik_globals;} gh_actor_motion_bindings;
typedef struct {gh_actor_motion_bindings b;gh_motion_view motion;size_t sequence_offset;
 gh_motion_cache caches[255];uint32_t stage,frontier,detail;size_t release_index;
 uint16_t flags;uint8_t bound,complete,sequence_valid;} gh_actor_motion;
/* Actual2A8578 initialzero-blend/start0 binding. Secondoffset is RAW a2 candidate, not forcedSEQ:
 * sameMTB3 candidate produces absentSEQ through actual2C7F88; SIZE_MAX isNULL.
 * Includes originaloldSEQ release,
 * 158198 SAMEparts bindrestore,1595F8 typedcaches,152448 initialflags.
 * ActiveIK without sharedik_globals remains a precise152448 frontier;
 * this never acknowledges sampledpose/animationstep for that branch.
 * Raw guest428/42C remain zero: this owner supplies their native identities. */
gh_actor_motion_status gh_actor_motion_begin_002a8578(gh_actor_motion *,const gh_actor_motion_bindings *,size_t motion_offset,size_t sequence_offset,uint16_t flags);
#ifdef __cplusplus
}
#endif
#endif
