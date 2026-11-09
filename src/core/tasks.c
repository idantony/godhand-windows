/* Copyright (c) 2026 Lucas Picoli and god-hand-decomp contributors.
 * MIT notice in LICENSE.upstream. Reconstructed functions from upstream
 * eaae18d0dfab1f9a0df276ae5ca1581ad326faf3; actual patched ELF inspected.
 * This ports slot selection, attr inheritance and filtered visitation only.
 * It does not invent replacements for EE thread start/sleep/termination.
 */
#include "tasks.h"
_Static_assert(sizeof(gh_task_record32)==0x58,"PS2 record stride");
_Static_assert(offsetof(gh_task_record32,thread_id)==4,"thread id");
_Static_assert(offsetof(gh_task_record32,running)==0xc,"running field");
_Static_assert(offsetof(gh_task_record32,attributes)==0x50,"attr field");

static int valid(const gh_task_manager *m)
{
    return m && m->count <= m->capacity && (!m->count || m->records);
}

gh_task_selection gh_tasks_select_002d5b30(const gh_task_manager *m, int32_t request)
{
    gh_task_selection result={GH_TASK_OK,-1};
    int32_t i;
    if (!valid(m)) { result.status=GH_TASK_INVALID_STATE; return result; }
    if (request < 0) {
        for(i=2;i<m->count;++i)
            if (m->records[i].running==0) { result.index=i; break; }
    } else if (request < m->count && m->records[request].running==0)
        result.index=request;
    return result;
}

gh_task_status gh_tasks_inherit_002d5440(gh_task_manager *m, int32_t index)
{
    if (!valid(m)) return GH_TASK_INVALID_STATE;
    if (!m->current || index==-1) return GH_TASK_OK;
    /* Valid original callers use an allocator-returned index. Enforce the
     * decoded host bound rather than allowing arbitrary guest memory writes. */
    if (index<0 || index>=m->count) return GH_TASK_INVALID_STATE;
    m->records[index].attributes=m->current->attributes;
    return GH_TASK_OK;
}

gh_task_run_result gh_tasks_run_matching_002d52a0(gh_task_manager *m)
{
    gh_task_run_result result={GH_TASK_OK,0};
    uint32_t i=0;
    gh_task_record32 *base;
    size_t capacity;
    if (!valid(m)) { result.status=GH_TASK_INVALID_STATE; return result; }
    if (m->count && !m->step) { result.status=GH_TASK_MISSING_STEP; return result; }
    base=m->records; capacity=m->capacity;
    while(i<m->count) {
        gh_task_record32 *work;
        uint32_t flags;
        if (!valid(m) || m->records!=base || m->capacity!=capacity) {
            result.status=GH_TASK_INVALID_STATE; return result;
        }
        work=&m->records[i];
        flags=m->flags;
        m->current_index=(int32_t)i;
        m->current=work;
        if (!(flags&1) || (work->attributes&1)) {
            if (!(flags&2) || (work->attributes&2)) {
                result.platform_error=m->step(m->step_context,m,work);
                if (result.platform_error) { result.status=GH_TASK_STEP_FAILED; return result; }
            }
        }
        ++i;
    }
    m->current=0;
    m->current_index=-1;
    return result;
}
