#ifndef GH_GAME_SCENE_VISIBILITY_H
#define GH_GAME_SCENE_VISIBILITY_H
#include "game_scene_post.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_scene_visibility gh_scene_visibility;
/* Borrow the SAME canonical registry actor. No truncated guest pointers. */
typedef gh_scene_post_status (*gh_scene_visibility_resolve)(void *,uint32_t handle,gh_title_player_view *,uint32_t *frontier);
/* Original14B6F8 linked-node380 bit consumer, for canonical actor_model OR
 * static scroll mesh. Native implementation must affect all actual nodes. */
typedef gh_scene_post_status (*gh_scene_visibility_mask_nodes)(void *,uint32_t handle,uint32_t and_mask,uint32_t or_mask,uint32_t *frontier);
typedef struct {gh_scene_post_bindings scene;gh_scene_visibility_resolve resolve_handle;
 gh_scene_visibility_mask_nodes mask_nodes;void *context;} gh_scene_visibility_bindings;
gh_scene_post_status gh_scene_visibility_create(gh_scene_visibility **);
/* Host metadata only; not original visibility reset or model destruction. */
void gh_scene_visibility_destroy(gh_scene_visibility *);
gh_scene_post_status gh_scene_visibility_step_00307b28(gh_scene_visibility *,const gh_scene_visibility_bindings *,uint32_t *frontier);
/* Borrow one original name/visibility field from a canonical object. The
 * kind500 cObjBase fields are resolved in the owner arena, never by extending
 * the 4E0-byte object view. Valid offsets are 538 and 5A0. */
gh_scene_post_status gh_scene_object_visibility_field(const gh_room_pools *,uint32_t handle,
 const gh_title_player_view *,unsigned offset,uint8_t **out);
/* Completed original invocation only; retains the canonical32-slot companion. */
gh_scene_post_status gh_scene_visibility_next_call(gh_scene_visibility *);
#ifdef __cplusplus
}
#endif
#endif
