#ifndef GH_GAME_COLLISION_SCROLL_H
#define GH_GAME_COLLISION_SCROLL_H
#include "game_scene_tables.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Mutable original triangle flags are native ownedbytes. SAMEcanonical actor
 * link4 points to them, so future collisionqueries see writes; rawasset stays
 * immutable. This is not a second collisionmanager or an ignoredshadow. */
typedef struct {gh_game_scene_tables *tables;gh_scene_table_view original;
 uint8_t *owned;const void *source_akg;size_t bytes;} gh_collision_scroll_mutation;
gh_scene_table_status gh_collision_scroll_set_enabled_001332d0(gh_game_scene_tables *,gh_collision_scroll_mutation *,const char *name,int enabled,uint32_t *changed);
gh_scene_table_status gh_collision_scroll_mutation_release(gh_collision_scroll_mutation *);
#ifdef __cplusplus
}
#endif
#endif
