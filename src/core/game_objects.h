#ifndef GH_GAME_OBJECTS_H
#define GH_GAME_OBJECTS_H
#include "game_stage.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_game_objects gh_game_objects;
typedef enum {GH_OBJECT_OK,GH_OBJECT_ARGUMENT,GH_OBJECT_MEMORY,GH_OBJECT_BOUNDS,
 GH_OBJECT_CAPACITY,GH_OBJECT_NOT_READY,GH_OBJECT_MISSING,GH_OBJECT_UNSUPPORTED,GH_OBJECT_DISC,GH_OBJECT_THREAD,GH_OBJECT_BUSY} gh_object_status;
typedef struct {const uint8_t *bytes;size_t size;uint32_t kind,state,flags,file_kind;int32_t use_count;
 size_t file_extent;uint8_t file_extent_known;} gh_object_view;
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
/* Hostprovenance only; never changes guest state. Exactsource pointer must
 * match SAMEholder and extent<=capacity. Pinnedcapacity neednot befilelength. */
gh_object_status gh_game_objects_set_file_extent(gh_game_objects *,uint32_t kind,const void *expected_source,size_t extent);
/* Actual1FEA48->1FF090: descending originalslotlookup, signednonnegative
 * usecountincrement modulo32, ALWAYSstate3; borrowed payload is not modelready.
 * Missing returns MISSING without mutation. Release applies original1FF0B0
 * to the samekind record; no sourcefree/GPU/destructor is fabricated. */
gh_object_status gh_game_objects_acquire_001fea48(gh_game_objects *,uint32_t kind,gh_object_view *);
gh_object_status gh_game_objects_release_reference_001ff0b0(gh_game_objects *,uint32_t kind);
/* Real1FE8A0 call continuation. Negativeempty soundslot(-1) is actual
 * 2CB778 leaftrue. Other slots require real provider; absent => exactfrontier.
 * Provider false causes originalSleep1 then restarts alltenchecks. */
typedef gh_object_status (*gh_object_sound_load_check)(void *,int32_t slot,int *original_ready,uint32_t *frontier);
typedef struct {gh_dvd_jobs *dvd;gh_heap *heap;gh_task_threads *threads;
 gh_object_sound_load_check sound_check;void *sound_context;} gh_object_resource_bindings;
typedef struct {gh_game_objects *manager;gh_object_resource_bindings bindings;uint32_t kind,index,stage;
 uint8_t reference;gh_object_status error;} gh_object_resource_load;
gh_object_status gh_game_objects_load_wait_001fe8a0(gh_object_resource_load *,gh_game_objects *,uint32_t kind,
 const gh_object_resource_bindings *,int use_reference,gh_object_view *,uint32_t *frontier);
/* Originalcase3 initial43 pins of already-loaded source assets, in ELF table
 * order. Caller performs originalroom/flags gating and fiber wait before this
 * primitive; flags400 is required, flags80 set only after concrete bindings. */
gh_object_status gh_game_objects_register_startup(gh_game_objects *,gh_game_assets *);
/* Actual1FE748 descending holder updates, not model readiness/GPU work.
 * Switchdefault state2 leaves real pinned sources untouched. States5/6 and
 * scalar dependency removal are recovered. IOstate1/releasestate7 or an
 * actualresource-free cascade fail preflight atomically atoriginalcallee. */
gh_object_status gh_game_objects_update_001fe748(gh_game_objects *,uint32_t *frontier);
/* Same holder pass with canonical DVD and sound owners for the state 1 check. */
gh_object_status gh_game_objects_update_bound_001fe748(gh_game_objects *,const gh_object_resource_bindings *,uint32_t *frontier);
typedef gh_object_status (*gh_object_slot_release)(void *,int32_t slot,uint32_t *frontier);
/* Original descending1FEF30: negative use-count pins persist. Required sound
 * slot consumer is invoked even for-1. Retry resumes after completed calls. */
gh_object_status gh_game_objects_release_nonpinned_001fef30(gh_game_objects *,gh_object_slot_release,void *,uint32_t *frontier);
/* Actual1FF028 descending reset; only call after release. */
gh_object_status gh_game_objects_reset_records_001ff028(gh_game_objects *,uint32_t *frontier);
/* Narrow canonical holder sound fields for actual1FF4D0/1FF238 callers.
 * These are original52bank IDs, never hostpointers or model readiness. */
gh_object_status gh_game_objects_sound_slots(const gh_game_objects *,uint32_t kind,uint32_t *count,int32_t slots[10]);
gh_object_status gh_game_objects_append_sound_slot(gh_game_objects *,uint32_t kind,int32_t slot);
gh_object_status gh_game_objects_check_ready_001ff238(gh_game_objects *,uint32_t kind,const gh_object_resource_bindings *,int *original_ready,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
