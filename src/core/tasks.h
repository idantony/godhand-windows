/* Reconstructed God Hand task-manager slice. MIT: LICENSE.upstream. */
#ifndef GODHAND_CORE_TASKS_H
#define GODHAND_CORE_TASKS_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Guest record layout. Addresses remain tokens, never native pointers. */
typedef struct gh_task_record32 {
    uint32_t owner_address;
    int32_t thread_id;
    uint32_t unknown08;
    uint16_t running;
    uint8_t unknown0e[0x42];
    uint8_t attributes;
    uint8_t unknown51[7];
} gh_task_record32;

typedef struct gh_task_manager gh_task_manager;
/* Replacement boundary for cTaskWork_step at 0x002D57C8, not the task entry
 * itself. Must implement the real step/thread semantics before game integration.
 * Return zero after performing the step or an explicit host error. */
typedef int (*gh_task_step_fn)(void *, gh_task_manager *, gh_task_record32 *);
struct gh_task_manager {
    gh_task_record32 *records;
    size_t capacity;       /* host memory bound; not an original scheduling field */
    gh_task_record32 *current;
    int32_t current_index;
    uint16_t count;        /* original manager +0x0C */
    uint32_t flags;        /* original manager +0x2C */
    gh_task_step_fn step;
    void *step_context;
};
typedef enum gh_task_status {
    GH_TASK_OK, GH_TASK_INVALID_STATE, GH_TASK_MISSING_STEP, GH_TASK_STEP_FAILED
} gh_task_status;
typedef struct gh_task_selection {
    gh_task_status status;
    int32_t index; /* original -1 means no slot, with status OK */
} gh_task_selection;
typedef struct gh_task_run_result {
    gh_task_status status;
    int platform_error;
} gh_task_run_result;

/* 002D5B30: choose only, do not reserve/start. Any negative request scans from
 * slot 2; explicit slot 0/1 is allowed. Nonzero running means occupied. */
gh_task_selection gh_tasks_select_002d5b30(const gh_task_manager *, int32_t request);
/* 002D5440: when current exists and index != -1, copy its attr to that slot. */
gh_task_status gh_tasks_inherit_002d5440(gh_task_manager *, int32_t index);
/* 002D52A0: increasing slot order; flags bits 0/1 require corresponding attr
 * bits. Does not test running. Reloads flags/count per original loop behavior.
 * On success current=NULL/current_index=-1, including count==0. Callback errors
 * retain current/index at the failed slot, distinct from original game returns.
 * Callbacks may change flags/count/record values within capacity, but must not
 * replace/free records, change capacity or replace the step callback during
 * traversal. Not thread-safe. */
gh_task_run_result gh_tasks_run_matching_002d52a0(gh_task_manager *);
#ifdef __cplusplus
}
#endif
#endif
