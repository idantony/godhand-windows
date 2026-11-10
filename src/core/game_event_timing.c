#include "game_event_timing.h"

#include <string.h>

_Static_assert(sizeof(gh_event_cut_timing) == 16, "original cut timing row");
_Static_assert(offsetof(gh_event_cut_timing, frames_seen) == 8, "cut frame counter");
_Static_assert(offsetof(gh_event_cut_timing, start_frame) == 12, "cut start frame");

static uint32_t read_word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

gh_game_event_status gh_event_timing_prepare_000054c0(
    gh_event_timing *timing, gh_event_components *components, gh_heap *heap,
    int *original_return)
{
    if (!original_return) return GH_EVENT_ARGUMENT;
    *original_return = 0;
    gh_event_component_view env;
    if (!timing || !heap || !components || !components->constructed ||
        gh_event_component_at(components, 0, &env) ||
        !env.bytes || env.original_offset != 0xd200 || env.size != 0x44)
        return GH_EVENT_ARGUMENT;

    /* 297810 writes the zero-extended cut count from the event header. */
    uint32_t count = read_word(env.bytes + 8);
    if (count > UINT8_MAX) return GH_EVENT_BOUNDS;

    if (timing->allocation.pointer) {
        if (timing->components != components || timing->heap != heap ||
            timing->event_number != components->event_no || timing->count != count)
            return GH_EVENT_ARGUMENT;
        *original_return = 1;
        return GH_EVENT_OK;
    }
    if (!count) return GH_EVENT_OK;

    gh_r207_allocation allocation = {0};
    size_t bytes = count * sizeof(gh_event_cut_timing);
    gh_mem_error status = gh_heap_allocate(heap, bytes, 16, &allocation);
    if (status == GH_MEM_CAPACITY || status == GH_MEM_SYSTEM) return GH_EVENT_OK;
    if (status) return GH_EVENT_CALLEE;

    gh_event_cut_timing *cuts = allocation.pointer;
    memset(cuts, 0, bytes);
    for (uint32_t cut = 0; cut < count; ++cut)
        cuts[cut].next_cut = (uint8_t)(cut + 1);
    cuts[count - 1].next_cut = UINT8_MAX;

    *timing = (gh_event_timing){heap, allocation, components, count, components->event_no};
    *original_return = 1;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_timing_abort(gh_event_timing *timing)
{
    if (!timing) return GH_EVENT_ARGUMENT;
    if (timing->allocation.pointer &&
        gh_heap_release(timing->heap, timing->allocation) != GH_MEM_OK)
        return GH_EVENT_CALLEE;
    memset(timing, 0, sizeof(*timing));
    return GH_EVENT_OK;
}
