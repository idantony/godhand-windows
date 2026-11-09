#ifndef GODHAND_CORE_MENU_STATE_H
#define GODHAND_CORE_MENU_STATE_H
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum gh_menu_error {
    GH_MENU_OK,GH_MENU_ARGUMENT,GH_MENU_BOUNDS,GH_MENU_MISSING,GH_MENU_UNSUPPORTED
} gh_menu_error;
/* Host views replace the original pointers at menu+0 and +0xC. Tables remain
 * opaque immutable data, not host callback arrays. Their source bundle must
 * outlive this state. Timer/state map to original+0xBDA/+0xBF0. */
typedef struct gh_menu_state {
    gh_bundle_entry data,texture_data;
    int16_t timer;
    uint8_t state;
    uint8_t tables_bound;
} gh_menu_state;
typedef struct gh_id_pair {
    gh_bundle_entry data,texture_data;
    uint8_t bound;
} gh_id_pair;
/* Native decoded object, NOT a memcpy-compatible guest object. Pending jobs
 * and pool are preserved opaque bytes until their consumers are recovered.
 * No pointer-bearing guest job is executed by treating it as a host record. */
typedef struct gh_id_state {
    gh_id_pair pair[32];
    uint8_t jobs[0x640]; /* original+0x100, 100 records of0x10 */
    uint32_t job_count; /* original+0x740 */
    uint8_t pool[0x1ae00]; /* original+0x744, untouched by systemInit */
    uint32_t pool_used; /* original+0x1b544 */
    uint8_t initialized;
} gh_id_state;

/* 002AE990: MSD occurrence0/1 from language bundle, timer900, state0. */
gh_menu_error gh_menu_load_tables_002ae990(gh_menu_state *,const gh_bundle *language);
/* SearchData language,"IDD",0, bounded view for the actual init argument. */
gh_menu_error gh_id_find_language_data(const gh_bundle *,gh_bundle_entry *);
/* 002ACC70/002ACCD8/002ACCF8: clear jobs/count/pair views, preserve pool bytes,
 * reset pool cursor, bind IDD's TBL0/TBL1 into slot1. No GPU upload or UI jobs.
 * Missing/malformed required tables report errors, never pretend empty success.
 * On failure the original clear and any earlier bindings remain visible. */
gh_menu_error gh_id_system_init_002acc70(gh_id_state *,const gh_bundle_entry *idd);
/* 002ACCF8 for later real ID resources, including MC idsavecore slot0xB.
 * Preserves all other pair slots, jobs, pool and counters. */
gh_menu_error gh_id_set_data_002accf8(gh_id_state *,uint32_t slot,const gh_bundle_entry *);
/* Host equivalent of *[D_003C2384]=[D_003C2388]. Can precede systemInit exactly
 * as the startup task does; this binds identity, not initialized readiness. */
gh_menu_error gh_id_bind_global(gh_id_state **destination,gh_id_state *manager);
#ifdef __cplusplus
}
#endif
#endif
