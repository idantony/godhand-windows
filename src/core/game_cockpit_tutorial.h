#ifndef GH_GAME_COCKPIT_TUTORIAL_H
#define GH_GAME_COCKPIT_TUTORIAL_H
#include "game_cockpit.h"
#include "dvd_jobs.h"
#include "packed_message.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_TUTORIAL_OK,GH_TUTORIAL_ARGUMENT,GH_TUTORIAL_BUSY,GH_TUTORIAL_DVD,GH_TUTORIAL_MEMORY,GH_TUTORIAL_ID} gh_tutorial_status;
typedef struct {gh_game_cockpit *cockpit;gh_dvd_jobs *dvd;gh_heap *room_heap;
 gh_id_state *id;gh_font_state *fonts;gh_packed_message_state *packed;
 const gh_bundle *language;const gh_boot_work *work;} gh_tutorial_bindings;
typedef struct {gh_tutorial_bindings b;gh_r207_allocation resource;gh_bundle_entry image;
 char path[128];uint32_t job,stage,frontier;gh_id_error id_error;int packed_return;
 uint8_t bound,pair_bound,complete;uint64_t completed_calls;} gh_cockpit_tutorial;
/* Original147028 on the SAME Cockpit+1A20. Uses real DVD CheckWait201160
 * (pumps real jobs; no fabricated frameyield), slot1D, ID Base, packed fonts,
 * seven native refs and actual scalar/entry mutations. Idempotent retries of
 * one original call; next_call explicitly denotes another original call.
 * Resource/language/manager/runtime outlive all consumers and source views. */
gh_tutorial_status gh_cockpit_tutorial_room_init_00147028(gh_cockpit_tutorial *,const gh_tutorial_bindings *);
gh_tutorial_status gh_cockpit_tutorial_next_call(gh_cockpit_tutorial *);
/* Native teardown after Main/queued consumers stop, BEFORE roomheap free or
 * ID compaction. Original147188 itself only resets B0/AC/B8. Clears only
 * matching source-backed views, detaches owned Base then frees owned bytes. */
gh_tutorial_status gh_cockpit_tutorial_release(gh_cockpit_tutorial *);
#ifdef __cplusplus
}
#endif
#endif
