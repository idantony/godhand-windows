#ifndef GH_GAME_COLLISION_SOLIDS_H
#define GH_GAME_COLLISION_SOLIDS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Canonical original00462FC0 manager plus00460D18 resetcounter. Originalhead becomes nativepointer; words08 retain32bitopaque values
 * until their consumers are restored. This is NOT gueststructlayout.
 * Reset doesnotdestroy referenced objects orclaimcollisionqueries. */
struct gh_collision_solid_unit;
typedef struct {void *unit_head;uint32_t unit_count,words08[32],field88;
 uint32_t global_00460d18;struct gh_collision_solid_unit *native_units;} gh_game_collision_solids;
/* Actual134690, explicitsharedowner, noallocation or emptyplatformcallback. */
void gh_game_collision_solids_reset_00134690(gh_game_collision_solids *);
#ifdef __cplusplus
}
#endif
#endif
