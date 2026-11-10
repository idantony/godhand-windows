#ifndef GH_GAME_OM_DAMAGE_H
#define GH_GAME_OM_DAMAGE_H
#include "game_hitarea_frame.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_OM_DAMAGE_OK,GH_OM_DAMAGE_ARGUMENT,GH_OM_DAMAGE_UNSUPPORTED,GH_OM_DAMAGE_NUMERIC} gh_om_damage_status;
typedef struct {const uint8_t *actor_bytes;gh_hitarea_object hit;
 uint32_t phase,frontier,damage_bits;float direction[3];int original_hit;
 uint8_t bound,has_hit;} gh_om_damage_call;
/* Actual1B7568 hit46/sourcekind/power classification, SAMEtypedlastsource34.
 * Original OR writes are retained. This helper does not acknowledge damage. */
gh_om_damage_status gh_om_classify_damage_001b7568(const gh_title_player_view *,const gh_hitarea_object *,uint32_t *frontier);
/* One original1B7380 invocation; preserve cursor after errors, clear only for
 * next call. NULLhit must be the caller's known originalNULL. double_damage
 * borrows canonical originalbyte5CB010; NULL means unknown, never default0.
 * power0 is independent of all256bytevalues, so only that numeric branch may
 * proceed without the byte. HP/flash/force still execute. Binary32 reference
 * only; no FEnv changes survive, no host signedoverflow/extraHPclamping. */
gh_om_damage_status gh_om_check_damage_001b7380(gh_om_damage_call *,const gh_title_player_view *,const gh_hitarea_object *,const uint8_t *double_damage,gh_static_transform_policy,int *original_hit,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
