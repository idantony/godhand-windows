#ifndef GH_GAME_WORLD_LIGHT_H
#define GH_GAME_WORLD_LIGHT_H
#include "../assets/core_bundle.h"
#include "../platform/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original cWorldLight scalar storage. Guestpointer fields remain zero tokens;
 * corresponding native resourceviews below carry the actual bounded pointers.
 * No caller may cast rawbytes into a hostpointer-bearing upstream type. */
typedef struct {uint8_t scalar[0x16290];gh_bundle_entry slots[8],area,table;
 uint8_t constructed,ready;uint32_t frontier;int detail;
 gh_heap *record_heap;gh_r207_allocation preset_records[8];size_t preset_record_capacity[8];const void *native_light_owners[256];} gh_game_world_light;
typedef enum {GH_LIGHT_OK,GH_LIGHT_ARGUMENT,GH_LIGHT_BOUNDS,GH_LIGHT_UNSUPPORTED,GH_LIGHT_MEMORY} gh_light_status;
/* ActualcoldPT_LOAD BSS plus2D6D80 constructor VF0=(0,0,0,1) writes.
 * Initialize nativeowner tozero; no arbitrarymidgame reset/reconstruction. */
gh_light_status gh_game_world_light_construct_002d6d80(gh_game_world_light *);
/* Actual2D6F40 /2D6A60 LI33-present zero-lightrecords path, trueR020 domain.
 * Unknownnonzero-light/default branch remains explicitunsupported. */
gh_light_status gh_game_world_light_initialize_002d6f40(gh_game_world_light *,const gh_bundle *room);
/* Actual positiveLI33 allocation branch30EE08 uses the caller's SAME heap.
 * Preset guestpointer74 remains0; native allocations carry real host pointers.
 * Native cleanup frees onlytheseownedrecords, before canonicalheap destruction. */
gh_light_status gh_game_world_light_initialize_with_heap_002d6f40(gh_game_world_light *,const gh_bundle *,gh_heap *);
gh_light_status gh_game_world_light_release_presets(gh_game_world_light *);
gh_light_status gh_game_world_light_preset_records(const gh_game_world_light *,unsigned,const uint8_t **,size_t *count);
/* Actual Getplayer parts input: owneridentity is required when the source
 * light requests a player attachment; zero parts is legitimate onlywith a
 * real known owner. Native part references never enter guestpointerwords. */
typedef struct {const void *owner;const void *const *parts;size_t count;} gh_light_player_parts;
gh_light_status gh_game_world_light_select_002d8408(gh_game_world_light *,unsigned,const gh_light_player_parts *);
#ifdef __cplusplus
}
#endif
#endif
