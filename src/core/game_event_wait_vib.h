#ifndef GH_GAME_EVENT_WAIT_VIB_H
#define GH_GAME_EVENT_WAIT_VIB_H

#include "game_event_data_prefix.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Original 30D168 writes the same Work+570 clock divisor, with minimum two. */
gh_game_event_status gh_event_set_frame_0030d168(gh_boot_work *, int32_t);
/* Four original PLAY visits for the proven absent EV01 Wait/Vib tracks.
 * On failure, relative distinguishes a REL offset from a main-ELF site. */
gh_game_event_status gh_event_wait_vib_visit(gh_event_components *, const gh_event_data_prefix *,
    gh_boot_work *, uint32_t original_site, uint32_t *frontier, int *relative);

#ifdef __cplusplus
}
#endif
#endif
