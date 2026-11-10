#ifndef GH_GAME_CLOTH_TEMPLATES_H
#define GH_GAME_CLOTH_TEMPLATES_H
#include <stddef.h>
#include <stdint.h>
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original 0x30-byte record; no guest pointers or native ABI longs. */
typedef struct {uint16_t flags;uint8_t bone_a,bone_b;float blend,radius;uint32_t reserved;float offset_a[4],offset_b[4];} gh_cloth_sphere;
typedef enum {GH_CLOTH_OK,GH_CLOTH_ARGUMENT,GH_CLOTH_BOUNDS,GH_CLOTH_UNSUPPORTED,GH_CLOTH_NUMERIC} gh_cloth_status;
/* ONE persistent mutable original BSS range460E00..4615DF, across rooms. */
typedef struct {uint8_t records[12][0xc0];uint32_t count;} gh_cloth_geometry;
typedef struct {gh_cloth_sphere records[42];gh_cloth_geometry geometry;uint8_t loaded,initialized;} gh_cloth_templates;
gh_cloth_status gh_cloth_templates_from_elf(gh_cloth_templates *,const void *,size_t);
/* Original guard: priorityFFFF AND init!=0. Does not silently suppress a real
 * second constructor invocation: enabled calls rewrite the same42records. */
gh_cloth_status gh_cloth_templates_initialize_00100fb8(gh_cloth_templates *,int init,int priority);
gh_cloth_status gh_cloth_templates_for_costume(gh_cloth_templates *,unsigned costume,gh_cloth_sphere **,size_t *count);
/* Original461600 output scratch is shared by all calls, not per child. The
 * returned view is invalidated by the next successful prepare. Native replacement
 * retains untouched original0xC0-record bytes; originalpointerword replaced by
 * this typed borrowed result. flags==1 capsule branch remains strict106468. */
gh_cloth_status gh_cloth_prepare_00106218(gh_cloth_templates *,const gh_actor_model *,
 const gh_cloth_sphere *,size_t,gh_static_transform_policy,const gh_cloth_geometry **original_result,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
