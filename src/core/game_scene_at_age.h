#ifndef GH_GAME_SCENE_AT_AGE_H
#define GH_GAME_SCENE_AT_AGE_H
#include "scene_at_units.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Cursor for ONE original2C1540 call, borrowing the canonical tagged lists.
 * A failed next-link traversal must not age completed units a second time. */
typedef struct {gh_scene_at_manager *manager;const void *native_owner;
 gh_at_native_unit current;size_t limit,completed;uint8_t active,stage;
} gh_scene_at_age;
gh_scene_at_status gh_scene_at_age_002c1540(gh_scene_at_age *,gh_scene_at_manager *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
