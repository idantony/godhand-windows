#ifndef GH_GAME_SCENE_SPAWNS_H
#define GH_GAME_SCENE_SPAWNS_H
#include "game_init.h"
#ifdef __cplusplus
extern "C" {
#endif
/* ExactnextCreateObj inputs andsaveditemdata; no fabricatednativeobject. */
typedef struct {uint32_t index;uint16_t stage,kind,serial,extra;
 uint8_t flags;int8_t yaw_half_degrees;int16_t position_tenths[3];} gh_saved_item_spawn;
typedef enum {GH_SPAWN_OK,GH_SPAWN_ARGUMENT,GH_SPAWN_BOUNDS,GH_SPAWN_UNSUPPORTED} gh_saved_item_spawn_status;
/* Actual1FB330: roomhighbyte0 (R020) genuinely returnsbeforeCoreSaveaccess.
 * Otherrooms withnonnullsave scan128records (notitemNum). A matchingrecord
 * returnsprecise30FA98 frontier plusrequest; no CreateObj/virtualInit/position
 * success is fabricated. Callermustrestorethoseconsumers beforecontinuing. */
gh_saved_item_spawn_status gh_core_save_spawn_items_001fb330(gh_core_save_view,
 uint16_t room,gh_saved_item_spawn *pending,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
