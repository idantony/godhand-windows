#ifndef GH_GAME_SCENE_PLACEMENTS_H
#define GH_GAME_SCENE_PLACEMENTS_H
#include "game_room_state.h"
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
/* stop_flag borrows canonical00586AB0 byte previouslyusedby2C33D8. */
typedef struct {uint8_t *stop_flag;uint32_t room04;gh_bundle_entry file,source_entries;
 uint8_t scratch10[0x30];uint32_t frontier;} gh_em_scene_placements;
typedef struct {gh_bundle_entry file,entries;uint32_t frontier,count,version,cursor,skipped,created;uint8_t native_bound;} gh_om_scene_placements;
typedef struct {const uint8_t *bytes;size_t size;uint32_t index,type;uint64_t key,extra_key;uint32_t version;} gh_om_placement_record;
struct gh_om_factory;

typedef struct {const uint8_t *records;size_t record_bytes;uint8_t scalar04[0x2c];uint32_t field30,frontier;} gh_acd_scene_binding;
typedef enum {GH_PLACE_OK,GH_PLACE_ARGUMENT,GH_PLACE_BOUNDS,GH_PLACE_UNSUPPORTED,GH_PLACE_MEMORY,GH_PLACE_BUSY} gh_scene_placement_status;
/* Actual294B38/295600; resetsfiles, conditionally48scratchbytes/roomFFFF. */
gh_scene_placement_status gh_em_scene_room_init_00294b38(gh_em_scene_placements *,const gh_boot_work *,uint8_t scenario14);
/* Actual294C68/294B98 emptyEMS path using canonicalroom.active. Positive
 * entries/alreadyloadedsetEmAll remain preciseunsupported, never fakeactors. */
gh_scene_placement_status gh_em_scene_bind_00294c68(gh_em_scene_placements *,gh_room_state *,const gh_bundle_entry *ems,int *original_load_return);
/* Explicit native numerical reference for original MUL.S/CVT.W.S. EXACT
 * refuses unverified fresh-positive conversion; this is not hardware proof. */
typedef enum {GH_EM_NUMERIC_EXACT=0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE=1} gh_em_numeric_policy;
/* Actual2BEDE8: reserved64*24B RoomSave span, byte8count. Host bound/policy
 * errors preserve page; floats8/9 and bytes28..2B copy without conversion. */
gh_scene_placement_status gh_em_room_table_write_002bede8(gh_room_state *,uint32_t count,const gh_bundle_entry *records,gh_em_numeric_policy,uint32_t *frontier);
/* Integer-significand reference; angle selects factor57.295776(42652EE0),
 * otherwise20(41A00000). Bounded |exactproduct|<2^24; output is originalSH low16. */
gh_scene_placement_status gh_em_quantize_chopzero_reference(uint32_t source_bits,int angle,uint16_t *out,uint32_t *frontier);
/* Actual295600 only; does not clear file/source views (294B38 does). */
gh_scene_placement_status gh_em_scene_reset_room_00295600(gh_em_scene_placements *,const gh_boot_work *,uint8_t scenario14);
/* Actual294B98 table-load primitive; original bool separated from status. */
gh_scene_placement_status gh_em_scene_load_native_00294b98(gh_em_scene_placements *,gh_room_state *,const gh_bundle_entry *,const gh_boot_work *,uint8_t scenario14,gh_em_numeric_policy,int *original_return);
/* Actual294C68 always invokes294CA0 after load bool1, including FRESH positive.
 * Only stop_flag==1 clear is restored; spawning path remains precise294CA0. */
gh_scene_placement_status gh_em_scene_bind_native_00294c68(gh_em_scene_placements *,gh_room_state *,const gh_bundle_entry *,const gh_boot_work *,uint8_t scenario14,gh_em_numeric_policy,int *original_load_return);
void gh_om_scene_construct_002a7308(gh_om_scene_placements *);
/* Actual1C3CB8/1C3C58 missing/invalid/emptyOMS; originalreturn0 is explicit. */
gh_scene_placement_status gh_om_scene_bind_001c3cb8(gh_om_scene_placements *,const gh_bundle_entry *oms,int *original_return);
/* Actual2BED08 reads SAME currentroom page, not a second activationtable. */
gh_scene_placement_status gh_om_room_slot_used_002bed08(const gh_room_state *,uint64_t key,int *original_return);
gh_scene_placement_status gh_om_scene_record_at(const gh_om_scene_placements *,uint32_t,gh_om_placement_record *);
/* Stateful positiveOMS original loop. Completedrecords never repeat onretry. */
gh_scene_placement_status gh_om_scene_bind_native_001c3cb8(gh_om_scene_placements *,const gh_bundle_entry *,struct gh_om_factory *,int *original_return);
/* Actual137AE8/137AA0, scalarheadercopy andboundednativeACDrecordview. */
gh_scene_placement_status gh_acd_scene_bind_00137ae8(gh_acd_scene_binding *,const gh_bundle_entry *acd,int *original_return);
#ifdef __cplusplus
}
#endif
#endif
