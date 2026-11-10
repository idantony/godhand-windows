#ifndef GH_GAME_OBJECTS_H
#define GH_GAME_OBJECTS_H
#include "game_stage.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_game_objects gh_game_objects;
typedef enum {GH_OBJECT_OK,GH_OBJECT_ARGUMENT,GH_OBJECT_MEMORY,GH_OBJECT_BOUNDS,
 GH_OBJECT_CAPACITY,GH_OBJECT_NOT_READY,GH_OBJECT_MISSING,GH_OBJECT_UNSUPPORTED} gh_object_status;
typedef struct {const uint8_t *bytes;size_t size;uint32_t kind,state,flags,file_kind;int32_t use_count;} gh_object_view;
/* Actual1FF780(global80)/28FF80(room32) array bindings. Configures the existing
 * canonical stage slot manager; no shadow scalar records. Constructor does not
 * run1FE6C8: originalcase1 must still reset the shared manager. */
gh_object_status gh_game_objects_create(gh_stage_slots *canonical_slots,int room,gh_game_objects **);
void gh_game_objects_destroy(gh_game_objects *);
/* Concrete1FECE0 ->1FEE00/1FEC90/1FEEA0/1FF470. Immutable asset source is
 * borrowed and must outlive registry. State2 is original holder readiness,
 * not model relocation, GPU upload or render completion. */
gh_object_status gh_game_objects_add_pinned_001fece0(gh_game_objects *,uint32_t kind,
 const void *bytes,size_t size,int *original_return);
gh_object_status gh_game_objects_find(const gh_game_objects *,uint32_t kind,gh_object_view *);
/* Originalcase3 initial43 pins of already-loaded source assets, in ELF table
 * order. Caller performs originalroom/flags gating and fiber wait before this
 * primitive; flags400 is required, flags80 set only after concrete bindings. */
gh_object_status gh_game_objects_register_startup(gh_game_objects *,gh_game_assets *);
/* Actual1FE748 descending holder updates, not model readiness/GPU work.
 * Switchdefault state2 leaves real pinned sources untouched. States5/6 and
 * scalar dependency removal are recovered. IOstate1/releasestate7 or an
 * actualresource-free cascade fail preflight atomically atoriginalcallee. */
gh_object_status gh_game_objects_update_001fe748(gh_game_objects *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
