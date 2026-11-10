#ifndef GH_GAME_HITAREA_OBJECTS_H
#define GH_GAME_HITAREA_OBJECTS_H
#include "game_init.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_HITAREA_OK,GH_HITAREA_ARGUMENT,GH_HITAREA_BOUNDS,GH_HITAREA_MEMORY,GH_HITAREA_UNSUPPORTED} gh_hitarea_status;
/* Borrow SAME574380 object_pool; A160*60, B256*70, plus originalinactive
 * fallbackAC00/AC60. Generation rejects views after realreset; originalraw
 * occupancy4A is authoritative, no extra allocation/occupancy shadow. */
typedef struct {gh_game_init_state *state;uint8_t *bytes;size_t size;
 uint32_t generation;uint16_t index;uint8_t group,fallback;} gh_hitarea_object;
gh_hitarea_status gh_hitarea_object_create_001fe218(gh_game_init_state *,gh_title_player_view *,uint8_t mode,gh_hitarea_object *,int *original_nonnull,uint32_t *frontier);
gh_hitarea_status gh_hitarea_object_create_001fe0e8(gh_game_init_state *,gh_title_player_view *,uint8_t mode,gh_hitarea_object *,int *original_nonnull,uint32_t *frontier);
gh_hitarea_status gh_hitarea_object_actor(const gh_hitarea_object *,gh_title_player_view **);
gh_hitarea_status gh_hitarea_object_set_mode_001fd988(const gh_hitarea_object *,uint8_t mode,uint32_t *frontier);
/* Host metadata only, invoked AFTER originalpool reset/release actually
 * succeeded. Does not release originaldamage nodes or change rawoccupancy. */
void gh_hitarea_detach_after_pool_reset(gh_game_init_state *);
#ifdef __cplusplus
}
#endif
#endif
