#ifndef GH_GAME_ACTOR_MOTION_H
#define GH_GAME_ACTOR_MOTION_H
#include "game_actor_model.h"
#include "../assets/motion.h"
#include "game_init.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_ACTOR_MOTION_OK,GH_ACTOR_MOTION_ARGUMENT,GH_ACTOR_MOTION_BOUNDS,GH_ACTOR_MOTION_BUSY,GH_ACTOR_MOTION_UNSUPPORTED,GH_ACTOR_MOTION_CALLEE} gh_actor_motion_status;
typedef gh_actor_motion_status (*gh_motion_effect_consumer)(void *,const gh_title_player_view *,int32_t kind,uint32_t mode,uint32_t *frontier);
struct gh_motion_ik_globals;
typedef struct {gh_actor_model *model;const gh_title_player_view *actor,*other_actor;const gh_actor_part *part;int16_t bank,program;uint32_t argument7,argument8;} gh_motion_sound_request;
/* Actual2CBA48 actor-attached path; Originalhandle0(poolfull) isdistinct from
 * unsupported. Provider must reallyown/queue the voice; originisnotasubstitute. */
typedef gh_actor_motion_status (*gh_motion_sound_consumer)(void *,const gh_motion_sound_request *,uint32_t *original_handle,uint32_t *frontier);
typedef struct {gh_actor_model *model;const uint8_t *resource;size_t resource_bytes;gh_boot_work *work;
 gh_motion_effect_consumer effect;void *context;const struct gh_motion_ik_globals *ik_globals;
 gh_core_save_view save;uint8_t save_binding_known;gh_motion_sound_consumer sound;void *sound_context;} gh_actor_motion_bindings;
typedef struct {gh_actor_motion_bindings b;gh_motion_view motion;size_t sequence_offset;
 gh_motion_cache caches[255];uint32_t stage,frontier,detail;size_t release_index;
 uint16_t flags;uint8_t bound,complete,sequence_valid;
 uint32_t start_frame_bits,blend_frames,step_rate_bits;float step_dt,step_ratio;uint8_t step_stage;
 uint32_t sequence_stage,sequence_index,sequence_rate_bits;float sequence_time,sequence_window;gh_motion_sound_request pending_sound;uint16_t sequence_variations;uint8_t sequence_pending,sequence_source_flags,sequence_costume_override,sequence_effect_gate;} gh_actor_motion;
/* Actual2A8578 initialzero-blend/start0 binding. Secondoffset is RAW a2 candidate, not forcedSEQ:
 * sameMTB3 candidate produces absentSEQ through actual2C7F88; SIZE_MAX isNULL.
 * Includes originaloldSEQ release,
 * 158198 SAMEparts bindrestore,1595F8 typedcaches,152448 initialflags.
 * ActiveIK without sharedik_globals remains a precise152448 frontier;
 * this never acknowledges sampledpose/animationstep for that branch.
 * Raw guest428/42C remain zero: this owner supplies their native identities. */
/* start0,flags0 native domain; positiveblend uses original SAMEpart+40
 * capturedlocalmatrices and438/43C counters, sameIKglobals binding. */
gh_actor_motion_status gh_actor_motion_begin_parameters_002a8578(gh_actor_motion *,const gh_actor_motion_bindings *,size_t motion_offset,size_t raw_a2_candidate_offset,uint32_t start_frame_bits,uint32_t blend_frames,uint16_t flags);
/* Direct 157568 initial call: f12=1, f13=0, a3/t0=0. Start with a fresh
 * owner and retry through this same entry. No old-SEQ/effect release. */
gh_actor_motion_status gh_actor_motion_begin_initial_00157568(gh_actor_motion *,
 const gh_actor_motion_bindings *,size_t motion_offset,size_t raw_a2_candidate_offset);
gh_actor_motion_status gh_actor_motion_begin_002a8578(gh_actor_motion *,const gh_actor_motion_bindings *,size_t motion_offset,size_t sequence_offset,uint16_t flags);
#ifdef __cplusplus
}
#endif
#endif
