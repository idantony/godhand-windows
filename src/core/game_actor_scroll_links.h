#ifndef GH_GAME_ACTOR_SCROLL_LINKS_H
#define GH_GAME_ACTOR_SCROLL_LINKS_H
#include "game_scene_tables.h"
#include "game_scene_meshes.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint64_t source_key,current_key;uint32_t scroll_handle;uint8_t resolved;} gh_actor_scroll_entry;
/* Actual global754C38 traversal, first full eight-byte descriptor+8 match.
 * Returns shared typed pool handle; not a guest or truncated host pointer. */
gh_scene_table_status gh_scene_search_scroll_002c5420(const gh_room_pools *,const gh_scene_meshes *,uint64_t,uint32_t *);
/* Same canonical actor owner/link7, represented by typed per-record writes.
 * Immutable original asset bytes retain provenance. Successful source store
 * e[0]=0/e[1]=guestptr becomes current_key=0,scroll_handle=canonical handle.
 * Partial matches survive a later callee failure exactly as original writes.
 * No match leaves name untouched. Original308D40 clear follows the full loop. */
gh_scene_table_status gh_scene_actor_scroll_links_native_00307a60(gh_game_scene_tables *,const gh_room_pools *,const gh_scene_meshes *);
gh_scene_table_status gh_scene_actor_scroll_entry_001333b8(const gh_game_scene_tables *,uint16_t,gh_actor_scroll_entry *);
/* Native sidecar lifecycle; called by actual actor reset before rebinding.
 * Does not release the borrowed meshes, pools, asset, or collision link4. */
void gh_scene_actor_scroll_links_release(gh_game_scene_tables *);
#ifdef __cplusplus
}
#endif
#endif
