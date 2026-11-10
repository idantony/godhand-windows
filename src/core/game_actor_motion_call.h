#ifndef GH_GAME_ACTOR_MOTION_CALL_H
#define GH_GAME_ACTOR_MOTION_CALL_H
#include "game_actor_motion.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Explicit ORIGINAL invocation companion, not a second actor/motion owner.
 * open marks a NEW2A8578 call (even identicalparams); step retries THAT call.
 * Calling stepaftercomplete is idempotent; nextoriginalcall mustopen again.
 * Caller exclusivelyborrows SAME motion untilcomplete/failureinspection; do
 * not sampleit during a pending originalcall. Oldresource remainsborrowed
 * until actualoldSEQ/effects release has finished. No cancel-as-success API. */
typedef struct {gh_actor_motion *current;gh_actor_motion_bindings next;
 size_t motion_offset,candidate_offset,index;uint32_t stage,frontier,pending_mode;
 int32_t pending_kind;uint16_t flags;uint8_t active,complete,pending;
 uint32_t start_frame_bits,blend_frames;uint8_t release_previous;} gh_actor_motion_call;
/* Validate unsupported start/flags BEFORE any oldSEQ/effect release. */
gh_actor_motion_status gh_actor_motion_call_open_parameters_002a8578(gh_actor_motion_call *,gh_actor_motion *,const gh_actor_motion_bindings *,size_t motion_offset,size_t raw_a2_candidate_offset,uint32_t start_frame_bits,uint32_t blend_frames,uint16_t flags,int release_previous_sequence);
gh_actor_motion_status gh_actor_motion_call_open_002a8578(gh_actor_motion_call *,gh_actor_motion *current,const gh_actor_motion_bindings *,size_t motion_offset,size_t raw_a2_candidate_offset,uint16_t flags);
gh_actor_motion_status gh_actor_motion_call_step_002a8578(gh_actor_motion_call *);
#ifdef __cplusplus
}
#endif
#endif
