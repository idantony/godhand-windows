#ifndef GH_GAME_CHILDREN_H
#define GH_GAME_CHILDREN_H
#include <stddef.h>
#include <stdint.h>
#include "id_base.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_game_children gh_game_children;
typedef enum {GH_CHILD_OK,GH_CHILD_ARGUMENT,GH_CHILD_MEMORY,GH_CHILD_BOUNDS} gh_child_status;
typedef struct gh_game_child_view {
 const uint8_t *bytes;size_t size; /*boundedspan, not a sizeofclass claim*/uint32_t original_offset,vtable_address,init_entry,kind;
 int initialized;gh_id_base *native_base;
} gh_game_child_view;
/* Actual1612C8 construction with native handles for pointer fields. Vtable
 * addresses in views are provenance IDs, never callable host addresses.
 * room_jump_at_construction replaces original global747A44 copied to+868;
 * normal cold construction precedes loading and suppliesNULL. */
gh_child_status gh_game_children_create(const void *room_jump_at_construction,gh_game_children **);
const void *gh_game_children_room_jump(const gh_game_children *);
void gh_game_children_destroy(gh_game_children *);
/* Actual15FD98: resetpairarray,insert9children in original order,invoke their
 * proven concrete initializer bodies,thenfixed footer reset. No UI/scene calls
 * are replaced with no-ops; the original nine methods only reset fields. */
gh_child_status gh_game_children_initialize_0015fd98(gh_game_children *);
/* Actual15FEE0: nine descendingfooterwordsEE8..F08=-1 andbyteF15=0.
 * No virtualwidgetdispatch, IDallocation or objectreconstruction occurs. */
gh_child_status gh_game_children_room_reset_0015fee0(gh_game_children *);
gh_child_status gh_game_child_at(const gh_game_children *,uint32_t registry_index,gh_game_child_view *);
/* NativeIDBase views reuse the already-recovered IDruntime implementation.
 * This module initializes but neverallocates IDentryruns; later consumers must
 * release their runs through IDruntime before destroying owner/runtime. */
gh_child_status gh_game_children_id_base(gh_game_children *,uint32_t original_offset,gh_id_base **);
gh_child_status gh_game_children_manager_view(const gh_game_children *,const uint8_t **,size_t *);
#ifdef __cplusplus
}
#endif
#endif
