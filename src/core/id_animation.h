#ifndef GH_ID_ANIMATION_H
#define GH_ID_ANIMATION_H
#include "id_jobs.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2AC6E0/2AC878 read cumulative[0] from unwritten guest stack memory.
 * This explicit host adaptation supplies it; zero is an intended path-origin
 * reference, NOT evidence that the original PS2 stack contained zero. NULL or
 * has_distance_seed=0 retains that precise unresolved-memory frontier. */
typedef struct gh_id_animation_profile {int has_distance_seed;float distance_seed;} gh_id_animation_profile;
/* gate0 UV,1 position/path,2 scale,3 angle; gate4color stays existingconsumer.
 * Called after source/done/hold gates; repeats their original guards itself.
 * Explicit finite nearest-binary32 reference, saves/restores complete FEnv. */
gh_id_error gh_id_animation_reference(gh_id_base*,gh_id_entry*,unsigned gate,
 const gh_id_animation_profile*,gh_id_frontier*);
#ifdef __cplusplus
}
#endif
#endif
