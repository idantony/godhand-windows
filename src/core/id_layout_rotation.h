#ifndef GH_ID_LAYOUT_ROTATION_H
#define GH_ID_LAYOUT_ROTATION_H
#include "id_jobs.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Explicit finite native binary32 nearest reference of actual EE/VU operation
 * order, not PS2 hardware bit-exact math. Saves/restores complete caller FEnv.
 * No libc sin/cos substitution: original VU9th-degree polynomial and SQRT.
 * Scalar CVT.W is truncation as in boot's EE ChopZero setting. Inputs outside
 * the bounded normal finite domain report UNSUPPORTED. */
gh_id_error gh_id_angle_reference_001f7d48(float,float*);
gh_id_error gh_id_sincos_reference(float,float*,float*);
gh_id_error gh_id_layout_rotation_reference(gh_id_base*,gh_id_frontier*);
#ifdef __cplusplus
}
#endif
#endif
