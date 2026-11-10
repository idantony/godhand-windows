#ifndef GH_GAME_EE_ACOS_H
#define GH_GAME_EE_ACOS_H
#include "game_camera.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual1F8140 COP1 MUL.S/SUB.S/SQRT.S then1F8180. Explicit nearest
 * binary32 numeric reference plus pinned PCSX2 EE SQRT negative-operand
 * magnitude semantics. No clamp of input or radicand; not libc acos.
 * Not a model of complete FCR31 flags or PS2 hardware bit identity.
 * Finite normal-or-zero input and finite product only; output atomic,
 * caller host rounding and exceptions preserved. */
gh_camera_status gh_game_ee_acos_reference_001f8140(float,float *,gh_camera_numeric_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
