#ifndef GH_GAME_OM_BASE_H
#define GH_GAME_OM_BASE_H
#include "game_scene_placements.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original1B6E90 sharedbase. Callerfactoryzeroscompleteconcreteallocation;
 * ctor preservesallotherbytes, includingunknown derivedstorage. */
/* Original1C7D98 adds610/630 VF0 and5B0bit8 only. */
gh_scene_placement_status gh_om_pushable_construct_001c7d98(void *,size_t,gh_title_player_view *);
/* Original1BBEB8 doorbase; exact writes through644, preservesunknownbytes. */
gh_scene_placement_status gh_om_door_construct_001bbeb8(void *,size_t,gh_title_player_view *);

gh_scene_placement_status gh_om_base_construct_001b6e90(void *,size_t,gh_title_player_view *);
#ifdef __cplusplus
}
#endif
#endif
