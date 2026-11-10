#ifndef GH_GAME_HITAREA_SHAPES_H
#define GH_GAME_HITAREA_SHAPES_H
#include "game_collision_pair_shapes.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_collision_pairs *pairs;const gh_game_math_globals *math;gh_static_transform_policy policy;} gh_hitarea_shape_bindings;
/* Actual1FDAF0/1FDC00 node allocation from SAMErecords_a160/records_b512,
 * nativepairshape constructor +attribute1 +originalnode/Unit listlink.
 * Originalnegative/full/fallback returns-1 withoutfakinganactive shape. */
gh_hitarea_status gh_hitarea_add_sphere_001fdaf0(const gh_hitarea_object *,const gh_hitarea_shape_bindings *,const uint8_t matrix[64],const uint32_t offset_xyz[3],uint32_t radius_bits,int *original_index,uint32_t *frontier);
/* Kind2 originalf12=height(+110), f13=radius(+114), order unchanged. */
gh_hitarea_status gh_hitarea_add_cylinder_001fdc00(const gh_hitarea_object *,const gh_hitarea_shape_bindings *,const uint8_t matrix[64],const uint32_t offset_xyz[3],const uint32_t rotation_xyz[3],uint32_t height_f12_bits,uint32_t radius_f13_bits,int *original_index,uint32_t *frontier);
gh_hitarea_status gh_hitarea_add_box_001fd9d8(const gh_hitarea_object *,const gh_hitarea_shape_bindings *,const uint8_t matrix[64],const uint32_t offset_xyz[3],const uint32_t rotation_xyz[3],const uint32_t full_size_xyz[3],int *original_index,uint32_t *frontier);
gh_hitarea_status gh_hitarea_object_set_active_001fd668(const gh_hitarea_object *,int enabled,uint32_t *frontier);
gh_hitarea_status gh_hitarea_node_at(const gh_hitarea_object *,unsigned ordinal,const uint8_t **record64,gh_collision_pair_shape *);
/* Actual1FD580: ordered1FCBA0 pair removal, linked-node unlink and
 * 1FC7F8 exact reset. Mode2 leaves deferred pair record flags intact. */
gh_hitarea_status gh_hitarea_object_clear_shapes_001fd580(gh_hitarea_object *,int *original_return,uint32_t *frontier);
/* Same canonical pool slot view, used by original1FDFA8 ordered release. */
gh_hitarea_status gh_hitarea_pool_slot_clear_001fd580(gh_game_init_state *,unsigned slot,uint32_t *frontier);
/* Nativeabort/shutdown only. Detachesmetadata/typedlinks before freeing its
 * pair associations; original1FD580 runtime callback stillnotclaimed. */
gh_hitarea_status gh_hitarea_shapes_abort_object(gh_hitarea_object *,uint32_t *frontier);
gh_hitarea_status gh_hitarea_shapes_abort(gh_game_init_state *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
