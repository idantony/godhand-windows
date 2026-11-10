#ifndef GH_GAME_COLLISION_PAIR_SHAPES_H
#define GH_GAME_COLLISION_PAIR_SHAPES_H
#include "game_collision_pairs.h"
#include "game_hitarea_objects.h"
#include "game_math_globals.h"
#include "game_actor_matrix.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PAIR_SHAPE_OK,GH_PAIR_SHAPE_ARGUMENT,GH_PAIR_SHAPE_MEMORY,GH_PAIR_SHAPE_UNSUPPORTED,GH_PAIR_SHAPE_BUSY} gh_pair_shape_status;
typedef struct {gh_collision_pairs *manager;uint8_t *record;uint32_t index,serial;} gh_collision_pair_shape;
typedef struct {const uint8_t *matrix;uint32_t offset_xyz[3],rotation_xyz[3];
 const gh_game_math_globals *math;gh_hitarea_object owner;uint32_t type,radius_bits,height_bits,size_xyz[3];
 uint8_t global_identity_matrix;gh_static_transform_policy policy;} gh_collision_shape_inputs;
/* Actual2B9DF0 sphere/2B9EF0 cylinder/2BA020 box. Same400*270 recordstorage, typedlist,
 * typedmatrix/current-positionaliases andattribute1 DamageUnit identity.
 * Explicit sharedSDK147728 reference for boundedEuler[-pi,pi]; no libc trig.
 * Does not advance shape/world update or claim pairquery/ChkHit. */
gh_pair_shape_status gh_collision_pair_sphere_002b9df0(gh_collision_pairs *,const gh_collision_shape_inputs *,gh_collision_pair_shape *,int *original_nonnull,uint32_t *frontier);
gh_pair_shape_status gh_collision_pair_cylinder_002b9ef0(gh_collision_pairs *,const gh_collision_shape_inputs *,gh_collision_pair_shape *,int *original_nonnull,uint32_t *frontier);
gh_pair_shape_status gh_collision_pair_box_002ba020(gh_collision_pairs *,const gh_collision_shape_inputs *,gh_collision_pair_shape *,int *original_nonnull,uint32_t *frontier);
int gh_collision_pair_shapes_have_live(const gh_collision_pairs *);
gh_pair_shape_status gh_collision_pair_shape_source_matrix(const gh_collision_pair_shape *,const uint8_t **);
gh_pair_shape_status gh_collision_pair_shape_position(const gh_collision_pair_shape *,const uint8_t **);
gh_pair_shape_status gh_collision_pair_shape_mode_pointer(const gh_collision_pair_shape *,uint32_t **);
/* Nativefailed-construction/shutdown cleanup, not originalruntime destructor.
 * Refuses a live canonicalobject_links targetborrow; unlinknodefirst. */
gh_pair_shape_status gh_collision_pair_shape_abort(gh_collision_pair_shape *,uint32_t *frontier);
gh_pair_shape_status gh_collision_pair_shapes_release_native(gh_collision_pairs *);
#ifdef __cplusplus
}
#endif
#endif
