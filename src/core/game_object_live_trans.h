#ifndef GH_GAME_OBJECT_LIVE_TRANS_H
#define GH_GAME_OBJECT_LIVE_TRANS_H
#include "game_object_step.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Appends the real nonstatic30F650 lane to the SAME trans cursor. Native
 * callbacks are required for actualVT+14/+1C/+0C bodies. No guestcode executes.
 * Destructor callback owns only its concrete destructor; it must not remove
 * the globalregistry binding, which the original caller removes afterward. */
gh_object_step_status gh_game_object_step_bind_trans(gh_game_object_step *,const gh_object_trans_bindings *);
/* Internal typed hook used by the canonical static/mixed traversal. */
int gh_object_live_trans_handle(const gh_room_pools *,uint32_t handle);
gh_object_step_status gh_object_live_trans_one_0030f650(gh_game_object_step *,const gh_room_pools *,const gh_boot_work *,uint32_t handle,int *erased,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
