#ifndef GH_GAME_CLOTH_COLLISION_H
#define GH_GAME_CLOTH_COLLISION_H
#include "game_cloth_templates.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original107360 flags0 body spheres. IMPORTANT: actual107600 uses the
 * normalized segment direction multiplied by dot(rawDelta)/lengthSquared;
 * this is NOT a conventional closestpoint formula for non-unit segments.
 * Preserve this observed original arithmetic, including zero-delta Y fallback.
 * Unknown capsuleflags!=0 strict107808, both endpoints native-atomic. */
gh_cloth_status gh_cloth_segment_body_00107360(const gh_cloth_geometry *,float first[3],float second[3],gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
