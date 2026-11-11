#ifndef GH_GAME_ROOM_STATE_H
#define GH_GAME_ROOM_STATE_H
#include "game_prepare.h"
#include "../platform/task_threads.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {int16_t room,save_slot;uint32_t flags,original_address;} gh_room_record;
typedef struct {const gh_room_record *record;uint8_t *active;size_t size;uint32_t native_handle;} gh_room_state;
typedef enum {GH_ROOM_OK,GH_ROOM_ARGUMENT,GH_ROOM_BOUNDS,GH_ROOM_UNSUPPORTED,GH_ROOM_THREAD} gh_room_status;
/* Actual3C26C0 source table, including sentinel. Three code-hook words are
 * verifiedzero in all112originalrecords, not guest callbacks to execute. */
gh_room_status gh_room_record_at(unsigned,gh_room_record *);
/* Actual2BEBC8 chooses one of seven persistent1650-byte banks or the self+8
 * transient bank. Writes canonical game_prepare.fields, never a shadow copy.
 * Original pointerword becomes an owner-local native handle; active is typed. */
gh_room_status gh_room_state_select_002bebc8(gh_game_prepare *,int16_t room,uint32_t work_flags,gh_room_state *);
/* Mark a destroyed object in the selected room's existing 256-slot table.
 * The original leaves a full table unchanged and allows duplicate keys. */
gh_room_status gh_room_state_mark_object_broken_002becb0(const gh_room_state *,uint64_t key);
/* Actual2C32F0: real killAll, clear16attrs, Work+5E4 bit40000000 clear,
 * select table record and roomstate. Caller supplies originalScenario manager
 * storage >=16records; this API does not invent its earlierboot allocation.
 * Active tasks require the matching native backend; unsupported never succeeds. */
gh_room_status gh_scenario_reset_002c32f0(gh_game_prepare *,gh_boot_work *,
 gh_task_manager *,gh_task_threads *,gh_room_state *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
