#ifndef GH_GAME_ACTOR_AUX_H
#define GH_GAME_ACTOR_AUX_H
#include "game_actor_model_nodes.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual14A3E8 auxiliary tail, kind1 thenkind0, requested parity. Only the
 * explicit1495A0 constructor-empty domain is restored. No packet emitted for
 * genuinely NULL refs; unknown/positive refs never become a queue success.
 * outputs unchanged on failure. No OT or GPU completion acknowledgment. */
gh_actor_model_status gh_actor_auxiliary_0014a3e8(const gh_actor_model *,unsigned parity,unsigned *original_packets,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
