#include "game_event_wait_vib.h"

#include <string.h>

_Static_assert(offsetof(gh_boot_work, pad570) == 0x570, "original event frame word");

gh_game_event_status gh_event_set_frame_0030d168(gh_boot_work *work, int32_t frame) {
    if (!work) return GH_EVENT_ARGUMENT;
    if (frame < 2) frame = 2;
    memcpy(work->pad570, &frame, sizeof(frame));
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_wait_vib_visit(gh_event_components *components,
    const gh_event_data_prefix *data, gh_boot_work *work, uint32_t site, uint32_t *frontier, int *relative) {
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = site;
    *relative = 0;
    if (!components || !components->constructed || !data || data->components != components || !work) {
        return GH_EVENT_ARGUMENT;
    }

    const int vibration = site == 0x296d8c || site == 0x296e9c;
    const int init = site == 0x296d8c || site == 0x296d98;
    if (!vibration && site != 0x296d98 && site != 0x296ea8) return GH_EVENT_ARGUMENT;
    const unsigned component = vibration ? 11 : 12;
    const uint32_t bind_site = vibration ? 0x29797c : 0x297998;
    gh_event_component_view view;
    if (gh_event_component_at(components, component, &view) || !view.bytes ||
        view.original_offset != (vibration ? 0xda20u : 0xda40u) || view.size != (vibration ? 0x1cu : 0x18u)) {
        return GH_EVENT_ARGUMENT;
    }
    gh_event_sequence_view track;
    gh_game_event_status status = gh_event_sequence_at(data, component, &track);
    if (status) {
        *frontier = bind_site;
        return status;
    }
    uint32_t count;
    memcpy(&count, view.bytes + 0x14, sizeof(count));
    if (track.count != count) {
        *frontier = bind_site;
        return GH_EVENT_BOUNDS;
    }
    if (track.count || track.records || track.resource_offset || track.bytes || track.stride) {
        *relative = 1;
        *frontier = vibration ? (init ? 0xaed8 : 0xad48) : (init ? 0xaaf0 : 0xa960);
        return GH_EVENT_UNSUPPORTED;
    }

    if (init) {
        if (vibration) memcpy(view.bytes, "VIB", 4); // REL09CD8 -> strcpy3A57C4.
        else memcpy(view.bytes, "WAIT", 5);
    }
    // Empty A398 cannot select a row. A738 takes the real fallback.
    if (!vibration) gh_event_set_frame_0030d168(work, 2);
    *frontier = 0;
    return GH_EVENT_OK;
}
