#ifndef GH_GAME_ACTOR_MOTION_STEP_H
#define GH_GAME_ACTOR_MOTION_STEP_H
#include "game_actor_motion.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual158328 sampled local channels into SAMEparts/rootmotion fields.
 * Current realflags0/unmirrored profile; no hierarchy/IKsolve acknowledgment. */
gh_actor_motion_status gh_actor_motion_sample_00158328(gh_actor_motion *,gh_static_transform_policy,uint32_t *frontier);
/* Actual157C18 initialno-blend profile, including realemptySEQ processing,
 * rootmotion deltas, originalSDK rotation, counter/end/loop progression.
 * canonical_rate is borrowed Root rate (original747A14), NOT Work padding.
 * phase569==1 applies originalhalf rate. ended is original functionreturn. */
gh_actor_motion_status gh_actor_motion_step_00157c18(gh_actor_motion *,float canonical_rate,gh_static_transform_policy,int *ended,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
