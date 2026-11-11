#ifndef GH_R100_WATCH_GEOMETRY_H
#define GH_R100_WATCH_GEOMETRY_H
#include "../core/game_ray_query.h"
#include "r100_scripts.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original REL2920 condition after both live GetPos publishers. Input Y
 * is unraised. Payload is selected for outside cone or blocked sight. */
gh_r100_script_status gh_r100_at1_geometry(const void *elf,size_t elf_size,
 gh_game_scene_tables *,const float player[4],const float enemy[4],float player_yaw,
 int *payload,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
