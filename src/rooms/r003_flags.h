#ifndef GH_R003_FLAGS_H
#define GH_R003_FLAGS_H

#include "r003_event.h"
#include "r100_init.h"
#include "../core/frame_state.h"
#include "../core/game_om_factory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* R003 REL 3D8 and R105 REL 10F8 use the same six authored flags and wind step. */
gh_r003_status gh_room_move_flag_wind(gh_boot_work *work,void *wind_words,
    const gh_om_factory *,gh_game_mt *,gh_frame_random *,gh_r100_om_scalar_view,void *,
    uint32_t relative,uint32_t *frontier);

/* Original REL+228 calls REL+3D8 once per room Move. Both random streams
 * are the shared game owners. The three wind scalars belong to R003Init. */
gh_r003_status gh_r003_move_flags_000003d8(gh_r003_event *,const gh_om_factory *,
    gh_game_mt *,gh_frame_random *,gh_r100_om_scalar_view other_view,void *other_context,
    uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
