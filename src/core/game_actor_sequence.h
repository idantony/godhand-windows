#ifndef GH_GAME_ACTOR_SEQUENCE_H
#define GH_GAME_ACTOR_SEQUENCE_H
#include "game_actor_motion.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2C7FD0 flags0: originalspeed records and sound event timegates.
 * Sound emission requires sameactor attachedconsumer; Savecostume/floormaps
 * areoriginal. Pendingrecords survive lateerror withoutduplicateemission.
 * Format-one flag and effect records preserve the original timing and slots.
 * Original418 NULL domain; unknownpositiveguestidentity isnot discarded. */
gh_actor_motion_status gh_actor_motion_sequence_002c7fd0(gh_actor_motion *,float canonical_rate,gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
