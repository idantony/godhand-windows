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
#ifdef __cplusplus
}
#endif
#endif
