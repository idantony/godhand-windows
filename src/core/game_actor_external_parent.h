#ifndef GH_GAME_ACTOR_EXTERNAL_PARENT_H
#define GH_GAME_ACTOR_EXTERNAL_PARENT_H
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual part+148 replacement, currently the child0 binding made by100ED8.
 * Parent/child stay canonical, parent cannot release while borrowed. */
gh_actor_model_status gh_actor_model_bind_external_parent(gh_actor_model *child,unsigned child_index,gh_actor_model *parent,unsigned parent_index,const void *owner);
gh_actor_model_status gh_actor_model_clear_external_parent(gh_actor_model *child,const void *owner);
gh_actor_model_status gh_actor_model_external_parent_at(const gh_actor_model *child,unsigned child_index,const uint8_t **parent_bytes);
typedef struct {uint8_t child, parent;} gh_actor_external_part_pair;
typedef struct gh_actor_external_parent_set {
    gh_actor_model *child, *parent;
    gh_actor_external_part_pair pairs[7];
    uint32_t child_handles[7], parent_handles[7];
    uint8_t count;
} gh_actor_external_parent_set;
/* REL attachment edges borrowing one existing model; each slot corresponds
 * to a real cParts+148 parent write. A set owns one parent lifetime borrow. */
gh_actor_model_status gh_actor_model_bind_external_parts(gh_actor_external_parent_set *,
    gh_actor_model *child, gh_actor_model *parent, const gh_actor_external_part_pair *, unsigned count);
gh_actor_model_status gh_actor_model_clear_external_parts(gh_actor_external_parent_set *);
gh_actor_model_status gh_actor_model_external_parts_at(const gh_actor_model *child, unsigned child_index,
    const uint8_t **parent_bytes);
#ifdef __cplusplus
}
#endif
#endif
