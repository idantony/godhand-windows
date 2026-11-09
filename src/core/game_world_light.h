#ifndef GH_GAME_WORLD_LIGHT_H
#define GH_GAME_WORLD_LIGHT_H
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original cWorldLight scalar storage. Guestpointer fields remain zero tokens;
 * corresponding native resourceviews below carry the actual bounded pointers.
 * No caller may cast rawbytes into a hostpointer-bearing upstream type. */
typedef struct {uint8_t scalar[0x16290];gh_bundle_entry slots[8],area,table;
 uint8_t constructed,ready;uint32_t frontier;int detail;} gh_game_world_light;
typedef enum {GH_LIGHT_OK,GH_LIGHT_ARGUMENT,GH_LIGHT_BOUNDS,GH_LIGHT_UNSUPPORTED} gh_light_status;
/* ActualcoldPT_LOAD BSS plus2D6D80 constructor VF0=(0,0,0,1) writes.
 * Initialize nativeowner tozero; no arbitrarymidgame reset/reconstruction. */
gh_light_status gh_game_world_light_construct_002d6d80(gh_game_world_light *);
/* Actual2D6F40 /2D6A60 LI33-present zero-lightrecords path, trueR020 domain.
 * Unknownnonzero-light/default branch remains explicitunsupported. */
gh_light_status gh_game_world_light_initialize_002d6f40(gh_game_world_light *,const gh_bundle *room);
#ifdef __cplusplus
}
#endif
#endif
