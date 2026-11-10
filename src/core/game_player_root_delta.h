#ifndef GH_GAME_PLAYER_ROOT_DELTA_H
#define GH_GAME_PLAYER_ROOT_DELTA_H
#include "game_title_player.h"
#include "boot.h"
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PLAYER_DELTA_OK,GH_PLAYER_DELTA_ARGUMENT,GH_PLAYER_DELTA_NUMERIC,GH_PLAYER_DELTA_UNSUPPORTED} gh_player_delta_status;
/* Actual SDK-rootdelta leaves with original f12=1 call profile. Work5E4
 * bit20000000 gates each separately. Output XYZ commits after finite math;
 * no fabricated velocity, no changes to positionW or EulerW. */
gh_player_delta_status gh_player_root_rotation_0014f9f0(const gh_title_player_view *,const gh_boot_work *,gh_static_transform_policy,uint32_t *frontier);
gh_player_delta_status gh_player_root_translation_0014f928(const gh_title_player_view *,const gh_boot_work *,gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
