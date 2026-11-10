#ifndef GH_GAME_COLLISION_SOLID_STEP_H
#define GH_GAME_COLLISION_SOLID_STEP_H
#include "game_collision_solid_units.h"
#include "../assets/static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {const void *identity;gh_game_collision_solids *manager;
 const uint8_t *actor_identity;gh_collision_solid_unit *source,*other;
 gh_collision_native_sphere *source_sphere,*other_sphere;
 float minimum[3],maximum[3];uint32_t phase,source_visited,other_visited,sphere_visited;uint8_t active,hit;} gh_collision_solid_frame;
/* Same original1348E8 invocation. Lateerrors retain SAMEcursor; no repeated
 * clear08/sphereupdate/push onretry. Success resetscursor for nextinvocation.
 * Explicitnativebinary32 reference, not EE/VU hardwarebitexact claim. */
gh_solid_unit_status gh_collision_solid_step_resume_001348e8(gh_game_collision_solids *,const gh_title_player_view *,
 gh_static_transform_policy,gh_collision_solid_frame *,int *original_hit,uint32_t *frontier);
/* count/duplicates are originalfield88. Releasedunit borrowedview is detached
 * withoutcompressing the count; such entry returnsUNSUPPORTED, not dangling. */
gh_solid_unit_status gh_collision_solid_contact_at(const gh_game_collision_solids *,unsigned,const gh_title_player_view **,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
