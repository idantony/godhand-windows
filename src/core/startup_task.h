#ifndef GH_STARTUP_TASK_H
#define GH_STARTUP_TASK_H
#include "boot.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Host objects replace guest pointers at work+554/+558, never truncated into
 * boot_work. Lifetime belongs to the DVD allocator. */
typedef struct gh_startup_task {
    gh_boot_work *work;
    void *core_data, *language_data;
    int error;
    unsigned failed_operation;
} gh_startup_task;
typedef enum {
    GH_START_SOUND, GH_START_READ, GH_START_WAIT, GH_START_FONT,
    GH_START_MENU, GH_START_BIND_ID, GH_START_FIND_ID,
    GH_START_INIT_ID, GH_START_REGISTER, GH_START_OP_COUNT
} gh_startup_operation;
typedef struct gh_startup_platform {
    void *context;
    /* 2CA798: actual startup-bank work, original buffer at 5FEE00. */
    int (*sound_startup)(void *);
    /* 200CB0: allocate/read through original DVD object (583F20), heap 754200;
     * remaining four original optional parameters are zero. No fake job ids. */
    int (*read_alloc)(void *, const char *path, void **destination, int32_t *job);
    /* 201108: waits on job using native task sleep as needed, not busy polling.
     * Callee return register is ignored by original caller; host errors separate. */
    int (*wait_frame)(void *, int32_t job);
    int (*bind_fonts)(void *, gh_startup_task *); /* 2A4970 */
    int (*load_menu_tables)(void *, gh_startup_task *); /* 2AE990 */
    int (*bind_id_global)(void *, gh_startup_task *); /* *[3C2384]=[3C2388] */
    int (*find_id_data)(void *, void *language_data, const char *name,
                        int32_t index, void **result); /* SearchData 31CCF0 */
    int (*init_id_system)(void *, void *data); /* 2ACC70 */
    /* 2D5470: must map the named original entry to real recovered native code;
     * unavailable entries must fail, never register a successful empty body. */
    int (*register_task)(void *, uint32_t entry_address, int32_t slot);
} gh_startup_platform;
/* Original 30D358 task body. Returns 0 only after full body completion.
 * Wrapper's normal return implements original final exit(752C04); do not
 * continue as completed if error!=0. Partial state is retained, no rollback.
 * All required callbacks are checked before mutations; backend is not supplied
 * here, so this body cannot itself claim startup assets/audio are connected. */
int gh_startup_task_0030d358(gh_startup_task *, const gh_startup_platform *);
#ifdef __cplusplus
}
#endif
#endif
